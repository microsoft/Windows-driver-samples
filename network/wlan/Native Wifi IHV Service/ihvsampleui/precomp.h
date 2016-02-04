//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#pragma once

#include <driverspecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#include <shlobj.h>
#include <windows.h>
#include <objbase.h>
#include <unknwn.h>
#include <strsafe.h>
#include <assert.h>
#include <wlanihv.h>

// MIDL generated
#include "ihvsample.h"

#include "iunk.h"
#include "resource.h"
#include "utils.h"
#include "IHVRegistryHelper.h"
#include "IHVSampleExtUI.h"
#include "IHVSampleProfile.h"
#include "IHVSampleExtUICon.h"
#include "IHVSampleExtUISec.h"
#include "IHVSampleExtUIKey.h"
#include "IHVClassFactory.h"
#include <new>

const GUID GUID_SAMPLE_IHVUI_CLSID = 
{ 0x4a01f9f9, 0x6012, 0x4343, { 0xa8, 0xc4, 0x10, 0xb5, 0xdf, 0x32, 0x67, 0x2a } };


#define IHV_SAMPLE_IHV_NAME L"IHV"

#define BAIL_ON_FAILURE( _hr ) if (FAILED(_hr)) goto error;
#define BAIL( ) goto error;
#define SYS_FREE_STRING( _s ) if ( _s ) { SysFreeString( _s ); (_s) = NULL;}

