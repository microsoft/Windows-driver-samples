/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ioctl.c

Abstract:

    This is the ioctl handler for the fakemodem.
Environment:

    Kernel mode

--*/

#include "fakemodem.h"



VOID
FmEvtIoDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t        OutputBufferLength,
    IN size_t        InputBufferLength,
    IN ULONG        IoControlCode
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{

    PFM_DEVICE_DATA   fmDeviceData = FmDeviceDataGet(WdfIoQueueGetDevice(Queue));
    NTSTATUS          status ;
    PVOID             requestBuffer;
    ULONG             information = 0;
    size_t            bufSize;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    status = STATUS_SUCCESS;

    switch (IoControlCode) {

        case IOCTL_SERIAL_GET_WAIT_MASK: {

            status = WdfRequestRetrieveOutputBuffer ( Request,
                                                      sizeof(ULONG),
                                                      &requestBuffer,
                                                      &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(( "Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }

            *((PULONG)requestBuffer)= fmDeviceData->CurrentMask;

            information = sizeof(ULONG);

            break;
        }

        case IOCTL_SERIAL_SET_WAIT_MASK: {

            WDFREQUEST    currentWaitRequest=NULL;
            ULONG         newMask;

            status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof(ULONG),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(("Could not get request memory buffer status %X\n", status));
                information = 0;
                break;

            }
            else {
                NTSTATUS tempStatus;
                //
                //  get rid of the current wait
                //

                newMask = *((ULONG *)requestBuffer);

                fmDeviceData->CurrentMask = newMask;

                KdPrint(("FAKEMODEM: set wait mask, %08lx\n", newMask));

                tempStatus = WdfIoQueueRetrieveNextRequest(fmDeviceData->FmMaskWaitQueue,
                                                        &currentWaitRequest);
                //  save the new mask

                if (NT_SUCCESS(tempStatus)) {//currentWaitRequest != NULL) {

                    PULONG  outBuffer;

                    KdPrint(("FAKEMODEM: set wait mask- complete wait\n"));

                    //
                    // The length was validated already.
                    //
                    tempStatus = WdfRequestRetrieveOutputBuffer(currentWaitRequest,
                                                            sizeof(ULONG),
                                                            &outBuffer,
                                                            &bufSize);
                    if (NT_SUCCESS(tempStatus)) {
                        if (outBuffer) {        // FIXME SAL
                            *outBuffer = 0;
                        }

                        WdfRequestCompleteWithInformation(currentWaitRequest,
                                                         STATUS_SUCCESS,
                                                         sizeof(ULONG));
                    } else {
                        WdfRequestComplete(currentWaitRequest, tempStatus);
                    }
                }
                information = sizeof(ULONG);
            }
            break;
        }

        case IOCTL_SERIAL_WAIT_ON_MASK: {

            WDFREQUEST    currentWaitRequest = NULL;
            NTSTATUS    tempStatus;

            status = WdfRequestRetrieveOutputBuffer (Request,
                                                    sizeof(ULONG),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {

               KdPrint(("Could not get request memory buffer status %X\n", status));
               information = 0;
               status = STATUS_BUFFER_TOO_SMALL;
               break;
            }

            KdPrint(("FAKEMODEM: wait on mask\n"));
            //
            //  remove the current request if any
            //
            tempStatus = WdfIoQueueRetrieveNextRequest(fmDeviceData->FmMaskWaitQueue,
                                                   &currentWaitRequest);

            if (NT_SUCCESS(tempStatus)) { //currentWaitRequest != NULL) {

                PULONG  outBuffer;

                KdPrint(("FAKEMODEM: wait on mask- complete wait\n"));

                //
                // The length was validated already.
                //

                tempStatus = WdfRequestRetrieveOutputBuffer(currentWaitRequest,
                                                        sizeof(ULONG),
                                                        &outBuffer,
                                                        &bufSize);
                if(NT_SUCCESS(tempStatus)) {
                    if (outBuffer) {                 // FIXME SAL
                        *((PULONG)outBuffer) = 0;
                    }

                    WdfRequestCompleteWithInformation(currentWaitRequest,
                                                      STATUS_SUCCESS,
                                                      sizeof(ULONG));
                } else {
                    WdfRequestComplete(currentWaitRequest, tempStatus);
                }

            }

            if (fmDeviceData->CurrentMask == 0) {
                //
                // can only set if mask is not zero
                //
                status=STATUS_UNSUCCESSFUL;

            } else {

                //
                // add the current request to the wait queue
                //
                status = WdfRequestForwardToIoQueue(Request, fmDeviceData->FmMaskWaitQueue);
                if (!NT_SUCCESS(status)) {
                    WdfRequestCompleteWithInformation(Request, STATUS_UNSUCCESSFUL, 0);
                    return;
                }

                status=STATUS_PENDING;
            }

            break;
        }

        case IOCTL_SERIAL_PURGE: {

            ULONG mask;
            status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof(ULONG),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(("Could not get request memory buffer status %X\n", status));
                information = 0;
                break;

            }
            mask=*((PULONG)requestBuffer);

            if (mask & SERIAL_PURGE_RXABORT) {

                WdfIoQueuePurge( fmDeviceData->FmReadQueue,
                                          WDF_NO_EVENT_CALLBACK,
                                          WDF_NO_CONTEXT );

                WdfIoQueueStart(fmDeviceData->FmReadQueue);
            }
            information = sizeof(ULONG);
            break;
        }


        case IOCTL_SERIAL_GET_MODEMSTATUS: {

            status = WdfRequestRetrieveOutputBuffer ( Request,
                                                      sizeof(ULONG),
                                                      &requestBuffer,
                                                      &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(( "Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }

            information = sizeof(ULONG);

            *((PULONG)requestBuffer) = fmDeviceData->ModemStatus;

            break;
        }


        case IOCTL_SERIAL_SET_TIMEOUTS: {
            PSERIAL_TIMEOUTS NewTimeouts;

            status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof(SERIAL_TIMEOUTS),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(("Could not get request memory buffer status %X\n", status));
                information = 0;
                break;

            }
            NewTimeouts= ((PSERIAL_TIMEOUTS)(requestBuffer));

            if ((NewTimeouts->ReadIntervalTimeout == MAXULONG) &&
                (NewTimeouts->ReadTotalTimeoutMultiplier == MAXULONG) &&
                (NewTimeouts->ReadTotalTimeoutConstant == MAXULONG))
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            information = sizeof(SERIAL_TIMEOUTS);

            break;
        }

        case IOCTL_SERIAL_GET_TIMEOUTS: {

            status = WdfRequestRetrieveOutputBuffer ( Request,
                                                      sizeof(SERIAL_TIMEOUTS),
                                                      &requestBuffer,
                                                      &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(( "Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }

            *((PSERIAL_TIMEOUTS)requestBuffer) =
                fmDeviceData->CurrentTimeouts;

            information = sizeof(SERIAL_TIMEOUTS);

            break;
        }

        case IOCTL_SERIAL_GET_COMMSTATUS: {

            PSERIAL_STATUS serialStatus  ;
            status = WdfRequestRetrieveOutputBuffer ( Request,
                                                      sizeof(SERIAL_STATUS),
                                                      &requestBuffer,
                                                      &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(( "Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }

            serialStatus   = (PSERIAL_STATUS) requestBuffer;

            RtlZeroMemory( serialStatus, sizeof(*serialStatus) );   // FIXME SAL

            serialStatus->AmountInInQueue = fmDeviceData->BytesInReadBuffer;

            information = sizeof(SERIAL_STATUS);

            break;
        }

        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR: {

            if (IoControlCode == IOCTL_SERIAL_SET_DTR) {

                //
                //  raising DTR
                //

                fmDeviceData->ModemStatus=SERIAL_DTR_STATE | SERIAL_DSR_STATE;

                KdPrint(("FAKEMODEM: Set DTR\n"));

            } else {
                //
                //  dropping DTR, drop connection if there is one
                //
                KdPrint(("FAKEMODEM: Clear DTR\n"));

                if (fmDeviceData->CurrentlyConnected == TRUE) {
                    //
                    //  not connected any more
                    //
                    fmDeviceData->CurrentlyConnected=FALSE;

                    fmDeviceData->ConnectionStateChanged=TRUE;
                }
            }

            ProcessConnectionStateChange( fmDeviceData);

            information = sizeof(ULONG);

            break;
        }

       case IOCTL_SERIAL_SET_QUEUE_SIZE: {

            status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof(SERIAL_QUEUE_SIZE),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(("Could not get request memory buffer status %X\n", status));
                information = 0;
                break;

            }

           //
           // This ioctl doesn't do anyhing except test for the size of the
           // buffer passed in.
           //
           information = sizeof(SERIAL_QUEUE_SIZE);
           break;
        }


        case IOCTL_SERIAL_SET_BAUD_RATE: {

            status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof(SERIAL_BAUD_RATE),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(("Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }
            else {
                fmDeviceData->BaudRate = ((PSERIAL_BAUD_RATE)requestBuffer)->BaudRate;
            }
            information = sizeof(SERIAL_BAUD_RATE);
            break;
        }

        case IOCTL_SERIAL_GET_BAUD_RATE: {

            PSERIAL_BAUD_RATE pBaudRate ;

            status = WdfRequestRetrieveOutputBuffer ( Request,
                                                      sizeof(SERIAL_BAUD_RATE),
                                                      &requestBuffer,
                                                      &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(( "Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }

            pBaudRate = (PSERIAL_BAUD_RATE)requestBuffer;
            pBaudRate->BaudRate = fmDeviceData->BaudRate;
            information = sizeof(SERIAL_BAUD_RATE);

            break;
        }

        case IOCTL_SERIAL_SET_LINE_CONTROL: {

            PSERIAL_LINE_CONTROL pLineControl ;
            UCHAR LData = 0;
            UCHAR LStop = 0;
            UCHAR LParity = 0;
            UCHAR Mask = 0xff;

            status = WdfRequestRetrieveInputBuffer (Request,
                                                    sizeof(SERIAL_LINE_CONTROL),
                                                    &requestBuffer,
                                                    &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(("Could not get request memory buffer status %X\n", status));
                information = 0;
                break;

            }
            pLineControl = ((PSERIAL_LINE_CONTROL)requestBuffer);

            switch(pLineControl->WordLength)
            {
                case 5: {

                    LData = SERIAL_5_DATA;
                    Mask = 0x1f;
                    break;

                }
                case 6: {

                    LData = SERIAL_6_DATA;
                    Mask = 0x3f;
                    break;

                }
                case 7: {

                    LData = SERIAL_7_DATA;
                    Mask = 0x7f;
                    break;

                }
                case 8: {

                    LData = SERIAL_8_DATA;
                    break;

                }
                default: {

                    status = STATUS_INVALID_PARAMETER;

                }
            }

            if (status != STATUS_SUCCESS)
            {
                break;
            }

            switch (pLineControl->Parity) {

                case NO_PARITY: {
                    LParity = SERIAL_NONE_PARITY;
                    break;

                }
                case EVEN_PARITY: {
                    LParity = SERIAL_EVEN_PARITY;
                    break;

                }
                case ODD_PARITY: {
                    LParity = SERIAL_ODD_PARITY;
                    break;

                }
                case SPACE_PARITY: {
                    LParity = SERIAL_SPACE_PARITY;
                    break;

                }
                case MARK_PARITY: {
                    LParity = SERIAL_MARK_PARITY;
                    break;

                }
                default: {

                    status = STATUS_INVALID_PARAMETER;
                    break;
                }

            }

            if (status != STATUS_SUCCESS)
            {
                break;
            }

            switch (pLineControl->StopBits) {

                case STOP_BIT_1: {

                    LStop = SERIAL_1_STOP;
                    break;
                }

                case STOP_BITS_1_5: {

                    if (LData != SERIAL_5_DATA) {

                        status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                    LStop = SERIAL_1_5_STOP;
                    break;

                }
                case STOP_BITS_2: {

                    if (LData == SERIAL_5_DATA) {

                        status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    LStop = SERIAL_2_STOP;
                    break;
                }

                default: {

                    status = STATUS_INVALID_PARAMETER;
                }

            }

            if (status != STATUS_SUCCESS)
            {
                break;
            }

            fmDeviceData->LineControl =
                (UCHAR)((fmDeviceData->LineControl & SERIAL_LCR_BREAK) |
                        (LData | LParity | LStop));

            fmDeviceData->ValidDataMask = Mask;
            information = sizeof(SERIAL_LINE_CONTROL);
            break;
        }

        case IOCTL_SERIAL_GET_LINE_CONTROL: {
            PSERIAL_LINE_CONTROL pLineControl ;

            status = WdfRequestRetrieveOutputBuffer ( Request,
                                                      sizeof(SERIAL_LINE_CONTROL),
                                                      &requestBuffer,
                                                      &bufSize );
            if( !NT_SUCCESS(status) ) {
                KdPrint(( "Could not get request memory buffer status %X\n", status));
                information = 0;
                break;
            }


            pLineControl =
                (PSERIAL_LINE_CONTROL)requestBuffer;

            RtlZeroMemory(requestBuffer,
                          bufSize);


            switch (fmDeviceData->LineControl & SERIAL_DATA_MASK) {
                case SERIAL_5_DATA:
                    pLineControl->WordLength = 5;
                    break;
                case SERIAL_6_DATA:
                    pLineControl->WordLength = 6;
                    break;
                case SERIAL_7_DATA:
                    pLineControl->WordLength = 7;
                    break;
                case SERIAL_8_DATA:
                    pLineControl->WordLength = 8;
                    break;
                default:
                    break;

            }

            switch (fmDeviceData->LineControl & SERIAL_PARITY_MASK) {
                case SERIAL_NONE_PARITY:
                    pLineControl->Parity = NO_PARITY;
                    break;
                case SERIAL_ODD_PARITY:
                    pLineControl->Parity = ODD_PARITY;
                    break;
                case SERIAL_EVEN_PARITY:
                    pLineControl->Parity = EVEN_PARITY;
                    break;
                case SERIAL_MARK_PARITY:
                    pLineControl->Parity = MARK_PARITY;
                    break;
                case SERIAL_SPACE_PARITY:
                    pLineControl->Parity = SPACE_PARITY;
                    break;
                default:
                    break;

            }

            if (fmDeviceData->LineControl & SERIAL_2_STOP) {

                if (pLineControl->WordLength == 5) {

                    pLineControl->StopBits = STOP_BITS_1_5;

                } else {

                    pLineControl->StopBits = STOP_BITS_2;

                }

            } else {

                pLineControl->StopBits = STOP_BIT_1;
            }

            information = sizeof(SERIAL_LINE_CONTROL);

            break;
        }

        case IOCTL_SERIAL_SET_RTS:
        case IOCTL_SERIAL_CLR_RTS:
        case IOCTL_SERIAL_SET_XON:
        case IOCTL_SERIAL_SET_XOFF:
        case IOCTL_SERIAL_SET_CHARS:
        case IOCTL_SERIAL_GET_CHARS:
        case IOCTL_SERIAL_GET_HANDFLOW:
        case IOCTL_SERIAL_SET_HANDFLOW:
        case IOCTL_SERIAL_RESET_DEVICE: {
            //
            // NOTE: The application expects STATUS_SUCCESS for these ioctsl.
            //  so don't merge this with default.
            //
            break;
        }
        default:
            status=STATUS_NOT_SUPPORTED;
            break;

    }

    if (status != STATUS_PENDING) {
        //
        //  complete now if not pending
        //
        WdfRequestCompleteWithInformation(Request, status, information);
    }
}



VOID
ProcessConnectionStateChange(
    IN PFM_DEVICE_DATA  FmDeviceData
    )
{
    WDFREQUEST  currentWaitRequest = NULL;
    NTSTATUS status;

    if (FmDeviceData->ConnectionStateChanged) {

        //
        //  state changed
        //

        FmDeviceData->ConnectionStateChanged=FALSE;

        if (FmDeviceData->CurrentlyConnected) {
            //
            //  now it is connected, raise CD
            //
            FmDeviceData->ModemStatus |= SERIAL_DCD_STATE;


        } else {
            //
            //  not  connected any more, clear CD
            //
            FmDeviceData->ModemStatus &= ~(SERIAL_DCD_STATE);

        }


        if (FmDeviceData->CurrentMask & SERIAL_EV_RLSD) {

            //
            //  app want's to know about these changes, tell it
            //
            status = WdfIoQueueRetrieveNextRequest(FmDeviceData->FmMaskWaitQueue,
                                                &currentWaitRequest);
            if(!NT_SUCCESS(status)){
                ASSERT(status == STATUS_NO_MORE_ENTRIES);
            }
        }

    }


    if (currentWaitRequest != NULL) {

        PULONG  outBuffer;
        size_t   bufSize;

        KdPrint(("FAKEMODEM: ProcessConectionState\n"));

        //
        // The length was validated already.
        //

        (VOID)WdfRequestRetrieveOutputBuffer(currentWaitRequest,
                                             sizeof(ULONG),
                                             &outBuffer,
                                             &bufSize);

        if (outBuffer) {                     // FIXME SAL
            *outBuffer = SERIAL_EV_RLSD;
        }

        WdfRequestCompleteWithInformation(currentWaitRequest,
                                         STATUS_SUCCESS,
                                         sizeof(ULONG));

    }

    return;

}


