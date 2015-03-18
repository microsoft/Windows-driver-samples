/*++

Copyright (c) Microsoft Corporation, All Rights Reserved

Module Name:

    queue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the read/write/ioctl operations.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/


#include "internal.h"

//
// IUnknown implementation
//

//
// Queue destructor.
// Free up the buffer, wait for thread to terminate and
// delete critical section.
//


CMyQueue::~CMyQueue(
    VOID
    )
/*++

Routine Description:


    IUnknown implementation of Release

Arguments:


Return Value:

--*/
{
        WUDF_TEST_DRIVER_ASSERT(m_Device);

        m_Device->Release();
}


//
// Initialize
HRESULT
CMyQueue::CreateInstance(
    _In_  CMyDevice *pDevice,
    _In_ IWDFDevice *FxDevice,
    _Out_ PCMyQueue *Queue
    )
/*++

Routine Description:


    CreateInstance creates an instance of the queue object.

Arguments:

    ppUkwn - OUT parameter is an IUnknown interface to the queue object

Return Value:

    HRESULT indicating success or failure

--*/
{
    CMyQueue *pMyQueue = new CMyQueue(pDevice);
    HRESULT hr;

    if (pMyQueue == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pMyQueue->Initialize(FxDevice);

    if (SUCCEEDED(hr))
    {
        *Queue = pMyQueue;
    }
    else
    {
        pMyQueue->Release();
    }
    return hr;
}

HRESULT
CMyQueue::Initialize(
    _In_ IWDFDevice *FxDevice
    )
{
    IWDFIoQueue *fxQueue;
    IUnknown *unknown = NULL;
    HRESULT hr;

    //
    // Initialize ring buffer
    //

    hr = m_RingBuffer.Initialize(DATA_BUFFER_SIZE);

    if (FAILED(hr))
    {
        goto Exit;
    }

    unknown = QueryIUnknown();

    //
    // Create the default queue
    //

    {
        hr = FxDevice->CreateIoQueue(unknown,
                                     TRUE,
                                     WdfIoQueueDispatchParallel,
                                     TRUE,
                                     FALSE,
                                     &fxQueue);
    }

    if (FAILED(hr))
    {
        goto Exit;
    }

    m_FxQueue = fxQueue;

    fxQueue->Release();

    //
    // Create a manual queue to hold pending read requests. By keeping
    // them in the queue, framework takes care of cancelling them if the app
    // exits
    //

    {
        hr = FxDevice->CreateIoQueue(NULL,
                                     FALSE,
                                     WdfIoQueueDispatchManual,
                                     TRUE,
                                     FALSE,
                                     &fxQueue);
    }

    if (FAILED(hr))
    {
        goto Exit;
    }

    m_FxReadQueue = fxQueue;

    fxQueue->Release();

    //
    // Create a manual queue to hold pending ioctl wait-on-mask requests.
    //

    {
        hr = FxDevice->CreateIoQueue(NULL,
                                     FALSE,
                                     WdfIoQueueDispatchManual,
                                     TRUE,
                                     FALSE,
                                     &fxQueue);
    }

    if (FAILED(hr))
    {
        goto Exit;
    }

    m_FxWaitMaskQueue = fxQueue;

    fxQueue->Release();

Exit:
    SAFE_RELEASE(unknown);
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CMyQueue::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID *Object
    )
/*++

Routine Description:


    Query Interface

Arguments:

    Follows COM specifications

Return Value:

    HRESULT indicating success or failure

--*/
{
    HRESULT hr;


    if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackWrite))) {
        *Object = QueryIQueueCallbackWrite();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackRead))) {
        *Object = QueryIQueueCallbackRead();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackDeviceIoControl))) {
        *Object = QueryIQueueCallbackDeviceIoControl();
        hr = S_OK;
    } else {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnDeviceIoControl(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ ULONG ControlCode,
    _In_ SIZE_T InputBufferSizeInBytes,
    _In_ SIZE_T OutputBufferSizeInBytes
    )
/*++

Routine Description:


    DeviceIoControl dispatch routine

Arguments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    ControlCode - IO Control Code
    InputBufferSizeInBytes - Length of input buffer
    OutputBufferSizeInBytes - Length of output buffer

    Always succeeds DeviceIoIoctl
Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(OutputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(InputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(pWdfQueue);

    HRESULT hr = S_OK;
    SIZE_T reqCompletionInfo = 0;
    IWDFMemory *inputMemory = NULL;
    IWDFMemory *outputMemory = NULL;
    UINT i;

    WUDF_TEST_DRIVER_ASSERT(pWdfRequest);
    WUDF_TEST_DRIVER_ASSERT(m_Device);

    switch (ControlCode)
    {
        case IOCTL_SERIAL_SET_BAUD_RATE:
        {
            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the baud rate and don't do
            // anything with it.
            //
            SERIAL_BAUD_RATE baudRateBuffer = {0};

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = inputMemory->CopyToBuffer(0,
                                               (void*) &baudRateBuffer,
                                               sizeof(SERIAL_BAUD_RATE));
            }

            if (SUCCEEDED(hr))
            {
                m_Device->SetBaudRate(baudRateBuffer.BaudRate);
            }

            break;
        }
        case IOCTL_SERIAL_GET_BAUD_RATE:
        {
            SERIAL_BAUD_RATE baudRateBuffer = {0};

            baudRateBuffer.BaudRate = m_Device->GetBaudRate();

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &baudRateBuffer,
                                                  sizeof(SERIAL_BAUD_RATE));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(SERIAL_BAUD_RATE);
            }

            break;
        }
        case IOCTL_SERIAL_SET_MODEM_CONTROL:
        {
            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the modem control register
            // configuration and don't do anything with it.
            //
            ULONG *pModemControlRegister = NULL;

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pModemControlRegister = m_Device->GetModemControlRegisterPtr();
                WUDF_TEST_DRIVER_ASSERT(pModemControlRegister);

                hr = inputMemory->CopyToBuffer(0,
                                               (void*) pModemControlRegister,
                                               sizeof(ULONG));
            }

            break;
        }
        case IOCTL_SERIAL_GET_MODEM_CONTROL:
        {
            ULONG *pModemControlRegister = NULL;

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pModemControlRegister = m_Device->GetModemControlRegisterPtr();
                WUDF_TEST_DRIVER_ASSERT(pModemControlRegister);

                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) pModemControlRegister,
                                                  sizeof(ULONG));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(ULONG);
            }

            break;
        }
        case IOCTL_SERIAL_SET_FIFO_CONTROL:
        {
            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the FIFO control register
            // configuration and don't do anything with it.
            //
            ULONG *pFifoControlRegister = NULL;

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                pFifoControlRegister = m_Device->GetFifoControlRegisterPtr();

                hr = inputMemory->CopyToBuffer(0,
                                               (void*) pFifoControlRegister,
                                               sizeof(ULONG));
            }

            break;
        }
        case IOCTL_SERIAL_GET_LINE_CONTROL:
        {
            ULONG *pLineControlRegister = NULL;
            SERIAL_LINE_CONTROL lineControl = {0};
            ULONG lineControlSnapshot;

            pLineControlRegister = m_Device->GetLineControlRegisterPtr();
            WUDF_TEST_DRIVER_ASSERT(pLineControlRegister);

            //
            // Take a snapshot of the line control register variable
            //
            lineControlSnapshot = *pLineControlRegister;

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
            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &lineControl,
                                                  sizeof(SERIAL_LINE_CONTROL));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(SERIAL_LINE_CONTROL);
            }

            break;
        }
        case IOCTL_SERIAL_SET_LINE_CONTROL:
        {
            ULONG *pLineControlRegister = NULL;
            SERIAL_LINE_CONTROL lineControl = {0};
            UCHAR lineControlData = 0;
            UCHAR lineControlStop = 0;
            UCHAR lineControlParity = 0;
            ULONG lineControlSnapshot;
            ULONG lineControlNew;
            ULONG lineControlPrevious;

            pLineControlRegister = m_Device->GetLineControlRegisterPtr();
            WUDF_TEST_DRIVER_ASSERT(pLineControlRegister);

            //
            // This is a driver for a virtual serial port. Since there is no
            // actual hardware, we just store the line control register
            // configuration and don't do anything with it.
            //
            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = inputMemory->CopyToBuffer(0,
                                               (void*) &lineControl,
                                               sizeof(SERIAL_LINE_CONTROL));
            }

            //
            // Bits 0 and 1 of the line control register
            //
            if (SUCCEEDED(hr))
            {
                switch (lineControl.WordLength)
                {
                    case 5:
                        lineControlData = SERIAL_5_DATA;
                        m_Device->SetValidDataMask(0x1f);
                        break;

                    case 6:
                        lineControlData = SERIAL_6_DATA;
                        m_Device->SetValidDataMask(0x3f);
                        break;

                    case 7:
                        lineControlData = SERIAL_7_DATA;
                        m_Device->SetValidDataMask(0x7f);
                        break;

                    case 8:
                        lineControlData = SERIAL_8_DATA;
                        m_Device->SetValidDataMask(0xff);
                        break;

                    default:
                        hr = E_INVALIDARG;
                }
            }

            //
            // Bit 2 of the line control register
            //
            if (SUCCEEDED(hr))
            {
                switch (lineControl.StopBits)
                {
                    case STOP_BIT_1:
                        lineControlStop = SERIAL_1_STOP;
                        break;

                    case STOP_BITS_1_5:
                        if (lineControlData != SERIAL_5_DATA)
                        {
                            hr = E_INVALIDARG;
                            break;
                        }
                        lineControlStop = SERIAL_1_5_STOP;
                        break;

                    case STOP_BITS_2:
                        if (lineControlData == SERIAL_5_DATA)
                        {
                            hr = E_INVALIDARG;
                            break;
                        }
                        lineControlStop = SERIAL_2_STOP;
                        break;

                    default:
                        hr = E_INVALIDARG;
                }
            }

            //
            // Bits 3, 4 and 5 of the line control register
            //
            if (SUCCEEDED(hr))
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
                        hr = E_INVALIDARG;
                }
            }

            //
            // Update our line control register variable atomically
            //
            i=0;
            do
            {
                i++;
                if ((i & 0xf) == 0)
                {
                    //
                    // We've been spinning in a loop for a while trying to
                    // update the line control register variable atomically.
                    // Yield the CPU for other threads for a while.
                    //
                    SwitchToThread();
                }

                lineControlSnapshot = *pLineControlRegister;

                lineControlNew = (lineControlSnapshot & SERIAL_LCR_BREAK) |
                                    (lineControlData |
                                     lineControlParity |
                                     lineControlStop);

                lineControlPrevious = InterlockedCompareExchange((LONG *) pLineControlRegister,
                                                                 lineControlNew,
                                                                 lineControlSnapshot);

            } while (lineControlPrevious != lineControlSnapshot);

            break;
        }
        case IOCTL_SERIAL_GET_TIMEOUTS:
        {
            SERIAL_TIMEOUTS timeoutValues = {0};

            pWdfRequest->GetOutputMemory(&outputMemory);
            if (NULL == outputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                m_Device->GetTimeouts(&timeoutValues);

                hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &timeoutValues,
                                                  sizeof(timeoutValues));
            }

            if (SUCCEEDED(hr))
            {
                reqCompletionInfo = sizeof(SERIAL_TIMEOUTS);
            }

            break;
        }
        case IOCTL_SERIAL_SET_TIMEOUTS:
        {
            SERIAL_TIMEOUTS timeoutValues = {0};

            pWdfRequest->GetInputMemory(&inputMemory);
            if (NULL == inputMemory)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (SUCCEEDED(hr))
            {
                hr = inputMemory->CopyToBuffer(0,
                                               (void*) &timeoutValues,
                                               sizeof(timeoutValues));
            }

            if (SUCCEEDED(hr))
            {
                if ((timeoutValues.ReadIntervalTimeout == MAXULONG) &&
                    (timeoutValues.ReadTotalTimeoutMultiplier == MAXULONG) &&
                    (timeoutValues.ReadTotalTimeoutConstant == MAXULONG))
                {
                    hr = E_INVALIDARG;
                }
            }

            if (SUCCEEDED(hr))
            {
                m_Device->SetTimeouts(timeoutValues);
            }

            break;
        }
        case IOCTL_SERIAL_WAIT_ON_MASK:
        {
            //
            // NOTE: the contract is that this ioctl should be marked pending
            // and not to be completed until some wait event happens. Therefore
            // it is incorrect for the driver to complete the ioctl right away,
            // no matter whether success or failure is returned. In either case
            // most likely the app will send down another iocl of wait-on-mask
            // in a tight loop, and that gets completed again, and again.
            // The end result will be high CPU utilization in task manager.
            //
            // The correct way would be to mark the ioctl as pending, or as in
            // WDF world, keep it in a manual queue. Since this is a driver for
            // a virtual serial port and there is no actual hardware, there will
            // be no wait event happening. The ioctl stays in the queue until
            // the calling app decides to no longer wait for it by means of
            // sending down a set-wait-mask request.
            //

            //
            // At most one pending wait-on-mask request is expected
            //
            IWDFIoRequest *pSavedRequest;
            hr = m_FxWaitMaskQueue->RetrieveNextRequest(&pSavedRequest);
            if (SUCCEEDED(hr))
            {
                pSavedRequest->Complete(E_FAIL);

                //
                // RetrieveNextRequest from a manual queue increments the reference
                // counter by 1. We need to decrement it, otherwise the request will
                // not be released and there will be an object leak.
                //
                SAFE_RELEASE(pSavedRequest);
            }

            //
            // Keep the request in a manual queue and the framework will take
            // care of cancelling them when the app exits.
            //
            pWdfRequest->ForwardToIoQueue(m_FxWaitMaskQueue);

            //
            // Instead of "break" out of the switch statement and complete the
            // request at the end of this function, use "return" directly.
            //
            return;
        }
        case IOCTL_SERIAL_SET_WAIT_MASK:
        {
            //
            // NOTE: the contract says a set-wait-mask will cause any pending
            // wait-on-mask to complete with STATUS_SUCCESS and the output wait
            // event mask is set to zero. This is also the way for app to break
            // out of the loop of sending down wait-on-mask
            //

            IWDFIoRequest *pSavedRequest;
            hr = m_FxWaitMaskQueue->RetrieveNextRequest(&pSavedRequest);
            if (SUCCEEDED(hr))
            {
                pSavedRequest->GetOutputMemory(&outputMemory);
                if (NULL == outputMemory)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }

                if (SUCCEEDED(hr))
                {
                    ULONG eventMask = 0;

                    hr = outputMemory->CopyFromBuffer(0,
                                                  (void*) &eventMask,
                                                  sizeof(eventMask));

                    pSavedRequest->CompleteWithInformation(hr, sizeof(eventMask));
                }
                else
                {
                    pSavedRequest->Complete(hr);
                }

                SAFE_RELEASE(pSavedRequest);

                //
                // outputMemory will be released at the end of the function
                //
            }

            //
            // NOTE: The application expects STATUS_SUCCESS for these IOCTLs.
            //
            hr = S_OK;

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
        {
            //
            // NOTE: The application expects STATUS_SUCCESS for these IOCTLs.
            //  so don't merge this with default.
            //
            break;
        }

        default:
        {
            hr = E_INVALIDARG;
        }
    }

    //
    // clean up
    //
    if (inputMemory)
    {
        inputMemory->Release();
    }

    if (outputMemory)
    {
        outputMemory->Release();
    }

    //
    // complete the request
    //
    pWdfRequest->CompleteWithInformation(hr, reqCompletionInfo);

    return;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnWrite(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T BytesToWrite
    )
/*++

Routine Description:


    Write dispatch routine
    IQueueCallbackWrite

Arguments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    BytesToWrite - Length of bytes in the write buffer

    Allocate and copy data to local buffer
Return Value:

    VOID

--*/
{
    IWDFMemory* pRequestMemory = NULL;
    IWDFIoRequest* pSavedRequest = NULL;
    SIZE_T availableData = 0;
    SIZE_T savedRequestBufferSize = 0;
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pWdfQueue);

    //
    // Get memory object
    //

    pWdfRequest->GetInputMemory(&pRequestMemory);

    //
    // Process input
    //

    ProcessWriteBytes((PUCHAR)pRequestMemory->GetDataBuffer(NULL), BytesToWrite);

    //
    // Release memory object and complete request
    //
    SAFE_RELEASE(pRequestMemory);
    pWdfRequest->CompleteWithInformation(hr, BytesToWrite);

    //
    // Get the amount of data available in the ring buffer
    //
    m_RingBuffer.GetAvailableData(&availableData);

    if (availableData > 0)
    {
        //
        // Continue with the next request, if there is one pending
        //
        hr = m_FxReadQueue->RetrieveNextRequest(&pSavedRequest);
        if ((pSavedRequest == NULL) || (FAILED(hr)))
        {
            goto Exit;
        }

        pSavedRequest->GetReadParameters(&savedRequestBufferSize, NULL, NULL);

        OnRead(m_FxQueue,
               pSavedRequest,
               savedRequestBufferSize);

        //
        // RetrieveNextRequest from a manual queue increments the reference
        // counter by 1. We need to decrement it, otherwise the request will
        // not be released and there will be an object leak.
        //
        SAFE_RELEASE(pSavedRequest);
    }

Exit:
    return;
}

VOID
STDMETHODCALLTYPE
CMyQueue::OnRead(
    _In_ IWDFIoQueue *pWdfQueue,
    _In_ IWDFIoRequest *pWdfRequest,
    _In_ SIZE_T SizeInBytes
    )
/*++

Routine Description:


    Read dispatch routine
    IQueueCallbackRead

Arguments:

    pWdfQueue - Framework Queue instance
    pWdfRequest - Framework Request  instance
    SizeInBytes - Length of bytes in the read buffer

    Copy available data into the read buffer
Return Value:

    VOID

--*/
{
    IWDFMemory* pRequestMemory = NULL;
    SIZE_T BytesCopied = 0;
    HRESULT     hr;

    UNREFERENCED_PARAMETER(pWdfQueue);

    //
    // Get memory object
    //
    pWdfRequest->GetOutputMemory(&pRequestMemory);

    hr = m_RingBuffer.Read((PBYTE)pRequestMemory->GetDataBuffer(NULL),
                            SizeInBytes,
                            &BytesCopied);

    //
    // Release memory object.
    //
    SAFE_RELEASE(pRequestMemory);

    if (FAILED(hr))
    {
        //
        // Error reading buffer
        //
        pWdfRequest->Complete(hr);
        goto Exit;
    }

    if (BytesCopied > 0)
    {
        //
        // Data was read from buffer succesfully
        //
        pWdfRequest->CompleteWithInformation(hr, BytesCopied);
    }
    else
    {
        //
        // No data to read. Queue the request for later processing.
        //
        pWdfRequest->ForwardToIoQueue(m_FxReadQueue);
    }

Exit:
    return;
}

VOID
CMyQueue::ProcessWriteBytes(
    _In_reads_bytes_(Length) PUCHAR Characters,
    _In_ SIZE_T Length
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

Return Value:

   VOID

--*/

{

    UCHAR currentCharacter;
    UCHAR connectString[] = "\r\nCONNECT\r\n";
    UCHAR connectStringCch = ARRAY_SIZE(connectString) - 1;
    UCHAR okString[] = "\r\nOK\r\n";
    UCHAR okStringCch = ARRAY_SIZE(okString) - 1;

    while (Length != 0) {

        currentCharacter = *(Characters++);
        Length--;

        if(currentCharacter == '\0')
        {
            continue;
        }

        m_RingBuffer.Write(&currentCharacter, sizeof(currentCharacter));

        switch (m_CommandMatchState) {
            case COMMAND_MATCH_STATE_IDLE:
                if ((currentCharacter == 'a') || (currentCharacter == 'A'))
                {
                    //
                    //  got an A
                    //
                    m_CommandMatchState=COMMAND_MATCH_STATE_GOT_A;
                    m_ConnectCommand=FALSE;
                    m_IgnoreNextChar=FALSE;
                }

            break;

            case COMMAND_MATCH_STATE_GOT_A:
                if ((currentCharacter == 't') || (currentCharacter == 'T'))
                {
                    //
                    //  got a T
                    //
                    m_CommandMatchState=COMMAND_MATCH_STATE_GOT_T;
                }
                else
                {
                    m_CommandMatchState=COMMAND_MATCH_STATE_IDLE;
                }

            break;

            case COMMAND_MATCH_STATE_GOT_T:

                if (!m_IgnoreNextChar)
                {
                    //
                    //  the last char was not a special char
                    //  check for CONNECT command
                    //
                    if ((currentCharacter == 'A') || (currentCharacter == 'a'))
                    {

                        m_ConnectCommand=TRUE;
                    }

                    if ((currentCharacter == 'D') || (currentCharacter == 'd'))
                    {

                        m_ConnectCommand=TRUE;
                    }
                }

                m_IgnoreNextChar=TRUE;

                if (currentCharacter == '\r')
                {
                    //
                    //  got a CR, send a response to the command
                    //
                    m_CommandMatchState = COMMAND_MATCH_STATE_IDLE;

                    if (m_ConnectCommand)
                    {
                        //
                        //  place <cr><lf>CONNECT<cr><lf>  in the buffer
                        //
                        m_RingBuffer.Write(connectString, connectStringCch);

                        //
                        //  connected now raise CD
                        //
                        m_CurrentlyConnected = TRUE;
                        m_ConnectionStateChanged = TRUE;

                    }
                    else
                    {
                        //
                        //  place <cr><lf>OK<cr><lf>  in the buffer
                        //
                        m_RingBuffer.Write(okString, okStringCch);
                    }
                }

            break;
            default:
            break;
        }
    }
    return;
}
