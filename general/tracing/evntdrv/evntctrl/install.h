/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    install.h

Abstract: 

    Win32 routines to dynamically load and unload a Windows NT kernel-mode
    driver using the Service Control Manager APIs.

Environment:

    User mode only


--*/

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02
#define DRIVER_FUNC_STOP        0x03

#define DRIVER_NAME       _T("Eventdrv")

BOOLEAN
ManageDriver(
    IN LPCTSTR  DriverName,
    IN LPCTSTR  ServiceName,
    IN USHORT   Function
    );

