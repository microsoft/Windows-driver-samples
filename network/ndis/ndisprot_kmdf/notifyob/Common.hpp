/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Common.hpp

Abstract:

    This is the header file containing the common include files for the project.

--*/

#ifndef _COMMON_HPP_
#define _COMMON_HPP

#include <windows.h>
#include <atlbase.h>

extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>

#include <wdfinstaller.h>

#include "Resource.h"
#include "ProtNotify.h"

extern PFN_WDFPREDEVICEINSTALL  pfnWdfPreDeviceInstall;
extern PFN_WDFPOSTDEVICEINSTALL pfnWdfPostDeviceInstall;
extern PFN_WDFPREDEVICEREMOVE   pfnWdfPreDeviceRemove;
extern PFN_WDFPOSTDEVICEREMOVE  pfnWdfPostDeviceRemove;
extern ATL::_ATL_OBJMAP_ENTRY* ObjectMap;

#endif // _COMMON_HPP_

