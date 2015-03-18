////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// 
// Module Name: NetNfpControl.cpp
// Abstract: Windows Near-field Proximity Test tool. Designed for simulating proximity hardware.
// 
//    NetNfpControl console app allows control of the NetNfpProvider test driver. 
//    Both the local and remote machine must have the NetNfpProvider driver installed.
//
//    *USAGE*
//    NetNfpControl.exe <remoteMachine> [/e]
//    NetNfpControl.exe [<remoteMachine>] [/k]
//    NetNfpControl.exe /q
//
//    Example: NetNfpControl.exe John-PC1
//    The first operating mode allows the user to specify a remote machine name (or IPv6 address) 
//    that the local machine should connect to and simulate proximity with. After it's connected,
//    a simple key-press ends the simulated proximity link. The console app then exits.
//    If the option /e is specified, rather than the waiting for a key to be pressed, the tool 
//    waits for the QUIT_NAMED_EVENT named event to be set. The named event can be set by running 
//    NetNfpControl.exe /q.
//
//    Example: NetNfpControl.exe John-PC1 /k
//    A second operating mode keeps the console app running with a Ctrl-F1 hotkey registered.
//    This hot key remains registered and functional even when the app is in the background.
//    When the hot key is intercepted, a near-field proximity event is simulated directly with
//    the specified remote machine.
//    Note: The console app needs to be running (only one one machine) to intercept the system hot key.
//  
//    Example: NetNfpControl.exe /k
//    A third operating mode also keeps the console app running with a Ctrl-F1 hotkey registered.
//    However, you'll have to run this on two or more machines at the same time. Pressing Ctrl-F1 
//    on any two machines at the same time causes the machines to exchange their network name via 
//    a file on a private share with a special file name.
//       - The server share used is hard coded to: \\scratch2\scratch\travm\proxrendezvous\
//          - Either create a file server with these folders shared, or change this to match yours.
//       - The file has an effective lifetime of 2 seconds. 
//       - In the event of a collision (two clients posting an event during the same interval), 
//         only one client 'wins'.
//  
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Includes
#include "precomp.h"
#pragma endregion 


#pragma region Globals
wchar_t g_szMachineName[MAX_PATH];
PCWSTR g_pszRemoteMachineName = nullptr;
#pragma endregion 

#define QUIT_NAMED_EVENT L"NetNfpControl_Quit_Event"


//----------------------------------------------------------------------------------------------------------------------
// Name:     DEVICE_INTERFACE_DETAIL
// Comments: 
//
//----------------------------------------------------------------------------------------------------------------------
///<summary>Device interface details.</summary>
struct DEVICE_INTERFACE_DETAIL
{
    DWORD cbSize;
    wchar_t szSymbolicLink[MAX_PATH*2];
};

//----------------------------------------------------------------------------------------------------------------------
// Name:     BeginProximity
// Comments: 
//
//----------------------------------------------------------------------------------------------------------------------
///<summary>Initalizes a proximity event.</summary>
///<remarks>
///</remarks>
HRESULT BeginProximity(_In_ PCWSTR pszName, _Out_ HANDLE* pHandle)
{
    HRESULT hr = S_OK;    
    LPGUID pGuid = (LPGUID) &GUID_DEVINTERFACE_NETNFP;

    HDEVINFO hDevSet = SetupDiGetClassDevs(pGuid, nullptr, nullptr, (DIGCF_PRESENT | DIGCF_INTERFACEDEVICE));
    if (hDevSet == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    DEVICE_INTERFACE_DETAIL deviceInterfaceDetail = {};
    if (SUCCEEDED(hr))
    {
        SP_DEVICE_INTERFACE_DATA devInterfaceData = {sizeof(devInterfaceData)};
        if (!SetupDiEnumDeviceInterfaces(hDevSet, nullptr, pGuid, 0, &devInterfaceData))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        
        if (SUCCEEDED(hr))
        {
            PSP_DEVICE_INTERFACE_DETAIL_DATA pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)&deviceInterfaceDetail;
            pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (!SetupDiGetDeviceInterfaceDetail(hDevSet, &devInterfaceData, pDetail, 
                                                 sizeof(deviceInterfaceDetail), nullptr, nullptr))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        SetupDiDestroyDeviceInfoList(hDevSet);
    }

    HANDLE hProximity = INVALID_HANDLE_VALUE;
    if (SUCCEEDED(hr))
    {
        hProximity = CreateFile(deviceInterfaceDetail.szSymbolicLink, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hProximity == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    
    BEGIN_PROXIMITY_ARGS args = {};
    if (SUCCEEDED(hr))
    {
        hr = StringCchCopy(args.szName, MAX_PATH, pszName);
    }
    
    if (SUCCEEDED(hr))
    {
        DWORD ignore;
        if (!DeviceIoControl(hProximity, IOCTL_BEGIN_PROXIMITY, &args, sizeof(args), nullptr, 0, &ignore, nullptr))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (FAILED(hr))
    {
        if (hProximity != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hProximity);
            hProximity = INVALID_HANDLE_VALUE;
        }
    }

    *pHandle = hProximity;

    return hr;
}

//----------------------------------------------------------------------------------------------------------------------
// Name:     AcquireFileLock
// Comments: 
//
//----------------------------------------------------------------------------------------------------------------------
///<summary>Aquires a lock on a file.</summary>
///<remarks>
///</remarks>
HANDLE AcquireFileLock(PCWSTR pszLockFilePath)
{
    HANDLE hLock = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 200; i++)
    {
        wprintf(L"Attempting to Acquire Lock: %u\n", (DWORD)GetTickCount64());
        hLock = CreateFile(pszLockFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
        if (hLock == INVALID_HANDLE_VALUE)
        {
            Sleep(50);
        }
        else
        {
            wprintf(L"Proximity Lock Acquired: %u\n", (DWORD)GetTickCount64());
            break;
        }
    }
    
    if (hLock == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Proximity Lock Not Acquired, check network connectivity Error = %u.\n", GetLastError());
    }

    return hLock;
}

//----------------------------------------------------------------------------------------------------------------------
// Name:     Proximity
// Comments: 
//
//----------------------------------------------------------------------------------------------------------------------
///<summary>Initiates a proximity event.</summary>
///<remarks>
///</remarks>
void Proximity()
{
    wprintf(L"Checking For Proximate Device\n");

    SYSTEMTIME sysTime = {};
    GetLocalTime(&sysTime);

    wchar_t szDirectory[MAX_PATH];
    StringCchPrintf(szDirectory, MAX_PATH, L"\\\\scratch2\\scratch\\travm\\proxrendezvous\\%u%02u%02u-%02u", 
                    sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour);
    
    CreateDirectory(szDirectory, NULL);
    
    wchar_t szLockPath[MAX_PATH];
    StringCchPrintf(szLockPath, MAX_PATH, L"%s\\lock.txt", szDirectory);

    HANDLE hLock = AcquireFileLock(szLockPath);
    
    wchar_t szOtherMachine[MAX_PATH] = {};
    if (hLock != INVALID_HANDLE_VALUE)
    {
        wprintf(L"looking for available devices in proximity.\n");
        wchar_t szFindPath[MAX_PATH];
        StringCchPrintf(szFindPath, MAX_PATH, L"%s\\*.available", szDirectory);
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(szFindPath, &findData);
        
        wchar_t szFilePath[MAX_PATH];
        bool fClient;
        if (hFind != INVALID_HANDLE_VALUE)
        {
            fClient = true;
            
            wprintf(L"Proximate device found: ");
            StringCchPrintf(szFilePath, MAX_PATH, L"%s\\%s", szDirectory, findData.cFileName);
            HANDLE hFile = CreateFile(szFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                DWORD ignore;
                (void)ReadFile(hFile, szOtherMachine, sizeof(szOtherMachine) - sizeof(wchar_t), &ignore, NULL);
                szOtherMachine[MAX_PATH-1] = L'\0';
                CloseHandle(hFile);
            }

            if (szOtherMachine[0] != L'\0')
            {
                wprintf(L"%s\n", szOtherMachine);
                wchar_t szNewFileName[MAX_PATH];
                StringCchPrintf(szNewFileName, MAX_PATH, L"%s\\%s.%s.%02u%02u", szDirectory, szOtherMachine, g_szMachineName, sysTime.wMinute, sysTime.wSecond);
                MoveFile(szFilePath, szNewFileName);
            }
        }
        else
        {
            fClient = false;
            
            wprintf(L"No proximate device found yet. Placing proximity marker on share...\n");
            StringCchPrintf(szFilePath, MAX_PATH, L"%s\\%s.available", szDirectory, g_szMachineName);
            HANDLE hFile = CreateFile(szFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            DWORD ignore;
            WriteFile(hFile, g_szMachineName, sizeof(g_szMachineName), &ignore, NULL);
            CloseHandle(hFile);
        }
        
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
        
        CloseHandle(hLock);
        DeleteFile(szLockPath);
        
        if (fClient)
        {
            HANDLE hProximity;
            HRESULT hr = BeginProximity(szOtherMachine, &hProximity);
            if (SUCCEEDED(hr))
            {
                wprintf(L"In Proximity for 1 second...\n");
                Sleep(1000);
                CloseHandle(hProximity);
                wprintf(L"Proximity Complete.\n");
            }
            else
            {
                wprintf(L"ERROR: BeginProximity() failed: 0x%x\n", hr);
            }
        }
        else
        {
            wprintf(L"Other machine will initiate proximity...\n");
            Sleep(2000);

            bool fCompleted = true;
            hLock = AcquireFileLock(szLockPath);
            if (hLock != INVALID_HANDLE_VALUE)
            {
                wchar_t szNewFileName[MAX_PATH];
                StringCchPrintf(szNewFileName, MAX_PATH, L"%s\\%s.%02u%02u.expired", szDirectory, g_szMachineName, sysTime.wMinute, sysTime.wSecond);
                if (MoveFile(szFilePath, szNewFileName))
                {
                    fCompleted = false;
                }
                
                CloseHandle(hLock);
                DeleteFile(szLockPath);
            }
            
            if (fCompleted)
            {
                wprintf(L"Proximity Successful!\n");
            }
            else
            {
                wprintf(L"ERROR: Proximity Unsuccessful. No proximate device found!\n");
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Name:     WndProc
// Comments: 
//
//----------------------------------------------------------------------------------------------------------------------
///<summary>Windows call-back procedure.</summary>
///<remarks>
///Used to receive call-backs for the Windows system-wide Hot Key's
///</remarks>
LRESULT CALLBACK WndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{
    switch (uMsg) 
    {
        case WM_HOTKEY:
            {
                if (g_pszRemoteMachineName != nullptr)
                {
                    HANDLE hProximity;
                    HRESULT hr = BeginProximity(g_pszRemoteMachineName, &hProximity);
                    if (SUCCEEDED(hr))
                    {
                        wprintf(L"In Proximity for 1 second...\n");
                        Sleep(1000);
                        CloseHandle(hProximity);
                        wprintf(L"Proximity Complete.\n");
                    }
                    else
                    {
                        wprintf(L"ERROR: BeginProximity() failed: 0x%x\n", hr);
                    }
                }
                else // No machine name specified, need to check for a name on the share
                {
                    Proximity();
                }
            }
            return 0;
 
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0; 
 
        //
        // Process other messages. 
        //
        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    }
} 

__analysis_noreturn void Usage()
{
    wprintf(L"Usage: \n");
    wprintf(L"  NetNfpControl.exe <Machine Name or IP>\n");
    wprintf(L"  NetNfpControl.exe /e <Machine Name or IP>\n");
    wprintf(L"  NetNfpControl.exe /q\n");
    wprintf(L"  NetNfpControl.exe /k\n");
    wprintf(L"  NetNfpControl.exe /k <Machine Name or IP>\n");
    wprintf(L"\n");
    wprintf(L"The /k option registers a hotkey for entering proximity while selfhosting\n");
    wprintf(L"The /e option keeps NetNfpControl.exe running until NetNfpControl.exe /q is called\n"
            L"    (or until the named event %s is set)\n", QUIT_NAMED_EVENT);
    wprintf(L"The /q option sets the the named event %s (and exits), causing \n"
            L"    all the outstanding instances of 'NetNfpControl.exe /e' to exit\n", QUIT_NAMED_EVENT);

    exit(1);
}

//----------------------------------------------------------------------------------------------------------------------
// Name:     wmain
// Comments: Main application entry point.
//
//----------------------------------------------------------------------------------------------------------------------
///<summary>wmain.</summary>
///<remarks>
///Main application entry point.
///</remarks>
int _cdecl wmain(_In_ int argc, _In_reads_(argc) PWSTR* argv)
{
    wprintf(L"\n*** Network NearFieldProvider Control Executable ***\n\n");
    if (argc < 2)
    {
        Usage();
    }

    bool fUseHotKey = false;
    bool fWaitOnNamedEvent = false;
    bool fQuitGlobalEventWaiters = false;

    for (int i = 1; i < argc; i++)
    {
        if ((argv[i][0] == L'/') || (argv[i][0] == L'-'))
        {
            if ((argv[i][1] == L'k') || (argv[i][1] == L'K'))
            {
                fUseHotKey = true;
            }
            else if ((argv[i][1] == L'e') || (argv[i][1] == L'E'))
            {
                fWaitOnNamedEvent = true;
            }
            else if ((argv[i][1] == L'q') || (argv[i][1] == L'Q'))
            {
                fQuitGlobalEventWaiters = true;
            }
            else
            {
                wprintf(L"*** Unknown command line argument: '%ws' ***\n\n", argv[i]);
                Usage();
            }
        }
        else
        {
            if (g_pszRemoteMachineName != nullptr)
            {
                wprintf(L"*** Can't specify two machine names ***\n\n");
                Usage();
            }
            g_pszRemoteMachineName = argv[i];
        }
    }

    if (fUseHotKey)
    {
        if (g_pszRemoteMachineName != nullptr)
        {
            wprintf(L"Press Ctrl-F1 on this machine to initiate proximity with: '%ws'.\n\n", g_pszRemoteMachineName);
        }
        else
        {
            // Dynamic Keyboard hotkey version
            wprintf(L"Press Ctrl-F1 on two machines at the same time to initiate proximity.\n\n");

            DWORD cchMachineName = MAX_PATH;
            if (!GetComputerNameEx(ComputerNameNetBIOS, g_szMachineName, &cchMachineName))
            {
                return FALSE;
            }
        }
        
        // Register the window class for the main window. 
        WNDCLASS wc = {};
        wc.lpfnWndProc = (WNDPROC)WndProc; 
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszMenuName =  L"MainMenu"; 
        wc.lpszClassName = L"MainWndClass"; 
 
        if (!RegisterClass(&wc)) 
        {
            return FALSE;
        }
     
        // Create the main window. 
        HWND hwndMain = CreateWindow(L"MainWndClass", L"Sample", 
                                     WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
                                     CW_USEDEFAULT, CW_USEDEFAULT, (HWND) NULL, 
                                     (HMENU) NULL, GetModuleHandle(NULL), (LPVOID) NULL); 
     
        // If the main window cannot be created, terminate 
        // the application. 
        if (!hwndMain)
        {
            return FALSE;
        }

        if (!RegisterHotKey(hwndMain, 264334, MOD_CONTROL, VK_F1))
        {
            wprintf(L"ERROR: Ctrl-F1 Hotkey already registered!\n");
            return FALSE;
        }
        
        // Start the message loop. 
     
        MSG msg;
        BOOL bRet; 
        while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
        { 
            if (bRet == -1)
            {
                // handle the error and possibly exit
            }
            else
            {
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            }
        } 
     
        // Return the exit code to the system. 
     
        return (int)msg.wParam; 

    }
    else if (fQuitGlobalEventWaiters)
    {
        HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, QUIT_NAMED_EVENT);
        if (NULL != hEvent)
        {
            wprintf(L"Quitting running instances of NetNfpControl.exe /e <machinename>\n");

            BOOL bSetErr = SetEvent(hEvent);
            if (!bSetErr)
            {
                wprintf(L"Failed to set named event %s (Err=0x%x)...\n", QUIT_NAMED_EVENT, GetLastError());
            }
        
            CloseHandle(hEvent);
        }
        else
        {
            wprintf(L"Failed to open named event %s (Err=0x%x)...\n", QUIT_NAMED_EVENT, GetLastError());
        }
    }
    else
    {
        if (NULL == g_pszRemoteMachineName)
        {
            Usage();
        }

        wprintf(L"Attempting connect: '%ws' ...\n\n", g_pszRemoteMachineName);

        HANDLE hProximity;
        HRESULT hr = BeginProximity(g_pszRemoteMachineName, &hProximity);
        if (SUCCEEDED(hr))
        {
            if (fWaitOnNamedEvent)
            {
                wprintf(L"run NetNfpControl.exe /q to end Proximity (or Set the named event %s)\n", QUIT_NAMED_EVENT);

                HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, QUIT_NAMED_EVENT);
                if (NULL != hEvent)
                {
                    DWORD dwWaitErr = WaitForSingleObject(hEvent, INFINITE);
                    if (WAIT_OBJECT_0 != dwWaitErr)
                    {
                        dwWaitErr = (WAIT_FAILED == dwWaitErr ? GetLastError() : dwWaitErr);
                        wprintf(L"Failed to wait on named event %s (Err=0x%x)...\n", QUIT_NAMED_EVENT, dwWaitErr);
                    }

                    CloseHandle(hEvent);
                }
                else
                {
                    wprintf(L"Failed to create/open named event %s (Err=0x%x)...\n", QUIT_NAMED_EVENT, GetLastError());
                }
            }
            else
            {
                wprintf(L"Press Any Key to End Proximity.\n");
                (void) _getch();
                wprintf(L"Ending Proximity...\n");
            }

            CloseHandle(hProximity);
        }
        else
        {
            wprintf(L"BeginProximity() failed: 0x%x\n", hr);
        }
    }

    return 0;
}

