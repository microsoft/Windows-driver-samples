/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the sample driver's
    device callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "initguid.h"

#include "device.tmh"

DEFINE_GUID (GUID_DEVINTERFACE_ECHO,
    0xcdc35b6e, 0xbe4, 0x4936, 0xbf, 0x5f, 0x55, 0x37, 0x38, 0xa, 0x7c, 0x1a);
// {CDC35B6E-0BE4-4936-BF5F-5537380A7C1A}

HRESULT
CMyDevice::CreateInstance(
    _In_ IWDFDriver *FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit,
    _Out_ PCMyDevice *Device
    )
/*++

  Routine Description:

    This method creates and initializs an instance of the driver's
    device callback object.

  Arguments:

    FxDeviceInit - the settings for the device.

    Device - a location to store the referenced pointer to the device object.

  Return Value:

    Status

--*/
{
    PCMyDevice device;
    HRESULT hr;

    //
    // Allocate a new instance of the device class.
    //

    device = new CMyDevice();

    if (NULL == device)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //

    hr = device->Initialize(FxDriver, FxDeviceInit);

    if (SUCCEEDED(hr))
    {
        *Device = device;
    }
    else
    {
        device->Release();
    }

    return hr;
}

HRESULT
CMyDevice::Initialize(
    _In_ IWDFDriver           * FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit
    )
/*++

  Routine Description:

    This method initializes the device callback object and creates the
    partner device object.

    The method should perform any device-specific configuration that:
        *  could fail (these can't be done in the constructor)
        *  must be done before the partner object is created -or-
        *  can be done after the partner object is created and which aren't
           influenced by any device-level parameters the parent (the driver
           in this case) might set.

  Arguments:

    FxDeviceInit - the settings for this device.

  Return Value:

    status.

--*/
{
    IWDFDevice *fxDevice = NULL;
    IWDFDeviceInitialize2 *fxDeviceInit2;
    HRESULT hr;

    //
    // Configure things like the locking model before we go to create our
    // partner device.
    //

    //
    // Set no locking unless you need an automatic callbacks synchronization
    //

    FxDeviceInit->SetLockingConstraint(None);

    //
    // TODO: If you're writing a filter driver then indicate that here.
    //
    // FxDeviceInit->SetFilter();
    //

    //
    // TODO: Any per-device initialization which must be done before
    //       creating the partner object.
    //

    //
    // Create a new FX device object and assign the new callback object to
    // handle any device level events that occur.
    //

    //
    // Set retrieval mode to direct I/O. This needs to be done before the call
    // to CreateDevice.
    //
    hr = FxDeviceInit->QueryInterface(IID_PPV_ARGS(&fxDeviceInit2));

    if (SUCCEEDED(hr))
    {
        //
        // WdfDeviceIoBufferedOrDirect for read/write and ioctrl operations. 
        // UMDF defaults to direct-I/O when the device is not running in a shared 
        // wudfhost process, and it defaults to buffered-I/O otherwise. Direct I/O
        // is not allowed when the device is pooled.
        //  
        //
        fxDeviceInit2->SetIoTypePreference(WdfDeviceIoBufferRetrievalDeferred,
                                           WdfDeviceIoBufferedOrDirect,
                                           WdfDeviceIoBufferedOrDirect);

        SAFE_RELEASE(fxDeviceInit2);

        //
        // QueryIUnknown references the IUnknown interface that it returns
        // (which is the same as referencing the device).  We pass that to
        // CreateDevice, which takes its own reference if everything works.
        //
        {
            IUnknown *unknown = this->QueryIUnknown();

            hr = FxDriver->CreateDevice(FxDeviceInit, unknown, &fxDevice);

            unknown->Release();
        }
    }

    //
    // If that succeeded then set our FxDevice member variable.
    //

    if (SUCCEEDED(hr))
    {
        m_FxDevice = fxDevice;

        //
        // Drop the reference we got from CreateDevice.  Since this object
        // is partnered with the framework object they have the same
        // lifespan - there is no need for an additional reference.
        //

        fxDevice->Release();
    }

    return hr;
}

HRESULT
CMyDevice::Configure(
    VOID
    )
/*++

  Routine Description:

    This method is called after the device callback object has been initialized
    and returned to the driver.  It would setup the device's queues and their
    corresponding callback objects.

  Arguments:

    FxDevice - the framework device object for which we're handling events.

  Return Value:

    status

--*/
{
    PCMyQueue defaultQueue;

    HRESULT hr;

    hr = CMyQueue::CreateInstance(m_FxDevice, &defaultQueue);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = defaultQueue->Configure();

    if (SUCCEEDED(hr))
    {
        //
        // In case of success store defaultQueue in our member
        // The reference is transferred to m_DefaultQueue
        //

        m_Queue = defaultQueue;
    }
    else
    {
        //
        // In case of failure release the reference
        //

        defaultQueue->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_ECHO,
                                               NULL);
    }

    return hr;
}

HRESULT
CMyDevice::QueryInterface(
    _In_ REFIID InterfaceId,
    _Out_ PVOID *Object
    )
/*++

  Routine Description:

    This method is called to get a pointer to one of the object's callback
    interfaces.

    Since the sample driver doesn't support any of the device events, this
    method simply calls the base class's BaseQueryInterface.

    If the sample is extended to include device event interfaces then this
    method must be changed to check the IID and return pointers to them as
    appropriate.

  Arguments:

    InterfaceId - the interface being requested

    Object - a location to store the interface pointer if successful

  Return Value:

    S_OK or E_NOINTERFACE

--*/
{
    HRESULT hr;

    if (IsEqualIID(InterfaceId, __uuidof(IPnpCallbackSelfManagedIo))) {
        *Object = QueryIPnpCallbackSelfManagedIo();
        hr = S_OK;
    } else {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

HRESULT
CMyDevice::OnSelfManagedIoInit(
    _In_ IWDFDevice * pWdfDevice
    )
/*++

  Routine Description:

    This method is called to allow driver to initialize any resources
    that driver might need to process I/O.

    Echo driver needs a thread to process completions. We initialize
    this thread here

  Arguments:

    pWdfDevice - framework device object for which to initialze resources

  Return Value:

    S_OK in case of success
    HRESULT correponding to error returned by CreateThread, in case of failure

--*/
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pWdfDevice);


    m_ThreadHandle = CreateThread( NULL,                        // Default Security Attrib.
                                   0,                           // Initial Stack Size,
                                   CMyQueue::CompletionThread,  // Thread Func
                                   (LPVOID)m_Queue,             // Arg to Thread Func is Queue
                                   0,                           // Creation Flags
                                   NULL );                      // Don't need the Thread Id.

    if (m_ThreadHandle == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

void
CMyDevice::OnSelfManagedIoCleanup(
    _In_ IWDFDevice * pWdfDevice
    )
/*++

  Routine Description:

    This method is called to allow driver to cleanup any resources
    that driver allocated to process I/O.

    It is critical that, in this routine driver wait for all of the
    threads which it created to exit. Otherwise those threads could
    continue to execute when framework unloads the driver which
    would lead to a crash.

    Echo driver created a thread to handle completions. We wait for
    that thread to exit in this routine

  Arguments:

    pWdfDevice - framework device object for which to cleanup resources

  Return Value:

    None

--*/
{
    //
    // Kill the thread and
    // wait for the thread to die.
    //

    UNREFERENCED_PARAMETER(pWdfDevice);

    if (m_ThreadHandle) {

        //
        // Ask queue to set terminate flag which will make
        // the thread exit
        //
        m_Queue->SetExitThread();

        //
        // Wait for the thread to exit
        //

        WaitForSingleObject(m_ThreadHandle, INFINITE);

        //
        // Close the thread handle
        //

        CloseHandle(m_ThreadHandle);
        m_ThreadHandle = NULL;
    }

    //
    // Release the reference we took on the queue callback object
    // to keep it alive until the thread exits
    //

    SAFE_RELEASE(m_Queue);
}

