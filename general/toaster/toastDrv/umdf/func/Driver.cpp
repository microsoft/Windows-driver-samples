/*++

  Copyright (c) Microsoft Corporation, All Rights Reserved

  Module Name:

    Driver.cpp

  Abstract:

    This file contains the implementation for the driver object.

  Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#include "stdafx.h"
#include "Driver.h"
#include "Device.h"
#include "Queue.h"

#include "internal.h"
#include "driver.tmh"

//Idle setting for the Toaster device
#define IDLEWAKE_TIMEOUT_MSEC 6000


HRESULT 
CDriver::OnDeviceAdd(
     _In_ IWDFDriver* pDriver,
     _In_ IWDFDeviceInitialize* pDeviceInit
    )
/*++

Routine Description:

    The framework calls this function when a device is being added to
    the driver stack.

Arguments:

    IWDFDriver           - Framework interface.  The driver uses this
                           interface to create device objects.
    IWDFDeviceInitialize - Framework interface.  The driver uses this
                           interface to set device parameters before
                           creating the device obeject.

Return Value:

   HRESULT S_OK - Device added successfully

--*/
{
    IUnknown    *pDeviceCallback  = NULL;
    IWDFDevice  *pIWDFDevice      = NULL;
    IWDFDevice2 *pIWDFDevice2     = NULL;
    IUnknown    *pIUnkQueue       = NULL;    
	
    //
    // UMDF Toaster is a function driver so set is as the power policy owner (PPO)
    //
       pDeviceInit->SetPowerPolicyOwnership(TRUE);     

    //
    // Create our device callback object.
    //
    HRESULT hr = CDevice::CreateInstance(&pDeviceCallback);
    
    //
    // Ask the framework to create a device object for us.
    // We pass in the callback object and device init object
    // as creation parameters.
    //
    if (SUCCEEDED(hr)) 
    {
        hr = pDriver->CreateDevice(pDeviceInit, 
                                   pDeviceCallback,
                                   &pIWDFDevice);
    }

    //
    // Create the queue callback object.
    //

    if (SUCCEEDED(hr)) 
    {
        hr = CQueue::CreateInstance(&pIUnkQueue);
    }

    //
    // Configure the default queue.  We pass in our queue callback
    // object to inform the framework about the callbacks we want.
    //

    if (SUCCEEDED(hr)) 
    {
        IWDFIoQueue * pDefaultQueue = NULL;
        hr = pIWDFDevice->CreateIoQueue(
                          pIUnkQueue,
                          TRUE,                        // bDefaultQueue
                          WdfIoQueueDispatchParallel,
                          TRUE,                        // bPowerManaged
                          FALSE,                       // bAllowZeroLengthRequests
                          &pDefaultQueue);
        SAFE_RELEASE(pDefaultQueue);
    }

    //
    // Enable the device interface.
    //

    if (SUCCEEDED(hr)) 
    {
        hr = pIWDFDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_TOASTER,
                                                NULL);
    }
             
    //
    // IWDFDevice2 interface is an extension of IWDFDevice interface that enables
    // Idle and Wake support. 
    // 

    //
    // Get a pointer to IWDFDevice2 interface
    //
    
    if (SUCCEEDED(hr)) 
    {
        hr = pIWDFDevice->QueryInterface(__uuidof(IWDFDevice2), (void**) &pIWDFDevice2);  
    }	 

    //
    // Since this is a virtual device we tell the framework that we cannot wake 
    // ourself if we sleep in S0. Only way the device can be brought to D0 is if 
    // the device recieves an I/O from the system.
    //

    if (SUCCEEDED(hr)) 
    {
     
       hr = pIWDFDevice2->AssignS0IdleSettings(
                            IdleCannotWakeFromS0,   
                            PowerDeviceD3,          //the lowest-powered device sleeping state
                            IDLEWAKE_TIMEOUT_MSEC,  //idle timeout
                            IdleAllowUserControl,   //user can control the device's idle behavior. 
                            WdfTrue);       
      
    }

    //
    // TODO: Add the Idle and Wake suupport specific for your hardware
    //

    SAFE_RELEASE(pDeviceCallback);
    SAFE_RELEASE(pIWDFDevice);
    SAFE_RELEASE(pIWDFDevice2);
    SAFE_RELEASE(pIUnkQueue);    

    return hr;
}

VOID
CDriver::OnDeinitialize(
     _In_ IWDFDriver * /* pDriver */
     )
/*++

Routine Description:

    The framework calls this function just before de-initializing itself. All
    WDF framework resources should be released by driver before returning from this call.

Arguments:

Return Value:

--*/
{
    return ;
}

HRESULT 
CDriver::OnInitialize(
     _In_ IWDFDriver * /* pDriver */
     )
/*++

Routine Description:

    The framework calls this function just after loading the driver. The driver can
    perform any global, device independent intialization in this routine.

Arguments:

Return Value:

--*/
{
    return S_OK;
}


