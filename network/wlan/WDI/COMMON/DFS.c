/******************************************************************************

     (c) Copyright 2010, RealTEK Technologies Inc. All Rights Reserved.

 Module:	DFS.c	(RTL8190  Source C File)

 Note:		Declare some variable which will be used by any debug command.
 
 Function:	
 		 
 Export:	

 Abbrev:	

 History:
	Data		Who		Remark
	
	??/??/2010	Cosa	Create initial version.
	12/23/2010	MHC		Gather the code from other source files.
		
******************************************************************************/
/* Header Files. */
#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "DFS.tmh"
#endif


extern VOID
AP_Restart(
	IN	PADAPTER		Adapter
	);
extern VOID
Hal_PauseTx(
	IN		PADAPTER	Adapter,
	u1Byte	type
	);

#if (DFS_SUPPORT == 1)
//============================================
//		The following is for DFS local function, 
//		start with dfs_*()
//============================================
VOID
dfs_StaDisableTx(
	IN	PADAPTER	Adapter,
	u1Byte			type
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	if(type == DFS_DISABLE_TX_ALL)
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_StaDisableTx all!!\n"));
		Hal_PauseTx(Adapter, HW_DISABLE_TX_ALL);		
	}
	else if(type == DFS_DISABLE_TX_DATA)
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_StaDisableTx data!!\n"));
		Hal_PauseTx(Adapter, HW_DISABLE_TX_DATA);
		//ReleaseDataFrameQueued(Adapter);
	}

	// set flag to disable tx
	pMgntDFS->staMode.bDisableTx = TRUE;
}

VOID
dfs_StaEnableTx(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_StaEnableTx all!!\n"));
	Hal_PauseTx(Adapter, HW_ENABLE_TX_ALL);
	pMgntDFS->staMode.bDisableTx = FALSE;
}

VOID
dfs_StaCsaWorkItemCallback(
	IN	PVOID	Context
	)
{
	PADAPTER		Adapter = (PADAPTER)Context;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	if(pMgntDFS->staMode.dfsState == DFS_STA_LISTEN)
	{
		pMgntDFS->staMode.dfsState = DFS_STA_SWITCH;

		dfs_StaDisableTx(Adapter, DFS_DISABLE_TX_DATA);

		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_StaCsaWorkItemCallback(), switch chnl to %d\n",
			pMgntDFS->staMode.dfsSwitchChannel));

		pMgntDFS->staMode.dfsOldConnectedChannel = pMgntInfo->dot11CurrentChannelNumber;
		
		if(pMgntInfo->dot11CurrentChannelNumber != pMgntDFS->staMode.dfsSwitchChannel)
		{
			pMgntInfo->dot11CurrentChannelNumber = pMgntDFS->staMode.dfsSwitchChannel;
			Mgnt_SwChnl(GetDefaultAdapter(Adapter)
				,pMgntInfo->dot11CurrentChannelNumber,2);
		}
		
		pMgntDFS->staMode.dfsSwitchChannel = 0;

		if(DFS_5G_RADAR_CHANNEL(pMgntInfo->dot11CurrentChannelNumber))
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_StaCsaWorkItemCallback(), DFS_5G_RADAR_CHANNEL, need to monitor first!!\n"));
			pMgntDFS->staMode.bMonitorAfterSwitch = TRUE;
			pMgntDFS->staMode.bMonitorAfterSwitchIsDone = FALSE;
			pMgntDFS->staMode.dfsMonitorStartTime = PlatformGetCurrentTime();
		}
		else
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_StaCsaWorkItemCallback(), Non DFS_5G_RADAR_CHANNEL!!\n"));
			dfs_StaEnableTx(Adapter);
		}

		pMgntDFS->staMode.dfsState = DFS_STA_LISTEN;
	}
}


u1Byte
dfs_ApConstructAvaChnlList(
	IN	PADAPTER	Adapter,
	OUT	pu1Byte		plist
	)
{
	PRT_CHANNEL_LIST	pChanneList = NULL;
	u1Byte	total, AvailableNum=0, i;

	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChanneList);
	total = pChanneList->ChannelLen;

	for(i=0; i<total; i++)
	{
		if(!DFS_ApChnlLocked(Adapter, pChanneList->ChnlListEntry[i].ChannelNum))
		{
			*plist = pChanneList->ChnlListEntry[i].ChannelNum;
			plist++;
			AvailableNum++;
		}
	}
	return AvailableNum;
}

VOID
dfs_ApRejectChnlListDump(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u1Byte			i;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApRejectChnlListDump(), Radar channel list dump : \n"));

	RT_TRACE( COMP_DFS, DBG_LOUD, ("["));
	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		if(pMgntDFS->apMode.dfsRadarChnl[i].radarChnl != 0)
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("%d, ", pMgntDFS->apMode.dfsRadarChnl[i].radarChnl));
		}
	}
	RT_TRACE( COMP_DFS, DBG_LOUD, ("]\n"));
}

VOID
dfs_ApInsertRejectChnl(
	IN	PADAPTER	Adapter,
	u1Byte			Rejectchnl
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u1Byte			i;
	
	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApInsertRejectChnl(), insert chnl %d to Radar channel list\n",
				Rejectchnl));
	
	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		if(pMgntDFS->apMode.dfsRadarChnl[i].radarChnl == 0)
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApInsertRejectChnl(), find empty and insert chnl %d to Radar channel list\n",
				Rejectchnl));
			pMgntDFS->apMode.dfsRadarChnl[i].radarChnl = Rejectchnl;
			pMgntDFS->apMode.dfsRadarChnl[i].rejectTime = PlatformGetCurrentTime(); // in micro-second.
			break;
		}
		else if(pMgntDFS->apMode.dfsRadarChnl[i].radarChnl == Rejectchnl)
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApInsertRejectChnl(), chnl %d is already in, update time.\n",
				Rejectchnl));
			pMgntDFS->apMode.dfsRadarChnl[i].rejectTime = PlatformGetCurrentTime(); // in micro-second.
			break;
		}
	}

	dfs_ApRejectChnlListDump(Adapter);
}

VOID
dfs_ApDisableTx(
	IN	PADAPTER	Adapter,
	u1Byte			type
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	
	if(type == DFS_DISABLE_TX_ALL)
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApDisableTx all!!\n"));
		Hal_PauseTx(Adapter, HW_DISABLE_TX_ALL);
	}
	else if(type == DFS_DISABLE_TX_DATA)
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApDisableTx data!!\n"));
		Hal_PauseTx(Adapter, HW_DISABLE_TX_DATA);
	}

	// set flag to disable tx
	pMgntDFS->apMode.bDisableTx = TRUE;
}

VOID
dfs_ApResetRadarDetectCounterAndFlag(
	IN	PADAPTER	Adapter
	)
{
#if (DFS_TEST_ALARM == 0)
	// Clear Radar Counter and Radar flag
	PHY_SetBBReg(Adapter, 0xc84, BIT(25), 0);
	PHY_SetBBReg(Adapter, 0xc84, BIT(25), 1);

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], After reset radar counter, 0xcf8 = 0x%x, 0xcf4 = 0x%x\n",
		PHY_QueryBBReg(Adapter, 0xcf8, bMaskDWord),
		PHY_QueryBBReg(Adapter, 0xcf4, bMaskDWord)));
#endif
}

VOID
dfs_ApResetMonitorState(
	IN	PADAPTER	Adapter,
	BOOLEAN			bNeedMonitor
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	
	if(bNeedMonitor)
	{
		pMgntDFS->apMode.bMonitored = FALSE;
	}
	else
	{
		pMgntDFS->apMode.bMonitored = TRUE;
	}

	pMgntDFS->apMode.monitorStartTime = 0;
	pMgntDFS->apMode.dfsState = DFS_AP_DETECTING;
	pMgntDFS->apMode.bRadarDetected = FALSE;
	pMgntDFS->apMode.dfsSwitchChannel = 0;
}

VOID
dfs_ApEnableTx(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApEnableTx all!!\n"));
	Hal_PauseTx(Adapter, HW_ENABLE_TX_ALL);
	pMgntDFS->apMode.bDisableTx = FALSE;
}

VOID
dfs_ApSwitchChnl(
	IN	PVOID	Context
	)	
{
	PADAPTER		Adapter = (PADAPTER)Context;
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApSwitchChnl(), switch chnl to %d\n",
		pMgntDFS->apMode.dfsSwitchChannel));
	
	dfs_ApDisableTx(Adapter, DFS_DISABLE_TX_ALL);

	MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(Adapter, pMgntDFS->apMode.dfsSwitchChannel);
	AP_Restart(Adapter);
}

u1Byte
dfs_ApSelectChannel(
	IN	PADAPTER	Adapter
	)
{
	u1Byte 			num, which_channel=36;
	u1Byte			aviChnl[MAX_CHANNEL_NUM]={0};
	u1Byte			AviNum=0, i;

	AviNum = dfs_ApConstructAvaChnlList(Adapter, aviChnl);
	if(AviNum)
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], Avi chnl list = [ "));
		for(i=0; i<AviNum; i++)
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("%d, ", aviChnl[i]));
		}
		RT_TRACE( COMP_DFS, DBG_LOUD, ("]\n"));

		num = (u1Byte)GetRandomNumber(0, AviNum);	
		which_channel = aviChnl[num];
	}
	else
	{
		which_channel = 153;
	}
	
	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], dfs_ApSelectChannel to chnl-%d\n", which_channel));
	return which_channel;
}

VOID
dfs_ApDetecting(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u4Byte			radar_detect_val=0, radar_cnt=0;
	u4Byte			radar_type=0;

	if(!DFS_5G_RADAR_CHANNEL(pMgntInfo->dot11CurrentChannelNumber))
		return;

#if (DFS_TEST_ALARM == 1)// Trigger one time every DFS_AP_TEST_ALARM_PERIOD
	if(pMgntDFS->apMode.testcnt++ >= (DFS_AP_TEST_ALARM_PERIOD*10))
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS test alarm!!\n"));
		pMgntDFS->apMode.testcnt = 0;
		pMgntDFS->apMode.dfsTestAlert = 1;
		pMgntDFS->apMode.bRadarDetected = TRUE;
	}
#else
	// Check radar detect flag.
	radar_detect_val = PHY_QueryBBReg(Adapter, 0xcf8, bMaskDWord);
	if( (radar_detect_val & BIT31) ||
		(radar_detect_val & BIT23) )
	{
		radar_type = 1;
		pMgntDFS->apMode.bRadarDetected = TRUE;
		radar_cnt = PHY_QueryBBReg(Adapter, 0xcf4, bMaskDWord);
	}
#endif

	if(pMgntDFS->apMode.bRadarDetected)
	{	
		// Only display counters when real radar signal is detected
		if(pMgntDFS->apMode.dfsTestAlert)
			pMgntDFS->apMode.dfsTestAlert = 0;

		if(radar_type == 1)
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], 0xcf8=0x%x, 0xcf4=0x%x !\n", 
				radar_detect_val, radar_cnt));
		}

		// Set DFS state to send notify beacon.
		pMgntDFS->apMode.dfsState = DFS_AP_BECONING;

		dfs_ApDisableTx(Adapter, DFS_DISABLE_TX_DATA);
		
		ReleaseDataFrameQueued(Adapter);

		// Insert current channel to the reject list.
		dfs_ApInsertRejectChnl(Adapter, pMgntInfo->dot11CurrentChannelNumber);
		//select a non-DFS channel
		pMgntDFS->apMode.dfsSwitchChannel = dfs_ApSelectChannel(Adapter);

		pMgntDFS->apMode.dfsSwitchChCountDown = 5;

		if (pMgntInfo->dot11DtimPeriod >= pMgntDFS->apMode.dfsSwitchChCountDown)
			pMgntDFS->apMode.dfsSwitchChCountDown = pMgntInfo->dot11DtimPeriod+1;

	}
	else
	{	// monitor to make sure the channel is available
		u8Byte currTime, diffTime;
		if(!pMgntDFS->apMode.bMonitored)
		{
			currTime = PlatformGetCurrentTime();
			if(pMgntDFS->apMode.monitorStartTime == 0)
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS monitor started!!\n"));
				dfs_ApDisableTx(Adapter, DFS_DISABLE_TX_ALL);
				pMgntDFS->apMode.monitorStartTime = currTime;
			}
			else
			{
				diffTime = currTime-pMgntDFS->apMode.monitorStartTime;
				if(PlatformDivision64(diffTime,1000000) >= DFS_AP_CHNL_MONITOR_TIME)
				{
					pMgntDFS->apMode.bMonitored = TRUE;
					RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS monitor finished, enable Tx!!\n"));
					dfs_ApEnableTx(Adapter);
				}
			}
		}
		else
		{
			if(pMgntDFS->apMode.bDisableTx)
			{
				dfs_ApEnableTx(Adapter);
			}
		}
	}
}

VOID
dfs_ApInitialize(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT	pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u1Byte		i;

	//============================
	// debug vars
	//============================
	pMgntDFS->apMode.testcnt = 0;
	pMgntDFS->apMode.dfsTestAlert = 0;
	//============================

	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		pMgntDFS->apMode.dfsRadarChnl[i].radarChnl = 0;
		pMgntDFS->apMode.dfsRadarChnl[i].rejectTime = 0;
	}
	
	pMgntDFS->apMode.bRadarDetected = FALSE;
	pMgntDFS->apMode.bDisableTx = FALSE;
	pMgntDFS->apMode.dfsSwitchChannel = 0;
	pMgntDFS->apMode.dfsSwitchChCountDown = 5;

	pMgntDFS->apMode.bMonitored = FALSE;
	pMgntDFS->apMode.monitorStartTime = 0;
}

VOID
dfs_APDFSTimerWorkItemCallback(
	IN	PVOID	Context
	)
{
	PADAPTER		Adapter = (PADAPTER)Context;
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	switch(pMgntDFS->apMode.dfsState)
	{
		case DFS_AP_UNINITIALIZED:
			RT_TRACE(COMP_DFS, DBG_LOUD, ("[DFS], Uninitialized state, initialize AP Mode!!\n"));
			dfs_ApInitialize(Adapter);
			pMgntDFS->apMode.dfsState = DFS_AP_INITIALIZED;
			break;

		case DFS_AP_INITIALIZED:
			RT_TRACE(COMP_DFS, DBG_LOUD, ("[DFS], AP start radar monitor!!\n"));
			pMgntDFS->apMode.dfsState = DFS_AP_DETECTING;
			break;

		case DFS_AP_DETECTING:
			dfs_ApDetecting(Adapter);
			break;

		case DFS_AP_BECONING:
			// do nothing now.
			break;

		case DFS_AP_SWITCH_CHNL:
			RT_TRACE(COMP_DFS, DBG_LOUD, ("[DFS], AP switch channel!!\n"));
			dfs_ApSwitchChnl(Adapter);
			break;
			
		default:
			RT_ASSERT(FALSE, ("AP Unknown state\n"));
			break;
	}
}

VOID
dfs_ApDfsTimerCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER	Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if(!ACTING_AS_AP(Adapter))
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], Warning!! dfs_ApDfsTimerCallback(): STA mode\n"));
		return;
	}
	else
	{
		PlatformScheduleWorkItem(&(pMgntInfo->DFSMgnt.apMode.dfsTimerWorkItem));
		PlatformSetTimer( Adapter, &pMgntInfo->DFSMgnt.apMode.dfsTimer, 100);
	}
}


//============================================
//		The following is for DFS Extern function, 
//		start with DFS_*()
//============================================

VOID
DFS_Init(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT	pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	BOOLEAN		bSupportDFS = FALSE;
	
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_SUPPORT_5G, (PBOOLEAN)&bSupportDFS);
	if (bSupportDFS)
	{
		Adapter->DFSSupport = TRUE;
#if(DFS_SUPPORT_AP_MODE == 1)
		pMgntDFS->apMode.bDisableDfs = FALSE;
#else
		pMgntDFS->apMode.bDisableDfs = TRUE;
#endif

#if(DFS_SUPPORT_STA_MODE == 1)
		pMgntDFS->staMode.bDisableDfs = FALSE;
#else
		pMgntDFS->staMode.bDisableDfs = TRUE;
#endif
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], Set support flag=TRUE!!\n"));
	}
	else
	{
		Adapter->DFSSupport = FALSE;
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], Set support flag=FALSE!!\n"));
	}
}


VOID
DFS_TimerContrl(
	IN	PADAPTER	Adapter,
	IN	UINT8		TimerCtrlType
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PDFS_MGNT	pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	if(Adapter->DFSSupport == FALSE)
		return;

	if(pMgntDFS->apMode.bDisableDfs)
		return;

	// Comment out for compiler type cast warnning on Vista x64	
	//RT_TRACE(COMP_DFS, DBG_LOUD, ("[DFS], DFS_TimerContrl(), Adapter=0x%x, type=", (u4Byte)Adapter));
	
	switch(TimerCtrlType)
	{
		case DFS_TIMER_INIT:
			RT_TRACE( COMP_DFS, DBG_LOUD, ("Timer initialization for AP mode!!\n"));
			PlatformInitializeTimer(Adapter, &(pMgntInfo->DFSMgnt.apMode.dfsTimer), (RT_TIMER_CALL_BACK)dfs_ApDfsTimerCallback, NULL, "DFSTimer");
			break;

		case DFS_TIMER_SET:
			RT_TRACE( COMP_DFS, DBG_LOUD, ("Set Timer for AP mode!!\n"));
			PlatformSetTimer( Adapter, &pMgntInfo->DFSMgnt.apMode.dfsTimer, 100);
			break;
		
		case DFS_TIMER_CANCEL:
			RT_TRACE( COMP_DFS, DBG_LOUD, ("Cancel Timer for AP mode!!\n"));
			PlatformCancelTimer(Adapter, &pMgntInfo->DFSMgnt.apMode.dfsTimer);
			break;

		case DFS_TIMER_RELEASE:
			RT_TRACE( COMP_DFS, DBG_LOUD, ("Release Timer for AP mode!!\n"));
			PlatformReleaseTimer(Adapter, &pMgntInfo->DFSMgnt.apMode.dfsTimer);
			break;
			
		default:
			RT_ASSERT(FALSE, ("Unknown Action!!\n"));
			break;
	}
}

//
// 2010/12/25 MH Gather DFS relative workitem control in the function.
//
VOID
DFS_WorkItemContrl(
	IN	PADAPTER	Adapter,
	IN	UINT8		WorkItemCtrlType
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PDFS_MGNT	pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	
	if(Adapter->DFSSupport == FALSE)
		return;
		
	// Comment out for compiler type cast warnning on Vista x64
	//RT_TRACE(COMP_DFS, DBG_LOUD, ("[DFS], DFS_WorkItemContrl(), Adapter=0x%x, type=", (u4Byte)Adapter));
	
	switch(WorkItemCtrlType)
	{
		case DFS_WORKITEM_INIT:
			if(!pMgntDFS->apMode.bDisableDfs)
			{
				RT_TRACE(COMP_DFS, DBG_LOUD, ("Workitem initialization for AP mode!!\n"));
				PlatformInitializeWorkItem(
				Adapter,
				&(pMgntInfo->DFSMgnt.apMode.dfsTimerWorkItem), 
				(RT_WORKITEM_CALL_BACK)dfs_APDFSTimerWorkItemCallback, 
				(PVOID)Adapter,
				"AP_DFSTimerWorkItem");
			}
			if(!pMgntDFS->staMode.bDisableDfs)
			{
				RT_TRACE(COMP_DFS, DBG_LOUD, ("Workitem initialization for STA mode!!\n"));
				PlatformInitializeWorkItem(
				Adapter,
				&(pMgntInfo->DFSMgnt.staMode.csaWorkItem), 
				(RT_WORKITEM_CALL_BACK)dfs_StaCsaWorkItemCallback, 
				(PVOID)Adapter,
				"DFS_CSAWorkItem");
			}
			break;
		
		case DFS_WORKITEM_FREE:
			if(!pMgntDFS->apMode.bDisableDfs)
			{
				RT_TRACE(COMP_DFS, DBG_LOUD, ("Free Workitem for AP mode!!\n"));
				PlatformFreeWorkItem(&(pMgntInfo->DFSMgnt.apMode.dfsTimerWorkItem));
			}
			if(!pMgntDFS->staMode.bDisableDfs)
			{
				RT_TRACE(COMP_DFS, DBG_LOUD, ("Free Workitem for STA mode!!\n"));
				PlatformFreeWorkItem(&(pMgntInfo->DFSMgnt.staMode.csaWorkItem));
			}
			break;
			
		default:
			break;
	}
}

BOOLEAN
DFS_IsTxDisabled(
	IN	PADAPTER			Adapter
)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	if (Adapter->DFSSupport == FALSE)
		return FALSE;
	
	if(ACTING_AS_AP(Adapter))
	{
		if (!pMgntDFS->apMode.bDisableDfs &&
			pMgntDFS->apMode.bDisableTx)
		{
			//RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], N6usbSendNetBufferLists(): AP mode, DFS tx disabled!!!\n"));
			return TRUE;
		}
	}
	else
	{
		if (!pMgntDFS->staMode.bDisableDfs &&
			pMgntDFS->staMode.bDisableTx)
		{
			//RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], N6usbSendNetBufferLists(): STA mode, DFS tx disabled!!!\n"));
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN
DFS_ApChnlLocked(
	IN	PADAPTER	Adapter,
	u1Byte			chnl
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u8Byte			currTime, diffTime;
	u1Byte			i;

	if (Adapter->DFSSupport == FALSE)
		return FALSE;

	if (pMgntDFS->apMode.bDisableDfs)
		return FALSE;
	
	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		if((pMgntDFS->apMode.dfsRadarChnl[i].radarChnl == chnl))
		{
			currTime = PlatformGetCurrentTime();
			diffTime = currTime - pMgntDFS->apMode.dfsRadarChnl[i].rejectTime;
			if(PlatformDivision64(diffTime,1000000) > DFS_AP_NON_OCCUPANCY_PERIOD)
			{
				pMgntDFS->apMode.dfsRadarChnl[i].radarChnl = 0;
				pMgntDFS->apMode.dfsRadarChnl[i].rejectTime = 0;
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_ApChnlLocked(), unlock chnl-%d!!\n", chnl));
				return FALSE;
			}
			else
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_ApChnlLocked(), chnl-%d is locked, choose another!!\n", chnl));
				return TRUE;
			}
		}
	}
	return FALSE;
}


VOID
DFS_ApIfNeedMonitorChnl(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	if (Adapter->DFSSupport == FALSE)
		return;

	if (pMgntDFS->apMode.bDisableDfs)
		return;
	
	if(!ACTING_AS_AP(Adapter))
		return;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_ApIfNeedMonitorChnl()!!\n"));
	
	dfs_ApResetRadarDetectCounterAndFlag(Adapter);

	if(DFS_5G_RADAR_CHANNEL(pMgntInfo->dot11CurrentChannelNumber))
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], chnl %d is a Radar channel, have to monitor first!!\n",
			pMgntInfo->dot11CurrentChannelNumber));
		
		dfs_ApResetMonitorState(Adapter, TRUE);
		dfs_ApDisableTx(Adapter, DFS_DISABLE_TX_ALL);
	}
	else
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], chnl %d is Not a Radar channel!!\n",
			pMgntInfo->dot11CurrentChannelNumber));

		dfs_ApResetMonitorState(Adapter, FALSE);
		dfs_ApEnableTx(Adapter);
	}
}

VOID
DFS_ApConstructBeaconIEcsa(
	IN	PADAPTER	Adapter
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;	
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	OCTET_STRING	ChlSwitchAnnounce;
	u1Byte			ChlSwitchAnnounceBuf[3];
	pu1Byte			pbcn;

	if (Adapter->DFSSupport == FALSE)
		return;

	if(pMgntDFS->apMode.bDisableDfs)
		return;
	
	if(ACTING_AS_AP(Adapter))
	{
		pbcn = pMgntInfo->beaconframe.Octet;
		//RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], AP construct beacon!!\n"));
		UNION_BEACON_PROBE_RSP_CAPABILITY_INFO(pbcn, cSpectrumMgnt);

		if (pMgntDFS->apMode.bRadarDetected && pMgntDFS->apMode.dfsSwitchChannel)
		{
			if (pMgntDFS->apMode.dfsSwitchChCountDown)
			{
				ChlSwitchAnnounceBuf[0] = 1;	// channel switch mode
				ChlSwitchAnnounceBuf[1] = pMgntDFS->apMode.dfsSwitchChannel;	// new channel number
				ChlSwitchAnnounceBuf[2] = pMgntDFS->apMode.dfsSwitchChCountDown;	// channel switch count

				FillOctetString(ChlSwitchAnnounce, ChlSwitchAnnounceBuf, 3);
				PacketMakeElement(&pMgntInfo->beaconframe, EID_ChlSwitchAnnounce, ChlSwitchAnnounce);
				
				pMgntDFS->apMode.dfsSwitchChCountDown--;
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS counting = %d\n", 
					pMgntDFS->apMode.dfsSwitchChCountDown));

				if( pMgntDFS->apMode.dfsSwitchChCountDown == 0 &&
					pMgntDFS->apMode.dfsState == DFS_AP_BECONING )
				{
					pMgntDFS->apMode.dfsState = DFS_AP_SWITCH_CHNL;
				}
			}
		}
	}
}

VOID
DFS_StaInsertToRadarChnlList(
	IN	PADAPTER	Adapter,
	u1Byte			Radarchnl
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u1Byte			i;

	if(Adapter->DFSSupport == FALSE)
		return;

	if(pMgntDFS->staMode.bDisableDfs)
		return;
	
	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		if((pMgntDFS->staMode.dfsRadarChnl[i].radarChnl == 0))
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaInsertToRadarChnlList(), insert chnl %d to Radar channel list\n",
				Radarchnl));
			pMgntDFS->staMode.dfsRadarChnl[i].radarChnl = Radarchnl;
			pMgntDFS->staMode.dfsRadarChnl[i].rxBcnTime = 0; // in micro-second.
			break;
		}
	}
}

VOID
DFS_StaMgntResetVars(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u1Byte			i;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaMgntResetVars()\n"));

	if(Adapter->DFSSupport == FALSE)
		return;

	if(pMgntDFS->staMode.bDisableDfs)
		return;

	pMgntDFS->staMode.dfsState = DFS_STA_LISTEN;

	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		pMgntDFS->staMode.dfsRadarChnl[i].radarChnl = 0;
		pMgntDFS->staMode.dfsRadarChnl[i].rxBcnTime = 0;
	}
	
	pMgntDFS->staMode.bDisableTx = FALSE;
	pMgntDFS->staMode.dfsSwitchChannel = 0;
	pMgntDFS->staMode.dfsSwitchChCountDown = 0;
	pMgntDFS->staMode.bMonitorAfterSwitch = FALSE;
	pMgntDFS->staMode.dfsMonitorStartTime = 0;
}

VOID
DFS_StaCheckCsaInfo(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;

	if (Adapter->DFSSupport == FALSE)
		return;

	if(pMgntDFS->staMode.bDisableDfs)
		return;
	
	if(pBssDesc->CSA.bWithCSA)
	{	
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], OnBeacon_Bss(), CSA info, Mode/ Newchnl/ Cnt = %d/ %d/ %d !!!\n", 
			pBssDesc->CSA.channelSwtichMode, pBssDesc->CSA.NewChnlNum, pBssDesc->CSA.channelSwitchCnt));

		if(pMgntDFS->staMode.dfsState == DFS_STA_LISTEN)
		{
			pMgntDFS->staMode.dfsSwitchChannel = pBssDesc->CSA.NewChnlNum;
			pMgntDFS->staMode.dfsSwitchChCountDown = pBssDesc->CSA.channelSwitchCnt;
			PlatformScheduleWorkItem(&(pMgntInfo->DFSMgnt.staMode.csaWorkItem));
		}
	}
}


VOID
DFS_StaConstructAssociateReq(
	IN	PADAPTER		Adapter,
	IN	u2Byte			asocCap,
	IN	OCTET_STRING	AsocReq
)
{
	PDFS_MGNT				pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	OCTET_STRING			PowerCap;
	u1Byte					PowerCapConnent[2];
	PRT_CHANNEL_LIST		pChannelList;
	OCTET_STRING			SupportChannelIE;
	u1Byte					SupportChannelConnent[48];
	u1Byte					i=0,j=0;

	if (Adapter->DFSSupport == FALSE)
		return;

	if(pMgntDFS->staMode.bDisableDfs)
		return;
	
	//for 11H, add Power Capability.
	//For 11H,add Supported Channels element 
	if(asocCap & BIT8)
	{
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaConstructAssociateReq()\n"));

		PowerCapConnent[0]= 0;
		PowerCapConnent[1] = 12;
		PowerCap.Octet = PowerCapConnent;
		PowerCap.Length = 2;
		PacketMakeElement( &AsocReq, EID_PowerCap, PowerCap );
		
		pChannelList = MgntActQuery_ChannelList(Adapter);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("@@@Channel Plan: "));
		for(i = 0; i < pChannelList->ChannelLen; i ++)
			RT_TRACE(COMP_SCAN, DBG_LOUD, (" %d ", pChannelList->ChnlListEntry[i].ChannelNum));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));
		for(i=0;i < pChannelList->ChannelLen*2;i++)
		{
			if(i%2 == 0)
				SupportChannelConnent[i] = pChannelList->ChnlListEntry[i/2].ChannelNum;
			else
				SupportChannelConnent[i] = 1;
		}
		
		//FillOctetString(SupportChannelIE, SuppRatesContent, i*2);
		SupportChannelIE.Octet = SupportChannelConnent;
		SupportChannelIE.Length = i;
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("@@@SUpport channel IE: "));
		for(j = 0; j < SupportChannelIE.Length; j++)
			RT_TRACE(COMP_SCAN, DBG_LOUD,(" %d",SupportChannelIE.Octet[j]));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));
		PacketMakeElement( &AsocReq, EID_SupportedChannels, SupportChannelIE );
	}

}


VOID
DFS_StaGetValueFromBeacon(
	IN		PADAPTER		Adapter,
	IN		OCTET_STRING	mmpdu,
	OUT		RT_WLAN_BSS	*bssDesc
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	OCTET_STRING	ChlSwitchAnnounceInfo;
	
	if (Adapter->DFSSupport == FALSE)
		return;
	
	if (pMgntDFS->staMode.bDisableDfs)
		return;

	ChlSwitchAnnounceInfo = PacketGetElement(mmpdu, EID_ChlSwitchAnnounce, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( ChlSwitchAnnounceInfo.Length != 0)
	{
		bssDesc->CSA.bWithCSA= TRUE;
		bssDesc->CSA.channelSwitchCnt = EF1Byte(*(ChlSwitchAnnounceInfo.Octet+2));
		bssDesc->CSA.NewChnlNum= EF1Byte(*(ChlSwitchAnnounceInfo.Octet+1));
		bssDesc->CSA.channelSwtichMode = EF1Byte(*(ChlSwitchAnnounceInfo.Octet));
		RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], GetValueFromBeaconOrProbeRsp(), CSA info, Mode/ Newchnl/ Cnt = %d/ %d/ %d !!!\n", 
			bssDesc->CSA.channelSwtichMode, bssDesc->CSA.NewChnlNum, bssDesc->CSA.channelSwitchCnt));
	}
	else
	{
		bssDesc->CSA.bWithCSA= FALSE;
		bssDesc->CSA.channelSwitchCnt = 0;
		bssDesc->CSA.NewChnlNum= 0;
		bssDesc->CSA.channelSwtichMode = 0;
	}
}

VOID
DFS_StaMonitor(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u8Byte			currTime, diffTime;

	if (Adapter->DFSSupport == FALSE)
		return;

	if (pMgntDFS->staMode.bDisableDfs)
		return;

	if(ACTING_AS_AP(Adapter))
		return;
	
	if(pMgntDFS->staMode.bMonitorAfterSwitch)
	{
		currTime = PlatformGetCurrentTime();
		diffTime = currTime - pMgntDFS->staMode.dfsMonitorStartTime;

		if(PlatformDivision64(diffTime,1000000) <= 3)
		{
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaMonitor(), monitor started!!\n"));
		}
		
		if(PlatformDivision64(diffTime,1000000) < DFS_STA_CHNL_MONITOR_TIME)
		{
			if( Adapter->MgntInfo.LinkDetectInfo.NumRecvBcnInPeriod==0 ||
				Adapter->MgntInfo.LinkDetectInfo.NumRecvDataInPeriod==0 )
			{				
				Adapter->MgntInfo.LinkDetectInfo.NumRecvBcnInPeriod = 1;
				Adapter->MgntInfo.LinkDetectInfo.NumRecvDataInPeriod= 1;	
			}
		}
		else
		{
			pMgntDFS->staMode.bMonitorAfterSwitch = FALSE;
			pMgntDFS->staMode.bMonitorAfterSwitchIsDone = TRUE;
			dfs_StaEnableTx(Adapter);
			RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaMonitor(), monitor is finished, over %d seconds!!\n", 
				DFS_STA_CHNL_MONITOR_TIME));
		}
	}
}



VOID
DFS_StaCheckRadarChnl(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pbssdesc
)
{
	PDFS_MGNT		pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	u1Byte			i, chnl=0;

	if (Adapter->DFSSupport == FALSE)
		return;

	if (pMgntDFS->staMode.bDisableDfs)
		return;
	
#if (DFS_CLIENT_PASSIVE_SCAN_ONLY == 1)
	return;
#endif

	chnl = pbssdesc->ChannelNumber;

	if(!DFS_5G_RADAR_CHANNEL(chnl))
		return;

	RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaCheckRadarChnl(), rx beacon, channel =  %d!!\n", 
		pbssdesc->ChannelNumber));
	
	if(pbssdesc->CSA.bWithCSA)
	{
		// Beacon with Channel Switch Annouence, set rx beacon time = 0.
		// We take it as the Radar signal is detected in the channel.
		// Update the rx beacon time.
		for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
		{
			if(pMgntDFS->staMode.dfsRadarChnl[i].radarChnl == chnl)
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaCheckRadarChnl(), CSA beacon, set rx beacon time = 0 at channel =  %d!!\n", 
					chnl));
				pMgntDFS->staMode.dfsRadarChnl[i].rxBcnTime = 0;
			}
		}
	}
	else
	{	
		// Update the rx beacon time.
		for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
		{
			if(pMgntDFS->staMode.dfsRadarChnl[i].radarChnl == chnl)
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaCheckRadarChnl(), update rx beacon time at channel =  %d!!\n", 
					chnl));
				pMgntDFS->staMode.dfsRadarChnl[i].rxBcnTime = PlatformGetCurrentTime();
			}
		}
	}

}


VOID
DFS_StaUpdateRadarChnlScanType(
	IN	PADAPTER	Adapter
	)
{
	PDFS_MGNT			pMgntDFS = &Adapter->MgntInfo.DFSMgnt;
	PRT_CHANNEL_LIST	pChanneList = NULL;
	u8Byte				currTime, diffTime;
	u2Byte				i, j;
	u1Byte				radarChnl, ch_idx=0xff;

	if (Adapter->DFSSupport == FALSE)
		return;

	if (pMgntDFS->staMode.bDisableDfs)
		return;

#if (DFS_CLIENT_PASSIVE_SCAN_ONLY == 1)
	return;
#endif

	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChanneList);

	currTime = PlatformGetCurrentTime();
	for(i=0; i<DFS_MAX_RADAR_CHNL_NUM; i++)
	{
		if(pMgntDFS->staMode.dfsRadarChnl[i].rxBcnTime)
		{
			radarChnl = pMgntDFS->staMode.dfsRadarChnl[i].radarChnl;
			diffTime = currTime - pMgntDFS->staMode.dfsRadarChnl[i].rxBcnTime;

			for(j=0; j<pChanneList->ChannelLen; j++)
			{
				if(pChanneList->ChnlListEntry[j].ChannelNum == radarChnl)
				{
					ch_idx = (u1Byte)j;
					RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaUpdateRadarChnlScanType(), find channel-%d in channel plan!!", 
						radarChnl));
					break;
				}
			}
			if(ch_idx == 0xff)
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("[DFS], DFS_StaUpdateRadarChnlScanType(), channel-%d NOT found in channel plan!!\n", 
					radarChnl));
				return;
			}
			
			if(PlatformDivision64(diffTime,1000000) < DFS_STA_CHNL_SCAN_TYPE_MONITOR_TIME)
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, (" and set to Active scan!!\n"));
				pChanneList->ChnlListEntry[ch_idx].ScanType = SCAN_ACTIVE;			
			}
			else
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, (" and set to Passive scan!!\n"));
				pChanneList->ChnlListEntry[ch_idx].ScanType = SCAN_PASSIVE;
			}
		}
	}	

}

#endif

/* End of DFS.c */

