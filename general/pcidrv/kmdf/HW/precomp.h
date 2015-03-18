//
// precomp.h for pcidrv driver
//
#define WIN9X_COMPAT_SPINLOCK
#include <ntddk.h> 
#include <wdf.h>

typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

#include <initguid.h> // required for GUID definitions
#include <wdmguid.h> // required for WMILIB_CONTEXT
#include <wmistr.h>
#include <wmilib.h>
#include <ntintsafe.h>


//
// Disable warnings that prevent our driver from compiling with /W4 MSC_WARNING_LEVEL
//
// Disable warning C4214: nonstandard extension used : bit field types other than int
// Disable warning C4201: nonstandard extension used : nameless struct/union
// Disable warning C4115: named type definition in parentheses
//
#pragma warning(disable:4214)
#pragma warning(disable:4201)
#pragma warning(disable:4115)

#include "ntddndis.h" // for OIDs

#pragma warning(default:4214)
#pragma warning(default:4201)
#pragma warning(default:4115)

#include "nuiouser.h" // for ioctls recevied from ndisedge
#include "public.h"

#include "e100_equ.h"
#include "e100_557.h"
#include "trace.h"
#include "nic_def.h"
#include "pcidrv.h"
#include "macros.h"


