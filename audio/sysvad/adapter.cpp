/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    adapter.cpp

Abstract:

    Setup and miniport installation.  No resources are used by sysvad.
    This sample is to demonstrate how to develop a full featured audio miniport driver.

--*/

#pragma warning (disable : 4127)

//
// All the GUIDS for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#include <sysvad.h>
#include <ContosoKeywordDetector.h>
#include "IHVPrivatePropertySet.h"

#include "simple.h"
#include "minipairs.h"
#ifdef SYSVAD_BTH_BYPASS
#include "bthhfpminipairs.h"
#endif // SYSVAD_BTH_BYPASS



typedef void (*fnPcDriverUnload) (PDRIVER_OBJECT);
fnPcDriverUnload gPCDriverUnloadRoutine = NULL;
extern "C" DRIVER_UNLOAD DriverUnload;

#ifdef _USE_SingleComponentMultiFxStates
C_ASSERT(sizeof(POHANDLE) == sizeof(PVOID));

//=============================================================================
//
// The number of F-states, the transition latency and residency requirement values
// used here are for illustration purposes only. The driver should use values that
// are appropriate for its device.
//
#define SYSVAD_FSTATE_COUNT                     4

#define SYSVAD_F0_LATENCY_IN_MS                 0
#define SYSVAD_F0_RESIDENCY_IN_SEC              0

#define SYSVAD_F1_LATENCY_IN_MS                 200
#define SYSVAD_F1_RESIDENCY_IN_SEC              3

#define SYSVAD_F2_LATENCY_IN_MS                 400
#define SYSVAD_F2_RESIDENCY_IN_SEC              6

#define SYSVAD_F3_LATENCY_IN_MS                 800  
#define SYSVAD_F3_RESIDENCY_IN_SEC              12

#define SYSVAD_DEEPEST_FSTATE_LATENCY_IN_MS     SYSVAD_F3_LATENCY_IN_MS
#define SYSVAD_DEEPEST_FSTATE_RESIDENCY_IN_SEC  SYSVAD_F3_RESIDENCY_IN_SEC


//-----------------------------------------------------------------------------
// PoFx - Single Component - Multi Fx States support.
//-----------------------------------------------------------------------------

PO_FX_COMPONENT_IDLE_STATE_CALLBACK         PcPowerFxComponentIdleStateCallback;
PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK   PcPowerFxComponentActiveConditionCallback;
PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK     PcPowerFxComponentIdleConditionCallback;
PO_FX_POWER_CONTROL_CALLBACK                PcPowerFxPowerControlCallback;
EVT_PC_POST_PO_FX_REGISTER_DEVICE           PcPowerFxRegisterDevice;
EVT_PC_PRE_PO_FX_UNREGISTER_DEVICE          PcPowerFxUnregisterDevice;

#pragma code_seg("PAGE")
_Function_class_(EVT_PC_POST_PO_FX_REGISTER_DEVICE)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PcPowerFxRegisterDevice(
    _In_ PVOID    PoFxDeviceContext,
    _In_ POHANDLE PoHandle
    )
{
    PDEVICE_OBJECT              DeviceObject    = (PDEVICE_OBJECT)PoFxDeviceContext;
    PortClassDeviceContext*     pExtension      = static_cast<PortClassDeviceContext*>(DeviceObject->DeviceExtension);
    
    PAGED_CODE(); 
    
    DPF(D_VERBOSE, ("PcPowerFxRegisterDevice Context %p, PoHandle %p", PoFxDeviceContext, PoHandle));

    pExtension->m_poHandle = PoHandle;
    
    //
    // Set latency and residency hints so that the power framework chooses lower
    // powered F-states when we are idle.
    // The values used here are for illustration purposes only. The driver 
    // should use values that are appropriate for its device.
    //
    PoFxSetComponentLatency(
        PoHandle,
        0, // Component
        (WDF_ABS_TIMEOUT_IN_MS(SYSVAD_DEEPEST_FSTATE_LATENCY_IN_MS) + 1)
        );
    PoFxSetComponentResidency(
        PoHandle,
        0, // Component
        (WDF_ABS_TIMEOUT_IN_SEC(SYSVAD_DEEPEST_FSTATE_RESIDENCY_IN_SEC) + 1)
        );

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
_Function_class_(EVT_PC_PRE_PO_FX_UNREGISTER_DEVICE)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PcPowerFxUnregisterDevice(
    _In_ PVOID    PoFxDeviceContext,
    _In_ POHANDLE PoHandle
    )
{
    PDEVICE_OBJECT              DeviceObject    = (PDEVICE_OBJECT)PoFxDeviceContext;
    PortClassDeviceContext*     pExtension      = static_cast<PortClassDeviceContext*>(DeviceObject->DeviceExtension);

    UNREFERENCED_PARAMETER(PoHandle);
        
    PAGED_CODE();
    
    DPF(D_VERBOSE, ("PcPowerFxUnregisterDevice Context %p, PoHandle %p", PoFxDeviceContext, PoHandle));

    //
    // Driver must not use the PoHandle after this call.
    //
    ASSERT(pExtension->m_poHandle == PoHandle);
    pExtension->m_poHandle = NULL;
}

#pragma code_seg()
__drv_functionClass(PO_FX_COMPONENT_IDLE_STATE_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PcPowerFxComponentIdleStateCallback(
    _In_ PVOID      Context,
    _In_ ULONG      Component,
    _In_ ULONG      State
    )
{
    PDEVICE_OBJECT              DeviceObject    = (PDEVICE_OBJECT)Context;
    PortClassDeviceContext*     pExtension      = static_cast<PortClassDeviceContext*>(DeviceObject->DeviceExtension);
    
    UNREFERENCED_PARAMETER(State);
    
    DPF(D_VERBOSE, ("PcPowerFxComponentIdleStateCallback Context %p, Component %d, State %d", 
        Context, Component, State));
    
    PoFxCompleteIdleState(pExtension->m_poHandle, Component);
}

#pragma code_seg()
__drv_functionClass(PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PcPowerFxComponentActiveConditionCallback(
    _In_ PVOID      Context,
    _In_ ULONG      Component
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Component);
    
    DPF(D_VERBOSE, ("PcPowerFxComponentActiveConditionCallback Context %p, Component %d", 
        Context, Component));
}

#pragma code_seg()
__drv_functionClass(PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PcPowerFxComponentIdleConditionCallback(
    _In_ PVOID      Context,
    _In_ ULONG      Component
    )
{
    PDEVICE_OBJECT              DeviceObject    = (PDEVICE_OBJECT)Context;
    PortClassDeviceContext*     pExtension      = static_cast<PortClassDeviceContext*>(DeviceObject->DeviceExtension);
    
    DPF(D_VERBOSE, ("PcPowerFxComponentIdleConditionCallback Context %p, Component %d", 
        Context, Component));

    PoFxCompleteIdleCondition(pExtension->m_poHandle, Component);
}

#pragma code_seg()
__drv_functionClass(PO_FX_POWER_CONTROL_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PcPowerFxPowerControlCallback(
    _In_                                    PVOID   DeviceContext,
    _In_                                    LPCGUID PowerControlCode,
    _In_reads_bytes_opt_(InBufferSize)      PVOID   InBuffer,
    _In_                                    SIZE_T  InBufferSize,
    _Out_writes_bytes_opt_(OutBufferSize)   PVOID   OutBuffer,
    _In_                                    SIZE_T  OutBufferSize,
    _Out_opt_                               PSIZE_T BytesReturned
)
{
    UNREFERENCED_PARAMETER(DeviceContext);
    UNREFERENCED_PARAMETER(PowerControlCode);
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);
    UNREFERENCED_PARAMETER(OutBuffer);
    UNREFERENCED_PARAMETER(OutBufferSize);
    UNREFERENCED_PARAMETER(BytesReturned);
    
    DPF(D_VERBOSE, ("PcPowerFxPowerControlCallback Context %p", DeviceContext));

    return STATUS_SUCCESS;
}
#endif // _USE_SingleComponentMultiFxStates

//-----------------------------------------------------------------------------
// Referenced forward.
//-----------------------------------------------------------------------------

DRIVER_ADD_DEVICE AddDevice;

NTSTATUS
StartDevice
( 
    _In_  PDEVICE_OBJECT,      
    _In_  PIRP,                
    _In_  PRESOURCELIST        
); 

_Dispatch_type_(IRP_MJ_PNP)
DRIVER_DISPATCH PnpHandler;

//
// Rendering streams are saved to a file by default. Use the registry value 
// DoNotCreateDataFiles (DWORD) > 0 to override this default.
//
DWORD g_DoNotCreateDataFiles = 0;


#ifdef SYSVAD_BTH_BYPASS
//
// This driver listens for arrival/removal of the bth sco bypass interfaces by 
// default. Use the registry value DisableBthScoBypass (DWORD) > 0 to override 
// this default.
//
DWORD g_DisableBthScoBypass = 0;
#endif // SYSVAD_BTH_BYPASS

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//=============================================================================
#pragma code_seg("PAGE")
extern "C"
void DriverUnload 
(
    _In_ PDRIVER_OBJECT DriverObject
)
/*++

Routine Description:

  Our driver unload routine. This just frees the WDF driver object.

Arguments:

  DriverObject - pointer to the driver object

Environment:

    PASSIVE_LEVEL

--*/
{
    PAGED_CODE(); 

    DPF(D_TERSE, ("[DriverUnload]"));

    if (DriverObject == NULL)
    {
        goto Done;
    }
    
    //
    // Invoke first the port unload.
    //
    if (gPCDriverUnloadRoutine != NULL)
    {
        gPCDriverUnloadRoutine(DriverObject);
    }

    //
    // Unload WDF driver object. 
    //
    if (WdfGetDriver() != NULL)
    {
        WdfDriverMiniportUnload(WdfGetDriver());
    }

Done:
    return;
}

//=============================================================================
#pragma code_seg("INIT")
__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
GetRegistrySettings(
    _In_ PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

    Initialize Driver Framework settings from the driver
    specific registry settings under

    \REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters

Arguments:

    RegistryPath - Registry path passed to DriverEntry

Returns:

    NTSTATUS - SUCCESS if able to configure the framework

--*/

{
    NTSTATUS                    ntStatus;
    UNICODE_STRING              parametersPath;
    RTL_QUERY_REGISTRY_TABLE    paramTable[4];
    
    DPF(D_TERSE, ("[GetRegistrySettings]"));
    
    PAGED_CODE(); 

    RtlInitUnicodeString(&parametersPath, NULL);

    parametersPath.MaximumLength =
        RegistryPath->Length + sizeof(L"\\Parameters") + sizeof(WCHAR);

    parametersPath.Buffer = (PWCH) ExAllocatePoolWithTag(PagedPool, parametersPath.MaximumLength, MINADAPTER_POOLTAG);
    if (parametersPath.Buffer == NULL) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(parametersPath.Buffer, parametersPath.MaximumLength);

    RtlAppendUnicodeToString(&parametersPath, RegistryPath->Buffer);
    RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

    RtlZeroMemory(&paramTable[0], sizeof(paramTable));

    g_DoNotCreateDataFiles = 0; // default is off.
    
    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"DoNotCreateDataFiles";
    paramTable[0].EntryContext  = &g_DoNotCreateDataFiles;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &g_DoNotCreateDataFiles;
    paramTable[0].DefaultLength = sizeof(ULONG);
    
#ifdef SYSVAD_BTH_BYPASS
    g_DisableBthScoBypass = 0;  // default is off.
    
    paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name          = L"DisableBthScoBypass";
    paramTable[1].EntryContext  = &g_DisableBthScoBypass;
    paramTable[1].DefaultType   = REG_DWORD;
    paramTable[1].DefaultData   = &g_DisableBthScoBypass;
    paramTable[1].DefaultLength = sizeof(ULONG);
#endif // SYSVAD_BTH_BYPASS


    ntStatus = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                 parametersPath.Buffer,
                 &paramTable[0],
                 NULL,
                 NULL
                );

    if (!NT_SUCCESS(ntStatus)) 
    {
        DPF(D_VERBOSE, ("RtlQueryRegistryValues failed, using default values, 0x%x", ntStatus));
        //
        // Don't return error because we will operate with default values.
        //
    }

    //
    // Dump settings.
    //
    DPF(D_VERBOSE, ("DoNotCreateDataFiles: %u", g_DoNotCreateDataFiles));
#ifdef SYSVAD_BTH_BYPASS
    DPF(D_VERBOSE, ("DisableBthScoBypass: %u", g_DisableBthScoBypass));
#endif // SYSVAD_BTH_BYPASS
    
    //
    // Cleanup.
    //
    ExFreePool(parametersPath.Buffer);

    return STATUS_SUCCESS;
}

#pragma code_seg("INIT")
extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" NTSTATUS
DriverEntry
( 
    _In_  PDRIVER_OBJECT          DriverObject,
    _In_  PUNICODE_STRING         RegistryPathName
)
{
/*++

Routine Description:

  Installable driver initialization entry point.
  This entry point is called directly by the I/O system.

  All audio adapter drivers can use this code without change.

Arguments:

  DriverObject - pointer to the driver object

  RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

  STATUS_SUCCESS if successful,
  STATUS_UNSUCCESSFUL otherwise.

--*/
    NTSTATUS                    ntStatus;
    WDF_DRIVER_CONFIG           config;

    DPF(D_TERSE, ("[DriverEntry]"));
    
    //
    // Get registry configuration.
    //
    ntStatus = GetRegistrySettings(RegistryPathName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Registry Configuration error 0x%x", ntStatus)),
        Done);
    
    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    //
    // Set WdfDriverInitNoDispatchOverride flag to tell the framework
    // not to provide dispatch routines for the driver. In other words,
    // the framework must not intercept IRPs that the I/O manager has
    // directed to the driver. In this case, they will be handled by Audio
    // port driver.
    //
    config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;
    config.DriverPoolTag    = MINADAPTER_POOLTAG;

    ntStatus = WdfDriverCreate(DriverObject,
                               RegistryPathName,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &config,
                               WDF_NO_HANDLE);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("WdfDriverCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Tell the class driver to initialize the driver.
    //
    ntStatus =  PcInitializeAdapterDriver(DriverObject,
                                          RegistryPathName,
                                          (PDRIVER_ADD_DEVICE)AddDevice);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("PcInitializeAdapterDriver failed, 0x%x", ntStatus)),
        Done);

    //
    // To intercept stop/remove/surprise-remove.
    //
    DriverObject->MajorFunction[IRP_MJ_PNP] = PnpHandler;

    //
    // Hook the port class unload function
    //
    gPCDriverUnloadRoutine = DriverObject->DriverUnload;
    DriverObject->DriverUnload = DriverUnload;

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;
    
Done:

    if (!NT_SUCCESS(ntStatus))
    {
        if (WdfGetDriver() != NULL)
        {
            WdfDriverMiniportUnload(WdfGetDriver());
        }
    }
    
    return ntStatus;
} // DriverEntry

#pragma code_seg()
// disable prefast warning 28152 because 
// DO_DEVICE_INITIALIZING is cleared in PcAddAdapterDevice
#pragma warning(disable:28152)
#pragma code_seg("PAGE")
//=============================================================================
NTSTATUS AddDevice
( 
    _In_  PDRIVER_OBJECT    DriverObject,
    _In_  PDEVICE_OBJECT    PhysicalDeviceObject 
)
/*++

Routine Description:

  The Plug & Play subsystem is handing us a brand new PDO, for which we
  (by means of INF registration) have been asked to provide a driver.

  We need to determine if we need to be in the driver stack for the device.
  Create a function device object to attach to the stack
  Initialize that device object
  Return status success.

  All audio adapter drivers can use this code without change.

Arguments:

  DriverObject - pointer to a driver object

  PhysicalDeviceObject -  pointer to a device object created by the
                            underlying bus driver.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS        ntStatus;
    ULONG           maxObjects;

    DPF(D_TERSE, ("[AddDevice]"));

    maxObjects = g_MaxMiniports;
    
#ifdef SYSVAD_BTH_BYPASS
    // 
    // Allow three (3) Bluetooth hands-free profile devices.
    //
    maxObjects += g_MaxBthHfpMiniports * 3; 
#endif // SYSVAD_BTH_BYPASS

    // Tell the class driver to add the device.
    //
    ntStatus = 
        PcAddAdapterDevice
        ( 
            DriverObject,
            PhysicalDeviceObject,
            PCPFNSTARTDEVICE(StartDevice),
            maxObjects,
            0
        );


    return ntStatus;
} // AddDevice

#pragma code_seg()
NTSTATUS
_IRQL_requires_max_(DISPATCH_LEVEL)
PowerControlCallback
(
    _In_        LPCGUID PowerControlCode,
    _In_opt_    PVOID   InBuffer,
    _In_        SIZE_T  InBufferSize,
    _Out_writes_bytes_to_(OutBufferSize, *BytesReturned) PVOID OutBuffer,
    _In_        SIZE_T  OutBufferSize,
    _Out_opt_   PSIZE_T BytesReturned,
    _In_opt_    PVOID   Context
)
{
    UNREFERENCED_PARAMETER(PowerControlCode);
    UNREFERENCED_PARAMETER(BytesReturned);
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(OutBuffer);
    UNREFERENCED_PARAMETER(OutBufferSize);
    UNREFERENCED_PARAMETER(InBufferSize);
    UNREFERENCED_PARAMETER(Context);
    
    return STATUS_NOT_IMPLEMENTED;
}

#pragma code_seg("PAGE")
NTSTATUS 
InstallEndpointRenderFilters(
    _In_ PDEVICE_OBJECT     _pDeviceObject, 
    _In_ PIRP               _pIrp, 
    _In_ PADAPTERCOMMON     _pAdapterCommon,
    _In_ PENDPOINT_MINIPAIR _pAeMiniports
    )
{
    NTSTATUS                    ntStatus                = STATUS_SUCCESS;
    PUNKNOWN                    unknownTopology         = NULL;
    PUNKNOWN                    unknownWave             = NULL;
    PPORTCLSETWHELPER           pPortClsEtwHelper       = NULL;
#ifdef _USE_IPortClsRuntimePower
    PPORTCLSRUNTIMEPOWER        pPortClsRuntimePower    = NULL;
#endif // _USE_IPortClsRuntimePower
    PPORTCLSStreamResourceManager pPortClsResMgr        = NULL;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(_pDeviceObject);

    ntStatus = _pAdapterCommon->InstallEndpointFilters(
        _pIrp,
        _pAeMiniports,
        NULL,
        &unknownTopology,
        &unknownWave);

    if (unknownWave) // IID_IPortClsEtwHelper and IID_IPortClsRuntimePower interfaces are only exposed on the WaveRT port.
    {
        ntStatus = unknownWave->QueryInterface (IID_IPortClsEtwHelper, (PVOID *)&pPortClsEtwHelper);
        if (NT_SUCCESS(ntStatus))
        {
            _pAdapterCommon->SetEtwHelper(pPortClsEtwHelper);
            ASSERT(pPortClsEtwHelper != NULL);
            pPortClsEtwHelper->Release();
        }

#ifdef _USE_IPortClsRuntimePower
        // Let's get the runtime power interface on PortCls.  
        ntStatus = unknownWave->QueryInterface(IID_IPortClsRuntimePower, (PVOID *)&pPortClsRuntimePower);
        if (NT_SUCCESS(ntStatus))
        {
            // This interface would typically be stashed away for later use.  Instead,
            // let's just send an empty control with GUID_NULL.
            NTSTATUS ntStatusTest =
                pPortClsRuntimePower->SendPowerControl
                (
                    _pDeviceObject,
                    &GUID_NULL,
                    NULL,
                    0,
                    NULL,
                    0,
                    NULL
                );

            if (NT_SUCCESS(ntStatusTest) || STATUS_NOT_IMPLEMENTED == ntStatusTest || STATUS_NOT_SUPPORTED == ntStatusTest)
            {
                ntStatus = pPortClsRuntimePower->RegisterPowerControlCallback(_pDeviceObject, &PowerControlCallback, NULL);
                if (NT_SUCCESS(ntStatus))
                {
                    ntStatus = pPortClsRuntimePower->UnregisterPowerControlCallback(_pDeviceObject);
                }
            }
            else
            {
                ntStatus = ntStatusTest;
            }

            pPortClsRuntimePower->Release();
        }
#endif // _USE_IPortClsRuntimePower

        //
        // Test: add and remove current thread as streaming audio resource.  
        // In a real driver you should only add interrupts and driver-owned threads 
        // (i.e., do not add the current thread as streaming resource).
        //
        ntStatus = unknownWave->QueryInterface(IID_IPortClsStreamResourceManager, (PVOID *)&pPortClsResMgr);
        if (NT_SUCCESS(ntStatus))
        {
            PCSTREAMRESOURCE_DESCRIPTOR res;
            PCSTREAMRESOURCE hRes = NULL;
            PDEVICE_OBJECT pdo = NULL;

            PcGetPhysicalDeviceObject(_pDeviceObject, &pdo);
            PCSTREAMRESOURCE_DESCRIPTOR_INIT(&res);
            res.Pdo = pdo;
            res.Type = ePcStreamResourceThread;
            res.Resource.Thread = PsGetCurrentThread();
            
            NTSTATUS ntStatusTest = pPortClsResMgr->AddStreamResource(NULL, &res, &hRes);
            if (NT_SUCCESS(ntStatusTest))
            {
                pPortClsResMgr->RemoveStreamResource(hRes);
                hRes = NULL;
            }

            pPortClsResMgr->Release();
            pPortClsResMgr = NULL;
        }
    }

    SAFE_RELEASE(unknownTopology);
    SAFE_RELEASE(unknownWave);

    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS 
InstallAllRenderFilters(
    _In_ PDEVICE_OBJECT _pDeviceObject, 
    _In_ PIRP           _pIrp, 
    _In_ PADAPTERCOMMON _pAdapterCommon
    )
{
    NTSTATUS            ntStatus;
    PENDPOINT_MINIPAIR* ppAeMiniports   = g_RenderEndpoints;
    
    PAGED_CODE();

    for(ULONG i = 0; i < g_cRenderEndpoints; ++i, ++ppAeMiniports)
    {
        ntStatus = InstallEndpointRenderFilters(_pDeviceObject, _pIrp, _pAdapterCommon, *ppAeMiniports);
        IF_FAILED_JUMP(ntStatus, Exit);
    }
    
    ntStatus = STATUS_SUCCESS;

Exit:
    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS 
InstallEndpointCaptureFilters(
    _In_ PDEVICE_OBJECT     _pDeviceObject, 
    _In_ PIRP               _pIrp, 
    _In_ PADAPTERCOMMON     _pAdapterCommon, 
    _In_ PENDPOINT_MINIPAIR _pAeMiniports
    )
{
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER(_pDeviceObject);

    ntStatus = _pAdapterCommon->InstallEndpointFilters(
        _pIrp,
        _pAeMiniports,
        NULL,
        NULL,
        NULL);
        
    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS 
InstallAllCaptureFilters(
    _In_ PDEVICE_OBJECT _pDeviceObject, 
    _In_ PIRP           _pIrp, 
    _In_ PADAPTERCOMMON _pAdapterCommon
    )
{
    NTSTATUS            ntStatus;
    PENDPOINT_MINIPAIR* ppAeMiniports     = g_CaptureEndpoints;
    
    PAGED_CODE();

    for(ULONG i = 0; i < g_cCaptureEndpoints; ++i, ++ppAeMiniports)
    {
        ntStatus = InstallEndpointCaptureFilters(_pDeviceObject, _pIrp, _pAdapterCommon, *ppAeMiniports);
        IF_FAILED_JUMP(ntStatus, Exit);
    }
    
    ntStatus = STATUS_SUCCESS;

Exit:
    return ntStatus;
}

#ifdef _USE_SingleComponentMultiFxStates
//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UseSingleComponentMultiFxStates(
    _In_  PDEVICE_OBJECT    DeviceObject    

)
{
    NTSTATUS status;
    PC_POWER_FRAMEWORK_SETTINGS poFxSettings;
    PO_FX_COMPONENT component;
    PO_FX_COMPONENT_IDLE_STATE idleStates[SYSVAD_FSTATE_COUNT];
    
    //
    // Note that we initialize the 'idleStates' array below based on the
    // assumption that SYSVAD_FSTATE_COUNT is 4.
    // If we increase the value of SYSVAD_FSTATE_COUNT, we need to initialize those
    // additional F-states below. If we decrease the value of SYSVAD_FSTATE_COUNT,
    // we need to remove the corresponding initializations below.
    //
    C_ASSERT(SYSVAD_FSTATE_COUNT == 4);
    
    PAGED_CODE();
    
    //
    // Initialization
    //
    RtlZeroMemory(&component, sizeof(component));
    RtlZeroMemory(idleStates, sizeof(idleStates));

    //
    // F0
    //
    idleStates[0].TransitionLatency = WDF_ABS_TIMEOUT_IN_MS(SYSVAD_F0_LATENCY_IN_MS);    
    idleStates[0].ResidencyRequirement = WDF_ABS_TIMEOUT_IN_SEC(SYSVAD_F0_RESIDENCY_IN_SEC);    
    idleStates[0].NominalPower = 0;    

    //
    // F1
    //
    idleStates[1].TransitionLatency = WDF_ABS_TIMEOUT_IN_MS(SYSVAD_F1_LATENCY_IN_MS);
    idleStates[1].ResidencyRequirement = WDF_ABS_TIMEOUT_IN_SEC(SYSVAD_F1_RESIDENCY_IN_SEC);   
    idleStates[1].NominalPower = 0;    

    //
    // F2
    //
    idleStates[2].TransitionLatency = WDF_ABS_TIMEOUT_IN_MS(SYSVAD_F2_LATENCY_IN_MS);    
    idleStates[2].ResidencyRequirement = WDF_ABS_TIMEOUT_IN_SEC(SYSVAD_F2_RESIDENCY_IN_SEC);    
    idleStates[2].NominalPower = 0;    

    //
    // F3
    //
    idleStates[3].TransitionLatency = WDF_ABS_TIMEOUT_IN_MS(SYSVAD_F3_LATENCY_IN_MS); 
    idleStates[3].ResidencyRequirement = WDF_ABS_TIMEOUT_IN_SEC(SYSVAD_F3_RESIDENCY_IN_SEC);
    idleStates[3].NominalPower = 0;

    //
    // Component 0 (the only component)
    //
    component.IdleStateCount = SYSVAD_FSTATE_COUNT;
    component.IdleStates = idleStates;

    PC_POWER_FRAMEWORK_SETTINGS_INIT(&poFxSettings);

    poFxSettings.EvtPcPostPoFxRegisterDevice = PcPowerFxRegisterDevice; 
    poFxSettings.EvtPcPrePoFxUnregisterDevice = PcPowerFxUnregisterDevice;
    poFxSettings.ComponentIdleStateCallback = PcPowerFxComponentIdleStateCallback;
    poFxSettings.ComponentActiveConditionCallback = PcPowerFxComponentActiveConditionCallback;
    poFxSettings.ComponentIdleConditionCallback = PcPowerFxComponentIdleConditionCallback;
    poFxSettings.PowerControlCallback = PcPowerFxPowerControlCallback;

    poFxSettings.Component = &component;
    poFxSettings.PoFxDeviceContext = (PVOID) DeviceObject;
    
    status = PcAssignPowerFrameworkSettings(DeviceObject, &poFxSettings);
    if (!NT_SUCCESS(status)) {
        DPF(D_ERROR, ("PcAssignPowerFrameworkSettings failed with status 0x%x", status));
    }
    
    return status;
}
#endif // _USE_SingleComponentMultiFxStates

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
StartDevice
( 
    _In_  PDEVICE_OBJECT          DeviceObject,     
    _In_  PIRP                    Irp,              
    _In_  PRESOURCELIST           ResourceList      
)  
{
/*++

Routine Description:

  This function is called by the operating system when the device is 
  started.
  It is responsible for starting the miniports.  This code is specific to    
  the adapter because it calls out miniports for functions that are specific 
  to the adapter.                                                            

Arguments:

  DeviceObject - pointer to the driver object

  Irp - pointer to the irp 

  ResourceList - pointer to the resource list assigned by PnP manager

Return Value:

  NT status code.

--*/
    UNREFERENCED_PARAMETER(ResourceList);

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(ResourceList);

    NTSTATUS                    ntStatus        = STATUS_SUCCESS;

    PADAPTERCOMMON              pAdapterCommon  = NULL;
    PUNKNOWN                    pUnknownCommon  = NULL;
    PortClassDeviceContext*     pExtension      = static_cast<PortClassDeviceContext*>(DeviceObject->DeviceExtension);

    DPF_ENTER(("[StartDevice]"));

    //
    // create a new adapter common object
    //
    ntStatus = NewAdapterCommon( 
                                &pUnknownCommon,
                                IID_IAdapterCommon,
                                NULL,
                                NonPagedPoolNx 
                                );
    IF_FAILED_JUMP(ntStatus, Exit);

    ntStatus = pUnknownCommon->QueryInterface( IID_IAdapterCommon,(PVOID *) &pAdapterCommon);
    IF_FAILED_JUMP(ntStatus, Exit);

    ntStatus = pAdapterCommon->Init(DeviceObject);
    IF_FAILED_JUMP(ntStatus, Exit);

    //
    // register with PortCls for power-management services
    ntStatus = PcRegisterAdapterPowerManagement( PUNKNOWN(pAdapterCommon), DeviceObject);
    IF_FAILED_JUMP(ntStatus, Exit);

    //
    // Install wave+topology filters for render devices
    //
    ntStatus = InstallAllRenderFilters(DeviceObject, Irp, pAdapterCommon);
    IF_FAILED_JUMP(ntStatus, Exit);

    //
    // Install wave+topology filters for capture devices
    //
    ntStatus = InstallAllCaptureFilters(DeviceObject, Irp, pAdapterCommon);
    IF_FAILED_JUMP(ntStatus, Exit);

#ifdef SYSVAD_BTH_BYPASS
    if (!g_DisableBthScoBypass)
    {
        //
        // Init infrastructure for Bluetooth HFP - SCO Bypass devices.
        //
        ntStatus = pAdapterCommon->InitBthScoBypass();
        IF_FAILED_JUMP(ntStatus, Exit);
    }
#endif // SYSVAD_BTH_BYPASS

#ifdef _USE_SingleComponentMultiFxStates
    //
    // Init single component - multi Fx states
    //
    ntStatus = UseSingleComponentMultiFxStates(DeviceObject);
    IF_FAILED_JUMP(ntStatus, Exit);
#endif // _USE_SingleComponentMultiFxStates

Exit:

    //
    // Stash the adapter common object in the device extension so
    // we can access it for cleanup on stop/removal.
    //
    if (pAdapterCommon)
    {
        ASSERT(pExtension != NULL);
        pExtension->m_pCommon = pAdapterCommon;
    }

    //
    // Release the adapter IUnknown interface.
    //
    SAFE_RELEASE(pUnknownCommon);
    
    return ntStatus;
} // StartDevice

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
PnpHandler
(
    _In_ DEVICE_OBJECT *_DeviceObject, 
    _Inout_ IRP *_Irp
)
/*++

Routine Description:

  Handles PnP IRPs                                                           

Arguments:

  _DeviceObject - Functional Device object pointer.

  _Irp - The Irp being passed

Return Value:

  NT status code.

--*/
{
    NTSTATUS                ntStatus = STATUS_UNSUCCESSFUL;
    IO_STACK_LOCATION      *stack;
    PortClassDeviceContext *ext;

    PAGED_CODE(); 

    ASSERT(_DeviceObject);
    ASSERT(_Irp);

    //
    // Check for the REMOVE_DEVICE irp.  If we're being unloaded, 
    // uninstantiate our devices and release the adapter common
    // object.
    //
    stack = IoGetCurrentIrpStackLocation(_Irp);


    if ((IRP_MN_REMOVE_DEVICE == stack->MinorFunction) ||
        (IRP_MN_SURPRISE_REMOVAL == stack->MinorFunction) ||
        (IRP_MN_STOP_DEVICE == stack->MinorFunction))
    {
        ext = static_cast<PortClassDeviceContext*>(_DeviceObject->DeviceExtension);

        if (ext->m_pCommon != NULL)
        {
            ext->m_pCommon->Cleanup();
            
            ext->m_pCommon->Release();
            ext->m_pCommon = NULL;
        }
    }
    
    ntStatus = PcDispatchIrp(_DeviceObject, _Irp);

    return ntStatus;
}

#pragma code_seg()

