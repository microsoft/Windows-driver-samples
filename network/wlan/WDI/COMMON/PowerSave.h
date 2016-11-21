#ifndef __INC_POWERSAVE_H
#define __INC_POWERSAVE_H

/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalUsb.h
	
Abstract:
	Prototype of HalUsbXXX() and related data structure.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2006-11-06 Rcnjko            Create.
	
--*/


//================================================================================
// Helper macros. 
//================================================================================
#define IN_LEGACY_POWER_SAVE(__pStaQos) (((__pStaQos)->Curr4acUapsd & 0x0f) == 0)
#define	GET_POWER_SAVE_CONTROL(_pMgntInfo)	((PRT_POWER_SAVE_CONTROL)(&((_pMgntInfo)->PowerSaveControl)))
// 
// Get Wifi Mode Power Save, if it is true, follow PS-POLL or WMM APSD specification. Or use NULL-frame to
// implement power save mode. By Bruce, 2008-09-17.
//
#define	IS_WIFI_POWER_SAVE(_pMgntInfo) \
	((_pMgntInfo)->dot11PowerSaveMode != eActive && (_pMgntInfo)->PsPollType != 2 &&\
		((_pMgntInfo)->bWiFiConfg	|| (_pMgntInfo)->PsPollType == 1 ||				\
		!((_pMgntInfo)->IOTAction & HT_IOT_ACT_NULL_DATA_POWER_SAVING)))

#define	IS_FW_POWER_SAVE(_pMgntInfo) \
	((_pMgntInfo)->dot11PowerSaveMode != eActive && GET_POWER_SAVE_CONTROL(_pMgntInfo)->bFwCtrlLPS)


#define	RT_SET_PS_LEVEL(_pAdapter, _PS_FLAG)	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CurPsLevel |= _PS_FLAG)
#define	RT_CLEAR_PS_LEVEL(_pAdapter, _PS_FLAG)	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CurPsLevel &= (~(_PS_FLAG)))
#define	RT_GET_CUR_PS_LEVEL(_pAdapter)	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CurPsLevel)
#define	RT_IN_PS_LEVEL(_pAdapter, _PS_FLAG)	((RT_GET_CUR_PS_LEVEL(_pAdapter) & _PS_FLAG) ? TRUE : FALSE)

//
// <Roger_Notes> Currently we only take PCIe interface into consideration for RTD3 function.
// 2013.02.08.
//

        #define IS_RTD3_SUPPORT(_pAdapter)	FALSE
        #define RTD3_GET_DEV_PWR_STATE PowerDeviceD0

// Currently we support the machanism on 8723BS A to D-cut for Intel Baytrail-CR old BIOS behavior. 
// 2014.04.09 by tynli.
#define IS_CARD_DISABLE_IN_FW_LOW_PWR_STATE(_pAdapter)	\
	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CardDisableInLowClk && IS_VENDOR_8723B_D_CUT_BEFORE(_pAdapter))

//
// Only allow enter WoWLAN mode in infrastructure mode or no link. Do not support WoWLAN if adapter is
// operating in AP or IBSS mode.
//
#define IS_WOWLAN_OPERATING_MODE(_pMgntInfo) \
	((_pMgntInfo)->OpMode == RT_OP_MODE_INFRASTRUCTURE || \
		(_pMgntInfo)->OpMode == RT_OP_MODE_NO_LINK)


#define IPS_DISABLE_DEF_OPMODE	BIT0		// Disable IPS by default port OpMode (IBSS, AP)
#define IPS_DISABLE_EXT_OPMODE	BIT1		// Disable IPS by Extension port OpMode (VWifi)
#define IPS_DISABLE_PROXIMITY	BIT2		// Disable IPS by PROXIMITY Mode
#define IPS_DISABLE_BT_ON			BIT3		// Disable IPS by BT3.0+HS
#define IPS_DISABLE_BY_OID		BIT4


#define LPS_DISABLE_AP_MODE							BIT0
#define LPS_DISABLE_BT_HS_CONNECTION					BIT1
#define LPS_DISABLE_TURN_OFF_ADAPTER					BIT2
#define LPS_DISABLE_UPDATE_LPS_STATUS_HNDR			BIT3
#define LPS_DISABLE_DEBUG_CONTROL					BIT4
#define LPS_DISABLE_INTEL_PROX_MODE					BIT5
#define LPS_DISABLE_SET_DISASSOCIATE					BIT6
#define LPS_DISABLE_SET_DEAUTH						BIT7
#define LPS_DISABLE_ON_DISASSOC						BIT8
#define LPS_DISABLE_ON_DEAUTH							BIT9
#define LPS_DISABLE_MGNT_DISCONNECT					BIT10
#define LPS_DISABLE_MGNT_INDI_DISCONNECT				BIT11
#define LPS_DISABLE_JOIN_REQ							BIT12
#define LPS_DISABLE_LINK_REQ							BIT13
#define LPS_DISABLE_FOR_ROAMING						BIT14
#define LPS_DISABLE_WATCH_DOG_BUSY_TRAFFIC			BIT15
#define LPS_DISABLE_WATCH_DOG_MEDIA_DISCONNEC		BIT16
#define LPS_DISABLE_OID_SET							BIT17
#define LPS_DISABLE_LEAVE_ALL_PS						BIT18
#define LPS_DISABLE_LEAVE_CHK_RDY						BIT19
#define LPS_DISABLE_RX_DETECT_BUSY					BIT20
#define LPS_DISABLE_TX_EAPOL_PKT						BIT21
#define LPS_DISABLE_TX_DHCP_PKT						BIT22
#define LPS_DISABLE_TX_ARP_PKT						BIT23
#define LPS_DISABLE_TX_SPECIAL_PKT						BIT24
#define LPS_DISABLE_TX_DETECT_BUSY					BIT25
#define LPS_DISABLE_BT_COEX							BIT26
#define LPS_DISABLE_DEVICE_DISCOVERY					BIT27
//================================================================================
// Variable Definition. 
//================================================================================
typedef enum _POWER_LEVEL_CONFIG
{
	POWER_SAVING_NO_POWER_SAVING, // Maximal Performance
	POWER_SAVING_FAST_PSP, // Low Power Saving
	POWER_SAVING_MAX_PSP, // Medium Power Saving
	POWER_SAVING_MAXIMUM_LEVEL, // Maximal Power Saving
}POWER_LEVEL_CONFIG;

typedef enum _POWER_SETTING_CONFIG
{
	POWERGUID_MAX_POWER_SAVINGS, // Power Saver
	POWERGUID_TYPICAL_POWER_SAVINGS, // Banlanced
	POWERGUID_MIN_POWER_SAVINGS, // High performance
	POWERGUID_UNDEFINED,
}POWER_SETTING_CONFIG;

typedef enum _POWER_POLICY_CONFIG
{
	POWERCFG_MAX_POWER_SAVINGS,			// Enter IPS, LPS when DC mode
	POWERCFG_GLOBAL_POWER_SAVINGS,		// Enter IPS, LPS when DC mode and PowerSaver config. Depends on OS configuration.
	POWERCFG_LOCAL_POWER_SAVINGS,		// Enter IPS, LPS when DC mode and not Maximum Performance config. Depends on Wifi configuration.
	POWERCFG_LENOVO,					// Enter IPS, LPS when DC mode and not Maximum Performance, AC mode and Maximum Power Saving.
	POWERCFG_LENOVO_JANPEN,				// Enter IPS, LPS when OS config to Banlanced or PowerSave. Don't care about AC/DC mode.
	POWERCFG_SAMSUNG,					// Enter IPS, LPS when OS config to PowerSave. Don't care about AC/DC mode.
	POWERCFG_HP,					 	// Enter IPS, LPS when OS config to PowerSave. Don't care about AC/DC mode.
}POWER_POLICY_CONFIG;


//================================================================================
// Callback routines of timers and workitems.  
//================================================================================
VOID 
OnMoreData(
	IN PADAPTER		pAdapter
	);

BOOLEAN
IsOurFrameBuffred(
	IN	OCTET_STRING	Tim,
	IN	u2Byte			AId
	);

BOOLEAN
IsMcstFrameBuffered(
	IN	OCTET_STRING	octetTim
	);

VOID
DozeWorkItemCallback(
	IN PVOID		pContext
	);


VOID
AwakeTimerCallback(
	IN PRT_TIMER		pTimer
	);

VOID
AwakeWorkItemCallback(
	IN PVOID		pContext
	);

VOID
InactivePsWorkItemCallback(
	IN PVOID		pContext
	);

VOID
InactivePsTimerCallback(
	IN PRT_TIMER		pTimer
	);

//================================================================================
// 802.11 legacy power save.
//================================================================================
VOID
LPS_OnBeacon_BSS(
	IN	PADAPTER			pAdapter,
	IN	OCTET_STRING		osBeacon
	);

VOID
LPS_DozeStart(
	PADAPTER	pAdapter
	);

VOID
LPS_DozeComplete(
	IN	PADAPTER		pAdapter
	);

VOID
LPS_AwakeComplete(
	IN	PADAPTER		pAdapter
	);

BOOLEAN
LPS_OnTx(
	IN PADAPTER		pAdapter,
	IN PRT_TCB		pTcb
	);

//================================================================================
// WMM power save.
//================================================================================
VOID
WMMPS_OnBeacon_BSS(
	IN	PADAPTER			pAdapter,
	IN	OCTET_STRING		osBeacon
	);

VOID
WMMPS_DozeStart(
	PADAPTER	pAdapter
	);

VOID
WMMPS_DozeComplete(
	IN	PADAPTER		pAdapter
	);

VOID
WMMPS_AwakeComplete(
	IN	PADAPTER		pAdapter
	);

BOOLEAN
WMMPS_OnTx(
	IN PADAPTER		pAdapter,
	IN PRT_TCB		pTcb
	);

BOOLEAN
GetQosQueueIDMaskAPSD(
	u1Byte		QueueID,
	AC_UAPSD	APSD
	);

BOOLEAN
GetPS_Doze(
	IN PADAPTER		pAdapter
	);



//================================================================================
// Inactive power save.
//================================================================================

VOID
IPSEnter(
	IN PADAPTER		pAdapter
	);

VOID
IPSLeave(
	IN PADAPTER		pAdapter,
	IN BOOLEAN		bForceEnter
	);

VOID
IPSEnable(
	IN PADAPTER		pAdapter
	);

VOID
IPSDisable(
	IN PADAPTER		pAdapter,
	IN BOOLEAN		bForceEnter	,
	IN u1Byte		nReason
	);

VOID
IPSReturn(
	IN PADAPTER		pAdapter,
	IN u1Byte		nReason
	);

//================================================================================
// Leisure Power Save in linked state.
//================================================================================

VOID
LeisurePSEnter(
	IN PADAPTER		pAdapter
	);

VOID
LeisurePSLeave(
	IN PADAPTER		pAdapter,
	IN u4Byte			reason
	);

//================================================================================
// Fw control LPS.
//================================================================================

BOOLEAN
GetFwLPS_Doze(
	IN PADAPTER 	pAdapter
	);

VOID
LeaveAllPowerSaveMode(
	IN PADAPTER		Adapter
	);

BOOLEAN
LPSLeaveAndCheckReady(
	IN PADAPTER		Adapter
	);

VOID
FwPsClockOnWorkitemCallback(
	IN	PVOID	Context
	);

#endif
