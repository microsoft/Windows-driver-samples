/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    render.h

Abstract:

    Contains structure definitions and function prototypes private to
    the driver.

Environment:

    Kernel mode

--*/

#pragma once

//
// Device settings for raw PDO.
//
DECLARE_CONST_UNICODE_STRING(DeviceId, L"SDCAVad\\ExtensionDevice");
DECLARE_CONST_UNICODE_STRING(HardwareId, L"SDCAVad\\ExtensionDevice");
DECLARE_CONST_UNICODE_STRING(InstanceId, L"00");
DECLARE_CONST_UNICODE_STRING(CompatibleId, SDCAVAD_COMPATIBLE_ID);
DECLARE_CONST_UNICODE_STRING(ContainerId, SDCAVAD_CONTAINER_ID);
DECLARE_CONST_UNICODE_STRING(DeviceDescription, L"SDCAVad Extension Device");
DECLARE_CONST_UNICODE_STRING(DeviceLocation, L"SDCAVad");

