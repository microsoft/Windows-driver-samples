//
//    Copyright (C) Microsoft.  All rights reserved.
//    SPDX-License-Identifier: MS-PL
//

#include <ntddk.h>
#include <devioctl.h>
#include <initguid.h>
#include <wdmguid.h>
#include <ntddser.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntstrsafe.h>
#include "log.h"
#include "serenum.h"

