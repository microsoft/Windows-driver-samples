#ifndef __INC_DRIVERINTERFACEDEF_H
#define __INC_DRIVERINTERFACEDEF_H

#define NicIFGetVenderDescription(_Adapter)				\
			(_Adapter)->HalFunc.pDescString
#define NicIFEnableInterrupt(_Adapter)					\
			(_Adapter)->HalFunc.EnableInterruptHandler(_Adapter)
#define NicIFDisableInterrupt(_Adapter)					\
			(_Adapter)->HalFunc.DisableInterruptHandler(_Adapter)
#define NicIFClearInterrupt(_Adapter)					\
			(_Adapter)->HalFunc.ClearInterruptHandler(_Adapter)
#define NicIFResetAllTimer(_Adapter)\
			(_Adapter)->HalFunc.HalResetAllTimerHandler(Adapter);

#define NicIFInterruptRecognized(_Adapter,_pContent, _ContnetLen)	\
			(_Adapter)->HalFunc.InterruptRecognizedHandler(_Adapter,_pContent, _ContnetLen)

#define NicIFHaltAdapter(_Adapter, _bReset)		\
			(_Adapter)->HalFunc.HaltAdapterHandler(_Adapter, _bReset);

#define NicIFSendPacket(_Adapter, _pTcb)					\
			PreTransmitTCB(_Adapter, _pTcb)

#define NicIFSetMacAddress(_Adapter, pMacAddr)			\
			MgntActSet_802_3_MAC_ADDRESS(_Adapter, pMacAddr)

#define	NicIFShutdown(_Adapter,_bReset)	\
		(_Adapter)->HalFunc.ShutdownHandler(_Adapter,_bReset);

//
// 1. Each Platform shall provide a 2 sec timer to invoke NicIFLinkStatusWatchdog.
// 2. Currently, both MgntLinkStatusWatchdog and HalDmWatchDog are designed to 
//    be callbacked in 2 sec period. We might want to seperate it in future to 
//    provide more flexible setting on callback period.
//
// 061010, by rcnjko.
// 	
#define NicIFLinkStatusWatchdog(_Adapter)		\
			MgntLinkMultiPortStatusWatchdog(_Adapter);	\
			HalDmWatchDog(_Adapter)

#define NicIFSetLinkStatus(_Adapter,_mStatus)	\
			MgntIndicateMediaStatus(_Adapter, _mStatus, FORCE_INDICATE )

#define NicIFGetLinkStatus(_Adapter)	\
			MgntLinkStatusQuery( _Adapter)

#define NicIFAMSDUSendPacket(_Adapter, _pTcb)	\
			AMSDU_InsertAggregationList(_Adapter, _pTcb)

RT_STATUS
NicIFAssociateNIC(
	PADAPTER		Adapter,
	u2Byte			HardwareType
	);

VOID
NicIFDisassociateNIC(
	PADAPTER		Adapter
	);

VOID
NicIFInitResource(
	PADAPTER		Adapter
	);

VOID
NicIFDeInitResource(
	PADAPTER		Adapter
	);

VOID
NicIFReadAdapterInfo(
	PADAPTER		Adapter
	);

RT_STATUS
NicIFAllocateMemory(
	PADAPTER		Adapter
	);

VOID
NicIFFreeMemory(
	PADAPTER		Adapter
	);

VOID
NicIFResetMemory(
	PADAPTER		Adapter
);

RT_STATUS
NicIFInitializeAdapter(
	PADAPTER		Adapter
	);

VOID
NicIFHandleInterrupt(
	PADAPTER		Adapter
	);

RESET_TYPE
NicIFCheckResetOrNot(
	PADAPTER		Adapter
	);

VOID
NicIFCoalesceReceivedPacketAndFreeUnusedRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd	
	);

VOID
NicIFReturnPacket(
	PADAPTER		Adapter,
	PRT_RFD			pRfd	
	);

VOID
NicIFCancelAllTimer(
	PADAPTER		Adapter
	);


//1 Following interface functions should be provided by platform driver

RT_STATUS
DrvIFAssociateRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	);

VOID
DrvIFDisassociateRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	);

VOID
DrvIFIndicatePackets(
	PADAPTER				Adapter,
	PRT_RFD					*pRfd_array,
	u2Byte					Num
	);

VOID
DrvIFD0RxIndicatePackets(
	PADAPTER				Adapter,
	PRT_RFD					*pRfd_array,
	u2Byte					Num
	);

VOID
DrvIFIndicatePacket(
	PADAPTER				Adapter,
	PRT_RFD					pRfd
	);

VOID
DrvIFIndicateMultiplePackets(
	PADAPTER				Adapter,
	PRT_RFD					pRfd
	);

VOID
DrvIFCompletePacket(
	PADAPTER		Adapter,
	PRT_TCB			pTcb,
	RT_STATUS		status
	);

VOID
DrvIFIndicateScanStart(
	PADAPTER		Adapter
);

VOID
DrvIFIndicateScanComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
);

VOID
DrvIFIndicateConnectionStart(
	PADAPTER		Adapter
);

VOID
DrvIFIndicateConnectionComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
);

VOID
DrvIFIndicateAssociationStart(
	PADAPTER		Adapter
);

VOID
DrvIFIndicateAssociationComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
);

VOID
DrvIFIndicateDisassociation(
	PADAPTER		Adapter,
	u2Byte			reason,
	pu1Byte			pAddr
);

VOID
DrvIFIndicateRoamingStart(
	PADAPTER		Adapter
);

VOID
DrvIFIndicateRoamingComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
);

VOID
DrvIFIndicateCurrentPhyStatus(
	PADAPTER		Adapter
);

VOID
DrvIFIndicateIncommingAssociationStart(
	PADAPTER		Adapter
	);

VOID
DrvIFIndicateIncommingAssociationComplete(
	PADAPTER		Adapter,
	u2Byte			status
	);

VOID
DrvIFIndicateIncommingAssocReqRecv(
	PADAPTER		Adapter
	);

RT_STATUS
NicIFDisableNIC(
	PADAPTER		Adapter
);

RT_STATUS
NicIFEnableNIC(
	PADAPTER		Adapter
);

RT_STATUS
NicIFResetNIC(
	PADAPTER		Adapter
);

VOID
DrvIFDeleteDatapathPeer(
	IN	PADAPTER	pAdapter,
	IN	pu1Byte		pAddr
);

VOID
DrvIFCheckTxCredit(
	PADAPTER		pAdapter
);

VOID
DrvIFIndicateDataInQueue(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					ExTid,
	IN	u2Byte					PeerId,
	IN	RT_RX_INDICATION_LEVEL	level
);

VOID
DrvIFIndicateRoamingNeeded(
	PADAPTER				pAdapter,
	RT_PREPARE_ROAM_TYPE	IndicationReason
);

VOID
DrvIFIndicateLinkStateChanged(
	PADAPTER		pAdapter,
	BOOLEAN			bForceLinkQuality,
	u1Byte			ucLinkQuality
);


VOID
DrvIFIndicateFWStalled(
	PADAPTER		Adapter
);

#endif // #ifndef __INC_DRIVERINTERFACEDEF_H

