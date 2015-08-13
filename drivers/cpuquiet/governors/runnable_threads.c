/*
 * Copyright (c) 2012-2013 NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/cpuquiet.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/sched.h>

// from cpuquiet.c
extern unsigned int cpq_max_cpus(void);
extern unsigned int cpq_min_cpus(void);
// from cpuquiet_driver.c
extern unsigned int best_core_to_turn_up (void);
//from core.c
extern unsigned long avg_nr_running(void);
extern unsigned long avg_cpu_nr_running(unsigned int cpu);

// from sysfs.c
extern unsigned int gov_enabled;

typedef enum {
	DISABLED,
	IDLE,
	RUNNING,
} RUNNABLES_STATE;

static struct work_struct runnables_work;
static struct kobject *runnables_kobject;
static struct timer_list runnables_timer;

static RUNNABLES_STATE runnables_state;
/* configurable parameters */
static unsigned int sample_rate = 20;		/* msec */

#define NR_FSHIFT_EXP	3
#define NR_FSHIFT	(1 << NR_FSHIFT_EXP)

static unsigned int nr_run_last;
static unsigned int nr_run_hysteresis = 2;		/* 1 / 2 thread */
/* avg run threads * 8 (e.g., 11 = 1.375 threads) */
static unsigned int nr_run_thresholds[] = {
	10, 18, 20, UINT_MAX
};

DEFINE_MUTEX(runnables_lock);

struct runnables_avg_sample {
	u64 previous_integral;
	unsigned int avg;
	bool integral_sampled;
	u64 prev_timestamp;
};

static DEFINE_PER_CPU(struct runnables_avg_sample, avg_nr_sample);

/* EXP = alpha in the exponential moving average.
 * Alpha = e ^ (-sample_rate / window_size) * FIXED_1
 * Calculated for sample_rate of 20ms, window size of 100ms
 */
#define EXP    1677

static unsigned int get_avg_nr_runnables(void)
{
	unsigned int i, sum = 0;
	static unsigned int avg;
	struct runnables_avg_sample *sample;
	u64 integral, old_integral, delta_integral, delta_time, cur_time;

	for_each_online_cpu(i) {
		sample = &per_cpu(avg_nr_sample, i);
		integral = nr_running_integral(i);
		old_integral = sample->previous_integral;
		sample->previous_integral = integral;
		cur_time = ktime_to_ns(ktime_get());
		delta_time = cur_time - sample->prev_timestamp;
		sample->prev_timestamp = cur_time;

		if (!sample->integral_sampled) {
			sample->integral_sampled = true;
			/* First sample to initialize prev_integral, skip
			 * avg calculation
			 */
			continue;
		}

		if (integral < old_integral) {
			/* Overflow */
			delta_integral = (ULLONG_MAX - old_integral) + integral;
		} else {
			delta_integral = integral - old_integral;
		}

		/* Calculate average for the previous sample window */
		do_div(delta_integral, delta_time);
		sample->avg = delta_integral;
		sum += sample->avg;
	}

	/* Exponential moving average
	 * Avgn = Avgn-1 * alpha + new_avg * (1 - alpha)
	 */
	avg *= EXP;
	avg += sum * (FIXED_1 - EXP);
	avg >>= FSHIFT;

	return avg;
}

static int get_action(unsigned int nr_run)
{
	unsigned int nr_cpus = num_online_cpus();
	unsigned int max_cpus = cpq_max_cpus();
	unsigned int min_cpus = cpq_min_cpus();
	
	if ((nr_cpus > max_cpus || nr_run < nr_cpus) && nr_cpus >= min_cpus)
		return -1;

	if (nr_cpus < min_cpus || nr_run > nr_cpus)
		return 1;

	return 0;
}

static void runnables_avg_sampler(unsigned long data)
{
	unsigned int nr_run, avg_nr_run;
	int action;

	rmb();
	if (runnables_state != RUNNING)
		return;

	avg_nr_run = get_avg_nr_runnables();
	mod_timer(&runnables_timer, jiffies + msecs_to_jiffies(sample_rate));

	for (nr_run = 1; nr_run < ARRAY_SIZE(nr_run_thresholds); nr_run++) {
		unsigned int nr_threshold = nr_run_thresholds[nr_run - 1];
		if (nr_run_last <= nr_run)
			nr_threshold += NR_FSHIFT / nr_run_hysteresis;
		if (avg_nr_run <= (nr_threshold << (FSHIFT - NR_FSHIFT_EXP)))
			break;
	}

	nr_run_last = nr_run;

	action = get_action(nr_run);
	if (action != 0) {
		wmb();

	}
}

static unsigned int get_lightest_loaded_cpu_n(void)
{
	unsigned long min_avg_runnables = ULONG_MAX;
	unsigned int cpu = nr_cpu_ids;
	int i;

	for_each_online_cpu(i) {
		struct runnables_avg_sample *s = &per_cpu(avg_nr_sample, i);
		unsigned int nr_runnables = s->avg;
		if (i > 0 && min_avg_runnables > nr_runnables) {
			cpu = i;
			min_avg_runnables = nr_runnables;
		}
	}

	return cpu;
}

static void runnables_work_func(struct work_struct *work)
{
	unsigned int cpu = nr_cpu_ids;
	int action;

	if (!gov_enabled)
		return;

	if (runnables_state != RUNNING)
		return;

	action = get_action(nr_run_last);
	if (action > 0) {
		cpu = cpumask_next_zero(0, cpu_online_mask);
		if (cpu < nr_cpu_ids)
			cpuquiet_wake_cpu(cpu);
	} else if (action < 0) {
		cpu = get_lightest_loaded_cpu_n();
		if (cpu < nr_cpu_ids)
			cpuquiet_quiesence_cpu(cpu);
	}
}

static ssize_t show_nr_run_thresholds(struct cpuquiet_attribute *cattr, char *buf)
{
	char *out = buf;
	
	out += sprintf(out, "%d %d %d %d\n", nr_run_thresholds[0], nr_run_thresholds[1], nr_run_thresholds[2], nr_run_thresholds[3]);

	return out - buf;
}

static ssize_t store_nr_run_thresholds(struct cpuquiet_attribute *cattr,
					const char *buf, size_t count)
{
	int ret;
	int user_nr_run_thresholds[] = { 9, 17, 25, UINT_MAX };
	
	ret = sscanf(buf, "%d %d %d %d", &user_nr_run_thresholds[0], &user_nr_run_thresholds[1], &user_nr_run_thresholds[2], &user_nr_run_thresholds[3]);

	if (ret != 4)
		return -EINVAL;

	nr_run_thresholds[0] = user_nr_run_thresholds[0];
	nr_run_thresholds[1] = user_nr_run_thresholds[1];
	nr_run_thresholds[2] = user_nr_run_thresholds[2];
	nr_run_thresholds[3] = user_nr_run_thresholds[3];
	
	return count;
}

CPQ_BASIC_ATTRIBUTE(sample_rate, 0644, uint);
CPQ_BASIC_ATTRIBUTE(nr_run_hysteresis, 0644, uint);
CPQ_ATTRIBUTE_CUSTOM(nr_run_thresholds, 0644, show_nr_run_thresholds, store_nr_run_thresholds);

static struct attribute *runnables_attributes[] = {
	&sample_rate_attr.attr,
	&nr_run_hysteresis_attr.attr,
	&nr_run_thresholds_attr.attr,
	NULL,
};

static const struct sysfs_ops runnables_sysfs_ops = {
	.show = cpuquiet_auto_sysfs_show,
	.store = cpuquiet_auto_sysfs_store,
};

static struct kobj_type ktype_runnables = {
	.sysfs_ops = &runnables_sysfs_ops,
	.default_attrs = runnables_attributes,
};

static int runnables_sysfs(void)
{
	int err;

	runnables_kobject = kzalloc(sizeof(*runnables_kobject),
				GFP_KERNEL);

	if (!runnables_kobject)
		return -ENOMEM;

	err = cpuquiet_kobject_init(runnables_kobject, &ktype_runnables,
				"runnable_threads");

	if (err)
		kfree(runnables_kobject);

	return err;
}

static void runnables_device_busy(void)
{
	mutex_lock(&runnables_lock);
	if (runnables_state == RUNNING) {
		runnables_state = IDLE;
		del_timer_sync(&runnables_timer);
	}
	mutex_unlock(&runnables_lock);
}

static void runnables_device_free(void)
{
	mutex_lock(&runnables_lock);
	if (runnables_state == IDLE) {
		runnables_state = RUNNING;
		mod_timer(&runnables_timer, jiffies + 1);
	}
	mutex_unlock(&runnables_lock);
}

static void runnables_stop(void)
{
	mutex_lock(&runnables_lock);

	runnables_state = DISABLED;
	del_timer_sync(&runnables_timer);
	kobject_put(runnables_kobject);
	kfree(runnables_kobject);
	
	mutex_unlock(&runnables_lock);
}

static int runnables_start(void)
{
	int err;

	err = runnables_sysfs();
	if (err)
		return err;

	INIT_WORK(&runnables_work, runnables_work_func);

	init_timer(&runnables_timer);
	runnables_timer.function = runnables_avg_sampler;

	mutex_lock(&runnables_lock);
	runnables_state = RUNNING;
	mutex_unlock(&runnables_lock);

	runnables_avg_sampler(0);

	return 0;
}

struct cpuquiet_governor runnables_governor = {
	.name		   	  = "runnable",
	.start			  = runnables_start,
	.device_free_notification = runnables_device_free,
	.device_busy_notification = runnables_device_busy,
	.stop			  = runnables_stop,
	.owner		   	  = THIS_MODULE,
};

static int __init init_runnables(void)
{
	return cpuquiet_register_governor(&runnables_governor);
}

static void __exit exit_runnables(void)
{
	cpuquiet_unregister_governor(&runnables_governor);
}

MODULE_LICENSE("GPL");
module_init(init_runnables);
module_exit(exit_runnables);