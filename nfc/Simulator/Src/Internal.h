/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Internal.h

Abstract:

    This module contains the local type definitions for the UMDF driver.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

//
// Include the type specific headers.
//
#include <windows.h>
#include <ntstatus.h>
#include <stdlib.h> 
#include <stddef.h>
#include <malloc.h>
#pragma warning( disable: 4201 )    // nonstandard extension used : nameless struct/union
#include <wudfwdm.h>
#include <devioctl.h>
#include <wdf.h>
#define INIT_GUID
#include <initguid.h>
#include <ntassert.h>
#include <winsmcrd.h>
#include <stdlib.h>
#include <strsafe.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <new.h>
#include <objbase.h>
#include <coguid.h>
#include <nfpdev.h>
#include <nfcsedev.h>
#include <nfcradiodev.h>
#include <ncidef.h>

//
// Define the tracing GUID for this driver
//
#define TRACE_CONTROL_GUID (12579E92, 1B46, 40A6, 9CFC, C718A677830B)

#ifndef SAFE_DELETEARRAY
#define SAFE_DELETEARRAY(x) if ((x) != nullptr) { delete [] (x); (x) = nullptr; }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) if ((x) != nullptr) { delete (x); (x) = nullptr; }
#endif

#ifndef SAFE_CLOSEHANDLE
#define SAFE_CLOSEHANDLE(x) if ((x) != nullptr) { CloseHandle(x); (x) = nullptr; }
#endif

#ifndef SAFE_FREELIBRARY
#define SAFE_FREELIBRARY(x) if ((x) != nullptr) { FreeLibrary(x); (x) = nullptr; }
#endif

#include "nfcsimulatorddi.h"
#include "constants.h"
#include "linklist.h"
#include "wppdefs.h"
#include "connection.h"
#include "filecontext.h"
#include "secureelement.h"
#include "driver.h"
#include "device.h"
#include "smartcard.h"
#include "smartcardreader.h"
#include "routingtable.h"
#include "queue.h"

