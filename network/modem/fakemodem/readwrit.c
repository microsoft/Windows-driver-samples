/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    ioctl.c

Abstract:

    This is a simple form of function driver for Fm device. The driver
    doesn't handle any PnP and Power events because the framework provides
    default behaviour for those events. This driver has enough support to
    allow an user application (toast/notify.exe) to open the device
    interface registered by the driver and send read, write or ioctl requests.

Environment:

    Kernel mode

--*/

#include "fakemodem.h"


VOID
FmEvtIoRead(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t         Length
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_READ
    requests from the system.
    This routines defers read for later processing if there is no data to read.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

   VOID

--*/
{
    PFM_DEVICE_DATA    fmDeviceData = FmDeviceDataGet(WdfIoQueueGetDevice(Queue));
    ULONG              information;
    NTSTATUS           status;
    PUCHAR             systemBuffer;
    size_t             bufLen;

    status = WdfRequestRetrieveOutputBuffer(Request, Length, &systemBuffer, &bufLen);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }

    if (fmDeviceData->BytesInReadBuffer > 0) {

        ProcessReadBuffer(fmDeviceData,
                          systemBuffer,
                          (ULONG) Length,
                          &information);

        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, information);

        return;

    } else {

        //
        // No data to read. Queue the request for later processing.
        //

        status = WdfRequestForwardToIoQueue(Request, fmDeviceData->FmReadQueue);
        if (!NT_SUCCESS(status)) {
            WdfRequestCompleteWithInformation(Request, status, 0);
            return;
        }
    }
}

VOID
FmEvtIoWrite(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t        Length
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_WRITE
    requests from the system.
    This routine will also drain the read queue if there are pending reads.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

   VOID

--*/
{
    PFM_DEVICE_DATA    fmDeviceData = FmDeviceDataGet(WdfIoQueueGetDevice(Queue));
    PUCHAR             systemBuffer;
    NTSTATUS           status;
    size_t             length;
    WDFREQUEST         readRequest;

    status = WdfRequestRetrieveInputBuffer(Request, Length, &systemBuffer, &length);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }

    ProcessWriteBytes( fmDeviceData, systemBuffer, (ULONG) Length);

    //
    // Process read requests and complete them here.
    //

    while (fmDeviceData->BytesInReadBuffer > 0) {
        ULONG bytesToMove = 0;

        status = WdfIoQueueRetrieveNextRequest(fmDeviceData->FmReadQueue, &readRequest);
        if (!NT_SUCCESS(status)) {
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(readRequest, 0, &systemBuffer, &length);
        if (NT_SUCCESS(status)) {
            ProcessReadBuffer(fmDeviceData,
                              systemBuffer,
                              (ULONG) length,
                              &bytesToMove);
        }
        WdfRequestCompleteWithInformation(readRequest, status, bytesToMove);
    }

    ProcessConnectionStateChange( fmDeviceData);

    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, Length);
}

VOID
ProcessWriteBytes(
    PFM_DEVICE_DATA   FmDeviceData,
    PUCHAR            Characters,
    ULONG             Length
    )
/*++
Routine Description:

    This function is called when the framework receives IRP_MJ_WRITE
    requests from the system. The write event handler(FmEvtIoWrite) calls ProcessWriteBytes.
    It parses the Characters passed in and looks for the  for sequences "AT" -ok  ,
    "ATA" --CONNECT, ATD<number> -- CONNECT and sets the state of the device appropriately.
    These bytes are placed in the read Buffer to be processed later since this device
    works in a loopback fashion.


Arguments:

    FmDeviceData - Handle to the framework queue object that is associated
            with the I/O request.
    Characters - Pointer to the write IRP's system buffer.

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

   VOID

--*/

{

    UCHAR               currentCharacter;

    while (Length != 0) {

        currentCharacter=*Characters++;
        Length--;

        if(currentCharacter == '\0')
        {
            continue;
        }

        PutCharInReadBuffer( FmDeviceData, currentCharacter);

        switch (FmDeviceData->CommandMatchState) {

            case COMMAND_MATCH_STATE_IDLE:

                if ((currentCharacter == 'a') || (currentCharacter == 'A')) {
                    //  got an A
                    FmDeviceData->CommandMatchState=COMMAND_MATCH_STATE_GOT_A;

                    FmDeviceData->ConnectCommand=FALSE;

                    FmDeviceData->IgnoreNextChar=FALSE;

                }

            break;

            case COMMAND_MATCH_STATE_GOT_A:

                if ((currentCharacter == 't') || (currentCharacter == 'T')) {
                    //  got an T
                    FmDeviceData->CommandMatchState=COMMAND_MATCH_STATE_GOT_T;

                } else {

                    if (currentCharacter == '\r') {

                        FmDeviceData->CommandMatchState=COMMAND_MATCH_STATE_IDLE;
                    }
                }

            break;

            case COMMAND_MATCH_STATE_GOT_T:

                if (!FmDeviceData->IgnoreNextChar) {
                    //  the last char was not a special char
                    //  check for CONNECT command
                    if ((currentCharacter == 'A') || (currentCharacter == 'a')) {

                        FmDeviceData->ConnectCommand=TRUE;
                    }

                    if ((currentCharacter == 'D') || (currentCharacter == 'd')) {

                        FmDeviceData->ConnectCommand=TRUE;
                    }
                }

                FmDeviceData->IgnoreNextChar=TRUE;

                if (currentCharacter == '\r') {
                    //
                    //  got a CR, send a response to the command
                    //
                    FmDeviceData->CommandMatchState=COMMAND_MATCH_STATE_IDLE;

                    if (FmDeviceData->ConnectCommand) {
                        //
                        //  place <cr><lf>CONNECT<cr><lf>  in the buffer
                        //
                        PutCharInReadBuffer(FmDeviceData,'\r');
                        PutCharInReadBuffer(FmDeviceData,'\n');

                        PutCharInReadBuffer(FmDeviceData,'C');
                        PutCharInReadBuffer(FmDeviceData,'O');
                        PutCharInReadBuffer(FmDeviceData,'N');
                        PutCharInReadBuffer(FmDeviceData,'N');
                        PutCharInReadBuffer(FmDeviceData,'E');
                        PutCharInReadBuffer(FmDeviceData,'C');
                        PutCharInReadBuffer(FmDeviceData,'T');

                        PutCharInReadBuffer(FmDeviceData,'\r');
                        PutCharInReadBuffer(FmDeviceData,'\n');

                        //
                        //  connected now raise CD
                        //
                        FmDeviceData->CurrentlyConnected=TRUE;

                        FmDeviceData->ConnectionStateChanged=TRUE;

                    } else {

                        //  place <cr><lf>OK<cr><lf>  in the buffer

                        PutCharInReadBuffer(FmDeviceData,'\r');
                        PutCharInReadBuffer(FmDeviceData,'\n');
                        PutCharInReadBuffer(FmDeviceData,'O');
                        PutCharInReadBuffer(FmDeviceData,'K');
                        PutCharInReadBuffer(FmDeviceData,'\r');
                        PutCharInReadBuffer(FmDeviceData,'\n');
                    }
                }


            break;

            default:

            break;

        }
    }

    return;

}

VOID
PutCharInReadBuffer(
    PFM_DEVICE_DATA   FmDeviceData,
    UCHAR             Character
    )
/*++
Routine Description:

    This routine puts the charcter into the circular read buffer checking for overflows while doing it.

Arguments:

    FmDeviceData - Handle to the framework queue object that is associated
            with the I/O request.
    Characters - Handle to a framework request object.


Return Value:

   VOID

--*/

{

    if (FmDeviceData->BytesInReadBuffer < READ_BUFFER_SIZE) {

        //  room in buffer
        FmDeviceData->ReadBuffer[FmDeviceData->ReadBufferEnd]=Character;
        FmDeviceData->ReadBufferEnd++;
        FmDeviceData->ReadBufferEnd %= READ_BUFFER_SIZE;
        FmDeviceData->BytesInReadBuffer++;

    }

    return;

}

VOID
ProcessReadBuffer(
    IN  PFM_DEVICE_DATA FmDeviceData,
    IN  PUCHAR  SystemBuffer,
    IN  ULONG   Length,
    OUT PULONG  BytesToMove
    )
/*++
Routine Description:

     This event is called when the framework receives IRP_MJ_READ
    requests from the system. It is called by the read event handler.
    It copies data from the IRp's system buffer to the device read buffer.
    if the size of data from the Irp's system buffer is greater than the size of the
    read buffer the number of bytes remaining is passed back in  BytesToMove.


Arguments:

    FmDeviceData - Handle to the framework queue object that is associated
            with the I/O request.
    SystemBuffer -   The buffer passed in the IRP which contains the read data.

    Length -  Length of data in the systembuffer
    BytesToMove -  Remaining bytes not copied into the read buffer.
Return Value:

   VOID

--*/
{
    ULONG              firstHalf;
    ULONG              secondHalf;
    ULONG              bytesToMove;
    NTSTATUS           status;
    ULONG              safeBoundsCheck;

    //
    //  there is an IRP and there are characters waiting
    //


    bytesToMove = (Length < FmDeviceData->BytesInReadBuffer) ? Length
                                    : FmDeviceData->BytesInReadBuffer;

    status =   RtlULongAdd (FmDeviceData->ReadBufferBegin,
                            bytesToMove,
                            &safeBoundsCheck);

    if (!NT_SUCCESS(status)) {
        return;
    }

    if (safeBoundsCheck > READ_BUFFER_SIZE) {

        //
        //  the buffer is wrapped around, have move in two pieces
        //

        firstHalf = READ_BUFFER_SIZE - FmDeviceData->ReadBufferBegin;

        secondHalf= bytesToMove - firstHalf;

        RtlCopyMemory(
            SystemBuffer,
            &FmDeviceData->ReadBuffer[FmDeviceData->ReadBufferBegin],
            firstHalf);

        RtlCopyMemory(
            (SystemBuffer + firstHalf),
            &FmDeviceData->ReadBuffer[0],
            secondHalf);

    } else {

        //
        //  can do it all at once
        //

        RtlCopyMemory(
            SystemBuffer,
            &FmDeviceData->ReadBuffer[FmDeviceData->ReadBufferBegin],
            bytesToMove);
    }

    //
    //  fix up queue pointers
    //
    FmDeviceData->BytesInReadBuffer -= bytesToMove;

    FmDeviceData->ReadBufferBegin += bytesToMove;

    FmDeviceData->ReadBufferBegin %= READ_BUFFER_SIZE;

    *BytesToMove = bytesToMove;
}

