/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Driver.cpp

Abstract:

    This module contains the implementation of the UMDF Socketecho Sample's 
    core driver callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/
#include "internal.h"

#include "driver.tmh"

DECLARE_TRACING_TLS;

STDMETHODIMP 
CMyDriver::OnInitialize(
    _In_ IWDFDriver* /*pWdfDriver*/
    )

   
/*++
 
  Routine Description:

    This routine is invoked by the framework at driver load . 
    This method will invoke the Winsock Library for using 
    Winsock API in this driver.
    
  Arguments:

    pWdfDriver - Framework driver object

  Return Value:

    S_OK if successful, or error otherwise.

--*/

{
    MethodEntry("...");

    HRESULT hr = S_OK;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0)
    {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
        TraceErrorHR(hr, "Failed to initialize Winsock 2.0");
    }

    MethodReturnHR(hr);
}

STDMETHODIMP_(void) 
CMyDriver::OnDeinitialize(
  _In_ IWDFDriver* /*pWdfDriver*/
  )

/*++
  Routine Description:

    The FX invokes this method when it unloads the driver. 
    This routine will Cleanup Winsock library 

  Arguments:

    pWdfDriver - the Fx driver object.

  Return Value:

    None
    
    --*/
{
    MethodEntry("...");

    WSACleanup();

    MethodReturnVoid();
}

STDMETHODIMP
CMyDriver::OnDeviceAdd(
    _In_ IWDFDriver *FxWdfDriver,
    _In_ IWDFDeviceInitialize *FxDeviceInit
    )
/*++
 
  Routine Description:

    The FX invokes this method when it wants to install our driver on a device
    stack.  This method creates a device callback object, then calls the Fx
    to create an Fx device object and associate the new callback object with
    it.

  Arguments:

    FxWdfDriver - the Fx driver object.

    FxDeviceInit - the initialization information for the device.

  Return Value:

    status

--*/
{
    MethodEntry("...");

    //
    // Create a new instance of our device callback object 
    //

    CComObject<CMyDevice> * device;
    HRESULT hr = CComObject<CMyDevice>::CreateInstance(&device);
    if (SUCCEEDED(hr))
    {
        device->AddRef();
        hr = device->Initialize(FxWdfDriver, FxDeviceInit);
        if (SUCCEEDED(hr)) 
        {
            //
            // If that succeeded then call the device's configure method.  This 
            // allows the device to create any queues or other structures that it
            // needs now that the corresponding fx device object has been created.
            //
            hr = device->Configure();
        }

        //
        // Release the reference we took on the device object.
        // The framework took its own references on the object's callback interfaces
        // when we called FxWdfDriver->CreateDevice, and will manage the object's lifetime.
        //
        SAFE_RELEASE(device);
    }

    MethodReturnHR(hr);
}
