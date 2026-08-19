// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#include <initguid.h>

#ifndef _KERNEL_MODE
// This is a user-mode driver
#include <windows.h>

#else
// This is a kernel-mode driver
#include <ntddk.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#endif

// This is a common WDF header (for both KMDF and UMDF)
#include <wdf.h>

#include <netadaptercx.h>
//#include <wdftriage.h>
#include "net/netringiterator.h"
#include "net/netpacketlibrary.h"
#include <net/rsc.h>
#include <net/gso.h>
#include <net/checksum.h>
#include <net/databuffer.h>
#include <net/returncontext.h>

#include "enlthreads.h"
#include "enl.h"

#include "trace.h"

