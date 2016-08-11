#pragma once

#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_driver_)

#define SAFE_DELETE(p) \
{                       \
    if(p)               \
    {                   \
        delete p;       \
        p = NULL;       \
    }                   \
}

///////////////////////////////////////////////////////////////////////////////
// Windows system headers
//

#include <windows.h>            // Windows defines
#include <stdio.h>              // std out defines
#include <coguid.h>             // COM defines
#include <objbase.h>            // COM defines
#include <shobjidl.h>           // Shell UI Extension
#include <shlobj.h>             // Shell UI Extension
#include <gdiplus.h>            // GDI+
#include <shlwapi.h>            // Shell light weight API


///////////////////////////////////////////////////////////////////////////////
// WIA common library headers
//

#include "basicstr.h"
#include "basicarray.h"           // CSimpleDynamicArray

///////////////////////////////////////////////////////////////////////////////
// WIA driver core headers
//

#include <sti.h>                // STI defines
#include <stiusd.h>             // IStiUsd interface
#include <wiamindr.h>           // IWiaMinidrv interface
#include <wiadevd.h>            // IWiaUIExtension interface
#include <wiamdef.h>


///////////////////////////////////////////////////////////////////////////////
// WIA driver common headers

#define DEFAULT_BUFFER_SIZE             (128 * 1024)

#include "wiapropertymanager.h"     // WIA driver property manager class
#include "wiacapabilitymanager.h"   // WIA driver capability manager class
#include "wiahelpers.h"             // WIA driver helper functions

#include "resource.h"               // WIA driver resource defines
#include "WiaDevice.h"              // WIA simulated device class
#include "wiadriver.h"              // WIA driver header

