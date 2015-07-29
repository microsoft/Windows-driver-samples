/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Driver.cpp

Abstract:

    This module contains the implementation of the Biometric
    core driver callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "driver.tmh"

HRESULT
CBiometricDriver::OnDeviceAdd(
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
    HRESULT hr = S_OK;
    CBiometricDevice *device = NULL;

    //
    // Create device callback object
    //

    hr = CBiometricDevice::CreateInstanceAndInitialize(FxWdfDriver,
                                                       FxDeviceInit,
                                                       &device);

    //
    // Call the device's construct method.  This 
    // allows the device to create any queues or other structures that it
    // needs now that the corresponding fx device object has been created.
    //

    if (SUCCEEDED(hr))
    {
        hr = device->Configure();
    }

    return hr;
}
