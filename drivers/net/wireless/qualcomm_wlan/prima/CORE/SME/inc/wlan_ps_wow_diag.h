/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _WLAN_PS_WOW_DIAG_H_
#define _WLAN_PS_WOW_DIAG_H_


#include "vos_diag_core_event.h"
#include "vos_diag_core_log.h"

#ifdef FEATURE_WLAN_DIAG_SUPPORT 

typedef enum
{
   WLAN_BMPS_ENTER_REQ =0,
   WLAN_UAPSD_START_REQ =1,
   WLAN_UAPSD_STOP_REQ =2,
   WLAN_ENTER_STANDBY_REQ =3,
   WLAN_ENTER_DEEP_SLEEP_REQ =4,
   WLAN_START_BMPS_AUTO_TIMER_REQ =5,
   WLAN_STOP_BMPS_AUTO_TIMER_REQ =6,
   WLAN_IMPS_ENTER_REQ =7,
   WLAN_ENTER_FULL_POWER_REQ =8,
   WLAN_PMC_CURRENT_STATE =9,
   WLAN_PS_MODE_ENABLE_REQ =10,
   WLAN_PS_MODE_DISABLE_REQ =11,
   WLAN_WINMOB_D_POWER_STATE =12,
   WLAN_BMPS_DTIM_PERIOD =13,
   WLAN_BMPS_FINAL_LI =14,
   WLAN_BMPS_SET_CONFIG =15,

} wlan_ps_evt_subtype_t;

typedef enum
{
   WLAN_MISSED_BEACON_IND_RCVD,    
   WLAN_BMPS_STATUS_IND_RCVD,      
   WLAN_BMPS_MODE_DISABLED,        
   WLAN_LINK_DISCONNECTED_BY_HDD,  
   WLAN_LINK_DISCONNECTED_BY_OTHER,
   WLAN_FULL_PWR_NEEDED_BY_HDD,    
   WLAN_FULL_PWR_NEEDED_BY_BAP,    
   WLAN_FULL_PWR_NEEDED_BY_CSR,    
   WLAN_FULL_PWR_NEEDED_BY_QOS,    
   WLAN_REASON_OTHER                

} wlan_ps_full_power_request_reason_t;

typedef enum 
{
   WLAN_PMC_STOPPED, 
   WLAN_PMC_FULL_POWER, 
   WLAN_PMC_LOW_POWER, 
   WLAN_PMC_REQUEST_IMPS,  
   WLAN_PMC_IMPS,  
   WLAN_PMC_REQUEST_BMPS,  
   WLAN_PMC_BMPS,  
   WLAN_PMC_REQUEST_FULL_POWER,  
   WLAN_PMC_REQUEST_START_UAPSD,  
   WLAN_PMC_REQUEST_STOP_UAPSD,  
   WLAN_PMC_UAPSD,           
   WLAN_PMC_REQUEST_STANDBY,  
   WLAN_PMC_STANDBY,  
   WLAN_PMC_REQUEST_ENTER_WOWL, 
   WLAN_PMC_REQUEST_EXIT_WOWL,  
   WLAN_PMC_WOWL                

} wlan_ps_pmc_current_state_t;

typedef enum
{
   WLAN_IDLE_MODE_POWER_SAVE,  
   WLAN_BEACON_MODE_POWER_SAVE,  
   WLAN_SPATIAL_MULTIPLEX_POWER_SAVE,  
   WLAN_UAPSD_MODE_POWER_SAVE,  
   WLAN_STANDBY_MODE_POWER_SAVE,  
   WLAN_WOWL_MODE_POWER_SAVE  

} wlan_ps_enable_disable_ps_mode_t;

typedef enum
{
   WLAN_D0,
   WLAN_D1,
   WLAN_D2,
   WLAN_D3,
   WLAN_D4

} wlan_ps_winmob_d_power_state_t;

typedef enum
{
   WLAN_WOW_ENTER_REQ =0,
   WLAN_WOW_EXIT_REQ =1,
   WLAN_WOW_DEL_PTRN_REQ =2,
   WLAN_WOW_WAKEUP = 3

} wlan_ps_wow_evt_subtype_t;

typedef enum
{
   WLAN_WOW_TYPE_NONE,
   WLAN_WOW_TYPE_MAGIC_PKT_ONLY,
   WLAN_WOW_TYPE_PTRN_BYTE_MATCH_ONLY,
   WLAN_WOW_TYPE_MAGIC_PKT_PTRN_BYTE_MATCH,

} wlan_ps_wow_type_t;

typedef enum
{
   WLAN_WOW_MAGIC_PKT_MATCH,
   WLAN_WOW_PTRN_BYTE_MATCH

} wlan_ps_wos_wakeup_cause_t;

#endif 

#endif 

