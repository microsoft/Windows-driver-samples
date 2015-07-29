/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include "ufxclient.h"
#include "registers.h"
#include "trace.h"

#define UFX_CLIENT_TAG ('cXFU')


//---------------------------------------------------------------------------
// While macro to supress 4127 warning: Constant expression
//---------------------------------------------------------------------------
#define WHILE(constant) \
    __pragma(warning(suppress:4127)) while(constant)

