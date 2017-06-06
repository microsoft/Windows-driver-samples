/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CppWindowsService.cpp

Abstract:

    The file defines the entry point of the application. According to the
    arguments in the command line, the function installs or uninstalls or
    starts the service by calling into different routines.

Environment:

    User mode

--*/

#pragma region Includes
#include <stdio.h>
#include <windows.h>
#include "ServiceBase.h"
#include "SampleService.h"
#pragma endregion

// 
// Settings of the service
// 

//
// Internal name of the service
//
#define SERVICE_NAME             L"OsrUsbFx2UmUserSvc"


/*++

Routine Description:

    Entry point for the service.

Arguments:

    Argc - The number of command line arguments

    Argv - The array of command line arguments

Return Value:

    VOID

--*/
INT
wmain(
    INT    Argc,
    WCHAR *Argv[]
    )
{
    CSampleService service(SERVICE_NAME);

    if (!CServiceBase::Run(service))
    {
        wprintf(L"Service failed to run w/err 0x%08lx\n", GetLastError());
    }

    return 0;
}