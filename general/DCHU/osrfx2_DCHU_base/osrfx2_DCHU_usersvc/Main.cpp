/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Main.cpp

Abstract:

    Implements the functions to control the OSR USB FX2 device.

Environment:

    User mode

--*/

#include "Main.h"
#include "Utils.h"

//
// Keep track of where the OSRFX2 device's bar graph currently is.
//
INT             CurrentBar;
BAR_GRAPH_STATE BarGraphState;

/*++

Routine Description:

    Sets the variables in this service to their default values.

Arguments:

    VOID

Return Value:

    VOID

--*/
VOID
SetVariables()
{
    CurrentBar = 0;
}


/*++

Routine Description:

    Retrieves the device path of a given interface.

Arguments:

    InterfaceGuid    - The GUID of the interface to search for

    DevicePath       - The resulting device path

    DevicePathLength - The length of DevicePath

Return Value:

    TRUE if the function succeeded and FALSE otherwise.  Errors
    are logged in the Application event log.

--*/
_Success_(return)
BOOL
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_writes_z_(DevicePathLength) PWCHAR DevicePath,
    _In_ size_t DevicePathLength
    )
{
    HRESULT hr = E_FAIL;
    CONFIGRET cr = CR_SUCCESS;
    PWSTR DeviceInterfaceList = NULL;
    ULONG DeviceInterfaceListLength = 0;
    PWSTR NextInterface;

    //
    // Determine if there are any interfaces that match the OSRFX2 device.
    //
    cr = CM_Get_Device_Interface_List_Size(&DeviceInterfaceListLength,
                                           InterfaceGuid,
                                           NULL,
                                           CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (cr != CR_SUCCESS)
    {
        WriteToErrorLog(L"CM_Get_DeviceInterface_List_Size",
                        CM_MapCrToWin32Err(cr, ERROR_FILE_NOT_FOUND));
        goto cleanup;
    }

    if (DeviceInterfaceListLength < 1)
    {
        WriteToErrorLog(L"CM_Get_DeviceInterface_List_Size",
                        CM_MapCrToWin32Err(cr, ERROR_EMPTY));
        goto cleanup;
    }

    DeviceInterfaceList = (PWSTR)malloc(DeviceInterfaceListLength * sizeof(WCHAR));

    if (DeviceInterfaceList == NULL)
    {
        WriteToEventLog(L"Failed to allocate memory for the device interface list",
                        TRACE_LEVEL_ERROR);
        goto cleanup;
    }

    cr = CM_Get_Device_Interface_List(InterfaceGuid,
                                      NULL,
                                      DeviceInterfaceList,
                                      DeviceInterfaceListLength,
                                      CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (cr != CR_SUCCESS)
    {
        WriteToErrorLog(L"CM_Get_Device_Interface_List",
                        CM_MapCrToWin32Err(cr, ERROR_FILE_NOT_FOUND));
        goto cleanup;
    }

    if (*DeviceInterfaceList == UNICODE_NULL)
    {
        WriteToEventLog(L"CM_Get_Device_Interface_List returned an empty list",
                        TRACE_LEVEL_ERROR);
    }

    //
    // This sample only expects one interface for the OSRFX2 device.  For other
    // devices, though, it maybe necessary to sift through the interfaces
    // from CM_Get_Device_Interface_List in order to find the correct device.
    //
    NextInterface = DeviceInterfaceList + wcslen(DeviceInterfaceList) + 1;

    if (*NextInterface != UNICODE_NULL)
    {
        WriteToEventLog(L"More than one device interface instance found.  "
                        "Selecting first matching device.",
                        TRACE_LEVEL_WARNING);
    }

    hr = StringCchCopy(DevicePath, DevicePathLength, DeviceInterfaceList);

    if (FAILED(hr))
    {
        WriteToErrorLog(L"StringCchCopy", HRESULT_CODE(hr));
        goto cleanup;
    }

cleanup:

    if (DeviceInterfaceList != NULL)
    {
        free(DeviceInterfaceList);
    }

    return (cr == CR_SUCCESS);
}


/*++

Routine Description:

    Opens up the OSR USB FX2 device handle.

Arguments:

    Synchronous - Whether or not this device should be opened for syncrhonous
                  access

Return Value:

    The handle to the OSR USB FX2 device.

--*/
_Check_return_
_Ret_notnull_
_Success_(return != INVALID_HANDLE_VALUE)
HANDLE
OpenDevice(
    _In_ BOOL Synchronous
    )
{
    HANDLE DeviceHandle = INVALID_HANDLE_VALUE;
    WCHAR DeviceName[MAX_DEVPATH_LENGTH];

    if (!GetDevicePath((LPGUID)&GUID_DEVINTERFACE_OSRUSBFX2,
                       DeviceName,
                       sizeof(DeviceName) / sizeof(DeviceName[0])))
    {
        goto cleanup;
    }

    //
    // Open a handle to the interface.
    //
    if (Synchronous)
    {
        DeviceHandle = CreateFile(DeviceName,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  NULL, // default security
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    }
    else
    {
        DeviceHandle = CreateFile(DeviceName,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  NULL, // default security
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                  NULL);
    }

    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        WriteToErrorLog(L"CreateFile", GetLastError());
    }
    else
    {
        WriteToEventLog(L"Opened Device Successfully", TRACE_LEVEL_INFORMATION);
    }

cleanup:

    return DeviceHandle;
}


/*++

Routine Description:

    Handles an interface arrival notification.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
InterfaceArrivalAction(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    //
    // Now that the interface has arrived, open a handle to it, and then
    // register that handle for device events.
    //
    EnterCriticalSection(&Context->Lock);

    if (Context->DeviceInterfaceHandle != INVALID_HANDLE_VALUE)
    {
        //
        // The handle was already retrieved.
        //
        Err = ERROR_SUCCESS;
        goto cleanup;
    }

    Context->DeviceInterfaceHandle = OpenDevice(FALSE);

    if (Context->DeviceInterfaceHandle == INVALID_HANDLE_VALUE)
    {
        Err = GetLastError();
        WriteToErrorLog(L"Could not open device interface", Err);
        goto cleanup;
    }

    Err = RegisterDeviceNotifications(Context);

    if (Err != ERROR_SUCCESS)
    {
        WriteToErrorLog(L"Could not register device notifications", Err);
        goto cleanup;
    }

cleanup:

    LeaveCriticalSection(&Context->Lock);

    return Err;
}


/*++

Routine Description:

    Handles an interface arrival notification.

Arguments:

    hNotify       - The notification that fired the callback

    hContext      - The callback context

    Action        - The type of notification

    EventData     - Additional information about the callback

    EventDataSize - The size of EventData

Return Value:

    A Win32 error code.

--*/
DWORD
InterfaceCallback(
    _In_ HCMNOTIFICATION       hNotify,
    _In_ PVOID                 hContext,
    _In_ CM_NOTIFY_ACTION      Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD                 EventDataSize
    )
{
    DWORD Err = ERROR_SUCCESS;
    PDEVICE_CONTEXT Context = (PDEVICE_CONTEXT)hContext;

    //
    // Validate Context.
    //
    if (Context == NULL)
    {
        goto cleanup;
    }

    if (Action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL)
    {
        Err = InterfaceArrivalAction(Context);
    }

cleanup:

    return Err;
}


/*++

Routine Description:

    Registers the service for notifications using the notification handle in
    Context.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
RegisterInterfaceNotifications(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    CONFIGRET cr;
    CM_NOTIFY_FILTER NotifyFilter = {0};

    if (Context == NULL)
    {
        goto cleanup;
    }

    ZeroMemory(&NotifyFilter, sizeof(NotifyFilter));
    NotifyFilter.cbSize = sizeof(NotifyFilter);
    NotifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    NotifyFilter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_OSRUSBFX2;

    cr = CM_Register_Notification(&NotifyFilter,
                                  (PVOID)Context,
                                  (PCM_NOTIFY_CALLBACK)InterfaceCallback,
                                  &Context->InterfaceNotificationHandle);

    if (cr != CR_SUCCESS)
    {
        Err = CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA);
        WriteToErrorLog(L"CM_Register_Notification", Err);
        goto cleanup;
    }

cleanup:

    return Err;
}


/*++

Routine Description:

    Unregister for interface notifications.  Note, this routine deadlocks
    when called from an interface callback.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
UnregisterInterfaceNotifications(
    _In_ PDEVICE_CONTEXT Context
    )
{
    CONFIGRET cr;

    if (Context->InterfaceNotificationHandle != NULL)
    {
        cr = CM_Unregister_Notification(Context->InterfaceNotificationHandle);

        Context->InterfaceNotificationHandle = NULL;
    }

    return CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA);
}


/*++

Routine Description:

    Callback for when a device is being query removed.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
DeviceQueryRemoveAction(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    EnterCriticalSection(&Context->Lock);

    if (Context->DeviceInterfaceHandle != INVALID_HANDLE_VALUE)
    {
        //
        // Close open handles to allow the device to exit
        //
        CloseHandle(Context->DeviceInterfaceHandle);

        Context->DeviceInterfaceHandle = INVALID_HANDLE_VALUE;
    }

    LeaveCriticalSection(&Context->Lock);

    return Err;
}


/*++

Routine Description:

    This callback avoids a deadlock when unregistering device notifications.
    Rather than calling CM_Unregister_Notification from the callback, the
    callback gives that work to a separate thread to avoid deadlock.

Arguments:

    Instance - The thread's callback instance

    hContext - The callback context

    pWork    - The thread handle

Return Value:

    VOID

--*/
VOID
CALLBACK
UnregisterWorkerThreadCallback(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 hContext,
    _Inout_     PTP_WORK              pWork
    )
{
    PDEVICE_CONTEXT Context = (PDEVICE_CONTEXT)hContext;

    EnterCriticalSection(&Context->Lock);

    UnregisterDeviceNotifications(Context);

    //
    // Close the device handle.
    //
    if (Context->DeviceInterfaceHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Context->DeviceInterfaceHandle);

        Context->DeviceInterfaceHandle = INVALID_HANDLE_VALUE;
    }

    LeaveCriticalSection(&Context->Lock);
}


/*++

Routine Description:

    Handles a device query remove failed notification.

Arguments:

    hNotify - The notification that spurred this callback

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
DeviceQueryRemoveFailedAction(
    _In_ HCMNOTIFICATION hNotify,
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    EnterCriticalSection(&Context->Lock);

    //
    // In case this callback fires before the registration call returns, make
    // sure the notification handle is properly set.
    //
    Context->InterfaceNotificationHandle = hNotify;

    //
    // Unregister the device callback, and then close the handle
    //
    if (!Context->Unregister)
    {
        Context->Unregister = TRUE;
        SubmitThreadpoolWork(Context->Work);
    }

    LeaveCriticalSection(&Context->Lock);

    //
    // Wait for the callback and then re-register the device
    //
    WaitForThreadpoolWorkCallbacks(Context->Work, FALSE);

    EnterCriticalSection(&Context->Lock);

    if (Context->DeviceInterfaceHandle == INVALID_HANDLE_VALUE)
    {
        Context->DeviceInterfaceHandle = OpenDevice(FALSE);
    }

    if (Context->DeviceInterfaceHandle != INVALID_HANDLE_VALUE)
    {
        RegisterDeviceNotifications(Context);
    }

    LeaveCriticalSection(&Context->Lock);

    return Err;
}


/*++

Routine Description:

    Handles a device remove pending notification.

Arguments:

    hNotify - The notification that spurred this callback

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
DeviceRemovePendingAction(
    _In_ HCMNOTIFICATION hNotify,
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    EnterCriticalSection(&Context->Lock);

    //
    // In case this callback fires before the registration call returns, make
    // sure the notification handle is properly set.
    //
    Context->InterfaceNotificationHandle = hNotify;

    //
    // Unregister the device callback, and then close the handle
    //
    if (!Context->Unregister)
    {
        Context->Unregister = TRUE;
        SubmitThreadpoolWork(Context->Work);
    }

    LeaveCriticalSection(&Context->Lock);

    return Err;
}


/*++

Routine Description:

    Handles a device remove complete notification.

Arguments:

    hNotify - The notification that spurred this callback

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
DeviceRemoveCompleteAction(
    _In_ HCMNOTIFICATION hNotify,
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    EnterCriticalSection(&Context->Lock);

    //
    // In case this callback fires before the registration call returns, make
    // sure the notification handle is properly set.
    //
    Context->InterfaceNotificationHandle = hNotify;

    //
    // Unregister the device callback, and then close the handle
    //
    if (!Context->Unregister)
    {
        Context->Unregister = TRUE;
        SubmitThreadpoolWork(Context->Work);
    }

    LeaveCriticalSection(&Context->Lock);

    return Err;
}


/*++

Routine Description:

    Handles device notifications.

Arguments:

    hNotify       - The notification that spurred this callback

    hContext      - The callback context

    Action        - The type of callback

    EventData     - Additional information about this callback

    EventDataSize - The size of EventData

Return Value:

    A Win32 error code.

--*/
DWORD
DeviceCallback(
    _In_ HCMNOTIFICATION       hNotify,
    _In_ PVOID                 hContext,
    _In_ CM_NOTIFY_ACTION      Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD                 EventDataSize
    )
{
    DWORD Err = ERROR_SUCCESS;
    PDEVICE_CONTEXT Context = (PDEVICE_CONTEXT)hContext;

    //
    // Validate Context.
    //
    if (Context == NULL)
    {
        goto cleanup;
    }

    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEQUERYREMOVE:
        DeviceQueryRemoveAction(Context);
        break;

    case CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED:
        DeviceQueryRemoveFailedAction(hNotify, Context);
        break;

    case CM_NOTIFY_ACTION_DEVICEREMOVEPENDING:
        DeviceRemovePendingAction(hNotify, Context);
        break;

    case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE:
        DeviceRemoveCompleteAction(hNotify, Context);
        break;
    }

cleanup:

    return Err;
}


/*++

Routine Description:

    Register for device notifications.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
RegisterDeviceNotifications(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    CONFIGRET cr;
    CM_NOTIFY_FILTER NotifyFilter = {0};

    NotifyFilter.cbSize = sizeof(NotifyFilter);
    NotifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
    NotifyFilter.u.DeviceHandle.hTarget = Context->DeviceInterfaceHandle;

    cr = CM_Register_Notification(&NotifyFilter,
                                  (PVOID)Context,
                                  (PCM_NOTIFY_CALLBACK)DeviceCallback,
                                  &Context->DeviceNotificationHandle);

    if (cr != CR_SUCCESS)
    {
        Err = CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA);
        WriteToEventLog(L"Could not register for notifications", TRACE_LEVEL_WARNING);
        goto cleanup;
    }

    Context->Unregister = FALSE;

cleanup:

    return Err;
}


/*++

Routine Description:

    Unregister for device notifications.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
UnregisterDeviceNotifications(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    CONFIGRET cr;

    if (Context->DeviceNotificationHandle != NULL)
    {
        cr = CM_Unregister_Notification(Context->DeviceNotificationHandle);

        if (cr != CR_SUCCESS)
        {
            Err = CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA);
            WriteToEventLog(L"Could not unregister notifications", TRACE_LEVEL_WARNING);
        }

        Context->DeviceNotificationHandle = NULL;
    }

    return Err;
}


/*++

Routine Description:

    Initialize the given PDEVICE_CONTEXT.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
InitializeContext(
    _Out_ PDEVICE_CONTEXT *Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    BOOL LockInitialized = FALSE;
    BOOL LockEntered = FALSE;
    BOOL InterfaceNotificationsInitialized = FALSE;
    BOOL DeviceNotificationsInitialized = FALSE;
    PDEVICE_CONTEXT DeviceContext;

    DeviceContext = (PDEVICE_CONTEXT)malloc(sizeof(DEVICE_CONTEXT));

    if (DeviceContext == NULL)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    DeviceContext->DeviceInterfaceHandle = INVALID_HANDLE_VALUE;
	DeviceContext->LockEnabled = FALSE;
	DeviceContext->InterfaceNotificationsEnabled = FALSE;
	DeviceContext->DeviceNotificationsEnabled = FALSE;

    InitializeCriticalSection(&DeviceContext->Lock);
	DeviceContext->LockEnabled = TRUE;

    DeviceContext->Work = CreateThreadpoolWork(UnregisterWorkerThreadCallback, (PVOID)DeviceContext, NULL);

    if (DeviceContext->Work == NULL)
    {
        Err = GetLastError();
        WriteToErrorLog(L"Could not create worker thread callback", Err);
        goto cleanup;
    }

    DeviceContext->DeviceNotificationHandle = NULL;
    DeviceContext->InterfaceNotificationHandle = NULL;

    //
    // Register for device interface events to open and close the handle to
    // the interface.
    //
    Err = RegisterInterfaceNotifications(DeviceContext);

    if (Err != ERROR_SUCCESS)
    {
        WriteToErrorLog(L"Could not register notifications", Err);
        goto cleanup;
    }

	DeviceContext->InterfaceNotificationsEnabled = TRUE;

    EnterCriticalSection(&DeviceContext->Lock);
    LockEntered = TRUE;

    //
    // The interface may already have arrived while registering for
    // notifications.  The lock could be moved earlier, but for sample
	// purposes this is the proper way to initialize notifications.
    //
    if (DeviceContext->DeviceInterfaceHandle == INVALID_HANDLE_VALUE)
    {
        DeviceContext->DeviceInterfaceHandle = OpenDevice(FALSE);
    }

    if (DeviceContext->DeviceInterfaceHandle != INVALID_HANDLE_VALUE)
    {
        Err = RegisterDeviceNotifications(DeviceContext);

        if (Err != ERROR_SUCCESS)
        {
            WriteToErrorLog(L"Could not register device notifications", Err);
            goto cleanup;
        }

		DeviceContext->DeviceNotificationsEnabled = TRUE;
    }

    //
    // If OpenDevice ends up returning INVALID_HANDLE_VALUE, that's fine
    // since a notification for the interface will arrive later.
    //

cleanup:

    if (LockEntered)
    {
        LeaveCriticalSection(&DeviceContext->Lock);
    }

	*Context = DeviceContext;
	DeviceContext = NULL;

    if (DeviceContext != NULL)
    {
		CloseContext(DeviceContext);
    }

    return Err;
}


/*++

Routine Description:

    Clean up the given PDEVICE_CONTEXT.

Arguments:

    Context - The callback context

Return Value:

    A Win32 error code.

--*/
DWORD
CloseContext(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    BOOL Unregister = FALSE;

    if (Context == NULL)
    {
        //
        // Nothing to remove.
        //
        goto cleanup;
    }

    EnterCriticalSection(&Context->Lock);

    if (!Context->Unregister)
    {
        //
        // Unregister from the callback here.
        //
        Unregister = TRUE;
        Context->Unregister = TRUE;
    }

    LeaveCriticalSection(&Context->Lock);

    //
    // Unregister from the interface first, so that re-appearance of the interface
    // doesn't cause us to register device events again.
    //
	if (Context->InterfaceNotificationsEnabled)
	{
		Err = UnregisterInterfaceNotifications(Context);

		if (Err != ERROR_SUCCESS)
		{
			WriteToErrorLog(L"Could not unregister interface notifications", Err);
		}
	}

    if (Unregister)
    {
		if (Context->DeviceNotificationsEnabled)
		{
			Err = UnregisterDeviceNotifications(Context);

			if (Err != ERROR_SUCCESS)
			{
				WriteToErrorLog(L"Could not unregister device notifications", Err);
			}
		}
    }
    else
    {
        WaitForThreadpoolWorkCallbacks(Context->Work, FALSE);
    }

    //
    // No need to lock here, UnregisterDeviceNotifications will wait for all
    // outstanding callbacks before returning.
    //
    if (Context->DeviceInterfaceHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Context->DeviceInterfaceHandle);

        Context->DeviceInterfaceHandle = INVALID_HANDLE_VALUE;
    }

    if (Context->Work != NULL)
    {
        CloseThreadpoolWork(Context->Work);
    }

    DeleteCriticalSection(&Context->Lock);

    free(Context);

cleanup:

    return Err;
}

/*++

Routine Description:

    Turns off all of the bar graph lights on the OSR USB FX2 device.

Arguments:

    Context - The callback context

Return Value:

    VOID

--*/
DWORD
ClearAllBars(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    ULONG BytesReturned;

    BarGraphState.BarsAsUChar = 0;

    if (!DeviceIoControl(Context->DeviceInterfaceHandle,
                         IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                         &BarGraphState,          // Pointer to InBuffer
                         sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                         NULL,                    // Pointer to OutBuffer
                         0,                       // Length of OutBuffer
                         &BytesReturned,          // BytesReturned
                         0))                      // Pointer to Overlapped structure
    {
        Err = GetLastError();
        WriteToErrorLog(L"DeviceIOControl", Err);
        goto cleanup;
    }

cleanup:

    return Err;
}


/*++

Routine Description:

    Lights the next bar on the OSR USB FX2 device.

Arguments:

    Context - The callback context

Return Value:

    VOID

--*/
DWORD
LightNextBar(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;
    ULONG BytesReturned;

    //
    // Normalize to 0-7
    //
    CurrentBar += 1;

    if (CurrentBar > 7)
    {
        CurrentBar = 0;
    }

    BarGraphState.BarsAsUChar = 1 << (UCHAR)CurrentBar;

    if (!DeviceIoControl(Context->DeviceInterfaceHandle,
                         IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                         &BarGraphState,          // Pointer to InBuffer
                         sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                         NULL,                    // Pointer to OutBuffer
                         0,                       // Length of OutBuffer
                         &BytesReturned,          // BytesReturned
                         0))                      // Pointer to Overlapped structure
    {
        Err = GetLastError();
        WriteToErrorLog(L"DeviceIOControl", Err);
        goto cleanup;
    }

cleanup:

    return Err;
}

/*++

Routine Description:

    Lights the next bar on the OSRFX2 device.

Arguments:

    Context - The device context

Return Value:

    A Win32 error code.

--*/
DWORD
ControlDevice(
    _In_ PDEVICE_CONTEXT Context
    )
{
    DWORD Err = ERROR_SUCCESS;

    EnterCriticalSection(&Context->Lock);

    Err = ClearAllBars(Context);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    Err = LightNextBar(Context);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

cleanup:

	LeaveCriticalSection(&Context->Lock);

    return Err;
}