#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <wuerror.h>

//
// These defines control which packages need to be updated.  In order to
// to customize this for your drivers needs, edit needed packages to 1 and
// unneeded packages to 0.
//
// If your UMDF driver uses USB functionality, you also need KMDF and WinUSB
//
#define INSTALL_KMDF   (TRUE)
#define INSTALL_WINUSB (TRUE)
#define INSTALL_UMDF   (TRUE)

//
// Reference strings that are used to build our MSU package name.  These will
// change between releases of the framework and will need to be updated
//
#define WINUSB_UPDATE_NAME L"WinUSB_1.9.msu"

// 
// MSU names are of the format:
// <kmdf|umdf>-<wdf major version>.<wdf minor version>-
//         Win-<windows major version>.<windows minor version>.msu
//
#define MSU_FORMAT_STRING L"%s-%d.%d-Win-%d.%d.msu"

#define WUSA_EXE L"%windir%\\system32\\wusa.exe"

#define WUSA_EXE_ARGUMENTS L"/quiet /norestart"

#define WDF_MAJOR_VERSION 1
#define WDF_MINOR_VERSION 11

DWORD
ApplyUpdate(
    PCWSTR MSUName
)
{
    DWORD error = ERROR_SUCCESS;
    size_t cmdLengthBytes;
    size_t applicationLengthBytes;
    BOOL ok;
    PROCESS_INFORMATION pInfo;
    STARTUPINFOW startInfo;
    PWCHAR applicationName = NULL;
    PWCHAR commandLine = NULL;
    HRESULT hr;
    
    ZeroMemory(&startInfo,sizeof(startInfo)) ;
    startInfo.cb = sizeof(STARTUPINFO) ;

    ZeroMemory(&pInfo,sizeof(pInfo));
    //
    // Check that the update package exists
    //
    error = GetFileAttributes(MSUName);

    if (error == INVALID_FILE_ATTRIBUTES) {
        error = GetLastError();
        wprintf(L"Error: Could not find update file %s; error %x\n", MSUName, error);
        goto exit;
    }

    //
    // Invoke wusa
    //
    applicationName = (PWCHAR) LocalAlloc(LPTR, (MAX_PATH + 1)*sizeof(WCHAR));
    if (applicationName == NULL) {
        error = ERROR_INSTALL_FAILURE;
        wprintf(L"Failed to allocate applicationName buffer\n");
        goto exit;
    }

    applicationName[0] = L'\0';
    
    applicationLengthBytes = ExpandEnvironmentStrings(WUSA_EXE,
                                                applicationName,
                                                MAX_PATH+1);

    if ((applicationLengthBytes == 0) ||
        (applicationLengthBytes > MAX_PATH+1)) {
        wprintf(L"Could not expland %s\n", WUSA_EXE);
        error = ERROR_INSTALL_FAILURE;
        goto exit;
    }

    applicationLengthBytes = sizeof(WCHAR) * applicationLengthBytes;

    hr = StringCbLength(MSUName,
                    MAX_PATH * sizeof(WCHAR),
                    &cmdLengthBytes);
    
    if (hr != S_OK) {
                   
        error = ERROR_INSTALL_FAILURE;
        wprintf(L"StringCbLength failed MSUName, %x\n",
               hr);
        goto exit;
    }
    //
    // Add enough padding for 2 \". The size returned by sizeof() includes
    // the terminating L'\0'
    //
    cmdLengthBytes = applicationLengthBytes + cmdLengthBytes + sizeof(WUSA_EXE_ARGUMENTS) + 3*sizeof(WCHAR);

    commandLine = (PWCHAR) LocalAlloc(LPTR, cmdLengthBytes );
    
    if (commandLine == NULL) {
        wprintf(L"Failed to allocate applicationName buffer\n");
        error = ERROR_INSTALL_FAILURE;
        goto exit;
    }

    hr = StringCbPrintf(commandLine,
                              cmdLengthBytes,
                              L"%s \"%s\" %s",
                              applicationName,
                              MSUName,
                              WUSA_EXE_ARGUMENTS);
    if (hr != S_OK) {
                   
        error = ERROR_INSTALL_FAILURE;
        wprintf(L"StringCbPrintf failed for applicationParameters, %x\n",
               hr);
        goto exit;
    }

    wprintf(L"Invoking: %s\n", commandLine);
    
    ok = CreateProcess(applicationName,    // name of executable module
                       commandLine,        // command line string
                       NULL,               // SD
                       NULL,               // SD
                       TRUE,               // handle inheritance option
                       0,                  // creation flags    CREATE_NO_WINDOW
                       NULL,               // new environment block
                       NULL,               // current directory name
                       &startInfo,         // startup information
                       &pInfo              // process information
                       );

    if (ok == FALSE) {
        
        error = GetLastError();
        wprintf(L"Create process failed : %x\n",
                error);
        goto exit;
        
    } else {
    
        //
        // Wait until child process exits.
        //
        
        error = WaitForSingleObject( pInfo.hProcess, INFINITE );

        if ( error != WAIT_OBJECT_0 ) {
            //
            // It can't hurt to add this
            //
            TerminateProcess(pInfo.hProcess, (UINT)-1);
        }

        GetExitCodeProcess(pInfo.hProcess, &error);

        //
        // The possible return values for wusa.exe are:
        // 1)ERROR_SUCCESS (0) : installation was successfull
        // 2)ERROR_SUCCESS_REBOOT_REQUIRED (3010) : installation was successful,
        // however a reboot is required, so that the binaries will be
        // loaded to memory
        // 3)S_FALSE (1) (Vista) OR WU_S_ALREADY_INSTALLED (240006) (Win7): 
        // No action was taken (i.e. files were already installed)
        // 4)Everything else (e.g. ERROR_INSTALL_FAILURE) is an error
        //

        switch (error) {
        case S_FALSE:
        case WU_S_ALREADY_INSTALLED:
            wprintf(L"The package was already installed in the system\n");
            error = ERROR_SUCCESS;
            break;
        case ERROR_SUCCESS:
            wprintf(L"The package was installed successfully\n");
            break;
        case ERROR_SUCCESS_REBOOT_REQUIRED:
            wprintf(L"The package was installed successfully but requires a reboot\n");
            break;
        case ERROR_SERVICE_DISABLED:
        case WU_E_WU_DISABLED:

            //
            // If the "Windows Update" service is disabled, then wusa
            // returns ERROR_SERVICE_DISABLED
            //
            
            wprintf(L"The \"Windows Update\" service is disabled. It "
                    L"has to be enabled for the installation to succeed."
                    L"\n");
            break;
        default:
            wprintf(L"The update process returned error code :%x. ",
                    error);
            wprintf(L"For additional information please look at the log "
                    L"files %%windir%%\\windowsupdate.log and "
                    L"%%windir%%\\Logs\\CBS\\CBS.log\n");
            break;
        }

        CloseHandle(pInfo.hProcess);
        CloseHandle(pInfo.hThread);
    }

exit:
    if (commandLine != NULL) {
        LocalFree(commandLine);
        commandLine = NULL;
    }

    if (applicationName != NULL) {
        LocalFree(applicationName);
        applicationName = NULL;
    }

    return error;
}

BOOL
PromptRestart()
{
   HANDLE hToken;              // handle to process token 
   TOKEN_PRIVILEGES tkp;       // pointer to token structure 
   BOOL fResult;               // system shutdown flag 
 
   // Get the current process token handle so we can get shutdown 
   // privilege. 
 
   if (!OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
      return FALSE; 
 
   // Get the LUID for shutdown privilege. 
 
   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
        &tkp.Privileges[0].Luid); 
 
   tkp.PrivilegeCount = 1;  // one privilege to set    
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
   // Get shutdown privilege for this process. 
 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
      (PTOKEN_PRIVILEGES) NULL, 0); 
 
   // Cannot test the return value of AdjustTokenPrivileges. 
 
   if (GetLastError() != ERROR_SUCCESS) {
      return FALSE; 
    }
 
   // Display the shutdown dialog box and start the countdown. 

   #pragma prefast(suppress:28159, "Ignore the suggestion against system shutdown.")
   fResult = InitiateSystemShutdownEx( 
                      NULL,    // shut down local computer 
                      NULL,    // message for user
                      0,       // time-out period, in seconds 
                      FALSE,   // ask user to close apps 
                      TRUE,    // reboot after shutdown  
                      SHTDN_REASON_FLAG_PLANNED // shutdown reason
                      | SHTDN_REASON_MAJOR_SOFTWARE
                      | SHTDN_REASON_MINOR_UPGRADE);
 
   // Disable shutdown privilege. 
 
   tkp.Privileges[0].Attributes = 0; 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
        (PTOKEN_PRIVILEGES) NULL, 0);
   
   return fResult; 

}

DWORD
UpdateWdf(
    VOID
    )
{
    BOOL ok;
    OSVERSIONINFO curOsvi;
    OSVERSIONINFOEX targetOsvi;
    DWORDLONG dwlConditionMask = 0;
    WCHAR MSUName[MAX_PATH]; 
    BOOL rebootNeeded = FALSE;
    DWORD error = ERROR_SUCCESS;
    HRESULT hr;
    
    //
    // Make sure updates are valid for this operating system.
    // Vista SP1/SP2; Win7 RTM
    // TODO: what happens on Vista RTM/SP3+/Win7 SP1+
    //
    
    ZeroMemory(&targetOsvi, sizeof(OSVERSIONINFOEX));
    dwlConditionMask = 0;
    
    targetOsvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);  
    targetOsvi.dwMajorVersion = 6;
    targetOsvi.dwMinorVersion = 0;
    targetOsvi.wServicePackMajor = 0;

    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_LESS_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_LESS_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_LESS_EQUAL );

    ok = VerifyVersionInfo( &targetOsvi,
                            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR,
                            dwlConditionMask);

    if (ok) {
        wprintf(L"Error: Updates are not supported on OS before Vista or Vista RTM\n");
        error = ERROR_OLD_WIN_VERSION;
        goto exit;
    }

    //
    // No need to update on Win8+
    //
    ZeroMemory(&targetOsvi, sizeof(OSVERSIONINFOEX));
    dwlConditionMask = 0;
    
    targetOsvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);  
    targetOsvi.dwMajorVersion = 6;
    targetOsvi.dwMinorVersion = 2;

    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );

    ok = VerifyVersionInfo( &targetOsvi,
                            VER_MAJORVERSION | VER_MINORVERSION,
                            dwlConditionMask);

    if (ok) {
        wprintf(L"Updates are not needed to Windows 8, they are already inbox\n");
        error = ERROR_SUCCESS;
        goto exit;
    }
    
    //
    // Create MSU name
    // 
    
    ZeroMemory(&curOsvi, sizeof(OSVERSIONINFO));
    curOsvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

#pragma warning( push )
#pragma warning( disable : 4996 ) // 'GetVersionEx': was declared deprecated
    ok = GetVersionEx(&curOsvi);

    if (ok == FALSE) {

        error = GetLastError();
        wprintf(L"GetVersionEx failed: %x\n", error);
        goto exit;
    }
    
    //
    // We want to apply the updates in a specific order.  This is because UMDF
    // is potentially dependent on WinUSB.  WinUSB is dependent on KMDF. If we
    // fail to apply an update, we want to make sure the machine is in a good
    // state.  This we apply required framework updates first.
    // KMDF > WinUSB > UMDF
    //
    
#if INSTALL_KMDF
    hr = StringCchPrintf(MSUName,
                         MAX_PATH,
                         MSU_FORMAT_STRING,
                         L"kmdf",
                         WDF_MAJOR_VERSION,
                         WDF_MINOR_VERSION,
                         curOsvi.dwMajorVersion,
                         curOsvi.dwMinorVersion);

    if (hr != S_OK) {
        wprintf(L"StringCchPrintf for KMDF MSU failed: %x\n", hr);
        error = ERROR_INSTALL_FAILURE;
        goto exit;
    }

    error = ApplyUpdate(MSUName);

    if (error == ERROR_SUCCESS_REBOOT_REQUIRED) {
        rebootNeeded = TRUE;
    } else if (error != ERROR_SUCCESS) {
        goto exit;
    }
#endif // INSTALL_KMDF
#pragma warning( pop ) // 'GetVersionEx': was declared deprecated

#if (INSTALL_WINUSB)
    //
    // WinUSB update only applies to Vista
    //

    ZeroMemory(&targetOsvi, sizeof(OSVERSIONINFOEX));
    dwlConditionMask = 0;
    
    targetOsvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);  
    targetOsvi.dwMajorVersion = 6;
    targetOsvi.dwMinorVersion = 0;

    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_EQUAL );

    ok = VerifyVersionInfo( &targetOsvi,
                            VER_MAJORVERSION | VER_MINORVERSION,
                            dwlConditionMask);

    if (ok) {
        error = ApplyUpdate(WINUSB_UPDATE_NAME);

        if (error == ERROR_SUCCESS_REBOOT_REQUIRED) {
            rebootNeeded = TRUE;
        } else if (error != ERROR_SUCCESS) {
            goto exit;
        }
    }
#endif  // INSTALL_WINUSB

#if INSTALL_UMDF
    hr = StringCchPrintf(MSUName,
                         MAX_PATH,
                         MSU_FORMAT_STRING,
                         L"umdf",
                         WDF_MAJOR_VERSION,
                         WDF_MINOR_VERSION,
                         curOsvi.dwMajorVersion,
                         curOsvi.dwMinorVersion);

    if (hr != S_OK) {
        wprintf(L"StringCchPrintf for UMDF MSU failed: %x\n", hr);
        error = ERROR_INSTALL_FAILURE;
        goto exit;
    }

    error = ApplyUpdate(MSUName);

    if (error == ERROR_SUCCESS_REBOOT_REQUIRED) {
        rebootNeeded = TRUE;
    } else if (error != ERROR_SUCCESS) {
        goto exit;
    }
#endif // INSTALL_UMDF

    //
    // If we have made it to this point there have been no fatal errors. If
    // there were fatal errors, these should be caught and we would've jumped
    // to exit.
    // 
    // We must account for the fact the latest update applied did not require
    // a reboot but earlier updates did.
    //
    if (rebootNeeded == TRUE) {
        
        error = ERROR_SUCCESS_REBOOT_REQUIRED;
        goto exit;
    }

    error = ERROR_SUCCESS;
    
exit:
    
    return error;
}

int __cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) char* argv[]
    )
{
    DWORD updateStatus;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    updateStatus = UpdateWdf();

    if (updateStatus == ERROR_SUCCESS_REBOOT_REQUIRED) {
        
        int msgboxID = MessageBox(
            NULL,
            L"A restart is needed for these changes to take effect\nRestart now?",
            L"Restart Required",
            MB_ICONEXCLAMATION | MB_YESNO
        );

        if (msgboxID == IDYES)
        {
            PromptRestart();
        }
    }

    return updateStatus;
}
