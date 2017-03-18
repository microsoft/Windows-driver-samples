/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    peripheral.h

Abstract:

    This module contains the function definitions for 
    interaction with the SPB API.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _PERIPHERAL_H_
#define _PERIPHERAL_H_

EVT_WDF_REQUEST_COMPLETION_ROUTINE SpbPeripheralOnCompletion;
EVT_WDF_REQUEST_CANCEL             SpbPeripheralOnCancel;

EVT_WDF_REQUEST_CANCEL             SpbPeripheralOnWaitOnInterruptCancel;

NTSTATUS
SpbPeripheralOpen(
    _In_  PDEVICE_CONTEXT  pDevice);

NTSTATUS
SpbPeripheralClose(
    _In_  PDEVICE_CONTEXT  pDevice);

VOID
SpbPeripheralLock(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest);

VOID
SpbPeripheralUnlock(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest);

VOID
SpbPeripheralLockConnection(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest);

VOID
SpbPeripheralUnlockConnection(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest);

VOID
SpbPeripheralRead(
    _In_   PDEVICE_CONTEXT  pDevice,
    _In_   WDFREQUEST       FxRequest);

VOID
SpbPeripheralWrite(
    _In_   PDEVICE_CONTEXT  pDevice,
    _In_   WDFREQUEST       FxRequest);

VOID
SpbPeripheralWriteRead(
    _In_   PDEVICE_CONTEXT  pDevice,
    _In_   WDFREQUEST       FxRequest);

VOID
SpbPeripheralFullDuplex(
    _In_   PDEVICE_CONTEXT  pDevice,
    _In_   WDFREQUEST       FxRequest);

VOID
SpbPeripheralSignalInterrupt(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest);

VOID
SpbPeripheralWaitOnInterrupt(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       FxRequest);

BOOLEAN
SpbPeripheralInterruptNotify(
    _In_  PDEVICE_CONTEXT  pDevice);

NTSTATUS
SpbPeripheralSendRequest(
    _In_  PDEVICE_CONTEXT  pDevice,
    _In_  WDFREQUEST       SpbRequest,
    _In_  WDFREQUEST       ClientRequest);

VOID
SpbPeripheralCompleteRequestPair(
    _In_  PDEVICE_CONTEXT   pDevice,
    _In_  NTSTATUS          status,
    _In_  ULONG_PTR         bytesCompleted);

#endif // _PERIPHERAL_H_
