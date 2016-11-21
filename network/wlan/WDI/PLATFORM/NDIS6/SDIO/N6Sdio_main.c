/*++

Copyright (c) 2003 Realtek Semiconductor Corporation

Module Name:
	N6Sdio_main.c
	
Abstract:
	This is an NDIS miniport driver can interface with a SDIO device. 
	Added by Roger, 2010.12.15.

--*/

#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N6Sdio_Main.tmh"
#endif

#if SOFTWARE_TRACE_LOGGING

//Define Tracelogging provider name
// {6F651B84-F358-4486-BF42-A8D60D911DC6}
TRACELOGGING_DEFINE_PROVIDER(g_hProvider,
    "RealtekWindowsWiFi",
    (0x6f651b84, 0xf358, 0x4486, 0xbf, 0x42, 0xa8, 0xd6, 0xd, 0x91, 0x1d, 0xc6));

#endif

#include "CppDrvReg.h"
#include "802_11_OID.h"

extern u8Byte	RTL_AllocateMemory_count;
extern u8Byte	RTL_FreeMemory_count;
extern u8Byte	RTL_AllocateMemory_Len;
extern u8Byte	RTL_FreeMemory_Len;

extern u8Byte	RTL_P2P_AllocateMemory_count ;
extern u8Byte	RTL_P2P_FreeMemory_count ;
extern u8Byte	RTL_P2P_AllocateMemory_Len;
extern u8Byte	RTL_P2P_FreeMemory_Len;

PCALLBACK_OBJECT       ProximityCallback = NULL;

//sherry added for dual mac 20110627
RT_LIST_ENTRY     GlobalAdapterList;
BOOLEAN	 GlobalFirstConfigurationForNormalChip = TRUE;
ULONG    GlobalMutexForGlobalAdapterList = 0;
GLOBAL_SPINLOCK    GlobalSpinlockForGlobalAdapterList;
ULONG		GlobalMutexForPowerAndEfuse = 0;
ULONG		GlobalMutexForPowerOnAndPowerOff = 0;
ULONG		GlobalMutexForFwDownload = 0;
ULONG		GlobalMutexForMac0_2G_Mac1_5G = 0;
GLOBAL_SPINLOCK		GlobalSpinLockForDusc;

//
// Global handle which initialized in DriverEntry.
//

#define MP_OFFSET(field) ((u4Byte)FIELD_OFFSET(RT_SDIO_DEVICE,field))
#define MP_SIZE(field) sizeof(((PRT_SDIO_DEVICE)0)->field)

static MP_REG_ENTRY NICRegTable[] = {
	// Registry value name						Type					Offset in MP_ADAPTER			Field size							Default Value		Min		Max
	//----------------------------------------------------------------------------
	// 8187 SW LED.
	{NDIS_STRING_CONST("LedCtrl"),				0,NdisParameterInteger,	MP_OFFSET(bRegLedCtrl),			MP_SIZE(bRegLedCtrl),					0,					0,		8},
	//----------------------------------------------------------------------------
	// Number of Bulk in issued at initialization, 2006.02.09, by rcnjko.
	{NDIS_STRING_CONST("MaxBulkInCnt"),	0,NdisParameterInteger,	MP_OFFSET(nMaxBulkInCnt),		MP_SIZE(nMaxBulkInCnt),			32,					1,		63}, // 1 ~ number of RFD allocated. 	
	//----------------------------------------------------------------------------

	{NDIS_STRING_CONST("EnableUsbSS"),	0,NdisParameterInteger,	MP_OFFSET(RegUsbSS),		MP_SIZE(RegUsbSS),			0,					0,		1}, // 0: disable, 1: enable. 
	//
	// 2011/09/07 MH Add for different channel plan tx power offset.
	// Middle offset is for ETSI europe low is for MKK/Korea. 
	//
	{NDIS_STRING_CONST("GainOffsetFCCLow"),	0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetFCCLow),		MP_SIZE(RegGainOffsetFCCLow),	0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	{NDIS_STRING_CONST("GainOffsetFCC"),		0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetFCC),		MP_SIZE(RegGainOffsetFCC),		0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	{NDIS_STRING_CONST("GainOffsetFCCHigh"),	0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetFCCHigh),		MP_SIZE(RegGainOffsetFCCHigh),	0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	
	{NDIS_STRING_CONST("GainOffsetMKKLow"),	0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetMKKLow),		MP_SIZE(RegGainOffsetMKKLow),	0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	{NDIS_STRING_CONST("GainOffsetMKK"),		0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetMKK),		MP_SIZE(RegGainOffsetMKK),		0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	{NDIS_STRING_CONST("GainOffsetMKKHigh"),	0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetMKKHigh),		MP_SIZE(RegGainOffsetMKKHigh),	0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.

	{NDIS_STRING_CONST("GainOffsetETSILow"),	0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetETSILow),		MP_SIZE(RegGainOffsetETSILow),	0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	{NDIS_STRING_CONST("GainOffsetETSI"),		0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetETSI),		MP_SIZE(RegGainOffsetETSI),		0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.
	{NDIS_STRING_CONST("GainOffsetETSIHigh"),	0,NdisParameterInteger,	MP_OFFSET(RegGainOffsetETSIHigh),		MP_SIZE(RegGainOffsetETSIHigh),	0x00000000,      0x0,       0xFFFFFFFF}, // 0x0~0xF.

	//
	// 2011/11/25 MH Add for Sercom requirement temporarily.	
	//
	{NDIS_STRING_CONST("InPipeCnt"),	0,NdisParameterInteger,	MP_OFFSET(RegInPipeCnt),		MP_SIZE(RegInPipeCnt),	1,      0x0,       0x5}, // 0x0~0x5.
	//----------------------------------------------------------------------------
	
	//
	// 2015/03/26 TzuHang Add to control SDIO bus speed SDR25 support or not (SDR25 = 50MHz), 0: not supported SDR25 (25MHz), 1: supported SDR25 (50MHz)
	{NDIS_STRING_CONST("SdioBusSDR25"),	0,NdisParameterInteger,	MP_OFFSET(bRegSdioSpeedSDR25),		MP_SIZE(bRegSdioSpeedSDR25),	1,      0x0,       0x1},
	// 2015/03/26 TzuHang Add to control SDIO Driving Capabilities
	{NDIS_STRING_CONST("bSdioDrivingEnable"),	0,NdisParameterInteger,	MP_OFFSET(bRegSdioDrivingEnable),		MP_SIZE(bRegSdioDrivingEnable),	0,      0x0,       0x1},
	{NDIS_STRING_CONST("SdioDriving"),	0,NdisParameterInteger,	MP_OFFSET(RegSdioDriving),		MP_SIZE(RegSdioDriving),	0,      0x0,       0x7},
	// 2015/03/26 TzuHang Add to control SurpriseRemoved while facing I/O Fail
	{NDIS_STRING_CONST("bSurpriseRemovedEnable"),	0,NdisParameterInteger,	MP_OFFSET(bRegSurpriseRemovedEnable),		MP_SIZE(bRegSurpriseRemovedEnable),	1,      0x0,       0x1},
	//----------------------------------------------------------------------------
};

#define NIC_NUM_REG_PARAMS (((ULONG)sizeof (NICRegTable)) / ((ULONG)sizeof(MP_REG_ENTRY)))

NDIS_STATUS
DriverEntry(
	IN PDRIVER_OBJECT  pDriverObject,
	IN PUNICODE_STRING RegistryPath
	) 
{
	NDIS_STATUS								Status = NDIS_STATUS_SUCCESS;
	NDIS_MINIPORT_DRIVER_CHARACTERISTICS	MChars;
	u4Byte									NdisVersion  =0;
	UNICODE_STRING 							dest;
#if USE_WDF_SDIO
	NTSTATUS								ntStatus;
	WDF_DRIVER_CONFIG						config;
	WDFDRIVER								hDriver; //vm control
#endif

#if WPP_SOFTWARE_TRACE
	WPP_INIT_TRACING(pDriverObject, RegistryPath);
#endif

#if SOFTWARE_TRACE_LOGGING
	// In DriverEntry or DLLMain, register the TraceLogging provider 
	// No need to check for errors. If this call fails all subsequent TraceLogging calls automatically become no-ops.
	TraceLoggingRegister(g_hProvider);

	TraceLoggingWrite(
		g_hProvider,
		"RealtekWDIDriverTraceLogging",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(pDriverObject,"pDriverObject"),
		TraceLoggingUnicodeString(RegistryPath, "RegistryPath"));
#endif

	RT_TRACE(COMP_INIT, DBG_LOUD,  (" ==>DriverEntry()\n") );
	DBG_ConsoleInitialize();

	PLATFORM_INIT_RT_SPINLOCK(GlobalRtDriverContext.ContextLock);
	RTInitializeListHead(&(GlobalRtDriverContext.AdapterList));
	RTInitializeListHead(&(GlobalRtDriverContext.NdisContext.IoDeviceList));

	GlobalRtDriverContext.NdisContext.pDriverObject = pDriverObject;

	RtlInitEmptyUnicodeString(&dest , GlobalRtDriverContext.NdisContext.RegistryPath , 256*sizeof(WCHAR));
	RtlCopyUnicodeString(&dest , RegistryPath);
	DBG_RecordSvnVersion();

	do 
	{
#if USE_WDF_SDIO
		//
		// Set WdfDriverInitNoDispatchOverride flag to tell the framework
		// not to provide dispatch routines for the driver. In other words,
		// the framework must not intercept IRPs that the I/O manager has
		// directed to the driver. In this case, it will be handled by NDIS
		// port driver.
		//
		WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
		config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;
		ntStatus = WdfDriverCreate(
					pDriverObject,
					RegistryPath,
					WDF_NO_OBJECT_ATTRIBUTES,
					&config,                
					&hDriver); //vm control
		if(!NT_SUCCESS(ntStatus))
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("WdfDriverCreate failed\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}
#endif 
		
		NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS));
		NdisVersion = NdisGetVersion();

		if(NdisVersion >= NDIS_VERSION_BASE_6_50)
		{
			//
			// WDI driver need at least NDIS 6.50
			//
			RT_TRACE_F(COMP_INIT, DBG_LOUD, ("Ndis 6.5 configuration\n"));
			
			//
			// Set the type and version of this structure
			//
			MChars.Header.Type      = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
			MChars.Header.Size       = sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS);
			MChars.Header.Revision  = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;

			//
			// Version
			//	
			// The version of the NDIS library. Current version is 6.50.
			MChars.MajorNdisVersion = NDIS_MAJOR_VERSION;	
			MChars.MinorNdisVersion = NDIS_MINOR_VERSION_5;		

		}
		else
		{
			//
			// Check NDIS version, 2006.10.31, by shien chang.
			//
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("NDIS version mismatch\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		// Reserved for the version number of the driver.
		MChars.MajorDriverVersion = DRIVER_MAJOR_VERSION;	
		MChars.MinorDriverVersion = DRIVER_MINOR_VERSION;

		Status = WDI_Initialize(pDriverObject, RegistryPath, &MChars);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("WDI_Initialize(), status 0x%x\n", Status));

		if (Status != NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Failed to register miniport with NDIS. Status = 0x%x\n", Status));
			break;
		}
		RTInitializeListHead(&GlobalAdapterList);	

	}while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Driver Entry Return Fail = 0x%x\n", Status));

#if SOFTWARE_TRACE_LOGGING
		TraceLoggingWrite(
			g_hProvider,
			"WDI Sample Driver Entry Failed",
			TraceLoggingLevel(TRACE_LEVEL_FATAL),
			TraceLoggingHexUInt32(Status, "Status"));

		TraceLoggingUnregister(g_hProvider);
#endif
		
#if WPP_SOFTWARE_TRACE
		WPP_CLEANUP(pDriverObject);
#endif
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_TRACE, ("Driver Entry Return Success = 0x%x\n", Status));
		/* Add for DBG ver. memory alloc/free, 2014.04.24, hsiao_ho */
		RtDbgInitMem();
		/* Add for DBG ver. Mutex, 2014.05.19, hsiao_ho */
		RtDbgInitMutex();
		/* Add for DBG ver. Spinlock, 2014.05.21, hsiao_ho */
		RtDbgInitSpinlock();
	}

	return Status;
}
	

RT_STATUS
N6sdioAllocateAdapterSketchAndStoreHandle(
	NDIS_HANDLE		MiniportAdapterHandle,
	PVOID			*pPtr,
	u4Byte			length
	)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;
	PADAPTER	Adapter = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = NULL;


	
	// Allocate the adapter structure.
	status = PlatformAllocateMemory(
			NULL, 
			(PVOID*)&Adapter, 
			sizeof(ADAPTER)
		);
	if (status != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6PciAllocateAdapterSketchAndStoreHandle(): failed to allocate Adapter!!!\n"));
		return RT_STATUS_FAILURE;
	}
	else
	{
		PlatformZeroMemory(Adapter, sizeof(ADAPTER));
	}

	// Allocate RT_NDIS6_COMMON.
	status = PlatformAllocateMemory(
			NULL, 
			(PVOID)&(pNdisCommon), 
			sizeof(RT_NDIS6_COMMON)
		);
	if (status != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6PciAllocateAdapterSketchAndStoreHandle(): failed to allocate RT_NDIS6_COMMON for the Adapter!!!\n"));
		PlatformFreeMemory(Adapter, sizeof(ADAPTER));
		return RT_STATUS_FAILURE;
	}
	else
	{
		PlatformZeroMemory(pNdisCommon, sizeof(RT_NDIS6_COMMON));
	}

	//
	// Store Handle.
	//
	pNdisCommon->hNdisAdapter = MiniportAdapterHandle;
	Adapter->pNdisCommon = pNdisCommon;

	*pPtr = Adapter;
	return status;
}

NDIS_STATUS
N6sdioAllocateAdapter(
	OUT	PADAPTER		*pAdapter,
	IN	NDIS_HANDLE		MiniportAdapterHandle
	)
{
	PADAPTER		Adapter;
	RT_STATUS		status;

	*pAdapter = NULL;
	do
	{
		status=N6sdioAllocateAdapterSketchAndStoreHandle(MiniportAdapterHandle, (PVOID*)(&Adapter), sizeof(ADAPTER));
		if(status!=RT_STATUS_SUCCESS)
		{
			return NDIS_STATUS_FAILURE;
		}

		// Port Common Info -------------------------------------
		status = PortCommonInfoAllocateMemoryWithCriticalInitialization(Adapter);
		if(status != RT_STATUS_SUCCESS)
		{
			PlatformFreeMemory( Adapter->pNdisCommon, sizeof(RT_NDIS6_COMMON) );
			PlatformFreeMemory( Adapter, sizeof(ADAPTER) );
			return NDIS_STATUS_FAILURE;
		}
		// -----------------------------------------------------

		status = MgntAllocMemory(Adapter);
		if(status != RT_STATUS_SUCCESS)
		{
			PortCommonInfoFreeMemory(Adapter);
			PlatformFreeMemory( Adapter->pNdisCommon, sizeof(RT_NDIS6_COMMON) );
			PlatformFreeMemory( Adapter, sizeof(ADAPTER) );
			return NDIS_STATUS_FAILURE;
		}
	}while(FALSE);

	*pAdapter = Adapter;

	return NDIS_STATUS_SUCCESS;
}

VOID
N6SdioCleanUpInitializedResourcesEx(
    IN	PADAPTER	Adapter,
    IN	RES_MON_OBJ	ResMonObj
    )
{
    PADAPTER        pDefaultAdapter = GetDefaultAdapter(Adapter);
    PRT_SDIO_DEVICE          device = GET_RT_SDIO_DEVICE(Adapter);
    PRT_NDIS6_COMMON    pNdisCommon = Adapter->pNdisCommon;
    BOOLEAN     isAllocAdapter      = FALSE,
                isAllocRxNBLPool    = FALSE,
                isSpinLock          = FALSE,
                isAWBs              = FALSE,
                isAsocNIC           = FALSE,
                isAllocMem          = FALSE,
                isSdioInit          = FALSE,
                isSdioTxRes         = FALSE,
                isN62CInit          = FALSE,
                isAllocNWifi        = FALSE,
                isAllocSdioRes      = FALSE,
                isBTIODevice        = FALSE,
                isCommonRS          = FALSE;

    u1Byte ResIndexPlueOne=0;
    RM_RES_TYPE	ResType = 0;

    // TODO: remove useless comment code and double check whether there is something missing to free
    for (ResIndexPlueOne=ResMonObj.nResToCheck;
        ResIndexPlueOne != 0;
        ResIndexPlueOne --)
    {
        ResType = ResMonObj.ResItems[ResIndexPlueOne-1];
        //
        // The clean up process should be the same with N6SdioFreeAll
        //
        switch (ResType)
        {
            case InitRM_AllocAdapter:		// 0x1
                isAllocAdapter = TRUE;
                break;

            case InitRM_AllocRxNBLPool:
                isAllocRxNBLPool = TRUE;
                break;

            case InitRM_SpinLock:
                isSpinLock = TRUE;
                break;

            case InitRM_AWBs:
                isAWBs = TRUE;
                break;

            case InitRM_AsocNIC:			// 0x5
                isAsocNIC = TRUE;
                break;

            case InitRM_AllocMem:
                isAllocMem = TRUE;
                break;

            case InitRM_SdioInit:
                isSdioInit = TRUE;
                break;

            case InitRM_SdioTxRes:
                isSdioTxRes = TRUE;
                break;

            case InitRM_N62CInit:
                isN62CInit = TRUE;
                break;

            case InitRM_AllocNWifi:			// 0xa
                isAllocNWifi = TRUE;
                break;

            case InitRM_AllocSdioRes:
                isAllocSdioRes = TRUE;
                break;

            case InitRM_BTIODevice:
                isBTIODevice = TRUE;
                break;

            case InitRM_CommonRS:			// 0xd
                isCommonRS = TRUE;
                break;

            case InitRM_Undefined:			// 0x0
            default:
                break;
        }
    }
    ResMonObj.nResToCheck = 0;

    if(isCommonRS)
    {
        NDBG_Halt(Adapter);
        ReleaseNdis6CommonResources(Adapter);
    }
    Adapter->bDriverIsGoingToUnload = TRUE;

    PoUnregisterPowerSettingCallback(pNdisCommon->N6PowerSettingHandle);
    PoUnregisterPowerSettingCallback(pNdisCommon->N6WLANPowerModeHandle);


    Adapter->bDriverStopped = TRUE;
    N6C_SET_MP_DRIVER_STATE(Adapter, MINIPORT_HALTED);

    if(isAllocNWifi)
        N6SdioFreeNative80211MIBs(Adapter);

	if(isSdioInit)
	{
#if USE_WDF_SDIO
			WdfSdio_Halt(Adapter);
#else
			N6WdmSdio_Disable(Adapter);
#endif
	}

    if(isAllocSdioRes)
        N6SdioFreeRes(Adapter);

    if( isSpinLock)
        N6WaitTimerSync(Adapter);

    if(isAllocMem)
        NicIFFreeMemory(Adapter);

    if(isN62CInit)    
        NDIS_6_2_FREE_EXTENSION_COMPONENT(Adapter);

    if(isSdioTxRes)
    {
        //Do it in N6SdioFreeDevice, skip here
        //N6SdioFreeTxQueueRes(Adapter);
    }

    if(isAsocNIC)
    {
        NicIFCancelAllTimer(Adapter);
        NicIFDeInitResource(Adapter);
        NicIFDisassociateNIC(Adapter);
    }

    if(isAWBs)
    {
        //Do it in N6SdioFreeDevice, skip here
        //FreeSdioAWBs(device, FALSE);
    }

    if(isSpinLock)
    {
        N6CFreeSpinLocks(Adapter);
        N6DeInitTimerSync(Adapter);
        NdisOIDHistoryDeInit(Adapter);	
    }

    if(isAllocAdapter)
    {
        N6SdioFreeDevice(device);
        PortCommonInfoFreeMemory(Adapter);
        N6FreeAdapter(Adapter);
    }
}

//
// Resources cleanup process, revised by Roger.
//
VOID
N6SdioCleanUpInitializedResources(
	IN	PADAPTER	Adapter,
	IN	RM_RES_TYPE	ResType
	)
{
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);
	
	switch (ResType)
	{
	case InitRM_AllocAdapter:
		N6FreeAdapter(Adapter);
		break;
	case InitRM_AllocRxNBLPool:
		N6SdioFreeRxNetBufferListPool(Adapter);
		break;
	case InitRM_SpinLock:
		NdisFreeSpinLock( &(device->IrpSpinLock) );	
		N6CFreeSpinLocks(Adapter);
		NdisOIDHistoryDeInit(Adapter);	
		break;
	case InitRM_AWBs:
		FreeSdioAWBs(device, FALSE);
		break;
	case InitRM_AsocNIC:
		NicIFDeInitResource(Adapter);
		NicIFDisassociateNIC(Adapter);
		break;
	case InitRM_AllocMem:
		NicIFFreeMemory(Adapter);
		break;
	case InitRM_SdioInit:
		
#if USE_WDF_SDIO
		WdfSdio_Halt(Adapter);
#else
		N6WdmSdio_Disable(Adapter);
#endif

		break;
	case InitRM_SdioTxRes:
		N6SdioFreeTxQueueRes(Adapter);
		break;
	case InitRM_AllocNWifi:
		N6SdioFreeNative80211MIBs(Adapter);
		break;
	case InitRM_AllocSdioRes:		
		N6SdioFreeRes(Adapter);		
		break;
	case InitRM_BTIODevice:
		N6C_DeregisterIoDevice(Adapter);
		break;
	case InitRM_CommonRS:
		ReleaseNdis6CommonResources(Adapter);
		break;
	case InitRM_Undefined:
	default:
		break;
	}
}

NDIS_STATUS
N6sdioSetMiniportAttributes(
	IN	PADAPTER		Adapter,
	IN	NDIS_HANDLE		MiniportAdapterHandle
	)
{
	NDIS_MINIPORT_ADAPTER_ATTRIBUTES    MiniportAttributes; 
	
	PlatformZeroMemory(&MiniportAttributes, sizeof(NDIS_MINIPORT_ADAPTER_ATTRIBUTES));

	N6_ASSIGN_OBJECT_HEADER(
			MiniportAttributes.RegistrationAttributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2,
			NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2
		);

	MiniportAttributes.RegistrationAttributes.MiniportAdapterContext = Adapter;
	MiniportAttributes.RegistrationAttributes.AttributeFlags = 
		NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK | 
		NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM |
		NDIS_MINIPORT_ATTRIBUTES_NO_PAUSE_ON_SUSPEND;

	MiniportAttributes.RegistrationAttributes.CheckForHangTimeInSeconds = RT_CHECK_FOR_HANG_PERIOD;
	MiniportAttributes.RegistrationAttributes.InterfaceType = NdisInterfacePNPBus;


	return NdisMSetMiniportAttributes(MiniportAdapterHandle, &MiniportAttributes);
}




VOID
N6SdioFill80211PhyAttributes(
	IN	PADAPTER		Adapter,
	IN	PNDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES	pDot11Attributes
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_PHY_ATTRIBUTES	pPhyAttributes;
	ULONG	i, index;
	UCHAR	rate;
	
	for (i=0, pPhyAttributes = pDot11Attributes->SupportedPhyAttributes ; 
		i<pDot11Attributes->NumSupportedPhys ;
		i++,pPhyAttributes++)
	{
		N6_ASSIGN_OBJECT_HEADER(
			pPhyAttributes->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PHY_ATTRIBUTES_REVISION_1,
			sizeof(DOT11_PHY_ATTRIBUTES));

		pPhyAttributes->PhyType = pNdisCommon->pDot11PhyMIBs[i].PhyType;
		pPhyAttributes->bHardwarePhyState = TRUE;
		pPhyAttributes->bSoftwarePhyState = TRUE;
		pPhyAttributes->bCFPollable = MgntActQuery_CfPollable(Adapter);
		pPhyAttributes->uMPDUMaxLength = NATIVE_802_11_MTU_SIZE;
		pPhyAttributes->TempType = dot11_temp_type_unknown;
		pPhyAttributes->DiversitySupport = dot11_diversity_support_dynamic;

		switch (pPhyAttributes->PhyType)
		{
		case dot11_phy_type_hrdsss:
			pPhyAttributes->HRDSSSAttributes.bShortPreambleOptionImplemented = FALSE;
			pPhyAttributes->HRDSSSAttributes.bPBCCOptionImplemented = FALSE;
			pPhyAttributes->HRDSSSAttributes.bChannelAgilityPresent = FALSE;
			pPhyAttributes->HRDSSSAttributes.uHRCCAModeSupported = FALSE;
			break;

		case dot11_phy_type_ofdm:
			pPhyAttributes->OFDMAttributes.uFrequencyBandsSupported = 
					DOT11_FREQUENCY_BANDS_LOWER | 
					DOT11_FREQUENCY_BANDS_MIDDLE;
			break;

		case dot11_phy_type_erp:
			pPhyAttributes->ERPAttributes.bShortPreambleOptionImplemented = TRUE;
			pPhyAttributes->ERPAttributes.bPBCCOptionImplemented = FALSE;
			pPhyAttributes->ERPAttributes.bChannelAgilityPresent = FALSE;
			pPhyAttributes->ERPAttributes.uHRCCAModeSupported = FALSE;
			pPhyAttributes->ERPAttributes.bERPPBCCOptionImplemented = FALSE;
			pPhyAttributes->ERPAttributes.bDSSSOFDMOptionImplemented = FALSE;
			pPhyAttributes->ERPAttributes.bShortSlotTimeOptionImplemented = TRUE;
			break;

		default:
			break;
		}

		// Tx Power Level.
		pPhyAttributes->uNumberSupportedPowerLevels = 
			pNdisCommon->dot11SupportedPowerLevels.uNumOfSupportedPowerLevels;
		PlatformMoveMemory(
			pPhyAttributes->TxPowerLevels,
			pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues,
			sizeof(pPhyAttributes->TxPowerLevels));
	
		// Data Rate.
		PlatformMoveMemory(
			&(pPhyAttributes->SupportedDataRatesValue),
			&(pNdisCommon->pDot11PhyMIBs[i].SupportedDataRatesValue),
			sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2));
		index = 0;
		while ( (rate = pNdisCommon->pDot11PhyMIBs[i].SupportedDataRatesValue.ucSupportedTxDataRatesValue[index]) != 0)
		{
			pPhyAttributes->DataRateMappingEntries[index].ucDataRateIndex = rate;
			pPhyAttributes->DataRateMappingEntries[index].ucDataRateFlag = 0;
			pPhyAttributes->DataRateMappingEntries[index].usDataRateValue = rate;

			index++;
		}
		pPhyAttributes->uNumDataRateMappingEntries = index;
	}
}

VOID
N6SdioFillHwRxFilterCapabilities(
	IN	PADAPTER		Adapter,
	IN OUT	PNDIS_RECEIVE_FILTER_CAPABILITIES	pRxFilterCapabilities
	)
{

// ADD Header
#if (NDIS_SUPPORT_NDIS630)
	N6_ASSIGN_OBJECT_HEADER(
			pRxFilterCapabilities->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_2,
			NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_2);
#else
	N6_ASSIGN_OBJECT_HEADER(
			pRxFilterCapabilities->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_1,
			NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_1);
#endif
//----------
	pRxFilterCapabilities->Flags = 0;

	pRxFilterCapabilities->SupportedHeaders = NDIS_RECEIVE_FILTER_MAC_HEADER_SUPPORTED;

#if (NDIS_SUPPORT_NDIS630)
	pRxFilterCapabilities->EnabledFilterTypes = NDIS_RECEIVE_FILTER_PACKET_COALESCING_FILTERS_ENABLED;

	pRxFilterCapabilities->SupportedQueueProperties = NDIS_RECEIVE_FILTER_PACKET_COALESCING_SUPPORTED_ON_DEFAULT_QUEUE;
	
	pRxFilterCapabilities->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED|
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_NOT_EQUAL_SUPPORTED; //0;

	pRxFilterCapabilities->SupportedHeaders |= (NDIS_RECEIVE_FILTER_ARP_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV4_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV6_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_UDP_HEADER_SUPPORTED);
	
	pRxFilterCapabilities->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PACKET_TYPE_SUPPORTED;
	
	pRxFilterCapabilities->SupportedARPHeaderFields = NDIS_RECEIVE_FILTER_ARP_HEADER_OPERATION_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_SPA_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_TPA_SUPPORTED ;
	
	pRxFilterCapabilities->SupportedIPv4HeaderFields = NDIS_RECEIVE_FILTER_IPV4_HEADER_PROTOCOL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedIPv6HeaderFields = NDIS_RECEIVE_FILTER_IPV6_HEADER_PROTOCOL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedUdpHeaderFields = NDIS_RECEIVE_FILTER_UDP_HEADER_DEST_PORT_SUPPORTED;
	
	pRxFilterCapabilities->MaxFieldTestsPerPacketCoalescingFilter = 5; // should be >= 5 
	
	pRxFilterCapabilities->MaxPacketCoalescingFilters = 10; // should be >= 10
	//pRxFilterCapabilities->NidsReserved = 0;
#else
	pRxFilterCapabilities->EnabledFilterTypes = 0;

	pRxFilterCapabilities->SupportedQueueProperties = 0;
	
	pRxFilterCapabilities->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED;
#endif

	pRxFilterCapabilities->EnabledQueueTypes = 0; //NDIS_RECEIVE_FILTER_VM_QUEUES_ENABLED;
	pRxFilterCapabilities->NumQueues = 0;
	pRxFilterCapabilities->MaxMacHeaderFilters = 2;
	pRxFilterCapabilities->MaxQueueGroups = 0;
	pRxFilterCapabilities->MaxQueuesPerQueueGroup = 0;
	pRxFilterCapabilities->MinLookaheadSplitSize = 0;
	pRxFilterCapabilities->MaxLookaheadSplitSize = 0;


	RT_PRINT_DATA(COMP_INIT, DBG_LOUD, "pRxFilterCapabilities:\n", pRxFilterCapabilities, sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES));
}


NDIS_STATUS
N6SdioSetHardwareAssistAttributes(
	IN	PADAPTER		Adapter
	)
{
	//NDIS_MINIPORT_ADAPTER_ATTRIBUTES    MiniportAttributes;
	NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES		HardwareAssistAttributes;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo=&(Adapter->MgntInfo);
	NDIS_STATUS		ndisStatus;
#if NDIS_SUPPORT_NDIS620
	NDIS_RECEIVE_FILTER_CAPABILITIES	RxFilterCapabilities;
	NDIS_RECEIVE_FILTER_CAPABILITIES	CurrRxFilterCapabilities;
#endif

	PlatformZeroMemory(&HardwareAssistAttributes, sizeof(NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES));

	if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_1)
	{
		N6_ASSIGN_OBJECT_HEADER(
			HardwareAssistAttributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_1,
			NDIS_SIZEOF_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_1);
	}
#if NDIS_SUPPORT_NDIS620
	else if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_20)
	{
		N6_ASSIGN_OBJECT_HEADER(
			HardwareAssistAttributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_2,
			NDIS_SIZEOF_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_2);
	}
#endif
#if NDIS_SUPPORT_NDIS630
	else if(pNdisCommon->NdisVersion <= NDIS_VERSION_BASE_6_40)
	{
	
		N6_ASSIGN_OBJECT_HEADER(
			HardwareAssistAttributes.Header,
			NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES,
			NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_3,
			NDIS_SIZEOF_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_3);
	}
#endif

	HardwareAssistAttributes.HDSplitAttributes = NULL;
#if NDIS_SUPPORT_NDIS620
	if(pMgntInfo->bSupportPacketCoalescing)
	{
		PlatformZeroMemory(&RxFilterCapabilities, sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES));
		HardwareAssistAttributes.HardwareReceiveFilterCapabilities = &RxFilterCapabilities;
		//MiniportAttributes.HardwareAssistAttributes.CurrentReceiveFilterCapabilities = &RxFilterCapabilities;
		N6SdioFillHwRxFilterCapabilities(Adapter, &RxFilterCapabilities)	;

		PlatformZeroMemory(&CurrRxFilterCapabilities, sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES));
		HardwareAssistAttributes.CurrentReceiveFilterCapabilities = &CurrRxFilterCapabilities;
		N6SdioFillHwRxFilterCapabilities(Adapter, &CurrRxFilterCapabilities)	;
	}
	else
	{
		HardwareAssistAttributes.HardwareReceiveFilterCapabilities = NULL;
		HardwareAssistAttributes.CurrentReceiveFilterCapabilities = NULL;
	}
	HardwareAssistAttributes.HardwareNicSwitchCapabilities = NULL;
	HardwareAssistAttributes.CurrentNicSwitchCapabilities = NULL;
#endif

#if NDIS_SUPPORT_NDIS630
	HardwareAssistAttributes.HardwareSriovCapabilities = NULL;
	HardwareAssistAttributes.CurrentSriovCapabilities = NULL;
	HardwareAssistAttributes.HardwareQosCapabilities = NULL;
	HardwareAssistAttributes.CurrentQosCapabilities = NULL;
#endif

	//
	// Ok, now register the attributes.
	//
	ndisStatus = NdisMSetMiniportAttributes(Adapter->pNdisCommon->hNdisAdapter,
										(PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&HardwareAssistAttributes);	

	RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioSetHardwareAssistAttributes(): ndisStatus: %#X\n", ndisStatus));

	return ndisStatus;
}

//
// Update SDIO dependent default settings.
//
VOID
N6SdioUpdateDefaultSetting(
	IN	PADAPTER		pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);	
	
	HalSdioUdtDefSet(pAdapter);
}

NDIS_STATUS
N6SdioInitialize(
	IN  NDIS_HANDLE                        MiniportAdapterHandle,
	IN  NDIS_HANDLE                        MiniportDriverContext,
	IN  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
	)
/*++
Routine Description:

	The MiniportInitialize function is a required function. Here is the
	list of things this function typically performs:

		Set the miniport attributes.
		Read configuration parameters from the registry.
		Allocate memory.
		Allocate the NET_BUFFER_LIST pool and the NET_BUFFER pool.
		Do hardware specific initialization like registering interrupt handlers/ set DMA resources etc.
		Read and write to the bus-specific configuration space.

	MiniportInitialize runs at IRQL = PASSIVE_LEVEL.

Arguments:

	MiniportAdapterHandle   The handle NDIS uses to refer to us
	MiniportDriverContext   Handle passed to NDIS when we registered the driver
	MiniportInitParameters  Initialization parameters contains a pointer
						to a list of the allocated hardware resources
						for the miniport adapter

Return Value:

	NDIS_STATUS_xxx code

--*/
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS		RtStatus = RT_STATUS_SUCCESS;
	NDIS_ERROR_CODE     ErrorCode = NDIS_ERROR_CODE_ADAPTER_NOT_FOUND;
	UINT			i;
	PADAPTER		Adapter = NULL;
	PRT_SDIO_DEVICE	sdiodevice = NULL;
	PMGNT_INFO		pMgntInfo = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = NULL;
	int				nIdx;
	RES_MON_OBJ	ResMonObj;
	GUID 				PowerSetting_GUID = GUID_ACTIVE_POWERSCHEME;
	GUID				WLANPowerMode_GUID = GUID_WLAN_POWER_MODE;

#if USE_WDF_SDIO
	NTSTATUS								ntStatus;
#endif

	BOOLEAN				bMemoryInitialized = FALSE;
	
	RT_TRACE(COMP_INIT, DBG_LOUD, (" ==> N6SdioInitialize(1)\n"));
	
	// Invoke resource checking mechanism.
	INIT_RES_MON_OBJ(ResMonObj);
	
	//1 Allocate memory to store usbdevice information.

	// Allocate ADAPTER structure
	ndisStatus = N6sdioAllocateAdapter(&Adapter, MiniportAdapterHandle);
	if( ndisStatus != NDIS_STATUS_SUCCESS ) 
		goto error;

	ADD_RES_TO_MON(ResMonObj, InitRM_AllocAdapter); // Add to resource monitor.

	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
	sdiodevice->pAdapter = Adapter;
	
	pMgntInfo = &(Adapter->MgntInfo);
	pNdisCommon = Adapter->pNdisCommon;
	pNdisCommon->NdisVersion  = NdisGetVersion();
	
	if(pNdisCommon->NdisVersion >= NDIS_VERSION_BASE_6_50)
		pNdisCommon->NdisVersion = NDIS_VERSION_BASE_6_50;
	
	NdisMGetDeviceProperty(MiniportAdapterHandle,
						&(sdiodevice->pPhysDevObj), // PhysicalDeviceObject 
						&(sdiodevice->FunctionalDeviceObject), // FunctionalDeviceObject 
						&(sdiodevice->pSdioDevObj), // NextDeviceObject
						NULL,
						NULL);

	sdiodevice->Sdbusinterface.Size = sizeof(SDBUS_INTERFACE_STANDARD);
	sdiodevice->Sdbusinterface.Version = SDBUS_INTERFACE_VERSION;
	sdiodevice->NextDeviceStackSize = (CHAR)sdiodevice->pSdioDevObj->StackSize + 1; 	

	//2 Save the related NDIS handles
	sdiodevice->hNdisAdapter = MiniportAdapterHandle;

	if(NDIS_STATUS_SUCCESS != N6SdioAllocateRxNetBufferListPool(Adapter))
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("XXX InitializeTxRxBuffer(): N6SdioAllocateRxNetBufferListPool() failed\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto error;
	}

	ADD_RES_TO_MON(ResMonObj, InitRM_AllocRxNBLPool); // Add to resource monitor.

#if USE_WDF_SDIO

	//
	// Create and keep WDFDEVICE handle for further use.
	//
	ntStatus = WdfDeviceMiniportCreate(WdfGetDriver(),
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         sdiodevice->FunctionalDeviceObject,
                                         sdiodevice->pSdioDevObj,
                                         sdiodevice->pPhysDevObj,
                                         &(sdiodevice->hWdfDevice));
	if(!NT_SUCCESS(ntStatus))
	{
		RT_TRACE(COMP_INIT,DBG_SERIOUS, ("WdfDeviceMiniportCreate failed (%#X)\n", ntStatus));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto error;
	}
#endif	


	//
	// Allocate Tx semaphore before create corresponding Tx thread.
	//
	PlatformInitializeSemaphore(&sdiodevice->TxSemaphore, 0);

#if RTL8723_SDIO_IO_THREAD_ENABLE 

	//
	// Allocate IO semaphore before create corresponding IO Handling thread.
	//
	PlatformInitializeSemaphore(&sdiodevice->IOSemaphore, 0);

#endif

	//
	// Set up synchronization mechanism for NdisTimer objects.
	//
	N6InitTimerSync(Adapter);

	
	// Allocate spin lock.
	N6CInitializeSpinLocks(Adapter);

	// Initialize event.
	NdisInitializeEvent(&(sdiodevice->evtSendingNBLCompleted));

	//
	// <Roger_Notes> Initialize event to make sure PnP action could be followed by Miniport CheckForHang routine
	// to prevent unexpect bug check code 0x1d.
	// 2009.09.30.
	//
	NdisInitializeEvent(&sdiodevice->SetPnpChkForHangEvent);

	//----------------------------------------------------------------------------
	// Initialize variables related to New IO method, 2005.01.06, by rcnjko.
	// 
	NdisAllocateSpinLock( &(sdiodevice->IrpSpinLock) );
	NdisOIDHistoryInit(Adapter);	
	ADD_RES_TO_MON(ResMonObj, InitRM_SpinLock); // Add to resource monitor.

#if (RK_PLATFORM_SUPPORT == 1)
	PlatformInitializeMutex(&sdiodevice->RxHandleIntMutex);
#endif

	// SyncIo Method 2.  
	KeInitializeEvent( &(sdiodevice->SyncIoEvent), NotificationEvent, TRUE);
	
	if (WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
		pNdisCommon->MaxPktSize=NIC_MAX_PACKET_SIZE;
	
	sdiodevice->nIrpPendingCnt = 0;
	RTInitializeListHead( &(sdiodevice->AwbIdleQueue) );
	RTInitializeListHead( &(sdiodevice->AwbWaitQueue) );
	sdiodevice->NumAwb = MAX_AWB_DATA_SIZE;
	PrepareSdioAWBs(sdiodevice);

	ADD_RES_TO_MON(ResMonObj, InitRM_AWBs); // Add to resource monitor.
	
	//2Init Event for canceling Asyn_Io pending IRP when unload	
	NdisInitializeEvent(&sdiodevice->AllAsynIoIrpReturnedEvent);

	//2Init Event for all SDIO cmd52/cmd53 returned when unload	
	NdisInitializeEvent(&sdiodevice->AllSdioCmdReturnedEvent);
	
	NdisInitializeEvent(&sdiodevice->FwPsClockOffEvent);
	NdisInitializeEvent(&sdiodevice->AllSdioRxTransCompleteEvent);
	
	N6WdmSdio_Enable(Adapter);
	
	sdiodevice->AsynIoIrpPendingCount++;
	
	// Read the registry parameters
	ndisStatus= N6SdioReadRegParameters(sdiodevice);
	if (ndisStatus != NDIS_STATUS_SUCCESS) 
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioReadRegParameters(X): Read Registry Parameter Failed!\n"));
		goto error;
	} 

	//
	// <Roger_Notes> We set default hardware type to unknown type to associate common NIC handler first and 
	// then re-assign correct HW type association handler when the type is recognized.
	// 2011.12.30.
	//
	ndisStatus = NicIFAssociateNIC(Adapter, HARDWARE_TYPE_MAX);	

#if USE_WDF_SDIO
	if( !NT_SUCCESS(WdfSdio_Initialize(Adapter) ))
	{
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("MiniportInitialize(X): WdfSdio_Initialize() failed!\n") );
		goto error;
	}
#else

	//
	// Perform SDIO WDM initialization
	//
	if( !NT_SUCCESS(N6WdmSdio_Initialize(Adapter) ))
	{
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("MiniportInitialize(X): N6WdmSdio_Initialize() failed!\n") );
		goto error;
	}
	
#endif

	if(Adapter->HardwareType == HARDWARE_TYPE_MAX)
		HAL_ReadTypeID(Adapter);
	
	HAL_SetInterfaceIndex(Adapter, 0);

	// We should set the correct hardware type to the following function
	ndisStatus = NicIFAssociateNIC(Adapter, Adapter->HardwareType);	
	if (ndisStatus != NDIS_STATUS_SUCCESS) 
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("NicIFAssociateNIC Failed!\n"));
		goto error;
	} 
	else
	{
		NicIFInitResource(Adapter);
	}
	
	ADD_RES_TO_MON(ResMonObj, InitRM_AsocNIC); // Add to resource monitor.


	// 
	// Update NDIS6 common setting as user specifed to Adapter, MgntInfo, and HalData.
	//
	N6UpdateDefaultSetting(Adapter);

	Dot11_UpdateDefaultSetting(Adapter);
	
	N6SdioUpdateDefaultSetting(Adapter);

	//
	// Initialize SDIO Data transfer related Tx context buffer.
	//
	if( !NT_SUCCESS(N6SdioInitTxQueue(Adapter) ) ) 
		goto error;	

	ADD_RES_TO_MON(ResMonObj, InitRM_SdioTxRes); // Add to resource monitor.
	
	//
	// Set SDIO Tx queue mapping
	//
	if(!Adapter->HalFunc.HalSdioSetQueueMappingHandler(
			Adapter, (u1Byte)sdiodevice->RtNumRxQueue, (u1Byte)sdiodevice->RtNumTxQueue))
	{
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioInitialize(X): Set SDIO Tx Queue mapping failed!\n") );
		goto error;
	}

	// Enable all Tx Queues.
	N6WdmSdioTx_Enable(Adapter);

	ADD_RES_TO_MON(ResMonObj, InitRM_SdioInit); // Add to resource monitor.
		
	// Read adapter information such as MAC address from EEPROM
	NicIFReadAdapterInfo(Adapter);
	HT_UpdateDefaultSetting(Adapter);
	VHT_UpdateDefaultSetting(Adapter);

#if 1
	//Initialize  Ndis 6.2, Ndis 6.0 only return success.
	ndisStatus=NDIS_6_2_INITIALIZE_EXTENSION_COMPONENT(Adapter,MiniportAdapterHandle,MiniportInitParameters);

	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N62CInitialize failed\n"));
		goto error;
	}
#endif


	//
	// Update default setting to HalData.
	//
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_LED, &(sdiodevice->bRegLedCtrl));

	//
	// Support WoW for future use. Added by Roger, 2008.12.30.
	//
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN, &(sdiodevice->bSupportRemoteWakeUp));
	
	// Settings for HctTest.
	if(Adapter->bInHctTest)
	{
	
		// For NDIS6 DOT11_STATISTICS test, we must open CRC, ICV error.
		// Revise this handler for DTM test, 2008.12.16, added by Roger.
		Adapter->HalFunc.AllowErrorPacketHandler(Adapter, TRUE, FALSE);
	}
	
	//
	// For usb selective suspend configuration. Added by Roger, 2010.04.16.
	//
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_USB_SELECTIVE_SUSPEND, &(sdiodevice->RegUsbSS));

	//
	// 2011/09/07 MH Add TX power for different channel plan.
	//
	RT_TRACE(COMP_INIT, DBG_LOUD, 
	("power offset MKK = %08x/%08x/%08x ETSI = %08x/%08x/%08x\n", 
	sdiodevice->RegGainOffsetMKKLow, sdiodevice->RegGainOffsetMKK, sdiodevice->RegGainOffsetMKKHigh,
	sdiodevice->RegGainOffsetETSILow, sdiodevice->RegGainOffsetETSI, sdiodevice->RegGainOffsetETSIHigh));

	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_FCC_LOW, &(sdiodevice->RegGainOffsetFCCLow));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_FCC, &(sdiodevice->RegGainOffsetFCC));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_FCC_HIGH, &(sdiodevice->RegGainOffsetFCCHigh));
	
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_MKK_LOW, &(sdiodevice->RegGainOffsetMKKLow));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_MKK, &(sdiodevice->RegGainOffsetMKK));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_MKK_HIGH, &(sdiodevice->RegGainOffsetMKKHigh));
	
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_ETSI_LOW, &(sdiodevice->RegGainOffsetETSILow));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_ETSI, &(sdiodevice->RegGainOffsetETSI));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_GAIN_OFFSET_ETSI_HIGH, &(sdiodevice->RegGainOffsetETSIHigh));

	//
	// 2011/11/25 MH Add for Sercom requirement temporarily.
	//
	//usbdevice->RegInPipeCnt; Need not assign to mgnt structure, we need to 
	
			
	// For PSP XLink mode, 2006.09.04, by shien chang.
	if(pMgntInfo->bPSPXlinkMode)
	{
		// Update ReceiveConfig variable. 
		Adapter->HalFunc.AllowAllDestAddrHandler(Adapter, TRUE, FALSE);
	}

	// 
	// <Roger_Notes> In this time we shall NOT allocate TxRx corresponding resources here
	// temporarily for 92CU on Vista or later, 2010.01.21.
	// 2010.07.01 hpfan: allocate memory for shorten prepending time is NOT used. 
	if(ACTING_AS_AP(Adapter) || !pMgntInfo->bRegVelocity|| pMgntInfo->RegPreInitMem)
	{
		ndisStatus=NicIFAllocateMemory(Adapter);
	
		if(ndisStatus!=NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("NicIFAllocateMemory failed\n"));
			goto error;
		}

		ADD_RES_TO_MON(ResMonObj, InitRM_AllocMem); // Add to resource monitor.
		bMemoryInitialized = TRUE;
	}

	//
	// A miniport driver must call the NdisMSetMiniportAttributes function from its 
	// MiniportInitializeEx function to identify a context area for miniport adapter to NDIS, 
	// and to provide NDIS with information about the miniport adapter.
	//
	ndisStatus= N6sdioSetMiniportAttributes(Adapter, MiniportAdapterHandle);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6sdioSetMiniportAttributes(): failed to set driver attributes.\n"));
		goto error;
	}


	N6SdioAllocRes(Adapter);
	
	ADD_RES_TO_MON(ResMonObj, InitRM_AllocSdioRes); // Add Usb related resource monitoring.

	sdiodevice->CurrentPowerState = NdisDeviceStateD0;
	
	//InitializAdapter and PHY Configuration.
	ADAPTER_SET_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT);
	ndisStatus = NicIFInitializeAdapter(Adapter);
	if( ndisStatus != NDIS_STATUS_SUCCESS )
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("NicIFInitializeAdapter() failed!!\n"));
		goto error;
	}
	ADAPTER_CLEAR_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT);

	//
	// HCT12.0 2c_AddressChange.
	// Note that, Adapter->CurrentAddress and Adapter->PermanentAddress had been  
	// set up in NicIFInitializeAdapter().
	//
	if(pNdisCommon->bOverrideAddress)
	{
		NicIFSetMacAddress(Adapter, pNdisCommon->CurrentAddress);
	}
	else
	{
		ETH_COPY_NETWORK_ADDRESS(pNdisCommon->CurrentAddress, Adapter->PermanentAddress);
	}

	//
	// Before setting of attributes, update the native 802.11 related variable.
	// 2006.10.09, by shien chang.
	//
	ndisStatus = N6SdioAllocateNative80211MIBs(Adapter);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioAllocateNative80211MIBs failed\n"));
		goto error;
	}
	
	ADD_RES_TO_MON(ResMonObj, InitRM_AllocNWifi); // Add to resource monitor.
	
	N6InitializeNative80211MIBs(Adapter);
	
	//
	// Set General Attribute, 2006.10.05, by shien chang.
	//
	ndisStatus = N6CSetGeneralAttributes(Adapter);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6usbSetGeneralAttributes failed\n"));
		goto error;
	}


	//Register Ndis 6.2 802.11 Attribute extend from Ndis6.0 and Include New ExtAP to OS
	//ndisStatus= N6XC_SET_802_11_ATTRIBUTES(Adapter);
	ndisStatus = N6Set80211Attributes(Adapter);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6usbSet80211Attributes failed\n"));
		goto error;
	}

	//
	// Set Hardware Assist Attribute. 2013.10.09, by tynli.
	//
	ndisStatus = N6SdioSetHardwareAssistAttributes(Adapter);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioSetHardwareAssistAttributes failed\n"));
		//goto error;
	}

	//Get Ndis Version
	pMgntInfo->NdisVersion = MgntTranslateNdisVersionToRtNdisVersion(pNdisCommon->NdisVersion);


	// At the moment, if HW is unpluged, fail the initialization immediate.
	if (Adapter->bSurpriseRemoved)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Adapter is unpluged, fail the init process.\n"));
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		goto error;
	}

	//
	// Set Power config callback, 2009.03.18, by lanhsin.
	//
	ndisStatus = PoRegisterPowerSettingCallback(
			NULL,
			(LPGUID)(&PowerSetting_GUID), 
			(PPOWER_SETTING_CALLBACK)&N6SdioPowerSettingCallback,
			(PVOID)Adapter,
			&pNdisCommon->N6PowerSettingHandle
			);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Failed to register miniport with NDIS. Status = 0x%x\n", ndisStatus ));
		goto error;
	}		

	ndisStatus = PoRegisterPowerSettingCallback(
			NULL,
			(LPGUID)(&WLANPowerMode_GUID), 
			(PPOWER_SETTING_CALLBACK)&N6SdioWLANPowerModeCallback,
			(PVOID)Adapter,
			&pNdisCommon->N6WLANPowerModeHandle
			);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Failed to register miniport with NDIS. Status = 0x%x\n", ndisStatus));
		goto error;
	}		

	N6CInitThread((PVOID)Adapter);

	InitNdis6CommonResources(Adapter);
	ADD_RES_TO_MON(ResMonObj, InitRM_CommonRS);
	NDBG_Init(Adapter);
#if 0
	// Initialize debugging realted information or resource. 2005.04.21, by rcnjko.
	N6SdioDbgInit(Adapter);
#endif
	// For AP Mode. 2005.07.19, by rcnjko.
	if(ACTING_AS_AP(Adapter))
	{
		MgntActSet_ApMode(Adapter, TRUE);
	}

       	// Set power save mode. 2005.02.15, by rcnjko.
	MgntActSet_802_11_PowerSaveMode(Adapter, pMgntInfo->dot11PowerSaveMode);

	// <Roger_Notes> We disable Rx aggregation for WiFi test. 2008.11.25.
	if(pMgntInfo->bWiFiConfg || Adapter->bInHctTest)
		Adapter->HalFunc.HalRxAggrHandler(Adapter, FALSE);

	// It should be called iff everything, especially security related, are initialized properly. 
	// 2005.01.24, by rcnjko.
	//N6RestoreLastInitSetting(Adapter);	

	//This is sw initialize ready.
	Adapter->bSWInitReady=TRUE;

	if(NDIS_STATUS_SUCCESS == ndisStatus) 
	{
		// After this adapter is initialzied ready, insert this adapter to the list.
		INSERT_GLOBAL_ADAPTER_LIST(Adapter);
		
		//
		// Register the Device object for this adapter!
		// Note:
		//	No matter the returned status of registration is success or not, the result has no
		//	effect on the status for Miniportinitialize.
		// 	So, the resiteration process should be placed in the success location of Initialization.
		// By Bruce, 2011-07-07.
		//	
		if(RT_STATUS_SUCCESS != (RtStatus = N6C_RegisterIoDevice(Adapter)))
		{
			RT_TRACE_F(COMP_INIT, DBG_LOUD, ("N6C_RegisterIoDevice() failed for status = %d\n", RtStatus));
		}
	}
	
	//This is sw initialize ready.
	if(!Adapter->bHWInitReady)
		PlatformScheduleWorkItem(&(pNdisCommon->InitializeAdapterWorkItem));

	NicIFClearInterrupt(Adapter);
	NicIFEnableInterrupt(Adapter);		

	//
	// Mark the miniport driver as paused state. 
	// Note that, please keep this action as last one in MiniportInitializeEx().
	//
	N6C_SET_MP_DRIVER_STATE(Adapter, MINIPORT_PAUSED);

	Adapter->initfinish = TRUE;
	Adapter->bInitComplete = TRUE;	
	DrvIFIndicateCurrentPhyStatus(Adapter);
	
	return ndisStatus;

error:
	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== N6SdioInitialize(), Initialize failed, clean up resources. ndisStatus(%#x)\n", ndisStatus));
	CLEANUP_RES_IN_MON(ResMonObj, Adapter);
	return ndisStatus;
}


//
//	Description:
//		Prepare SDIO data transfer related NonPaged memory Pool for SDIO Asyn Extension CMD 
//	use in the future.
//
//	2011.01.19, added by Roger. 
//
NTSTATUS
N6SdioInitTxQueue(
	IN  PADAPTER 	Adapter
)
{
	NTSTATUS 	ntStatus = STATUS_SUCCESS;
	PRT_SDIO_DEVICE	pDevice = GET_RT_SDIO_DEVICE(Adapter);
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;		
	u4Byte	 QueueIdx = 0;

	pDevice->RtNumTxQueue = SDIO_MAX_TX_QUEUE; // TX_HIQ, TX_MIQ and TX_LOQ	  

	// Initialize the contexts in Tx Queue.
	pDevice->RtTxQueue = 
		(PRT_SDIO_TX_QUEUE)ExAllocatePoolWithTag(NonPagedPool, SDIO_MAX_TX_QUEUE*sizeof(RT_SDIO_TX_QUEUE), '3278' );
	PlatformZeroMemory(pDevice->RtTxQueue, SDIO_MAX_TX_QUEUE*sizeof(RT_SDIO_TX_QUEUE));	
  
	if (pDevice->RtTxQueue != NULL) 
	{

		for (QueueIdx=0; QueueIdx<SDIO_MAX_TX_QUEUE; QueueIdx++) //about TxQueue
		{
			pTxQueue= &(pDevice->RtTxQueue[QueueIdx]);				
			pTxQueue->TransferType = SDIO_TRANSFER_TX;
			RTInitializeListHead(&(pTxQueue->ContextBusyList));
			pTxQueue->IrpPendingCount = 0;

			//Init Event for canceling Tx/Rx pending IRP when unload.
			NdisInitializeEvent(&(pTxQueue->AllIrpReturnedEvent));								
		}
	}
	else
	{					
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
		goto Error;
	}			
	
	return ntStatus;

Error:
	if(pDevice->RtTxQueue != NULL)
	{
		ExFreePoolWithTag(pDevice->RtTxQueue, '3278' );		
	}
	
	return ntStatus;
}


//
//	Description:
//		Dump IO bus related properties and configurations for debugging. 
//	Common I/O Area(CIA) in function 0:
//		- Card Common Control Registers(CCCR)
//		- Function Basic Registers(FBR)
//		- Card Information Structure(CIS)
//
//	Assumption:
//		 A Secure Digital (SD) card bus interface has been initialized.	
//
//	2011.03.15, added by Roger. 
//
RT_STATUS
N6SdioDumpDeviceProperty(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	u2Byte	Idx = 0;
	u1Byte	Data;
	NTSTATUS ntStatus = STATUS_SUCCESS;	
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	
	
	//
	// Retrieve CCCR info
	//
	if(GlobalSdioDbg & SDIO_DBG_CCCR)
	{
		for(Idx=SDIO_CCCR_BEGIN; Idx<=SDIO_CCCR_END; Idx++)
		{
			ntStatus = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)Idx, //Offset
									FALSE, //Read operation
									&Data);

			RT_TRACE(COMP_INIT, DBG_LOUD, ("SDIO CCCR[%#x]:%x\n", Idx, Data));
			
			if (!NT_SUCCESS(ntStatus))
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Get CCCR[%#x] fail!! status(%#x)\n", Idx, ntStatus));
				return RT_STATUS_FAILURE;
			}
		}
	}


	//
	// Retrieve FBR info
	//
	if(GlobalSdioDbg & SDIO_DBG_FBR)
	{
		for(Idx=SDIO_FBR_FUNC1_BEGIN; Idx<=SDIO_FBR_FUNC1_END; Idx++)
		{
			ntStatus = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)Idx, //Offset
									FALSE, //Read operation
									&Data);

			RT_TRACE(COMP_INIT, DBG_LOUD, ("SDIO FBR[%#x]:%x\n", Idx, Data));
			
			if (!NT_SUCCESS(ntStatus))
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Get FBR[%#x] fail!! status(%#x)\n", Idx, ntStatus));
				return RT_STATUS_FAILURE;
			}
		}
	}		


	//
	// Retrieve CIS Area info
	//
	if(GlobalSdioDbg & SDIO_DBG_CIS)
	{
		
		for(Idx=SDIO_CIS_BEGIN; Idx<=SDIO_CIS_END; Idx++)
		{
			ntStatus = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)Idx, //Offset
									FALSE, //Read operation
									&Data);

			RT_TRACE(COMP_INIT, DBG_LOUD, ("SDIO CIS[%#x]:%x\n", Idx, Data));
			
			if (!NT_SUCCESS(ntStatus))
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Get CIS[%#x] fail!! status(%#x)\n", Idx, ntStatus));
				return RT_STATUS_FAILURE;
			}
		}
	}


	return rtstatus;
}



//
//	Description:
//		Find a device's tuple in CIS Area.
//		input: pDevice - the device that is the target of the command.
//		input: Tuple - 8-bit ID of tuple to find
//		input: pTupleScanAddress - On entry pTupleScanAddress is the adddress to start scanning
//		input: pLength - length of pBuffer
//
//		output: pBuffer - storage for tuple
//		output: pTupleScanAddress - address of the next tuple
//		output: pLength - length of tuple read
//
//		return: RT_STATUS
//
//	Assumption:
//		 A Secure Digital (SD) card bus interface has been initialized.	
//
//	2011.03.17, created by Roger. 
//
RT_STATUS
N6SdioFindCisTuple(
	IN 	PRT_SDIO_DEVICE	pDevice,
	IN 	u1Byte	Tuple,
	IN OUT 	pu4Byte	pTupleScanAddress,
	OUT 	pu1Byte	pBuffer,
	IN OUT 	pu1Byte	pLength	
	)
{
	u4Byte	scanStart = *pTupleScanAddress;
	u1Byte	tupleCode;
    	u1Byte    tupleLink;
	RT_STATUS	status = RT_STATUS_SUCCESS;

	// Sanity check
	if( scanStart < SDIO_CIS_BEGIN )
		return RT_STATUS_FAILURE;

	while (TRUE) {
	 	
            	//Check for end
		if (scanStart > SDIO_CIS_END) {
	       	status = RT_STATUS_FAILURE;
	            	break;
	       }
		
		//Get the code		
		status = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)scanStart, //Offset
									FALSE, //Read operation
									&tupleCode);
		if ( status != RT_STATUS_SUCCESS ) {
		    break;
		}
		
		if (CISTPL_END == tupleCode) {
			//found the end
		    	status = RT_STATUS_FAILURE;
		    	break;
		}
		
		// Bump past tuple code
		scanStart++;
		
		//Get the tuple link value
		status = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)scanStart, //Offset
									FALSE, //Read operation
									&tupleLink);
		
		if ( status != RT_STATUS_SUCCESS ) {
		    break;
		}

		//Bump past tuple link
		scanStart++;
		
		//Check tuple we just found
		if (tupleCode == Tuple) {		

			RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioFindCisTuple(): Tuple:0x%2.2X Found at Address:0x%X, TupleLink:0x%X \n",
				Tuple, (scanStart - 2), tupleLink));
			
			if (tupleLink != CISTPL_LINK_END) {
				//return the next scan address to the caller
			    	*pTupleScanAddress = scanStart + tupleLink;
			} else {
				//the tuple link is an end marker
			    	*pTupleScanAddress = 0xFFFFFFFF;
			}

			//Go get the tuple		
			status = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									MIN(*pLength,tupleLink), //Byte Count
									(ULONG)scanStart, //Offset
									FALSE, //Read operation
									pBuffer);
			
			if (status == RT_STATUS_SUCCESS) {
				//Set the actual return length
			    	*pLength = MIN(*pLength,tupleLink);
			}
			//Break out of loop
			break;
		}

		//Increment past this entire tuple
		scanStart += tupleLink;
	}
	
	return RT_STATUS_SUCCESS;
}


//
//	Description:
//		The Card Information Structure provides more complete information about the card 
//	and the individual functions. The CIS is the common area to read information about all I/O 
//	functions that exist in a card. The design is based on the PC Card16 design standardized by 
//	PCMCIA. All cards that support I/O shall have a common CIS and a CIS for each function.
//
//	Assumption:
//		 A Secure Digital (SD) card bus interface has been initialized.	
//
//	2011.03.16, created by Roger. 
//
RT_STATUS
N6SdioQueryCISArea(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	RT_STATUS rtStatus = RT_STATUS_SUCCESS;	
	PADAPTER	pAdapter = pDevice->pAdapter;
	u1Byte	CisBuffer[3];	
	u4Byte	tplAddr = 0;
	u1Byte	Length = 0;
	

	//
	// This 3-byte pointer points to the start of the card's common CIS. The common CIS
	// contains information relation to the entire card. A card common CIS is mandatory for 
	// all SDIO cards. This pointer is stored in little-endian format (LSB first).
	//
	rtStatus = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									3, //Byte Count
									(ULONG)SDIO_CCCR_COMMON_CIS_PTR, //Offset
									FALSE, //Read operation
									CisBuffer);
	
	// This is Endian-Safe
	pDevice->SdioCommonCISPtr = EF4Byte(((u4Byte)CisBuffer[0]) |
                                                (((u4Byte)CisBuffer[1]) << 8) |
                                                (((u4Byte)CisBuffer[2]) << 16));

	RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioQueryCISArea(): SdioCommonCISPtr(%#x)\n", pDevice->SdioCommonCISPtr));
	
	PlatformZeroMemory(&pDevice->Sdiomanfid, sizeof(pDevice->Sdiomanfid));
	Length = sizeof(pDevice->Sdiomanfid);
	tplAddr = RT_GET_SDIO_COMMON_CISPTR(pDevice);

	// Get the MANFID tuple
	rtStatus = N6SdioFindCisTuple(
					pDevice,
                                   CISTPL_MANFID,
                                   &tplAddr,
                                   (pu1Byte)&pDevice->Sdiomanfid,
                                   &Length);
	
	if ( rtStatus != RT_STATUS_SUCCESS ) 
	{	       
		RT_TRACE(COMP_INIT, DBG_WARNING, ("SDIO Bus Driver: Failed to get MANFID tuple error:%d \n", rtStatus));	      
	} 
	else 
	{
	
		//save this off so that it can be copied into each SDIO Func's SDDEVICE structure
		pDevice->SDIO_ManufacturerCode =
		                            EF2Byte(pDevice->Sdiomanfid.ManufacturerCode);
		pDevice->SDIO_ManufacturerInfo =
		                            EF2Byte(pDevice->Sdiomanfid.ManufacturerInfo);

		//
		// HardwareType shall be assigned in HAL_ReadTypeID routine instead on SDIO interface
		// to meet extensible requirements for PID creation.
		// Revised by Roger. 2014.10.28.
		//
#if 0
		if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8723S_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8723AS;
		else if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8188E_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8188ES;
		else if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8821S_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8821S;
		else if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8192ES_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8192ES;
		else if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8814AS_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8814AS;
		else if((pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8723BS_PID) ||
			(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8723BS_PID_TEST_CHIP) ||
			(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8723BS_PID_ACER) ||
			(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8723BS_PID_HP))
		{
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8723BS;
		}
		else if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8703BS_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8703BS;
		else if(pDevice->SDIO_ManufacturerInfo == HAL_HW_SDIO_8821BS_PID)
			pAdapter->HardwareType = HARDWARE_TYPE_RTL8821BS;		
		else
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("N6SdioQueryCISArea(): Invalid MANFINFO!!"));				
			pAdapter->HardwareType = HARDWARE_TYPE_MAX;				
		}	
#endif

		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_VID, (pu1Byte)&pDevice->SDIO_ManufacturerCode);
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_PID, (pu1Byte)&pDevice->SDIO_ManufacturerInfo);

		RT_TRACE(COMP_INIT, DBG_LOUD, ("SDIO MANFCODE:0x%X, MANFINFO:0x%X \n",			
			pDevice->SDIO_ManufacturerCode,
			pDevice->SDIO_ManufacturerInfo));	   
	}
	
	RT_TRACE(COMP_INIT, DBG_WARNING, ("N6Sdio(): SDIO_ManufacturerInfo(%#x)\n", pDevice->SDIO_ManufacturerInfo));
	RT_TRACE(COMP_INIT, DBG_WARNING, ("N6SdioQueryCISArea(): HardwareType(%#x)\n", pAdapter->HardwareType));		

	return rtStatus;

}

//
//	Description:
//		For Intel Bay trail platform, SDIO bus clock is expected to run in 50MHz (high speed),
//		so we confiure it to 50MHz by miniport driver. It is a work around because
// 		clock value and CCCR[13] should be configured by bus driver.
//
//
//	2013.08.14, added by tynli.
//
NTSTATUS
N6SdioConfigureBusCLKByWorkAround(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	NTSTATUS ntStatus		= STATUS_SUCCESS;	
	RT_STATUS	status		= RT_STATUS_FAILURE;
	u1Byte 		Data=0;
	u4Byte		SdioBusClk=0;
	
	status = PlatformSdioCmd52ReadWrite(
							pDevice, 
							SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
							0, //Function Number
							1, //Byte Count
							(ULONG)0x13, //Offset
							FALSE, //Read operation
							&Data);

	if(status != RT_STATUS_SUCCESS){ 
		RT_TRACE_F(COMP_INIT, DBG_LOUD, ("Read SDIO CCCR[0x13] Property FAIL!!\n"));
		return STATUS_SUCCESS; //do not cause init fail
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, (">>>>>>>>>>>> 1 SDIO CCCR[0x13]:%x\n", Data));

	if(Data&BIT0) // support SHS
	{
		if( !(Data&BIT1) ) // not in 50MHz
		{
			//
			// Set SDIO clock to 50MHz if support SHS in CCCR register.
			//
			SdioBusClk = 50000;
			ntStatus = PlatformSdioSetProperty(pDevice,
							SDP_BUS_CLOCK,
							&SdioBusClk,
							sizeof(SdioBusClk));  
			if(!NT_SUCCESS(ntStatus)){ 
				RT_TRACE_F(COMP_INIT, DBG_LOUD, ("Set SDP_BUS_CLOCK Property FAIL!!\n"));
				return STATUS_SUCCESS; //do not cause init fail
			}

			//
			// Enable BSS in CCCR.
			//
			Data |= BIT1; //[3:1] BSS
			status = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)0x13, //Offset
									TRUE, //Write operation
									&Data);


			if(status != RT_STATUS_SUCCESS){ 
				RT_TRACE_F(COMP_INIT, DBG_LOUD,("Write SDIO CCCR[0x13] Property FAIL!!\n"));
				return STATUS_SUCCESS; //do not cause init fail
			}

			// To be removed ...
			RT_TRACE(COMP_INIT, DBG_LOUD, (">>>>>>>>>>>> 2 SDIO CCCR[0x13]:%x\n", Data));

			Data = 0;
			delay_ms(10);
			status = PlatformSdioCmd52ReadWrite(
							pDevice, 
							SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
							0, //Function Number
							1, //Byte Count
							(ULONG)0x13, //Offset
							FALSE, //Read operation
							&Data);
			
			RT_TRACE(COMP_INIT, DBG_LOUD, (">>>>>>>>>>>> 3 SDIO CCCR[0x13]:%x\n", Data));

			if(status != RT_STATUS_SUCCESS){ 
				RT_TRACE_F(COMP_INIT, DBG_LOUD, ("3 SDIO CCCR[0x13] Property FAIL!!\n"));
				return STATUS_SUCCESS; //do not cause init fail
			}
			// End of To be removed ...	

			pDevice->bSdioBusClkRaised = TRUE;
			
		}
	}
	return STATUS_SUCCESS;
}

//
//	Description:
//		Similiar with N6SdioConfigureBusCLKByWorkAround(), but this is for set the SDIO bus clk at 25MHz.
//
//	2013.04.14, added by TzuHang.
//
NTSTATUS
N6SdioConfigureBusCLK_SDR12(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	NTSTATUS 	ntStatus	= STATUS_SUCCESS;	
	RT_STATUS	status		= RT_STATUS_FAILURE;
	u1Byte 		Data		= 0;
	u4Byte		SdioBusClk	= 0;
	
	status = PlatformSdioCmd52ReadWrite(
							pDevice, 
							SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
							0, //Function Number
							1, //Byte Count
							(ULONG)0x13, //Offset
							FALSE, //Read operation
							&Data);

	if(status != RT_STATUS_SUCCESS){ 
		RT_TRACE_F(COMP_INIT, DBG_LOUD, ("Read SDIO CCCR[0x13] Property FAIL!!\n"));
		return STATUS_SUCCESS; //do not cause init fail
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, (">>>>>>>>>>>> 1 SDIO CCCR[0x13]:%x\n", Data));

	//
	// Set SDIO clock to 50MHz if support SHS in CCCR register.
	//
	SdioBusClk = 25000;
	ntStatus = PlatformSdioSetProperty(pDevice,
						SDP_BUS_CLOCK,
						&SdioBusClk,
						sizeof(SdioBusClk));  
					
	if(!NT_SUCCESS(ntStatus)){ 
		RT_TRACE_F(COMP_INIT, DBG_LOUD, ("Set SDP_BUS_CLOCK Property FAIL!!\n"));
		return STATUS_SUCCESS; //do not cause init fail
	}

	//
	// Enable BSS in CCCR.
	//
	Data &= ~(BIT1|BIT2); //[3:1] BSS
	status = PlatformSdioCmd52ReadWrite(
							pDevice, 
							SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
							0, //Function Number
							1, //Byte Count
							(ULONG)0x13, //Offset
							TRUE, //Write operation
							&Data);

	if(status != RT_STATUS_SUCCESS){ 
		RT_TRACE_F(COMP_INIT, DBG_LOUD,("Write SDIO CCCR[0x13] Property FAIL!!\n"));
		return STATUS_SUCCESS; //do not cause init fail
	}

	// To be removed ...
	RT_TRACE(COMP_INIT, DBG_LOUD, (">>>>>>>>>>>> 2 SDIO CCCR[0x13]:%x\n", Data));

	Data = 0;
	delay_ms(10);
	status = PlatformSdioCmd52ReadWrite(
					pDevice, 
					SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
					0, //Function Number
					1, //Byte Count
					(ULONG)0x13, //Offset
					FALSE, //Read operation
					&Data);
		
	RT_TRACE(COMP_INIT, DBG_LOUD, (">>>>>>>>>>>> 3 SDIO CCCR[0x13]:%x\n", Data));

	if(status != RT_STATUS_SUCCESS){ 
		RT_TRACE_F(COMP_INIT, DBG_LOUD, ("3 SDIO CCCR[0x13] Property FAIL!!\n"));
		return STATUS_SUCCESS; //do not cause init fail
	}
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("Done Set SDIO clk to SDR12\n"));
	
	// End of To be removed ...	
	return STATUS_SUCCESS;
}


//
//	Description:
//		Configure/Retrieve SDIO bus related properties and configurations. 
//	Common I/O Area(CIA) in function 0:
//		- Card Common Control Registers(CCCR)
//		- Function Basic Registers(FBR)
//		- Card Information Structure(CIS)
//
//	Assumption:
//		 A Secure Digital (SD) card bus interface has been initialized.	
//
//	2010.12.09, added by Roger. 
//
NTSTATUS
N6SdioConfigureDevice(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	NTSTATUS 	ntStatus = STATUS_SUCCESS;	
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	BOOLEAN		bEnableInt = FALSE;
	u1Byte		IENx 	= 0;
	u4Byte		Idx 	= 0;
	u1Byte		u1bData	= 0;


	//
	// We should get Function number first.
	//
	ntStatus = PlatformSdioGetProperty(
								pDevice,
       							SDP_FUNCTION_NUMBER,
                         		&pDevice->SdioFuncNum,
                         		sizeof(pDevice->SdioFuncNum));	
    if(!NT_SUCCESS(ntStatus)){ 
		return ntStatus;
	}		

	
	//
	// Query SDIO Card Information Structure
	//
	rtstatus = N6SdioQueryCISArea(pDevice);
	if(rtstatus != RT_STATUS_SUCCESS){ 
			return STATUS_UNSUCCESSFUL;
	}

	
	//
	// Dump SDIO IO Bus related information if needed.
	//
	rtstatus = N6SdioDumpDeviceProperty(pDevice);
	if(rtstatus != RT_STATUS_SUCCESS){ 
			return STATUS_UNSUCCESSFUL;
	}

	
	//
	// Get Driver Version
	//
	ntStatus = PlatformSdioGetProperty(
								pDevice,
		       					SDP_BUS_DRIVER_VERSION,
		                     	&pDevice->SdioBusDriverVer,
		                     	sizeof(pDevice->SdioBusDriverVer));    	
	if(!NT_SUCCESS(ntStatus)){ 
		return ntStatus;
	}


	//
	// Read CCCRx Format Version Number and SDIOx Specification Revision Number. 
	//
	ntStatus = PlatformSdioCmd52ReadWrite(
								pDevice, 
								SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
								0, //Function Number
								1, //Byte Count
								(ULONG)SDIO_CCCRx_SDIOx, //Offset
								FALSE, //Read operation
								&pDevice->SdioRevNum);
	
	if (!NT_SUCCESS(ntStatus))
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("Get SDIO_CCCRx_SDIOx failed:%x\n", ntStatus));
		return ntStatus;
	}


	//
	// Retrieve SD Format Version Number.
	//
	ntStatus = PlatformSdioCmd52ReadWrite(
								pDevice, 
								SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
								0, //Function Number
								1, //Byte Count
								(ULONG)SDIO_CCCR_SDx, //Offset
								FALSE, //Read operation
								&pDevice->SdioFormatVer);
	if (!NT_SUCCESS(ntStatus))
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("Get SDIO_CCCR_SDx failed:%x\n", ntStatus));
		return ntStatus;
	}

	if( pDevice->SdioBusDriverVer >= SDBUS_DRIVER_VERSION_2 &&
		((pDevice->SdioRevNum & 0x0f) >= 0x02) &&  // CCCR v2.0
		((pDevice->SdioRevNum & 0xf0) >= 0x30)) // SDIO v2.0
	{ 		
	
		u2Byte	SdioFuncBlockSize = SDIO_BUS_SIZE;


		//
		// Read interrupt property from current function.
		//
		ntStatus = PlatformSdioGetProperty(
									pDevice,
	       							SDP_FUNCTION_INT_ENABLE,
	                         		&bEnableInt,
	                         		sizeof(bEnableInt));	
	    if(!NT_SUCCESS(ntStatus)){ 
			return ntStatus;
		}

		//
		// Set interrupt property from current function.
		//
		if(!bEnableInt)
		{
			bEnableInt = TRUE; // Enable interrupt for our function
			ntStatus = PlatformSdioSetProperty(
							pDevice,
		       				SDP_FUNCTION_INT_ENABLE,
		                         		&bEnableInt,
		                         		sizeof(bEnableInt));	
		    	if(!NT_SUCCESS(ntStatus)){ 
					return ntStatus;
			}
		}
		
		//
		// Read CCCR interrupt
		//
		ntStatus = PlatformSdioCmd52ReadWrite(
									pDevice, 
									SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
									0, //Function Number
									1, //Byte Count
									(ULONG)SDIO_CCCR_IENx, //Offset
									FALSE, //Read operation
									&IENx);	
		
		if (!NT_SUCCESS(ntStatus))
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("Get REG_CCCR_IENx failed:%x\n", ntStatus));
			return ntStatus;
		}


		if(pDevice->bRegSdioDrivingEnable)
		{
			//
			// Set SDIO Driving capabilities
			// NOTICE:  If the host driving is too weak to reach here, 
			// 			this function will be no-use.
			//
			u1bData = 0;
			rtstatus = PlatformSdioCmd52ReadWrite(
										pDevice, 
										WLAN_IOREG_DEVICE_ID, //Device ID
										1, //Function Number
										1, //Byte Count
										(ULONG)0x74, //Offset
										FALSE, //Read operation
										&u1bData);	

			u1bData &= ~((BIT5)|(BIT6)|(BIT7));
			u1bData |= (pDevice->RegSdioDriving) << 5;

			RT_TRACE(COMP_INIT, DBG_LOUD, ("Set SDIO Driving to: %#x\n", u1bData));
			
			rtstatus = PlatformSdioCmd52ReadWrite(
										pDevice, 
										WLAN_IOREG_DEVICE_ID, //Device ID
										1, //Function Number
										1, //Byte Count
										(ULONG)0x74, //Offset
										TRUE, //Write operation
										&u1bData);	

			if (rtstatus != RT_STATUS_SUCCESS)
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Set SDIO Driving failed:%x\n", ntStatus));
				return ntStatus;
			}
		}

		//
		// Get Bus Bandwidth
		//
		ntStatus = PlatformSdioGetProperty(pDevice,
			   			SDP_BUS_WIDTH,
			                 	&pDevice->SdioBusWidth,
			                 	sizeof(pDevice->SdioBusWidth));  
		if(!NT_SUCCESS(ntStatus)){ 
				return ntStatus;
		}

		// Control SDIO Clock rate settings
		if(pDevice->bRegSdioSpeedSDR25)
		{
			N6SdioConfigureBusCLKByWorkAround(pDevice);
		}

		//
		// Get Bus clock
		//
		ntStatus = PlatformSdioGetProperty(pDevice,
			   			SDP_BUS_CLOCK,
			                	&pDevice->SdioBusClk,
			                	sizeof(pDevice->SdioBusClk));  
		if(!NT_SUCCESS(ntStatus)){ 
				return ntStatus;
		}


		//
		// Get Host Block size, the maximum block length that is supported by the controller
		//
		ntStatus = PlatformSdioGetProperty(pDevice,
			   			SDP_HOST_BLOCK_LENGTH,
			                 	&pDevice->SdioHostBlockSize,
			                	sizeof(pDevice->SdioHostBlockSize)); 
		if(!NT_SUCCESS(ntStatus)){ 
				return ntStatus;
		}
				

		//
		// Set SDIO Function Block Size
		//
		ntStatus = PlatformSdioSetProperty(pDevice,
			       		SDP_FUNCTION_BLOCK_LENGTH,
			                     &SdioFuncBlockSize,
			                     sizeof(SdioFuncBlockSize));
		if(!NT_SUCCESS(ntStatus)){ 
				return ntStatus;
		}


		//
		// Get SDIO Function Block length, i.e., Read I/O Block size for Function 1 in FBR register offset 0x110
		//
		ntStatus = PlatformSdioGetProperty(pDevice,
						SDP_FUNCTION_BLOCK_LENGTH,
			                 	&pDevice->SdioFuncBlockSize,
			                	sizeof(pDevice->SdioFuncBlockSize));
		if(!NT_SUCCESS(ntStatus)){ 
				return ntStatus;
		}

		if(pDevice->SdioFuncBlockSize != SDIO_BUS_SIZE)
			return STATUS_UNSUCCESSFUL;
		
		RT_TRACE(COMP_INIT, DBG_LOUD, ("==================SDIO Bus Info==================\n"));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioFuncBlockSize(%d)\n", pDevice->SdioFuncBlockSize));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioHostBlockSize(%d)\n", pDevice->SdioHostBlockSize));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioBusClk(%d)\n", pDevice->SdioBusClk));	
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioBusWidth(%d)\n", pDevice->SdioBusWidth));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioFuncNum(%d)\n", pDevice->SdioFuncNum));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioBusDriverVer(%d)\n", pDevice->SdioBusDriverVer));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioRevNum(%d)\n", pDevice->SdioRevNum));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): SdioFormatVer(%d)\n", pDevice->SdioFormatVer));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): CCCR IENx(%#x)\n", IENx));		
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioConfigureDevice(): Function Interrupt Enable(%d)\n", bEnableInt));
		RT_TRACE(COMP_INIT, DBG_LOUD, ("=================================================\n"));

	}
	else
	{ 
		ntStatus = STATUS_UNSUCCESSFUL;		
	}

	return ntStatus;
}



NDIS_STATUS
N6SdioRestart(
	IN  NDIS_HANDLE                         MiniportAdapterContext,
	IN  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
	)
/*++

Routine Description:

	NDIS calls the miniport driver's MiniportRestart function to cause
	the miniport to return the adapter to the Running state.
	During the restart, the miniport driver must complete any tasks
	that are required to resume send and receive operations before
	completing the restart request.

Argument:

	MiniportAdapterContext  Pointer to our adapter

Return Value:

	NDIS_STATUS_SUCCESS
	NDIS_STATUS_PENDING  Can it return pending
	NDIS_STATUS_XXX      The driver fails to restart


--*/
{
	PADAPTER pAdapter = (PADAPTER)MiniportAdapterContext;
	N6C_MP_DRIVER_STATE CurrDriverState;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("==> MiniportRestart(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	//
	// Reset rx packet waiting mechanism.
	//
	N6CStartWaitReturnPacketMechanism(pAdapter);

	//
	// Mark the miniport driver as running state to accept tx/rx request.
	//
	CurrDriverState = N6C_GET_MP_DRIVER_STATE(pAdapter);

	//RT_ASSERT(	(CurrDriverState == MINIPORT_PAUSED),
	//			("MiniportRestart(): unexpected state (%#d)!!!\n", CurrDriverState));

	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_RUNNING);

	{
		PADAPTER pTargetAdapter = GetFirstExtAdapter(pAdapter);
		while(pTargetAdapter != NULL)
		{
			N62CHelperHandleMiniportRestart(pTargetAdapter, MiniportRestartParameters);
			pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
		}
	}

	// To make sure that we will NOT fail to indicate phy status during initialization process.
	//DrvIFIndicateCurrentPhyStatus(pAdapter);
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== MiniportRestart(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));
	
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
N6SdioPause(
	IN  NDIS_HANDLE                         MiniportAdapterContext,
	IN  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
	)
/*++

Routine Description:

	Miniport pause and restart are new features introduced in NDIS 6.0.
	NDIS calls a miniport driver's MiniportPause function to stop
	data flow before a Plug and Play operation, such as adding or
	removing a filter driver or binding or unbinding a protocol driver,
	is performed. The adapter remains in the Pausing state until the
	pause operation has completed.

	When in the Paused state, a miniport driver can indicate status by
	calling the NdisMIndicateStatusEx function. In addition, the miniport
	driver should do the following:
	- Wait for all calls to the NdisMIndicateReceiveNetBufferLists
	    function to return.
	- Wait for NDIS to return the ownership of all NET_BUFFER_LIST
	    structures from outstanding receive indications to the miniport
	    driver's MiniportReturnNetBufferLists function.
	- Complete all outstanding send requests by calling NdisMSendNetBufferListsComplete.
	- Reject all new send requests made to its MiniportSendNetBufferLists
	    function by immediately calling the NdisMSendNetBufferListsComplete
	    function. The driver should set the completion status in each
	    NET_BUFFER_LIST structure to NDIS_STATUS_PAUSED.
	- Continue to handle OID requests in its MiniportOidRequest function.
	- Not stop the adapter completely if doing so prevents the driver
	    from handling OID requests or providing status indications.
	- Not free the resources that the driver allocated during initialization.
	- Continue to handle DevicePnPEvent notifications.

	NDIS calls the miniport driver's MiniportRestart function to cause
	the miniport to return the adapter to the Running state.
	During the restart, the miniport driver must complete any tasks
	that are required to resume send and receive operations before
	completing the restart request.

Argument:

	MiniportAdapterContext  Pointer to our adapter

Return Value:

	NDIS_STATUS_SUCCESS
	NDIS_STATUS_PENDING

NOTE: A miniport can't fail the pause request

--*/
{
	PADAPTER pAdapter = (PADAPTER)MiniportAdapterContext;
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	
	N6C_MP_DRIVER_STATE CurrDriverState;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("==> MiniportPause(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	//
	// Mark the miniport driver as pausing state to reject tx/rx request.
	//
	
	if(pAdapter->MgntInfo.NdisVersion < RT_NDIS_VERSION_6_20)
	{	
	CurrDriverState = N6C_GET_MP_DRIVER_STATE(pAdapter);
	RT_ASSERT(	(CurrDriverState < MINIPORT_PAUSED ||
				CurrDriverState > MINIPORT_PAUSING),
				("MiniportPause(): unexpected state (%#d)!!!\n", CurrDriverState));
	}
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSING);

	//
	// Wait all NBL indicated up returned.
	//
	N6CWaitForReturnPacket(pAdapter);

	if(!pAdapter->MgntInfo.bSetWatchDogTimerByDriver)
		CancelWatchDogTimer(pDefaultAdapter);

	//
	// Mark the miniport driver as paused state.
	//
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSED);

	//
	// Pause the ext adapter.
	//
	{
		PADAPTER pTargetAdapter = GetFirstExtAdapter(pAdapter);

		while(pTargetAdapter != NULL)
		{
			N62CHelperHandleMiniportPause(pTargetAdapter, MiniportPauseParameters); 
			pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
		}
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== MiniportPause(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));
	return NDIS_STATUS_SUCCESS;
}


VOID
N6SdioShutdown(
	IN  NDIS_HANDLE             MiniportAdapterContext,
	IN  NDIS_SHUTDOWN_ACTION    ShutdownAction
	)
/*++

Routine Description:

	The MiniportShutdown handler restores a NIC to its initial state when
	the system is shut down, whether by the user or because an unrecoverable
	system error occurred. This is to ensure that the NIC is in a known
	state and ready to be reinitialized when the machine is rebooted after
	a system shutdown occurs for any reason, including a crash dump.

	Here just disable the interrupt and stop the DMA engine.
	Do not free memory resources or wait for any packet transfers
	to complete.


	Runs at an arbitrary IRQL <= DIRQL. So do not call any passive-level
	function.

Arguments:

	MiniportAdapterContext  Pointer to our adapter

Return Value:

	None

--*/
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	KIRQL			Irql = KeGetCurrentIrql();
	PADAPTER 		pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(&Adapter->MgntInfo);


	RT_TRACE(COMP_INIT, DBG_LOUD, (" ==> N6SdioShutdown()\n"));

	if(Irql == PASSIVE_LEVEL)
	{
		//
		// Halt H/W.
		// 060118, rcnjko: 
		// We must do it for Vista DTM will shutdown DUT machine before starting NDISTEST6.5.
		// Otherwise, 87B F/W will default setting to respond what Host controler query 
		// which might different with the one in EEPROM, and therefore, 
		// NDISTEST 6.5 won't select the devise to test.
		//
		RT_TRACE(COMP_INIT, DBG_LOUD, ("calling NicIFHaltAdapter() by N6SdioShutdown()......\n"));

		Adapter->bDriverShutdown = TRUE;
		
{
		PADAPTER pAdapter =GetNextExtAdapter(pDefaultAdapter);
		while(pAdapter != NULL)
		{
			PADAPTER ptmpAdapter =GetNextExtAdapter(pAdapter);
			pAdapter->bDriverShutdown= TRUE;
			if( MultiPortIsActiveExtAdapter(pAdapter) == TRUE )
				N62CDeleteVirtualPort(pAdapter);
			pAdapter =ptmpAdapter;
		}
}

		//		
		// 2010/09/02 MH Before halt adapter, we need to leave ips/lps mode. Otherwise, selective suspend
		// power save level will be limited by IPS flag RT_RF_OFF_LEVL_HALT_NIC(add by Tynli at 2010/03)
		// 88/92C HW constraint, we can not use HW automatic shutdown sequence. According to Scoot and 
		// Alfred's suggestion (amil at 2010/09/02), if 8051 is alive and we try to close or reset clock. MAC hardware
		// handshake may fail and 8051 can not access MAC register. It will produce many error condition.
		// We need to close 8051 and execute firmware self reset to prevent HW power domain error.
		//
		LeaveAllPowerSaveMode(Adapter);

		// Leave HW 32K CLK
		if(RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K) && 
			!(IS_CARD_DISABLE_IN_FW_LOW_PWR_STATE(Adapter)))
		{
			u1Byte FwPSState;
			
			RT_CLEAR_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K); // To prevent from clock off again
			if(pDefaultAdapter->bFWReady)
			{
				pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
				if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState))
				{
					RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioHalt(): Leave Hw 32K CLK.\n"));
					pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
				}
			}
		}

		NicIFHaltAdapter(Adapter, FALSE);
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, (" <== N6SdioShutdown()\n"));
}


VOID
N6SdioPnPEventNotify(
	IN NDIS_HANDLE                  MiniportAdapterContext,
	IN PNET_DEVICE_PNP_EVENT        NetDevicePnPEvent
	)
/*++

Routine Description:

	MPPnPEventNotify is to handle PnP notification messages.
	An NDIS miniport driver with a WDM lower edge should export a
	MiniportPnPEventNotify function so that it can be notified
	when its NIC is removed without prior notification through
	the user interface. When a miniport driver receives
	notification of a surprise removal, it should note internally
	that the device has been removed and cancel any pending Requests
	that it sent down to the underlying bus driver. After calling
	the MiniportPnPEventNotify function to indicate the surprise
	removal, NDIS calls the miniportŠö MiniportHalt function.
	If the miniport driver receives any requests to send packets
	or query or set OIDs before its MiniportHalt function is
	called, it should immediately complete such requests with a
	status value of NDIS_STATUS_NOT_ACCEPTED.

	All NDIS 5.1 miniport drivers must export a MPPnPEventNotify
	function.

	Runs at IRQL = PASSIVE_LEVEL in the context of system thread.

	Available - NDIS5.1 (WinXP) and later.

Arguments:

	MiniportAdapterContext      Pointer to our adapter
	PnPEvent                    Self-explanatory

Return Value:

	None

--*/
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO		pMgntInfo=&(Adapter->MgntInfo);
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PVOID 			InformationBuffer = NetDevicePnPEvent->InformationBuffer;
	u8Byte	CurrentTime;

	RT_TRACE(COMP_INIT, DBG_LOUD, (" ==> N6SdioPnPEventNotify()\n"));
	
	switch(NetDevicePnPEvent->DevicePnPEvent)
	{
	case NdisDevicePnPEventSurpriseRemoved:
			RT_TRACE(COMP_INIT, DBG_LOUD,  ("N6SdioPnPEventNotify: NdisDevicePnPEventSurpriseRemoved\n"));
			Adapter->bSurpriseRemoved = TRUE;
			Adapter->SurpriseRemovedReason = HALT_BY_PNP_EVENT;

			// Release Tx semaphore before cancel corresponding Tx thread.
			PlatformReleaseSemaphore(&device->TxSemaphore);

#if (RK_PLATFORM_SUPPORT == 1)
			PlatformReleaseMutex(&device->RxHandleIntMutex);
#endif

#if RTL8723_SDIO_IO_THREAD_ENABLE 
			// Release IO semaphore before cancel corresponding IO thread.
			PlatformReleaseSemaphore(&device->IOSemaphore);
#endif

			RT_DISP(FIOCTL, IOCTL_IRP_HANDLE, ("[IRP], complete driver unload IRP due to surprise removed!!!\n"));
			PlatformIndicateCustomStatus(
				Adapter,
				RT_CUSTOM_EVENT_IRP_UNLOAD,
				RT_CUSTOM_INDI_TARGET_IRP,
				NULL,
				0);
			
			N6SdioReturnAllPendingTxPackets(Adapter);

			if(OS_SUPPORT_WDI(Adapter))
			{
				if(Adapter->pPortCommonInfo->WdiData.bCommandHangHappened == TRUE)
				{
					WDICommandHangCleanup(Adapter);
				}
				WDI_PnPNotiry(Adapter);
			}

			//
			// <Roger_Notes> Beacuse our NIC might be unplugged before system resume or under initialization.
			// Which will cause BugCheck code 0x1d i.e., MiniPortCheckForHang calling back after driver was unloaded.
			// We simply defer specific interval to fix this issue in passive level.
			// To make sure that MiniPortCheckForHang has called back before driver halted.
			// 2009.09.23.
			//
			if(!device->bChkForHangAfterPnP)
			{
				CurrentTime = PlatformGetCurrentTime();
				NdisWaitEvent(&device->SetPnpChkForHangEvent, RT_CHECK_FOR_HANG_PERIOD*1000);
				RT_TRACE(COMP_INIT, DBG_TRACE, ("N6SdioPnPEventNotify(): Time spent %I64d us\n", 
					PlatformGetCurrentTime()-CurrentTime));				
			}
			break;

	case NdisDevicePnPEventPowerProfileChanged:
			pPSC->PowerProfile = *( ( PULONG ) InformationBuffer );
			RT_TRACE(COMP_INIT, DBG_LOUD, ("NdisDevicePnPEventPowerProfileChanged- PowerProfile(%d)\n",pPSC->PowerProfile));
			N6SdioPowerSettingChangeNotify(Adapter);
			break;
	
	case NdisDevicePnPEventQueryStopped:
	case NdisDevicePnPEventStopped:          

	case NdisDevicePnPEventQueryRemoved:
	case NdisDevicePnPEventRemoved:     
	default:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioPnPEventNotify: unknown PnP event %x \n", NetDevicePnPEvent->DevicePnPEvent));
			break;         
    }
	RT_TRACE(COMP_INIT, DBG_LOUD, (" <== N6SdioPnPEventNotify()\n"));
	return;
}

NDIS_STATUS
N6SdioPowerSettingCallback(
	IN 	CONST LPGUID 	SettingGuid,
    	IN 	PULONG      		 Value,
    	IN	ULONG       		 ValueLength,
	IN 	PVOID       		 MiniportAdapterContext
	)
{
	PADAPTER     		Adapter = (PADAPTER) MiniportAdapterContext;
	NDIS_STATUS 	Status = STATUS_INVALID_PARAMETER;    
	GUID			PowerSetting_GUID = GUID_ACTIVE_POWERSCHEME;//GUID_POWERSCHEME_PERSONALITY;
	GUID			MaxPowerSaving_GUID = GUID_MAX_POWER_SAVINGS;
	GUID			TypicalPowerSaving_GUID = GUID_TYPICAL_POWER_SAVINGS;
	GUID			MinPowerSaving_GUID = GUID_MIN_POWER_SAVINGS;

	if( 	IsEqualGUID((LPGUID)(&PowerSetting_GUID), SettingGuid)
        	&& (ValueLength == sizeof(GUID)) && Value != NULL ) 
        {
       	PMGNT_INFO		pMgntInfo=&(Adapter->MgntInfo);
		PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

        	if(IsEqualGUID((LPGUID)&MaxPowerSaving_GUID,(LPGUID)Value) )
        	 	  pPSC->PowerSetting = POWERGUID_MAX_POWER_SAVINGS;
		else if(IsEqualGUID((LPGUID)&TypicalPowerSaving_GUID,(LPGUID)Value)) 
            		 pPSC->PowerSetting = POWERGUID_TYPICAL_POWER_SAVINGS;
		else if(IsEqualGUID((LPGUID)&MinPowerSaving_GUID,(LPGUID)Value) )
			 pPSC->PowerSetting = POWERGUID_MIN_POWER_SAVINGS;
		
		RT_TRACE(COMP_POWER, DBG_LOUD, ("%s PowerSetting(%d)\n", __FUNCTION__, pPSC->PowerSetting));	
		
		N6SdioPowerSettingChangeNotify(Adapter);	
		 Status = STATUS_SUCCESS;
        }
	return Status;
}

NDIS_STATUS
N6SdioWLANPowerModeCallback(
	IN 	CONST LPGUID 	SettingGuid,
    	IN 	PULONG      		 Value,
    	IN	ULONG       		 ValueLength,
	IN 	PVOID       		 MiniportAdapterContext
	)
{
	PADAPTER     		Adapter = (PADAPTER) MiniportAdapterContext;
	NDIS_STATUS 	Status = STATUS_INVALID_PARAMETER;    
	GUID			WLANPowerMode_GUID = GUID_WLAN_POWER_MODE;
	u1Byte			WLAN_Maximum_Performance[16] =
	{0x4f,0x16,0xab,0xc1,0x4f,0x83,0x3d,0x46, 0x85, 0x44, 0xa4, 0x0e, 0x93, 0xab, 0x54, 0x72}; 
	u1Byte			WLAN_Low_Power_Saving[16] =
	{0xcb,0xbc,0x7c,0x78,0x4b,0xcd,0x76,0x47,0x8b, 0xe5, 0x5f, 0x8a, 0xe4, 0x72, 0x6f, 0x2b}; 
	u1Byte			WLAN_Medium_Power_Saving[16] =
	{0x12,0xe4,0x28,0x67,0xd1,0x40,0xb0,0x4a,0x8d, 0x15, 0xf3, 0xc5, 0x6f, 0x30, 0x3e, 0xb5};
	u1Byte			WLAN_Maximum_Power_Saving[16] =
	{0x0F,0xDC,0x12,0xE0,0x97,0x83,0xB5,0x46,0xA0, 0x60, 0x0D, 0xE8, 0x4F, 0x96, 0x38, 0x8E};

	if( IsEqualGUID((LPGUID)(&WLANPowerMode_GUID), SettingGuid)
        	&& (ValueLength == sizeof(GUID)) && Value != NULL ) 
        {
       	PMGNT_INFO		pMgntInfo=&(Adapter->MgntInfo);
		PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

        	if(PlatformCompareMemory(WLAN_Maximum_Power_Saving, Value, ValueLength)==0)
        	 	  pPSC->PowerMode = POWER_SAVING_MAXIMUM_LEVEL;
		else if(PlatformCompareMemory(WLAN_Medium_Power_Saving, Value, ValueLength)==0) 
            		 pPSC->PowerMode = POWER_SAVING_MAX_PSP;
		else if(PlatformCompareMemory(WLAN_Low_Power_Saving, Value, ValueLength)==0)
			 pPSC->PowerMode = POWER_SAVING_FAST_PSP;
		else if(PlatformCompareMemory(WLAN_Maximum_Performance, Value, ValueLength)==0)
			 pPSC->PowerMode = POWER_SAVING_NO_POWER_SAVING;

		RT_TRACE(COMP_POWER, DBG_LOUD, ("%s PowerMode(%d)\n", __FUNCTION__, pPSC->PowerMode));	
		
		N6SdioPowerSettingChangeNotify(Adapter);	
		 Status = STATUS_SUCCESS;
        }
	return Status;
}

VOID
N6SdioPowerSettingChangeNotify(
	IN     PADAPTER	Adapter
	)
{
	PMGNT_INFO			pMgntInfo=&(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioPowerSettingChangeNotify()\n"));
	
	Adapter->HalFunc.UpdateIPSStatusHandler(Adapter, pNdisCommon->RegInactivePsMode);
	Adapter->HalFunc.UpdateLPSStatusHandler(Adapter, pNdisCommon->RegLeisurePsMode, pNdisCommon->RegPowerSaveMode);
		
	RT_TRACE(	COMP_POWER, DBG_LOUD, ("%s : PowerPolicy %d bInactivePs=%d bLeisurePS=%d  <==\n", 
				__FUNCTION__, pPSC->PowerPolicy, pPSC->bInactivePs, pPSC->bLeisurePs));
}

VOID
N6SdioHalt(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HALT_ACTION        HaltAction
    )
/*++

Routine Description:

    Halt handler is called when NDIS receives IRP_MN_STOP_DEVICE,
    IRP_MN_SUPRISE_REMOVE or IRP_MN_REMOVE_DEVICE requests from the
    PNP manager. Here, the driver should free all the resources acquired
    in MiniportInitialize and stop access to the hardware. NDIS will
    not submit any further request once this handler is invoked.

    1) Free and unmap all I/O resources.
    2) todo  Disable interrupt and deregister interrupt handler.
    3) Cancel all queued up timer callbacks.
    4) Finally wait indefinitely for all the outstanding receive
        packets indicated to the protocol to return.

    MiniportHalt runs at IRQL = PASSIVE_LEVEL.


Arguments:

    MiniportAdapterContext  Pointer to our adapter
    HaltAction              The reason adapter is being halted

Return Value:

    None.

--*/
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	PADAPTER 		pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	NTSTATUS		ntStatus = STATUS_SUCCESS;
	BOOLEAN			bCanceled;
	PHAL_DATA_TYPE pHalData=GET_HAL_DATA(Adapter);
	PRT_SDIO_DEVICE sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);	

	RT_TRACE(COMP_INIT, DBG_LOUD, (" ==> N6SdioHalt()\n"));

	NDBG_Halt(Adapter);
	
	Adapter->bDriverIsGoingToUnload = TRUE;
	
	{
		PADAPTER pAdapter = GetNextExtAdapter(pDefaultAdapter);
		while(pAdapter != NULL)
		{
			PADAPTER ptmpAdapter = GetNextExtAdapter(pAdapter);
			pAdapter->bDriverIsGoingToUnload = TRUE;
			if( MultiPortIsActiveExtAdapter(pAdapter) == TRUE )
				N62CDeleteVirtualPort(pAdapter);
			pAdapter = ptmpAdapter;
		}
	}

	PoUnregisterPowerSettingCallback(pNdisCommon->N6PowerSettingHandle);
	PoUnregisterPowerSettingCallback(pNdisCommon->N6WLANPowerModeHandle);
	//BT_HaltNotify(Adapter);
	MgntResetJoinProcess(Adapter);
	N6C_DeregisterIoDevice(Adapter);

	REMOVE_GLOBAL_ADAPTER_LIST(Adapter);
	
	if(ACTING_AS_AP(pDefaultAdapter))
	{
		AP_DisassociateAllStation(pDefaultAdapter, unspec_reason);
		PlatformSleepUs(500);
		PlatformStopWorkItem( &(pMgntInfo->TbttPollingWorkItem));
		PlatformCancelTimer(pDefaultAdapter, &pDefaultAdapter->MgntInfo.pChannelInfo->SwBwTimer);
		PlatformCancelTimer(pDefaultAdapter, &(pMgntInfo->SwBeaconTimer));

	}

	PlatformStopWorkItem( &(pHalData->RtCheckForHangWorkItem) );

	LeaveAllPowerSaveMode(Adapter);

	// Leave HW 32K CLK
	if(RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K) &&
		!IS_CARD_DISABLE_IN_FW_LOW_PWR_STATE(Adapter))
	{
		u1Byte FwPSState;
		
		RT_CLEAR_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K); // To prevent from setting clock off again
		if(pDefaultAdapter->bFWReady)
		{
			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
			if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState))
			{
				RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioHalt(): Leave Hw 32K CLK.\n"));
				pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
			}
		}
	}

	NicIFClearInterrupt(Adapter);

	//sync with pcie,wait for RF change complete 
	PlatformAcquireSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
	while( pMgntInfo->RFChangeInProgress)
	{
		PlatformReleaseSpinLock(Adapter,RT_RF_STATE_SPINLOCK);

		RT_TRACE(COMP_POWER,DBG_LOUD,
			("RF is in progress, need to wait until rf chang is done.\n"));
		delay_ms(1);
		
		PlatformAcquireSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
	}	
	PlatformReleaseSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
	
	//
	// 2010/08/13 MH We need to move the flag setting after LeaveAllPowerSaveMode().
	// Otherwise, we may not leave IPS and call NicIFHaltAdapter. This will block the halt sequence
	// or S3/S4 enter sequence for about 10010000 us.
	//
	Adapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;

	//
	// Halt H/W.
	//
	NicIFHaltAdapter(Adapter, FALSE);
	
	Adapter->HalFunc.WaitForH2CQueueEmptyHandler(Adapter);

	if(((pMgntInfo->RfOffReason & (RF_CHANGE_BY_IPS|RF_CHANGE_BY_SW|RF_CHANGE_BY_HW)) &&
		RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K)) || pPSC->bInDxFwIPSPeriod)
	{
		// Wait FW 32K Event
		// Prefast warning C28121: The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
		// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
		// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
		if(!NdisWaitEvent(&sdiodevice->FwPsClockOffEvent, 20))
		{
			// Force Hw clock off to 32K
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Force Hw clock off to 32K!\n"));
			SetFwPSRFOffLowPower(Adapter);
		}
	}
	
	if(device->bSdioBusClkRaised)
	{
		N6SdioConfigureBusCLK_SDR12(device);
	}

	// <NOTE> We MUST perform necessary IO before setting bDriverStopped and bSurpriseRemoved flags, 
	// since we will reject further Sync IO in RTusbCallUSBD() if bDriverStopped || bSurpriseRemoved.
	// 2005.01.13, by rcnjko.
	Adapter->bDriverStopped = TRUE;

	//
	// Mark the miniport driver as halted state.
	//
//	RT_ASSERT( (N6C_GET_MP_DRIVER_STATE(Adapter) == MINIPORT_PAUSED), 
//		("MiniportHalt(): DriverState(%d) != PAUSED !!!\n", N6C_GET_MP_DRIVER_STATE(Adapter)));
	N6C_SET_MP_DRIVER_STATE(Adapter, MINIPORT_HALTED);

	// SyncIo Method 2.  
	// We must wake up the workitems waiting for SyncIo event before, 
	// waiting all workitem completed. 2005.01.06, by rcnjko.
	if(device->SyncIoWaitingCount>0)
	{ // If there is workitem waiting.
		RT_TRACE(COMP_DBG, DBG_LOUD,("RTusbHalt(): there is workitem is waiting...\n"));
		KeSetEvent( &(device->SyncIoEvent), 0, FALSE);
	}
#if 0
	// DeInitialize for debugging realted resource. 2005.04.21, by rcnjko.
	N6SdioDbgHalt(Adapter);
#endif
	// Release common NDIS resource in PRT_NDIS6_COMMON. 2006.05.07, by rcnjko.
	if(pMgntInfo->bSupportPacketCoalescing)
	{
		// All D0 close packet !!
		N6FulshD0CoalescingQueue(Adapter, TRUE, FALSE);
	}
	ReleaseNdis6CommonResources(Adapter);

#if POWER_MAN
	//PNP
	//20061206 david
	PlatformFreeWorkItem( &(device->PnpWorkItem) );
#endif //powerman


// =======================
// This part shall be move to COMMON folder.
	// RemoveAllTS must call here!!
	// This function shall be called before MPShutdown().
	// For Traffic Stream. Remove all TS before HALT
	
	NicIFDisableInterrupt(Adapter);
	RemoveAllTS(Adapter);
	ReleaseAllTSTimer(Adapter);
	PlatformCancelTimer(Adapter, &pMgntInfo->pChannelInfo->SwBwTimer);
//=========================

	// Free native 802.11 MIBs, 2006.10.10, by shien chang.
	N6SdioFreeNative80211MIBs(Adapter);
	
#if USE_WDF_SDIO
	WdfSdio_Disable(Adapter);
#else
	N6WdmSdio_Disable(Adapter);
#endif

	// Release Tx semaphore before cancel corresponding Tx thread.
	PlatformReleaseSemaphore(&device->TxSemaphore);

#if (RK_PLATFORM_SUPPORT == 1)
	PlatformReleaseMutex(&device->RxHandleIntMutex);
#endif

	// Free Tx semaphore.
	PlatformFreeSemaphore(&device->TxSemaphore);

#if RTL8723_SDIO_IO_THREAD_ENABLE 
	// Release IO semaphore before cancel corresponding IO thread.
	PlatformReleaseSemaphore(&device->IOSemaphore);

	// Free IO semaphore.
	PlatformFreeSemaphore(&device->IOSemaphore);
#endif

	N6SdioCancelAllTimer( Adapter );

	//
	// <Roger_Notes> We should configure our EHCI into J-State to save more power consumption
	// while NIC is still plug-in but disabled.
	// 2010.05.02.
	//
	if(!Adapter->bSurpriseRemoved)
	{
		// TODO
	}

	//----------------------------------------------------------------------------
	// NOTE! Adapter is released in N6SdioFreeAll(), 
	// so, we MUST NOT access Adapter after it. 2006.03.08, by rcnjko. 
	//----------------------------------------------------------------------------
	N6SdioFreeAll( Adapter );
#if (WPP_SOFTWARE_TRACE == 0)
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_AllocateMemory_count = %"i64fmt"d\n",RTL_AllocateMemory_count));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_FreeMemory_count = %"i64fmt"d\n",RTL_FreeMemory_count));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_AllocateMemory_Len = %"i64fmt"d\n",RTL_AllocateMemory_Len));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_FreeMemory_Len = %"i64fmt"d\n",RTL_FreeMemory_Len));

	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_P2P_AllocateMemory_count = %"i64fmt"d\n",RTL_P2P_AllocateMemory_count));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_P2P_FreeMemory_count = %"i64fmt"d\n",RTL_P2P_FreeMemory_count));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_P2P_AllocateMemory_Len = %"i64fmt"d\n",RTL_P2P_AllocateMemory_Len));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("  RTL_P2P_FreeMemory_Len = %"i64fmt"d\n",RTL_P2P_FreeMemory_Len));
#endif	
	RT_TRACE(COMP_INIT, DBG_LOUD, (" <== N6SdioHalt()\n"));
	return;
}


VOID
N6SdioUnload(
	IN  PDRIVER_OBJECT  DriverObject
	)
/*++

Routine Description:

    The unload handler is called during driver unload to free up resources
    acquired in DriverEntry. Note that an unload handler differs from
    a MiniportHalt function in that the unload handler has a more global
    scope, whereas the scope of the MiniportHalt function is restricted
    to a particular miniport driver instance.

    During an unload operation, an NDIS 6.0 miniport driver must deregister
    the miniport driver by calling NdisMDeregisterMiniportDriver
    and passing the MiniportDriverHandle as a parameter.
    The driver obtained the MiniportDriverHandle when it called
    NdisMRegisterMiniportDriver.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    DriverObject        Not used

Return Value:

    None

--*/
{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("==> N6SdioUnload()\n"));
	
	UNREFERENCED_PARAMETER(DriverObject);

#if USE_WDF_SDIO
	WdfDriverMiniportUnload(WdfGetDriver());
#endif	

#if (WDI_SUPPORT == 1)	
    if(NdisGetVersion() >= NDIS_VERSION_BASE_6_50)
    {
        WDI_DeInitialize(GlobalRtDriverContext.NdisContext.Ndis6MiniportDriverHandle);
    }
    else
#endif
	{
		NdisMDeregisterMiniportDriver(GlobalRtDriverContext.NdisContext.Ndis6MiniportDriverHandle);
	}

	GlobalRtDriverContext.NdisContext.Ndis6MiniportDriverHandle = NULL;
	
	RT_ASSERT(RTIsListEmpty(&(GlobalRtDriverContext.AdapterList)), ("GlobalRtDriverContext.AdapterList is not empty!\n"));
	RT_ASSERT(RTIsListEmpty(&(GlobalRtDriverContext.NdisContext.IoDeviceList)), ("IoDevice List is not empty!\n"));
	PLATFORM_FREE_RT_SPINLOCK(GlobalRtDriverContext.ContextLock);

	/* Add for DBG ver. memory alloc/free, 2014.04.24, hsiao_ho */
	RtDbgDeinitMem();
    /* Add for DBG ver. Mutex, 2014.05.19, hsiao_ho */
    RtDbgDeinitMutex();
	/* Add for DBG ver. Spinlock, 2014.05.21, hsiao_ho */
	RtDbgDeinitSpinlock();

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== N6SdioUnload()\n"));
	
#if SOFTWARE_TRACE_LOGGING
	TraceLoggingUnregister(g_hProvider);
#endif
	
#if WPP_SOFTWARE_TRACE
	WPP_CLEANUP(DriverObject);
#endif

}


NDIS_STATUS
N6SdioOidRequest(
	IN  NDIS_HANDLE         MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST   NdisRequest
	)
/*++
Routine Description:

    NDIS calls a miniport driver's MiniportOidRequest function to send an OID
    request to the driver. These OID requests are serialized so the
    MiniportOidRequest function is not called until the pending OID request
    has completed. MiniportOidRequest can be called at either IRQL 
    PASSIVE_LEVEL or IRQL DISPATCH_LEVEL. 
    
    If the MiniportOidRequest function returns NDIS_STATUS_PENDING, 
    the miniport driver must subsequently call the NdisMOidRequestComplete
    function to complete the request. NdisMOidRequestComplete can be called 
    at either IRQL DISPATCH_LEVEL or IRQL PASSIVE_LEVEL. 

Arguments:

    MiniportAdapterContext  Pointer to the adapter structure
    NdisRequest             Pointer to NDIS_OID_REQUEST
    
Return Value:
    
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_BUFFER_TOO_SHORT
    
--*/
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);	
	PADAPTER		pTargetAdapter = GetAdapterByPortNum(Adapter, (u1Byte)NdisRequest->PortNumber);
	NDIS_STATUS		ndisStatus=NDIS_STATUS_NOT_RECOGNIZED;
	u2Byte			i;
	BOOLEAN			bSetOPMode=FALSE;
	u4Byte			portType = 0;
	KIRQL 			irql = NDIS_CURRENT_IRQL();

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> N6SdioOidRequest()\n"));

	NdisOIDHistoryUpdate(Adapter, NdisRequest, RT_OID_HISTORY_STARTED);

	do
	{
		if(OS_SUPPORT_WDI(Adapter))
		{
			ndisStatus = WDI_HandleOidRequest(
							MiniportAdapterContext,
							NdisRequest
							);
		}

		if( NDIS_STATUS_NOT_RECOGNIZED == ndisStatus )
		{
			if(!N6_INIT_READY(Adapter))		
			{
				if(	(NdisRequest->RequestType==NdisRequestMethod|| NdisRequest->RequestType==NdisRequestSetInformation) &&
					(NdisRequest->DATA.QUERY_INFORMATION.Oid==OID_DOT11_SCAN_REQUEST)	)
				{
					RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6SdioOidRequest(): The pending oid is Scan request\n"));
					pMgntInfo->bPendScanOID = TRUE;
					pMgntInfo->bCheckScanTime = TRUE;
					pMgntInfo->ScanOnlyStartTime = PlatformGetCurrentTime();
				}
			}

			if(InitializePendingOIDBeforeReady(Adapter, NdisRequest, &ndisStatus))
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6SdioOidRequest(): OID(%#X) when initializing. return PENDING\n", (u4Byte)NdisRequest->DATA.QUERY_INFORMATION.Oid));
				break;
			}

			if((N6C_GET_MP_DRIVER_STATE(Adapter)==MINIPORT_HALTED))
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== N6SdioOidRequest(): driver halted.\n"));
				ndisStatus = NDIS_STATUS_FAILURE;
				break;
			}

			ndisStatus = N6CProcessOidRequest(Adapter, NdisRequest, FALSE);
		}		
	}while(FALSE);

	if(OS_SUPPORT_WDI(Adapter))
		return ndisStatus;

	if( irql < DISPATCH_LEVEL ){
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6SdioOidRequest(): Raise IRQL!!\n"));
		KeRaiseIrql(DISPATCH_LEVEL, &irql);
	}

	switch(ndisStatus)
	{
		case NDIS_STATUS_PENDING:
			//
			// We have pended this OID, so we shall save this OID for OID completion indication later.
			//
			PlatformAcquireSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
			if( Adapter->pNdisCommon->PendedRequest != NULL )
			{
				PlatformReleaseSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
				RT_ASSERT(Adapter->pNdisCommon->PendedRequest == NULL, ("N6SdioOidRequest(): We have already pended one OID (%p)\n", Adapter->pNdisCommon->PendedRequest));
				N6CompletePendedOID(Adapter, RT_PENDED_OID_DONT_CARE, NDIS_STATUS_FAILURE);
				PlatformAcquireSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
			}

			Adapter->pNdisCommon->PendedRequest = NdisRequest;
			NdisOIDHistoryUpdate(Adapter, NdisRequest, RT_OID_HISTORY_PENDING);
			PlatformReleaseSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6SdioOidRequest(): Pend this OID(%p, ID =0x%08X)\n", 
				Adapter->pNdisCommon->PendedRequest, NdisRequest->DATA.METHOD_INFORMATION.Oid));
			break;

		case NDIS_STATUS_SUCCESS:
			NdisOIDHistoryUpdate(Adapter, NdisRequest, RT_OID_HISTORY_COMPLETE);
			break;

		default:
			NdisOIDHistoryUpdate(Adapter, NdisRequest, RT_OID_HISTORY_UNKNOWN);
			break;
	}

	if( irql < DISPATCH_LEVEL ){
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6SdioOidRequest(): Restore IRQL!!\n"));
		KeLowerIrql(irql);	
	}

	return ndisStatus;
}


VOID
N6SdioCancelOidRequest(
	IN NDIS_HANDLE hMiniportAdapterContext,
	IN PVOID       RequestId
	)
/*++
Routine Description:

	An NDIS 6.0 miniport driver must provide a MiniportCancelOidRequest
	function if it pends any OID request. MiniportCancelOidRequest 
	is called when NDIS or an overlying driver calls NdisCancelOidRequest 
	or NdisFCancelOidRequest. If the miniport driver finds the OID 
	request to be cancelled in its queue, it should complete the OID request
	by calling NdisMOidRequestComplete with NDIS_STATUS_REQUEST_ABORTED.

Arguments:

	MiniportAdapterContext  Pointer to the adapter structure
	RequestId               Specify the request to be cancelled.
    
Return Value:
    
--*/
{
	// TODO: Implement It!
}


NDIS_STATUS
N6SdioSetOptions(
	IN NDIS_HANDLE  NdisMiniportDriverHandle,
	IN NDIS_HANDLE  MiniportDriverContext
	)
/*++

Routine Description:

	MiniportSetOptions function is called by NDIS to ask the
	miniport driver to register its optional handlers.
	A miniport driver must call NdisSetOptionalHandler 
	from its MiniportSetOptions function to register its
	optional handlers with NDIS.

Arguments:


Return Value:

	None

--*/
{
	// TODO: Implement It!
	return NDIS_STATUS_SUCCESS;
}


//
//	Description:
//		This function is called by NDIS when the OS submits a list of NBL for transmission to
//		this miniport. The NBLs will be transmitted immediately if possible, or otherwise
//		is queued up for later trannmission by the miniport.
//
//	Input:
//		MiniportAdapterContext        The context for the instance of miniport driver
//		pNetBufferLists               The chain of NBL to be transmitted
//		PortNumber                    Port number for the packets
//		SendFlags                     Flags such as dispatch level, etc for send
//
//	Note:
//		1. IRQL might not be DISPATCH level.
//
//
VOID
N6SdioSendNetBufferLists(
	IN  NDIS_HANDLE         MiniportAdapterContext,
	IN  PNET_BUFFER_LIST    pNetBufferLists,
	IN  NDIS_PORT_NUMBER    PortNumber,
	IN  ULONG               SendFlags
	)
{
	PADAPTER Adapter = GetAdapterByPortNum((PADAPTER)MiniportAdapterContext, (u1Byte) PortNumber);
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(Adapter);
	BOOLEAN				bDispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);
	PNET_BUFFER_LIST	pCurrNetBufferList, pNextNetBufferList;
	PNET_BUFFER			pNetBuffer;
	ULONG				NetBufferCount;

	//
	// Drop the packets to send if we are not in proper state.
	//
	if( N6SDIO_CANNOT_TX(Adapter) )
	{
		RT_TRACE(COMP_SEND, DBG_LOUD, 
			("N6SdioSendNetBufferLists(): drop packet for N6SDIO_CANNOT_TX(), bDriverStopped(%d) or bSurpriseRemoved(%d)!\n", 
			Adapter->bDriverStopped, Adapter->bSurpriseRemoved));
		if(N6C_GET_MP_DRIVER_STATE(Adapter) <= MINIPORT_PAUSING)
		{// Indicate pause state while we're in pause state. added by Roger, 2008.03.13.
			RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioSendNetBufferLists(): NDIS_STATUS_PAUSED...\n"));
			N6CompleteNetBufferLists(Adapter, pNetBufferLists, NDIS_STATUS_PAUSED, bDispatchLevel);
		}
		else
		{
			N6CompleteNetBufferLists(Adapter, pNetBufferLists, NDIS_STATUS_SUCCESS, bDispatchLevel);
		}
		return;
	}

	//
	// TODO: we shall buffer packet to send, do it in Common\. So, fix me!
	//
	if(MgntRoamingInProgress(pMgntInfo))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("N6SdioSendNetBufferLists(): Roaming in Progress(%d)\n", pMgntInfo->RoamingType));
		N6CompleteNetBufferLists(Adapter, pNetBufferLists, NDIS_STATUS_SUCCESS, bDispatchLevel);
		return;
	}

	if(pMgntInfo->bSetPnpPwrInProgress)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioSendNetBufferLists(): PnP operation is in progress, complete NBLs immediately\n"));
		N6CompleteNetBufferLists(Adapter, pNetBufferLists, NDIS_STATUS_SUCCESS, bDispatchLevel);
		return;
	}

	if(Adapter->bResetInProgress)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("N6SdioSendNetBufferLists(): Reset!!!\n"));
		N6CompleteNetBufferLists(Adapter, pNetBufferLists, NDIS_STATUS_SUCCESS, bDispatchLevel);
		return;
	}

	if(DFS_IsTxDisabled(Adapter))
	{
		N6CompleteNetBufferLists(Adapter, pNetBufferLists, NDIS_STATUS_SUCCESS, bDispatchLevel);
		return;
	}

	PlatformAcquireSpinLock(Adapter, RT_BUFFER_SPINLOCK);

	//
	// Send each NBL in list.
	//
	for(pCurrNetBufferList = pNetBufferLists;
		pCurrNetBufferList != NULL;
		pCurrNetBufferList = pNextNetBufferList)
	{
		pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);

		//
		// Take one NBL from NBL chains.
		//
		NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList) = NULL;

		//
		// Initialize reference count as (#NB + 1) to prevent this 
		// NBL being completed before end of N6SdioSendSingleNetBufferList().
		//
		NetBufferCount = 0;
		for(pNetBuffer = NET_BUFFER_LIST_FIRST_NB(pCurrNetBufferList);
			pNetBuffer != NULL;
			pNetBuffer = NET_BUFFER_NEXT_NB(pNetBuffer))
		{
			NetBufferCount ++;
		}

		RT_NBL_SET_REF_CNT(pCurrNetBufferList, (NetBufferCount+1));

		// Prevent from Tx out-of-order
		{
			RT_NBL_NEXT_SEND(pCurrNetBufferList) = NET_BUFFER_LIST_FIRST_NB(pCurrNetBufferList);

			N6CAddNblWaitQueue(
				&pNdisCommon->TxNBLWaitQueue,
				pCurrNetBufferList, 
				FALSE); // bToHead.
		}
	}

	PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);

	if(!ACTING_AS_AP(Adapter) && !ACTING_AS_IBSS(Adapter) && pMgntInfo->SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER )
	{
		if(pMgntInfo->bSendPacketByTimer && SecIsTxKeyInstalled(Adapter, pMgntInfo->Bssid))
		{
			PlatformSetTimer(Adapter, &(pNdisCommon->N6CSendSingleNetBufferListTimer),0);
			return;
		}
	}
	else
	{
		PlatformSetTimer(Adapter, &(pNdisCommon->N6CSendSingleNetBufferListTimer),0);
		return;
	}
	
	if(PlatformAtomicExchange(&Adapter->IntrNBLRefCount, TRUE)==TRUE)
		return;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(pDevice->SendingNetBufferList == NULL)
	{
		while(TRUE)
		{
			PlatformAcquireSpinLock(Adapter, RT_BUFFER_SPINLOCK);
			if(N6CIsNblWaitQueueEmpty(pNdisCommon->TxNBLWaitQueue))
			{
				PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
				break;
			}
			
			if(!pNdisCommon->bReleaseNblWaitQueueInProgress)
			{
				pCurrNetBufferList = N6CGetHeadNblWaitQueue(pNdisCommon->TxNBLWaitQueue);
				PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
				
				RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioSendNetBufferLists(): N6SdioSendSingleNetBufferList pCurrNetBufferList(%p)\n", pCurrNetBufferList));
				if (!N6SdioSendSingleNetBufferList(
					Adapter, 
					pCurrNetBufferList, 
					TRUE) )
				{
					RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioSendSingleNetBufferList() returns FALSE\n"));
					break;
				}
			}
			else
			{
				PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
			}
		}
		
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	PlatformAtomicExchange(&Adapter->IntrNBLRefCount, FALSE);
}



//
//	Description:
//		Process each NB in the NBL.
//
//	Output:
//		Return FALSE if we can send this NBL and had put it into 
//		wait queue if necessary, TRUE otherwise.
//
//	Assumption:
//		1. RT_TX_SPINLOCK is acquired.
//
BOOLEAN
N6SdioSendSingleNetBufferList(
	IN  PADAPTER			Adapter,
	IN  PNET_BUFFER_LIST	pNetBufferList,
	IN	BOOLEAN				bFromQueue
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(Adapter);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	PNET_BUFFER		pCurrNetBuffer, pNextNetBuffer;

	PDOT11_EXTSTA_SEND_CONTEXT	pDot11SendContext;
	u2Byte			ExemptionActionType = 0; // 0:Auto 1:encrypt 2:no encrypt 3: key UNAVAILABLE
	u1Byte			Priority = 0;
	NDIS_NET_BUFFER_LIST_8021Q_INFO	NBL8021qInfo;

	PRT_TCB			pTcb;
	NDIS_STATUS		ndisStatus;

	//RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioSendSingleNetBufferList(), start to process pNetBufferList=0x%x\n", (u4Byte)pNetBufferList));
	//
	// Because we'll release TX lock before doing scatter/gather operation,
	// we use SendingNetBufferList to prevent out of order. 
	//
	// Prefast warning ignore for false positive
#pragma warning( disable:6271 )
	RT_ASSERT(pDevice->SendingNetBufferList == NULL, ("SendingNetBufferList should be NULL\n!!!", pDevice->SendingNetBufferList));
	pDevice->SendingNetBufferList = pNetBufferList;

	//
	// Parse per-NBL information.
	//
	pDot11SendContext = (PDOT11_EXTSTA_SEND_CONTEXT) NET_BUFFER_LIST_INFO(pNetBufferList, MediaSpecificInformation);   
	switch( pDot11SendContext->usExemptionActionType )
	{
	case DOT11_EXEMPT_NO_EXEMPTION :
		ExemptionActionType = 1;
		break;
	case DOT11_EXEMPT_ALWAYS :
		ExemptionActionType = 2;
		break;
	case DOT11_EXEMPT_ON_KEY_MAPPING_KEY_UNAVAILABLE :
		ExemptionActionType = 3;
		break;
	default :
		ExemptionActionType = 0;
		break;
	}

	//
	// Parse WMM info
	//
	if(pStaQos->CurrentQosMode != QOS_DISABLE)
	{
		NBL8021qInfo.Value = NET_BUFFER_LIST_INFO(pNetBufferList, Ieee8021QNetBufferListInfo);
		Priority = (u1Byte)NBL8021qInfo.WLanTagHeader.WMMInfo;
		if (Priority > 7)
		{
			RT_TRACE(COMP_SEND, DBG_WARNING, ("N6SdioSendSingleNetBufferList(): WMMInfo should less or equal than 7 (%d)\n", Priority));
			Priority = 0;
		}
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], start to process each Net Buffer\n"));
	//
	// Send each NB remaining.
	//
	for(pCurrNetBuffer = RT_NBL_NEXT_SEND(pNetBufferList);
		pCurrNetBuffer != NULL;
		pCurrNetBuffer = pNextNetBuffer)
	{
		pNextNetBuffer = NET_BUFFER_NEXT_NB(pCurrNetBuffer);
		
		if(!pNdisCommon->RegNblRacingWA)
		{
			if(pNextNetBuffer == NULL)
			{
				pDevice->SendingNetBufferList = NULL;
			}
		}

		//
		// Out of Tx resource.
		//
		if( RTIsListEmpty(GET_TCB_IDLE_QUEUE(Adapter)))
		{
			//if( !bFromQueue )
			//{ // Add the NBL to wait queue and remaining NBs insided will be processed later.
			//	RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioSendSingleNetBufferList(): +++N6CAddNblWaitQueue(): out of TCB pCurrNetBufferList(%p), pCurrNetBuffer(%p)\n", pNetBufferList, pCurrNetBuffer));
			//	PlatformAcquireSpinLock(Adapter, RT_BUFFER_SPINLOCK);
			//	N6CAddNblWaitQueue(
			//		pNdisCommon,
			//		pNetBufferList, 
			//		TRUE); // bToHead.
			//	PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
			//}

			//
			// Case 1: This NBL is in wait queue and will be processed later.
			// Leave this function.
			//
			pDevice->SendingNetBufferList = NULL;
			RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioSendSingleNetBufferList(), tcb idle queue Empty!!!\n"));
			return FALSE;
		}

		//
		// We have enough Tx resource to send this NB.
		//
		else
		{
			pTcb = (PRT_TCB)RTRemoveHeadListWithCnt(GET_TCB_IDLE_QUEUE(Adapter),Get_NUM_IDLE_TCB(Adapter));
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], get pTcb = 0x%x!!!\n", (u4Byte)pTcb));
			pTcb->sysTime[0] = PlatformGetCurrentTime();

			//
			// Set up pTcb->Reserved.
			//
			RT_ASSERT((pTcb->Reserved == NULL), ("N6SdioSendSingleNetBufferList(): pTcb->Reserved should be NULL!!!\n"));

			pTcb->Reserved = pNetBufferList;

			//
			// Add per-NBL info into TCB.
			//
			pTcb->priority = Priority;  // Qos information. 2007.01.05, by shien chang.	
			pTcb->EncInfo.ExemptionActionType = ExemptionActionType; // by CCW. 

			GetDefaultAdapter(Adapter)->MgntInfo.OutstandingNdisPackets++;
			//
			// Case 2.1: Fill up virtual address of buffers into TCB.
			//

// 20100309 Joseph: Copy packet from NET_BUFFER_LIST to local buffer and complete packet immediately.
// This fix the Iperf throughput test issue. It takes too long time to complete packets to OS.
// This long delay makes OS send less and less packet so troughput cannot be higher.

			//
			// 2011/09/14 MH According to iperf test in WIN7/XP, we use Ralink2870/AR95 to verify
			// iperf TCP/UDP test result. We found that RA2870 can send UDP in higher TP but AR95 fail
			// There are two ways to improve UDP throughput. A). increase CPU utilization by trigger more
			// and more system resource. B). Complete packet immediately. For TCP case, it seems almost
			// the same when completing packet in platfrom or HAL layer. RA2870 seems complete packet
			// in common layer. Because the throughptu will not increase in TX side.
			//
#if 1
			if(N6SdioGetPacketBuffers(Adapter, pCurrNetBuffer, pTcb) &&
				!Adapter->bInHctTest && !pMgntInfo->RegTxMode)
			{ 
				//
				// Set up TCB.
				//
				pTcb->ProtocolType = RT_PROTOCOL_802_11;
				pTcb->BufferType = RT_TCB_BUFFER_TYPE_SYSTEM;
				if ( IsDataFrame(pTcb->BufferList[0].VirtualAddress) )
				{
					pTcb->SpecifiedQueueID = UNSPECIFIED_QUEUE_ID;	//LOW_QUEUE;	// Rewrited by Annie, 2005-12-05.
				}
				else
				{
					pTcb->SpecifiedQueueID = NORMAL_QUEUE;
				}
				pTcb->DataRate = UNSPECIFIED_DATA_RATE;
				pTcb->bFromUpperLayer = TRUE;
				pTcb->FragCount = 1;
				pTcb->TSID = DEFAULT_TSID;


				//
				// Send TCB down.
				//
				if(ACTING_AS_AP(Adapter) == FALSE)
				{
					BOOLEAN bCompleteAfterSend;

					//RT_ASSERT(pTcb->BufferType == RT_TCB_BUFFER_TYPE_SYSTEM, ("pTcb->BufferType(%#d) shoulde be RT_TCB_BUFFER_TYPE_SYSTEM\n"));
					//pTcb->Reserved = NULL; // Prevent the NB complete before NicIFSendPacket() returns.

					bCompleteAfterSend = NicIFSendPacket(Adapter, pTcb);

					if (bCompleteAfterSend)
					{
						//
						// Complete the NB ASAP to enhance throughput for some application, such as iPerf.
						// Note that, we can complete the NB here because it 
						// had been coalesced in PreTransmitTCB().
						//
						RT_NBL_DECREASE_REF_CNT(pNetBufferList);
						pTcb->Reserved = NULL;
					}

				}
				else
				{
					//
					// 07525, rcnjko: 
					// AP_PS_SendPacket() will queue the packet to send in proper queue if necessary.
					// So, we must gurantee the buffers referenced in the TCB is still valid, 
					// that is, not yet compelted to upper layer.
					//
					NicIFSendPacket(Adapter, pTcb);
				}
			}
			else
#endif
			{
				//
				// Case 2.2 Too many buffers in the NB, send it in coalesced way.
				//
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], call N6SdioSendCoalescedNetBuffer()!!!\n"));
				N6SdioSendCoalescedNetBuffer( Adapter, pCurrNetBuffer, pTcb);
			}

			//
			// Update RT_NBL_NEXT_SEND()
			//
			// Prefast warning ignore
#pragma warning( disable:28182 )
			RT_NBL_NEXT_SEND(pNetBufferList) = pNextNetBuffer;
		}
	}

	//
	// Case 2: we are here if all NBs in the NBL had been processed.
	//
	
	if(bFromQueue)
	{
		PNET_BUFFER_LIST pTmpNBL;

		//
		// Remove the NBL from wait queue.
		//
		RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioSendSingleNetBufferList(): bFromQueue ---N6CRemvoeNblWaitQueue(): pCurrNetBufferList(%p)\n", pNetBufferList ));
		PlatformAcquireSpinLock(Adapter, RT_BUFFER_SPINLOCK);
		pTmpNBL = N6CRemoveNblWaitQueue(&pNdisCommon->TxNBLWaitQueue, TRUE);
		PlatformReleaseSpinLock(Adapter, RT_BUFFER_SPINLOCK);
		RT_ASSERT(pTmpNBL == pNetBufferList, 
			("N6SdioSendSingleNetBufferList(): pTmpNBL(%p) != pNetBufferList(%p)!!!\n", 
			pTmpNBL, pNetBufferList));
	}

	if(RT_NBL_GET_REF_CNT(pNetBufferList) > 0)
	{
		//
		// Count NBL reference counet for that added in N6SdioSendNetBufferLists().
		//
		RT_NBL_DECREASE_REF_CNT(pNetBufferList);

		//
		// Compelete the NBL if required.
		//
		RT_ASSERT( NET_BUFFER_LIST_NEXT_NBL(pNetBufferList) == NULL,
			("N6SdioSendSingleNetBufferList(): pNBL(%p) is not a single one!!!\n", pNetBufferList));	

		{
			PNET_BUFFER_LIST	pCurrNetBufferList, pNextNetBufferList;
			if(RT_NBL_GET_REF_CNT(pNetBufferList) == 0)
			{			
				GetDefaultAdapter(Adapter)->MgntInfo.OutstandingNdisPackets--;
				GetDefaultAdapter(Adapter)->MgntInfo.CompleteFlag = 0;
			
				PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			                // Complete each NB as success.
					for(pCurrNetBufferList = pNetBufferList;
							pCurrNetBufferList != NULL;
							pCurrNetBufferList = pNextNetBufferList)
						{
							pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
							NET_BUFFER_LIST_STATUS(pCurrNetBufferList) = NDIS_STATUS_SUCCESS;
						}

				NdisMSendNetBufferListsComplete(
						pDevice->hNdisAdapter,
						pNetBufferList,
							((NDIS_CURRENT_IRQL()==DISPATCH_LEVEL) ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0));

				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
			}
		}
	}
	else
	{
		RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioSendSingleNetBufferList(): check RT_NBL_GET_REF_CNT = 0\n"));
	}

	//
	// We are now ready to process another NBL.
	//
	pDevice->SendingNetBufferList = NULL;

	//
	// Signal thread waiting in N6SdioReturnAllPendingTxPackets().
	//
	if(pDevice->bWaitingSendingNBL)
	{
		pDevice->bWaitingSendingNBL = FALSE;
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
		NdisSetEvent(&(pDevice->evtSendingNBLCompleted));
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	}

	return TRUE;
}


BOOLEAN
N6SdioGetPacketBuffers(
	IN	PADAPTER		Adapter,
	IN	PNET_BUFFER		pNetBuffer,
	OUT	PRT_TCB			pTcb
	)
{
	ULONG			i,index;   
	UINT			MdlCount;
	PMDL			pCurrMdl, pMdlToMap;
	PMDL 			pNextMdl = NULL;
	UINT			PacketLength,RemainMdlLength;
	PUCHAR			ptr = NULL;
	UINT			BufLen;
	UINT			FirstMdlLength = 0;
	UINT			CopyLength;
	PUCHAR			CopyPtr = NULL;
	UINT			MaxAvailableBufferCount;

	//
	// Calculate buffer count.
	//
	MdlCount = 0;
	for (pCurrMdl = NET_BUFFER_CURRENT_MDL(pNetBuffer);
		pCurrMdl != NULL;
		pCurrMdl = NDIS_MDL_LINKAGE(pCurrMdl))
	{
		MdlCount ++;
	}

	// Prefast warning C28182: Dereferencing NULL pointer.
	if (NET_BUFFER_CURRENT_MDL(pNetBuffer) != NULL)
	{
		FirstMdlLength = MmGetMdlByteCount(NET_BUFFER_CURRENT_MDL(pNetBuffer)) - NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer);
	}

	if (FirstMdlLength == sMacHdrLng)
	{
		MaxAvailableBufferCount = MAX_PER_PACKET_PHYSICAL_BUF_NUM;
	}
	else
	{
		MaxAvailableBufferCount = MAX_PER_PACKET_PHYSICAL_BUF_NUM-1;
	}
	
	if (MdlCount > MaxAvailableBufferCount)
	{
		return FALSE;
	}

	PacketLength = NET_BUFFER_DATA_LENGTH(pNetBuffer);
	
	pTcb->PacketLength=PacketLength;

	//2 Assign header buffer	
	// Prefast warning C30030: Warning: Allocating executable memory via specifying a MM_PAGE_PRIORITY type without a bitwise OR with MdlMappingNoExecute
	// False positive and safe here, disable the warning
#pragma warning( disable:30030 )
	// Prefast warning C28182: Dereferencing NULL pointer. '((pNetBuffer))->CurrentMdl'
	if (NET_BUFFER_CURRENT_MDL(pNetBuffer) != NULL)
	{
		ptr = (PUCHAR)MmGetSystemAddressForMdlSafe(NET_BUFFER_CURRENT_MDL(pNetBuffer), NormalPagePriority)
			+ NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer);
		BufLen = MmGetMdlByteCount(NET_BUFFER_CURRENT_MDL(pNetBuffer))
			- NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer);
	}

	if (FirstMdlLength > sMacHdrLng)
	{
		PlatformMoveMemory(pTcb->Header[0].VirtualAddress, ptr, sMacHdrLng);
		pTcb->Header[0].Length = sMacHdrLng;
		pTcb->BufferList[0] = pTcb->Header[0];
		pTcb->BufferList[1].VirtualAddress = ptr + sMacHdrLng;
		pTcb->BufferList[1].Length = FirstMdlLength - sMacHdrLng;
		pTcb->BufferCount = 2;
		index=2;
		pMdlToMap = NDIS_MDL_LINKAGE( NET_BUFFER_CURRENT_MDL(pNetBuffer) );
	}
	else if (FirstMdlLength < sMacHdrLng)
	{
		PlatformMoveMemory(pTcb->Header[0].VirtualAddress, ptr, FirstMdlLength);
		CopyLength = FirstMdlLength;

		// Prefast warning C28182: Dereferencing NULL pointer. '((pNetBuffer))->CurrentMdl'
		if (NET_BUFFER_CURRENT_MDL(pNetBuffer) != NULL)
		{
			for (pCurrMdl = NDIS_MDL_LINKAGE(NET_BUFFER_CURRENT_MDL(pNetBuffer));
			pCurrMdl != NULL;
				pCurrMdl = pNextMdl)
			{
				pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
				// Prefast warning C30030: Warning: Allocating executable memory via specifying a MM_PAGE_PRIORITY type without a bitwise OR with MdlMappingNoExecute
				// False positive and safe here, disable the warning
#pragma warning( disable:30030 )
				ptr = (PUCHAR)MmGetSystemAddressForMdlSafe(pCurrMdl, NormalPagePriority);
				BufLen = MmGetMdlByteCount(pCurrMdl);

				if ((CopyLength + BufLen) >= sMacHdrLng)
				{
					PlatformMoveMemory(pTcb->Header[0].VirtualAddress + CopyLength, ptr, sMacHdrLng - CopyLength);
					CopyPtr = ptr + (sMacHdrLng - CopyLength);
					CopyLength += (sMacHdrLng - CopyLength);
					break;
				}

				PlatformMoveMemory(pTcb->Header[0].VirtualAddress + CopyLength, ptr, BufLen);
				CopyLength += BufLen;
			}
		}

		pTcb->Header[0].Length = CopyLength;
		pTcb->BufferList[0] = pTcb->Header[0];
		if (CopyLength < sMacHdrLng || (CopyPtr-ptr) == BufLen)
		{
			pTcb->BufferCount = 1;
			index=1;
		}
		else
		{
			pTcb->BufferList[1].VirtualAddress = CopyPtr;
			pTcb->BufferList[1].Length = (u4Byte)(BufLen - (CopyPtr - ptr));
			pTcb->BufferCount = 2;
			index=2;
		}
		pMdlToMap = pNextMdl;
	}
	else
	{
		PlatformMoveMemory(pTcb->Header[0].VirtualAddress, ptr, BufLen);
		pTcb->Header[0].Length = BufLen;
		pTcb->BufferList[0] = pTcb->Header[0];
		pTcb->BufferCount = 1;
		index=1;
		pMdlToMap = NDIS_MDL_LINKAGE( NET_BUFFER_CURRENT_MDL(pNetBuffer) );
	}
	
	RemainMdlLength=PacketLength;
	for(i=0; i < index; i++)
	{
		RemainMdlLength -= pTcb->BufferList[i].Length;
	}
	
	//2 Get Virtual buffer list
	for (pCurrMdl = pMdlToMap;
		pCurrMdl != NULL;
		pCurrMdl = pNextMdl)
	{
		pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
		
		// Prefast warning C30030: Warning: Allocating executable memory via specifying a MM_PAGE_PRIORITY type without a bitwise OR with MdlMappingNoExecute
		// False positive and safe here, disable the warning
#pragma warning( disable:30030 )
		ptr = (pCurrMdl == NET_BUFFER_CURRENT_MDL(pNetBuffer)) ?
				(PUCHAR)MmGetSystemAddressForMdlSafe(pCurrMdl, NormalPagePriority) + NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer) :
				MmGetSystemAddressForMdlSafe(pCurrMdl, NormalPagePriority);
		BufLen = (pCurrMdl == NET_BUFFER_CURRENT_MDL(pNetBuffer)) ?
				MmGetMdlByteCount(pCurrMdl)-NET_BUFFER_CURRENT_MDL_OFFSET(pNetBuffer) :
				MmGetMdlByteCount(pCurrMdl);
				
		pTcb->BufferList[index].VirtualAddress = ptr;
		if(RemainMdlLength < BufLen )
		{
			pTcb->BufferList[index].Length = RemainMdlLength;
			RemainMdlLength=0;
		}
		else // >=
		{
		pTcb->BufferList[index].Length = BufLen;
			RemainMdlLength-=BufLen;
		}
		pTcb->BufferCount++;
		index++;
		
		if( RemainMdlLength <= 0 )
			break;
		
	}

	RT_ASSERT(pTcb->BufferCount<=(MAX_PER_PACKET_PHYSICAL_BUF_NUM+2),("Too many physical memory fragment !!\n"));

	return TRUE;
}


//
//	Description:
//		Copy buffer in the NBL into a local buffer and send it down.
//
//	Assumption:
//		1. RT_TX_SPINLOCK is acquired.
//
VOID
N6SdioSendCoalescedNetBuffer(
	IN PADAPTER			Adapter,
	IN PNET_BUFFER		pNetBuffer,
	OUT PRT_TCB			pTcb
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(Adapter);
	PRT_TX_LOCAL_BUFFER	pBuf;

	pBuf = GetLocalBuffer(Adapter);
	if(pBuf == NULL)
	{
		//
		// Recycle resource and drop the packet.
		//
		pTcb->ProtocolType = RT_PROTOCOL_802_11;
		pTcb->BufferType = RT_TCB_BUFFER_TYPE_SYSTEM; // For DrvIFCompletePacket().
		pTcb->bFromUpperLayer = TRUE;
		pTcb->TSID = DEFAULT_TSID;
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);

		RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioSendCoalescedNetBuffer(): No local buffer!!\n"));
	}
	else
	{
		u2Byte PktLength = 0;

		//
		// Copy buffers of this NB into local buffer.
		//
		N6CopyPacket(pNetBuffer, pBuf, &PktLength);

		//
		// Complete the NB and related resource since we 
		// had copy everything need into TCB.
		//
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
		DrvIFCompletePacket(Adapter, pTcb, NDIS_STATUS_SUCCESS);
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

		//
		// Set up TCB.
		//

		pTcb->Header[0].Length = (PktLength >= sMacHdrLng) ? sMacHdrLng : PktLength;
		PlatformMoveMemory(
				pTcb->Header[0].VirtualAddress, 
				pBuf->Buffer.VirtualAddress, 
				pTcb->Header[0].Length);
		pTcb->BufferList[0] = pTcb->Header[0];
		pTcb->BufferCount = 1;

		if(PktLength > sMacHdrLng)
		{
			pTcb->BufferList[1].VirtualAddress = pBuf->Buffer.VirtualAddress + sMacHdrLng;
			pTcb->BufferList[1].Length = PktLength-sMacHdrLng;
			pTcb->BufferCount = 2;
		}

		pTcb->PacketLength = PktLength;
		pTcb->Tailer.Length = 0;
		pTcb->ProtocolType = RT_PROTOCOL_802_11;
		pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
		if ( IsDataFrame(pTcb->BufferList[0].VirtualAddress) )
		{
			pTcb->SpecifiedQueueID = UNSPECIFIED_QUEUE_ID;
		}
		else
		{
			pTcb->SpecifiedQueueID = NORMAL_QUEUE;
		}
		pTcb->DataRate = UNSPECIFIED_DATA_RATE;
		pTcb->bFromUpperLayer = TRUE;
		pTcb->Reserved = pBuf;
		pTcb->FragCount = 1;
		pTcb->TSID = DEFAULT_TSID;

		NicIFSendPacket(Adapter, pTcb);
	}
}



//
//	Description:
//		Return all pending tx packets to upper layer.
//
//	Assumption:
//		1. RT_TX_SPINLOCK is NOT acquired.
//		2. N6SDIO_CANNOT_TX() MUST be TRUE before calling this function.
//
VOID
N6SdioReturnAllPendingTxPackets(
	IN 	PADAPTER 		pAdapter
	)
{
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PRT_SDIO_DEVICE			pDevice = GET_RT_SDIO_DEVICE(pAdapter);
	u1Byte					QueueID;
	PRT_TCB					pTcb = NULL;
	PNET_BUFFER_LIST		pNetBufferList;
	KIRQL					OldIrql = NDIS_CURRENT_IRQL();
	PNET_BUFFER_LIST		pCurrNetBufferList, pNextNetBufferList;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(N6SDIO_INC_TX_RESET_COUNT(pDevice) == 1) // At most one thread is working here. 
	{
		RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioReturnAllPendingTxPackets(): 1st thread enter here....\n"));

		//
		// Pending TCBs.
		//
		for( QueueID = LOW_QUEUE; QueueID < MAX_TX_QUEUE; QueueID++ )
		{
			while(	!RTIsListEmpty(&pAdapter->TcbBusyQueue[QueueID])	
					|| !RTIsListEmpty(&pAdapter->TcbWaitQueue[QueueID])	
					|| !RTIsListEmpty(&pAdapter->TcbAggrQueue[QueueID])	
					)
			{
				if(!RTIsListEmpty(&pAdapter->TcbBusyQueue[QueueID]))
				{
					pTcb = (PRT_TCB)RTRemoveHeadList(&pAdapter->TcbBusyQueue[QueueID]);
				}
				else if(!RTIsListEmpty(&pAdapter->TcbWaitQueue[QueueID]))
				{
					pTcb = (PRT_TCB)RTRemoveHeadList(&pAdapter->TcbWaitQueue[QueueID]);
				}
				else if (!RTIsListEmpty(&pAdapter->TcbAggrQueue[QueueID]))
				{
					pTcb=(PRT_TCB)RTRemoveHeadList(&pAdapter->TcbAggrQueue[QueueID]);
					pAdapter->TcbCountInAggrQueue[QueueID]--;
				}
				ReturnTCB(pAdapter, pTcb, RT_STATUS_SUCCESS);
			}
		}

		//
		// AP mode power-save packets.
		//
		AP_PS_ReturnAllQueuedPackets(pAdapter, FALSE);

		// Qos buffered packet.
		//QosReturnAllPendingTxMsdu(pAdapter);

		if(OldIrql == PASSIVE_LEVEL)
		{
			if(pNdisCommon->RegNblRacingWA)
			{
				PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
				N6CReleaseDataFrameQueuedWorkItemCallback(pAdapter);
				PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
			}
			else
			{
				//
				// Wait SendingNetBufferList completed.
				//
				if( pDevice->SendingNetBufferList != NULL )
				{
					pDevice->bWaitingSendingNBL = TRUE;
					PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
		
					RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioReturnAllPendingTxPackets(): wait SendingNetBufferList...\n"));
					// Preafast warning C28121 : The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
					// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
					// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
					NdisWaitEvent(&(pDevice->evtSendingNBLCompleted), 0);
					NdisResetEvent(&(pDevice->evtSendingNBLCompleted));
		
					PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
				}
				RT_ASSERT(pDevice->SendingNetBufferList == NULL, ("SendingNetBufferList(%p) should be NULL!!!\n", pDevice->SendingNetBufferList));

				//
				// Remove pending NBLs in wait queue.
				// Note that, we must make sure: 
				// 1. SendingNetBufferList is NULL.
				// 2. No one can call N6SdioSendSingleNetBufferList() now.
				// 3. All TCB and related resource are completed.
				//
				PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
				PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
			while( !N6CIsNblWaitQueueEmpty(pNdisCommon->TxNBLWaitQueue) )
				{
				pNetBufferList = N6CRemoveNblWaitQueue(&pNdisCommon->TxNBLWaitQueue, TRUE);

					for (pCurrNetBufferList = pNetBufferList;
						pCurrNetBufferList != NULL;
						pCurrNetBufferList = pNextNetBufferList)
					{
						pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
						NET_BUFFER_LIST_STATUS(pCurrNetBufferList) = NDIS_STATUS_SUCCESS;
					}
					PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);

					NdisMSendNetBufferListsComplete(
						pDevice->hNdisAdapter, 
						pNetBufferList, 
						((OldIrql==DISPATCH_LEVEL) ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0));

					PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
				}
				PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
				PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
			}
		}
		else
		{
			RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioReturnAllPendingTxPackets(): IRQL(%d) != PASSIVE_LEVEL\n", OldIrql));
		}
	}

	N6SDIO_DEC_TX_RESET_COUNT(pDevice);

	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);	

	{
		if(pAdapter == GetDefaultAdapter(pAdapter))
		{
			//
			// Recursively to free pending packet for the ext adapter.
			//

			{
				PADAPTER pTargetAdapter = GetFirstExtAdapter(pAdapter);

				while(pTargetAdapter != NULL)
				{
					N6SdioReturnAllPendingTxPackets(pTargetAdapter);
					pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
				}
				
			}
		}
	}
}


VOID
N6SdioCancelSend(
	IN    NDIS_HANDLE              MiniportAdapterContext,
	IN    PVOID                    CancelId
	)
 /**
 * This function is called by NDIS to cancel any NBL that have
 * not yet been sent and which correspond to CancelId passed in.
 * 
 * \param MiniportAdapterContext    The context for this miniport
 * \param CancelId                  Packets with this Id will be cancelled
 * \sa MPSendPackets
 */
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);

	RT_TRACE(COMP_DBG, DBG_LOUD, (" ==> N6SdioCancelSend()\n"));
	

	RT_TRACE(COMP_DBG, DBG_LOUD, (" <== N6SdioCancelSend()\n"));
	return;
}


VOID 
N6SdioReturnNetBufferLists(
	IN  NDIS_HANDLE         MiniportAdapterContext,
	IN  PNET_BUFFER_LIST    NetBufferLists,
	IN  ULONG               ReturnFlags
	)
/**
 * This function is called by NDIS when the protocol above returns NetBufferLists
 * previously indicated by this miniport.
 * 
 * \param MiniportAdapterContext    The adapter context for this miniport
 * \param NetBufferLists            The NBLs that was previously indicated to NDIS
 * \param ReturnFlags               Flags for return information (dispatch level, etc)
 * \sa Hw11ReturnFragment
 */
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	PRT_RFD			pRfd;
	PMDL			pMdl, pNextMdl;
	PNET_BUFFER_LIST	pCurrNetBufferList, pNextNetBufferList;
	PDOT11_EXTSTA_RECV_CONTEXT	pRecvContext;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte			Count = 0;

	for (pCurrNetBufferList = NetBufferLists;
		pCurrNetBufferList != NULL;
		pCurrNetBufferList = pNextNetBufferList)
	{
		pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);

		pRfd = MP_GET_PACKET_RFD(pCurrNetBufferList);
		
		//
		// Free MDL.
		//
		pMdl = NET_BUFFER_FIRST_MDL( NET_BUFFER_LIST_FIRST_NB(pCurrNetBufferList) );
		while (pMdl)
		{
			pNextMdl = NDIS_MDL_LINKAGE(pMdl);
			NdisFreeMdl(pMdl);
			pMdl = pNextMdl;
		}

		//
		// Free OOB.
		//
		pRecvContext = (PDOT11_EXTSTA_RECV_CONTEXT)NET_BUFFER_LIST_INFO(pCurrNetBufferList, MediaSpecificInformation);
		PlatformFreeMemory(pRecvContext, sizeof(DOT11_EXTSTA_RECV_CONTEXT));
		
		NdisFreeNetBufferList(pCurrNetBufferList);	

		PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_DEC_RCV_REF(GetDefaultAdapter(Adapter));
		Count = RT_GET_RCV_REF(GetDefaultAdapter(Adapter));
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);

		if(pRfd)
		{
			ReturnRFDList(Adapter, pRfd);
			//NicIFReturnPacket(Adapter, pRfd);
		}
	}

	//
	// Signal the AllPacketReturnedEvent if all packet are returned. 
	// 2007.01.19, by shien chang.
	//
	if (Count == 0)
	{
		NdisSetEvent(&pNdisCommon->AllPacketReturnedEvent);

		if(Adapter->MgntInfo.NdisVersion >= RT_NDIS_VERSION_6_20)		
		{
			PADAPTER pTempAdapter = GetFirstExtAdapter(Adapter);
			while(pTempAdapter != NULL)
			{
				NdisSetEvent(&pTempAdapter->pNdisCommon->AllPacketReturnedEvent);
				pTempAdapter = GetNextExtAdapter(pTempAdapter);				

			}

		}

	}
}


BOOLEAN
N6SdioCheckForHang(
	IN NDIS_HANDLE MiniportAdapterContext
	)
/*++

Routine Description:

	The MiniportCheckForHang handler is called to report the state of the
	NIC, or to monitor the responsiveness of an underlying device driver.
	This is an optional function. If this handler is not specified, NDIS
	judges the driver unresponsive when the driver holds
	MiniportQueryInformation or MiniportSetInformation requests for a
	time-out interval (deafult 4 sec), and then calls the driver's
	MiniportReset function. A NIC driver's MiniportInitialize function can
	extend NDIS's time-out interval by calling NdisMSetAttributesEx to
	avoid unnecessary resets.

	Always runs at IRQL = DISPATCH_LEVEL.

Arguments:

	MiniportAdapterContext  Pointer to our adapter

Return Value:

	TRUE    NDIS calls the driver's MiniportReset function.
	FALSE   Everything is fine

Note:
	CheckForHang handler is called in the context of a timer DPC.
	take advantage of this fact when acquiring/releasing spinlocks

--*/
{
	PADAPTER Adapter = (PADAPTER) MiniportAdapterContext;

	FunctionIn(COMP_INIT);

	if(Adapter->MgntInfo.WatchDogTimer.Status & RT_TIMER_STATUS_INITIALIZED)
	{
		if(!(Adapter->MgntInfo.WatchDogTimer.Status & RT_TIMER_STATUS_PERIODIC))
			PlatformSetPeriodicTimer(Adapter, &Adapter->MgntInfo.WatchDogTimer , RT_CHECK_FOR_HANG_PERIOD * 1000);

		return Adapter->MgntInfo.WatchDogReturnFlag;
	}
	FunctionOut(COMP_INIT);
	return FALSE;
}

VOID
WatchDogTimerCallback(
	IN	PRT_TIMER		pTimer
)
{
	#define MAX_NDIS_STATUS_REPORT_TIME	5	// In 2 sec

	PADAPTER			Adapter = pTimer->Adapter;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PRT_SDIO_DEVICE		device = GET_RT_SDIO_DEVICE(Adapter);
	u2Byte				T2byte;
	u1Byte				QueueID;
	u1Byte				IdxPipe;
	BOOLEAN			bHasPendingNBL = FALSE;

	RT_TRACE(COMP_INIT, DBG_LOUD, (" N6SdioCheckForHang() ==>\n"));

	// To prevent from error state which configured in watchdog when power off state. 2010.05.07. Added by tynli.
	if(device->CurrentPowerState != NdisDeviceStateD0)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioCheckForHang(): Do nothing because state in (%x).-----\n", device->CurrentPowerState));
		pDefaultAdapter->MgntInfo.WatchDogReturnFlag = FALSE;
		return;
	}

	if(!Adapter->bHWInitReady)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioCheckForHang(): bHWInitReady FALSE\n"));
	
		pDefaultAdapter->MgntInfo.WatchDogReturnFlag = FALSE;

		if(Adapter->MgntInfo.bSetWatchDogTimerByDriver)
		{
			PlatformSetCheckForHangTimer(Adapter);
		}

		return;
	}

	//
	// <Roger_Notes> Signal corresponding event when checkforhang routine is called after PnP action. 
	// 2009.09.30.
	// 
	if(!device->bChkForHangAfterPnP)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioCheckForHang(): Signal SetPnpChkForHangEvent!!\n"));
		NdisSetEvent(&device->SetPnpChkForHangEvent);
		device->bChkForHangAfterPnP = TRUE;
	}


	if(Adapter->bSurpriseRemoved)
	{
		N6SdioReturnAllPendingTxPackets(Adapter);

		// Release Tx Queue buffered context if needed. added by Roger, 2012.04.17.
		N6SdioReleaseTxQueuePending(Adapter);	
	}
	

	if(Adapter->NdisSdioDev.NdisStatusIndicateCount < MAX_NDIS_STATUS_REPORT_TIME)
	{
		Adapter->NdisSdioDev.NdisStatusIndicateCount++;
		if(	Adapter->NdisSdioDev.NdisStatusIndicateCount==MAX_NDIS_STATUS_REPORT_TIME	&&
			!Adapter->NdisSdioDev.bNdisStatusIndicated
			)
		{
			NicIFSetLinkStatus( Adapter, RT_MEDIA_DISCONNECT );
			Adapter->NdisSdioDev.NdisStatusIndicateCount = 0;			
		}
	}

#if USE_WORKITEM
	PlatformScheduleWorkItem(&(pHalData->RtCheckForHangWorkItem));
#endif //USE_WORKITEM

	Adapter->HalFunc.DumpHardwareProfileHandler(Adapter);

	// Annie Test ONLY. 2005-11-09.
	RT_TRACE( COMP_QOS, DBG_LOUD, ("N6SdioCheckForHang(): pMgntInfo->pStaQos->CurrentQosMode=%d\n", pMgntInfo->pStaQos->CurrentQosMode) );

	if(Adapter->MgntInfo.bSetWatchDogTimerByDriver)
	{
		PlatformSetCheckForHangTimer(Adapter);
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, (" <== N6SdioCheckForHang()\n"));

	pDefaultAdapter->MgntInfo.WatchDogReturnFlag = FALSE;
	return;
}


NDIS_STATUS
N6SdioReset(
	IN  NDIS_HANDLE     MiniportAdapterContext,
	OUT PBOOLEAN        AddressingReset
	)
/*++

Routine Description:

    An NDIS 6.0 miniport driver's reset function can cancel pending OID
    requests and send requests in the context of a reset.
    NDIS no longer performs such cancellations. Alternatively,
    the miniport driver can complete pending OID requests
    and send requests after the reset completes.

    MiniportReset function can be called at either IRQL PASSIVE_LEVEL
    or IRQL = DISPATCH_LEVEL.

Arguments:

    AddressingReset         To let NDIS know whether we need help from it with our reset
    MiniportAdapterContext  Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_RESET_IN_PROGRESS
    NDIS_STATUS_HARD_ERRORS

Note:

--*/
{
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;

	RT_TRACE(COMP_INIT, DBG_WARNING, (" ==> N6SdioReset()\n"));

	// A temporary WorkAround for WDI, so just do nothing.
	if(OS_SUPPORT_WDI(Adapter))
		return NDIS_STATUS_SUCCESS;

	if(!Adapter->bHWInitReady)
		return NDIS_STATUS_SUCCESS;
	

	//
	// <Roger_Notes> We complete any pending OID requests. 
	// NDIS will submit no further OID requests to the miniport driver for the NIC 
	// being reset until the reset operation has finished.
	// In this case, we cancel all pending SetPower OIDs to prevent unexpect blocking IRPs
	// due to MiniportQueryInformation, MiniportSetInformation function seem to have timed out. 
	// 2008.03.13.
	//
	// Complete the pended OID and return aborted caused by reset request. By Bruce, 2008-10-29.
	
	N6CompletePendedOID(
		Adapter, 
		RT_PENDED_OID_DONT_CARE,
		NDIS_STATUS_REQUEST_ABORTED);

	RT_TRACE(COMP_INIT, DBG_WARNING, (" <== N6SdioReset()\n"));

	return ndisStatus;
}



//
// NDIS5 Legacy -------------------------------------------------------------------
//


NDIS_STATUS
N6SdioAllocateNative80211MIBs(
	IN	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS		rtStatus;
	ULONG			AllocSize;

	do {
		// pDot11SupportedPhyTypes.
		AllocSize = sizeof(DOT11_SUPPORTED_PHY_TYPES) + (NATIVE_802_11_MAX_NUM_PHY_TYPES-1) * sizeof(DOT11_PHY_TYPE);
		rtStatus = PlatformAllocateMemory(
			Adapter,
			&(pNdisCommon->pDot11SupportedPhyTypes),
			AllocSize
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioAllocateNative80211MIBs(): failed to allocate pDot11SupportedPhyTypes\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		PlatformZeroMemory(pNdisCommon->pDot11SupportedPhyTypes, AllocSize);

		// pDot11PhyMIBs.
		AllocSize = sizeof(NIC_PHY_MIB) * NATIVE_802_11_MAX_NUM_PHY_TYPES;
		rtStatus = PlatformAllocateMemory(
			Adapter,
			&(pNdisCommon->pDot11PhyMIBs),
			AllocSize
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioAllocateNative80211MIBs(): failed to allocate pDot11PhyMIBs\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}

	}while (FALSE);

	//
	// If allocation failed, reclaim memory resources.
	//
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		if (pNdisCommon->pDot11SupportedPhyTypes)
		{
			PlatformFreeMemory(
				pNdisCommon->pDot11SupportedPhyTypes,
				sizeof(DOT11_SUPPORTED_PHY_TYPES) + (NATIVE_802_11_MAX_NUM_PHY_TYPES-1) * sizeof(DOT11_PHY_TYPE)
				);
			pNdisCommon->pDot11SupportedPhyTypes = NULL;
		}

		if (pNdisCommon->pDot11PhyMIBs)
		{
			PlatformFreeMemory(
				pNdisCommon->pDot11PhyMIBs,
				sizeof(NIC_PHY_MIB) * NATIVE_802_11_MAX_NUM_PHY_TYPES
				);
			pNdisCommon->pDot11PhyMIBs = NULL;
		}
	}
	
	return ndisStatus;
}

VOID
N6SdioFreeNative80211MIBs(
	IN	PADAPTER	Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	if (pNdisCommon->pDot11SupportedPhyTypes)
	{
		PlatformFreeMemory(
			pNdisCommon->pDot11SupportedPhyTypes,
			sizeof(DOT11_SUPPORTED_PHY_TYPES) + (NATIVE_802_11_MAX_NUM_PHY_TYPES-1) * sizeof(DOT11_PHY_TYPE)
			);
	}

	if (pNdisCommon->pDot11PhyMIBs)
	{
		PlatformFreeMemory(
			pNdisCommon->pDot11PhyMIBs,
			sizeof(NIC_PHY_MIB) * NATIVE_802_11_MAX_NUM_PHY_TYPES
			);
	}	
	
	if(pNdisCommon->dot11IbssParams.AdditionalIEData)
	{
		PlatformFreeMemory(pNdisCommon->dot11IbssParams.AdditionalIEData, pNdisCommon->dot11IbssParams.AdditionalIESize);
	}
}

VOID
MPFragPhysicalBuffer(
	PSHARED_MEMORY	pSharedMemory,
	u2Byte			BufferCount,
	ULONG			BufferIndex,
	UINT			VirtualLength
	)
{
	u2Byte	i;

	for(i=BufferCount-1;i>=(LONG)BufferIndex;i--)
	{
		pSharedMemory[i+1].PhysicalAddressHigh=pSharedMemory[i].PhysicalAddressHigh;
		pSharedMemory[i+1].PhysicalAddressLow=pSharedMemory[i].PhysicalAddressLow;
		pSharedMemory[i+1].Length=pSharedMemory[i].Length;
	}

	// Shorten this buffer
	pSharedMemory[BufferIndex].Length=VirtualLength;

	// Shift this buffer
	pSharedMemory[BufferIndex+1].Length -= VirtualLength;
	pSharedMemory[BufferIndex+1].PhysicalAddressLow += VirtualLength;
}


NDIS_STATUS 
N6SdioReadRegParameters(
	IN  PRT_SDIO_DEVICE		device
	)
/*++
Routine Description:

	Read the following from the registry
	1. All the parameters
	2. NetworkAddres

Arguments:

    device							Pointer to our USB device

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_RESOURCES                                       

--*/    
{
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	NDIS_HANDLE		ConfigurationHandle;
	PMP_REG_ENTRY	pRegEntry;
	PUCHAR			pointer;
	UINT			i;
	UINT			Length;
	PNDIS_CONFIGURATION_PARAMETER	ReturnedValue;
	ULONG_PTR			value = 0;

	RT_TRACE(COMP_DBG, DBG_TRACE, (" ==> N6SdioReadRegParameters()\n"));

	Status = N6ReadCommonRegistry((PADAPTER)(device->pAdapter));
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT,DBG_TRACE, ("<== N6SdioReadRegParameters(), failed in N6ReadCommonRegistry(). Status=0x%X !!!\n", Status));
		return Status;
	}

	// Open the registry for this adapter.
	Status = N6OpenConfigurationHandle((PADAPTER)(device->pAdapter), &ConfigurationHandle);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_TRACE, ("<== N6SdioReadRegParameters(), failed in N6OpenConfigurationHandle(), Status=0x%X !!!\n", Status));
		return Status;
	}

	// Read all the registry values.
	for(i = 0, pRegEntry = NICRegTable; i < NIC_NUM_REG_PARAMS; i++, pRegEntry++)
	{
		pointer = (PUCHAR) device + pRegEntry->FieldOffset;
//#if (WPP_SOFTWARE_TRACE == 0)
//		RT_TRACE(COMP_INIT,DBG_LOUD, ("%s",&pRegEntry->RegName));
//#endif
		// Get the configuration value for a specific parameter.  Under NT the
		// parameters are all read in as DWORDs.
		NdisReadConfiguration(
			&Status,
			&ReturnedValue,
			ConfigurationHandle,
			&pRegEntry->RegName,
			(enum _NDIS_PARAMETER_TYPE)pRegEntry->Type);

		// If the parameter was present, then check its value for validity.
		if (Status == NDIS_STATUS_SUCCESS)
		{
			if( (pRegEntry->Type == NdisParameterInteger)||
				(pRegEntry->Type == NdisParameterHexInteger) 	)
			{
				// Check that param value is not too small or too large
				if(ReturnedValue->ParameterData.IntegerData < pRegEntry->Min ||
					ReturnedValue->ParameterData.IntegerData > pRegEntry->Max)
				{
					value = pRegEntry->Default;
				}
				else
				{
					value = ReturnedValue->ParameterData.IntegerData;
				}
			}
			else if( pRegEntry->Type == NdisParameterString )
			{
				USHORT	copyLen;
				copyLen = ( (ReturnedValue->ParameterData.StringData.Length/2)>(USHORT)pRegEntry->Max) ? pRegEntry->Max : ReturnedValue->ParameterData.StringData.Length/2;

				CopyFromUnicodeToOS(
					pointer,
					&ReturnedValue->ParameterData.StringData,
					copyLen);
			}
		}
		else if(pRegEntry->bRequired)
		{
			RT_TRACE(COMP_DBG, DBG_SERIOUS,(" -- failed\n"));

			RT_ASSERT(FALSE,("Read parameter FAIL!"));

			Status = NDIS_STATUS_FAILURE;
			break;
		}
		else
		{
			if(	(pRegEntry->Type == NdisParameterInteger) ||
				(pRegEntry->Type == NdisParameterHexInteger) )
			{
				value = pRegEntry->Default;
			}
			else
			{
				CopyFromUnicodeToOS( 
					pointer,
					(NDIS_STRING *)(UINT_PTR)pRegEntry->Default,
					((NDIS_STRING *)(UINT_PTR)pRegEntry->Default)->Length/2 );
			}
			Status = NDIS_STATUS_SUCCESS;
		}

		// Store the value in the adapter structure.
		switch(pRegEntry->FieldSize)
		{
			case 1:
				*((PUCHAR) pointer) = (UCHAR) value;
				break;
			case 2:
				*((PUSHORT) pointer) = (USHORT) value;
				break;
			case 4:
				*((PULONG) pointer) = (ULONG) value;
				break;
			default:
				RT_TRACE(COMP_DBG, DBG_SERIOUS,("Bogus field size %d\n", pRegEntry->FieldSize));
				break;
		}
	}

	// Close the registry
	N6CloseConfigurationHandle(ConfigurationHandle);

	RT_TRACE(COMP_DBG, DBG_TRACE, ("<-- N6SdioReadRegParameters, Status=%x\n", Status));
	return Status;
}


VOID 
N6SdioFreeTxQueueRes( 
	IN PADAPTER		pAdapter
)
{
	PRT_SDIO_DEVICE	pDevice = GET_RT_SDIO_DEVICE(pAdapter);

	// Free Rt TxQueues
	if(pDevice->RtTxQueue) 
	{
      		ExFreePoolWithTag(pDevice->RtTxQueue, '3278' );
       	pDevice->RtTxQueue = NULL;
	}
}

VOID 
N6SdioDereferInterface( 
	IN PRT_SDIO_DEVICE		pDevice
)
{
    // Dereference SDIO Bus interface.
    if(pDevice && pDevice->Sdbusinterface.InterfaceDereference)
    {
        RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioDereferInterface+\n"));
        //
        // Call the bus driver to dereference the interface.
        //
        (pDevice->Sdbusinterface.InterfaceDereference)
            (pDevice->Sdbusinterface.Context);
        RtlZeroMemory(&pDevice->Sdbusinterface, sizeof(SDBUS_INTERFACE_STANDARD));
    }
}


VOID 
N6SdioFreeDevice( 
	IN PRT_SDIO_DEVICE		pDevice
)
{
	PADAPTER	Adapter;
	Adapter = (PADAPTER)pDevice->pAdapter;

	FreeSdioAWBs(pDevice, FALSE);

	// Free Tx Rx queue resources
	N6SdioFreeTxQueueRes(Adapter);

#if USE_WDF_SDIO
	WdfSdio_DeInitAsyncSDRequest(Adapter);
#endif

	// Free Rx NetBufferListPool, 2006.10.04, by shien chang.
	if (pDevice->RxNetBufferListPool)
	{
		N6SdioFreeRxNetBufferListPool(Adapter);
	}

	// Dereference SDIO Bus interface.
	if(pDevice->Sdbusinterface.InterfaceDereference)
	{
		//
		// Call the bus driver to dereference the interface.
		//
		(pDevice->Sdbusinterface.InterfaceDereference)
			(pDevice->Sdbusinterface.Context);
		RtlZeroMemory(&pDevice->Sdbusinterface, sizeof(SDBUS_INTERFACE_STANDARD));
	}
	
}

VOID
N6SdioCancelAllTimer(
	IN	PADAPTER	Adapter
	)
{
	NicIFCancelAllTimer((PADAPTER)Adapter);
}

VOID
N6SdioFreeAll(
	IN	PADAPTER	Adapter
	)
{
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);

	// Release all we do during initialization
	
	//
	// All packets queued in the driver are returned (to the upper layer). By Bruce, 2008-11-28.
	//
	ReleaseDataFrameQueued(Adapter);
	
	// Wait for All Timer Callback Completion (Including Extension Port) ----
	N6WaitTimerSync(Adapter);
	// ----------------------------------------------------------

	// Free All Ndis 6.20+ Component ---------------------
	NDIS_6_2_FREE_EXTENSION_COMPONENT(Adapter);
	// -----------------------------------------------
	
	NicIFDeInitResource(Adapter);

	NicIFFreeMemory((PADAPTER)Adapter);
	NicIFDisassociateNIC((PADAPTER)Adapter);

	//
	// 070307, rcnjko: 
	// We MUST free USB pipes after NicIFDisassociateNIC() 
	// because some workitem are still working and manipulate 
	// USB pipes until NicIFDisassociateNIC() is finished, 
	// e.g. GPIOChangeRFWorkItem.
	//
	N6SdioFreeDevice(device);

	// Free spin lock.
	// <RJ_NOTE> We free spin locks here because NicIFFreeMemory() still use TX and RX spin locks. 2006.03.08, by rcnjko.
	NdisFreeSpinLock( &(device->IrpSpinLock) );	
	N6CFreeSpinLocks(Adapter);

	// Free the Timer Synchronization Mechanism ---------
	N6DeInitTimerSync(Adapter);
	// ---------------------------------------------
	
	NdisOIDHistoryDeInit(Adapter);	

	// Free the PortCommonInfo -----------------------
	PortCommonInfoFreeMemory(Adapter);
	// --------------------------------------------
	
	// Free the memory block allocated for ADAPTER, RT_NDIS6_COMMON and so on.
	N6FreeAdapter((PADAPTER)Adapter);

}


NDIS_STATUS
N6SdioAllocateRxNetBufferListPool(
	IN	PADAPTER	Adapter
	)
{
	NET_BUFFER_LIST_POOL_PARAMETERS	NBLPoolParameters;
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);
	NDIS_HANDLE		NBLPoolHandle = NULL;
	
	PlatformZeroMemory(&NBLPoolParameters, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		NBLPoolParameters.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1,
		sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
	
	NBLPoolParameters.ProtocolId = 0;
	NBLPoolParameters.fAllocateNetBuffer = TRUE;
	NBLPoolParameters.ContextSize = 0;
	NBLPoolParameters.PoolTag = '3278';
	NBLPoolParameters.DataSize = 0;

	NBLPoolHandle = NdisAllocateNetBufferListPool(device->hNdisAdapter, &NBLPoolParameters);
	if (NBLPoolHandle == NULL)
	{
		return NDIS_STATUS_FAILURE;
	}

	device->RxNetBufferListPool = NBLPoolHandle;

	return NDIS_STATUS_SUCCESS;
}


VOID
N6SdioFreeRxNetBufferListPool(
	IN	PADAPTER	Adapter
	)
{
	NDIS_HANDLE		NBLPoolHandle = Adapter->NdisSdioDev.RxNetBufferListPool;
	
    if(NBLPoolHandle)
    {
        RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioFreeRxNetBufferListPool +1\n"));
	NdisFreeNetBufferListPool(NBLPoolHandle);
        Adapter->NdisSdioDev.RxNetBufferListPool = NULL;
    }
    else
    {
        RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioFreeRxNetBufferListPool RX NBL Pool NULL!\n"));
    }
}

//
//  Description:
//          For connected standby platform, we should disable device interrupt and
//  instruct the SD bus driver to stop forwarding device interrupt before the device
//  entering D2 state.
//
//  2013.08.07, Added by tynli.
//
VOID
N6SdioDisableDeviceInterrupt(
	IN	PADAPTER	Adapter
)
{
	PRT_SDIO_DEVICE	pDevice = GET_RT_SDIO_DEVICE(Adapter);
	BOOLEAN			bEnableInt;
	NTSTATUS 		ntStatus = STATUS_SUCCESS;
	
	//
	// Read interrupt property from current function.
	//
	ntStatus = PlatformSdioGetProperty(
					pDevice,
					SDP_FUNCTION_INT_ENABLE,
					&bEnableInt,
					sizeof(bEnableInt));	
		if(!NT_SUCCESS(ntStatus)){ 
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Cannot get property SDP_FUNCTION_INT_ENABLE\n"));
			 //return ntStatus;
	}

	//
	// Set interrupt property from current function.
	//
	if(bEnableInt)
	{
		bEnableInt = FALSE; // mask interrupt for our function
		ntStatus = PlatformSdioSetProperty(
						pDevice,
						SDP_FUNCTION_INT_ENABLE,
						&bEnableInt,
						sizeof(bEnableInt));	
			if(!NT_SUCCESS(ntStatus)){ 
				RT_TRACE(COMP_POWER, DBG_LOUD, ("Cannot set property SDP_FUNCTION_INT_ENABLE\n"));
				//return ntStatus;
			}
	}

	//
	// Instruct the SD bus driver to stop forwarding device interrupt.
	// 
	bEnableInt = FALSE;
	ntStatus = PlatformSdioSetProperty(
					pDevice,
					SDP_SET_CARD_INTERRUPT_FORWARD,
					&bEnableInt,
					sizeof(bEnableInt));	
		if(!NT_SUCCESS(ntStatus)){ 
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Cannot set property SDP_SET_CARD_INTERRUPT_FORWARD\n"));
			//return ntStatus;
		}
}

//
//  Description:
//          For connected standby platform, we should enable device interrupt and
//  instruct the SD bus driver to forward interrupts to the miniport driver after the device
//  resuming from D2 state.
//
//  2013.08.07, Added by tynli.
//
VOID
N6SdioEnableDeviceInterrupt(
	IN	PADAPTER	Adapter
)
{
	PRT_SDIO_DEVICE	pDevice = GET_RT_SDIO_DEVICE(Adapter);
	BOOLEAN			bEnableInt;
	NTSTATUS 		ntStatus = STATUS_SUCCESS;

	//
	// Instruct the SD bus driver to forward interrupts to our miniport driver.
	// 
	bEnableInt = TRUE; 
	ntStatus = PlatformSdioSetProperty(
					pDevice,
					SDP_SET_CARD_INTERRUPT_FORWARD,
					&bEnableInt,
					sizeof(bEnableInt));	
	if(!NT_SUCCESS(ntStatus)){ 
		RT_TRACE(COMP_POWER, DBG_LOUD, ("Cannot set property SDP_SET_CARD_INTERRUPT_FORWARD\n"));
		//return ntStatus;
	}
	
	//
	// Read interrupt property from current function.
	//
	ntStatus = PlatformSdioGetProperty(
					pDevice,
					SDP_FUNCTION_INT_ENABLE,
								&bEnableInt,
								sizeof(bEnableInt));	
		if(!NT_SUCCESS(ntStatus)){ 
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Cannot get property SDP_FUNCTION_INT_ENABLE\n"));
			 //return ntStatus;
	}
	
	//
	// Set interrupt property from current function.
	//
	if(!bEnableInt)
	{
		bEnableInt = TRUE; // Enable interrupt for our function
		ntStatus = PlatformSdioSetProperty(
						pDevice,
						SDP_FUNCTION_INT_ENABLE,
									&bEnableInt,
									sizeof(bEnableInt));	
			if(!NT_SUCCESS(ntStatus)){ 
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Cannot set property SDP_FUNCTION_INT_ENABLE\n"));
				//return ntStatus;
		}
	}
}

#if POWER_MAN

/**
** @@@
**20061205 by david, this function is for power management s3/s4 state
**
**
**
**/
NDIS_STATUS
N6Sdio_Mgnt_SetPower(
	PADAPTER	            	Adapter,
	IN	PVOID	            	InformationBuffer,
	IN	ULONG            		InformationBufferLength,
	OUT	PULONG          		BytesNeeded,
	OUT	PULONG            		BytesRead
)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER					pDefaultAdapter = GetDefaultAdapter(Adapter);
	
	NDIS_STATUS					Status = NDIS_STATUS_SUCCESS;
	PRT_SDIO_DEVICE				pDevice = GET_RT_SDIO_DEVICE(Adapter);
	PRT_NDIS6_COMMON			pNdisCommon = Adapter->pNdisCommon;
	NDIS_DEVICE_POWER_STATE 	NewPowerState;
	OCTET_STRING	 			savedssid;
	u1Byte		     		 	ssidbuf[33];
	u1Byte						bssidbuf[6];
	BOOLEAN						bEnableFastResume = FALSE;
	BOOLEAN 					bSupportRemoteWakeUp, bEnableWoLCapabilities;
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	BOOLEAN 					bResult = FALSE;
	
	LARGE_INTEGER	StartTime, EndTime;
	LARGE_INTEGER	Freq;

	if( InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE) )
	{
		Status = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
		RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
		return Status;
	}
	
	NewPowerState = *((NDIS_DEVICE_POWER_STATE*)InformationBuffer);
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);
	bEnableWoLCapabilities = MgntIsWoWLANCapabilityEnable(Adapter);

    if (NewPowerState <= NdisDeviceStateUnspecified || NewPowerState >= NdisDeviceStateMaximum)
	{
		Status = NDIS_STATUS_INVALID_DATA;
		*BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
		RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER: inavalid power state\n") );
		return Status;
	}

	if(pDevice->CurrentPowerState ==NewPowerState)
	{
		Status = NDIS_STATUS_SUCCESS;
		RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER:Alread in this  power state\n") );
		return Status;
	}

	
	// For Working Time Calculation
	Adapter->bCtrlPnPTime = TRUE;
	Adapter->PnPTotalTime.QuadPart = 0;
	Adapter->PnPIOTime.QuadPart = 0;
	pDevice->PnPSdBusWorkTime.QuadPart = 0;
				
	StartTime = KeQueryPerformanceCounter(&Freq);
	//
	
	// 2010/08/11 MH Add to protect USB minicard selective suspend HW radio detection test
	pMgntInfo->bSetPnpPwrInProgress = TRUE;
	
	if(NewPowerState == NdisDeviceStateD0)
	{ // Wake up.

		RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER: wake up from %d to D0\n", pDevice->CurrentPowerState));

		pDevice->CurrentPowerState = NewPowerState;

		pPSC->PnpWakeD0Cnt++;
		pPSC->LastPnpWakeTime = PlatformGetCurrentTime();

		Adapter->bSurpriseRemoved = FALSE;

		Adapter->bInSetPower = TRUE;

		MgntStopBeacon(Adapter);		

		if(pMgntInfo->bPreTransPnP)
		{
			//
			// <Roger_Notes> We need to complete this OID request with pending status immediately to shorten the 
			// power transition latency caused by the device resumption operation in OID_PNP_SET_POWER handling.
			// 2014.05.09.
			//
			RT_TRACE(COMP_POWER, DBG_LOUD, ("N6Sdio_Mgnt_SetPower(): Schedule WI for PnP Configuration!!\n"));
			//MultiPortSetAllPortsHWReadyStatus(Adapter, FALSE);
			bResult = PlatformScheduleWorkItem( &(pDevice->PnpWorkItem) );

			if(bResult == FALSE)
			{				
				pMgntInfo->bSetPnpPwrInProgress = FALSE;
				Status= NDIS_STATUS_SUCCESS;
			}
			else
				Status= NDIS_STATUS_PENDING;// Always pend this request for PnP pre-transition
		}
		else
		{
			if(NDIS_CURRENT_IRQL() > PASSIVE_LEVEL)
			{
				// Schedule a workitem to do necessary tasks before entering sleep state. 
				// The OID will be completed until these tasks finished.
				RT_TRACE(COMP_POWER, DBG_LOUD, ("N6Sdio_Mgnt_SetPower(): IRQL > PASSIVE. Call workitem!!!\n"));
				bResult = PlatformScheduleWorkItem( &(pDevice->PnpWorkItem) );
				if(bResult == FALSE){				
					pMgntInfo->bSetPnpPwrInProgress = FALSE;
					Status= NDIS_STATUS_SUCCESS;
				}
				else
					Status= NDIS_STATUS_PENDING;
			}
			else
			{
				//
				// Immediately disable adapter in this irql level, then return success. By Bruce, 2008-10-29.
				//
				PnpWorkItemCallback(Adapter);
				Status= NDIS_STATUS_SUCCESS;
			}
		}
	}
	else if(NewPowerState!=NdisDeviceStateD0)
	{ // Sleep.
		RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER: going to sleep: %d\n", NewPowerState));

		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			PMGNT_INFO		pExtMgntInfo = NULL;
			PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;

			pDefaultMgntInfo->bDriverIsGoingToSleep = TRUE; 
		
			while(pExtAdapter != NULL)
			{
				pExtMgntInfo = &pExtAdapter->MgntInfo;
			
				pExtMgntInfo->bDriverIsGoingToSleep = TRUE; 
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}

		MgntResetJoinProcess(Adapter);

		// For Intel Bay trail platform, we do not support WoWLAN during hibernate (S4). 2013.10.30, by tynli.
		if(NewPowerState != NdisDeviceStateD2)
		{
			pPSC->bPnpEnterD2 = FALSE;
			if(NewPowerState == NdisDeviceStateD3)
				pPSC->PnpSleepEnterD3Cnt++;
			else
				pPSC->PnpSleepEnterUnknownDxCnt++;
		}
		else
		{
			pPSC->bPnpEnterD2 = TRUE;
			pPSC->PnpSleepEnterD2Cnt++;
		}
		
		pPSC->LastPnpSleepTime = PlatformGetCurrentTime();

		// Cancel watchdog workitem
#if USE_WORKITEM
		{
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	Stop check for hang workitem\n"));
			PlatformStopWorkItem( &(pHalData->RtCheckForHangWorkItem) );
		}
#endif
		
		if( !Adapter->bInHctTest )
		{	// Those in vWifi mode now disassociate all STA before sleep, and will be reset after waking up.
		
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);
				
			while(pTargetAdapter != NULL)
			{
				if(MgntActQuery_ApType(pTargetAdapter) == RT_AP_TYPE_VWIFI_AP)
				{
					AP_DisassociateAllStation(pTargetAdapter, unspec_reason);
					SecSetSwEncryptionDecryption(pTargetAdapter, FALSE, FALSE);
		
					RT_TRACE(COMP_AP, DBG_LOUD,("vWifi mode on Port: %d disassociates all STA.\n", pTargetAdapter->pNdis62Common->PortNumber));
				}		
		
					pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
		else
		{	// For OS before Win7 still go turning OFF directly.
			NDIS_6_2_AP_CLEAR_BEFORE_SLEEP(Adapter);	
		}

#if P2P_SUPPORT == 1
		if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
		{
			if(P2P_DOING_DEVICE_DISCOVERY(GET_P2P_INFO(Adapter)))
			{// Doing P2P Device Discovery
				P2PScanListCeaseScan(GET_P2P_INFO(Adapter));
				P2PDeviceDiscoveryComplete(GET_P2P_INFO(Adapter), TRUE); // P2P State is restored in this function.
			}			
		}

#if (MULTIPORT_SUPPORT == 1)
		if(GetFirstDevicePort(Adapter))
		{
			(GET_P2P_INFO(GetFirstDevicePort(Adapter)))->uListenStateDiscoverability = 0;
		}
#endif		
#endif
		//
		// Stop scan, roaming and link operation.
		//
		if(MgntScanInProgress(pMgntInfo) || MgntIsLinkInProgress(pMgntInfo) || MgntRoamingInProgress(pMgntInfo))
		{
			RT_TRACE(COMP_POWER, DBG_WARNING, ("@!!!!!Mgnt_SetPower we are scanning.....\n"));

			if(MgntScanInProgress(pMgntInfo))
				MgntResetScanProcess(Adapter);
			PlatformCancelTimer(Adapter, &pMgntInfo->ScanTimer);
			{
				PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);
			
				while(pLoopAdapter)
				{
					pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
					pLoopAdapter->MgntInfo.bDualModeScanStep = 0;
					
					pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
				}
			}

			if( MgntIsLinkInProgress(pMgntInfo) || MgntRoamingInProgress(pMgntInfo))
			{
				pMgntInfo->bJoinInProgress = FALSE;
				if(MgntRoamingInProgress(pMgntInfo))
					DrvIFIndicateRoamingStart(Adapter);
				else
					DrvIFIndicateConnectionStart(Adapter);
				
				DrvIFIndicateAssociationStart(Adapter);
				DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
				
				if(MgntRoamingInProgress(pMgntInfo))
					DrvIFIndicateRoamingComplete(Adapter, RT_STATUS_FAILURE);
				else
					DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);		
				MgntActSet_802_11_DISASSOCIATE( Adapter, unspec_reason);	
			}			
		}

		MgntResetRoamingState(pMgntInfo);
				
		//MgntDisconnectAP(Adapter , unspec_reason);
		if(Adapter->MgntInfo.bMediaConnect || pMgntInfo->bIbssStarter)
		{
			RT_RF_POWER_STATE eRfPowerState;

			RT_TRACE(COMP_POWER, DBG_LOUD, ("@!!!!!Mgnt_SetPower before we save the ssid, the ssid is %s bMediaConnect %d bIbssStarter %d\n", Adapter->MgntInfo.Ssid.Octet, pMgntInfo->bMediaConnect, pMgntInfo->bIbssStarter));

			//save SSID
			savedssid.Octet = ssidbuf;
			CopySsidOS(savedssid, Adapter->MgntInfo.Ssid);	
			CopyMem(bssidbuf,Adapter->MgntInfo.Bssid, 6);

			//Add for DTM 1.0c test.
			if(!pMgntInfo->bHiddenSSIDEnable)
				MgntRemoveSsidsToScan(Adapter, Adapter->MgntInfo.Ssid);			

			//
			// CCW: don't indicae dissociation event on infra. mode client.
			// 070125, rcnjko: don't indicate disassociation event
			//		

			if(!bEnableWoLCapabilities ||
				(pPSC->FSSDetection && (!pPSC->bSetPMParameters || (pPSC->bSetPMParameters && !pPSC->bOSSupportProtocolOffload)))) 
			{	// Do not disasso to AP when WoWLAN. by tynli.
			
				if( Adapter->MgntInfo.OpMode == RT_OP_MODE_INFRASTRUCTURE )
				{
					Adapter->pNdisCommon->bDissociateBeforeSleep = TRUE;
				}
			
				MgntActSet_802_11_DISASSOCIATE(Adapter , unspec_reason);
				Adapter->pNdisCommon->bPnpKeepConnectToAP = FALSE;

                                //delay 50ms to let hw send disassociation packet successful
                                //fix s3 fail with Netgear3500 v1
                                //by sherry 20101124
				PlatformStallExecution(50000);
			}
			else
			{ // WoWLAN
				if( Adapter->MgntInfo.OpMode == RT_OP_MODE_INFRASTRUCTURE && pPSC->bPnpEnterD2)
					Adapter->pNdisCommon->bPnpKeepConnectToAP = TRUE;
				else
					Adapter->pNdisCommon->bPnpKeepConnectToAP = FALSE;
			}

			//restore SSID
			CopySsidOS(Adapter->MgntInfo.Ssid, savedssid);	
			CopyMem(Adapter->MgntInfo.Bssid, bssidbuf, 6);
		}
		else
		{
			Adapter->pNdisCommon->bPnpKeepConnectToAP = FALSE;
		}

		{
			//PADAPTER pTargetAdapter = 
			//	MgntActQuery_ApType(GetDefaultAdapter(Adapter)) == RT_AP_TYPE_VWIFI_AP ? 
			//	GetDefaultAdapter(Adapter) : 
			//	(MgntActQuery_ApType(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE)) == RT_AP_TYPE_VWIFI_AP ? ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE) : NULL);

			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

			while(pTargetAdapter != NULL)
			{
				if(MgntActQuery_ApType(pTargetAdapter) == RT_AP_TYPE_VWIFI_AP)
				{
					// stop beaconing. for NDISTest preview 3 SoftAP_Power_ext.
					PMGNT_INFO pTargetMgntInfo = &pTargetAdapter->MgntInfo;
				
					MgntStopBeacon(pTargetAdapter);

					// Configure the HW to be No Link.
					pTargetMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pTargetMgntInfo->OpMode));
					pTargetMgntInfo->OpMode = RT_OP_MODE_AP;
				}
				
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
			
		pDevice->CurrentPowerState = NewPowerState;
		Adapter->bInSetPower = TRUE;

		if(NDIS_CURRENT_IRQL() > PASSIVE_LEVEL)
		{
			// Schedule a workitem to do necessary tasks before entering sleep state. 
			// The OID will be completed until these tasks finished.
			RT_TRACE(COMP_POWER, DBG_LOUD, ("N6Sdio_Mgnt_SetPower(): IRQL > PASSIVE. Call workitem!!!\n"));
			bResult = PlatformScheduleWorkItem( &(pDevice->PnpWorkItem) );

			if(bResult == FALSE){				
				pMgntInfo->bSetPnpPwrInProgress = FALSE;
				Status= NDIS_STATUS_SUCCESS;
			}
			else
				Status= NDIS_STATUS_PENDING;
		}
		else
		{
			//
			// Immediately disable adapter in this irql level, then return success. By Bruce, 2008-10-29.
			//
			PnpWorkItemCallback(Adapter);
			Status= NDIS_STATUS_SUCCESS;
		}
	}
	else 
	{ // Otherwise.
		RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER should not reach here: Current: %d to New: %d\n", pDevice->CurrentPowerState, NewPowerState));

		Status = NDIS_STATUS_SUCCESS;
	}

	// For Working Time Calculation 		
	EndTime = KeQueryPerformanceCounter(NULL);
	Adapter->bCtrlPnPTime = FALSE;
	
	Adapter->PnPTotalTime.QuadPart = (EndTime.QuadPart - StartTime.QuadPart) * 1000000;
	Adapter->PnPTotalTime.QuadPart = Adapter->PnPTotalTime.QuadPart / Freq.QuadPart;
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[PnP Time] Wake Up Total Cost Time: %llu us\n", Adapter->PnPTotalTime.QuadPart));
	
	Adapter->PnPIOTime.QuadPart = Adapter->PnPIOTime.QuadPart * 1000000;
	Adapter->PnPIOTime.QuadPart = Adapter->PnPIOTime.QuadPart / Freq.QuadPart;
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[PnP Time] IO Cost Time: %llu us\n", Adapter->PnPIOTime.QuadPart));
	
	pDevice->PnPSdBusWorkTime.QuadPart = pDevice->PnPSdBusWorkTime.QuadPart * 1000000;
	pDevice->PnPSdBusWorkTime.QuadPart = pDevice->PnPSdBusWorkTime.QuadPart / Freq.QuadPart;
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[PnP Time] SdBus Submit Cost Time: %llu us\n", pDevice->PnPSdBusWorkTime.QuadPart));
	//

	*BytesRead = InformationBufferLength;
	
	return  Status;
}

//@@@ 
// 
// Callback function of the workitem PnpWorkitem
// 20061205 pnp work item for wake up from s3 mode
//by David
VOID
PnpWorkItemCallback(
	IN PVOID			pContext
	)
{

	PADAPTER				Adapter 		= (PADAPTER)pContext;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_SDIO_DEVICE			pDevice 		= GET_RT_SDIO_DEVICE(Adapter);
	PRT_POWER_SAVE_CONTROL	pPSC 			= GET_POWER_SAVE_CONTROL(&(Adapter->MgntInfo));
	PRT_NDIS_COMMON			pNdisCommon 	= Adapter->pNdisCommon;
	PMGNT_INFO				pMgntInfo		= &Adapter->MgntInfo;
	PRT_NLO_INFO			pNLOInfo 		= &(pMgntInfo->NLOInfo);		
	BOOLEAN					bWakeReconnect	= TRUE;
	BOOLEAN 				bMacPwrCtrlOn;	
	BOOLEAN 				bSupportRemoteWakeUp;
	BOOLEAN					bEnableWoLCapabilities;
	RT_RF_POWER_STATE 		rfState;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("-----> PnpWorkItemCallback()\n"));

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);
	bEnableWoLCapabilities = MgntIsWoWLANCapabilityEnable(Adapter);
	
	if(pDevice->CurrentPowerState == NdisDeviceStateD0)
	{ // wake up. 
		
		//
		// <Roger_Notes> Reset corresponding PnP event to monitor MiniportCheckForHang routine.
		// 2009.09.30.
		//
		pDevice->bChkForHangAfterPnP = FALSE;
		NdisResetEvent(&pDevice->SetPnpChkForHangEvent);
	
		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			PMGNT_INFO		pExtMgntInfo = NULL;
			PRT_POWER_SAVE_CONTROL	pExtPSC = NULL;
			PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;
			PRT_POWER_SAVE_CONTROL pDefualtPSC = GET_POWER_SAVE_CONTROL(pDefaultMgntInfo);

			pDefaultAdapter->bEnterPnpSleep = FALSE;
			pDefaultAdapter->bWakeFromPnpSleep = TRUE;
			pDefaultAdapter->bDriverIsGoingToPnpSetPowerSleep = FALSE;
		
			while(pExtAdapter != NULL)
			{
				pExtMgntInfo = &pExtAdapter->MgntInfo;
				pExtPSC = GET_POWER_SAVE_CONTROL(pExtMgntInfo);
			
				pExtAdapter->bEnterPnpSleep = FALSE;
				pExtAdapter->bWakeFromPnpSleep = TRUE;
				pExtAdapter->bDriverIsGoingToPnpSetPowerSleep = FALSE;
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}

		if(Adapter->MgntInfo.RegSuspendTimerInLowPwr)
			N6CTimerResourceAction(Adapter, RT_TIMER_RESOURCE_ACTION_RESUME);

		// Clear the flag after resuming from device low low power state. We expect that
		// before the system sets OID_PNP_CAPABILITIES to put the device to low PS state, it will
		// set OID_PM_PARAMETERS first. In order to seperate FSS mode from S3/S4/Selective suspend
		// mode, we use "bSetPMParameters" flag to decide some behaviors. 2012.09.13, by tynli.
		pPSC->bSetPMParameters = FALSE; 

		RT_ENABLE_SDIO_TRANSFERS(Adapter);

		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

			while(pTargetAdapter != NULL)
			{
				pTargetAdapter->bInitializeInProgress=TRUE;
				pTargetAdapter->bSWInitReady=TRUE;

				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}

#if USE_WDF_SDIO
		WdfSdioTx_Enable(Adapter);
#else
		N6WdmSdio_Enable(Adapter);
		N6WdmSdioTx_Enable(Adapter);
#endif
		N6SdioDummyIO(pDevice);
		
		// Control SDIO Clock rate settings
		if(pDevice->bRegSdioSpeedSDR25)
		{
			N6SdioConfigureBusCLKByWorkAround(pDevice);
		}

		if(bEnableWoLCapabilities)
		{
			N6SdioEnableDeviceInterrupt(Adapter);
		}

	
		//
		// 070106, rcnjko:
		// Re-initialize H/W for USB Bus is suspend.
		// Besides, we also issue bulk IN transfers at InitializeAdapterHandler().
		//
		// <tynli_note> On Intel Bay Trail low power platform, "D2 -> D0 -> miniport shutdown" function will be 
		// called by user initialiated Restart. It has site effect to 8723BS card lost issue. So this is a work around
		// to add the condition to prevent from acting HaltAdapter in PNP D2 and D0 under Restart flow. 2013.11.18.
		if((pMgntInfo->bIntelPatchPNP && !pMgntInfo->bReceiveSystemPSOID) || (!pMgntInfo->bIntelPatchPNP))
		{
			Adapter->HalFunc.InitializeAdapterHandler(Adapter, Adapter->MgntInfo.dot11CurrentChannelNumber);
		}

		//
		// <tynli_Note> Reconnect to AP in the following conditions for AOAC on connected standby platform.
		// (1) The media state is connected before PNP sleep and HW is re-initiated after resupmtion.
		// (2) The media state is connected before PNP sleep and wake reason is AP lost, on disassoc, or on deauth.
		// (3) According to the variable "bPnpKeepConnectToAP", we should keep connection (do not perform reconnect
		//      mechanism) if "bPnpKeepConnectToAP" is TRUE. It will be set to TRUE while remote wake up function
		//      is decided to be operation in PNP sleep flow. 2014.11.12, by tynli.
		//
		if(pMgntInfo->bRegPnpKeepLink)
		{
			if((Adapter->MgntInfo.bMediaConnect && (Adapter->bReInitHW || pPSC->WakeUpReason == WOL_REASON_AP_LOST ||
				pPSC->WakeUpReason == WOL_REASON_DISASSOC ||pPSC->WakeUpReason == WOL_REASON_DEAUTH)) || 
				(!Adapter->pNdisCommon->bPnpKeepConnectToAP))
			{
				pMgntInfo->bPerformPnpReconnect = TRUE;
			}
			else
			{
				pMgntInfo->bPerformPnpReconnect = FALSE;
			}
	
			RT_TRACE(COMP_POWER, DBG_LOUD, ("PnPSetPower(): bPerformPnpReconnect=%d, bReInitHW=%d\n", 
				pMgntInfo->bPerformPnpReconnect, Adapter->bReInitHW));
		}
		else
		{
			pMgntInfo->bPerformPnpReconnect = TRUE;
		}

		if(bSupportRemoteWakeUp) // Need to be set before N6RestoreLastInitSettingAterWakeUP().
		{							
			Adapter->HalFunc.EnableHWSecCfgHandler(Adapter); //For HW Security. by tynli. 2009.06.24.
		}	
		

		PlatformSetCheckForHangTimer(Adapter);

		PlatformStartWorkItem(&(Adapter->pPortCommonInfo->pPortHelper->CreateDeleteMacWorkitem));
		
		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

			while(pTargetAdapter != NULL)
			{
				pTargetAdapter->bInitializeInProgress = FALSE;
				
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
		
		//ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE)->bInitializeInProgress=FALSE;
		//pDefaultAdapter->bInitializeInProgress=FALSE;

		//fix some InitilaAdapter error under sleep

		if( !Adapter->bInHctTest )
		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);
				
			while(pTargetAdapter != NULL)
			{
				RT_TRACE(COMP_AP, DBG_LOUD, ("Adapter on Port: %d, APType: %d\n", pTargetAdapter->pNdis62Common->PortNumber, MgntActQuery_ApType(pTargetAdapter)));
				if(MgntActQuery_ApType(pTargetAdapter) == RT_AP_TYPE_VWIFI_AP)
				{
					// Force vWifi being OFF(STATE: not available), then reset it(STATE: not started)
					pTargetAdapter->bvWifiStopBeforeSleep = TRUE;
					N62CApIndicateStopAp(pTargetAdapter);
				}
					
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
		
		Adapter->pNdisCommon->bWakeupAutoLinkInProgressing = TRUE;

		if(pMgntInfo->bPerformPnpReconnect)
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	PnpWorkItemCallback()  try to connect to %s\n", Adapter->MgntInfo.Ssid.Octet));
		}
		N6RestoreLastInitSettingAterWakeUP(Adapter);

		// Indicate disassociation state when RF is off becuase we do not disconnect before sleep in WoWLAN mode.		
		// 2011.10.27. by tynli.
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));
		if(rfState == eRfOff)
		{
			if(Adapter->pNdisCommon->bPnpKeepConnectToAP)
			{
				DrvIFIndicateDisassociation(Adapter, unspec_reason, pMgntInfo->Bssid);
			}
			pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
		}	

		if(OS_SUPPORT_WDI(Adapter))
		{
		pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
			// TODO: WoWLAN
		
			// TODO: NLO
		}
		else if(pNdisCommon->bWakeupAutoLinkInProgressing)
		{
			if( pNLOInfo->NumDot11OffloadNetwork != 0 && Adapter->bInHctTest)
			{
				u1Byte i = 0;
				BOOLEAN bTargetInNLO = FALSE;
				
				if(Adapter->MgntInfo.bMediaConnect || pNdisCommon->bDissociateBeforeSleep)
				{				
					for(i=0; i<pNLOInfo->NumDot11OffloadNetwork; i++)
					{
						if(pMgntInfo->Ssid.Length && (pMgntInfo->Ssid.Length == pNLOInfo->dDot11OffloadNetworkList[i].ssidlen))
							if(!PlatformCompareMemory(pMgntInfo->Ssid.Octet, pNLOInfo->dDot11OffloadNetworkList[i].ssidbuf, pMgntInfo->Ssid.Length))
								bTargetInNLO = TRUE;
					}

					if(!bTargetInNLO)
					{
						RT_TRACE(COMP_POWER, DBG_LOUD, ("PnPSetPower(): LinkTargetSSID is not in Dot11OffloadNetworks!\n"));
						pNdisCommon->ScanPeriod = 0;
						PlatformHandleNLOnScanRequest(Adapter);
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
					else
					{
						RT_TRACE(COMP_POWER, DBG_LOUD, ("PnPSetPower(): FFFFind LinkTargetSSID in Dot11OffloadNetworks!\n"));
						if(!(ACTING_AS_AP(Adapter) || IsExtAPModeExist(Adapter)))
						{
							if(pMgntInfo->Ssid.Length != 0)
							{
								RT_TRACE(COMP_POWER, DBG_LOUD, ("PnpSetPower()	try to connect to %s\n", Adapter->MgntInfo.Ssid.Octet));

								// Set Roam flag 
								MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_SLEEP, ROAMINGSTATE_SCANNING);

								DrvIFIndicateRoamingStart(Adapter);
								pNdisCommon->bDissociateBeforeSleep = FALSE;

								MgntActSet_802_11_SSID(Adapter, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, TRUE );
							}
						}
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
				}
				else
				{
					//if(pPSC->DxNLOEnable && (pPSC->WakeUpReason == WOL_REASON_NLO_SSID_MATCH))
					{ // FW NLO SSID matched
						// We should execute scan here because after indicating NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED,
						// OS will not set scan request OID (different with msdn's mention) and then query BSS list OID. So if we
						// do not keep NLO matched SSID in BSS list, the state machine will keep in disconnect then HCK test will be failed.
						// This may be a workaround because we do not know the behavior expected by MSFT. 2014.03.06. Add by tynli.
						pNdisCommon->ScanPeriod = 0;
						PlatformHandleNLOnScanRequest(Adapter);
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
				}
//				pMgntInfo->bPwrSaveState = FALSE;
//				Adapter->bWakeFromPnpSleep = FALSE;
			}
			else
			{
				// NLO enabled by the upper layer and in disconnect state, we should make sure that we will connect
				// to the expected AP by indicating NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED
				// to declare that NLO scan is finished.
				if((pNLOInfo->NumDot11OffloadNetwork != 0) && (!Adapter->MgntInfo.bMediaConnect) &&
					(!pNdisCommon->bDissociateBeforeSleep)) 
				{
					// We should execute scan here because after indicating NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED,
					// OS will not set scan request OID (different with msdn's mention) and then query BSS list OID. So if we
					// do not keep NLO matched SSID in BSS list, the state machine will keep in disconnect then HCK test will be failed.
					// This may be a workaround because we do not know the behavior expected by MSFT. 2014.03.06. Add by tynli.
					pNdisCommon->ScanPeriod = 0;
					PlatformHandleNLOnScanRequest(Adapter);
					pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
				}
				else
				{
					// Temporarily modify this for Lenovo PEAP-GTC S3/S4 reconnect issue.
					// This issue shall be confirmed by DTM and other function test.
					/*if(pMgntInfo->CustomerID==RT_CID_819x_Lenovo)
					{
						bWakeReconnect = FALSE;
						// Customize for Lenovo. 2010.11.09. by tynli.
						DrvIFIndicateDisassociation(Adapter, unspec_reason);
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
					else*/
					{
						if((!ACTING_AS_AP(Adapter) || (!IsExtAPModeExist(Adapter) && bEnableWoLCapabilities)) 
							&& (pMgntInfo->Ssid.Length != 0) && !(IsSSIDDummy(pMgntInfo->Ssid)) && pMgntInfo->bPerformPnpReconnect
							)
						{
							RT_TRACE(COMP_POWER, DBG_LOUD, ("PnpSetPower()	try to connect to %s ACTING_AS_AP(Adapter)	%d IsExtAPModeExist(Adapter) %d\n", 
									Adapter->MgntInfo.Ssid.Octet, ACTING_AS_AP(Adapter) , IsExtAPModeExist(Adapter)));
							bWakeReconnect = TRUE;
						}
						else
						{
							bWakeReconnect = FALSE;
						}

						if(bWakeReconnect)
						{
							// Set Roam flag 
							MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_SLEEP, ROAMINGSTATE_SCANNING);

							pNdisCommon->PNPconnentCout = 0;
							// Try to connent AP !!
							// Set timer to 10ms to prevent from being returned by bSetPnpPwrInProgress in MgntLinkRetry function
							// or we could call PNPReConnectTimer directly.
							// 1. Indicate roaming start. 2. Reconnect to AP in PNPReConnectTimer().
							PlatformSetTimer(Adapter, &pNdisCommon->PNPReConnentTimer, 10);
						}
						else
						{
							RT_TRACE(COMP_POWER, DBG_LOUD, ("PnpSetPower(): do not try to connect to AP\n"));
							pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
							pNdisCommon->bDissociateBeforeSleep = FALSE;

							//
							// <tynli_note> If we keep connection after PNP resumption, we should reset Rx TS to avoid 
							// packet droped by the sequence number is over Rx reorder window size because we will not
							// update the Rx TS sequence number during PNP sleep stage. 2015.01.22.
							//
							if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE && pMgntInfo->mAssoc)
							{
								QosResetRxTS(Adapter);
								MgntRecoverFWOffloadMechanism(Adapter);
							}
						}
					}
				}
			}
			// Move the flag to be cleared in PNPReConnentTimerCallBack().
			//pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
		}
		Adapter->bWakeFromPnpSleep = FALSE;
#if USE_WORKITEM
		{
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	Start check for hang workitem\n"));
			PlatformStartWorkItem( &(pHalData->RtCheckForHangWorkItem) );
		}
#endif		
	}
	else if(pDevice->CurrentPowerState > NdisDeviceStateD0)
	{ // sleep.
		PlatformIndicateCustomStatus(
				Adapter,
				RT_CUSTOM_EVENT_IRP_UNLOAD,
				RT_CUSTOM_INDI_TARGET_IRP,
				NULL,
				0);
		
	{
		PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
		PMGNT_INFO		pExtMgntInfo = NULL;
		PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;

		pDefaultMgntInfo->bStartApDueToWakeup=TRUE;	

		pDefaultAdapter->bEnterPnpSleep = TRUE;
	
		while(pExtAdapter != NULL)
		{
			pExtMgntInfo = &pExtAdapter->MgntInfo;
		
			pExtMgntInfo->bStartApDueToWakeup=TRUE;	

			pExtAdapter->bEnterPnpSleep = TRUE;
			
			pExtAdapter = GetNextExtAdapter(pExtAdapter);
		}
	}
		pMgntInfo->bInToSleep = TRUE;

		//Add by Maddest for add weighting for BSSID matched AP For DTM test, 2007.08.07.
		// Run as roaming to connect to the original AP. By Bruce, 2008-06-09.

		//Moved to PNP wake flow. 2013.04.
		//MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_SLEEP, ROAMINGSTATE_SCANNING);
		
		// 20100721 Joseph: Modified for Velocity Suspend test.
		// Reset IPS state only.
		// Since NIC is going to Halt for sleep, driver does not need to initialize the HW again.
		// This shall be revised in the future and also take NIC Disable/Radio-off into consideration.
		if((!bSupportRemoteWakeUp) &&
			(pPSC->IPSState==eIPSDozed) &&
			/*(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC) &&*/ 
			RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVL_HALT_NIC))
		{
			LPSLeaveAndCheckReady(Adapter);

			NicIFDisableInterrupt(Adapter); 
		
			pPSC->eInactivePowerState = eRfOn;
			pPSC->IPSState = eIPSAwake;
			pMgntInfo->RfOffReason &= (~RF_CHANGE_BY_IPS);
		}
		else
		{
			LeaveAllPowerSaveMode(Adapter);		
		}
		NicIFClearInterrupt(Adapter);
		
		// Recover the BW to 40MHz for a specific case. To see the description of 
		// function HTRecoverBWTo40MHz().  2012.12.20, by tynli.
		if(bSupportRemoteWakeUp && 
			(Adapter->MgntInfo.OpMode == RT_OP_MODE_INFRASTRUCTURE))
		{
			HTRecoverBWTo40MHz(Adapter);
		}
		
		PlatformAcquireSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
		while( pMgntInfo->RFChangeInProgress)
		{
			PlatformReleaseSpinLock(Adapter,RT_RF_STATE_SPINLOCK);

			RT_TRACE(COMP_POWER,DBG_LOUD,
				("RF is in progress, need to wait until rf chang is done.\n"));
			delay_ms(1);
			
			PlatformAcquireSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
		}	
		PlatformReleaseSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
//		Adapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;

		//In Win7 for WoWLAN, we should write CAM to allow Hw to encrypt/decrypt packets when S3/S4 because
		//there are some type of packets are en/decrypted by Sw when wake up. by tynli.
		if(bSupportRemoteWakeUp)
		{
			PRT_SECURITY_T	pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
			u1Byte	KeyIndex;
			u1Byte CAM_CONST_BROAD[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
			
			Adapter->bHWSecurityInWoL = TRUE;
			
			Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);

			if(pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 ||
				pSecInfo->PairwiseEncAlgorithm ==RT_ENC_ALG_WEP104)
			{	//Set WEP key to hw (but WEP 1.x cannot work)
				for(KeyIndex=0; KeyIndex<4; KeyIndex++)
				{
					if(pSecInfo->KeyLen[KeyIndex] != 0)
					{
						Adapter->HalFunc.SetKeyHandler(Adapter, 
								KeyIndex, 
								CAM_CONST_BROAD, 
								FALSE, 
								pSecInfo->PairwiseEncAlgorithm, 
								TRUE, 
								FALSE);
					}
				}
			}
			else if( pSecInfo->PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER )
			{	//Set group key for other encrypyion mode (TKIP, AES)
				Adapter->HalFunc.SetKeyHandler(Adapter, 
							pSecInfo->GroupTransmitKeyIdx, //KeyIndex,
							CAM_CONST_BROAD,
							TRUE, //IsGroup,
							pSecInfo->GroupEncAlgorithm,
							FALSE,
							FALSE);
			}
			Adapter->bHWSecurityInWoL = FALSE;
		}
		//-Write CAM End------------------------------------------

{
		PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
		pDefaultAdapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;

		while(pExtAdapter != NULL)
		{
			pExtAdapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;
			
			pExtAdapter = GetNextExtAdapter(pExtAdapter);
		}
}

		// <tynli_note> On Intel Bay Trail low power platform, "D2 -> D0 -> miniport shutdown" function will be 
		// called by user initialiated Restart. It has site effect to 8723BS card lost issue. So this is a work around
		// to add the condition to prevent from acting HaltAdapter in PNP D2 and D0 under Restart flow. 2013.11.18.
		if((pMgntInfo->bIntelPatchPNP && !pMgntInfo->bReceiveSystemPSOID) || (!pMgntInfo->bIntelPatchPNP))
		{
			if(bSupportRemoteWakeUp && IS_WOWLAN_OPERATING_MODE(pMgntInfo))
				Adapter->HalFunc.SleepAdapterHandler(Adapter);
			else
				Adapter->HalFunc.HaltAdapterHandler(Adapter, FALSE);
		}

		// We may set H2C to Fw during HaltAdapter so release H2C queue after Halt. 2011.10.04. by tynli.
		Adapter->HalFunc.WaitForH2CQueueEmptyHandler(Adapter);

		if((pMgntInfo->RfOffReason & (RF_CHANGE_BY_IPS|RF_CHANGE_BY_SW|RF_CHANGE_BY_HW)) &&
			RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K))
		{
			// Wait FW 32K Event
			// Prefast warning C28121: The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
			// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
			// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
			if(!NdisWaitEvent(&pDevice->FwPsClockOffEvent, 20))
			{
				// Force Hw clock off to 32K
				RT_TRACE(COMP_POWER, DBG_LOUD, ("Force Hw clock off to 32K!\n"));
				SetFwPSRFOffLowPower(Adapter);
			}
		}

		if(pDevice->CurrentPowerState == NdisDeviceStateD2 && bEnableWoLCapabilities)
		{
			N6SdioDisableDeviceInterrupt(Adapter);
		}

		// Wait until all H2C cmd finished than disable cmd53 to prevent from cmd53 lost issue.
		// To disable cmd53 operation here to avoid that system reset NIC power during sleep or hibernate mode
		// but driver does not call card disable flow to disable cmd53 operation, it will cause driver use cmd53
		// to access non-power on regsiter then I/O fail after resume.
		// Dsiable CMD53 R/W operation
		bMacPwrCtrlOn = FALSE;// Disable CMD53 R/W
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));

		if(pDevice->bRegSdioSpeedSDR25)
		{
			N6SdioConfigureBusCLK_SDR12(pDevice);
		}

		RT_DISABLE_SDIO_TRANSFERS(Adapter);

#if USE_WDF_SDIO
		WdfSdio_Disable(Adapter);
#else
		N6WdmSdio_Disable(Adapter);	
#endif

		//
		// 070531, rcnjko: although NDIS6 had made driver stack go to paused state 
		// before sleeping, it is safer to return pending tx to upper layer.
		//
		N6SdioReturnAllPendingTxPackets(Adapter);

		// Release Tx Queue buffered context if needed. added by Roger, 2014.04.29.
		N6SdioReleaseTxQueuePending(Adapter);	

		// 2011/08/23 MH For check for hang test only. Prevent check for HANG IQK write incorrect BB register under
		// RF off state.
		{
			HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	Stop check for hang workitem\n"));
			PlatformStopWorkItem( &(pHalData->RtCheckForHangWorkItem) );
		}

		PlatformStopWorkItem(&(Adapter->pPortCommonInfo->pPortHelper->CreateDeleteMacWorkitem));

		//
		// As devices enter low power states, the driver managing the devices should cancel any programmed timers. 
		// Added by Roger, 2016.01.18
		//
		if(Adapter->MgntInfo.RegSuspendTimerInLowPwr){
			N6CTimerResourceAction(Adapter, RT_TIMER_RESOURCE_ACTION_SUSPEND);
			N6WaitTimerSync(Adapter);
		}

		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			PMGNT_INFO		pExtMgntInfo = NULL;
			PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;

			pDefaultMgntInfo->bDriverIsGoingToSleep = FALSE; 
		
			while(pExtAdapter != NULL)
			{
				pExtMgntInfo = &pExtAdapter->MgntInfo;
			
				pExtMgntInfo->bDriverIsGoingToSleep = FALSE; 
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}

		RT_TRACE(COMP_POWER, DBG_LOUD, ("	PnpWorkItemCallback()  sleep\n"));
	}
	else
	{
		RT_TRACE(COMP_POWER, DBG_WARNING, ("PnpWorkItemCallback(): unexpected CurrentPowerState: %#X\n", pDevice->CurrentPowerState));
	}
	
	if(pMgntInfo->bPreTransPnP)
	{	
		if(pDevice->CurrentPowerState == NdisDeviceStateD0)
		{// wake up. 
		
			//MultiPortSetAllPortsHWReadyStatus(Adapter, TRUE);

			//
			// <Roger_Notes> We should complete corresponding OID we pended before for PnP wakeup pre-transition if needed.
			// 2012.11.29.
			//
			PlatformAcquireSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
			if(pNdisCommon->PendedRequest)
			{
				PlatformReleaseSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
				RT_TRACE(COMP_POWER, DBG_LOUD, ("PnpWorkItemCallback(): Complete Pended OID for PnP!!\n"));
				N6CompletePendedOID(Adapter, RT_PENDED_OID_PNP, 
					N6SdioOidRequest((NDIS_HANDLE)Adapter, pNdisCommon->PendedRequest));	
			}
			else
			{
				PlatformReleaseSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
			}
		}
		else if(pDevice->CurrentPowerState > NdisDeviceStateD0)
		{// Sleep
		
			N6CompletePendedOID(
				Adapter,
				RT_PENDED_OID_PNP,
				NDIS_STATUS_SUCCESS);
		}
		else
		{
			RT_TRACE(COMP_POWER, DBG_WARNING, ("PnpWorkItemCallback(): unexpected CurrentPowerState: %#X\n", pDevice->CurrentPowerState));
		}
	}
	else
	{// Complete pended OID.
		N6CompletePendedOID(Adapter, RT_PENDED_OID_PNP, NDIS_STATUS_SUCCESS);
	}

	Adapter->bInSetPower = FALSE;	
	pMgntInfo->bSetPnpPwrInProgress = FALSE;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("<----- PnpWorkItemCallback() return ndis_status_success\n"));
}

#endif //power_man


//
// 	Description:
// 		Fill up our capability about power management.
//
VOID
FillPnpCapabilities(
	IN PADAPTER							pAdapter,
	IN OUT PNDIS_PNP_CAPABILITIES		pPnpCapabilities
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(pAdapter);
	BOOLEAN			bSupportRemoteWakeUp;
	PMGNT_INFO  		pMgntInfo = &pAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);
	PlatformZeroMemory(pPnpCapabilities, sizeof(NDIS_PNP_CAPABILITIES));

#if POWER_MAN
	if(bSupportRemoteWakeUp)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("FillPnpCapabilities(): support remote wake up\n"));

		//
		// Report we support WOL feature.
		//
		pPnpCapabilities->Flags = NDIS_DEVICE_WAKE_UP_ENABLE;

		//
		// Set type of WOL supported.
		//
		pPnpCapabilities->WakeUpCapabilities.MinMagicPacketWakeUp   = NdisDeviceStateD3; // 070201, rcnjko: magic packet tought me a lesson, we should call one's name 16 times to awake him. So, be patient.
		pPnpCapabilities->WakeUpCapabilities.MinPatternWakeUp       = NdisDeviceStateD3;
		pPnpCapabilities->WakeUpCapabilities.MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;
		if(pPSC->WoWLANMode == eWakeOnMagicPacketOnly)	//by tynli.
			pPnpCapabilities->WakeUpCapabilities.MinPatternWakeUp       = NdisDeviceStateUnspecified;
		else if (pPSC->WoWLANMode == eWakeOnPatternMatchOnly)	
			pPnpCapabilities->WakeUpCapabilities.MinMagicPacketWakeUp	= NdisDeviceStateUnspecified;

	}
	else
#endif
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("FillPnpCapabilities(): NOT support remote wake up\n"));

		//
		// Not support WOL
		//
		pPnpCapabilities->WakeUpCapabilities.MinMagicPacketWakeUp   = NdisDeviceStateUnspecified;
		pPnpCapabilities->WakeUpCapabilities.MinPatternWakeUp       = NdisDeviceStateUnspecified;
		pPnpCapabilities->WakeUpCapabilities.MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;
	}

}


//
// 	Description:
// 		Fill up the capabilities about power management for NDIS 6.2 and later version. Added by tynli. 2009.06.05
//
VOID
FillPmCapabilities(
	IN PADAPTER							pAdapter,
	IN OUT PNDIS_PM_CAPABILITIES		pPmCapabilities
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(pAdapter);
	BOOLEAN				bEnableWoLCapabilities;
	PMGNT_INFO  		pMgntInfo = &pAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	bEnableWoLCapabilities = MgntIsWoWLANCapabilityEnable(pAdapter);
	PlatformZeroMemory(pPmCapabilities, sizeof(NDIS_PM_CAPABILITIES));
	
#if NDIS_SUPPORT_NDIS630
	pPmCapabilities->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	pPmCapabilities->Header.Revision = NDIS_PM_CAPABILITIES_REVISION_2;
	pPmCapabilities->Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;
#else
	pPmCapabilities->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	pPmCapabilities->Header.Revision = NDIS_PM_CAPABILITIES_REVISION_1;
	pPmCapabilities->Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1;
#endif

#if POWER_MAN
	if(bEnableWoLCapabilities)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, 
			("FillPmCapabilities(): support Win7 wake-on-lan (%d)\n", pPSC->WoWLANMode));

		//
		// Report we support WOL feature.
		//
#if NDIS_SUPPORT_NDIS630
		pPmCapabilities->Flags = NDIS_PM_WAKE_PACKET_INDICATION_SUPPORTED;// 0;
#else
		pPmCapabilities->Flags =  0;
#endif
		// We should not enable NDIS_PM_WOL_EAPOL_REQUEST_ID_MESSAGE_SUPPORTED
		// when supporting GTK rekey offload or the upper layer will not send GTK offload OID to our driver.
		// 2012.02.22. by tynli.
		pPmCapabilities->SupportedWoLPacketPatterns = 
						(NDIS_PM_WOL_MAGIC_PACKET_SUPPORTED|
						NDIS_PM_WOL_BITMAP_PATTERN_SUPPORTED|
						NDIS_PM_WOL_EAPOL_REQUEST_ID_MESSAGE_SUPPORTED|
						NDIS_PM_WOL_IPV4_TCP_SYN_SUPPORTED|	
						NDIS_PM_WOL_IPV6_TCP_SYN_SUPPORTED|						
						NDIS_PM_WOL_IPV4_DEST_ADDR_WILDCARD_SUPPORTED|						
						NDIS_PM_WOL_IPV6_DEST_ADDR_WILDCARD_SUPPORTED);
		pPmCapabilities->NumTotalWoLPatterns = MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter);
		pPmCapabilities->MaxWoLPatternSize = MAX_WOL_PATTERN_SIZE;
		pPmCapabilities->MaxWoLPatternOffset = 256;
		pPmCapabilities->MaxWoLPacketSaveBuffer = (MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter))*MAX_WOL_PATTERN_SIZE;
		pPmCapabilities->SupportedProtocolOffloads = 0;
		pPmCapabilities->NumArpOffloadIPv4Addresses = 0;
		pPmCapabilities->NumNSOffloadIPv6Addresses = 0; // WHQL requirment at least 2
		if(pPSC->RegARPOffloadEnable)
		{
			pPmCapabilities->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_ARP_SUPPORTED;
			pPmCapabilities->NumArpOffloadIPv4Addresses = 1;
		}
		if(pPSC->RegNSOffloadEnable)
		{
			pPmCapabilities->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_NS_SUPPORTED;
			pPmCapabilities->NumNSOffloadIPv6Addresses =2;
		}
		if(pPSC->RegGTKOffloadEnable)
			pPmCapabilities->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_SUPPORTED;

		//
		// Set type of WOL supported.
		//
		pPmCapabilities->MinMagicPacketWakeUp	= NdisDeviceStateD3; // 070201, rcnjko: magic packet tought me a lesson, we should call one's name 16 times to awake him. So, be patient.
		pPmCapabilities->MinPatternWakeUp		= NdisDeviceStateD3;
		pPmCapabilities->MinLinkChangeWakeUp	= NdisDeviceStateUnspecified;
		if(pPSC->WoWLANMode == eWakeOnMagicPacketOnly)	//by tynli.
			pPmCapabilities->MinPatternWakeUp		= NdisDeviceStateUnspecified;
		else if (pPSC->WoWLANMode == eWakeOnPatternMatchOnly)	
			pPmCapabilities->MinMagicPacketWakeUp	= NdisDeviceStateUnspecified;
	
		//
		// Set new attributes for Win8
		//
#if NDIS_SUPPORT_NDIS630
		// Temporarily mark some attributes because we cannot support now. 
		// They will be enable in the feature. 2012.03.12. by tynli.
		pPmCapabilities->SupportedWakeUpEvents = 0
			/*NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_SUPPORTED |
			NDIS_PM_WAKE_ON_MEDIA_CONNECT_SUPPORTED*/;
		pPmCapabilities->MediaSpecificWakeUpEvents = /*0 |*/
			NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_SUPPORTED |
			NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_SUPPORTED |
			NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_SUPPORTED |
			NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_SUPPORTED;
#endif
	}
	else
#endif
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("FillPmCapabilities(): NOT support Win7 wake-on-lan\n"));

		//
		// Not support WOL
		//
		pPmCapabilities->MinMagicPacketWakeUp   = NdisDeviceStateUnspecified;
		pPmCapabilities->MinPatternWakeUp       = NdisDeviceStateUnspecified;
		pPmCapabilities->MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;
	}
}



//
// 	Description:
// 		Allocate N6 Usb related resources(e.g., Fix InOut pipe fix workitems).
//		
//	Created by Roger, 2009.04.30.
//
VOID
N6SdioAllocRes(
	IN	PADAPTER	Adapter
	)
{

	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);
		
#if POWER_MAN
	PlatformInitializeWorkItem(
		Adapter,
		&(device->PnpWorkItem), 
		(RT_WORKITEM_CALL_BACK)PnpWorkItemCallback,
		(PVOID)Adapter,
		"PnpWorkItem");
#endif

}

//
// 	Description:
// 		Free N6 Usb related resources(e.g., Fix InOut pipe fix workitems).
//		
//	Created by Roger, 2009.04.30.
//
VOID
N6SdioFreeRes(
	IN	PADAPTER	Adapter
	)
{
	PRT_SDIO_DEVICE	device = GET_RT_SDIO_DEVICE(Adapter);
	
	PlatformFreeWorkItem( &(device->PnpWorkItem));
}


//
//	Description:
//		Cancel IRP pending to of specified Tx Queue and 
//		wait until all outstanding request completed.
//
VOID
N6SdioStopTxQueue(
	IN	PADAPTER		pAdapter,
	IN	int				nTxQueueIdx
	)
{
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(pAdapter);
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;

	RT_ASSERT(nTxQueueIdx >= 0 && nTxQueueIdx < pDevice->RtNumTxQueue, 
		("RTsdioStartTxQueue(): nTxQueueIdx(%d) shall falls in [0,%d)\n",
		nTxQueueIdx, pDevice->RtNumTxQueue));
	
	pTxQueue = &(pDevice->RtTxQueue[nTxQueueIdx]);

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(!pTxQueue->bEnabled)
	{
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
		return;	
	}
	else
	{
		pTxQueue->bEnabled = FALSE; // Mark the pipe as disabled
	}

	if(pTxQueue->IrpPendingCount > 0)
		pTxQueue->IrpPendingCount--; // Decrease 1 for RTusbSelectConfiguration() had initialized it to 1.
	else
		RT_ASSERT(FALSE, ("IrpPendingCount is 0 when TxQueue is Enable. Abnormal!!\n"));

	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

}



//
//	Description:
//		Mark the Tx Queue as enabled state.
//
VOID
N6SdioStartTxQueue(
	IN	PADAPTER		pAdapter,
	IN	int				nTxQueueIdx
	)
{
	PRT_SDIO_DEVICE		pDevice = GET_RT_SDIO_DEVICE(pAdapter);
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;

	RT_ASSERT(nTxQueueIdx >= 0 && nTxQueueIdx < pDevice->RtNumTxQueue, 
		("RTsdioStartTxQueue(): nTxQueueIdx(%d) shall falls in [0,%d)\n",
		nTxQueueIdx, pDevice->RtNumTxQueue));
	
	pTxQueue = &(pDevice->RtTxQueue[nTxQueueIdx]);

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(!pTxQueue->bEnabled)
	{
		pTxQueue->bEnabled = TRUE;
		pTxQueue->IrpPendingCount++;
	}

	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}


//
// 	Description:
// 		Query current SD bus operating clock after resumption especially for the case of FSS 
//	mode and configure corresponding clock rate to fix mismatched phase modulation for 
//	SD request packets
//		
//	Created by Roger, 2015.03.24.
//
RT_STATUS
N6SdioSyncCurrentSdBusClk(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	RT_STATUS	status = RT_STATUS_FAILURE;
	NTSTATUS	ntstatus;
	u1Byte 		Data=0;
	u4Byte		SdioBusClk=0;

	
	ntstatus = PlatformSdioGetProperty(pDevice,
			   			SDP_BUS_CLOCK,
			                	&SdioBusClk,
			                	sizeof(SdioBusClk)); 
	
	if(!NT_SUCCESS(ntstatus)){ 
		return status;
	}	

	// Configuration for SDIO v2.0
	Data |= ((SdioBusClk == 50000) ? BIT1:0); // Set BIT1 for 50MHz operation clock
	status = PlatformSdioCmd52ReadWrite(
								pDevice, 
								SDIO_WITHOUT_REF_DEVICE_ID, //Device ID
								0, //Function Number
								1, //Byte Count
								(ULONG)0x13, //Offset
								TRUE, //Write operation
								&Data);

	if(status != RT_STATUS_SUCCESS){ 
		return status;
	}
	
	return RT_STATUS_SUCCESS;
}

//
// 	Description:
//		Dummy I/O is to distingush who sends the Bus I/O. (Driver? Host?) Those 
//	I/O before this Dummy I/O are sent by Host, and those after it are sent by
//	driver.
//
//	Created by TzuHang, 2015.04.08.
//
VOID
N6SdioDummyIO(
	IN PRT_SDIO_DEVICE	pDevice
)
{
	RT_STATUS	status	= RT_STATUS_FAILURE;
	u1Byte 		Data	= 0;

	//	Dummy I/O
	//
	Data = 0x10; // Dummy
	status = PlatformSdioCmd52ReadWrite(
							pDevice, 
							SDIO_LOCAL_DEVICE_ID, //Device ID
							1, //Function Number
							1, //Byte Count
							(ULONG)0x0, //Offset
							TRUE, //Write operation
							&Data);
		
	if(status != RT_STATUS_SUCCESS){ 
		RT_TRACE_F(COMP_INIT, DBG_LOUD,("Write SDIO Dummy I/O FAIL!!\n"));
	}	
}
