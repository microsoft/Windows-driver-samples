/*++
 
Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the UMDF OSR Fx2 driver's
    device callback object.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/
#include "internal.h"
#include "initguid.h"
#include "usb_hw.h"
#include <devpkey.h>

#include "device.tmh"
#define CONCURRENT_READS 2

CMyDevice::~CMyDevice(
    )
{
    SAFE_RELEASE(m_pIoTargetInterruptPipeStateMgmt); 
}

HRESULT
CMyDevice::CreateInstance(
    _In_ IWDFDriver *FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit,
    _Out_ PCMyDevice *Device
    )
/*++
 
  Routine Description:

    This method creates and initializs an instance of the OSR Fx2 driver's 
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
    IWDFDevice2 *fxDevice = NULL;

    HRESULT hr = S_OK;

    //
    // TODO: Any per-device initialization which must be done before 
    //       creating the partner object.
    //

    //
    // Set no locking unless you need an automatic callbacks synchronization
    //

    FxDeviceInit->SetLockingConstraint(None);

    //
    // TODO: If you're writing a filter driver then indicate that here. 
    //       And then don't claim power policy ownership below
    //
    // FxDeviceInit->SetFilter();
    //
        
    //
    // Set the Fx2 driver as the power policy owner.
    //

    FxDeviceInit->SetPowerPolicyOwnership(TRUE);

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
        IWDFDevice* device1;

        hr = FxDriver->CreateDevice(FxDeviceInit, unknown, &device1);

        //
        // Convert the interface to version 2
        //

        if (SUCCEEDED(hr)) {
            device1->QueryInterface(IID_PPV_ARGS(&fxDevice));
            _Analysis_assume_(fxDevice != NULL);
            device1->Release();
        }

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
    // Get the bus type GUID for the device and confirm that we're attached to
    // USB.
    //
    // NOTE: Since this device only supports USB we'd normally trust our INF 
    // to ensure this.
    //
    // But if the device also supported 1394 then we could 
    // use this to determine which type of bus we were attached to.
    // 

    hr = GetBusTypeGuid();

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Create the read-write queue.
    //

    hr = CMyReadWriteQueue::CreateInstance(this, &m_ReadWriteQueue);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // We use default queue for read/write
    //
    
    hr = m_ReadWriteQueue->Configure();

    m_ReadWriteQueue->Release();

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
                m_FxDevice->ConfigureRequestDispatching(
                                m_ControlQueue->GetFxQueue(),
                                WdfRequestDeviceIoControl,
                                true
                                );
            }
            m_ControlQueue->Release();         
        }
    }

    //
    // Create a manual I/O queue to hold requests for notification when
    // the switch state changes.
    //

    hr = m_FxDevice->CreateIoQueue(NULL,
                                   FALSE,
                                   WdfIoQueueDispatchManual,
                                   FALSE,
                                   FALSE,
                                   &m_SwitchChangeQueue);
    

    //
    // Release creation reference as object tree will keep a reference
    //
    
    m_SwitchChangeQueue->Release();

    if (SUCCEEDED(hr)) 
    {
        hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_OSRUSBFX2,
                                               NULL);
    }

    //
    // Mark the interface as restricted to allow access to applications bound
    // using device metadata.  Failures here are not fatal so we log them but
    // ignore them otherwise.
    //
    if (SUCCEEDED(hr))
    {
        WDF_PROPERTY_STORE_ROOT RootSpecifier;
        IWDFUnifiedPropertyStoreFactory * pUnifiedPropertyStoreFactory = NULL;
        IWDFUnifiedPropertyStore * pUnifiedPropertyStore = NULL;
        DEVPROP_BOOLEAN isRestricted = DEVPROP_TRUE;
        HRESULT hrSetProp;

        hrSetProp = m_FxDevice->QueryInterface(IID_PPV_ARGS(&pUnifiedPropertyStoreFactory));

        WUDF_TEST_DRIVER_ASSERT(SUCCEEDED(hrSetProp));

        RootSpecifier.LengthCb = sizeof(RootSpecifier);
        RootSpecifier.RootClass = WdfPropertyStoreRootClassDeviceInterfaceKey;
        RootSpecifier.Qualifier.DeviceInterfaceKey.InterfaceGUID = &GUID_DEVINTERFACE_OSRUSBFX2;
        RootSpecifier.Qualifier.DeviceInterfaceKey.ReferenceString = NULL;

        hrSetProp = pUnifiedPropertyStoreFactory->RetrieveUnifiedDevicePropertyStore(&RootSpecifier,
                                                                                     &pUnifiedPropertyStore);

        if (SUCCEEDED(hrSetProp))
        {
            hrSetProp = pUnifiedPropertyStore->SetPropertyData(&DEVPKEY_DeviceInterface_Restricted,
                                                               0, // Lcid
                                                               0, // Flags
                                                               DEVPROP_TYPE_BOOLEAN,
                                                               sizeof(isRestricted),
                                                               &isRestricted);
        }

        if (FAILED(hrSetProp))
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Could not set restricted property %!HRESULT!",
                        hrSetProp
                        );
        }

        SAFE_RELEASE(pUnifiedPropertyStoreFactory);
        SAFE_RELEASE(pUnifiedPropertyStore);
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
    else if (IsEqualIID(InterfaceId, __uuidof(IPnpCallback)))
    {
        *Object = QueryIPnpCallback();
        hr = S_OK;    
    }     
    else if (IsEqualIID(InterfaceId, __uuidof(IPnpCallbackSelfManagedIo)))
    {
        *Object = QueryIPnpCallbackSelfManagedIo();
        hr = S_OK;    
    }     
    else if(IsEqualIID(InterfaceId, __uuidof(IUsbTargetPipeContinuousReaderCallbackReadersFailed))) 
    {    
        *Object = QueryContinousReaderFailureCompletion();
        hr = S_OK;  
    } 
    else if(IsEqualIID(InterfaceId, __uuidof(IUsbTargetPipeContinuousReaderCallbackReadComplete))) 
    {    
        *Object = QueryContinousReaderCompletion();
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
                    "%!FUNC! Cannot get device name %!HRESULT!",
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
                        "%!FUNC! Cannot get device name %!HRESULT!",
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

    if (SUCCEEDED(hr))
    {
        hr = ConfigureUsbPipes();
    }

    // Setup power-management settings on the device.
    //

    if (SUCCEEDED(hr))
    {
        hr = SetPowerManagement();
    }

    //
    //
    // Clear the seven segement display to indicate that we're done with 
    // prepare hardware.
    //

    if (SUCCEEDED(hr))
    {
        hr = IndicateDeviceReady();
    }

    if (SUCCEEDED(hr))
    {
        hr = ConfigContReaderForInterruptEndPoint();
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
    // Delete USB Target Device WDF Object, this will in turn
    // delete all the children - interface and the pipe objects
    //
    // This makes sure that 
    //    1. We drain the the pending read which does not come from an I/O queue
    //    2. We remove USB target objects from object tree (and thereby free them)
    //       before any potential subsequent OnPrepareHardware creates new ones
    //
    // m_pIUsbTargetDevice could be NULL if OnPrepareHardware failed so we need
    // to guard against that
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
    UCHAR                   NumEndPoints = 0;
    IWDFUsbTargetFactory *  pIUsbTargetFactory = NULL;
    IWDFUsbTargetDevice *   pIUsbTargetDevice = NULL;
    IWDFUsbInterface *      pIUsbInterface = NULL;
    IWDFUsbTargetPipe *     pIUsbPipe = NULL;
    
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
        else
        {
            m_pIUsbTargetDevice = pIUsbTargetDevice;

            //
            // Release the creation reference as object tree will maintain a reference
            //
            
            pIUsbTargetDevice->Release();            
        }
    }

    if (SUCCEEDED(hr)) 
    {
        UCHAR NumInterfaces = pIUsbTargetDevice->GetNumInterfaces();

        WUDF_TEST_DRIVER_ASSERT(1 == NumInterfaces);
        
        hr = pIUsbTargetDevice->RetrieveUsbInterface(0, &pIUsbInterface);
        if (FAILED(hr))
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Unable to retrieve USB interface from USB Device I/O Target %!HRESULT!",
                        hr
                        );        
        }
        else
        {
            m_pIUsbInterface = pIUsbInterface;

            pIUsbInterface->Release(); //release creation reference                        
        }
    }

    if (SUCCEEDED(hr)) 
    {
        NumEndPoints = pIUsbInterface->GetNumEndPoints();

        if (NumEndPoints != NUM_OSRUSB_ENDPOINTS) {
            hr = E_UNEXPECTED;
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Has %d endpoints, expected %d, returning %!HRESULT! ", 
                        NumEndPoints,
                        NUM_OSRUSB_ENDPOINTS,
                        hr
                        );
        }
    }

    if (SUCCEEDED(hr)) 
    {
        for (UCHAR PipeIndex = 0; PipeIndex < NumEndPoints; PipeIndex++)
        {
            hr = pIUsbInterface->RetrieveUsbPipeObject(PipeIndex, 
                                                  &pIUsbPipe);

            if (FAILED(hr))
            {
                TraceEvents(TRACE_LEVEL_ERROR, 
                            TEST_TRACE_DEVICE, 
                            "%!FUNC! Unable to retrieve USB Pipe for PipeIndex %d, %!HRESULT!",
                            PipeIndex,
                            hr
                            );        
            }
            else
            {
                if ( pIUsbPipe->IsInEndPoint() )
                {
                    if ( UsbdPipeTypeInterrupt == pIUsbPipe->GetType() )
                    {
                        m_pIUsbInterruptPipe = pIUsbPipe;

                        WUDF_TEST_DRIVER_ASSERT (m_pIoTargetInterruptPipeStateMgmt == NULL);

                        hr = m_pIUsbInterruptPipe->QueryInterface(__uuidof(
                                                   IWDFIoTargetStateManagement),
                                                   reinterpret_cast<void**>(&m_pIoTargetInterruptPipeStateMgmt)
                                                   );
                        if (FAILED(hr))
                        {
                            m_pIoTargetInterruptPipeStateMgmt = NULL;
                        }                        
                    }
                    else if ( UsbdPipeTypeBulk == pIUsbPipe->GetType() )
                    {
                        m_pIUsbInputPipe = pIUsbPipe;
                    }
                    else
                    {
                        pIUsbPipe->DeleteWdfObject();
                    }                      
                }
                else if ( pIUsbPipe->IsOutEndPoint() && (UsbdPipeTypeBulk == pIUsbPipe->GetType()) )
                {
                    m_pIUsbOutputPipe = pIUsbPipe;
                }
                else
                {
                    pIUsbPipe->DeleteWdfObject();
                }
    
                SAFE_RELEASE(pIUsbPipe); //release creation reference
            }
        }

        if (NULL == m_pIUsbInputPipe || NULL == m_pIUsbOutputPipe)
        {
            hr = E_UNEXPECTED;
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Input or output pipe not found, returning %!HRESULT!",
                        hr
                        );        
        }
    }

    SAFE_RELEASE(pIUsbTargetFactory);

    return hr;
}

HRESULT
CMyDevice::ConfigureUsbPipes(
    )
/*++

Routine Description:

    This routine retrieves the IDs for the bulk end points of the USB device.

Arguments:

    None

Return Value:

    HRESULT
--*/
{
    HRESULT                 hr = S_OK;
    LONG                    timeout;

    //
    // Set timeout policies for input/output pipes
    //

    if (SUCCEEDED(hr)) 
    {
        timeout = ENDPOINT_TIMEOUT;

        hr = m_pIUsbInputPipe->SetPipePolicy(PIPE_TRANSFER_TIMEOUT, 
                                             sizeof(timeout),
                                             &timeout);
        if (FAILED(hr))
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Unable to set timeout policy for input pipe %!HRESULT!",
                        hr
                        );
        }
    }
        
    if (SUCCEEDED(hr)) 
    {
        timeout = ENDPOINT_TIMEOUT;

        hr = m_pIUsbOutputPipe->SetPipePolicy(PIPE_TRANSFER_TIMEOUT,
                                             sizeof(timeout),
                                             &timeout);
        if (FAILED(hr)) 
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Unable to set timeout policy for output pipe %!HRESULT!",
                        hr
                        );
        }
    }

    return hr;
}

HRESULT
CMyDevice::IndicateDeviceReady(
    VOID
    )
/*++
 
  Routine Description:

    This method lights the period on the device's seven-segment display to 
    indicate that the driver's PrepareHardware method has completed.

  Arguments:

    None

  Return Value:

    Status

--*/
{
    SEVEN_SEGMENT display = {0};

    HRESULT hr;

    //
    // First read the contents of the seven segment display.
    //

    hr = GetSevenSegmentDisplay(&display);

    if (SUCCEEDED(hr)) 
    {
        display.Segments |= 0x08;

        hr = SetSevenSegmentDisplay(&display);
    }

    return hr;
}

HRESULT
CMyDevice::GetBarGraphDisplay(
    _In_ PBAR_GRAPH_STATE BarGraphState
    )
/*++
 
  Routine Description:

    This method synchronously retrieves the bar graph display information 
    from the OSR USB-FX2 device.  It uses the buffers in the FxRequest
    to hold the data it retrieves.

  Arguments:

    FxRequest - the request for the bar-graph info.

  Return Value:

    Status

--*/
{
    WINUSB_CONTROL_SETUP_PACKET setupPacket;

    ULONG bytesReturned;

    HRESULT hr = S_OK;

    //
    // Zero the contents of the buffer - the controller OR's in every
    // light that's set.
    //

    BarGraphState->BarsAsUChar = 0;

    //
    // Setup the control packet.
    //

    WINUSB_CONTROL_SETUP_PACKET_INIT( &setupPacket,
                                      BmRequestDeviceToHost,
                                      BmRequestToDevice,
                                      USBFX2LK_READ_BARGRAPH_DISPLAY,
                                      0,
                                      0 );

    //
    // Issue the request to WinUsb.
    //

    hr = SendControlTransferSynchronously(
                &(setupPacket.WinUsb),
                (PUCHAR) BarGraphState,
                sizeof(BAR_GRAPH_STATE),
                &bytesReturned
                );

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
CMyDevice::GetSevenSegmentDisplay(
    _In_ PSEVEN_SEGMENT SevenSegment
    )
/*++
 
  Routine Description:

    This method synchronously retrieves the bar graph display information 
    from the OSR USB-FX2 device.  It uses the buffers in the FxRequest
    to hold the data it retrieves.

  Arguments:

    FxRequest - the request for the bar-graph info.

  Return Value:

    Status

--*/
{
    WINUSB_CONTROL_SETUP_PACKET setupPacket;

    ULONG bytesReturned;

    HRESULT hr = S_OK;

    //
    // Zero the output buffer - the device will or in the bits for 
    // the lights that are set.
    //

    SevenSegment->Segments = 0;

    //
    // Setup the control packet.
    //

    WINUSB_CONTROL_SETUP_PACKET_INIT( &setupPacket,
                                      BmRequestDeviceToHost,
                                      BmRequestToDevice,
                                      USBFX2LK_READ_7SEGMENT_DISPLAY,
                                      0,
                                      0 );

    //
    // Issue the request to WinUsb.
    //

    hr = SendControlTransferSynchronously(
                &(setupPacket.WinUsb),
                (PUCHAR) SevenSegment,
                sizeof(SEVEN_SEGMENT),
                &bytesReturned
                );

    return hr;
}

HRESULT
CMyDevice::SetSevenSegmentDisplay(
    _In_ PSEVEN_SEGMENT SevenSegment
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
                                      USBFX2LK_SET_7SEGMENT_DISPLAY,
                                      0,
                                      0 );

    //
    // Issue the request to WinUsb.
    //

    hr = SendControlTransferSynchronously(
                &(setupPacket.WinUsb),
                (PUCHAR) SevenSegment,
                sizeof(SEVEN_SEGMENT),
                &bytesTransferred
                );

    return hr;
}

HRESULT
CMyDevice::ReadSwitchState(
    _In_ PSWITCH_STATE SwitchState
    )
/*++
 
  Routine Description:

    This method synchronously retrieves the bar graph display information 
    from the OSR USB-FX2 device.  It uses the buffers in the FxRequest
    to hold the data it retrieves.

  Arguments:

    FxRequest - the request for the bar-graph info.

  Return Value:

    Status

--*/
{
    WINUSB_CONTROL_SETUP_PACKET setupPacket;

    ULONG bytesReturned;

    HRESULT hr = S_OK;

    //
    // Zero the output buffer - the device will or in the bits for 
    // the lights that are set.
    //

    SwitchState->SwitchesAsUChar = 0;

    //
    // Setup the control packet.
    //

    WINUSB_CONTROL_SETUP_PACKET_INIT( &setupPacket,
                                      BmRequestDeviceToHost,
                                      BmRequestToDevice,
                                      USBFX2LK_READ_SWITCHES,
                                      0,
                                      0 );

    //
    // Issue the request to WinUsb.
    //

    hr = SendControlTransferSynchronously(
                &(setupPacket.WinUsb),
                (PUCHAR) SwitchState,
                sizeof(SWITCH_STATE),
                &bytesReturned
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

WDF_IO_TARGET_STATE
CMyDevice::GetTargetState(
    IWDFIoTarget * pTarget
    )
{
    IWDFIoTargetStateManagement * pStateMgmt = NULL;
    WDF_IO_TARGET_STATE state;

    HRESULT hrQI = pTarget->QueryInterface(IID_PPV_ARGS(&pStateMgmt));
    WUDF_TEST_DRIVER_ASSERT((SUCCEEDED(hrQI) && pStateMgmt));
    
    state = pStateMgmt->GetState();

    SAFE_RELEASE(pStateMgmt);
    
    return state;
}
    
VOID
CMyDevice::ServiceSwitchChangeQueue(
    _In_ SWITCH_STATE NewState,
    _In_ HRESULT CompletionStatus,
    _In_opt_ IWDFFile *SpecificFile
    )
/*++
 
  Routine Description:

    This method processes switch-state change notification requests as 
    part of reading the OSR device's interrupt pipe.  As each read completes
    this pulls all pending I/O off the switch change queue and completes
    each request with the current switch state.

  Arguments:

    NewState - the state of the switches

    CompletionStatus - all pending operations are completed with this status.

    SpecificFile - if provided only requests for this file object will get
                   completed.

  Return Value:

    None

--*/
{
    IWDFIoRequest *fxRequest;

    HRESULT enumHr = S_OK;

    do 
    {
        HRESULT hr;

        //
        // Get the next request.
        //

        if (NULL != SpecificFile)
        {
            enumHr = m_SwitchChangeQueue->RetrieveNextRequestByFileObject(
                                            SpecificFile,
                                            &fxRequest
                                            );
        }
        else
        {
            enumHr = m_SwitchChangeQueue->RetrieveNextRequest(&fxRequest);
        }

        //
        // if we got one then complete it.
        //

        if (SUCCEEDED(enumHr)) 
        {
            if (SUCCEEDED(CompletionStatus)) 
            {
                IWDFMemory *fxMemory;

                //
                // First copy the result to the request buffer.
                //

                fxRequest->GetOutputMemory(&fxMemory );

                hr = fxMemory->CopyFromBuffer(0, 
                                              &NewState, 
                                              sizeof(SWITCH_STATE));
                fxMemory->Release();
            }
            else 
            {
                hr = CompletionStatus;
            }

            //
            // Complete the request with the status of the copy (or the completion
            // status if that was an error).
            //

            if (SUCCEEDED(hr)) 
            {
                fxRequest->CompleteWithInformation(hr, sizeof(SWITCH_STATE));
            }
            else
            {
                fxRequest->Complete(hr);
            }

            fxRequest->Release();            
        }
    }
    while (SUCCEEDED(enumHr));
}

HRESULT
CMyDevice::SetPowerManagement(
    VOID
    )
/*++

  Routine Description:

    This method enables the idle and wake functionality
    using UMDF. UMDF has been set as the power policy
    owner (PPO) for the device stack and we are using power
    managed queues.

  Arguments:

    None

  Return Value:
    
    Status

--*/
{ 
    HRESULT hr;

    //
    // Enable USB selective suspend on the device.    
    // 
    
    hr = m_FxDevice->AssignS0IdleSettings( IdleUsbSelectiveSuspend,
                                PowerDeviceMaximum,
                                IDLE_TIMEOUT_IN_MSEC,
                                IdleAllowUserControl,
                                WdfUseDefault);                                                                                                   

    if (FAILED(hr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, 
                    TEST_TRACE_DEVICE, 
                    "%!FUNC! Unable to assign S0 idle settings for the device %!HRESULT!",
                    hr
                    );
    }

    //
    // Enable Sx wake settings
    //

    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->AssignSxWakeSettings( PowerDeviceMaximum,
                                    WakeAllowUserControl,
                                    WdfUseDefault);
                                    
        if (FAILED(hr))
        {
            TraceEvents(TRACE_LEVEL_ERROR, 
                        TEST_TRACE_DEVICE, 
                        "%!FUNC! Unable to set Sx Wake Settings for the device %!HRESULT!",
                        hr
                        );
        }
        
    }


    return hr;
}

HRESULT
CMyDevice::OnD0Entry(
    _In_ IWDFDevice*  pWdfDevice,
    _In_ WDF_POWER_DEVICE_STATE  previousState
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(previousState);

    // 
    // Start/Stop the I/O target if you support a continuous reader.
    // The rest of the I/O is fed through power managed queues. The queue 
    // itself will stop feeding I/O to targets (and will wait for any pending 
    // I/O to complete before going into low power state), hence targets 
    // don’t need to be stopped/started. The continuous reader I/O is outside
    // of power managed queues so we need to Stop the I/O target on D0Exit and
    //  start it on D0Entry. Please note that bulk pipe target doesn't need to 
    // be stopped/started because I/O submitted to this pipe comes from power 
    // managed I/O queue, which delivers I/O only in power on state.
    //

    m_pIoTargetInterruptPipeStateMgmt->Start();

    return S_OK;
}

HRESULT
CMyDevice::OnD0Exit(
    _In_ IWDFDevice*  pWdfDevice,
    _In_ WDF_POWER_DEVICE_STATE  previousState
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(previousState);

    //
    // Stop the I/O target always succeedes.
    //
    m_pIoTargetInterruptPipeStateMgmt->Stop(WdfIoTargetCancelSentIo);
    return S_OK;
}

void
CMyDevice::OnSurpriseRemoval(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return;
}

HRESULT
CMyDevice::OnQueryRemove(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

HRESULT
CMyDevice::OnQueryStop(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

//
// Self Managed Io Callbacks
//

VOID
CMyDevice::OnSelfManagedIoCleanup(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return;
}

VOID
CMyDevice::OnSelfManagedIoFlush(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);

    //
    // Complete every switch change operation with an error.
    //
    ServiceSwitchChangeQueue(m_SwitchState, 
                             HRESULT_FROM_WIN32(ERROR_DEVICE_REMOVED),
                             NULL);

    return;
}

HRESULT
CMyDevice::OnSelfManagedIoInit(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

HRESULT
CMyDevice::OnSelfManagedIoRestart(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

HRESULT
CMyDevice::OnSelfManagedIoStop(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

HRESULT
CMyDevice::OnSelfManagedIoSuspend(
    _In_ IWDFDevice*  pWdfDevice
    )
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}



HRESULT
CMyDevice::ConfigContReaderForInterruptEndPoint(
    VOID
    )
/*++

Routine Description:

    This routine configures a continuous reader on the
    interrupt endpoint. It's called from the PrepareHarware event.

Arguments:


Return Value:

    HRESULT value

--*/
{
    HRESULT hr, hrQI;
    IUsbTargetPipeContinuousReaderCallbackReadComplete *pOnCompletionCallback = NULL;
    IUsbTargetPipeContinuousReaderCallbackReadersFailed *pOnFailureCallback= NULL;
    IWDFUsbTargetPipe2 * pIUsbInterruptPipe2;

    hrQI = this->QueryInterface(IID_PPV_ARGS(&pOnCompletionCallback));
    WUDF_TEST_DRIVER_ASSERT((SUCCEEDED(hrQI) && pOnCompletionCallback));

    hrQI = this->QueryInterface(IID_PPV_ARGS(&pOnFailureCallback));
    WUDF_TEST_DRIVER_ASSERT((SUCCEEDED(hrQI) && pOnFailureCallback));

    hrQI = m_pIUsbInterruptPipe->QueryInterface(IID_PPV_ARGS(&pIUsbInterruptPipe2));
    WUDF_TEST_DRIVER_ASSERT((SUCCEEDED(hrQI) && pIUsbInterruptPipe2));

    //
    // Reader requests are not posted to the target automatically.
    // Driver must explictly call WdfIoTargetStart to kick start the
    // reader.  In this sample, it's done in D0Entry.
    // By defaut, framework queues two requests to the target
    // endpoint. Driver can configure up to 10 requests with the 
    // parameter CONCURRENT_READS
    //    
    hr = pIUsbInterruptPipe2->ConfigureContinuousReader( sizeof(m_SwitchStateBuffer), 
                                                          0,//header
                                                          0,//trailer
                                                          CONCURRENT_READS, 
                                                          NULL,
                                                          pOnCompletionCallback,
                                                          m_pIUsbInterruptPipe,
                                                          pOnFailureCallback
                                                          );
               
    if (FAILED(hr)) {
        TraceEvents(TRACE_LEVEL_ERROR, TEST_TRACE_DEVICE,
                    "OsrFxConfigContReaderForInterruptEndPoint failed %!HRESULT!",
                    hr);
    }

    SAFE_RELEASE(pOnCompletionCallback);
    SAFE_RELEASE(pOnFailureCallback);
    SAFE_RELEASE(pIUsbInterruptPipe2);

    return hr;
}


BOOL
CMyDevice::OnReaderFailure(
    IWDFUsbTargetPipe * pPipe,
    HRESULT hrCompletion
    )
{   
    UNREFERENCED_PARAMETER(pPipe);
    
    m_InterruptReadProblem = hrCompletion;
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TEST_TRACE_DEVICE, 
                "%!FUNC! Failure completed with %!HRESULT!",
                hrCompletion
                );
    
    ServiceSwitchChangeQueue(m_SwitchState, 
                             hrCompletion,
                             NULL);

    return TRUE;
}

VOID
CMyDevice::OnReaderCompletion(
    IWDFUsbTargetPipe * pPipe,
    IWDFMemory * pMemory,
    SIZE_T NumBytesTransferred,
    PVOID Context
    )
{        
    WUDF_TEST_DRIVER_ASSERT(pPipe ==  (IWDFUsbTargetPipe *)Context);

    //
    // Make sure that there is data in the read packet.  Depending on the device
    // specification, it is possible for it to return a 0 length read in
    // certain conditions.
    //

    if (NumBytesTransferred == 0) {
        TraceEvents(TRACE_LEVEL_INFORMATION, 
                    TEST_TRACE_DEVICE, 
                    "%!FUNC! Zero length read occured on the Interrupt Pipe's "
                    "Continuous Reader\n"
                    );
        return;
    }

    WUDF_TEST_DRIVER_ASSERT(NumBytesTransferred == sizeof(m_SwitchState));
    
    //
    // Get the switch state
    //
    
    PVOID pBuff = pMemory->GetDataBuffer(NULL);

    CopyMemory(&m_SwitchState, pBuff, sizeof(m_SwitchState));
    
        
    //
    // Satisfy application request for switch change notification
    //
    
    ServiceSwitchChangeQueue(m_SwitchState, 
                             S_OK,
                             NULL);

    //
    // Make sure that the request that got completed is the one that we reuse
    // Don't Delete the request because it gets reused
    //
}


HRESULT
CMyDevice::GetBusTypeGuid(
    VOID
    )
/*++
 
  Routine Description:

    This routine gets the device instance ID then invokes SetupDi to 
    retrieve the bus type guid for the device.  The bus type guid is 
    stored in object.

  Arguments:

    None

  Return Value:

    Status

--*/
{
    ULONG instanceIdCch = 0;
    PWSTR instanceId = NULL;

    HDEVINFO deviceInfoSet = NULL;
    SP_DEVINFO_DATA deviceInfo = {sizeof(SP_DEVINFO_DATA)};

    HRESULT hr;

    //
    // Retrieve the device instance ID.
    //

    hr = m_FxDevice->RetrieveDeviceInstanceId(NULL, &instanceIdCch);

    if (FAILED(hr))
    {
        goto Exit;
    }

    instanceId = new WCHAR[instanceIdCch];

    if (instanceId == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = m_FxDevice->RetrieveDeviceInstanceId(instanceId, &instanceIdCch);

    if (FAILED(hr))
    {
        goto Exit2;
    }

    //
    // Call SetupDI to open the device info.
    //

    deviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit2;
    }

    if (SetupDiOpenDeviceInfo(deviceInfoSet,
                              instanceId,
                              NULL,
                              0,
                              &deviceInfo) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit3;
    }

    if (SetupDiGetDeviceRegistryProperty(deviceInfoSet,
                                         &deviceInfo,
                                         SPDRP_BUSTYPEGUID,
                                         NULL,
                                         (PBYTE) &m_BusTypeGuid,
                                         sizeof(m_BusTypeGuid),
                                         NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit3;
    }

Exit3:
    SetupDiDestroyDeviceInfoList(deviceInfoSet);

Exit2:

    delete[] instanceId;

Exit:

    return hr;
}

HRESULT
CMyDevice::PlaybackFile(
    _In_ PFILE_PLAYBACK PlayInfo,
    _In_ IWDFIoRequest *FxRequest
    )
/*++
 
  Routine Description:

    This method impersonates the caller, opens the file and prints each 
    character to the seven segement display.

  Arguments:

    PlayInfo - the playback info from the request.
  
    FxRequest - the request (used for impersonation)

  Return Value:

    Status

--*/

{
    PLAYBACK_IMPERSONATION_CONTEXT context = {PlayInfo, NULL, S_OK};
    IWDFIoRequest2* fxRequest2;

    HRESULT hr;

    // Convert FxRequest to FxRequest2.  No error can occur here.
    FxRequest->QueryInterface(IID_PPV_ARGS(&fxRequest2));
    _Analysis_assume_(fxRequest2 != NULL);

    //
    // Impersonate and open the playback file.
    //

    hr = FxRequest->Impersonate(
                        SecurityImpersonation,
                        this->QueryIImpersonateCallback(),
                        &context
                        );
    if (FAILED(hr))
    {
        goto exit;
    }

    //
    // Release the reference that was added in QueryIImpersonateCallback()
    //
    this->Release();

    hr = context.Hr;

    if (FAILED(hr))
    {
        goto exit;
    }
    
    //
    // The impersonation callback succeeded - tell code analysis that the 
    // file handle is non-null
    //

    _Analysis_assume_(context.FileHandle != NULL);

    //
    // Read from the file one character at a time until we hit 
    // EOF or the request is cancelled.
    //

    do
    {
        UCHAR c;
        ULONG bytesRead;

        //
        // Check for cancellation.
        //

        if (fxRequest2->IsCanceled())
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        else
        {
            BOOL result;

            //
            // Read a character from the file and see if we can 
            // encode it on the display.
            //

            result = ReadFile(context.FileHandle,
                              &c,
                              sizeof(c),
                              &bytesRead,
                              NULL);

            if (result)
            {
                SEVEN_SEGMENT segment;
                BAR_GRAPH_STATE barGraph;

                if (bytesRead > 0)
                {
                    #pragma prefast(suppress:__WARNING_USING_UNINIT_VAR,"Above this->Release() method does not actually free 'this'")
                    if(EncodeSegmentValue(c, &segment) == true)
                    {
                        barGraph.BarsAsUChar = c;

                        SetSevenSegmentDisplay(&segment);
                        SetBarGraphDisplay(&barGraph);
                    }

                    Sleep(PlayInfo->Delay);
                }
                else
                {
                    hr = S_OK;
                    break;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

    } while(SUCCEEDED(hr));
    
    CloseHandle(context.FileHandle);

exit:

    fxRequest2->Release();
    return hr;
}

VOID
CMyDevice::OnImpersonate(
    _In_ PVOID Context
    )
/*++
 
  Routine Description:

    This routine handles the impersonation for the PLAY FILE I/O control.

  Arguments:

    Context - pointer to the impersonation context

  Return Value:

    None

--*/
{
    PPLAYBACK_IMPERSONATION_CONTEXT context;

    context = (PPLAYBACK_IMPERSONATION_CONTEXT) Context;

    context->FileHandle = CreateFile(context->PlaybackInfo->Path, 
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);

    if (context->FileHandle == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        context->Hr = HRESULT_FROM_WIN32(error);
    }
    else
    {
        context->Hr = S_OK;
    }

    return;
}

#define SS_LEFT (SS_TOP_LEFT | SS_BOTTOM_LEFT)
#define SS_RIGHT (SS_TOP_RIGHT | SS_BOTTOM_RIGHT)

bool
CMyDevice::EncodeSegmentValue(
    _In_ UCHAR Character,
    _Out_ SEVEN_SEGMENT *SevenSegment
    )
{
    UCHAR letterMap[] = {
        (SS_TOP | SS_BOTTOM_LEFT | SS_RIGHT | SS_CENTER | SS_BOTTOM),   // a
        (SS_LEFT | SS_CENTER | SS_BOTTOM | SS_BOTTOM_RIGHT),            // b
        (SS_CENTER | SS_BOTTOM_LEFT | SS_BOTTOM),                       // c
        (SS_BOTTOM_LEFT | SS_CENTER | SS_BOTTOM | SS_RIGHT),            // d
        (SS_LEFT | SS_TOP | SS_CENTER | SS_BOTTOM),                     // e
        (SS_LEFT | SS_TOP | SS_CENTER),                                 // f
        (SS_TOP | SS_TOP_LEFT | SS_CENTER | SS_BOTTOM | SS_RIGHT),      // g
        (SS_LEFT | SS_RIGHT | SS_CENTER),                               // h
        (SS_BOTTOM_LEFT),                                               // i
        (SS_BOTTOM | SS_RIGHT),                                         // j
        (SS_LEFT | SS_CENTER | SS_BOTTOM),                              // k
        (SS_LEFT | SS_BOTTOM),                                          // l
        (SS_LEFT | SS_TOP | SS_RIGHT),                                  // m
        (SS_BOTTOM_LEFT | SS_CENTER | SS_BOTTOM_RIGHT),                 // n
        (SS_BOTTOM_LEFT | SS_BOTTOM_RIGHT | SS_CENTER | SS_BOTTOM),     // o
        (SS_LEFT | SS_TOP | SS_CENTER | SS_TOP_RIGHT),                  // p
        (SS_TOP_LEFT | SS_TOP | SS_CENTER | SS_RIGHT),                  // q
        (SS_BOTTOM_LEFT | SS_CENTER),                                   // r
        (SS_TOP_LEFT | 
         SS_TOP | SS_CENTER | SS_BOTTOM | 
         SS_BOTTOM_RIGHT),                                              // s
        (SS_TOP | SS_RIGHT),                                            // t
        (SS_LEFT | SS_RIGHT | SS_BOTTOM),                               // u
        (SS_BOTTOM_LEFT | SS_BOTTOM | SS_BOTTOM_RIGHT),                 // v
        (SS_LEFT | SS_BOTTOM | SS_BOTTOM_RIGHT),                        // w
        (SS_LEFT | SS_CENTER | SS_RIGHT),                               // x
        (SS_TOP_LEFT | SS_CENTER | SS_RIGHT),                           // y 
        (SS_TOP_RIGHT | 
         SS_TOP | SS_CENTER | SS_BOTTOM | 
         SS_BOTTOM_LEFT),                                               // z
    };

    UCHAR numberMap[] = {
        (SS_LEFT | SS_TOP | SS_BOTTOM | SS_RIGHT | SS_DOT),             // 0
        (SS_RIGHT | SS_DOT),                                            // 1
        (SS_TOP | 
         SS_TOP_RIGHT | SS_CENTER | SS_BOTTOM_LEFT | 
         SS_BOTTOM | SS_DOT),                                           // 2
        (SS_TOP | SS_CENTER | SS_BOTTOM | SS_RIGHT | SS_DOT),           // 3
        (SS_TOP_LEFT | SS_CENTER | SS_RIGHT | SS_DOT),                  // 4
        (SS_TOP_LEFT | 
         SS_TOP | SS_CENTER | SS_BOTTOM | 
         SS_BOTTOM_RIGHT | SS_DOT),                                     // 5
        (SS_TOP | SS_CENTER | SS_BOTTOM | 
         SS_LEFT | SS_BOTTOM_RIGHT | SS_DOT),                           // 6
        (SS_TOP | SS_RIGHT | SS_DOT),                                   // 7
        (SS_TOP | SS_BOTTOM | SS_CENTER | 
         SS_LEFT | SS_RIGHT | SS_DOT),                                  // 8
        (SS_TOP_LEFT | SS_TOP | SS_CENTER | SS_RIGHT | SS_DOT),         // 9
    };

    if (((Character >= 'a') && (Character <= 'z')) || 
        ((Character >= 'A') && (Character <= 'Z')))
    {
        SevenSegment->Segments = letterMap[tolower(Character) - 'a'];
        return true;
    }
    else if ((Character >= '0') && (Character <= '9'))
    {
        SevenSegment->Segments = numberMap[Character - '0'];
        return true;
    }
    else
    {
        SevenSegment->Segments = 0;
        return false;
    }
}

