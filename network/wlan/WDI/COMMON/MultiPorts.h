
#ifndef __INC_MULTIPORTS_H
#define __INC_MULTIPORTS_H


//============================================================================
// All Type Port Supported 
//============================================================================

#define GetDefaultAdapter(_Adapter)		((_Adapter)->pPortCommonInfo->pDefaultAdapter)
#define GetDefaultMgntInfo(_Adapter)		((PMGNT_INFO)&(GetDefaultAdapter(_Adapter)->MgntInfo))
#define IsDefaultAdapter(_Adapter)		(GetDefaultAdapter(_Adapter) == (_Adapter))
#define GetAdapterByMgntInfo(_pMgntInfo)	 ((PADAPTER)(((pu1Byte)(_pMgntInfo)) - FIELD_OFFSET(ADAPTER, MgntInfo)))

VOID
MultiPortSetAllPortsHWReadyStatus(
	PADAPTER	pAdapter,
	BOOLEAN		bReady
);


#if (MULTIPORT_SUPPORT == 1)

//============================================================================
// Multiple Port Supported 
//============================================================================

#define GetFirstExtAdapter(_Adapter) 				GetNextExtAdapter(GetDefaultAdapter(_Adapter))
#define IsFirstGoAdapter(_Adapter)				(_Adapter==GetFirstGOPort(_Adapter))
		
typedef struct _MULTIPORT_COMMON_CONTEXT {

	// Only for active extension adapters
	u1Byte				uNumberOfActiveExtAdapters;

	// For default port only: The RFDs will be used to clone the input RFDs (no physical shared memory allocated).
	MEMORY_BUFFER		CloneRfdMemoryBuffer;	// The memory for the allocated RFD structures
	u4Byte				uNumberOfCloneRfds;	// The total number of Clone RFDs
	
	RT_LIST_ENTRY		CloneRfdIdleQueue;
	u4Byte				uCloneRfdIdleQueueSize;
	
	RT_LIST_ENTRY		CloneRfdBusyQueue;
	u4Byte				uCloneRfdBusyQueueSize;

} MULTIPORT_COMMON_CONTEXT, *PMULTIPORT_COMMON_CONTEXT;

#define MULTIPORT_SIZE_OF_COMMON_CONTEXT 		sizeof(MULTIPORT_COMMON_CONTEXT)



typedef struct _MULTIPORT_PORT_CONTEXT {

	RT_LIST_ENTRY		MultiList;
	BOOLEAN				bActiveAdapter;
	PADAPTER			pAdapter;
} MULTIPORT_PORT_CONTEXT, *PMULTIPORT_PORT_CONTEXT;

#define MULTIPORT_SIZE_OF_PORT_CONTEXT 		sizeof(MULTIPORT_PORT_CONTEXT)



BOOLEAN
IsActiveAPModeExist(
	PADAPTER	Adapter
);

BOOLEAN
IsAPModeExist(
	PADAPTER	Adapter
);

BOOLEAN
IsExtAPModeExist(
	PADAPTER	Adapter
	);

BOOLEAN
IsCloneRFD(
	PADAPTER pAdapter,
	PRT_RFD pRfd
);

BOOLEAN
IsDevicePortDiscoverable(
	PADAPTER pAdapter
);

PADAPTER
GetNextExtAdapter(
	PADAPTER	pAdapter
	);

PADAPTER
GetFirstAPAdapter(
	PADAPTER	pAdapter
);

PADAPTER
GetFirstDevicePort(
	PADAPTER	pAdapter
);

PADAPTER
GetFirstGOPort(
	PADAPTER	pAdapter
);

PADAPTER
GetFirstClientPort(
	PADAPTER	pAdapter
);

PADAPTER
MultiPortGetIdleExtAdapter(
	PADAPTER	pAdapter
);

PADAPTER
MultiPortRemoveExtAdapter(
	PADAPTER pAdapter
);

BOOLEAN
MultiPortInsertIntoTargetAdapterList(
	IN PADAPTER			TargetAdapter, 
	OUT PADAPTER		TargetList[],
	OUT pu4Byte 			puCurrentTarget,
	IN u4Byte			uMaxTarget
);

u4Byte
MultiPortGetTargetAdapterList(
	PADAPTER		pAdapter,
	PRT_RFD			pRfd,
	OCTET_STRING		osFrame,
	PADAPTER		TargetList[],
	u4Byte			uMaxTarget
);

PADAPTER
MultiPortFeedPacketToMultipleAdapter(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
);

VOID
MultiPortReturnCloneRFD(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
);

VOID
MultiPortDumpPortStatus(
	PADAPTER pAdapter
);

RT_STATUS
MultiPortAllocateCloneRfdBuffer(
	PADAPTER pAdapter,
	PRT_RFD pRfd
);

VOID
MultiPortReleaseCloneRfdBuffer(
	PRT_RFD pRfd
);

VOID
MultiPortInitializeCloneRfdQueue(
	PADAPTER	pAdapter
);

RT_STATUS
MultiPortPrepareCloneRfd(
	PADAPTER	pAdapter
);

VOID
MultiPortFreeCloneRfd(
	PADAPTER	pAdapter
);

VOID
MultiPortInitializeContext(
	PADAPTER pDefaultAdapter
);


BOOLEAN
MultiPortCanCheckCheckBssid(
	PADAPTER	pAdapter
);

VOID
MultiPortInsertIdleExtAdapter(
	PADAPTER	pDefaultAdapter,
	PADAPTER 	pAdapter
);

VOID
MultiPortChangeExtAdapterActiveState(
	PADAPTER 	pAdapter,
	BOOLEAN		bActive
);

u4Byte
MultiPortGetNumberOfActiveExtAdapters(
	PADAPTER pAdapter
);

BOOLEAN
MultiPortIsPreallocatedExtAdapterExist(
	PADAPTER pDefaultAdapter
);

BOOLEAN
MultiPortIsActiveExtAdapter(
	PADAPTER pAdapter
);
BOOLEAN
IsMultiPortAllowDisableHWSecurity(
	PADAPTER	Adapter
	);

u8Byte
MultiportGetLastConnectionActionTime(
	IN	PADAPTER	pAdapter
	);

#else 

//============================================================================
// Single Port Supported 
//============================================================================

#define IsActiveAPModeExist(_Adapter)			AP_DetermineAlive(_Adapter)
#define IsAPModeExist(_Adapter)					ACTING_AS_AP(_Adapter)
#define IsExtAPModeExist(_Adapter)				FALSE
#define IsDevicePortDiscoverable(_pAdapter) 		FALSE
#define IsCloneRFD(_pAdapter, _pRfd)				FALSE
#define IsFirstGoAdapter(_Adapter)				FALSE

#define GetFirstAPAdapter(_Adapter) 				(ACTING_AS_AP(_Adapter) ? (_Adapter) : NULL)
#define GetFirstExtAdapter(_Adapter)				NULL
#define GetFirstDevicePort(_Adapter)				NULL
#define GetFirstGOPort(_Adapter)					NULL
#define GetFirstClientPort(_Adapter)				NULL
#define GetNextExtAdapter(_Adapter)				NULL

#define MultiPortAllocateCloneRfdBuffer(_pAdapter, _pRfd) 				RT_STATUS_FAILURE
#define MultiPortReleaseCloneRfdBuffer(_pRfd)
#define MultiPortReturnCloneRFD(_pAdapter, _pRfd)
#define MultiPortRemoveExtAdapter(_pAdapter)							NULL
#define MultiPortInitializeContext(_pAdapter)						
#define MultiPortCanCheckCheckBssid(_pAdapter)						TRUE
#define MultiPortIsPreallocatedExtAdapterExist(_Adapter)				FALSE
#define MultiPortInsertIdleExtAdapter(_pDefaultAdapter, _pAdapter)
#define MultiPortChangeExtAdapterActiveState(_pAdapter, _bActive)
#define MultiPortGetNumberOfActiveExtAdapters(_pAdapter)				0
#define MultiPortIsActiveExtAdapter(_pAdapter)							FALSE
#define IsMultiPortAllowDisableHWSecurity(_pAdapter)						TRUE
#define	MultiportGetLastConnectionActionTime(_pAdapter)((_pAdapter)->LastConnectionActionTime)
#endif


VOID
MultiportRecordLastScanTime(
	IN	PADAPTER	pAdapter
);


#endif
