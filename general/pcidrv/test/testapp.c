/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name: Testapp.c


Abstract:

    Testapp for PCIDRV

Environment:

    User mode only.

--*/

#include "testapp.h"


//
// Global variables
//
HINSTANCE   HWndInstance;
HWND        HWndList; // handle to the embedded list box
TCHAR       WindowTitle[]=TEXT("MyPing - Test Application for PCIDRV");
LIST_ENTRY  ListHead;
HDEVNOTIFY  InterfaceNotificationHandle;
TCHAR       OutText[500];
UINT        ListBoxIndex = 0;
GUID        InterfaceGuid;// = GUID_DEVINTERFACE_PCIDRV;
ULONG       DeviceIndex;
BOOLEAN     Verbose = FALSE;


VOID
Display(
    _In_ LPWSTR pstrFormat,  // @parm A printf style format string
    ...                 // @parm | ... | Variable paramters based on <p pstrFormat>
    )
{
    HRESULT hr;
    va_list va;

    va_start(va, pstrFormat);
    //
    // Truncation is acceptable.
    //
    hr = StringCbVPrintf(OutText, sizeof(OutText)-sizeof(WCHAR), pstrFormat, va);
    va_end(va);

    if(FAILED(hr)){
        return;
    }

    SendMessage(HWndList, LB_INSERTSTRING, ListBoxIndex, (LPARAM)OutText);
    SendMessage(HWndList, LB_SETCURSEL, ListBoxIndex, 0);
    ListBoxIndex++;

}

_Use_decl_annotations_
int
PASCAL
WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
    )
{
    static    TCHAR szAppName[]=TEXT("MYPING");
    HWND      hWnd;
    MSG       msg;
    WNDCLASS  wndclass;

    InterfaceGuid = GUID_DEVINTERFACE_PCIDRV;
    HWndInstance=hInstance;

    if (!hPrevInstance)
       {
         wndclass.style        =  CS_HREDRAW | CS_VREDRAW;
         wndclass.lpfnWndProc  =  WndProc;
         wndclass.cbClsExtra   =  0;
         wndclass.cbWndExtra   =  0;
         wndclass.hInstance    =  hInstance;
         wndclass.hIcon        =  LoadIcon (NULL, IDI_APPLICATION);
         wndclass.hCursor      =  LoadCursor(NULL, IDC_ARROW);
         wndclass.hbrBackground=  GetStockObject(WHITE_BRUSH);
         wndclass.lpszMenuName =  TEXT("GenericMenu");
         wndclass.lpszClassName=  szAppName;

         RegisterClass(&wndclass);
       }

        hWnd = CreateWindow (szAppName,
                         WindowTitle,
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

    ShowWindow (hWnd, nShowCmd);
    UpdateWindow(hWnd);

    while (GetMessage (&msg, NULL, 0,0))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

    return (0);
}


LRESULT FAR PASCAL
WndProc (HWND hWnd,
         UINT message,
         WPARAM wParam,
         LPARAM lParam
         )
{
    DWORD                           nEventType = (DWORD)wParam;
    PDEV_BROADCAST_HDR              p = (PDEV_BROADCAST_HDR) lParam;
    DEV_BROADCAST_DEVICEINTERFACE   filter;
    WSADATA                         wsd;

    switch (message)
    {

        case WM_COMMAND:
            HandleCommands(hWnd, message, wParam, lParam);
            return 0;

        case WM_CREATE:

            // Load Winsock
            if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
            {
                MessageBox(hWnd, TEXT("WSAStartup failed"), TEXT("Error"), MB_OK);
                exit(0);
            }

            HWndList = CreateWindow (TEXT("listbox"),
                         NULL,
                         WS_CHILD|WS_VISIBLE|LBS_NOTIFY |
                         WS_VSCROLL | WS_BORDER,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         hWnd,
                         (HMENU)ID_EDIT,
                         HWndInstance,
                         NULL);

            filter.dbcc_size = sizeof(filter);
            filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            filter.dbcc_classguid = InterfaceGuid;
            InterfaceNotificationHandle = RegisterDeviceNotification(hWnd, &filter, 0);

            InitializeListHead(&ListHead);
            EnumExistingDevices(hWnd);

            return 0;

      case WM_SIZE:

            MoveWindow(HWndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            return 0;

      case WM_SETFOCUS:
            SetFocus(HWndList);
            return 0;

      case WM_DEVICECHANGE:

            //
            // The DBT_DEVNODES_CHANGED broadcast message is sent
            // everytime a device is added or removed. This message
            // is typically handled by Device Manager kind of apps,
            // which uses it to refresh window whenever something changes.
            // The lParam is always NULL in this case.
            //
            if(DBT_DEVNODES_CHANGED == wParam) {
                DisplayV(TEXT("Received DBT_DEVNODES_CHANGED broadcast message"));
                return 0;
            }

            //
            // All the events we're interested in come with lParam pointing to
            // a structure headed by a DEV_BROADCAST_HDR.  This is denoted by
            // bit 15 of wParam being set, and bit 14 being clear.
            //
            if((wParam & 0xC000) == 0x8000) {

                if (!p)
                    return 0;

                if (p->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {

                    HandleDeviceInterfaceChange(hWnd, nEventType, (PDEV_BROADCAST_DEVICEINTERFACE) p);
                } else if (p->dbch_devicetype == DBT_DEVTYP_HANDLE) {

                    HandleDeviceChange(hWnd, nEventType, (PDEV_BROADCAST_HANDLE) p);
                }
            }
            return 0;

      case WM_POWERBROADCAST:
            HandlePowerBroadcast(hWnd, wParam, lParam);
            return 0;

      case WM_CLOSE:
            Cleanup(hWnd);
            UnregisterDeviceNotification(InterfaceNotificationHandle);
            return  DefWindowProc(hWnd,message, wParam, lParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd,message, wParam, lParam);
  }


LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM     lParam
    )

{
    PDIALOG_RESULT result = NULL;
    PDEVICE_INFO deviceInfo = NULL;

    switch (wParam) {

        case IDM_CLOSE:
            Cleanup(hWnd);
            Display(TEXT("Handle to the device closed"));
            EnableMenuItem(GetMenu(hWnd), IDM_PING, MF_BYCOMMAND|MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd), IDM_CLOSE, MF_BYCOMMAND|MF_GRAYED);
            break;

        case IDM_ENUMERATE:
            //
            // First cleanup everything, and then reenumerate all the devices.
            Cleanup(hWnd);
            EnumExistingDevices(hWnd);
            EnableMenuItem(GetMenu(hWnd), IDM_PING, MF_BYCOMMAND|MF_ENABLED);
            break;

        case IDM_PING:

            result = (PDIALOG_RESULT)DialogBox(HWndInstance, MAKEINTRESOURCE(IDD_DIALOG), hWnd, DlgProc);
            if(result) {
                deviceInfo = FindDeviceInfo(result);
                if(!deviceInfo){
                    MessageBox(hWnd, TEXT("FindDeviceInfo failed"), TEXT("Error"), MB_OK);
                    break;
                }

                if(!OpenDevice(hWnd, deviceInfo)){
                    MessageBox(hWnd, TEXT("OpenDevice failed"), TEXT("Error"), MB_OK);
                    break;
                }

                if (!CreatePingThread(deviceInfo)) {
                    MessageBox(hWnd, TEXT("CreatePingThread failed"), TEXT("Error"), MB_OK);
                    break;
                }

                EnableMenuItem(GetMenu(hWnd), IDM_PING, MF_BYCOMMAND|MF_GRAYED);
                EnableMenuItem(GetMenu(hWnd), IDM_CLOSE, MF_BYCOMMAND|MF_ENABLED);

            }
            break;

        case IDM_CLEAR:
            SendMessage(HWndList, LB_RESETCONTENT, 0, 0);
            ListBoxIndex = 0;
            break;

        case IDM_VERBOSE: {

                HMENU hMenu = GetMenu(hWnd);
                Verbose = !Verbose;
                if(Verbose) {
                    CheckMenuItem(hMenu, (UINT)wParam, MF_CHECKED);
                } else {
                    CheckMenuItem(hMenu, (UINT)wParam, MF_UNCHECKED);
                }
            }
            break;

        case IDM_EXIT:
            Cleanup(hWnd);
            PostQuitMessage(0);
            break;

        default:
            break;
    }

    if(result) {
        HeapFree (GetProcessHeap(), 0, result);
    }
    return TRUE;
}

INT_PTR CALLBACK
DlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL            success;
    PDIALOG_RESULT  dialogResult = NULL;
    ULONG           value;
    WCHAR SourceIP[80];
    WCHAR DestinationIP[80];
    DWORD SourceIPLen = sizeof(SourceIP);

    switch(message)
    {
        case WM_INITDIALOG:
            //
            // Set default values.
            //
            if(GetRegistryInfo(SourceIP, &SourceIPLen, DestinationIP, &SourceIPLen)) {
                SetDlgItemText(hDlg, IDC_SOURCE_IP, SourceIP);
                SetDlgItemText(hDlg, IDC_DESTINATION_IP, DestinationIP);
            } else {
                SetDlgItemText(hDlg, IDC_SOURCE_IP, DEF_SOURCE_IP);
                SetDlgItemText(hDlg, IDC_DESTINATION_IP, DEF_DEST_IP);

            }

            SetDlgItemInt(hDlg, IDC_PACKET_SIZE, MAX_PAYLOAD_SIZE, FALSE);
            return TRUE;

        case WM_COMMAND:
            switch( wParam)
            {
            case ID_OK:
                //
                // Allocate memory to store the input values.
                //
                dialogResult = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(DIALOG_RESULT));
                if(dialogResult) {

                    dialogResult->DeviceIndex = GetDlgItemInt(hDlg,
                                            IDC_DEVICE_INDEX, &success, FALSE );
                    if(!success){
                        break;
                    }
                    value = GetDlgItemText(hDlg, IDC_SOURCE_IP,
                                            dialogResult->SourceIp, MAX_LEN-1 );
                    if(!value){
                        break;
                    }


                    GetDlgItemText(hDlg, IDC_DESTINATION_IP,
                                        dialogResult->DestIp, MAX_LEN-1 );
                    if(!value){
                        break;
                    }

                    value = GetDlgItemInt(hDlg,IDC_PACKET_SIZE, &success, FALSE );
                    if(success){
                        value = min(value, MAX_PAYLOAD_SIZE);
                        value = max(value, MIN_PAYLOAD_SIZE);

                    } else {
                        value = MIN_PAYLOAD_SIZE;
                    }

                    dialogResult->PacketSize = value;

                    SetRegistryInfo(dialogResult->SourceIp,
                                    sizeof(dialogResult->SourceIp),
                                    dialogResult->DestIp,
                                    sizeof(dialogResult->DestIp));

                }
                EndDialog(hDlg, (UINT_PTR)dialogResult);
                return TRUE;
            case ID_CANCEL:
                EndDialog(hDlg, 0);
                return TRUE;

            }
            break;

    }
    return FALSE;
}


BOOL
HandleDeviceInterfaceChange(
    HWND        hWnd,
    DWORD       evtype,
    PDEV_BROADCAST_DEVICEINTERFACE dip
    )
{
    switch (evtype)
    {
        case DBT_DEVICEARRIVAL:
        //
        // New device arrived. Create a devicinfo structure and record
        // information about the device.
        //
        Display(TEXT("New device Arrived (Interface Change Notification)"));

        if(!CreateDeviceInfo(dip->dbcc_name)) {
            return FALSE;
        }

        break;

        case DBT_DEVICEREMOVECOMPLETE:
        //
        // Device Removed.
        //
        Display(TEXT("Remove Complete (Interface Change Notification)"), NULL);
        break;

        default:
        DisplayV(TEXT("Unknown (Interface Change Notification)"), NULL);
        break;
    }
    return TRUE;
}

BOOL
HandleDeviceChange(
    HWND    hWnd,
    DWORD   evtype,
    PDEV_BROADCAST_HANDLE dhp
    )
{
    PDEVICE_INFO            deviceInfo = NULL;
    PLIST_ENTRY             thisEntry;

    //
    // Walk the list to get the deviceInfo for this device
    // by matching the notification handle saved in our deviceInfo
    // and the one provided as part of the message.
    //
    for(thisEntry = ListHead.Flink; thisEntry != &ListHead;
                        thisEntry = thisEntry->Flink)
    {
        deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
        if(dhp->dbch_hdevnotify == deviceInfo->hHandleNotification) {
            break;
        }
        deviceInfo = NULL;
    }

    if(!deviceInfo) {
        Display(TEXT("Error: spurious message Event Type %x, Device Type %x"),
                                evtype, dhp->dbch_devicetype);
        return FALSE;
    }

    switch (evtype)
    {

    case DBT_DEVICEQUERYREMOVE:

        Display(TEXT("Query Remove (Handle Notification): %ws"),
                        deviceInfo->DeviceName);
        // User is trying to disable, uninstall, or eject our device.
        // Terminate the ping thread and close the handle
        // to the device so that the target device can
        // get removed. Do not unregister the notification
        // at this point, because we want to know whether
        // the device is successfully removed or not.
        //
        TerminatePingThread(deviceInfo);
        break;

    case DBT_DEVICEREMOVECOMPLETE:

        Display(TEXT("Remove Complete (Handle Notification):%ws"),
                deviceInfo->DeviceName);
        //
        // Device is getting surprise removed. So terminate the
        // ping thread to close the handle to device and
        // unregister the PNP notification.
        //
        TerminatePingThread(deviceInfo);

        if (deviceInfo->hHandleNotification) {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
        }

        //
        // Unlink this deviceInfo from the list and free the memory
        //
        RemoveEntryList(&deviceInfo->ListEntry);
        HeapFree (GetProcessHeap(), 0, deviceInfo);

        break;

    case DBT_DEVICEREMOVEPENDING:

        Display(TEXT("Remove Pending (Handle Notification):%ws"),
                                        deviceInfo->DeviceName);
        //
        // Device is successfully removed so unregister the notification
        // and free the memory.
        //
        FreeDeviceInfo(deviceInfo);

        break;

    case DBT_DEVICEQUERYREMOVEFAILED :
        Display(TEXT("Remove failed (Handle Notification):%ws"),
                                    deviceInfo->DeviceName);
        //
        // Remove failed. So reopen the device and register for
        // notification on the new handle. But first we should unregister
        // the previous notification.
        //
        if (deviceInfo->hHandleNotification) {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
        }

        if(!OpenDevice(hWnd, deviceInfo)) {
            Display(TEXT("Failed to reopen the device: %ws"),
                    deviceInfo->DeviceName);
            FreeDeviceInfo(deviceInfo);
            break;
        }

        Display(TEXT("Reopened device %ws"), deviceInfo->DeviceName);
        //
        // Restart the ping operation.
        //
        if(CreatePingThread(deviceInfo)){
            FreeDeviceInfo(deviceInfo);
            break;
        }

        break;

    default:
        Display(TEXT("Unknown (Handle Notification)"));
        break;

    }
    return TRUE;
}


BOOLEAN
EnumExistingDevices(
    HWND   hWnd
)
{
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0, i;
    DWORD                               error;
    PDEVICE_INFO                        deviceInfo =NULL;

    DisplayV(TEXT("Entered EnumExistingDevices"));

    //
    // Make sure the list is empty
    //
    if(!IsListEmpty(&ListHead) ){
        MessageBox(hWnd, TEXT("ListHead should be empty"), TEXT("Error!"), MB_OK);
        return FALSE;
    }

    DeviceIndex = 0;

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&InterfaceGuid,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        goto Error;
    }

    //
    // Enumerate devices of a specific interface class
    //
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

    for(i=0; SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&InterfaceGuid,
                                 i, //
                                 &deviceInterfaceData); i++ ) {

        //
        // Allocate a function class device data structure to
        // receive the information about this particular device.
        //

        //
        // First find out required length of the buffer
        //
        if (deviceInterfaceDetailData) {
            HeapFree (GetProcessHeap(), 0, deviceInterfaceDetailData);
            deviceInterfaceDetailData = NULL;
        }

        if(!SetupDiGetDeviceInterfaceDetail (
                hardwareDeviceInfo,
                &deviceInterfaceData,
                NULL, // probing so no output buffer yet
                0, // probing so output buffer length of zero
                &requiredLength,
                NULL) && (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Error;
        }
        predictedLength = requiredLength;

        deviceInterfaceDetailData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        predictedLength);
        if (deviceInterfaceDetailData == NULL) {
            goto Error;
        }

        deviceInterfaceDetailData->cbSize =
                        sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);


        if (! SetupDiGetDeviceInterfaceDetail (
                   hardwareDeviceInfo,
                   &deviceInterfaceData,
                   deviceInterfaceDetailData,
                   predictedLength,
                   &requiredLength,
                   NULL)) {
            goto Error;
        }

        deviceInfo = CreateDeviceInfo(deviceInterfaceDetailData->DevicePath);

        if(!deviceInfo)
            goto Error;


    }

    if(deviceInterfaceDetailData) {
        HeapFree (GetProcessHeap(), 0, deviceInterfaceDetailData);
    }

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    return 0;

Error:

    error = GetLastError();
    MessageBox(hWnd, TEXT("EnumExisting Devices failed"), TEXT("Error!"), MB_OK);
    if(deviceInterfaceDetailData)
        HeapFree (GetProcessHeap(), 0, deviceInterfaceDetailData);

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    Cleanup(hWnd);
    return 0;
}

PDEVICE_INFO
FindDeviceInfo(
    PDIALOG_RESULT InputInfo
    )
{
    PLIST_ENTRY thisEntry, listHead;
    PDEVICE_INFO    deviceInfo = NULL, result = NULL;

    listHead = &ListHead;

    for(thisEntry = listHead->Flink;
       thisEntry != listHead;
       thisEntry = thisEntry->Flink){

        deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);

        if(deviceInfo->DeviceIndex == InputInfo->DeviceIndex){

            if(deviceInfo->IsANetworkMiniport){
                Display(TEXT("You can't use this app on a device installed as a network device"));
                break;
            }

            if(deviceInfo->hDevice &&
                deviceInfo->hDevice != INVALID_HANDLE_VALUE){
                Display(TEXT("%ws device is already in use"),
                        deviceInfo->DeviceName);
                break;
            }


            deviceInfo->DeviceIndex = InputInfo->DeviceIndex;
            deviceInfo->PacketSize = InputInfo->PacketSize;
            memcpy(deviceInfo->UnicodeSourceIp, InputInfo->SourceIp, MAX_LEN);
            memcpy(deviceInfo->UnicodeDestIp, InputInfo->DestIp, MAX_LEN);
            //
            // Convert the unicode source and destination IP string
            // to ANSI and store it.
            //
            WideCharToMultiByte(CP_ACP, //ANSI code page
                        0, deviceInfo->UnicodeSourceIp, -1,
                        deviceInfo->SourceIp, MAX_LEN, NULL, NULL);

            //
            // Convert Unicode string to ANSI.
            //
            WideCharToMultiByte(CP_ACP, 0, deviceInfo->UnicodeDestIp, -1,
                        deviceInfo->DestIp, MAX_LEN, NULL, NULL);


            result = deviceInfo;
            break;
        }

    }

    return result;

}

PDEVICE_INFO
CreateDeviceInfo(
    _In_ LPWSTR DevicePath
    )
{
    PDEVICE_INFO deviceInfo = NULL;
    HRESULT      hr;

    DisplayV(TEXT("Entered CreateDeviceInfo"));

    deviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO));
    if(!deviceInfo) {
        goto Error;
    }


    if(!GetDeviceDescription(DevicePath,
                                deviceInfo->DeviceName,
                                sizeof(deviceInfo->DeviceName),
                                &deviceInfo->IsANetworkMiniport
                                )) {
        Display(TEXT("GetDeviceDescription failed %x"), GetLastError());
        goto Error;
    }


    //
    // Copy the device path so that we can open the device using CreateFile.
    //
    hr = StringCchCopy(deviceInfo->DevicePath, MAX_PATH, DevicePath);
    if(FAILED(hr)){
        goto Error;
    }

    DeviceIndex++;
    deviceInfo->DeviceIndex = DeviceIndex;

    //
    // Link this to the global list of devices.
    //
    InitializeListHead(&deviceInfo->ListEntry);
    InsertTailList(&ListHead, &deviceInfo->ListEntry);

    Display(TEXT("Device %d is %ws"), DeviceIndex, deviceInfo->DeviceName);

    return deviceInfo;

Error:

    if(deviceInfo) {
        HeapFree (GetProcessHeap(), 0, deviceInfo);
    }
    return NULL;

}

VOID
FreeDeviceInfo(
    _In_ PDEVICE_INFO DeviceInfo
    )
{
    DisplayV(TEXT("Entered FreeDeviceInfo"));

    if (DeviceInfo->hHandleNotification) {
        UnregisterDeviceNotification(DeviceInfo->hHandleNotification);
        DeviceInfo->hHandleNotification = NULL;
    }
    if (DeviceInfo->hDevice != INVALID_HANDLE_VALUE &&
            DeviceInfo->hDevice != NULL) {
        CloseHandle(DeviceInfo->hDevice);
        DeviceInfo->hDevice = INVALID_HANDLE_VALUE;
        Display(TEXT("Closed handle to device %ws"), DeviceInfo->DeviceName );
    }

    RemoveEntryList(&DeviceInfo->ListEntry);

    HeapFree (GetProcessHeap(), 0, DeviceInfo);

    return;
}

BOOL
SetRegistryInfo(
    _In_reads_bytes_(SourceIPLen) LPWSTR SourceIP,
    _In_ DWORD SourceIPLen,
    _In_reads_bytes_(DestinationIPLen) LPWSTR  DestinationIP,
    _In_ DWORD DestinationIPLen
    )
{
    HKEY    hKey;
    BOOL    ret = FALSE;
    size_t  srcStrLen, destStrLen;

    if (FAILED(StringCbLengthW(SourceIP, SourceIPLen, &srcStrLen))) {
        return ret;
    }

    if (FAILED(StringCbLengthW(DestinationIP, DestinationIPLen, &destStrLen))) {
        return ret;
    }

    //
    // RegSetValueEx takes a DWORD, in the rare case that size_t is larger than
    // a DWORD return an error.
    //
    if (srcStrLen > (DWORD_MAX - sizeof(WCHAR))|| 
        destStrLen > (DWORD_MAX - sizeof(WCHAR))) {
        return ret;
    }

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REG_PATH,  &hKey)) {

        if (ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, REG_PATH, &hKey)) {
            Display(TEXT("RegCreateKey failed: %x"), GetLastError());
            return ret;
        }
    }

    if (ERROR_SUCCESS == RegSetValueEx(hKey, L"SourceIP", 0, REG_SZ,
                (LPBYTE)SourceIP, (DWORD) srcStrLen+sizeof(WCHAR))) {

        if (ERROR_SUCCESS == RegSetValueEx(hKey, L"DestinationIP", 0, REG_SZ,
                    (LPBYTE)DestinationIP, (DWORD) destStrLen+sizeof(WCHAR))) {
            ret = TRUE;
        }
    }

    RegCloseKey(hKey);
    return ret;
}

_Success_(return)
BOOL
GetRegistryInfo(
    _Out_writes_bytes_(* SourceIPLen) PWSTR SourceIP,
    _Inout_ LPDWORD SourceIPLen,
    _Out_writes_bytes_(* DestinationIPLen) PWSTR  DestinationIP,
    _Inout_ LPDWORD DestinationIPLen
    )
{
    HKEY                                hKey;
    DWORD   dwType = REG_SZ;
    BOOL ret = FALSE;

    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REG_PATH,  &hKey)) {

        if(ERROR_SUCCESS == RegQueryValueEx(hKey, L"SourceIP", NULL, &dwType,
                                            (LPBYTE)SourceIP, SourceIPLen)){

            if(ERROR_SUCCESS == RegQueryValueEx(hKey, L"DestinationIP", NULL, &dwType,
                                            (LPBYTE)DestinationIP, DestinationIPLen)){

                ret = TRUE;
            }

       }
        RegCloseKey(hKey);
    }

    return ret;
}



BOOLEAN
OpenDevice(
    _In_ HWND HWnd,
    _In_ PDEVICE_INFO DeviceInfo
    )
{
    DEV_BROADCAST_HANDLE    filter;
    HANDLE                  hDevice;

    DisplayV(TEXT("Entered OpenDevice"));

    //
    // Open an handle to the device.
    //
    hDevice = CreateFile (
            DeviceInfo->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL, // no SECURITY_ATTRIBUTES structure
            OPEN_EXISTING, // No special create flags
            FILE_FLAG_OVERLAPPED,
            NULL);

    if (INVALID_HANDLE_VALUE == hDevice) {
        Display(TEXT("Failed to open the device: %ws"),
                DeviceInfo->DeviceName);
        return FALSE;
    }

    Display(TEXT("Opened handled to the device: %ws"),
                DeviceInfo->DeviceName);
    //
    // Register handle based notification to receive pnp
    // device change notification on the handle.
    //

    memset (&filter, 0, sizeof(filter)); //zero the structure
    filter.dbch_size = sizeof(filter);
    filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
    filter.dbch_handle = hDevice;

    DeviceInfo->hHandleNotification = RegisterDeviceNotification(HWnd, &filter, 0);
    if(!DeviceInfo->hHandleNotification){
        Display(TEXT("Failed to register notification: %ws"),
                DeviceInfo->DeviceName);
        CloseHandle(hDevice);
        return FALSE;
    }

    DeviceInfo->hDevice = hDevice;

    return TRUE;

}


BOOL
CreatePingThread(
    PDEVICE_INFO DeviceInfo
    )
{
    ULONG id;

    DisplayV(TEXT("CreatePingThread"));

    DeviceInfo->ExitThread = FALSE;

    //
    // Start the ping operation in a separate thread.
    //
    DeviceInfo->ThreadHandle = CreateThread( NULL,      // security attributes
                        0,         // initial stack size
                        (LPTHREAD_START_ROUTINE) PingThread,    // Main() function
                        DeviceInfo,      // arg to Reader thread
                        0,         // creation flags
                        (LPDWORD)&id); // returned thread id

    if ( NULL == DeviceInfo->ThreadHandle) {
        Display(TEXT("CreateThread failed %x"), GetLastError());
        return FALSE;
    }

    return TRUE;
}

VOID
TerminatePingThread(
    PDEVICE_INFO DeviceInfo
    )
{
    DWORD status;

    DisplayV(TEXT("TerminatePingThread"));

    if(DeviceInfo->ThreadHandle){

        DeviceInfo->ExitThread = TRUE;
        //
        // Wait for the thread to exit
        //
        status = WaitForSingleObjectEx(DeviceInfo->ThreadHandle, 1000, TRUE );
        if(status == WAIT_FAILED){
            Display(TEXT("Wait failed %x"), GetLastError());
        }
        CloseHandle(DeviceInfo->ThreadHandle);
        DeviceInfo->ThreadHandle = NULL;
    }

}

BOOLEAN
Cleanup(
    HWND hWnd
    )
/*++

    This routine walks the global list of currently enumerated devices
    and close all handles and frees the memory.
 --*/
{
    PDEVICE_INFO    deviceInfo =NULL;
    PLIST_ENTRY     thisEntry;

    DisplayV(TEXT("Entered Cleanup"));

    while (!IsListEmpty(&ListHead)) {
        thisEntry = ListHead.Flink;
        deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
        //
        // First let us make sure the PingThread is not running.
        //
        TerminatePingThread(deviceInfo);
        FreeDeviceInfo(deviceInfo);
    }
    return TRUE;
}


_Success_(return != FALSE)
BOOL
GetDeviceDescription(
    _In_ LPTSTR DevPath,
    _Out_writes_bytes_all_(OutBufferLen) LPTSTR OutBuffer,
    _In_ ULONG OutBufferLen,
    BOOL   *NetClassDevice
)
{
    HDEVINFO                            hardwareDeviceInfo = NULL;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    SP_DEVINFO_DATA                     deviceInfoData;
    DWORD                               dwRegType, error;
    TCHAR                               classGuidString[MAX_GUID_STRING_LEN];
    HRESULT                             hr;
    GUID                                classGuid;
    BOOL                                ret = FALSE;

    DisplayV(TEXT("GetDeviceDescription"));

    hardwareDeviceInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        Display(TEXT("Couldn't create DeviceInfoList: %x"), GetLastError());
        goto Error;
    }

    //
    // Enumerate devices of toaster class
    //
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

    SetupDiOpenDeviceInterface (hardwareDeviceInfo, DevPath,
                                 0, //
                                 &deviceInterfaceData);

    deviceInfoData.cbSize = sizeof(deviceInfoData);
    if(!SetupDiGetDeviceInterfaceDetail (
            hardwareDeviceInfo,
            &deviceInterfaceData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            NULL,
            &deviceInfoData) && (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
    {
        Display(TEXT("Couldn't get interface detail: %x"), GetLastError());
        goto Error;
    }
    //
    // Get the friendly name for this instance, if that fails
    // try to get the device description.
    //

    if(!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, &deviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     &dwRegType,
                                     (BYTE*) OutBuffer,
                                     OutBufferLen,
                                     NULL))
    {
        if(!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, &deviceInfoData,
                                     SPDRP_DEVICEDESC,
                                     &dwRegType,
                                     (BYTE*) OutBuffer,
                                     OutBufferLen,
                                     NULL)){
            Display(TEXT("Couldn't get friendlyname: %x"), GetLastError());
            goto Error;

        }


    }

    //
    // Get the class guid of the device and find out whether this is a
    // network miniport.
    //
    if(!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo,
                 &deviceInfoData,
                 SPDRP_CLASSGUID,
                 &dwRegType,
                 (BYTE*) classGuidString,
                 sizeof(classGuidString),
                 NULL)) {
        Display(TEXT("Class guid is not available for device: %ws"), OutBuffer );
    }

    hr = CLSIDFromString(classGuidString, &classGuid);
    if(FAILED(hr)) {
        goto Error;
    }

    if(IsEqualGUID(&classGuid, &GUID_DEVCLASS_NET)){
        *NetClassDevice = TRUE;
    } else {
        *NetClassDevice = FALSE;
    }

    ret = TRUE;

Error:

    if(hardwareDeviceInfo) {
        SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    }
    return ret;
}


BOOL
HandlePowerBroadcast(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fRet = TRUE;

    switch (wParam)
    {
        case PBT_APMQUERYSTANDBY:
            DisplayV(TEXT("PBT_APMQUERYSTANDBY"));
            break;
        case PBT_APMQUERYSUSPEND:
            DisplayV(TEXT("PBT_APMQUERYSUSPEND"));
            break;
        case PBT_APMSTANDBY :
            DisplayV(TEXT("PBT_APMSTANDBY"));
            break;
        case PBT_APMSUSPEND :
            DisplayV(TEXT("PBT_APMSUSPEND"));
            break;
        case PBT_APMQUERYSTANDBYFAILED:
            DisplayV(TEXT("PBT_APMQUERYSTANDBYFAILED"));
            break;
        case PBT_APMRESUMESTANDBY:
            DisplayV(TEXT("PBT_APMRESUMESTANDBY"));
            break;
        case PBT_APMQUERYSUSPENDFAILED:
            DisplayV(TEXT("PBT_APMQUERYSUSPENDFAILED"));
            break;
        case PBT_APMRESUMESUSPEND:
            DisplayV(TEXT("PBT_APMRESUMESUSPEND"));
            break;
        case PBT_APMBATTERYLOW:
            DisplayV(TEXT("PBT_APMBATTERYLOW"));
            break;
        case PBT_APMOEMEVENT:
            DisplayV(TEXT("PBT_APMOEMEVENT"));
            break;
        case PBT_APMRESUMEAUTOMATIC:
            DisplayV(TEXT("PBT_APMRESUMEAUTOMATIC"));
            break;
        case PBT_APMRESUMECRITICAL:
            DisplayV(TEXT("PBT_APMRESUMECRITICAL"));
            break;
        case PBT_APMPOWERSTATUSCHANGE:
            DisplayV(TEXT("PBT_APMPOWERSTATUSCHANGE"));
            break;
        default:
            DisplayV(TEXT("Default"));
            break;
    }
    return fRet;
}


