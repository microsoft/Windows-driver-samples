//
// File History Sample Setup Tool
// Copyright (c) Microsoft Corporation.  All Rights Reserved.
//

#include <windows.h>
#include <atlcore.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <strsafe.h>

#include <fherrors.h>
#include <fhstatus.h>
#include <fhcfg.h>
#include <fhsvcctl.h>

//
// Define CLSID_FhConfigMgr.
// We must include initguid.h before using DEFINE_GUID otherwise
// DEFINE_GUID will declare CLSID_FhConfigMgr as extern.
//

#include <initguid.h>
DEFINE_GUID(CLSID_FhConfigMgr,0xED43BB3C,0x09E9,0x498a,0x9D,0xF6,0x21,0x77,0x24,0x4C,0x6D,0xB4);