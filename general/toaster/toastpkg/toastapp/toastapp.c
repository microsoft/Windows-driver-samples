/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    toastapp.c

Abstract:

    TOASTAPP is an application that provides an automatically updated list of
    all currently available "toaster" devices (as enumerated by the toaster
    sample in the Windows 2000 and Windows XP DDKs).

    The toasters' friendly names, along with their pathnames (i.e., for use
    with CreateFile) are displayed in the dialog box.

Notes:

    For a complete description of device interfaces and PnP event notification,
    please see the Microsoft Windows 2000/Windows XP DDK and SDK Documentation.

--*/

#include "precomp.h"
#pragma hdrstop

//
// Instantiate toaster device interface class GUID (from DDK toaster sample,
// src\general\toaster\bus\common.h)
//
//     {781EF630-72B2-11d2-B852-00C04FAD5171}
//

#include <initguid.h>

DEFINE_GUID (GUID_TOASTER_INTERFACE, 0x781EF630, 0x72B2, 0x11d2, 0xB8, 0x52, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);

//
// Declare a global string buffer to be used when retrieving error text via
// FormatMessage.
//
TCHAR ErrorStringBuffer[1024];

//
// Define structure used to store device interface dialogbox data.
//
typedef struct _DIDLG_DATA {
    HDEVINFO   DeviceInfoSet;
    HDEVNOTIFY hDevNotify;
    GUID       InterfaceClassGuid;
} DIDLG_DATA, *PDIDLG_DATA;

//
// Function prototypes
//
INT_PTR
CALLBACK
DeviceInterfaceDlgProc(
    _In_ HWND hwnd,
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOL
FillInDeviceInterfaceListBox(
    _In_ HWND        hWnd,
    _In_ HDEVINFO    DeviceInfoSet,
    _In_ CONST GUID *InterfaceClassGuid
    );

BOOL
GetDeviceInterfaceFriendlyName(
    _In_  HDEVINFO                  DeviceInfoSet,
    _In_  PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _Out_writes_all_(FriendlyNameSize) PTSTR FriendlyName,
    _In_  DWORD                     FriendlyNameSize
    );

//
// Implementation
//

int
__cdecl
_tmain(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    INT_PTR DlgResult;
    DIDLG_DATA DIDlgData;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //
    // Initialize our dialogbox data structure.
    //
    ZeroMemory(&DIDlgData, sizeof(DIDlgData));

    CopyMemory(&(DIDlgData.InterfaceClassGuid),
               &GUID_TOASTER_INTERFACE,
               sizeof(GUID)
              );

    //
    // Now fire off the dialog that will present the automatically-updated list
    // of active device interfaces.
    //
    DlgResult = DialogBoxParam(GetModuleHandle(NULL),
                               MAKEINTRESOURCE(IDD_DEVICE_INTERFACES),
                               NULL,
                               DeviceInterfaceDlgProc,
                               (LPARAM)&DIDlgData
                              );
    if(!DlgResult) {
        if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                         NULL,
                         GetLastError(),
                         0,
                         ErrorStringBuffer,
                         sizeof(ErrorStringBuffer) / sizeof(TCHAR),
                         NULL)) {

            _tprintf(TEXT("%s"), ErrorStringBuffer);
        }
        return -1;

    } else {
        return 0;
    }
}


INT_PTR
CALLBACK
DeviceInterfaceDlgProc(
    _In_ HWND hWnd,
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )

/*++

Routine Description:

    This is the dialog procedure for the device interface dialog box that
    presents an automatically-updated list of active device interfaces.

    It expects to get an lParam during WM_INITDIALOG that is a pointer to a
    DIDLGDATA structure where the InterfaceClassGuid field is initialized to
    the interface class GUID for which the device interface list is to be
    displayed.  (The other fields in this structure are initialized, used, and
    destroyed during the lifetime of the dialogbox.)

--*/

{
    PDIDLG_DATA DIDlgData;
    DWORD Err;
    PDEV_BROADCAST_DEVICEINTERFACE DevBroadcastDeviceInterface;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;

    if(msg == WM_INITDIALOG) {

        DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
        HDEVINFO NewDeviceInfoSet;

        DIDlgData = (PDIDLG_DATA)lParam;

        //
        // Create a device information set that will be the container for our
        // device interfaces.
        //
        DIDlgData->DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

        if(DIDlgData->DeviceInfoSet == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            _tprintf(TEXT("SetupDiCreateDeviceInfoList failed with %lx\n"), Err);
            goto clean0;
        }

        //
        // Now register to begin receiving notifications for the comings
        // and goings of device interfaces which are members of the
        // interface class whose GUID was passed in as the lParam to this
        // dialog procedure.
        //
        ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
        NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        CopyMemory(&(NotificationFilter.dbcc_classguid), &(DIDlgData->InterfaceClassGuid), sizeof(GUID));

        DIDlgData->hDevNotify = RegisterDeviceNotification(hWnd,
                                                           &NotificationFilter,
                                                           DEVICE_NOTIFY_WINDOW_HANDLE
                                                          );
        if(!DIDlgData->hDevNotify) {
            Err = GetLastError();
            _tprintf(TEXT("RegisterDeviceNotification failed with %lx\n"), Err);
            goto clean1;
        }

        //
        // OK, now we can retrieve the existing list of active device
        // interfaces into the device information set we created above.
        //
        NewDeviceInfoSet = SetupDiGetClassDevsEx(&(DIDlgData->InterfaceClassGuid),
                                                 NULL,
                                                 NULL,
                                                 DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
                                                 DIDlgData->DeviceInfoSet,
                                                 NULL,
                                                 NULL
                                                );

        if(NewDeviceInfoSet == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            _tprintf(TEXT("SetupDiGetClassDevsEx failed with %lx\n"), Err);
            goto clean2;
        }

        //
        // If SetupDiGetClassDevsEx succeeds and it was passed in an
        // existing device information set to be used, then the HDEVINFO
        // it returns is the same as the one it was passed in.  Thus, we
        // can just use the original DeviceInfoSet handle from here on.
        //

        //
        // Now fill in our listbox with the current device interface list.
        //
        if(!FillInDeviceInterfaceListBox(hWnd,
                                         DIDlgData->DeviceInfoSet,
                                         &(DIDlgData->InterfaceClassGuid))) {
            Err = GetLastError();
            goto clean2;
        }

        //
        // Success!  Store away a pointer to our device interface dialog data
        // structure.
        //
        SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)DIDlgData);
        return TRUE;

        //
        // Clean-up code for error path...
        //
clean2:
        UnregisterDeviceNotification(DIDlgData->hDevNotify);
clean1:
        SetupDiDestroyDeviceInfoList(DIDlgData->DeviceInfoSet);
clean0:
        if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                         NULL,
                         HRESULT_FROM_SETUPAPI(Err),
                         0,
                         ErrorStringBuffer,
                         sizeof(ErrorStringBuffer) / sizeof(TCHAR),
                         NULL)) {

            _tprintf(TEXT("%s"), ErrorStringBuffer);
        }

        EndDialog(hWnd, 0);
        SetLastError(Err);
        return TRUE;
    } else {
        //
        // For the small set of messages that we get before WM_INITDIALOG, we
        // won't have a devwizdata pointer!
        //
        DIDlgData = (PDIDLG_DATA)GetWindowLongPtr(hWnd, DWLP_USER);
        if(DIDlgData == NULL) {
            //
            // If we haven't gotten a WM_INITDIALOG message yet, or if for some
            // reason we weren't able to retrieve the DIDlgData pointer when we
            // did, then we simply return FALSE.
            //
            return FALSE;
        }
    }

    switch(msg) {

        case WM_COMMAND:
            if(LOWORD(wParam) == IDOK) {
                //
                // Clean up and return.
                //
                UnregisterDeviceNotification(DIDlgData->hDevNotify);
                SetupDiDestroyDeviceInfoList(DIDlgData->DeviceInfoSet);
                SetWindowLongPtr(hWnd, DWLP_USER, 0);
                EndDialog(hWnd, 1);
                return TRUE;
            }

            //
            // All other WM_COMMAND messages unhandled.
            //
            break;

        case WM_DEVICECHANGE:
            //
            // All the events we're interested in come with lParam pointing to
            // a structure headed by a DEV_BROADCAST_HDR.  This is denoted by
            // bit 15 of wParam being set, and bit 14 being clear.
            //
            if((wParam & 0xC000) == 0x8000) {
                //
                // Make sure that this is a device interface notification...
                //
                if(((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                    DevBroadcastDeviceInterface = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
                } else {
                    //
                    // This isn't a device interface notification.  Instead,
                    // it's a broadcasted notification sent for backwards-
                    // compatibility (e.g., DBT_DEVTYP_VOLUME).
                    //
                    break;
                }

                if(wParam == DBT_DEVICEARRIVAL) {
                    _tprintf(TEXT("Received DBT_DEVICEARRIVAL for %s\n"), DevBroadcastDeviceInterface->dbcc_name);
                } else if(wParam == DBT_DEVICEREMOVEPENDING) {
                    _tprintf(TEXT("Received DBT_DEVICEREMOVEPENDING for %s\n"), DevBroadcastDeviceInterface->dbcc_name);
                } else if(wParam == DBT_DEVICEREMOVECOMPLETE) {
                    _tprintf(TEXT("Received DBT_DEVICEREMOVECOMPLETE for %s\n"), DevBroadcastDeviceInterface->dbcc_name);
                } else {
                    //
                    // Presently, there are no other events that are sent for
                    // device interface notification, thus we should never get
                    // here.
                    //
                    break;
                }

            } else {
                //
                // We received some broadcasted system message we don't care
                // about (e.g., DBT_QUERYCHANGECONFIG).
                //
                break;
            }

            if(wParam == DBT_DEVICEARRIVAL) {
                //
                // Open this new device interface into our device information
                // set.
                //
                if(!SetupDiOpenDeviceInterface(DIDlgData->DeviceInfoSet,
                                               DevBroadcastDeviceInterface->dbcc_name,
                                               0,
                                               NULL)) {
                    Err = GetLastError();
                    _tprintf(TEXT("SetupDiOpenDeviceInterface failed with %lx\n"), Err);
                    return TRUE;
                }

            } else {
                //
                // First, locate this device interface in our device information
                // set.
                //
                DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
                if(SetupDiOpenDeviceInterface(DIDlgData->DeviceInfoSet,
                                              DevBroadcastDeviceInterface->dbcc_name,
                                              DIODI_NO_ADD,
                                              &DeviceInterfaceData)) {

                    if(!SetupDiDeleteDeviceInterfaceData(DIDlgData->DeviceInfoSet,
                                                         &DeviceInterfaceData)) {

                        Err = GetLastError();
                        _tprintf(TEXT("SetupDiDeleteDeviceInterfaceData failed with %lx\n"), Err);
                        return TRUE;
                    }
                }
            }

            //
            // If we get to here, we've successfully added or deleted a
            // device interface in our device information set.  Now go update
            // our listbox with the new list.  (Ignore any errors.)
            //
            FillInDeviceInterfaceListBox(hWnd,
                                         DIDlgData->DeviceInfoSet,
                                         &(DIDlgData->InterfaceClassGuid)
                                        );

            return TRUE;

        default:
            break;
    }

    return FALSE;
}


BOOL
FillInDeviceInterfaceListBox(
    _In_ HWND        hWnd,
    _In_ HDEVINFO    DeviceInfoSet,
    _In_ CONST GUID *InterfaceClassGuid
    )

/*++

Routine Description:

    This routine fills in the listbox of currently-active device interfaces
    with their corresponding friendly names and pathnames.

Arguments:

    hWnd - Supplies the window handle of the dialog box containing the device
        interface listbox to be updated.

    DeviceInfoSet - Supplies a handle to the device information set containing
        device interfaces to be used in updating the listbox.

    InterfaceClassGuid - Supplies the address of the interface class GUID for
        the device interfaces to be placed into the listbox.

Return Value:

    If the function succeeds, the return value is non-zero.
    If the function fails, the return value is FALSE.  To find out what the
    cause of failure was, call GetLastError().

--*/

{
    DWORD i, Err;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    PTSTR FriendlyName;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;
    PBYTE Buffer;
    DWORD BufferSize = 0;
    DWORD RequiredSize;
    LRESULT ListBoxReturn;
    size_t FriendlyNameLen;
    size_t devicePathLen;

    //
    // Reset the listbox in preparation for adding the current list of device
    // interfaces.
    //
    SendDlgItemMessage(hWnd,
                       IDC_DEVICE_INTERFACE_LIST,
                       LB_RESETCONTENT,
                       0,
                       0
                      );

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    Err = NO_ERROR;

    //
    // Start out with a buffer that should be large enough to hold the friendly
    // name plus the device interface detail data for a "reasonably-sized"
    // device interface pathname.  Note that device interface paths aren't
    // confined to MAX_PATH length on Windows 2000, so we deal with the case
    // where we may need a larger buffer.
    //
    // Note that we add a space and an open paren between the friendly name and
    // the pathname.  We have space for this (even in Unicode), because the
    // device interface detail data buffer always begins with a DWORD cbSize
    // field, that we can overwrite, without touching the character DevicePath
    // buffer.
    //
    BufferSize = (LINE_LEN * sizeof(TCHAR)) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)
                 + (MAX_PATH * sizeof(TCHAR));

    Buffer = malloc(BufferSize);

    if(Buffer) {
        //
        // Leave the first LINE_LEN characters to retrieve the friendly name
        // into...
        //
        FriendlyName = (PTSTR)Buffer;
        DeviceInterfaceDetailData =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA)(Buffer + (LINE_LEN * sizeof(TCHAR)));

        DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    } else {
        //
        // Failure!
        //
        _tprintf(TEXT("Couldn't allocate %d bytes for device interface detail buffer\n"),
                 BufferSize
                );

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for(i = 0;
        SetupDiEnumDeviceInterfaces(DeviceInfoSet,
                                    NULL,
                                    InterfaceClassGuid,
                                    i,
                                    &DeviceInterfaceData);
        i++) {

        //
        // To retrieve the device interface name (e.g., that you can call
        // CreateFile() on...
        //
        while(!SetupDiGetDeviceInterfaceDetail(DeviceInfoSet,
                                               &DeviceInterfaceData,
                                               DeviceInterfaceDetailData,
                                               BufferSize - (LINE_LEN * sizeof(TCHAR)),
                                               &RequiredSize,
                                               NULL)) {
            //
            // We failed to get the device interface detail data--was it because
            // our buffer was too small? (Hopefully so!)
            //
            Err = GetLastError();

            //
            // We can get rid of our current buffer regardless of what the
            // error was...
            //
            free(Buffer);
            Buffer = NULL;

            if(Err != ERROR_INSUFFICIENT_BUFFER) {
                //
                // Failure!
                //
                _tprintf(TEXT("SetupDiGetDeviceInterfaceDetail failed with %lx\n"), Err);
                break;
            }

            //
            // We failed due to insufficient buffer.  Allocate one that's
            // sufficiently large and try again.
            //
            BufferSize = RequiredSize + (LINE_LEN * sizeof(TCHAR));

            Buffer = malloc(BufferSize);

            if(Buffer) {
                //
                // Leave the first LINE_LEN characters to retrieve the friendly
                // name into...
                //
                FriendlyName = (PTSTR)Buffer;
                DeviceInterfaceDetailData =
                    (PSP_DEVICE_INTERFACE_DETAIL_DATA)(Buffer + (LINE_LEN * sizeof(TCHAR)));

                DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                Err = NO_ERROR;

            } else {
                //
                // Failure!
                //
                Err = ERROR_NOT_ENOUGH_MEMORY;
                _tprintf(TEXT("Couldn't allocate %d bytes for device interface detail buffer\n"), RequiredSize);
                break;
            }
        }

        if(!Buffer) {
            //
            // We encountered a failure above--abort.
            //
            break;
        }

        //
        // Now that we've successfully retrieved the device interface pathname,
        // we can retrieve the friendly name.  We left enough space at the
        // start of the buffer for this, so we'll now retrieve this string,
        // then move the pathname down next to it for display (the pathname
        // will be enclosed in parentheses).
        //
        if(!GetDeviceInterfaceFriendlyName(DeviceInfoSet,
                                           &DeviceInterfaceData,
                                           FriendlyName,
                                           LINE_LEN)) {
            //
            // This generally won't happen, but it _is_ possible.  We'll just
            // use two double-quotes to indicate an empty string.
            //
            if(FAILED(StringCchCopy(FriendlyName, LINE_LEN, TEXT("\"\"")))) {
                break;
            }
        }

        //
        // Add a space and the opening paren (see previous comment on why we're
        // safe in doing this without fear of overwriting the DevicePath string.
        //
        if(FAILED(StringCchCat(FriendlyName, LINE_LEN, TEXT(" (")))) {
            break;
        }

        if(FAILED(StringCchLength(FriendlyName, LINE_LEN, &FriendlyNameLen))) {
            break;
        }

        //
        // Now move the pathname down to the character immediately following
        // the open paren.  (Note: since source and destination blocks may
        // overlap, we must use MoveMemory.)
        //
        if(FAILED(StringCbLength(DeviceInterfaceDetailData->DevicePath,
                                  MAX_PATH,
                                  &devicePathLen))) {
            break;
        }

        devicePathLen += sizeof(TCHAR);

        MoveMemory((PBYTE)(FriendlyName + FriendlyNameLen),
                   DeviceInterfaceDetailData->DevicePath,
                   devicePathLen
                  );

        //
        // Now add close paren
        //
        if (FAILED(StringCchCat(FriendlyName, LINE_LEN, TEXT(")")))) {
            break;
        }

        //
        // Add this device interface to our listbox.
        //
        ListBoxReturn = SendDlgItemMessage(hWnd,
                                           IDC_DEVICE_INTERFACE_LIST,
                                           LB_ADDSTRING,
                                           0,
                                           (LPARAM)FriendlyName
                                          );

        if((ListBoxReturn == LB_ERR) || (ListBoxReturn == LB_ERRSPACE)) {
            //
            // Set Err to some generic failure
            //
            Err = ERROR_INVALID_DATA;

            _tprintf(TEXT("Failed to add %s to listbox (%s)\n"),
                     DeviceInterfaceDetailData->DevicePath,
                     (ListBoxReturn == LB_ERR) ? TEXT("LB_ERR") : TEXT("LB_ERRSPACE")
                    );
            break;
        }

        //
        // Since we may have overwritten the device interface detail data
        // 'cbSize' field, restore it now.
        //
        DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    }

    if(Buffer) {
        free(Buffer);
    }

    SetLastError(Err);

    return (Err == NO_ERROR);
}


BOOL
GetDeviceInterfaceFriendlyName(
    _In_  HDEVINFO                  DeviceInfoSet,
    _In_  PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _Out_writes_all_(FriendlyNameSize) PTSTR FriendlyName,
    _In_  DWORD                     FriendlyNameSize
    )

/*++

Routine Description:

    This routine retrieves the friendly name associated with the specified
    device interface.  It first looks for a "FriendlyName" value entry in the
    device interface's registry key.  If not found, it then tries to use the
    "FriendlyName" property for the underlying devnode.  If that isn't present,
    it uses the devnode's device description (and if there isnt one of those,
    it returns FALSE).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device interface whose friendly name is to be retrieved.

    DeviceInterfaceData - Supplies a context structure indicating which device
        interface we're retrieving a friendly name for.

    FriendlyName - Supplies a character buffer that is filled in, upon
        successful return, with the friendly name for the device interface.

    FriendlyNameSize - Supplies the size, in characters, of the FriendlyName
        buffer.

Return Value:

    If the function succeeds, the return value is non-zero.
    If no FriendlyName is found (or buffer is too small), the return value is
    FALSE.

--*/

{
    HKEY hkey;
    DWORD Err;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD RegDataType, RegDataLength;

    //
    // First, open up the device interface registry key to see if the interface
    // has its own friendly name.
    //
    hkey = SetupDiOpenDeviceInterfaceRegKey(DeviceInfoSet,
                                            DeviceInterfaceData,
                                            0,
                                            KEY_READ
                                           );

    if(hkey != INVALID_HANDLE_VALUE) {

        RegDataLength = FriendlyNameSize * sizeof(TCHAR);

        Err = RegQueryValueEx(hkey,
                              TEXT("FriendlyName"),
                              NULL,
                              &RegDataType,
                              (PBYTE)FriendlyName,
                              &RegDataLength
                             );

        RegCloseKey(hkey);

        if((Err == ERROR_SUCCESS) && (RegDataType == REG_SZ)) {
            return TRUE;
        }
    }

    //
    // Find out what device instance is exposing this interface.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if(!SetupDiGetDeviceInterfaceDetail(DeviceInfoSet,
                                        DeviceInterfaceData,
                                        NULL,
                                        0,
                                        NULL,
                                        &DeviceInfoData)) {
        //
        // We should always get here (i.e., SetupDiGetDeviceInterfaceDetail
        // should always fail) since we didn't pass in a buffer to retrieve
        // the device interface detail data.  Of course, all we really care
        // about is getting at the underlying device info data.
        //

        //
        // Now check the underlying device for a FriendlyName property.
        //
        if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                            &DeviceInfoData,
                                            SPDRP_FRIENDLYNAME,
                                            &RegDataType,
                                            (PBYTE)FriendlyName,
                                            FriendlyNameSize * sizeof(TCHAR),
                                            NULL)) {
            if(RegDataType == REG_SZ) {
                return TRUE;
            }
        }

        //
        // Fall back to device description
        //
        if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                            &DeviceInfoData,
                                            SPDRP_DEVICEDESC,
                                            &RegDataType,
                                            (PBYTE)FriendlyName,
                                            FriendlyNameSize * sizeof(TCHAR),
                                            NULL)) {
            if(RegDataType == REG_SZ) {
                return TRUE;
            }
        }
    }

    //
    // Couldn't find anything usable as a friendly name--return failure.
    //
    return FALSE;
}

