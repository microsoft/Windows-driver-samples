#pragma warning(disable:4214)   // bit field types other than int

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4054)   // cast of function pointer to PVOID
#pragma warning(disable:4244)   // conversion from 'int' to 'BOOLEAN', possible loss of data
#pragma warning(disable:4206)   // nonstandard extension used : translation unit is empty

#define WIN9X_COMPAT_SPINLOCK

#include "ndis.h"
#include "ntddk.h"
#include <wdf.h>
#include <wmistr.h>
#include <wdmsec.h>
#include <wdmguid.h>
#include "debug.h"
#include "ndisprot.h"
#include "macros.h"
#include "protuser.h"
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
