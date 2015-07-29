/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    vfeature.h

Abstract:

    This module contains the Windows Driver Framework HID I/O Target
    handlers for the firefly filter driver.

Environment:

    Kernel mode

--*/

#if !defined(_VFEATURE_H_)
#define _VFEATURE_H_

NTSTATUS
FireflySetFeature(
    IN  PDEVICE_CONTEXT DeviceContext,
    IN  UCHAR           PageId,
    IN  USHORT          FeatureId,
    IN  BOOLEAN         EnableFeature
    );

#endif // _VFEATURE_H
