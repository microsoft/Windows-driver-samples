/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    controller.h

Abstract:

    This module contains the controller-specific function
    definitions.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

//
// Controller specific function prototypes.
//

VOID ControllerInitialize(
    _In_  PPBC_DEVICE   pDevice);

VOID ControllerUninitialize(
    _In_  PPBC_DEVICE   pDevice);

VOID
ControllerConfigureForTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest);

NTSTATUS
ControllerTransferData(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest);
    
VOID
ControllerCompleteTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest,
    _In_  BOOLEAN       AbortSequence);

VOID
ControllerEnableInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask);

VOID
ControllerDisableInterrupts(
    _In_  PPBC_DEVICE   pDevice);

ULONG
ControllerGetInterruptStatus(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask);

VOID
ControllerAcknowledgeInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask);

VOID
ControllerProcessInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest,
    _In_  ULONG         InterruptStatus);

#endif
