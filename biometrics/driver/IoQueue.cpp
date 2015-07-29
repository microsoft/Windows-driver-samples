/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    IoQueue.cpp

Abstract:

    This file implements the I/O queue interface and performs
    the ioctl operations.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "ioqueue.tmh"

 
HRESULT 
CBiometricIoQueue::CreateInstanceAndInitialize(
    _In_ IWDFDevice *FxDevice,
    _In_ CBiometricDevice *BiometricDevice,
    _Out_ CBiometricIoQueue** Queue
    )
/*++

Routine Description:

    CreateInstanceAndInitialize creates an instance of the queue object.

Arguments:
    

Return Value:

    HRESULT indicating success or failure

--*/
{
    //
    // Create a new instance of the device class
    //
    CComObject<CBiometricIoQueue> *pMyQueue = NULL;
    HRESULT hr = CComObject<CBiometricIoQueue>::CreateInstance( &pMyQueue );

    if (SUCCEEDED(hr)) {

        //
        // Initialize the instance.
        //

        if (NULL != pMyQueue)
        {
            hr = pMyQueue->Initialize(FxDevice, BiometricDevice);
        }

        *Queue = pMyQueue;

    }

    return hr;
}

HRESULT
CBiometricIoQueue::Initialize(
    _In_ IWDFDevice *FxDevice,
    _In_ CBiometricDevice *BiometricDevice
    )
/*++

Routine Description:

    Initialize creates a framework queue and sets up I/O for the queue object.

Arguments:

    FxDevice - Framework device associated with this queue.

    BiometricDevice - Pointer to the Biometric device class object.

Return Value:

    HRESULT indicating success or failure

--*/
{
    IWDFIoQueue *fxQueue = NULL;
    HRESULT hr = S_OK;
    IUnknown *unknown = NULL;
    
    //
    // Make sure we have valid parameters.
    //
    if (FxDevice == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, 
                   BIOMETRIC_TRACE_QUEUE, 
                   "%!FUNC!Pointer to framework device object is NULL.");
        return (E_INVALIDARG);
    }
    if (BiometricDevice == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, 
                   BIOMETRIC_TRACE_QUEUE, 
                   "%!FUNC!Pointer to Biometric device is NULL.");
        return (E_INVALIDARG);
    }

    //
    // Create the framework queue
    //

    if (SUCCEEDED(hr)) 
    {
        hr = this->QueryInterface(__uuidof(IUnknown), (void **)&unknown);

    }

    if (SUCCEEDED(hr))
    {
        hr = FxDevice->CreateIoQueue(unknown,
                                     FALSE,     // Default Queue?
                                     WdfIoQueueDispatchParallel,  // Dispatch type
                                     FALSE,     // Power managed?
                                     FALSE,     // Allow zero-length requests?
                                     &fxQueue); // I/O queue
        BiometricSafeRelease(unknown);
    }

    if (FAILED(hr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, 
                   BIOMETRIC_TRACE_QUEUE, 
                   "%!FUNC!Failed to create framework queue.");
        return hr;
    }

    //
    // Configure this queue to filter all Device I/O requests.
    //
    hr = FxDevice->ConfigureRequestDispatching(fxQueue,
                                               WdfRequestDeviceIoControl,
                                               TRUE);

    if (SUCCEEDED(hr))
    {
        m_FxQueue = fxQueue;
        m_BiometricDevice= BiometricDevice;
    }

    //
    // Safe to release here.  The framework keeps a reference to the Queue
    // for the lifetime of the device.
    //
    BiometricSafeRelease(fxQueue);

    return hr;
}

VOID
STDMETHODCALLTYPE
CBiometricIoQueue::OnDeviceIoControl(
    _In_ IWDFIoQueue *FxQueue,
    _In_ IWDFIoRequest *FxRequest,
    _In_ ULONG ControlCode,
    _In_ SIZE_T InputBufferSizeInBytes,
    _In_ SIZE_T OutputBufferSizeInBytes
    )
/*++

Routine Description:


    DeviceIoControl dispatch routine

Aruments:

    FxQueue - Framework Queue instance
    FxRequest - Framework Request  instance
    ControlCode - IO Control Code
    InputBufferSizeInBytes - Lenth of input buffer
    OutputBufferSizeInBytes - Lenth of output buffer

    Always succeeds DeviceIoIoctl
Return Value:

    VOID

--*/
{
    UNREFERENCED_PARAMETER(FxQueue);
    UNREFERENCED_PARAMETER(InputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(OutputBufferSizeInBytes);

    if (m_BiometricDevice == NULL) {
        // We don't have pointer to device object
        TraceEvents(TRACE_LEVEL_ERROR, 
                   BIOMETRIC_TRACE_QUEUE, 
                   "%!FUNC!NULL pointer to device object.");
        FxRequest->Complete(E_POINTER);
        return;
    }

    //
    // Process the IOCTLs
    //

    switch (ControlCode) {

        //
        // Mandatory IOCTLs
        //
        case IOCTL_BIOMETRIC_GET_ATTRIBUTES:
            m_BiometricDevice->OnGetAttributes(FxRequest);
            break;

        case IOCTL_BIOMETRIC_RESET:
            m_BiometricDevice->OnReset(FxRequest);
            break;

        case IOCTL_BIOMETRIC_CALIBRATE:
            m_BiometricDevice->OnCalibrate(FxRequest);
            break;
            
        case IOCTL_BIOMETRIC_GET_SENSOR_STATUS:
            m_BiometricDevice->OnGetSensorStatus(FxRequest);
            break;

        case IOCTL_BIOMETRIC_CAPTURE_DATA:
            m_BiometricDevice->OnCaptureData(FxRequest);
            break;

        //
        // Optional IOCTLs
        //
        case IOCTL_BIOMETRIC_UPDATE_FIRMWARE:
            m_BiometricDevice->OnUpdateFirmware(FxRequest);
            break;

        case IOCTL_BIOMETRIC_GET_SUPPORTED_ALGORITHMS:
            m_BiometricDevice->OnGetSupportedAlgorithms(FxRequest);
            break;

        case IOCTL_BIOMETRIC_GET_INDICATOR:
            m_BiometricDevice->OnGetIndicator(FxRequest);
            break;

        case IOCTL_BIOMETRIC_SET_INDICATOR:
            m_BiometricDevice->OnSetIndicator(FxRequest);
            break;

        default:

            //
            // First check to see if this is for a BIOMETRIC file.
            //
            if ((ControlCode & CTL_CODE(0xFFFFFFFF, 0, 0, 0)) == CTL_CODE(FILE_DEVICE_BIOMETRIC, 0, 0, 0)) {

                if ((ControlCode & IOCTL_BIOMETRIC_VENDOR) == IOCTL_BIOMETRIC_VENDOR) {
                    // This is a vendor IOCTL.
                    m_BiometricDevice->OnControlUnit(FxRequest);
                    break;
                }

            } else {

                // This is a legacy IOCTL - non-Windows Biometric Framework
                TraceEvents(TRACE_LEVEL_ERROR, 
                           BIOMETRIC_TRACE_QUEUE, 
                           "%!FUNC!Legacy control units not supported by the driver.");
 
            }

            //
            // Didn't match any of the above.
            //
            TraceEvents(TRACE_LEVEL_ERROR, 
                       BIOMETRIC_TRACE_QUEUE, 
                       "%!FUNC! Unsupported IOCTL - 0x%x.",
                       ControlCode);
            FxRequest->Complete(HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION));
            break;

    }

    return;

}

