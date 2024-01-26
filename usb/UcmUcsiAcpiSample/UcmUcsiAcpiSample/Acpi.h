/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Acpi.h

Abstract:

    ACPI transport class for UCSI Cx Client.

Environment:

    Kernel-mode.

--*/

#pragma once

namespace UcmUcsiAcpiClient
{

class Ppm;

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
void
EVT_ACPI_NOTIFICATION_CALLBACK(
    _In_ Ppm *PpmObject,
    _In_ ULONG NotifyValue
);

typedef EVT_ACPI_NOTIFICATION_CALLBACK *PFN_ACPI_NOTIFICATION_CALLBACK;

typedef struct
{
    Ppm* m_PpmObject = nullptr;
    PFN_ACPI_NOTIFICATION_CALLBACK m_CallbackFunction = nullptr;
}ACPI_NOTIFICATION_CALLBACK_CONTEXT, *PACPI_NOTIFICATION_CALLBACK_CONTEXT;

class Acpi
{
public:

    typedef struct _CONFIG
    {
        PHYSICAL_ADDRESS DataBlockAddress;
        ULONG DataBlockLength;
    } CONFIG, *PCONFIG;
   

    Acpi() = default;

    ~Acpi() = default;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    Initialize (
        _In_ WDFDEVICE Device
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    PrepareHardware(_In_ PCONFIG Config);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    EnumChildren(_Out_ WDFMEMORY* EnumChildrenOutput);

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    EvaluatePld (
        _In_ LPCSTR DeviceName,
        _Out_ PACPI_PLD_BUFFER PldBuffer
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    CheckIfUsbDeviceControllerIsEnabled (
        _Inout_ bool& IsUsbDeviceControllerEnabled
        );
    
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    ReleaseHardware();

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    SendData(
        _In_ const PUCSI_DATA_BLOCK DataBlock
        );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    ReceiveData(
        _Out_ PUCSI_DATA_BLOCK DataBlockOut
        );

    //template<typename F>
    _IRQL_requires_max_(PASSIVE_LEVEL)
        PAGED_CODE_SEG
        NTSTATUS
        RegisterNotificationCallback(
            _In_ PACPI_NOTIFICATION_CALLBACK_CONTEXT CallbackConext
        )
    {
        PAGED_CODE();
        return RegisterNotificationCallbackInternal(CallbackConext);
    }

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    UnRegisterNotificationCallback();

    //
    // The callback that is registered with ACPI to be called asynchronously when
    // PPM firmware wants to send notification to the OPM.
    //
    static
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    VOID
    NotificationCallback(
        _In_ PVOID Context,
        _In_ ULONG NotifyValue
        );

    FORCEINLINE
    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID
    AcquireCallbackLock()
    {
        WdfWaitLockAcquire(m_CallbackLock, nullptr);
    }

    FORCEINLINE
    _IRQL_requires_max_(PASSIVE_LEVEL)
    VOID
    ReleaseCallbackLock()
    {
        WdfWaitLockRelease(m_CallbackLock);
    }

    FORCEINLINE
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    InvokeCallback(
    _In_ ULONG NotifyValue)
    {
        m_CallbackContext.m_CallbackFunction(m_CallbackContext.m_PpmObject, NotifyValue);
    }

    FORCEINLINE
    _IRQL_requires_max_(PASSIVE_LEVEL)
    PUCSI_DATA_BLOCK
    GetUcsiDataBlock()
    {
        return m_UcsiDataBlock;
    }

private:

    Acpi(_In_ const Acpi&) = delete;
    Acpi& operator =(_In_ const Acpi&) = delete;

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    EvaluateUcsiDsm (
        _In_ ULONG FunctionIndex,
        _Outptr_opt_ PACPI_EVAL_OUTPUT_BUFFER* Output
    );

    _IRQL_requires_max_(PASSIVE_LEVEL)
    PAGED_CODE_SEG
    NTSTATUS
    RegisterNotificationCallbackInternal(
        _In_ PACPI_NOTIFICATION_CALLBACK_CONTEXT
        );

    // The owning client device.
    WDFDEVICE m_OwningDevice = WDF_NO_HANDLE;

    bool m_InterfaceQueried = false;
    ACPI_INTERFACE_STANDARD2 m_AcpiInterface;
    
    WDFWAITLOCK m_CallbackLock = nullptr;

    // The function to be ACPI notificaiton.
    ACPI_NOTIFICATION_CALLBACK_CONTEXT m_CallbackContext;

    // Pointer to the mapped memory that contaisn UCSI Data block
    // that is accessed by OS and firmware.
    PUCSI_DATA_BLOCK m_UcsiDataBlock = nullptr;
    ULONG m_MappedMemoryLength = 0;

};
}