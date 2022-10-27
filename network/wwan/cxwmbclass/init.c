//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "precomp.h"
#include "init.tmh"
#include "WMBClassTelemetry.h"

MINIPORT_DRIVER_CONTEXT GlobalControl = {0};

const NDIS_OID WmbClassSupportedOids[] =
{

    OID_GEN_SUPPORTED_LIST,
    OID_GEN_MEDIA_IN_USE, 
    OID_GEN_MAXIMUM_LOOKAHEAD, 
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_PHYSICAL_MEDIUM,
    OID_GEN_STATISTICS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_PM_PARAMETERS,
    OID_PM_ADD_WOL_PATTERN,
    OID_PM_REMOVE_WOL_PATTERN,
    OID_WWAN_DRIVER_CAPS,
    OID_WWAN_DEVICE_CAPS,
    OID_WWAN_READY_INFO,
    OID_WWAN_RADIO_STATE,
    OID_WWAN_PIN_EX,
    OID_WWAN_PIN_LIST,
    OID_WWAN_HOME_PROVIDER,
    OID_WWAN_PREFERRED_PROVIDERS,
    OID_WWAN_VISIBLE_PROVIDERS,
    OID_WWAN_REGISTER_STATE,
    OID_WWAN_PACKET_SERVICE,
    OID_WWAN_SIGNAL_STATE,
    OID_WWAN_CONNECT,
    OID_WWAN_PROVISIONED_CONTEXTS,
    OID_WWAN_SERVICE_ACTIVATION,
    OID_WWAN_SMS_CONFIGURATION,
    OID_WWAN_SMS_READ,
    OID_WWAN_SMS_SEND,
    OID_WWAN_SMS_DELETE,
    OID_WWAN_SMS_STATUS,
    OID_WWAN_VENDOR_SPECIFIC,
    OID_WWAN_USSD,
    OID_WWAN_AUTH_CHALLENGE,
    OID_WWAN_ENUMERATE_DEVICE_SERVICES,
    OID_WWAN_DEVICE_SERVICE_COMMAND,
    OID_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS,
    OID_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS,
    OID_WWAN_DEVICE_SERVICE_SESSION,
    OID_WWAN_DEVICE_SERVICE_SESSION_WRITE,
    OID_WWAN_PREFERRED_MULTICARRIER_PROVIDERS,    
    OID_WWAN_CREATE_MAC,
    OID_WWAN_DELETE_MAC,
    OID_WWAN_UICC_FILE_STATUS,
    OID_WWAN_UICC_ACCESS_BINARY,
    OID_WWAN_UICC_ACCESS_RECORD,
    OID_WWAN_PIN_EX2,
    OID_WWAN_MBIM_VERSION,
    OID_WWAN_SYS_CAPS,
    OID_WWAN_DEVICE_CAPS_EX,
    OID_WWAN_SYS_SLOTMAPPINGS,
    OID_WWAN_SLOT_INFO_STATUS,
    OID_WWAN_DEVICE_BINDINGS,
    OID_WWAN_REGISTER_STATE_EX,
    OID_WWAN_IMS_VOICE_STATE,
    OID_WWAN_SIGNAL_STATE_EX,
    OID_WWAN_LOCATION_STATE,
    OID_WWAN_NITZ,
    OID_WWAN_NETWORK_IDLE_HINT,
    OID_WWAN_PRESHUTDOWN,
    OID_WWAN_UICC_ATR,
    OID_WWAN_UICC_OPEN_CHANNEL,
    OID_WWAN_UICC_CLOSE_CHANNEL,
    OID_WWAN_UICC_APDU,
    OID_WWAN_UICC_TERMINAL_CAPABILITY,
    OID_WWAN_SAR_CONFIG,
    OID_WWAN_SAR_TRANSMISSION_STATUS,
    OID_WWAN_LTE_ATTACH_CONFIG,
    OID_WWAN_LTE_ATTACH_STATUS,
    OID_WWAN_NETWORK_BLACKLIST,
    OID_WWAN_PCO,
    OID_WWAN_UICC_RESET,
    OID_WWAN_DEVICE_RESET,
    OID_WWAN_BASE_STATIONS_INFO,
    OID_WWAN_UICC_APP_LIST,
};

const UINT WmbClassSupportedOidsNum = sizeof(WmbClassSupportedOids) / sizeof(NDIS_OID);

//volatile ULONG MbbGlobalTestFlags = MbbTestFlagNone;

DRIVER_INITIALIZE DriverEntry;

NDIS_STATUS
InitializeGeneralAttributes(
    __in PMINIPORT_ADAPTER_CONTEXT                 Adapter,
    __in PNDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES GeneralAttributes,
    __out PNDIS_PM_CAPABILITIES                     PnpCaps
    );

MINIPORT_UNLOAD      MbbNdisDriverUnload;

MINIPORT_INITIALIZE  MbbNdisMiniportInitialize;

MINIPORT_SHUTDOWN    MbbNdisMiniportShutdownEx;

MINIPORT_HALT        MbbNdisMiniportHaltEx;

MINIPORT_SET_OPTIONS MbbNdisMiniportSetOption;

MINIPORT_PAUSE       MbbNdisMiniportPause;

MINIPORT_RESTART     MbbNdisMiniportRestart;

MINIPORT_DEVICE_PNP_EVENT_NOTIFY MbbNdisMiniportDevicePnPEventNotify;

MINIPORT_RESET       MbbNdisMiniportResetEx;

MINIPORT_CHECK_FOR_HANG  MbbNdisMiniportCheckForHangEx;

MINIPORT_IDLE_NOTIFICATION MbbNdisMiniportIdleNotification;

MINIPORT_CANCEL_IDLE_NOTIFICATION MbbNdisMiniportCancelIdleNotification;

NTSTATUS
SendOpenCommand(
    PMINIPORT_ADAPTER_CONTEXT                 Adapter
    );

NTSTATUS
SendCloseCommand(
    PMINIPORT_ADAPTER_CONTEXT                 Adapter
    );

VOID
MbbNdisResponseFragmentAvailable(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle
    );


NDIS_STATUS 
MbbNdisAdapterReadParameters(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    );

VOID
AdapterPauseHandler(
    PSTATE_CHANGE_EVENT             StateChange
    );

VOID
AdapterRestartHandler(
    PSTATE_CHANGE_EVENT             StateChange
    );


VOID
AdapterPowerHandler(
    PSTATE_CHANGE_EVENT             StateChange
    );

VOID
AdapterResetHandler(
    PSTATE_CHANGE_EVENT             StateChange
    );


VOID
MbbNdisMiniportBusIdleConfirm(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    DEVICE_POWER_STATE      PowerState
    );

VOID
MbbNdisMiniportIdleNotificationComplete(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    NTSTATUS                Status
    );

VOID
MbbSystemPowerNotification(
    _In_ NDIS_HANDLE                          MiniportAdapterContext,
    _In_ NDIS_MINIPORT_SYSPOWER_NOTIFY const *SystemPowerNotification
    );

void 
NTAPI MbbDumpAdapterState(
  _In_      LPCGUID SourceId,
  _In_      ULONG IsEnabled,
  _In_      UCHAR Level,
  _In_      ULONGLONG MatchAnyKeyword,
  _In_      ULONGLONG MatchAllKeywords,
  _In_opt_  PEVENT_FILTER_DESCRIPTOR FilterData,
  _In_opt_  PVOID CallbackContext
);

#pragma alloc_text(PAGE, MbbNdisDriverUnload)
#pragma alloc_text(PAGE, MbbNdisMiniportHaltEx)
#pragma alloc_text(PAGE, MbbNdisMiniportPause)
#pragma alloc_text(PAGE, MbbNdisMiniportRestart)
#pragma alloc_text(PAGE, MbbNdisMiniportDevicePnPEventNotify)
#pragma alloc_text(PAGE, MbbNdisMiniportInitialize)
#pragma alloc_text(PAGE, InitializeGeneralAttributes)
#pragma alloc_text(PAGE, DriverEntry)

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:

    In the context of its DriverEntry function, a miniport driver associates
    itself with NDIS, specifies the NDIS version that it is using, and
    registers its entry points.


Arguments:
    PVOID DriverObject - pointer to the driver object.
    PVOID RegistryPath - pointer to the driver registry path.

    Return Value:

    NDIS_STATUS_xxx code

--*/
{
    NDIS_STATUS                             Status = STATUS_UNSUCCESSFUL;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS    Characteristics;
    WDF_DRIVER_CONFIG                       config;
    NTSTATUS                                ntStatus;

    PAGED_CODE();

    do
    {
        WPP_INIT_TRACING(DriverObject,RegistryPath);
        TraceLoggingRegister(g_hLoggingProvider);
        TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

        WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

        InitializeListHead( &GlobalControl.AdapterList );
        NdisAllocateSpinLock( &GlobalControl.AdapterListLock );

        ntStatus=EtwRegister(&WmbclassServiceTrace, MbbDumpAdapterState, NULL, &GlobalControl.TraceHandle);

        if (!NT_SUCCESS(ntStatus)){

            TraceError(WMBCLASS_INIT, "EtwRegister failed %08lx", ntStatus);
            Status = ntStatus;
            break;
        }

        MbbWriteEvent(
            &WMBCLASS_DRIVER_LOAD,
            NULL,
            NULL,
            1,
            &RegistryPath->Buffer,
            RegistryPath->Length
            );

        ntStatus=EtwRegister(&WmbclassServiceTraceOpn, NULL, NULL, &GlobalControl.TraceHandleOpn);

        if (!NT_SUCCESS(ntStatus)){

            TraceError(WMBCLASS_INIT, "EtwRegister(Opn) failed %08lx", ntStatus);
            //
            // Don't fail the driver load if OPN trace handle cannot be registered
            //
        }

        //
        // Set WdfDriverInitNoDispatchOverride flag to tell the framework
        // not to provide dispatch routines for the driver. In other words,
        // the framework must not intercept IRPs that the I/O manager has
        // directed to the driver. In this case, it will be handled by NDIS
        // port driver.
        //
        config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;

        ntStatus = WdfDriverCreate(DriverObject,
                                   RegistryPath,
                                   WDF_NO_OBJECT_ATTRIBUTES,
                                   &config,                
                                   &GlobalControl.hDriver); //vm control

        if (!NT_SUCCESS(ntStatus)){

            TraceError(WMBCLASS_INIT, "WdfDriverCreate failed %08lx", ntStatus);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Fill in the Miniport characteristics structure with the version numbers
        // and the entry points for driver-supplied MiniportXxx
        //
        NdisZeroMemory(&Characteristics, sizeof(Characteristics));


        Characteristics.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS,
        Characteristics.Header.Size = sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS);
        Characteristics.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_3;
        Characteristics.MajorNdisVersion = 6;
        Characteristics.MinorNdisVersion = 80;
        Characteristics.MajorDriverVersion  = 1;
        Characteristics.MinorDriverVersion  = 0;

        //
        // Init/PnP handlers
        //
        Characteristics.UnloadHandler            = MbbNdisDriverUnload;

        Characteristics.InitializeHandlerEx      = MbbNdisMiniportInitialize;
        Characteristics.HaltHandlerEx            = MbbNdisMiniportHaltEx;

        Characteristics.RestartHandler           = MbbNdisMiniportRestart;
        Characteristics.PauseHandler             = MbbNdisMiniportPause;

        Characteristics.DevicePnPEventNotifyHandler  = MbbNdisMiniportDevicePnPEventNotify;

        Characteristics.ShutdownHandlerEx        = MbbNdisMiniportShutdownEx;

        Characteristics.SetOptionsHandler        = MbbNdisMiniportSetOption;

#if 0
        //
        //  reset
        //
        Characteristics.ResetHandlerEx           = MbbNdisMiniportResetEx;
        Characteristics.CheckForHangHandlerEx    = MbbNdisMiniportCheckForHangEx;

#endif
        //
        // Query/Set/Method requests handlers
        //
        Characteristics.OidRequestHandler        = MbbNdisMiniportOidRequest;
        Characteristics.CancelOidRequestHandler  = MbbNdisMiniportCancelOidRequest;

        //
        // Send/Receive handlers
        //
        Characteristics.SendNetBufferListsHandler    = MbbNdisMiniportSendNetBufferLists;
        Characteristics.ReturnNetBufferListsHandler  = MbbNdisMiniportReturnNetBufferLists;
        Characteristics.CancelSendHandler            = MbbNdisMiniportCancelSend;

        Characteristics.Flags = NDIS_WDM_DRIVER;

        TraceVerbose(WMBCLASS_INIT, "Calling NdisMRegisterMiniportDriver...");
        //
        // Register with IP Helper for IP change notifications.
        //
        ntStatus = MbbIpInitialize( &GlobalControl );
        if( ntStatus != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[Init] FAILED to initialize IP subsystem, status=%!STATUS!", ntStatus );
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        //
        // Registers miniport's entry points with the NDIS library as the first
        // step in NIC driver initialization. The NDIS will call the
        // MiniportInitialize when the device is actually started by the PNP
        // manager.
        //
        if( (Status = NdisMRegisterMiniportDriver(
                            DriverObject,
                            RegistryPath,
                            (PNDIS_HANDLE)&GlobalControl,
                            &Characteristics,
                            &GlobalControl.NdisDriverHandle
                            )) != NDIS_STATUS_SUCCESS )
        {
            TraceError(WMBCLASS_INIT, "NdisMRegisterMiniportDriver() failed %08lx", Status);
            break;
        }
    }
    while( FALSE );

    if( NT_ERROR(Status) )
    {
        MbbIpCleanup( &GlobalControl );

        if (GlobalControl.TraceHandle != 0)
        {
            EtwUnregister(GlobalControl.TraceHandle);
            TraceLoggingUnregister(g_hLoggingProvider);
            GlobalControl.TraceHandle=0;
        }

        if (GlobalControl.TraceHandleOpn != 0)
        {
            EtwUnregister(GlobalControl.TraceHandleOpn);
            GlobalControl.TraceHandleOpn=0;
        }

        WPP_CLEANUP(DriverObject);
    }

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return Status;
}

VOID
MbbNdisDriverUnload(
    __in  PDRIVER_OBJECT  DriverObject
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
    PAGED_CODE();

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    UNREFERENCED_PARAMETER(DriverObject);

    MbbIpCleanup( &GlobalControl );

    WdfDriverMiniportUnload(WdfGetDriver());

    NdisMDeregisterMiniportDriver(GlobalControl.NdisDriverHandle);

    if (GlobalControl.TraceHandle != 0)
    {
        EtwUnregister(GlobalControl.TraceHandle);
        TraceLoggingUnregister(g_hLoggingProvider);
        GlobalControl.TraceHandle=0;
    }

    if (GlobalControl.TraceHandleOpn != 0)
    {
        EtwUnregister(GlobalControl.TraceHandleOpn);
        GlobalControl.TraceHandleOpn=0;
    }

    ASSERT( IsListEmpty( &GlobalControl.AdapterList ) );
    NdisFreeSpinLock( &GlobalControl.AdapterListLock );

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");
    WPP_CLEANUP(DriverObject);
}

NDIS_STATUS
MbbNdisMiniportInitialize(
    __in  NDIS_HANDLE                        MiniportAdapterHandle,
    __in  NDIS_HANDLE                        MiniportDriverContext,
    __in  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
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
    NDIS_STATUS                         Status = NDIS_STATUS_SUCCESS;
    BOOLEAN                             SendQueueInitialized = FALSE;
    BOOLEAN                             RecvQueueInitialized = FALSE;
    NDIS_MINIPORT_ADAPTER_ATTRIBUTES    MiniportAttributes;
    NDIS_MINIPORT_SS_CHARACTERISTICS    SelectiveSuspend;
    PMINIPORT_ADAPTER_CONTEXT           Adapter=NULL;
    ULONG_PTR                           ModelNameLength=0;
    ULONG                               TempInstance=0;
    NDIS_PM_CAPABILITIES                PnpCaps;
    int                                 i = 0;
    PMBB_PORT                           DefaultPort = NULL;

    PAGED_CODE();

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocates a miniport adapter attributes structure, released in MbbNdisMiniportHaltEx.")
    Adapter=ALLOCATE_NONPAGED_POOL(sizeof(*Adapter));

    if (Adapter == NULL)
    {

        TraceError(WMBCLASS_INIT, "[Init] FAILED to allocate adapter context");
        Status=NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    NdisZeroMemory(Adapter,sizeof(*Adapter));

    //
    //  init elements of the adapter structure
    //
    NdisAllocateSpinLock(&Adapter->Lock);

    Adapter->TraceInstance=InterlockedIncrement(&GlobalControl.CurrentTraceInstance);
    TempInstance=Adapter->TraceInstance;

    Adapter->MiniportAdapterHandle = MiniportAdapterHandle;

    Adapter->IfIndex=MiniportInitParameters->IfIndex;
    Adapter->NetLuid=MiniportInitParameters->NetLuid;   
    Adapter->MaxActivatedContexts = 1; // initialize to 1.OID_WWAN_DRIVER_CAPS should return the correct value.
    Adapter->SurpriseRemoved = FALSE;
    
    RtlInitAnsiString(&Adapter->FriendlyName, NULL);    
    
    NdisAllocateSpinLock(&Adapter->PortsLock);
  
    NdisAllocateSpinLock(&Adapter->SessionIdPortTableLock);

    //Initialize all session Ids as available
    for(i=0; i < ARRAYSIZE(Adapter->SessionIdPortTable); i++)
    {
        Adapter->SessionIdPortTable[i].InUse = FALSE;
        Adapter->SessionIdPortTable[i].PortNumber = (ULONG)MBB_INVALID_PORT_NUMBER;
    }

    MbbWriteEvent(
        &INITIALIZE_EVENT,
        NULL,
        NULL,
        1,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance)
        );
    
    //
    //  object managed state of adapter while processing power and pnp events
    //
    Status=InitAdapterStateObject(
        &Adapter->AdapterState,
        Adapter->MiniportAdapterHandle,
        AdapterPauseHandler,
        AdapterRestartHandler,
        AdapterPowerHandler,
        AdapterResetHandler,
        AdapterStallClearHandler
        );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "InitAdapterStateObject with %!STATUS!", Status);
        goto Cleanup;

    }


    Status=GetDeviceFriendlyName( MiniportAdapterHandle, &Adapter->FriendlyName);

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] GetDeviceFriendlyName with %!STATUS!", Status);
        goto Cleanup;

    }

    NdisMGetDeviceProperty(
        MiniportAdapterHandle,
        &Adapter->Pdo,
        &Adapter->Fdo,
        &Adapter->NextDeviceObject,
        NULL,
        NULL
        );


    Status=MbbBusInitialize(
        Adapter->Pdo,
        Adapter->Fdo,
        Adapter->NextDeviceObject,
        MbbNdisResponseFragmentAvailable,
        MbbNdisReceiveCallback,
        MbbNdisMiniportBusIdleConfirm,
        MbbNdisMiniportIdleNotificationComplete,
        Adapter,
        GlobalControl.TraceHandle,
        Adapter->TraceInstance,
        &Adapter->BusHandle
        );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] MbbBusInitialize with %!STATUS!", Status);
        goto Cleanup;

    }
    //
    // Initialize the OID/CID handlers before the device is OPENED.
    // Once the device is opened and the interrupt pipe is connected
    // ReqMgr will start receiving unsolicited indications from the device.
    //
    Status = MbbBusQueryBusParameters(
                Adapter->BusHandle,
                &Adapter->BusParams
                );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] MbbBusQueryBusParameters() with %!STATUS!", Status);
        goto Cleanup;
    }

    if( (Status = MbbReqMgrInitialize(
                    &(Adapter->RequestManagerAllocation),
                    Adapter
                    )) != NDIS_STATUS_SUCCESS )
    {
        goto Cleanup;
    }
    MbbAdapterSetRequestManager(
        Adapter,
        &(Adapter->RequestManagerAllocation)
        );

    if (Adapter->BusParams.PowerFiltersSupported > 0)
    {
        ULONG i = 0;
        
        Adapter->PowerFilterTable=(PPOWER_FILTER_LOOKUP)ALLOCATE_NONPAGED_POOL(sizeof(POWER_FILTER_LOOKUP) * Adapter->BusParams.PowerFiltersSupported);

        if (Adapter->PowerFilterTable == NULL)
        {
            TraceError(USBBUS_ALL, "power filter table");
            Status=STATUS_NO_MEMORY;
            goto Cleanup;

        }

        RtlZeroMemory(Adapter->PowerFilterTable, sizeof(POWER_FILTER_LOOKUP) * Adapter->BusParams.PowerFiltersSupported);
        
        for (i = 0; i < Adapter->BusParams.PowerFiltersSupported; i++)
        {
            Adapter->PowerFilterTable[i].PortNumber = MBB_INVALID_PORT_NUMBER;          
        }        
    }


    Status=MbbBusOpen(Adapter->BusHandle,MbbLibraryGetNextTransactionId(), FastIOSendNetBufferListsComplete, FastIOIndicateReceiveNetBufferLists);

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] MbbBusOpen(1) with %!STATUS!", Status);

        MbbBusClose(Adapter->BusHandle,MbbLibraryGetNextTransactionId(), TRUE);

        Status=MbbBusOpen(Adapter->BusHandle,MbbLibraryGetNextTransactionId(), FastIOSendNetBufferListsComplete, FastIOIndicateReceiveNetBufferLists);

        if (NT_ERROR(Status))
        {
            TraceError(WMBCLASS_INIT, "[Init] MbbBusOpen(2) with %!STATUS!", Status);

            goto Cleanup;
        }
    }
    //
    // Set the miniport registration attributes with NDIS
    //
    NdisZeroMemory(&MiniportAttributes, sizeof(MiniportAttributes));
    MiniportAttributes.RegistrationAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
    MiniportAttributes.RegistrationAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
    MiniportAttributes.RegistrationAttributes.Header.Size = sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES);

    MiniportAttributes.RegistrationAttributes.MiniportAdapterContext = Adapter;
    MiniportAttributes.RegistrationAttributes.AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM |
                                                               NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK |
                                                               NDIS_MINIPORT_ATTRIBUTES_NO_PAUSE_ON_SUSPEND ;

    MiniportAttributes.RegistrationAttributes.CheckForHangTimeInSeconds = WMBCLASS_CHECK_FOR_HANG_INTERVAL;
    MiniportAttributes.RegistrationAttributes.InterfaceType = WMBCLASS_INTERFACE_TYPE;

    Status=NdisMSetMiniportAttributes(
        MiniportAdapterHandle,
        &MiniportAttributes
        );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] NdisMSetMiniportAttributes(registration) with Status=%!STATUS!", Status);
        goto Cleanup;

    }

    Status = MbbAdapterQueryDeviceId( Adapter );
    if( NT_ERROR( Status ) )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to query DeviceId with NdisStatus=%!STATUS!", Status );
        goto Cleanup;
    }

    Status=InitializeGeneralAttributes(Adapter,&MiniportAttributes.GeneralAttributes,&PnpCaps);

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] InitializeGeneralAttributes with Status=%!STATUS!", Status);
        goto Cleanup;

    }

    Status=NdisMSetMiniportAttributes(
        MiniportAdapterHandle,
        &MiniportAttributes
        );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "[Init] NdisMSetMiniportAttributes(general) with Status=%!STATUS!", Status);
        goto Cleanup;

    }


    //
    //  init the stat structure
    //
    Adapter->Stats.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
    Adapter->Stats.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
    Adapter->Stats.Header.Size     = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

    Adapter->Stats.SupportedStatistics=                         \
          NDIS_STATISTICS_XMIT_OK_SUPPORTED                     \
        | NDIS_STATISTICS_RCV_OK_SUPPORTED                      \
        | NDIS_STATISTICS_XMIT_ERROR_SUPPORTED                  \
        | NDIS_STATISTICS_RCV_ERROR_SUPPORTED                   \
        | NDIS_STATISTICS_RCV_NO_BUFFER_SUPPORTED               \
        | NDIS_STATISTICS_DIRECTED_BYTES_XMIT_SUPPORTED         \
        | NDIS_STATISTICS_DIRECTED_FRAMES_XMIT_SUPPORTED        \
        | NDIS_STATISTICS_DIRECTED_BYTES_RCV_SUPPORTED          \
        | NDIS_STATISTICS_DIRECTED_FRAMES_RCV_SUPPORTED         \
        | NDIS_STATISTICS_BYTES_RCV_SUPPORTED                   \
        | NDIS_STATISTICS_BYTES_XMIT_SUPPORTED                  \
        | NDIS_STATISTICS_RCV_DISCARDS_SUPPORTED                \
        | NDIS_STATISTICS_XMIT_DISCARDS_SUPPORTED;

    if( (Status = MbbNdisAdapterReadParameters( Adapter )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to read adapter parameters, status=%!STATUS!", Status );
        goto Cleanup;
    }

    Status=RtlStringCbLengthW(
        Adapter->BusParams.Model,
        sizeof(Adapter->BusParams.Model),
        &ModelNameLength
        );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "RtlStringCbLength with %!STATUS!", Status);
        goto Cleanup;

    }

    if (Adapter->OverrideDeviceName)
    {
        if (ModelNameLength > 0)
        {
            //
            //  Update the device name with the model from the USB descriptor
            //
            Status = IoSetDevicePropertyData(
                 Adapter->Pdo,
                 &DEVPKEY_Device_FriendlyName,
                 0,
                 PLUGPLAY_PROPERTY_PERSISTENT,
                 DEVPROP_TYPE_STRING,
                 (ULONG)ModelNameLength + sizeof(WCHAR),
                 Adapter->BusParams.Model
                 );

            if (NT_ERROR(Status))
            {
                TraceError(WMBCLASS_INIT, "IoSetDevicePropertyData with %!STATUS!", Status);

                Status=STATUS_SUCCESS;

            }
        }
    }
    //
    // Initialize the data queues.
    // They would not be active until the miniport is put in the started state.
    //
    if( (Status = MbbSendQInitialize(
                    &Adapter->SendQueue,
                    MAX_PENDING_SENDS,
                    DrainCompleteCallback,
                    Adapter,
                    Adapter,
                    Adapter->BusHandle,
                    Adapter->MiniportAdapterHandle
                    )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to initialize send queue, status=%!STATUS!", Status );
        goto Cleanup;
    }
    SendQueueInitialized = TRUE;

    if( (Status = MbbRecvQInitialize(
                    &Adapter->ReceiveQueue,
                    DrainCompleteCallback,
                    Adapter,
                    Adapter->MiniportAdapterHandle
                    )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to initialize receive queue, status=%!STATUS!", Status );
        goto Cleanup;
    }
    RecvQueueInitialized = TRUE;

    // Perform some requests that we need to do to get setup
    if( (Status = MbbAdapterPerformInitialRequests( Adapter )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_INIT, "[Init] FAILED to perform initial requests, status=%!STATUS!", Status );
        goto Cleanup;
    }

//#define TEST_CODE
#ifdef TEST_CODE
    MbbAdapterConfigureDeviceServiceSubscription(
        Adapter,
        FALSE,
        NDIS_WWAN_WAKE_ON_REGISTER_STATE_ENABLED     |
            NDIS_WWAN_WAKE_ON_SMS_RECEIVE_ENABLED    |
            NDIS_WWAN_WAKE_ON_USSD_RECEIVE_ENABLED   |
            NDIS_WWAN_WAKE_ON_PACKET_STATE_ENABLED   |
            NDIS_WWAN_WAKE_ON_UICC_CHANGE_ENABLED
        );

    MbbAdapterConfigureDeviceServiceSubscription(
        Adapter,
        TRUE,
        NDIS_WWAN_WAKE_ON_REGISTER_STATE_ENABLED     |
            NDIS_WWAN_WAKE_ON_SMS_RECEIVE_ENABLED    |
            NDIS_WWAN_WAKE_ON_USSD_RECEIVE_ENABLED   |
            NDIS_WWAN_WAKE_ON_PACKET_STATE_ENABLED   |
            NDIS_WWAN_WAKE_ON_UICC_CHANGE_ENABLED
        );

    {
        UCHAR   Mask=0xff;
        UCHAR   Pattern[8]={1,2,3,4,5,6,7,8};

        MbbUtilSetPowerFilterPattern(
            Adapter,
            MBB_DEFAULT_SESSION_ID,
            1,
            &Mask,
            sizeof(Mask),
            (PCUCHAR)&Pattern,
            sizeof(Pattern)
            );

        MbbAdapterConfigurePacketFilters(Adapter,TRUE);
    }
#endif

    MbbAdapterSetShutdownNotificationCapabilities( Adapter );

    TraceLoggingWrite(
        g_hLoggingProvider,
        "ShutdownCapability",
        TraceLoggingUInt64(Adapter->AdapterFlags.IsPreshutdownCapable, "IsPreshutdownCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.ShutdownNotificationCapable, "ShutdownNotificationCapable"),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

    MbbAdapterSetOptionalServiceSupport(Adapter);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: OptionalServiceCapability SAR %d NetworkBlacklist %d ProvisionedContextV2 %d LTEAttachConfig %d MultiSIM %d UiccLowLevel %d PCO %d DeviceReset %d BaseStationsInfo %d",
        Adapter->AdapterFlags.IsSARCapable, Adapter->AdapterFlags.IsNetworkBlacklistCapable, Adapter->AdapterFlags.IsProvisionedContextV2Capable,
        Adapter->AdapterFlags.IsLTEAttachConfigCapable, Adapter->AdapterFlags.IsMultiSIMCapable, Adapter->AdapterFlags.IsUiccLowLevelCapable,
        Adapter->AdapterFlags.IsPcoCapable, Adapter->AdapterFlags.IsDeviceResetCapable, Adapter->AdapterFlags.IsBaseStationsInfoCapable);

    TraceLoggingWrite(
        g_hLoggingProvider,
        "OptionalServiceCapability",
        TraceLoggingUInt64(Adapter->AdapterFlags.IsSARCapable, "IsSARCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsNetworkBlacklistCapable, "IsNetworkBlacklistCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsProvisionedContextV2Capable, "IsProvisionedContextV2Capable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsLTEAttachConfigCapable, "IsLTEAttachConfigCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsMultiSIMCapable, "IsMultiSIMCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsUiccLowLevelCapable, "IsUiccLowLevelCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsDeviceCapsV2Capable, "IsDeviceCapsV2Capable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsPcoCapable, "IsPcoCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsDeviceResetCapable, "IsDeviceResetCapable"),
        TraceLoggingUInt64(Adapter->AdapterFlags.IsBaseStationsInfoCapable, "IsBaseStationsInfoCapable"),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

    MbbDriverAddAdapter( &GlobalControl, Adapter );

    // Clear the adapter port list
    NdisZeroMemory(Adapter->PortList, sizeof(MBB_PORT*) * MBB_MAX_NUMBER_OF_PORTS);

    // Allocate the adapter port corresponding to default NDIS port ( port number 0). We are 
    // not managing port 0, however the adapter maintains a list of ports on which data/control traffic flows.
    // Instead of managing Port 0 separately, we will attach it to the list of ports maintained by the adapter itself. 

    Status = MbbWwanAllocatePort(Adapter, FALSE, &DefaultPort); // FALSE because we dont want to handle NDIS default port ourselves.

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "MbbWwanAllocatePort failed with %!STATUS!", Status);
        goto Cleanup;
    }

    ASSERT(DefaultPort->PortNumber == DEFAULT_NDIS_PORT_NUMBER);

    // Add the port to adapter ports list  
    MbbWwanAddPortToAdapterList(Adapter,&DefaultPort);

Cleanup:

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (Adapter != NULL)
        {
            PMBB_REQUEST_MANAGER RequestManager;
            PMBB_PORT            Port = NULL;

            if (Adapter->BusHandle != NULL)
            {
                MbbBusClose(Adapter->BusHandle,MbbLibraryGetNextTransactionId(), FALSE);
            }

            if( SendQueueInitialized == TRUE )
                MbbSendQCleanup( &Adapter->SendQueue );

            if( RecvQueueInitialized == TRUE )
                MbbRecvQCleanup( &Adapter->ReceiveQueue );

            if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) != NULL )
            {
                MbbReqMgrCleanup( RequestManager );
            }

            WaitStallClearComplete(&Adapter->AdapterState);

            if (Adapter->BusHandle != NULL)
            {
                MbbBusCleanup(Adapter->BusHandle);
                Adapter->BusHandle = NULL;
            }

            if((Port = Adapter->PortList[0]) != NULL) // if default port is allocated
            { 
                // remove it from the adapter port list
                MbbWwanRemovePortFromAdapterList(Adapter,Port);

                // Deallocate the port. It will get cleaned up when
                // its reference goes to 0
                MbbWwanDeallocatePort(Port);
            }
            
            FreeAdapterBlock(Adapter);
            Adapter=NULL;
        }

        TraceError(WMBCLASS_INIT, "[Init] miniport init failed with %08lx", Status);
    }

    MbbWriteEvent(
        &INITIALIZE_EXIT_EVENT,
        NULL,
        NULL,
        2,
        &TempInstance,
        sizeof(TempInstance),
        &Status,
        sizeof(Status)
        );

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return Status;

}

/****************************************************************************/
/*                          InitializeGeneralizeAttributes                  */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Queries the device for information and initializes the general          */
/*  attributes structure                                                    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - Pointer to the adapter structure                              */
/*  GeneralAttributes - Pointer to the general attributes structure         */
/*                      (Zeroed out by the caller)                          */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NDIS_STATUS_SUCCESS on succes, error code otherwise                    */
/*                                                                          */
/****************************************************************************/

NDIS_STATUS
InitializeGeneralAttributes(
    __in PMINIPORT_ADAPTER_CONTEXT                 Adapter,
    __in PNDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES GeneralAttributes,
    __out PNDIS_PM_CAPABILITIES                     PnpCaps
    )
{
    NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
    NDIS_STRING                     IfTypeStr = NDIS_STRING_CONST("*IfType");
    ULONG                           MediaState;
    DWORD                           MaxPowerFilterSize = 0;

    PAGED_CODE();

    GeneralAttributes->Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
    GeneralAttributes->Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;//6.2
    GeneralAttributes->Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;//6.2

    GeneralAttributes->MediaType = NdisMediumWirelessWan;
    GeneralAttributes->PhysicalMediumType = NdisPhysicalMediumWirelessWan;

    if (Adapter->BusParams.IsErrataDevice)
    {
       GeneralAttributes->MtuSize = Adapter->BusParams.MTU;
       GeneralAttributes->LookaheadSize = Adapter->BusParams.MTU;
    } else {
       GeneralAttributes->MtuSize = Adapter->BusParams.MaxSegmentSize;
       GeneralAttributes->LookaheadSize = Adapter->BusParams.MaxSegmentSize;
    }

    GeneralAttributes->MacAddressLength=0;
    //
    // Update the link speed in the adapter structure as well as
    // general attributes
    //

    Adapter->CurrentLinkSpeed = 0;
    Adapter->MaxLinkSpeed = 100L*1000*1000;

    GeneralAttributes->MaxXmitLinkSpeed  = Adapter->MaxLinkSpeed;
    GeneralAttributes->XmitLinkSpeed  = Adapter->CurrentLinkSpeed;

    GeneralAttributes->MaxRcvLinkSpeed  = Adapter->MaxLinkSpeed;
    GeneralAttributes->RcvLinkSpeed = Adapter->CurrentLinkSpeed;

    // Setting this to connected so that the filter driver state is also connected.
    // The media connect state corresponding to the NDIS ports handled by this miniport
    // will be indicated using NDIS_STATUS_PORT_STATE indication, which will be
    // converted to NDIS_STATUS_LINK_STATE indications by the filter driver.    
    GeneralAttributes->MediaConnectState = MediaConnectStateDisconnected;

    GeneralAttributes->MediaDuplexState  = MediaDuplexStateUnknown;

    RtlZeroMemory(PnpCaps,sizeof(*PnpCaps));

    PnpCaps->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
    PnpCaps->Header.Revision=NDIS_PM_CAPABILITIES_REVISION_2;
    PnpCaps->Header.Size=NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;

    PnpCaps->MediaSpecificWakeUpEvents = NDIS_WWAN_WAKE_ON_REGISTER_STATE_SUPPORTED |
                                         NDIS_WWAN_WAKE_ON_SMS_RECEIVE_SUPPORTED    |
                                         NDIS_WWAN_WAKE_ON_USSD_RECEIVE_SUPPORTED   |
                                         NDIS_WWAN_WAKE_ON_PACKET_STATE_SUPPORTED   |
                                         NDIS_WWAN_WAKE_ON_UICC_CHANGE_SUPPORTED;

    PnpCaps->MinLinkChangeWakeUp = NdisDeviceStateD2;

    PnpCaps->SupportedWakeUpEvents=NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_SUPPORTED;

    if (Adapter->BusParams.PowerFiltersSupported > 0)
    {
        PnpCaps->SupportedWoLPacketPatterns = NDIS_PM_WOL_BITMAP_PATTERN_SUPPORTED;
        PnpCaps->NumTotalWoLPatterns = Adapter->BusParams.PowerFiltersSupported;

        if (Adapter->BusParams.IsErrataDevice)
        {
            // For errata devices we will report the device specific max pattern size.
            // 192 bytes is the maximum power filter size as per the MBIM spec.
            // If a device reports more than 192 bytes as the power filter size, we
            // normalize it here to 192 bytes. This will prevent erroneous devices 
            // from indicating arbitrary pattern size. This needs to change with any
            // MBIM spec revision.
            
            if(Adapter->BusParams.MaxPowerFilterSize > WMBCLASS_MAX_MBIM_WOL_PATTERN)
            {
                MaxPowerFilterSize = WMBCLASS_MAX_MBIM_WOL_PATTERN;
            }
            else
            {
                MaxPowerFilterSize = Adapter->BusParams.MaxPowerFilterSize;
            }
        }
        else
        {
            // To maintain backward compatibility with Win8 devices, we continue
            // reporting 256 bytes as the wake pattern size.
            MaxPowerFilterSize = WMBCLASS_MAX_WOL_PATTERN;
        }

        PnpCaps->MaxWoLPatternSize = MaxPowerFilterSize;
        PnpCaps->MaxWoLPatternOffset = MaxPowerFilterSize;
        PnpCaps->MinPatternWakeUp = NdisDeviceStateD2;
    }

    if (Adapter->BusParams.SelectiveSuspendSupported)
    {
        PnpCaps->Flags |= NDIS_PM_SELECTIVE_SUSPEND_SUPPORTED;
    }

    GeneralAttributes->PowerManagementCapabilitiesEx = PnpCaps;

    GeneralAttributes->MacOptions = NDIS_MAC_OPTION_TRANSFERS_NOT_PEND | NDIS_MAC_OPTION_NO_LOOPBACK;

    GeneralAttributes->MaxMulticastListSize=0;

    //
    // Although MB device has no mac address DHCP requires a hardware id.
    //
    RtlCopyMemory(
        GeneralAttributes->CurrentMacAddress,
        Adapter->MACAddress,
        MBB_MAC_ADDRESS_LENGTH
        );
    RtlCopyMemory(
        GeneralAttributes->PermanentMacAddress,
        Adapter->MACAddress,
        MBB_MAC_ADDRESS_LENGTH
        );
    GeneralAttributes->MacAddressLength = MBB_MAC_ADDRESS_LENGTH;

    GeneralAttributes->IfType = IF_TYPE_WWANPP;

    GeneralAttributes->RecvScaleCapabilities = NULL;

    GeneralAttributes->AccessType = NET_IF_ACCESS_POINT_TO_POINT;
    GeneralAttributes->ConnectionType = NET_IF_CONNECTION_DEDICATED;
    GeneralAttributes->DirectionType = NET_IF_DIRECTION_SENDRECEIVE;
    GeneralAttributes->SupportedPacketFilters = NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_PROMISCUOUS;

    GeneralAttributes->IfConnectorPresent  = TRUE;

    GeneralAttributes->SupportedStatistics =
            NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV |
            NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
            NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS |
            NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR |
            NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT |
            NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
            NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR |
            NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_RCV |
            NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_XMIT ;

    GeneralAttributes->SupportedPauseFunctions = NdisPauseFunctionsUnknown;
    GeneralAttributes->DataBackFillSize = 0;
    GeneralAttributes->ContextBackFillSize = 0;

    GeneralAttributes->SupportedOidList = (PNDIS_OID)&WmbClassSupportedOids[0];
    GeneralAttributes->SupportedOidListLength = WmbClassSupportedOidsNum*sizeof(NDIS_OID);

    GeneralAttributes->AutoNegotiationFlags = 0;

    return Status;
}


VOID
MbbNdisMiniportShutdownEx(
    __in  NDIS_HANDLE MiniportAdapterContext,
    __in  NDIS_SHUTDOWN_ACTION ShutdownAction
    )

{
    UNREFERENCED_PARAMETER(ShutdownAction);

    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;

    TraceInfo(WMBCLASS_INIT, "[MbbAdapter] SHUTTING DOWN Adapter=0x%p", Adapter);
    
    MbbAdapterSendNetworkShutdownHint(Adapter);

    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        MbbBusShutdown(Adapter->BusHandle, ShutdownAction);
    }

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;
}

VOID
MbbNdisMiniportHaltEx(
    __in  NDIS_HANDLE MiniportAdapterContext,
    __in  NDIS_HALT_ACTION HaltAction
    )
/*++
Description:
    Close the MBIM protocol before cleaning up the request manager.
    Once MBIM CLOSE is sent the device should not queue any more notifications.
--*/
{
    PMINIPORT_ADAPTER_CONTEXT               Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    ULONG                                   TempInstance=Adapter->TraceInstance;
    NDIS_STATUS                             TempStatus=NDIS_STATUS_SUCCESS;
    PMBB_REQUEST_MANAGER                    RequestManager;
    PMBB_PORT                               Port = NULL;
    
    UNREFERENCED_PARAMETER(HaltAction);

    PAGED_CODE();

    TraceInfo(WMBCLASS_INIT, "[MbbAdapter] HALTING Adapter=0x%p", Adapter );

    MbbWriteEvent(
        &HALT_EVENT,
        NULL,
        NULL,
        1,
        &TempInstance,
        sizeof(TempInstance)
        );

    MbbSendQCleanup( &Adapter->SendQueue );
    MbbRecvQCleanup( &Adapter->ReceiveQueue );

    if (Adapter->BusHandle != NULL)
    {
        if (MbbBusIsFastIO(Adapter->BusHandle))
        {
            MbbBusHalt(Adapter->BusHandle, HaltAction);
        }
        MbbBusClose(Adapter->BusHandle,MbbLibraryGetNextTransactionId(), FALSE);
    }

    if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) != NULL )
    {
        MbbReqMgrCleanup( RequestManager );
    }
    else
    {
        TraceError(WMBCLASS_INIT, "[MbbAdapter] FAILED to reference RequestManager on  Adapter=0x%p for HALT", Adapter );
    }

    WaitStallClearComplete(&Adapter->AdapterState);

    if (Adapter->BusHandle != NULL)
    {
        MbbBusCleanup(Adapter->BusHandle);
        Adapter->BusHandle=NULL;
    }

    while ((Port = Adapter->PortList[0]) != NULL) // if default port is allocated
    {
        // remove it from the adapter port list
        if (MbbWwanRemovePortFromAdapterList(Adapter, Port))
        {
            TempStatus = MbbWwanDeactivatePort(Adapter, Port);
            if (TempStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_INIT, "MbbWwanDeactivatePort failed with status = 0x%x", TempStatus);
            }

            // Deallocate the port. It will get cleaned up when
            // its reference goes to 0
            MbbWwanDeallocatePort(Port);
        }
        else
        {
            TraceInfo(WMBCLASS_INIT, "%!FUNC!: Port(0x%p) was already removed", Port);
        }
    }

    ASSERT(Adapter->PortList[0] == NULL);

    MbbDriverRemoveAdapter( &GlobalControl, Adapter );

    TraceInfo(WMBCLASS_INIT, "[MbbAdapter] HALTED Adapter=0x%p", Adapter );

    MbbWriteEvent(
        &HALT_EXIT_EVENT,
        NULL,
        NULL,
        2,
        &TempInstance,
        sizeof(TempInstance),
        &TempStatus,
        sizeof(TempStatus)
        );

    return;
}

NDIS_STATUS
MbbNdisMiniportSetOption(
    __in  NDIS_HANDLE   NdisDriverHandle,
    __in  NDIS_HANDLE   DriverContext
    )

{
    NDIS_MINIPORT_SS_CHARACTERISTICS    SelectiveSuspendChars = {0};
    NDIS_MINIPORT_SYSPOWER_NOTIFY_CHARACTERISTICS  SysPowerChars = {0};
    NDIS_STATUS                         Status=NDIS_STATUS_SUCCESS;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    SelectiveSuspendChars.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_SS_CHARACTERISTICS;
    SelectiveSuspendChars.Header.Revision = NDIS_MINIPORT_SS_CHARACTERISTICS_REVISION_1;
    SelectiveSuspendChars.Header.Size = sizeof(SelectiveSuspendChars);

    //
    // Set the two function callbacks used by NDIS
    //
    SelectiveSuspendChars.IdleNotificationHandler       = &MbbNdisMiniportIdleNotification;
    SelectiveSuspendChars.CancelIdleNotificationHandler = &MbbNdisMiniportCancelIdleNotification;
    
    Status = NdisSetOptionalHandlers(
            NdisDriverHandle,
            (PVOID)&SelectiveSuspendChars
            );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "NdisMSetMiniportAttributes(general) with %08lx", Status);
        goto Cleanup;

    }

    SysPowerChars.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_SYSPOWER_NOTIFICATION;
    SysPowerChars.Header.Revision = NDIS_MINIPORT_SYSPOWER_NOTIFY_CHARACTERISTICS_REVISION_1;
    SysPowerChars.Header.Size = NDIS_SIZEOF_MINIPORT_SYSPOWER_NOTIFY_CHARACTERISTICS_REVISION_1;

    SysPowerChars.SysPowerNotifyHandler = MbbSystemPowerNotification;

    Status = NdisSetOptionalHandlers(
            NdisDriverHandle,
            (PVOID)&SysPowerChars
            );

    if (NT_ERROR(Status))
    {
        TraceError(WMBCLASS_INIT, "NdisMSetMiniportAttributes(general) with %08lx SysPowerCcaps", Status);
        goto Cleanup;
    }
Cleanup:

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return Status;
}

NDIS_STATUS
MbbNdisMiniportIdleNotification(
    __in  NDIS_HANDLE   MiniportAdapterContext,
    __in  BOOLEAN       ForceIdle
    )
{
    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    NTSTATUS            Status=NDIS_STATUS_SUCCESS;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    Status=MbbBusIdleNotification(Adapter->BusHandle, ForceIdle);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return Status;
}


VOID
MbbNdisMiniportCancelIdleNotification(
    __in  NDIS_HANDLE MiniportAdapterContext
    )
{
    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    MbbBusCancelIdleNotification(Adapter->BusHandle);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;
}


VOID
MbbNdisMiniportBusIdleConfirm(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    DEVICE_POWER_STATE      PowerState
    )

{
    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    NdisMIdleNotificationConfirm(
        Adapter->MiniportAdapterHandle,
        PowerState
        );

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;

}

VOID
MbbNdisMiniportIdleNotificationComplete(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    NTSTATUS                Status
    )

{
    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    NdisMIdleNotificationCompleteEx(Adapter->MiniportAdapterHandle, Status);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;

}

VOID
MbbSystemPowerNotification(
    _In_ NDIS_HANDLE                          MiniportAdapterContext,
    _In_ NDIS_MINIPORT_SYSPOWER_NOTIFY const *SystemPowerNotification
    )

{
    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Entered");

    NdisAcquireSpinLock(&Adapter->Lock);

    Adapter->LastLowSystemPowerState=*SystemPowerNotification;

    NdisReleaseSpinLock(&Adapter->Lock);

    TraceInfo(WMBCLASS_POWER, "%!FUNC!: TargetSystemPowerState S%d EffectiveSystemPowerState S%d", 
        SystemPowerNotification->TargetSystemState - PowerSystemWorking,
        SystemPowerNotification->EffectiveSystemState - PowerSystemWorking);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;

}

VOID
DrainCompleteCallback(
    PVOID       Context
    )

{
    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)Context;
    BOOLEAN                                     CompletePause=FALSE;
    BOOLEAN                                     ResetComplete=FALSE;
    BOOLEAN                                     StallClear=FALSE;
    PMBB_REQUEST_CONTEXT                        Request=NULL;
    PSTATE_CHANGE_EVENT                         StateChange=NULL;
    NDIS_STATUS                                 TempStatus=NDIS_STATUS_SUCCESS;

    TraceInfo(WMBCLASS_INIT, "Entered Pending=%d", Adapter->AdapterState.PendingActions-1);

    NdisAcquireSpinLock(&Adapter->Lock);

    ASSERT(Adapter->AdapterState.PendingActions > 0);

    Adapter->AdapterState.PendingActions--;

    if (Adapter->AdapterState.PendingActions == 0)
    {
        StateChange=Adapter->AdapterState.RunningEvent;

        NdisReleaseSpinLock(&Adapter->Lock);

        switch (StateChange->EventType)
        {

            case STATE_CHANGE_TYPE_PAUSE:

                CompletePause=TRUE;

                break;

            case STATE_CHANGE_TYPE_POWER:

                Request=StateChange->Power.Request;

                break;

            case STATE_CHANGE_TYPE_RESET:

                ResetComplete=TRUE;

                break;

            case STATE_CHANGE_TYPE_STALL_CLEAR:

                StallClear=TRUE;

                break;

            default:

                ASSERT(0);
                break;
        }

        if (ResetComplete)
        {
            if ((Adapter->AdapterState.CurrentPowerState == NetDeviceStateD0) && Adapter->AdapterState.Started)
            {
                //
                //  powered up, start send queue
                //
                DrainComplete( &(Adapter->SendQueue.QueueDrainObject) );

            }

            TempStatus=StateChange->Reset.PipeStartStatus;

            MbbWriteEvent(
                &RESET_COMPLETE_EVENT,
                NULL,
                NULL,
                2,
                &Adapter->TraceInstance,
                sizeof(Adapter->TraceInstance),
                &TempStatus,
                sizeof(TempStatus)
                );


            if (!NT_SUCCESS(StateChange->Reset.PipeStartStatus))
            {
                TraceError(WMBCLASS_INIT, "Reset failed %!STATUS!",StateChange->Reset.PipeStartStatus);
            }

            NdisMResetComplete(
                Adapter->MiniportAdapterHandle,
                StateChange->Reset.PipeStartStatus,
                FALSE
                );


        }


        if (Request)
        {
            TraceInfo(WMBCLASS_INIT, "Completing set power request");

            MbbReqMgrQueueEvent(
                Request->RequestManager,
                Request,
                MbbRequestEventSendComplete,
                (PVOID)(NDIS_STATUS_SUCCESS),
                0
                );
        }


        if (CompletePause)
        {
            TraceInfo(WMBCLASS_INIT, "Completing Pause");

            MbbWriteEvent(
                &PAUSE_COMPLETE_EVENT,
                NULL,
                NULL,
                2,
                &Adapter->TraceInstance,
                sizeof(Adapter->TraceInstance),
                &TempStatus,
                sizeof(TempStatus)
                );


            NdisMPauseComplete(Adapter->MiniportAdapterHandle);
        }

        if (StallClear)
        {
            TraceInfo(WMBCLASS_INIT, "Stall cleared");

            MbbAdapterLock(Adapter);

            Adapter->AdapterState.Hung=FALSE;
            KeSetEvent(&Adapter->AdapterState.StallClearCompleteEvent, IO_NO_INCREMENT, FALSE);

            MbbAdapterUnlock(Adapter);

        }

        Adapter->AdapterState.RunningEvent=NULL;

        CompleteStateChange(
            &Adapter->AdapterState,
            StateChange
            );

        NdisAcquireSpinLock(&Adapter->Lock);
    }

    NdisReleaseSpinLock(&Adapter->Lock);

    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Exited");

    return;
}


NDIS_STATUS
MbbNdisMiniportPause(
    __in  NDIS_HANDLE                         MiniportAdapterContext,
    __in  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
    )

{

    PMINIPORT_ADAPTER_CONTEXT                   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    PSTATE_CHANGE_EVENT                         StateChange=NULL;
    NDIS_STATUS                                 Status=STATUS_INSUFFICIENT_RESOURCES;

    UNREFERENCED_PARAMETER(MiniportPauseParameters);

    PAGED_CODE();

    MbbWriteEvent(
        &PAUSE_EVENT,
        NULL,
        NULL,
        1,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance)
        );


    StateChange = AllocateStateChangeEvent(&Adapter->AdapterState);

    if (StateChange != NULL)
    {
        StateChange->EventType=STATE_CHANGE_TYPE_PAUSE;
        StateChange->Context1=Adapter;

        QueueStateChangeEvent(&Adapter->AdapterState,StateChange);

        Status= STATUS_PENDING;

    }


    MbbWriteEvent(
        &PAUSE_EXIT_EVENT,
        NULL,
        NULL,
        2,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance),
        &Status,
        sizeof(Status)
        );


    return Status;
}



NDIS_STATUS
MbbNdisMiniportRestart(
    __in  NDIS_HANDLE                         MiniportAdapterContext,
    __in  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
    )

{

    PMINIPORT_ADAPTER_CONTEXT               Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    NDIS_STATUS                             Status=STATUS_INSUFFICIENT_RESOURCES;
    PSTATE_CHANGE_EVENT                     StateChange=NULL;

    UNREFERENCED_PARAMETER(MiniportRestartParameters);

    PAGED_CODE();

    MbbWriteEvent(
        &RESTART_EVENT,
        NULL,
        NULL,
        1,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance)
        );

    TraceInfo( WMBCLASS_INIT, "[MbbAdapter] RESUMING Adapter=0x%p", Adapter );

    StateChange = AllocateStateChangeEvent(&Adapter->AdapterState);

    if (StateChange != NULL)
    {
        StateChange->EventType=STATE_CHANGE_TYPE_RESTART;
        StateChange->Context1=Adapter;

        QueueStateChangeEvent(&Adapter->AdapterState,StateChange);

        Status=STATUS_PENDING;
    }
    else
    {
        TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to resume Adapter=0x%p", Adapter );
    }


    MbbWriteEvent(
        &RESTART_EXIT_EVENT,
        NULL,
        NULL,
        2,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance),
        &Status,
        sizeof(Status)
        );

    TraceInfo(WMBCLASS_INIT, "[MbbAdapter] RESUMING Adapter=0x%p Complete", Adapter);

    return Status;

}



VOID
MbbNdisMiniportDevicePnPEventNotify(
    __in  NDIS_HANDLE MiniportAdapterContext,
    __in  PNET_DEVICE_PNP_EVENT NetDevicePnPEvent
    )

{

    PMBB_REQUEST_MANAGER RequestManager;
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT) MiniportAdapterContext;

    PAGED_CODE();

    if( NetDevicePnPEvent->DevicePnPEvent == NdisDevicePnPEventSurpriseRemoved )
    {
        TraceInfo( WMBCLASS_INIT, "[MbbAdapter] SURPRISE_REMOVAL Adapter=0x%p. Cancelling OID requests", Adapter );
        // The Surprise Removed Flag is not used anywhere. It is just there for debugging purposes, so no need to set it under a lock
        Adapter->SurpriseRemoved = TRUE;
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to reference RequestManager for SURPRISE_REMOVAL" );
            return;
        }
        // Adapter Now Surprise removed, so now close the Request Manager so that no more requests are allowed
        // Pass that information to the MbbRrqMgrCancelRequests Call
        MbbReqMgrCancelRequests( RequestManager, TRUE);
        MbbReqMgrDeref( RequestManager );
    }

    return ;
}

//
// Driver routines
//
_Acquires_lock_( Driver->AdapterListLock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, Driver )
VOID
MbbDriverLockAdapterList(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    )
{
    NdisAcquireSpinLock( &Driver->AdapterListLock );
}

_Releases_lock_( Driver->AdapterListLock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, Driver )
VOID
MbbDriverUnlockAdapterList(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    )
{
    NdisReleaseSpinLock( &Driver->AdapterListLock );
}

VOID
MbbDriverAddAdapter(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    MbbDriverLockAdapterList( Driver );
    InsertTailList( &Driver->AdapterList, &Adapter->DriverLink );
    MbbAdapterRef( Adapter );
    MbbDriverUnlockAdapterList( Driver );
}

VOID
MbbDriverRemoveAdapter(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter
    )
{
    MbbDriverLockAdapterList( Driver );
    RemoveEntryList( &Adapter->DriverLink );
    MbbAdapterDeref( Adapter );
    MbbDriverUnlockAdapterList( Driver );
}

PMINIPORT_ADAPTER_CONTEXT
MbbDriverFindAdapterByNetLuid(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PNET_LUID                  NetLuid
    )
{
    PLIST_ENTRY                 ListEntry;
    PMINIPORT_ADAPTER_CONTEXT   Adapter;

    MbbDriverLockAdapterList( Driver );
    for( ListEntry  = Driver->AdapterList.Flink, Adapter = NULL;
         ListEntry != &Driver->AdapterList;
         ListEntry  = ListEntry->Flink, Adapter = NULL )
    {
        Adapter = CONTAINING_RECORD(
                    ListEntry,
                    MINIPORT_ADAPTER_CONTEXT,
                    DriverLink
                    );
        if( RtlCompareMemory(
                &Adapter->NetLuid,
                NetLuid,
                sizeof(NET_LUID)
                ) == sizeof(NET_LUID) )
        {
            MbbAdapterRef( Adapter );
            break;
        }
    }
    MbbDriverUnlockAdapterList( Driver );

    return Adapter;
}

NDIS_STATUS
MbbNdisAdapterReadParameters(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter
    )
{
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE                     ConfigurationHandle = NULL;
    PNDIS_CONFIGURATION_PARAMETER   pParameter = NULL;
    NDIS_STRING                     NetCfgGuid = RTL_CONSTANT_STRING(MBB_REGVAL_NETCFGID);
    NDIS_STRING                     NameOverride = RTL_CONSTANT_STRING(MBB_REGVAL_OVERRIDE_NAME);
    NDIS_STRING                     RadioOffRegString = RTL_CONSTANT_STRING(MBB_REGVAL_RADIO_OFF);
    NDIS_CONFIGURATION_OBJECT       ConfigObject;

    do
    {
        //
        // Open the registry for this adapter to read advanced 
        // configuration parameters stored by the INF file.
        //
        ConfigObject.Header.Type        = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
        ConfigObject.Header.Revision    = NDIS_CONFIGURATION_OBJECT_REVISION_1;
        ConfigObject.Header.Size        = sizeof(NDIS_CONFIGURATION_OBJECT);
        ConfigObject.NdisHandle         = Adapter->MiniportAdapterHandle;
        ConfigObject.Flags              = 0;

        if( (NdisStatus = NdisOpenConfigurationEx(
                            &ConfigObject,
                            &ConfigurationHandle
                            ))!= NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to open configuration, status=%!STATUS!", NdisStatus );
            break;
        }

        // NetCfgInstanceId
        NdisReadConfiguration(
            &NdisStatus,
            &pParameter,
            ConfigurationHandle,
            &NetCfgGuid,
            NdisParameterString
            );
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to read NetCfgId, status=%!STATUS!", NdisStatus );
            break;
        }
        if( (NdisStatus = RtlGUIDFromString(
                            &pParameter->ParameterData.StringData,
                            &Adapter->NetCfgId
                            )) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to convert NetCfgId string to guid, status=%!STATUS!", NdisStatus );
            break;
        }

        // DeviceNameOverride
        NdisReadConfiguration(
            &NdisStatus,
            &pParameter,
            ConfigurationHandle,
            &NameOverride,
            NdisParameterInteger
            );

        if ( NdisStatus == NDIS_STATUS_SUCCESS )
        {
            if (pParameter->ParameterType == NdisParameterInteger)
            {
                Adapter->OverrideDeviceName = pParameter->ParameterData.IntegerData != 0;

                TraceInfo( WMBCLASS_INIT, "[MbbAdapter] Name override set to %d", Adapter->OverrideDeviceName);
            }
        }
        else
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to read name override setting, status=%!STATUS!", NdisStatus );
            NdisStatus = NDIS_STATUS_SUCCESS;
        }

        // RadioState: Default = ON
        Adapter->RadioOff = FALSE;        
        NdisReadConfiguration(
            &NdisStatus,
            &pParameter,
            ConfigurationHandle,
            &RadioOffRegString,
            NdisParameterInteger
            );

        if ( NdisStatus == NDIS_STATUS_SUCCESS )
        {
            if (pParameter->ParameterType == NdisParameterInteger)
            {
                // Non zero value in registry means the radio should be OFF
                Adapter->RadioOff = (pParameter->ParameterData.IntegerData != 0);

                TraceInfo( WMBCLASS_INIT, "[MbbAdapter] Persisted radio state %s", Adapter->RadioOff ? "OFF" : "ON");
            }
        }
        else
        {
            TraceError( WMBCLASS_INIT, "[MbbAdapter] FAILED to read radio state, status=%!STATUS!", NdisStatus );
            NdisStatus = NDIS_STATUS_SUCCESS;
        }

    }
    while(FALSE);

    if( ConfigurationHandle != NULL )
    {
        NdisCloseConfiguration( ConfigurationHandle );
    }

    return NdisStatus;
}

void 
NTAPI MbbDumpAdapterState(
  _In_      LPCGUID SourceId,
  _In_      ULONG IsEnabled,
  _In_      UCHAR Level,
  _In_      ULONGLONG MatchAnyKeyword,
  _In_      ULONGLONG MatchAllKeywords,
  _In_opt_  PEVENT_FILTER_DESCRIPTOR FilterData,
  _In_opt_  PVOID CallbackContext
)
{
    if(IsEnabled == EVENT_CONTROL_CODE_CAPTURE_STATE)
    {
        PLIST_ENTRY                 ListEntry;
        PMINIPORT_ADAPTER_CONTEXT*  Adapters = NULL;
        PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
        PMINIPORT_DRIVER_CONTEXT    Driver = &GlobalControl;
        ULONG numberOfAdapters = 0, x = 0, y = 0;

        do
        {               
            // Get the adapter context
            MbbDriverLockAdapterList( Driver );
            
            for( ListEntry = Driver->AdapterList.Flink, Adapter = NULL;
                     ListEntry != &Driver->AdapterList;
                     ListEntry  = ListEntry->Flink, Adapter = NULL )
            {
                if((Adapter = CONTAINING_RECORD(
                                ListEntry,
                                MINIPORT_ADAPTER_CONTEXT,
                                DriverLink
                                ))!=NULL)
                {
                    numberOfAdapters++;
                }           
            }

            if(numberOfAdapters == 0)
            {
                MbbDriverUnlockAdapterList( Driver );
                break;
            }
            
            // Allocate an array of pointers
            Adapters = ALLOCATE_NONPAGED_POOL(sizeof(Adapter) * numberOfAdapters);

            if(Adapters == NULL)
            {
                MbbDriverUnlockAdapterList( Driver );
                break;
            }

            NdisZeroMemory(Adapters, sizeof(Adapter) * numberOfAdapters);

            numberOfAdapters = 0;
            
            for( ListEntry = Driver->AdapterList.Flink, Adapter = NULL;
                     ListEntry != &Driver->AdapterList;
                     ListEntry  = ListEntry->Flink, Adapter = NULL )
            {
                if((Adapter = CONTAINING_RECORD(
                                ListEntry,
                                MINIPORT_ADAPTER_CONTEXT,
                                DriverLink
                                ))!=NULL)
                {
                    Adapters[numberOfAdapters++] = Adapter;
                    
                    // Take a ref on the adapter so that it doesnt 
                    // get cleaned up while processing
                    MbbAdapterRef(Adapter);
                }
            }
             
            MbbDriverUnlockAdapterList( Driver );

            // Now log the state of each adapter
            for(x = 0; x < numberOfAdapters; x++)
            {               
                MBB_CELLULAR_CLASS      AdapterSupportedCellularClass;
                MBB_CELLULAR_CLASS      AdapterCurrentCellularClass;
                ULONG MaxActivatedContexts = 0;
                ULONG NumberOfPorts = 0;
                PMBB_PORT*   PortList = NULL;
                ULONG  RefCount = 0;
                ULONG  PendingRequests = 0;
                BOOL RadioOff = TRUE;

                Adapter = Adapters[x];    
                
                MbbAdapterLock(Adapter);
                AdapterSupportedCellularClass = Adapter->AdapterSupportedCellularClass;
                AdapterCurrentCellularClass = Adapter->AdapterCurrentCellularClass;
                MbbAdapterUnlock(Adapter);

                MbbAdapterPortsLock(Adapter);
                MaxActivatedContexts = Adapter->MaxActivatedContexts;
                NumberOfPorts = Adapter->NumberOfPorts;
                PortList = ALLOCATE_NONPAGED_POOL(sizeof(PMBB_PORT) * NumberOfPorts);

                if(PortList == NULL)
                {
                    MbbAdapterPortsUnlock(Adapter);
                    MbbAdapterDeref(Adapter);
                    continue;
                }

                // Copy the port information of the adapter under adapter ports lock
                NdisMoveMemory(PortList, &(Adapter->PortList), sizeof(PMBB_PORT)* NumberOfPorts);
                MbbAdapterPortsUnlock(Adapter);

                // Pending request count
                MbbReqMgrLockManager(Adapter->RequestManager);
                  
                for( ListEntry = Adapter->RequestManager->PendingRequestQueue.Flink;
                         ListEntry != &(Adapter->RequestManager->PendingRequestQueue);
                         ListEntry  = ListEntry->Flink)
                {
                    PendingRequests++;
                }

                MbbReqMgrUnlockManager(Adapter->RequestManager);
                
                RadioOff = Adapter->RadioOff;
                    
                InterlockedExchange(&RefCount, Adapter->Reference);

                MbbWriteEvent(
                        &ADAPTER_CONTEXT,
                        NULL,
                        NULL,
                        21,                      
                        Adapter->FriendlyName.Buffer,
                        ((Adapter->FriendlyName.Buffer) ? Adapter->FriendlyName.Length + 1 : 0),   // 1
                        &(Adapter->IfIndex),
                        sizeof(Adapter->IfIndex),                                                  // 2
                        Adapter->MACAddress,
                        MBB_MAC_ADDRESS_LENGTH,                                                    // 3
                        &(Adapter->NetCfgId), 
                        sizeof(Adapter->NetCfgId),                                                 // 4
                        &RadioOff,
                        sizeof(RadioOff),                                                          // 5
                        &(Adapter->MaxLinkSpeed), 
                        sizeof(Adapter->MaxLinkSpeed),                                             // 6
                        &(Adapter->CurrentLinkSpeed),
                        sizeof(Adapter->CurrentLinkSpeed),                                         // 7
                        &(Adapter->UplinkSpeed),
                        sizeof(Adapter->UplinkSpeed),                                              // 8
                        &(Adapter->DownlinkSpeed), 
                        sizeof(Adapter->DownlinkSpeed),                                            // 9
                        &(Adapter->AvailableDataClass),
                        sizeof(Adapter->AvailableDataClass),                                       // 10
                        &(Adapter->AvailableDataClassValid),
                        sizeof(BOOL),                                                              // 11
                        &AdapterSupportedCellularClass, 
                        sizeof(Adapter->AdapterSupportedCellularClass),                            // 12
                        &AdapterCurrentCellularClass,
                        sizeof(Adapter->AdapterCurrentCellularClass),                              // 13
                        &(Adapter->GenXmitFramesOk),
                        sizeof(Adapter->GenXmitFramesOk),                                          // 14
                        &(Adapter->Stats.ifOutDiscards),
                        sizeof(Adapter->Stats.ifOutDiscards),                                      // 15
                        &(Adapter->GenRcvFramesOk),
                        sizeof(Adapter->GenRcvFramesOk),                                           // 16
                        &(Adapter->Stats.ifInDiscards),
                        sizeof(Adapter->Stats.ifInDiscards),                                       // 17                        
                        &MaxActivatedContexts,
                        sizeof(Adapter->MaxActivatedContexts),                                     // 18
                        &NumberOfPorts,
                        sizeof(Adapter->NumberOfPorts),                                            // 19
                        &RefCount,
                        sizeof(Adapter->Reference),                                                // 20                        
                        &PendingRequests,
                        sizeof(PendingRequests)                                                    // 21
                        );                                    

                MbbAdapterDeref(Adapter);

                // Log port information               
                for(y = 0; y < NumberOfPorts; y++)
                {
                    NDIS_PORT_NUMBER PortNumber = NDIS_DEFAULT_PORT_NUMBER;           
                    BOOL  ConnectionUp = FALSE;
                    ULONG ConnectionId = 0;
                    LONG SessionId = MBB_INVALID_SESSION_ID;    
                    ULONG RefCount = 0;
                    
                    PMBB_PORT Port = PortList[y];

                    MBB_ACQUIRE_PORT_LOCK(Port);

                    PortNumber = Port->PortNumber;
                    ConnectionUp = Port->ConnectionState.ConnectionUp;
                    ConnectionId = Port->ConnectionId;
                    SessionId = Port->SessionId;                    

                    MBB_RELEASE_PORT_LOCK(Port);

                    InterlockedExchange(&RefCount, Port->RefCount.RefCount);

                    MbbWriteEvent(
                        &PORT_CONTEXT,
                        NULL,
                        NULL,
                        5, 
                        &PortNumber,
                        sizeof(Port->PortNumber),           // 1
                        &ConnectionUp,
                        sizeof(ConnectionUp),               // 2
                        &ConnectionId,
                        sizeof(Port->ConnectionId),         // 3
                        &SessionId,
                        sizeof(Port->SessionId),            // 4
                        &RefCount,
                        sizeof(Port->RefCount.RefCount)     // 5
                        );
                }                                

                if(PortList != NULL)
                {
                    FREE_POOL(PortList);
                    PortList = NULL;
                }
            }
        }while(FALSE);
        
        if(Adapters != NULL)
        {
            FREE_POOL(Adapters);
            Adapters = NULL;
        }        
    }
}

