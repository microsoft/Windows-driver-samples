#include <windows.h>
#include <wdf.h>

#include "Device.h"

extern "C" DRIVER_INITIALIZE DriverEntry;

// [Required]
// The entry point for the driver.
//
// In general, almost nothing should be done during driver initialization. Instead, almost all resources should
// be associated with a specific device.
//
// https://docs.microsoft.com/windows-hardware/drivers/wdf/driverentry-for-kmdf-drivers
extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    // Specify `DeviceContext::AddDevice` as the `EvtDriverDeviceAdd` callback function
    // for the driver.
    WDF_DRIVER_CONFIG driverConfig;
    WDF_DRIVER_CONFIG_INIT(&driverConfig, DeviceContext::AddDevice);

    // Initialize WDF.
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &driverConfig,
        WDF_NO_HANDLE
        );
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}
