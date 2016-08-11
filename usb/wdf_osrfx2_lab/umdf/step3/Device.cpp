/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the UMDF Skeleton sample driver's
    device callback object.

    The skeleton sample device does very little.  It does not implement either
    of the PNP interfaces so once the device is setup, it won't ever get any
    callbacks until the device is removed.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "initguid.h"
#include "usb_hw.h"

#include "device.tmh"

CMyDevice::~CMyDevice(
    )
{
}

HRESULT
CMyDevice::CreateInstance(
    _In_ IWDFDriver *FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit,
    _Out_ PCMyDevice *Device
    )
/*++
 
  Routine Description:

    This method creates and initializs an instance of the skeleton driver's 
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

    HRESULT hr = S_OK;

    //
    // TODO: If you're writing a filter driver then indicate that here. 
    //
    // FxDeviceInit->SetFilter();
    //

    //
    // Set no locking unless you need an automatic callbacks synchronization
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

    if (SUCCEEDED(hr)) 
    {
        IUnknown *unknown = this->QueryIUnknown();

        hr = FxDriver->CreateDevice(FxDeviceInit, unknown, &fxDevice);

        unknown->Release();
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
    HRESULT hr = S_OK;


    //
    // Create the control queue and configure forwarding for IOCTL requests.
    //

    if (SUCCEEDED(hr)) 
    {
        hr = CMyControlQueue::CreateInstance(this, &m_ControlQueue);

        if (SUCCEEDED(hr)) 
        {
            hr = m_ControlQueue->Configure();
            if (SUCCEEDED(hr)) 
            {
                hr = m_FxDevice->ConfigureRequestDispatching(
                                    m_ControlQueue->GetFxQueue(),
                                    WdfRequestDeviceIoControl,
                                    true
                                    );
            }
            m_ControlQueue->Release();
        }
    }
    
    if (SUCCEEDED(hr)) 
    {
        hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_OSRUSBFX2,
                                               NULL);
    }

    return hr;
}

HRESULT
CMyDevice::QueryInterface(
    _In_ REFIID InterfaceId,
    _Outptr_ PVOID *Object
    )
/*++
 
  Routine Description:

    This method is called to get a pointer to one of the object's callback
    interfaces.  

    Since the skeleton driver doesn't support any of the device events, this
    method simply calls the base class's BaseQueryInterface.

    If the skeleton is extended to include device event interfaces then this 
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

    if (IsEqualIID(InterfaceId, __uuidof(IPnpCallbackHardware)))
    {
        *Object = QueryIPnpCallbackHardware();
        hr = S_OK;    
    }
    else
    {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

HRESULT
CMyDevice::OnPrepareHardware(
    _In_ IWDFDevice * /* FxDevice */
    )
/*++

Routine Description:

    This routine is invoked to ready the driver
    to talk to hardware. It opens the handle to the 
    device and talks to it using the WINUSB interface.
    It invokes WINUSB to discver the interfaces and stores
    the information related to bulk endpoints.

Arguments:

    FxDevice  : Pointer to the WDF device interface

Return Value:

    HRESULT 

--*/
{
    PWSTR deviceName = NULL;
    DWORD deviceNameCch = 0;

    HRESULT hr;

    //
    // Get the device name.
    // Get the length to allocate first
    //

    hr = m_FxDevice->RetrieveDeviceName(NULL, &deviceNameCch);

    if (FAILED(hr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, 
                    TEST_TRACE_DEVICE, 
                    "%!FUNC! Cannot get device name %!hresult!",
                    hr
                    );
    }

    //
    // Allocate the buffer
    //

    if (SUCCEEDED(hr))
    {
        deviceName = new WCHAR[deviceNameCch];

        if (deviceName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // Get the actual name
    //

    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->RetrieveDeviceName(deviceName, &deviceNameCch);

        if (FAILED(hr))
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Cannot get device name %!hresult!",
                        hr
                        );
        }
    }

    if (SUCCEEDED(hr))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, 
                    TEST_TRACE_DEVICE, 
                    "%!FUNC! Device name %S",
                    deviceName
                    );
    }

    //
    // Create USB I/O Targets and configure them
    //

    if (SUCCEEDED(hr))
    {
        hr = CreateUsbIoTargets();
    }

    if (SUCCEEDED(hr))
    {
        ULONG length = sizeof(m_Speed);

        hr = m_pIUsbTargetDevice->RetrieveDeviceInformation(DEVICE_SPEED, 
                                                            &length,
                                                            &m_Speed);
        if (FAILED(hr)) 
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Cannot get usb device speed information %!HRESULT!",
                        hr
                        );
        }
    }

    if (SUCCEEDED(hr)) 
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, 
                    TEST_TRACE_DEVICE, 
                    "%!FUNC! Speed - %x\n",
                    m_Speed
                    );
    }

    delete[] deviceName;

    return hr;
}

HRESULT
CMyDevice::OnReleaseHardware(
    _In_ IWDFDevice * /* FxDevice */
    )
/*++

Routine Description:

    This routine is invoked when the device is being removed or stopped
    It releases all resources allocated for this device.

Arguments:

    FxDevice - Pointer to the Device object.

Return Value:

    HRESULT - Always succeeds.

--*/
{
    //
    // Remove I/O target from object tree before any potential subsequent 
    // OnPrepareHardware creates a new one
    //
    
    if (m_pIUsbTargetDevice)
    {
        m_pIUsbTargetDevice->DeleteWdfObject();
    }
    
    return S_OK;
}

HRESULT
CMyDevice::CreateUsbIoTargets(
    )
/*++

Routine Description:

    This routine creates Usb device, interface and pipe objects

Arguments:

    None

Return Value:

    HRESULT
--*/
{
    HRESULT                 hr;
    IWDFUsbTargetFactory *  pIUsbTargetFactory = NULL;
    IWDFUsbTargetDevice *   pIUsbTargetDevice = NULL;
    
    hr = m_FxDevice->QueryInterface(IID_PPV_ARGS(&pIUsbTargetFactory));

    if (FAILED(hr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, 
                    TEST_TRACE_DEVICE, 
                    "%!FUNC! Cannot get usb target factory %!HRESULT!",
                    hr
                    );        
    }

    if (SUCCEEDED(hr)) 
    {
        hr = pIUsbTargetFactory->CreateUsbTargetDevice(
                                                  &pIUsbTargetDevice);
        if (FAILED(hr))
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Unable to create USB Device I/O Target %!HRESULT!",
                        hr
                        );        
        }
    }

    if (SUCCEEDED(hr)) 
    {
        m_pIUsbTargetDevice = pIUsbTargetDevice;

        //
        // Release the creation reference as object tree will maintain a reference
        //
        
        pIUsbTargetDevice->Release();
    }

    SAFE_RELEASE(pIUsbTargetFactory);

    return hr;
}

HRESULT
CMyDevice::SetBarGraphDisplay(
    _In_ PBAR_GRAPH_STATE BarGraphState
    )
/*++
 
  Routine Description:

    This method synchronously sets the bar graph display on the OSR USB-FX2 
    device using the buffers in the FxRequest as input.

  Arguments:

    FxRequest - the request to set the bar-graph info.

  Return Value:

    Status

--*/
{
    WINUSB_CONTROL_SETUP_PACKET setupPacket;

    ULONG bytesTransferred;

    HRESULT hr = S_OK;

    //
    // Setup the control packet.
    //

    WINUSB_CONTROL_SETUP_PACKET_INIT( &setupPacket,
                                      BmRequestHostToDevice,
                                      BmRequestToDevice,
                                      USBFX2LK_SET_BARGRAPH_DISPLAY,
                                      0,
                                      0 );

    //
    // Issue the request to WinUsb.
    //

    hr = SendControlTransferSynchronously(
                &(setupPacket.WinUsb),
                (PUCHAR) BarGraphState,
                sizeof(BAR_GRAPH_STATE),
                &bytesTransferred
                );


    return hr;
}

HRESULT
CMyDevice::SendControlTransferSynchronously(
    _In_ PWINUSB_SETUP_PACKET SetupPacket,
    _Inout_updates_(BufferLength) PBYTE Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG LengthTransferred
    )
{
    HRESULT hr = S_OK;
    HRESULT hrRequest = S_OK;
    IWDFIoRequest *pWdfRequest = NULL;
    IWDFDriver * FxDriver = NULL;
    IWDFMemory * FxMemory = NULL; 
    IWDFRequestCompletionParams * FxComplParams = NULL;
    IWDFUsbRequestCompletionParams * FxUsbComplParams = NULL;

    *LengthTransferred = 0;
    
    hr = m_FxDevice->CreateRequest( NULL, //pCallbackInterface
                                    NULL, //pParentObject
                                    &pWdfRequest);
    hrRequest = hr;

    if (SUCCEEDED(hr))
    {
        m_FxDevice->GetDriver(&FxDriver);

        hr = FxDriver->CreatePreallocatedWdfMemory( Buffer,
                                                    BufferLength,
                                                    NULL, //pCallbackInterface
                                                    pWdfRequest, //pParetObject
                                                    &FxMemory );
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pIUsbTargetDevice->FormatRequestForControlTransfer( pWdfRequest,
                                                                   SetupPacket,
                                                                   FxMemory,
                                                                   NULL); //TransferOffset
    }                                                          
                        
    if (SUCCEEDED(hr))
    {
        hr = pWdfRequest->Send( m_pIUsbTargetDevice,
                                WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
                                0); //Timeout
    }

    if (SUCCEEDED(hr))
    {
        pWdfRequest->GetCompletionParams(&FxComplParams);

        hr = FxComplParams->GetCompletionStatus();
    }

    if (SUCCEEDED(hr))
    {
        HRESULT hrQI = FxComplParams->QueryInterface(IID_PPV_ARGS(&FxUsbComplParams));
        WUDF_TEST_DRIVER_ASSERT(SUCCEEDED(hrQI));

        WUDF_TEST_DRIVER_ASSERT( WdfUsbRequestTypeDeviceControlTransfer == 
                            FxUsbComplParams->GetCompletedUsbRequestType() );

        FxUsbComplParams->GetDeviceControlTransferParameters( NULL,
                                                             LengthTransferred,
                                                             NULL,
                                                             NULL );
    }

    SAFE_RELEASE(FxUsbComplParams);
    SAFE_RELEASE(FxComplParams);
    SAFE_RELEASE(FxMemory);

    if (SUCCEEDED(hrRequest))
    {
        pWdfRequest->DeleteWdfObject();
    }       
    SAFE_RELEASE(pWdfRequest);

    SAFE_RELEASE(FxDriver);

    return hr;
}
