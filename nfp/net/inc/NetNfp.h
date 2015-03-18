/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    NetNfp.h

Abstract:

    This header contains definitions common to the both the NetNfpProvider.sys
    driver and the NetNfpControl.exe.

Environment:

    User Mode

--*/
#pragma once


/* 2DD081BE-1294-440B-AB9F-F0E9FDD77FBE */
const GUID GUID_DEVINTERFACE_NETNFP =
  {0x2DD081BE, 0x1294, 0x440B, {0xAB, 0x9F, 0xF0, 0xE9, 0xFD, 0xD7, 0x7F, 0xBE}};
  
#define IOCTL_BEGIN_PROXIMITY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1000, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct BEGIN_PROXIMITY_ARGS
{
    WCHAR szName[MAX_PATH];    // Name or IP address
};

