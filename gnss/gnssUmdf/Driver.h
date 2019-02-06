/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

EVT_WDF_DRIVER_UNLOAD OnDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP GnssUmdfEvtDriverContextCleanup;
