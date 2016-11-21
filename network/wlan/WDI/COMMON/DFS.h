/*****************************************************************************
 *	Copyright 2010,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:		DFS.h	(RTL8190  Header H File)
 *
 *
 * Note:		Declare some variable which will be used by any debug command.
 *			
 *
 * Export:		
 *
 * Abbrev:			
 *
 * History:		
 *	Data		Who		Remark 
 *	??/??/2010  Cosa   	Create initial version in different filed.
 *	12/23/2010	MHC		Gather the code from other source files.
 * 
 *****************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef	__DFS_H__
#define __DFS_H__

/*--------------------------Define Parameters-------------------------------*/
//===============================================
//		DFS related definition
//===============================================
#if (DFS_SUPPORT == 1)

#if 1//real radar chnl
#define DFS_5G_RADAR_CHANNEL(__channel)	\
	((__channel >= 50 && __channel <= 70) ||\
	(__channel >= 94 && __channel <= 145))
#else//test radar chnl
#define DFS_5G_RADAR_CHANNEL(__channel)	\
	(__channel >= 40 && __channel <= 60) 
#endif

#define DFS_DEFAULT_NON_RADAR_CHNL			40

#define DFS_TEST_ALARM							1			// enable test alarm
#define DFS_AP_TEST_ALARM_PERIOD				50			//sec
#define DFS_CLIENT_PASSIVE_SCAN_ONLY			1

#define DFS_MAX_RADAR_CHNL_NUM				30
//#define DFS_STA_CHNL_MONITOR_TIME				10			//sec
#define DFS_STA_CHNL_MONITOR_TIME				60			//sec
#define DFS_STA_CHNL_SCAN_TYPE_MONITOR_TIME	(10-2)		//sec
#define DFS_AP_NON_OCCUPANCY_PERIOD			(30*60)		//sec
#define DFS_AP_CHNL_MONITOR_TIME				20			//sec

#define DFS_DISABLE_TX_ALL						0
#define DFS_DISABLE_TX_DATA					1
#define DFS_TEST_CHNL_PLAN						0

typedef enum _DFS_STATE_AP
{
	DFS_AP_UNINITIALIZED = 0,
	DFS_AP_INITIALIZED = 1,
	DFS_AP_DETECTING = 2,
	DFS_AP_BECONING = 3,
	DFS_AP_SWITCH_CHNL = 4,
	DFS_AP_MAX = 5,
} DFS_STATE_AP, *PDFS_STATE_AP;

typedef enum _DFS_STATE_STA
{
	DFS_STA_LISTEN = 0,
	DFS_STA_SWITCH = 1,
	DFS_STA_MAX = 2,
} DFS_STATE_STA, *PDFS_STATE_STA;

typedef struct _DFS_RADAR_CHNL_AP{
	u1Byte		radarChnl;
	u8Byte		rejectTime;
}DFS_RADAR_CHNL_AP, *PDFS_RADAR_CHNL_AP;

typedef struct _DFS_RADAR_CHNL_STA{
	u1Byte		radarChnl;
	u8Byte		rxBcnTime;
}DFS_RADAR_CHNL_STA, *PDFS_RADAR_CHNL_STA;

typedef struct _DFS_MGNT_AP{
//==========================
	u4Byte				testcnt;
//==========================
	BOOLEAN				bDisableDfs;
	DFS_STATE_AP			dfsState;
	DFS_RADAR_CHNL_AP	dfsRadarChnl[DFS_MAX_RADAR_CHNL_NUM];
	u1Byte				dfsTestAlert;
	BOOLEAN				bRadarDetected;
	BOOLEAN				bDisableTx;
	u1Byte				dfsSwitchChannel;
	u1Byte				dfsSwitchChCountDown;
	BOOLEAN				bMonitored;
	u8Byte				monitorStartTime;
	// Timer & WorkItem
	RT_TIMER				dfsTimer;
	RT_WORK_ITEM		dfsTimerWorkItem;
}DFS_MGNT_AP, *PDFS_MGNT_AP;

typedef struct _DFS_MGNT_STA{
	BOOLEAN				bDisableDfs;
	DFS_STATE_STA		dfsState;
	DFS_RADAR_CHNL_STA	dfsRadarChnl[DFS_MAX_RADAR_CHNL_NUM];
	BOOLEAN				bDisableTx;
	u1Byte				dfsSwitchChannel;
	u1Byte				dfsSwitchChCountDown;
	u1Byte				dfsOldConnectedChannel;
	BOOLEAN				bMonitorAfterSwitchIsDone;
	BOOLEAN				bMonitorAfterSwitch;
	u8Byte				dfsMonitorStartTime;
	RT_WORK_ITEM		csaWorkItem;
}DFS_MGNT_STA, *PDFS_MGNT_STA;

typedef struct _DFS_MGNT{
	DFS_MGNT_AP		apMode;
	DFS_MGNT_STA	staMode;
}DFS_MGNT, *PDFS_MGNT;
//===============================================
typedef enum _DFS_TIMER_CTRL_TYPE
{
	DFS_TIMER_INIT = 0,
	DFS_TIMER_SET,
 	DFS_TIMER_CANCEL,
 	DFS_TIMER_RELEASE,
	DFS_TIMER_MAX
}DFS_TIMER_CTRL_TYPE;

// 2010/12/27 MH Add for DFS workitem control.
typedef enum _DFS_WORKITEM_CTRL_TYPE
{
	DFS_WORKITEM_INIT = 0,
	DFS_WORKITEM_SET,
 	DFS_WORKITEM_CANCEL,
 	DFS_WORKITEM_FREE,
	DFS_WORKITEM_MAX
}DFS_WORKITEM_CTRL_TYPE;

VOID
DFS_Init(
	IN	PADAPTER	Adapter
);

VOID
DFS_TimerContrl(
	IN	PADAPTER	Adapter,
	IN	UINT8		TimerCtrlType
);

VOID
DFS_WorkItemContrl(
	IN	PADAPTER	Adapter,
	IN	UINT8		WorkItemCtrlType
);

BOOLEAN
DFS_IsTxDisabled(
	IN	PADAPTER			Adapter
);

VOID
DFS_ApIfNeedMonitorChnl(
	IN	PADAPTER	Adapter
);

VOID
DFS_ApConstructBeaconIEcsa(
	IN	PADAPTER	Adapter
);

BOOLEAN
DFS_ApChnlLocked(
	IN	PADAPTER	Adapter,
	u1Byte			chnl
	);

VOID
DFS_StaInsertToRadarChnlList(
	IN	PADAPTER	Adapter,
	u1Byte			Radarchnl
	);

VOID
DFS_StaMgntResetVars(
	IN	PADAPTER	Adapter
	);

VOID
DFS_StaCheckCsaInfo(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
);

VOID
DFS_StaConstructAssociateReq(
	IN	PADAPTER		Adapter,
	IN	u2Byte			asocCap,
	IN	OCTET_STRING	AsocReq
);

VOID
DFS_StaGetValueFromBeacon(
	IN		PADAPTER		Adapter,
	IN		OCTET_STRING	mmpdu,
	OUT		RT_WLAN_BSS	*bssDesc
	);

VOID
DFS_StaMonitor(
	IN	PADAPTER	Adapter
	);

VOID
DFS_StaCheckRadarChnl(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pbssdesc
);

VOID
DFS_StaUpdateRadarChnlScanType(
	IN	PADAPTER	Adapter
	);

#else	// #if (DFS_SUPPORT == 1)

#define	DFS_MGNT	u1Byte
#define	DFS_5G_RADAR_CHANNEL(__channel)		(FALSE)

#define	DFS_Init(Adapter)
#define	DFS_TimerContrl(Adapter, TimerCtrlType)
#define	DFS_WorkItemContrl(Adapter, WorkItemCtrlType)
#define	DFS_IsTxDisabled(Adapter)						FALSE
#define	DFS_ApIfNeedMonitorChnl(Adapter)
#define	DFS_ApConstructBeaconIEcsa(Adapter)
#define	DFS_ApChnlLocked(Adapter, chnl)				FALSE
#define	DFS_StaInsertToRadarChnlList(Adapter, Radarchnl)
#define	DFS_StaMgntResetVars(Adapter)
#define	DFS_StaGetValueFromBeacon(Adapter, mmpdu, bssDesc)
#define	DFS_StaCheckCsaInfo(Adapter, pBssDesc)
#define	DFS_StaConstructAssociateReq(Adapter, asocCap, AsocReq)
#define	DFS_StaMonitor(Adapter)
#define	DFS_StaCheckRadarChnl(Adapter, pbssdesc)
#define	DFS_StaUpdateRadarChnlScanType(Adapter)
#endif	// #if (DFS_SUPPORT == 1)


#endif	// __DFS_H__

