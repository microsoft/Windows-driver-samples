// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Windows Header Files:
#include <windows.h>
#include <minwindef.h>
#include <ks.h>
#include <propvarutil.h>
//#include <mfstd.h> // Must be included before <initguid.h>, or else DirectDraw GUIDs will be defined twice. See the comment in <uuids.h>.
#include <ole2.h>
#include <initguid.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <nserror.h>
#include <winmeta.h>
#include <wrl.h>
#include <d3d9types.h>
#include <ksmedia.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;