/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    Queue.c

Abstract:

    This file implements the I/O queue interface and performs
    the read/write/ioctl operations.

Environment:

    Windows Driver Framework

--*/


#include "internal.h"

NTSTATUS
QueueCreate(
    _In_  PDEVICE_CONTEXT   DeviceContext
    )
{
    NTSTATUS                status;
    WDFDEVICE               device = DeviceContext->Device;
    WDF_IO_QUEUE_CONFIG     queueConfig;
    WDF_OBJECT_ATTRIBUTES   queueAttributes;
    WDFQUEUE                queue;
    PQUEUE_CONTEXT          queueContext;

    //
    // Create the default queue
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
                            &queueConfig,
                            WdfIoQueueDispatchParallel);

    queueConfig.EvtIoRead           = EvtIoRead;
    queueConfig.EvtIoWrite          = EvtIoWrite;
    queueConfig.EvtIoDeviceControl  = EvtIoDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
                            &queueAttributes,
                            QUEUE_CONTEXT);

    status = WdfIoQueueCreate(
                            device,
                            &queueConfig,
                            &queueAttributes,
                            &queue);

    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfIoQueueCreate failed 0x%x", status);
        return status;
    }

    queueContext = GetQueueContext(queue);
    queueContext->Queue = queue;
    queueContext->DeviceContext = DeviceContext;

    //
    // Create a manual queue to hold pending read requests. By keeping
    // them in the queue, framework takes care of cancelling them if the app
    // exits
    //

    WDF_IO_QUEUE_CONFIG_INIT(
                            &queueConfig,
                            WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(
                            device,
                            &queueConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &queue);

    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfIoQueueCreate manual queue failed 0x%x", status);
        return status;
    }

    queueContext->ReadQueue = queue;

    //
    // Create another manual queue to hold pending IOCTL_SERIAL_WAIT_ON_MASK
    //

    WDF_IO_QUEUE_CONFIG_INIT(
                            &queueConfig,
                            WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(
                            device,
                            &queueConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &queue);

    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfIoQueueCreate manual queue failed 0x%x", status);
        return status;
    }

    queueContext->WaitMaskQueue = queue;

    RingBufferInitialize(&queueContext->RingBuffer,
                            queueContext->Buffer,
                            sizeof(queueContext->Buffer));

    return status;
}


NTSTATUS
RequestCopyFromBuffer(
    _In_  WDFREQUEST        Request,
    _In_  PVOID             SourceBuffer,
    _In_  size_t            NumBytesToCopyFrom
    )
{
    NTSTATUS                status;
    WDFMEMORY               memory;

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfRequestRetrieveOutputMemory failed 0x%x", status);
        return status;
    }

    status = WdfMemoryCopyFromBuffer(memory, 0,
                            SourceBuffer, NumBytesToCopyFrom);
    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfMemoryCopyFromBuffer failed 0x%x", status);
        return status;
    }

    WdfRequestSetInformation(Request, NumBytesToCopyFrom);
    return status;
}


NTSTATUS
RequestCopyToBuffer(
    _In_  WDFREQUEST        Request,
    _In_  PVOID             DestinationBuffer,
    _In_  size_t            NumBytesToCopyTo
    )
{
    NTSTATUS                status;
    WDFMEMORY               memory;

    status = WdfRequestRetrieveInputMemory(Request, &memory);
    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfRequestRetrieveInputMemory failed 0x%x", status);
        return status;
    }

    status = WdfMemoryCopyToBuffer(memory, 0,
                            DestinationBuffer, NumBytesToCopyTo);
    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfMemoryCopyToBuffer failed 0x%x", status);
        return status;
    }

    WdfRequestSetInformation(Request, NumBytesToCopyTo);
    return status;
}


VOID
EvtIoDeviceControl(
    _In_  WDFQUEUE          Queue,
    _In_  WDFREQUEST        Request,
    _In_  size_t            OutputBufferLength,
    _In_  size_t            InputBufferLength,
    _In_  ULONG             IoControlCode
    )
{
    NTSTATUS                status;
    PQUEUE_CONTEXT          queueContext = GetQueueContext(Queue);
    PDEVICE_CONTEXT         deviceContext = queueContext->DeviceContext;
    UNREFERENCED_PARAMETER  (OutputBufferLength);
    UNREFERENCED_PARAMETER  (InputBufferLength);

    Trace(TRACE_LEVEL_INFO,
        "EvtIoDeviceControl 0x%x", IoControlCode);

    switch (IoControlCode)
    {

    case IOCTL_SERIAL_SET_BAUD_RATE:
    {
        //
        // This is a driver for a virtual serial port. Since there is no
        // actual hardware, we just store the baud rate and don't do
        // anything with it.
        //
        SERIAL_BAUD_RATE baudRateBuffer = {0};

        status = RequestCopyToBuffer(Request,
                            &baudRateBuffer,
                            sizeof(baudRateBuffer));

        if( NT_SUCCESS(status) ) {
            SetBaudRate(deviceContext, baudRateBuffer.BaudRate);
        };
        break;
    }

    case IOCTL_SERIAL_GET_BAUD_RATE:
    {
        SERIAL_BAUD_RATE baudRateBuffer = {0};

        baudRateBuffer.BaudRate = GetBaudRate(deviceContext);

        status = RequestCopyFromBuffer(Request,
                            &baudRateBuffer,
                            sizeof(baudRateBuffer));
        break;
    }

    case IOCTL_SERIAL_SET_MODEM_CONTROL:
    {
        //
        // This is a driver for a virtual serial port. Since there is no
        // actual hardware, we just store the modem control register
        // configuration and don't do anything with it.
        //
        ULONG *modemControlRegister = GetModemControlRegisterPtr(deviceContext);

        ASSERT(modemControlRegister);

        status = RequestCopyToBuffer(Request,
                            modemControlRegister,
                            sizeof(ULONG));
        break;
    }

    case IOCTL_SERIAL_GET_MODEM_CONTROL:
    {
        ULONG *modemControlRegister = GetModemControlRegisterPtr(deviceContext);

        ASSERT(modemControlRegister);

        status = RequestCopyFromBuffer(Request,
                            modemControlRegister,
                            sizeof(ULONG));
        break;
    }

    case IOCTL_SERIAL_SET_FIFO_CONTROL:
    {
        //
        // This is a driver for a virtual serial port. Since there is no
        // actual hardware, we just store the FIFO control register
        // configuration and don't do anything with it.
        //
        ULONG *fifoControlRegister = GetFifoControlRegisterPtr(deviceContext);

        ASSERT(fifoControlRegister);

        status = RequestCopyToBuffer(Request,
                            fifoControlRegister,
                            sizeof(ULONG));
        break;
    }

    case IOCTL_SERIAL_GET_LINE_CONTROL:
    {
        status = QueueProcessGetLineControl(
                            queueContext,
                            Request);
        break;
    }


    case IOCTL_SERIAL_SET_LINE_CONTROL:
    {
        status = QueueProcessSetLineControl(
                            queueContext,
                            Request);
        break;
    }

    case IOCTL_SERIAL_GET_TIMEOUTS:
    {
        SERIAL_TIMEOUTS timeoutValues = {0};

        status = RequestCopyFromBuffer(Request,
                            (void*) &timeoutValues,
                            sizeof(timeoutValues));
        break;
    }

    case IOCTL_SERIAL_SET_TIMEOUTS:
    {
        SERIAL_TIMEOUTS timeoutValues = {0};

        status = RequestCopyToBuffer(Request,
                            (void*) &timeoutValues,
                            sizeof(timeoutValues));

        if( NT_SUCCESS(status) )
        {
            if ((timeoutValues.ReadIntervalTimeout        == MAXULONG) &&
                (timeoutValues.ReadTotalTimeoutMultiplier == MAXULONG) &&
                (timeoutValues.ReadTotalTimeoutConstant   == MAXULONG))
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }

        if( NT_SUCCESS(status) ) {
            SetTimeouts(deviceContext, timeoutValues);
        }

        break;
    }

    case IOCTL_SERIAL_WAIT_ON_MASK:
    {
        //
        // NOTE: A wait-on-mask request should not be completed until either:
        //  1) A wait event occurs; or
        //  2) A set-wait-mask request is received
        //
        // This is a driver for a virtual serial port. Since there is no
        // actual hardware, we complete the request with some failure code.
        //
        WDFREQUEST savedRequest;

        status = WdfIoQueueRetrieveNextRequest(
                            queueContext->WaitMaskQueue,
                            &savedRequest);

        if (NT_SUCCESS(status)) {
            WdfRequestComplete(savedRequest,
                            STATUS_UNSUCCESSFUL);
        }

        //
        // Keep the request in a manual queue and the framework will take
        // care of cancelling them when the app exits
        //
        status = WdfRequestForwardToIoQueue(
                            Request,
                            queueContext->WaitMaskQueue);

        if( !NT_SUCCESS(status) ) {
            Trace(TRACE_LEVEL_ERROR,
                "Error: WdfRequestForwardToIoQueue failed 0x%x", status);
            WdfRequestComplete(Request, status);
        }

        //
        // Instead of "break", use "return" to prevent the current request
        // from being completed.
        //
        return;
    }

    case IOCTL_SERIAL_SET_WAIT_MASK:
    {
        //
        // NOTE: If a wait-on-mask request is already pending when set-wait-mask
        // request is processed, the pending wait-on-event request is completed
        // with STATUS_SUCCESS and the output wait event mask is set to zero.
        //
        WDFREQUEST savedRequest;

        status = WdfIoQueueRetrieveNextRequest(
                            queueContext->WaitMaskQueue,
                            &savedRequest);

        if (NT_SUCCESS(status)) {

            ULONG eventMask = 0;
            status = RequestCopyFromBuffer(
                            savedRequest,
                            &eventMask,
                            sizeof(eventMask));

            WdfRequestComplete(savedRequest, status);
        }

        //
        // NOTE: The application expects STATUS_SUCCESS for these IOCTLs.
        //
        status = STATUS_SUCCESS;
        break;
    }

    case IOCTL_SERIAL_SET_QUEUE_SIZE:
    case IOCTL_SERIAL_SET_DTR:
    case IOCTL_SERIAL_SET_RTS:
    case IOCTL_SERIAL_CLR_RTS:
    case IOCTL_SERIAL_SET_XON:
    case IOCTL_SERIAL_SET_XOFF:
    case IOCTL_SERIAL_SET_CHARS:
    case IOCTL_SERIAL_GET_CHARS:
    case IOCTL_SERIAL_GET_HANDFLOW:
    case IOCTL_SERIAL_SET_HANDFLOW:
    case IOCTL_SERIAL_RESET_DEVICE:
        //
        // NOTE: The application expects STATUS_SUCCESS for these IOCTLs.
        //
        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    //
    // complete the request
    //
    WdfRequestComplete(Request, status);
}


VOID
EvtIoWrite(
    _In_  WDFQUEUE          Queue,
    _In_  WDFREQUEST        Request,
    _In_  size_t            Length
    )
{
    NTSTATUS                status;
    PQUEUE_CONTEXT          queueContext = GetQueueContext(Queue);
    WDFMEMORY               memory;
    WDFREQUEST              savedRequest;
    size_t                  availableData = 0;

    Trace(TRACE_LEVEL_INFO,
            "EvtIoWrite 0x%p", Request);

    status = WdfRequestRetrieveInputMemory(Request, &memory);
    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfRequestRetrieveInputMemory failed 0x%x", status);
        return;
    }

    //
    // Process input
    //
    status = QueueProcessWriteBytes(
                            queueContext,
                            (PUCHAR)WdfMemoryGetBuffer(memory, NULL),
                            Length);
    if( !NT_SUCCESS(status) ) {
        return;
    }

    WdfRequestCompleteWithInformation(Request, status, Length);

    //
    // Get the amount of data available in the ring buffer
    //
    RingBufferGetAvailableData(
                            &queueContext->RingBuffer,
                            &availableData);

    if (availableData == 0) {
        return;
    }

    //
    // Continue with the next request, if there is one pending
    //
    for ( ; ; ) {

        status = WdfIoQueueRetrieveNextRequest(
                            queueContext->ReadQueue,
                            &savedRequest);

        if (!NT_SUCCESS(status)) {
            break;
        }

        status = WdfRequestForwardToIoQueue(
                            savedRequest,
                            Queue);

        if( !NT_SUCCESS(status) ) {
            Trace(TRACE_LEVEL_ERROR,
                "Error: WdfRequestForwardToIoQueue failed 0x%x", status);
            WdfRequestComplete(savedRequest, status);
        }
    }
}


VOID
EvtIoRead(
    _In_  WDFQUEUE          Queue,
    _In_  WDFREQUEST        Request,
    _In_  size_t            Length
    )
{
    NTSTATUS                status;
    PQUEUE_CONTEXT          queueContext = GetQueueContext(Queue);
    WDFMEMORY               memory;
    size_t                  bytesCopied = 0;

    Trace(TRACE_LEVEL_INFO,
            "EvtIoRead 0x%p", Request);

    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if( !NT_SUCCESS(status) ) {
        Trace(TRACE_LEVEL_ERROR,
            "Error: WdfRequestRetrieveOutputMemory failed 0x%x", status);
        WdfRequestComplete(Request, status);
        return;
    }

    status = RingBufferRead(&queueContext->RingBuffer,
                            (BYTE*)WdfMemoryGetBuffer(memory, NULL),
                            Length,
                            &bytesCopied);
    if( !NT_SUCCESS(status) ) {
        WdfRequestComplete(Request, status);
        return;
    }

    if (bytesCopied > 0) {
        //
        // Data was read from buffer succesfully
        //
        WdfRequestCompleteWithInformation(Request, status, bytesCopied);
        return;
    }
    else {
        //
        // No data to read. Queue the request for later processing.
        //
        status = WdfRequestForwardToIoQueue(Request,
                            queueContext->ReadQueue);
        if( !NT_SUCCESS(status) ) {
            Trace(TRACE_LEVEL_ERROR,
                "Error: WdfRequestForwardToIoQueue failed 0x%x", status);
            WdfRequestComplete(Request, status);
        }
    }
}


NTSTATUS
QueueProcessWriteBytes(
    _In_  PQUEUE_CONTEXT    QueueContext,
    _In_reads_bytes_(Length)
          PUCHAR            Characters,
    _In_  size_t            Length
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

    Characters - Pointer to the write IRP's system buffer.

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.
--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    UCHAR                   currentCharacter;
    UCHAR                   connectString[]  = "\r\nCONNECT\r\n";
    UCHAR                   connectStringCch = ARRAY_SIZE(connectString) - 1;
    UCHAR                   okString[]       = "\r\nOK\r\n";
    UCHAR                   okStringCch      = ARRAY_SIZE(okString) - 1;

    while (Length != 0) {

        currentCharacter = *(Characters++);
        Length--;

        if(currentCharacter == '\0') {
            continue;
        }

        status = RingBufferWrite(&QueueContext->RingBuffer,
                            &currentCharacter,
                            sizeof(currentCharacter));
        if( !NT_SUCCESS(status) ) {
            return status;
        }

        switch (QueueContext->CommandMatchState) {

        case COMMAND_MATCH_STATE_IDLE:

            if ((currentCharacter == 'a') || (currentCharacter == 'A')) {
                //
                //  got an A
                //
                QueueContext->CommandMatchState = COMMAND_MATCH_STATE_GOT_A;
                QueueContext->ConnectCommand = FALSE;
                QueueContext->IgnoreNextChar = FALSE;
            }
            break;

        case COMMAND_MATCH_STATE_GOT_A:

            if ((currentCharacter == 't') || (currentCharacter == 'T')) {
                //
                //  got a T
                //
                QueueContext->CommandMatchState = COMMAND_MATCH_STATE_GOT_T;
            }
            else {
                QueueContext->CommandMatchState = COMMAND_MATCH_STATE_IDLE;
            }

            break;

        case COMMAND_MATCH_STATE_GOT_T:

            if (! QueueContext->IgnoreNextChar) {
                //
                //  the last char was not a special char
                //  check for CONNECT command
                //
                if ((currentCharacter == 'A') || (currentCharacter == 'a')) {
                    QueueContext->ConnectCommand = TRUE;
                }

                if ((currentCharacter == 'D') || (currentCharacter == 'd')) {
                    QueueContext->ConnectCommand = TRUE;
                }
            }

            QueueContext->IgnoreNextChar = TRUE;

            if (currentCharacter == '\r') {
                //
                //  got a CR, send a response to the command
                //
                QueueContext->CommandMatchState = COMMAND_MATCH_STATE_IDLE;

                if (QueueContext->ConnectCommand) {
                    //
                    //  place <cr><lf>CONNECT<cr><lf>  in the buffer
                    //
                    status = RingBufferWrite(&QueueContext->RingBuffer,
                            connectString,
                            connectStringCch);
                    if( !NT_SUCCESS(status) ) {
                        return status;
                    }
                    //
                    //  connected now raise CD
                    //
                    QueueContext->CurrentlyConnected = TRUE;
                    QueueContext->ConnectionStateChanged = TRUE;
                }
                else {
                    //
                    //  place <cr><lf>OK<cr><lf>  in the buffer
                    //
                    status = RingBufferWrite(&QueueContext->RingBuffer,
                            okString,
                            okStringCch);
                    if( !NT_SUCCESS(status) ) {
                        return status;
                    }
                }
            }
            break;

        default:
            break;
        }
    }
    return status;
}


NTSTATUS
QueueProcessGetLineControl(
    _In_  PQUEUE_CONTEXT    QueueContext,
    _In_  WDFREQUEST        Request
    )
{
    NTSTATUS                status;
    PDEVICE_CONTEXT         deviceContext;
    SERIAL_LINE_CONTROL     lineControl = {0};
    ULONG                   lineControlSnapshot;
    ULONG                   *lineControlRegister;

    deviceContext = QueueContext->DeviceContext;
    lineControlRegister = GetLineControlRegisterPtr(deviceContext);

    ASSERT(lineControlRegister);

    //
    // Take a snapshot of the line control register variable
    //
    lineControlSnapshot = *lineControlRegister;

    //
    // Decode the word length
    //
    if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_5_DATA)
    {
        lineControl.WordLength = 5;
    }
    else if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_6_DATA)
    {
        lineControl.WordLength = 6;
    }
    else if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_7_DATA)
    {
        lineControl.WordLength = 7;
    }
    else if ((lineControlSnapshot & SERIAL_DATA_MASK) == SERIAL_8_DATA)
    {
        lineControl.WordLength = 8;
    }

    //
    // Decode the parity
    //
    if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_NONE_PARITY)
    {
        lineControl.Parity = NO_PARITY;
    }
    else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_ODD_PARITY)
    {
        lineControl.Parity = ODD_PARITY;
    }
    else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_EVEN_PARITY)
    {
        lineControl.Parity = EVEN_PARITY;
    }
    else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_MARK_PARITY)
    {
        lineControl.Parity = MARK_PARITY;
    }
    else if ((lineControlSnapshot & SERIAL_PARITY_MASK) == SERIAL_SPACE_PARITY)
    {
        lineControl.Parity = SPACE_PARITY;
    }

    //
    // Decode the length of the stop bit
    //
    if (lineControlSnapshot & SERIAL_2_STOP)
    {
        if (lineControl.WordLength == 5)
        {
            lineControl.StopBits = STOP_BITS_1_5;
        }
        else
        {
            lineControl.StopBits = STOP_BITS_2;
        }
    }
    else
    {
        lineControl.StopBits = STOP_BIT_1;
    }

    //
    // Copy the information that was decoded to the caller's buffer
    //
    status = RequestCopyFromBuffer(Request,
                        (void*) &lineControl,
                        sizeof(lineControl));
    return status;
}


NTSTATUS
QueueProcessSetLineControl(
    _In_  PQUEUE_CONTEXT    QueueContext,
    _In_  WDFREQUEST        Request
    )
{
    NTSTATUS                status;
    PDEVICE_CONTEXT         deviceContext;
    SERIAL_LINE_CONTROL     lineControl = {0};
    ULONG                   *lineControlRegister;
    UCHAR                   lineControlData = 0;
    UCHAR                   lineControlStop = 0;
    UCHAR                   lineControlParity = 0;
    ULONG                   lineControlSnapshot;
    ULONG                   lineControlNew;
    ULONG                   lineControlPrevious;
    ULONG                   i;

    deviceContext = QueueContext->DeviceContext;
    lineControlRegister = GetLineControlRegisterPtr(deviceContext);

    ASSERT(lineControlRegister);

    //
    // This is a driver for a virtual serial port. Since there is no
    // actual hardware, we just store the line control register
    // configuration and don't do anything with it.
    //
    status = RequestCopyToBuffer(Request,
                        (void*) &lineControl,
                        sizeof(lineControl));

    //
    // Bits 0 and 1 of the line control register
    //
    if( NT_SUCCESS(status) )
    {
        switch (lineControl.WordLength)
        {
        case 5:
            lineControlData = SERIAL_5_DATA;
            SetValidDataMask(deviceContext, 0x1f);
            break;

        case 6:
            lineControlData = SERIAL_6_DATA;
            SetValidDataMask(deviceContext, 0x3f);
            break;

        case 7:
            lineControlData = SERIAL_7_DATA;
            SetValidDataMask(deviceContext, 0x7f);
            break;

        case 8:
            lineControlData = SERIAL_8_DATA;
            SetValidDataMask(deviceContext, 0xff);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    //
    // Bit 2 of the line control register
    //
    if( NT_SUCCESS(status) )
    {
        switch (lineControl.StopBits)
        {
        case STOP_BIT_1:
            lineControlStop = SERIAL_1_STOP;
            break;

        case STOP_BITS_1_5:
            if (lineControlData != SERIAL_5_DATA)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            lineControlStop = SERIAL_1_5_STOP;
            break;

        case STOP_BITS_2:
            if (lineControlData == SERIAL_5_DATA)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            lineControlStop = SERIAL_2_STOP;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    //
    // Bits 3, 4 and 5 of the line control register
    //
    if( NT_SUCCESS(status) )
    {
        switch (lineControl.Parity)
        {
        case NO_PARITY:
            lineControlParity = SERIAL_NONE_PARITY;
            break;

        case EVEN_PARITY:
            lineControlParity = SERIAL_EVEN_PARITY;
            break;

        case ODD_PARITY:
            lineControlParity = SERIAL_ODD_PARITY;
            break;

        case SPACE_PARITY:
            lineControlParity = SERIAL_SPACE_PARITY;
            break;

        case MARK_PARITY:
            lineControlParity = SERIAL_MARK_PARITY;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    //
    // Update our line control register variable atomically
    //
    i=0;
    do {
        i++;
        if ((i & 0xf) == 0) {
            //
            // We've been spinning in a loop for a while trying to
            // update the line control register variable atomically.
            // Yield the CPU for other threads for a while.
            //
#ifdef _KERNEL_MODE
            LARGE_INTEGER   interval;
            interval.QuadPart = 0;
            KeDelayExecutionThread(UserMode, FALSE, &interval);
#else
            SwitchToThread();
#endif
        }

        lineControlSnapshot = *lineControlRegister;

        lineControlNew = (lineControlSnapshot & SERIAL_LCR_BREAK) |
                        (lineControlData | lineControlParity | lineControlStop);

        lineControlPrevious = InterlockedCompareExchange(
                      (LONG *) lineControlRegister,
                       lineControlNew,
                       lineControlSnapshot);

    } while (lineControlPrevious != lineControlSnapshot);

    return status;
}
