/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    pep.h

Abstract:

    This is the general header.


--*/

//
//--------------------------------------------------------------------- Pragmas
//

#pragma once

//
//-------------------------------------------------------------------- Includes
//

#include <ntddk.h>
#include <pepfx.h>
#include "pch.h"

//
//----------------------------------------------------------------- Definitions
//

#define PEP_POOL_TAG 'xEPT'

#define PEP_REGISTRY_OPTIN_KEY L"PepOptIn"
#define PEP_REGISTRY_OPTIN_KEY_ACPI_MASK 0x00000001
#define PEP_REGISTRY_OPTIN_KEY_DPM_MASK  0x00000002
#define PEP_REGISTRY_OPTIN_KEY_PPM_MASK  0x00000004

#define PEP_MAKE_DEVICE_TYPE(Major, Minor, UniqueId) \
    (ULONG)(((ULONG)(Major) << 24) | \
    (((ULONG)(Minor) & 0xFF) << 16) |   \
    ((ULONG)(UniqueId) & 0xFFFF))

#define OffsetToPtr(base, offset) ((PVOID)((ULONG_PTR)(base) + (offset)))

#define PEP_ACPI_NOTIFICATION_ID_TO_WORK_TYPE(Id) \
    (((Id) == PEP_NOTIFY_ACPI_EVALUATE_CONTROL_METHOD) ? \
        PepWorkAcpiEvaluateControlMethodComplete : \
        PepWorkAcpiNotify)

//
// ACPI method names.
//

#define ACPI_OBJECT_NAME_AC0  ((ULONG)'0CA_')
#define ACPI_OBJECT_NAME_AC1  ((ULONG)'1CA_')
#define ACPI_OBJECT_NAME_AC2  ((ULONG)'2CA_')
#define ACPI_OBJECT_NAME_AC3  ((ULONG)'3CA_')
#define ACPI_OBJECT_NAME_AC4  ((ULONG)'4CA_')
#define ACPI_OBJECT_NAME_AC5  ((ULONG)'5CA_')
#define ACPI_OBJECT_NAME_AC6  ((ULONG)'6CA_')
#define ACPI_OBJECT_NAME_AC7  ((ULONG)'7CA_')
#define ACPI_OBJECT_NAME_AC8  ((ULONG)'8CA_')
#define ACPI_OBJECT_NAME_AC9  ((ULONG)'9CA_')
#define ACPI_OBJECT_NAME_ADR  ((ULONG)'RDA_')
#define ACPI_OBJECT_NAME_AL0  ((ULONG)'0LA_')
#define ACPI_OBJECT_NAME_AL1  ((ULONG)'1LA_')
#define ACPI_OBJECT_NAME_AL2  ((ULONG)'2LA_')
#define ACPI_OBJECT_NAME_AL3  ((ULONG)'3LA_')
#define ACPI_OBJECT_NAME_AL4  ((ULONG)'4LA_')
#define ACPI_OBJECT_NAME_AL5  ((ULONG)'5LA_')
#define ACPI_OBJECT_NAME_AL6  ((ULONG)'6LA_')
#define ACPI_OBJECT_NAME_AL7  ((ULONG)'7LA_')
#define ACPI_OBJECT_NAME_AL8  ((ULONG)'8LA_')
#define ACPI_OBJECT_NAME_AL9  ((ULONG)'9LA_')
#define ACPI_OBJECT_NAME_BST  ((ULONG)'TSB_')
#define ACPI_OBJECT_NAME_CCA  ((ULONG)'ACC_')
#define ACPI_OBJECT_NAME_CID  ((ULONG)'DIC_')
#define ACPI_OBJECT_NAME_CLS  ((ULONG)'SLC_')
#define ACPI_OBJECT_NAME_CRS  ((ULONG)'SRC_')
#define ACPI_OBJECT_NAME_CRT  ((ULONG)'TRC_')
#define ACPI_OBJECT_NAME_DCK  ((ULONG)'KCD_')
#define ACPI_OBJECT_NAME_DDN  ((ULONG)'NDD_')
#define ACPI_OBJECT_NAME_DEP  ((ULONG)'PED_')
#define ACPI_OBJECT_NAME_DIS  ((ULONG)'SID_')
#define ACPI_OBJECT_NAME_DLM  ((ULONG)'MLD_')
#define ACPI_OBJECT_NAME_DSM  ((ULONG)'MSD_')
#define ACPI_OBJECT_NAME_DSW  ((ULONG)'WSD_')
#define ACPI_OBJECT_NAME_DTI  ((ULONG)'ITD_')
#define ACPI_OBJECT_NAME_EJD  ((ULONG)'DJE_')
#define ACPI_OBJECT_NAME_EJ0  ((ULONG)'0JE_')
#define ACPI_OBJECT_NAME_EJ1  ((ULONG)'1JE_')
#define ACPI_OBJECT_NAME_EJ2  ((ULONG)'2JE_')
#define ACPI_OBJECT_NAME_EJ3  ((ULONG)'3JE_')
#define ACPI_OBJECT_NAME_EJ4  ((ULONG)'4JE_')
#define ACPI_OBJECT_NAME_EJ5  ((ULONG)'5JE_')
#define ACPI_OBJECT_NAME_FST  ((ULONG)'TSF_')
#define ACPI_OBJECT_NAME_GHID ((ULONG)'DIHG')
#define ACPI_OBJECT_NAME_HID  ((ULONG)'DIH_')
#define ACPI_OBJECT_NAME_HRV  ((ULONG)'VRH_')
#define ACPI_OBJECT_NAME_HOT  ((ULONG)'TOH_')
#define ACPI_OBJECT_NAME_INI  ((ULONG)'INI_')
#define ACPI_OBJECT_NAME_IRC  ((ULONG)'CRI_')
#define ACPI_OBJECT_NAME_LCK  ((ULONG)'KCL_')
#define ACPI_OBJECT_NAME_LID  ((ULONG)'DIL_')
#define ACPI_OBJECT_NAME_MAT  ((ULONG)'TAM_')
#define ACPI_OBJECT_NAME_NTT  ((ULONG)'TTN_')
#define ACPI_OBJECT_NAME_OFF  ((ULONG)'FFO_')
#define ACPI_OBJECT_NAME_ON   ((ULONG)'_NO_')
#define ACPI_OBJECT_NAME_OSC  ((ULONG)'CSO_')
#define ACPI_OBJECT_NAME_OST  ((ULONG)'TSO_')
#define ACPI_OBJECT_NAME_PCCH ((ULONG)'HCCP')
#define ACPI_OBJECT_NAME_PR0  ((ULONG)'0RP_')
#define ACPI_OBJECT_NAME_PR1  ((ULONG)'1RP_')
#define ACPI_OBJECT_NAME_PR2  ((ULONG)'2RP_')
#define ACPI_OBJECT_NAME_PR3  ((ULONG)'3RP_')
#define ACPI_OBJECT_NAME_PRS  ((ULONG)'SRP_')
#define ACPI_OBJECT_NAME_PRT  ((ULONG)'TRP_')
#define ACPI_OBJECT_NAME_PRW  ((ULONG)'WRP_')
#define ACPI_OBJECT_NAME_PS0  ((ULONG)'0SP_')
#define ACPI_OBJECT_NAME_PS1  ((ULONG)'1SP_')
#define ACPI_OBJECT_NAME_PS2  ((ULONG)'2SP_')
#define ACPI_OBJECT_NAME_PS3  ((ULONG)'3SP_')
#define ACPI_OBJECT_NAME_PSC  ((ULONG)'CSP_')
#define ACPI_OBJECT_NAME_PSL  ((ULONG)'LSP_')
#define ACPI_OBJECT_NAME_PSV  ((ULONG)'VSP_')
#define ACPI_OBJECT_NAME_PSW  ((ULONG)'WSP_')
#define ACPI_OBJECT_NAME_PTS  ((ULONG)'STP_')
#define ACPI_OBJECT_NAME_REG  ((ULONG)'GER_')
#define ACPI_OBJECT_NAME_RMV  ((ULONG)'VMR_')
#define ACPI_OBJECT_NAME_S0   ((ULONG)'_0S_')
#define ACPI_OBJECT_NAME_S0D  ((ULONG)'D0S_')
#define ACPI_OBJECT_NAME_S0W  ((ULONG)'W0S_')
#define ACPI_OBJECT_NAME_S1   ((ULONG)'_1S_')
#define ACPI_OBJECT_NAME_S1D  ((ULONG)'D1S_')
#define ACPI_OBJECT_NAME_S1W  ((ULONG)'W1S_')
#define ACPI_OBJECT_NAME_S2   ((ULONG)'_2S_')
#define ACPI_OBJECT_NAME_S2D  ((ULONG)'D2S_')
#define ACPI_OBJECT_NAME_S2W  ((ULONG)'W2S_')
#define ACPI_OBJECT_NAME_S3   ((ULONG)'_3S_')
#define ACPI_OBJECT_NAME_S3D  ((ULONG)'D3S_')
#define ACPI_OBJECT_NAME_S3W  ((ULONG)'W3S_')
#define ACPI_OBJECT_NAME_S4   ((ULONG)'_4S_')
#define ACPI_OBJECT_NAME_S4D  ((ULONG)'D4S_')
#define ACPI_OBJECT_NAME_S4W  ((ULONG)'W4S_')
#define ACPI_OBJECT_NAME_S5   ((ULONG)'_5S_')
#define ACPI_OBJECT_NAME_S5D  ((ULONG)'D5S_')
#define ACPI_OBJECT_NAME_S5W  ((ULONG)'W5S_')
#define ACPI_OBJECT_NAME_SCP  ((ULONG)'PCS_')
#define ACPI_OBJECT_NAME_SEG  ((ULONG)'GES_')
#define ACPI_OBJECT_NAME_SI   ((ULONG)'_IS_')
#define ACPI_OBJECT_NAME_SRS  ((ULONG)'SRS_')
#define ACPI_OBJECT_NAME_SST  ((ULONG)'TSS_')
#define ACPI_OBJECT_NAME_STA  ((ULONG)'ATS_')
#define ACPI_OBJECT_NAME_STD  ((ULONG)'DTS_')
#define ACPI_OBJECT_NAME_SUB  ((ULONG)'BUS_')
#define ACPI_OBJECT_NAME_SUN  ((ULONG)'NUS_')
#define ACPI_OBJECT_NAME_SWD  ((ULONG)'DWS_')
#define ACPI_OBJECT_NAME_TC1  ((ULONG)'1CT_')
#define ACPI_OBJECT_NAME_TC2  ((ULONG)'2CT_')
#define ACPI_OBJECT_NAME_TMP  ((ULONG)'PMT_')
#define ACPI_OBJECT_NAME_TSP  ((ULONG)'PST_')
#define ACPI_OBJECT_NAME_TZD  ((ULONG)'DZT_')
#define ACPI_OBJECT_NAME_UID  ((ULONG)'DIU_')
#define ACPI_OBJECT_NAME_WAK  ((ULONG)'KAW_')
#define ACPI_OBJECT_NAME_BBN  ((ULONG)'NBB_')
#define ACPI_OBJECT_NAME_PXM  ((ULONG)'MXP_')
#define ACPI_OBJECT_NAME_PLD  ((ULONG)'DLP_')
#define ACPI_OBJECT_NAME_REV  ((ULONG)'VER_')

#define NAME_NATIVE_METHOD(_Name) (((_Name) == NULL) ? "Unknown" : (_Name))
#define NAME_DEBUG_INFO(_Info) (((_Info) == NULL) ? "" : (_Info))

//
//----------------------------------------------------------------------- Types
//

typedef enum _PEP_MAJOR_DEVICE_TYPE {
    PepMajorDeviceTypeProcessor,
    PepMajorDeviceTypeAcpi,
    PepMajorDeviceTypeMaximum
} PEP_MAJOR_DEVICE_TYPE, *PPEP_MAJOR_DEVICE_TYPE;

C_ASSERT(PepMajorDeviceTypeMaximum <= 0xFF); // 8 bits max

typedef enum _PEP_ACPI_MINOR_DEVICE_TYPE {
    PepAcpiMinorTypeDevice,
    PepAcpiMinorTypePowerResource,
    PepAcpiMinorTypeThermalZone,
    PepAcpiMinorTypeMaximum
} PEP_ACPI_MINOR_DEVICE_TYPE, *PPEP_ACPI_MINOR_DEVICE_TYPE;

C_ASSERT(PepAcpiMinorTypeMaximum <= 0xFF); // 8 bits max

#define PEP_INVALID_DEVICE_TYPE \
    PEP_MAKE_DEVICE_TYPE(PepMajorDeviceTypeMaximum, \
                         PepAcpiMinorTypeMaximum, \
                         0xFFFF)

typedef enum _PEP_NOTIFICATION_HANDLER_RESULT {
    PEP_NOTIFICATION_HANDLER_COMPLETE,
    PEP_NOTIFICATION_HANDLER_MORE_WORK,
    PEP_NOTIFICATION_HANDLER_MAX
} PEP_NOTIFICATION_HANDLER_RESULT,
  *PPEP_NOTIFICATION_HANDLER_RESULT;

typedef
VOID
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE (
    _In_ PVOID Data
    );
typedef PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE
    *PPEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE;

typedef
PEP_NOTIFICATION_HANDLER_RESULT
PEP_NOTIFICATION_HANDLER_ROUTINE (
    _In_ PVOID Data,
    _Out_opt_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );
typedef PEP_NOTIFICATION_HANDLER_ROUTINE
    *PPEP_NOTIFICATION_HANDLER_ROUTINE;

typedef struct _PEP_GENERAL_NOTIFICATION_HANDLER {
    ULONG Notification;
    PPEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE Handler;
    CONST PCHAR Name;
} PEP_GENERAL_NOTIFICATION_HANDLER, *PPEP_GENERAL_NOTIFICATION_HANDLER;

typedef struct _PEP_DEVICE_NOTIFICATION_HANDLER {
    ULONG Notification;
    PPEP_NOTIFICATION_HANDLER_ROUTINE Handler;
    PPEP_NOTIFICATION_HANDLER_ROUTINE WorkerCallbackHandler;
} PEP_DEVICE_NOTIFICATION_HANDLER, *PPEP_DEVICE_NOTIFICATION_HANDLER;

typedef ULONG _PEP_DEVICE_TYPE, PEP_DEVICE_TYPE, *PPEP_DEVICE_TYPE;

typedef struct _PEP_OBJECT_INFORMATION {
    ULONG ObjectName;
    ULONG InputArgumentCount;
    ULONG OutputArgumentCount;
    PEP_ACPI_OBJECT_TYPE ObjectType;
} PEP_OBJECT_INFORMATION, *PPEP_OBJECT_INFORMATION;

struct _PEP_INTERNAL_DEVICE_HEADER;

typedef
NTSTATUS
PEP_DEVICE_CONTEXT_INITIALIZE (
    _In_ struct _PEP_INTERNAL_DEVICE_HEADER *Context
    );
typedef PEP_DEVICE_CONTEXT_INITIALIZE *PPEP_DEVICE_CONTEXT_INITIALIZE;

typedef struct _PEP_DEVICE_DEFINITION {
    PEP_DEVICE_TYPE Type;
    ULONG ContextSize;
    PPEP_DEVICE_CONTEXT_INITIALIZE Initialize;

    //
    // The following fields are valid if device is DPM-owned.
    //

    //
    // The following fields are valid if device is ACPI-owned.
    //

    ULONG ObjectCount;
    _Field_size_(ObjectCount) PPEP_OBJECT_INFORMATION Objects;
    ULONG AcpiNotificationHandlerCount;
    _Field_size_(AcpiNotificationHandlerCount)
        PPEP_DEVICE_NOTIFICATION_HANDLER AcpiNotificationHandlers;
    ULONG DpmNotificationHandlerCount;
    _Field_size_(DpmNotificationHandlerCount)
        PPEP_DEVICE_NOTIFICATION_HANDLER DpmNotificationHandlers;
} PEP_DEVICE_DEFINITION, *PPEP_DEVICE_DEFINITION;

//
// Common device object header
//

typedef struct _PEP_INTERNAL_DEVICE_HEADER {
    LIST_ENTRY ListEntry;
    PEP_DEVICE_TYPE DeviceType;
    POHANDLE KernelHandle;
    PWSTR InstancePath;
    KSPIN_LOCK Lock;
    PPEP_DEVICE_DEFINITION DeviceDefinition;
} PEP_INTERNAL_DEVICE_HEADER, *PPEP_INTERNAL_DEVICE_HEADER;

typedef struct _PEP_PROCESSOR_DEVICE {
    PEP_INTERNAL_DEVICE_HEADER Header;

    //
    // Processor identification
    //

    PROCESSOR_NUMBER Processor;
    ULONG NtNumber;

    //
    // Idle states
    //

    ULONG SelectedIdleState;
    ULONG IdleStateCount;
    PEP_PROCESSOR_IDLE_STATE_V2 IdleStates[10];

    //
    // Performance states
    //

    ULONG CurrentPerformance;

} PEP_PROCESSOR_DEVICE, *PPEP_PROCESSOR_DEVICE;

typedef struct _PEP_ACPI_DEVICE {
    PEP_INTERNAL_DEVICE_HEADER Header;
} PEP_ACPI_DEVICE, *PPEP_ACPI_DEVICE;

typedef enum _PEP_NOTIFICATION_CLASS {
    PEP_NOTIFICATION_CLASS_NONE = 0,
    PEP_NOTIFICATION_CLASS_ACPI = 1,
    PEP_NOTIFICATION_CLASS_DPM = 2,
    PEP_NOTIFICATION_CLASS_PPM = 4
} PEP_NOTIFICATION_CLASS, *PPEP_NOTIFICATION_CLASS;

#define PEP_CHECK_DEVICE_TYPE_ACCEPTED(_Type, _Mask) \
    (((_Type) & (_Mask)) == (_Mask))

typedef enum _PEP_DEVICE_ID_MATCH {
    PepDeviceIdMatchPartial, // Substring match.
    PepDeviceIdMatchFull, // Whole string match.
} PEP_DEVICE_ID_MATCH, *PPEP_DEVICE_ID_MATCH;

typedef struct _PEP_DEVICE_MATCH {
    PEP_DEVICE_TYPE Type;
    PEP_NOTIFICATION_CLASS OwnedType;
    PWSTR DeviceId;
    PEP_DEVICE_ID_MATCH CompareMethod;
} PEP_DEVICE_MATCH, *PPEP_DEVICE_MATCH;

typedef enum _PEP_HANDLER_TYPE {
    PepHandlerTypeSyncCritical,
    PepHandlerTypeWorkerCallback
} PEP_HANDLER_TYPE, *PPEP_HANDLER_TYPE;

typedef struct _PEP_WORK_CONTEXT {

    //
    // Entry of this request on its current (pending or completed) queue.
    //

    LIST_ENTRY ListEntry;

    //
    // Request signature (for validation purposes).
    //

    ULONG Signature;

    //
    // The type of the request.
    //

    PEP_NOTIFICATION_CLASS WorkType;
    ULONG NotificationId;
    BOOLEAN WorkCompleted;

    //
    // The device for which the request is associated with. This value may be
    // NULL if request is tagged as a parent.
    //

    PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice;
    PPEP_DEVICE_DEFINITION DeviceDefinitionEntry;

    //
    // PoFx-supplied PEP_WORK for work requests.
    //

    PEP_WORK_INFORMATION LocalPoFxWorkInfo;

    //
    // Work item context
    //

    PVOID WorkContext;
    SIZE_T WorkContextSize;
    PNTSTATUS WorkRequestStatus;
} PEP_WORK_CONTEXT, *PPEP_WORK_CONTEXT;

typedef struct _PEP_WORK_ITEM_CONTEXT {
    WDFWORKITEM WorkItem;
    PEP_NOTIFICATION_CLASS WorkType;
} PEP_WORK_ITEM_CONTEXT, *PPEP_WORK_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PEP_WORK_ITEM_CONTEXT, PepGetWorkItemContext)

//
//--------------------------------------------------------------------- Globals
//

extern PEP_GENERAL_NOTIFICATION_HANDLER PepAcpiNotificationHandlers[];
extern PEP_GENERAL_NOTIFICATION_HANDLER PepDpmNotificationHandlers[];
extern PEP_KERNEL_INFORMATION PepKernelInformation;
extern LIST_ENTRY PepCompletedWorkList;
extern PEP_DEVICE_DEFINITION PepDeviceDefinitionArray[];
extern ULONG PepDeviceDefinitionArraySize;
extern LIST_ENTRY PepDeviceList;
extern KSPIN_LOCK PepGlobalSpinLock;
extern WDFDEVICE PepGlobalWdfDevice;
extern PEP_DEVICE_MATCH PepDeviceMatchArray[];
extern ULONG PepDeviceMatchArraySize;
extern LIST_ENTRY PepPendingWorkList;
extern BOOLEAN PepRegistered;
extern WDFSPINLOCK PepWorkListLock;

//
//------------------------------------------------------------------ Prototypes
//

//
// pep.c
//

NTSTATUS
PepRegister (
    _In_ BOOLEAN AcpiHandlerOptIn,
    _In_ BOOLEAN DpmHandlerOptIn,
    _In_ BOOLEAN PpmHandlerOptIn
    );

NTSTATUS
PepInitialize (
    _In_ PDRIVER_OBJECT WdmDriverObject,
    _In_ WDFDRIVER WdfDriver,
    _In_ PUNICODE_STRING RegistryPath
    );

__drv_requiresIRQL(PASSIVE_LEVEL)
VOID
PepQueryRegisterOptions (
    _In_ PDRIVER_OBJECT WdmDriverObject,
    _In_ WDFDRIVER WdfDriver,
    _In_ PUNICODE_STRING RegistryPath,
    _Out_ PBOOLEAN PepAcpiOptin,
    _Out_ PBOOLEAN PepDpmOptin,
    _Out_ PBOOLEAN PepPpmOptin
    );

BOOLEAN
PepAcpiNotify (
    _In_ ULONG Notification,
    _In_ PVOID Data
    );

BOOLEAN
PepDpmNotify (
    _In_ ULONG Notification,
    _In_ PVOID Data
    );

BOOLEAN
PepPpmNotify (
    _In_ PEPHANDLE Handle,
    _In_ ULONG Notification,
    _Inout_opt_ PVOID Data
    );

// acpinotify.c

PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiPrepareDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiAbandonDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiRegisterDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiUnregisterDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiEnumerateDeviceNamespace;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiQueryObjectInformation;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiEvaluateControlMethod;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiQueryDeviceControlResources;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiTranslatedDeviceControlResources;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiWorkNotification;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepAcpiIgnoreDeviceNotification;

//
// dpmnotify.c
//

PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepDpmPrepareDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepDpmAbandonDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepDpmRegisterDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepDpmUnregisterDevice;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepDpmIgnoreDeviceNotification;
PEP_GENERAL_NOTIFICATION_HANDLER_ROUTINE PepDpmWorkNotification;

//
// driver.c
//

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

EVT_WDF_DRIVER_DEVICE_ADD PepEvtDeviceAdd;

VOID
PepEvtDriverUnload (
    _In_ WDFDRIVER Driver
    );

//
// util.c
//

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
PepQueryRegistry (
    _Inout_opt_ WDFKEY *ParametersKey,
    _In_opt_ WDFDRIVER WdfDriver,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PCUNICODE_STRING KeyValueName,
    _Out_ PULONG OptionValue
    );

BOOLEAN
PepIsDeviceAccepted (
    _In_ PEP_NOTIFICATION_CLASS OwnedType,
    _In_ PCUNICODE_STRING DeviceId,
    _Out_ PPEP_DEVICE_DEFINITION *DeviceDefinition
    );

BOOLEAN
PepIsDeviceIdMatched (
    _In_ PWSTR String,
    _In_ ULONG StringLength,
    _In_ PWSTR SearchString,
    _In_ ULONG SearchStringLength,
    _In_ PEP_DEVICE_ID_MATCH DeviceIdCompareMethod
    );

VOID
PepScheduleNotificationHandler (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_ ULONG NotificationId,
    _In_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_opt_ PVOID WorkContext,
    _In_ SIZE_T WorkContextSize,
    _In_opt_ PNTSTATUS WorkRequestStatus
    );

VOID
PepInvokeNotificationHandler (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_opt_ PPEP_WORK_CONTEXT WorkRequest,
    _In_ PEP_HANDLER_TYPE HandlerType,
    _In_ ULONG NotificationId,
    _In_opt_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_ PVOID Data,
    _In_ SIZE_T DataSize,
    _In_opt_ PNTSTATUS WorkRequestStatus
    );

PWSTR
PepGetDeviceName (
    _In_ ULONG DeviceType
    );

VOID
PepRequestCommonWork (
    _In_ PPEP_ACPI_REQUEST_CONVERT_TO_BIOS_RESOURCES Request
    );

NTSTATUS
PepGetBiosResourceSize (
    _In_ PPEP_ACPI_RESOURCE ResourceBuffer,
    _In_ ULONG ResourceBufferSize,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PSIZE_T BiosResourceSize
    );

VOID
PepReturnBiosResource (
    _In_ PPEP_ACPI_RESOURCE ResourceBuffer,
    _In_ ULONG ResourceBufferSize,
    _In_ SIZE_T RequiredSize,
    _In_ PACPI_METHOD_ARGUMENT Arguments,
    _Inout_ PSIZE_T BiosResourcesSize,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PNTSTATUS Status
    );

VOID
PepReturnAcpiData (
    _In_ PVOID Value,
    _In_ USHORT ValueType,
    _In_ ULONG ValueLength,
    _In_ BOOLEAN ReturnAsPackage,
    _Out_ PACPI_METHOD_ARGUMENT Arguments,
    _Inout_ PSIZE_T OutputArgumentSize,
    _Out_opt_ PULONG OutputArgumentCount,
    _Out_ PNTSTATUS Status,
    _In_opt_ PCHAR MethodName,
    _In_opt_ PCHAR DebugInfo,
    _Out_ PPEP_NOTIFICATION_HANDLER_RESULT CompleteResult
    );

//
// work.c
//

NTSTATUS
PepCreateWorkRequest (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_ ULONG NotificationId,
    _In_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_ PPEP_DEVICE_DEFINITION DeviceDefinitionEntry,
    _In_opt_ PVOID WorkContext,
    _In_ SIZE_T WorkContextSize,
    _In_opt_ PNTSTATUS WorkRequestStatus,
    _Out_ PPEP_WORK_CONTEXT *OutputWorkRequest
    );

VOID
PepDestroyWorkRequest (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    );

VOID
PepPendWorkRequest (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PepCompleteWorkRequest (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    );

NTSTATUS
PepCompleteSelfManagedWork (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_ ULONG NotificationId,
    _In_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_ PPEP_WORK_INFORMATION PoFxWorkInfo
    );

VOID
PepMarkWorkRequestComplete (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    );

NTSTATUS
PepScheduleWorker (
    _In_ PPEP_WORK_CONTEXT WorkContext
    );

VOID
PepWorkerWrapper (
    _In_ WDFWORKITEM WorkItem
    );

VOID
PepProcessPendingWorkRequests (
    VOID
    );

VOID
PepProcessCompleteWorkRequests (
    _In_ PVOID Data
    );

