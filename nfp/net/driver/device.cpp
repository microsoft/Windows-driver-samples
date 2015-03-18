/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the UMDF sample
    driver's device callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "device.tmh"

HRESULT
CMyDevice::Initialize(
    _In_ IWDFDriver* FxDriver,
    _In_ IWDFDeviceInitialize* FxDeviceInit
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
    FxDriver    - IWDF Driver for this device.

  Return Value:

    status.

--*/
{
    MethodEntry("...");

    CComPtr<IWDFDevice> fxDevice;
    HRESULT hr;

    //
    // Configure things like the locking model before we go to create our
    // partner device.
    //

    //
    // Set the locking model
    //

    FxDeviceInit->SetLockingConstraint(None);

    //
    // TODO: Any per-device initialization which must be done before
    //       creating the partner object.
    //

    //
    // Create a new FX device object and assign the new callback object to
    // handle any device level events that occur.
    //

    //
    // QueryIUnknown references the IUnknown interface that it returns
    // (which is the same as referencing the device).  We pass that to
    // CreateDevice, which takes its own reference if everything works.
    //

    CComPtr<IUnknown> pUnk;
    HRESULT hrQI = this->QueryInterface(__uuidof(IUnknown),(void**)&pUnk);
    WUDF_SAMPLE_DRIVER_ASSERT(SUCCEEDED(hrQI));

    hr = FxDriver->CreateDevice(FxDeviceInit, pUnk, &fxDevice);

    //
    // If that succeeded then set our FxDevice member variable.
    //

    if (SUCCEEDED(hr))
    {
        m_FxDevice = fxDevice;
    }

    MethodReturnHR(hr);
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
    MethodEntry("void");

    //
    // Create a new instance of our Queue callback object
    //
    CComObject<CMyQueue> * defaultQueue = NULL;
    HRESULT hr = CComObject<CMyQueue>::CreateInstance(&defaultQueue);

    if (SUCCEEDED(hr))
    {
        defaultQueue->AddRef();
        hr = defaultQueue->Initialize(this);
    }

    if (SUCCEEDED(hr))
    {
        hr = defaultQueue->Configure();
    }

    //
    // Create and Enable Device Interfaces for this device.
    //
    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_NETNFP,
                                               NULL);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->AssignDeviceInterfaceState(&GUID_DEVINTERFACE_NETNFP,
                                                    NULL,
                                                    TRUE);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_NFP,
                                               NULL);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->AssignDeviceInterfaceState(&GUID_DEVINTERFACE_NFP,
                                                    NULL,
                                                    TRUE);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Save a pointer to our queue, so we can lock it during file cleanup
        //
        m_MyQueue = defaultQueue;
    }

    //
    // Release the reference we took on the queue object.
    // The framework took its own references on the object's callback interfaces
    // when we called m_FxDevice->CreateIoQueue, and will manage the object's lifetime.
    //
    SAFE_RELEASE(defaultQueue);

    MethodReturnHR(hr);
}

STDMETHODIMP_(void)
CMyDevice::OnCloseFile(
    _In_ IWDFFile* pWdfFileObject
    )
{
    MethodEntry("...");
    
    m_MyQueue->OnCloseFile(pWdfFileObject);

    MethodReturnVoid();
}

STDMETHODIMP_(void)
CMyDevice::OnCleanupFile(
    _In_ IWDFFile* /*pWdfFileObject*/
    )
/*++

  Routine Description:

    This method is when app with open handle device terminates.

  Arguments:

    pWdfFileObject - the framework file object for which close is handled.

  Return Value:

    None

--*/
{
}

STDMETHODIMP_(void)
CMyDevice::OnCleanup(
    _In_ IWDFObject* pWdfObject
    )
/*++

  Routine Description:

    This device callback method is invoked by the framework when the WdfObject
    is about to be released by the framework.

  Arguments:

    pWdfObject - the framework device object for which OnCleanup.

  Return Value:

    None

--*/
{
    MethodEntry("...");

    WUDF_SAMPLE_DRIVER_ASSERT(pWdfObject == m_FxDevice);

    m_MyQueue = NULL;
    
    //
    // CMyDevice has a reference to framework device object via m_Device. 
    // Framework device object has a reference to CMyDevice object via the callbacks. 
    // This leads to circular reference and both the objects can't be destroyed until this circular reference is broken. 
    // To break the circular reference we release the reference to the framework device object here in OnCleanup.
    //
    m_FxDevice = NULL;

    MethodReturnVoid();
}
