// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

#include <initguid.h>

#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#include <new>
#endif

#include <wdf.h>
#include <netadaptercx.h>
