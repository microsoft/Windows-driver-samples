/*++

Copyright (c) 1990-2010  Microsoft Corporation

Module Name:

    Device.cpp

Abstract:

    This is a simple UMDF device driver that consumes GPIO pins for I/O and interrupt.


Environment:

    UMDF

--*/

//
// Inlcude the below set of headers to get the CM_RESOURCE_ definitions
//


#include "internal.h"

DEFINE_GUID (GUID_DEVINTERFACE_ECHO,
    0xcdc35b6e, 0xbe4, 0x4936, 0xbf, 0x5f, 0x55, 0x37, 0x38, 0xa, 0x7c, 0x1a);
// {CDC35B6E-0BE4-4936-BF5F-5537380A7C1A}


HRESULT
CSimdevice::CreateInstance(
    _In_ IWDFDriver *FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit,
    _Out_ PCSimdevice *Device
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
    PCSimdevice device;
    HRESULT hr;

    //
    // Allocate a new instance of the device class.
    //

    device = new CSimdevice();

    if (NULL == device) {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //

    hr = device->Initialize(FxDriver, FxDeviceInit);

    if (SUCCEEDED(hr)) {
        *Device = device;
    }
    else {
        device->Release();
    }

    return hr;
}

HRESULT
CSimdevice::Initialize(
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
    IWDFDevice *fxDevice;
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
    // Create a new FX device object and assign the new callback object to
    // handle any device level events that occur.
    //

    //
    // Set retrieval mode to direct I/O. This needs to be done before the call
    // to CreateDevice.
    //
    
    FxDeviceInit->QueryInterface(IID_PPV_ARGS(&fxDeviceInit2));

    if (fxDeviceInit2 == NULL) {
        hr = E_FAIL;
        return hr;
    }
    
    fxDeviceInit2->SetIoTypePreference(WdfDeviceIoBufferRetrievalDeferred,
                                       WdfDeviceIoDirect,
                                       WdfDeviceIoDirect);

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

    //
    // If that succeeded then set our FxDevice member variable.
    //

    if (SUCCEEDED(hr)) {
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
CSimdevice::Configure(
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
    PCSimdeviceQueue defaultQueue;

    HRESULT hr;

    hr = CSimdeviceQueue::CreateInstance(m_FxDevice, &defaultQueue);

    if (FAILED(hr)) {
        return hr;
    }

    hr = defaultQueue->Configure();

    if (SUCCEEDED(hr)) {
        //
        // In case of success store defaultQueue in our member
        // The reference is transferred to m_DefaultQueue
        //

        m_Queue = defaultQueue;
    }
    else {
        //
        // In case of failure release the reference
        //

        defaultQueue->Release();
    }

    if (SUCCEEDED(hr)) {
        hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_ECHO,
                                               NULL);
    }

    return hr;
}

HRESULT
CSimdevice::QueryInterface(
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
    } else if (IsEqualIID(InterfaceId, __uuidof(IPnpCallbackHardware2))) {
        *Object = QueryIPnpCallbackHardware2();
        hr = S_OK;
    } else if (IsEqualIID(InterfaceId, __uuidof(IPnpCallback))) {
        *Object = QueryIPnpCallback();
        hr = S_OK;
    } else {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

HRESULT
CSimdevice::OnSelfManagedIoInit(
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
                                   CSimdeviceQueue::CompletionThread,  // Thread Func
                                   (LPVOID)m_Queue,             // Arg to Thread Func is Queue
                                   0,                           // Creation Flags
                                   NULL );                      // Don't need the Thread Id.

    if (m_ThreadHandle == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

void
CSimdevice::OnSelfManagedIoCleanup(
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


HRESULT
CSimdevice::OnPrepareHardware(
    _In_ IWDFDevice3 * pWdfDevice,
    _In_ IWDFCmResourceList * pWdfResourcesRaw,
    _In_ IWDFCmResourceList * pWdfResourcesTranslated
    )
/*++

Routine Description:

    This routine is called by WUDF to initialize hardware resources (e.g. interrupts,
    IO resources)
    
Arguments:

    pWdfDevice - pointer to an IWDFDevice object for the device
    
    pWdfResourcesRaw - Supplies a pointer to a collection of framework resource
        objects. This collection identifies the raw (bus-relative) hardware
        resources that have been assigned to the device.

    pWdfResourcesTranslated - Supplies a pointer to a collection of framework
        resource objects. This collection identifies the translated
        (system-physical) hardware resources that have been assigned to the
        device. The resources appear from the CPU's point of view.

Return Value:

    HRESULT

--*/
{

    ULONG               i;
    HRESULT             hr = S_OK;
    ULONG ConnectionCount = 0;
    BOOLEAN fInterruptFound = FALSE;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorTranslated;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorRaw;
    
    //
    // Parse the resource list and save the resource information.
    //
    
    for (i=0; i < pWdfResourcesTranslated->GetCount(); i++) {
 
        DescriptorTranslated = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)pWdfResourcesTranslated->GetDescriptor(i);
        DescriptorRaw = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)pWdfResourcesRaw->GetDescriptor(i);
 
        if(DescriptorTranslated == NULL) {
            return  E_FAIL;
        }
 
        switch (DescriptorTranslated->Type) {
 
        //
        // One or more GPIO IO resources are expected. In this implementation of the sample
        // driver, however, only the first IO resource will be used (see OnD0Entry).
        //
 
        case CmResourceTypeConnection:
 
            //
            // Check against expected connection type for a GPIO IO descriptor
            //
 
            if ((DescriptorTranslated->u.Connection.Class ==
                 CM_RESOURCE_CONNECTION_CLASS_GPIO) &&
                (DescriptorTranslated->u.Connection.Type ==
                 CM_RESOURCE_CONNECTION_TYPE_GPIO_IO)) {
 
                if (ConnectionCount >= MAX_CONNECTIONS) {
                    break;
                }

                //
                // Store GPIO IO resource connection ID in the device extension
                //
                
                this->GetDeviceExtension()->ConnectionId[ConnectionCount].LowPart =
                    DescriptorTranslated->u.Connection.IdLowPart;
 
                this->GetDeviceExtension()->ConnectionId[ConnectionCount].HighPart =
                    DescriptorTranslated->u.Connection.IdHighPart;
                }
 
            break;
 
           case CmResourceTypeInterrupt:
 
                //
                // Connect the first interrupt resource we find
                //
                
                if (fInterruptFound == FALSE) {
                    hr = this->SimdeviceConnectInterrupt(
                        pWdfDevice,
                        DescriptorRaw,
                        DescriptorTranslated);
 
                    if (SUCCEEDED(hr)) {
                        fInterruptFound = TRUE;
                    }
                }
            default:
                
                //
                // Ignore all other descriptors
                //
                
                break;
        }
    }
 
    return hr;
    
}

HRESULT
CSimdevice::OnReleaseHardware(
    _In_ IWDFDevice3 * pWdfDevice,
    _In_ IWDFCmResourceList * pWdfResourcesTranslated
    )
/*++


Routine Description:

    This method is called by WUDF to uninitialize the hardware.

Parameters:

    pWdfDevice - pointer to an IWDFDevice object for the device
    
    pWdfResourcesTranslated - pointer to the translated resource list

Return Values:
    status

--*/
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(pWdfResourcesTranslated);

    return S_OK;
}

HRESULT
CSimdevice::OnD0Entry(
    _In_ IWDFDevice*  pWdfDevice,
    _In_ WDF_POWER_DEVICE_STATE  previousState
    )
    
/*++

Routine Description:

This method is called after a new device enters the system

Parameters:

    pWdfDevice    - pointer to a device object
    
    previousState - previous WDF power state

Return Values:
    status
    
--*/
{

    PDEVICE_EXTENSION DevExt;
    HRESULT hr = S_OK;
    BYTE Data;
    NTSTATUS Status;
    NTSTATUS Status1;
    WCHAR ReadStringBuffer[100];
    WCHAR WriteStringBuffer[100];

    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(previousState);

    DevExt = GetDeviceExtension();

    Status = StringCbPrintfW(&ReadStringBuffer[0],
                                sizeof(ReadStringBuffer),
                                L"\\\\.\\RESOURCE_HUB\\%0*I64x",
                                (size_t)(sizeof(LARGE_INTEGER) * 2),
                                DevExt->ConnectionId[0].QuadPart);

    Status = StringCbPrintfW(&WriteStringBuffer[0],
                                sizeof(WriteStringBuffer),
                                L"\\\\.\\RESOURCE_HUB\\%0*I64x",
                                (size_t)(sizeof(LARGE_INTEGER) * 2),
                                DevExt->ConnectionId[0].QuadPart);

    //
    // Connect the first GPIO IO descriptor we find (in OnPrepareHardware) for read and write operations
    //
    
    Data = 0xDB;   
    Status = STATUS_SUCCESS;
    Status1 = TestReadWrite(pWdfDevice, &WriteStringBuffer[0], FALSE, &Data, sizeof(Data), NULL);
    if (!NT_SUCCESS(Status1)) {
        Status = Status1;
    }

    Data = 0;
    Status1 = TestReadWrite(pWdfDevice, &ReadStringBuffer[0], TRUE, &Data, sizeof(Data), NULL);
    if (!NT_SUCCESS(Status1)) {
        Status = Status1;
    }

    return hr;
}

HRESULT
CSimdevice::OnD0Exit(
    _In_ IWDFDevice*  pWdfDevice,
    _In_ WDF_POWER_DEVICE_STATE  TargetState
    )

/*++

Routine Description:

This method is called when a device exit D0

Parameters:

    pWdfDevice    pointer to a device object
    
    TargetState - target D-state

Return Values:
    status
    
--*/

{

    UNREFERENCED_PARAMETER(pWdfDevice);
    UNREFERENCED_PARAMETER(TargetState);
    
    return S_OK;
}

HRESULT
CSimdevice::TestReadWrite(
    _In_ IWDFDevice* pWdfDevice,
    _In_ PWSTR RequestString,
    _In_ BOOLEAN ReadOperation,
    _Inout_updates_bytes_(Size) UCHAR *Data,
    _In_ ULONG Size,
    _Inout_opt_ IWDFRemoteTarget *IoTargetOut
    )

/*++

Routine Description:

    This is a utility routine to test read or write on a set of GPIO pins.

Arguments:

    pWdfDevice - Supplies a pointer to the framework device object.

    RequestString - Supplies a pointer to the unicode string to be opened.

    ReadOperation - Supplies a boolean that identifies whether read (TRUE) or
        write (FALSE) should be performed.

    Data - Supplies a pointer containing the buffer that should be read from
        or written to.

    Size - Supplies the size of the data buffer in bytes.

    IoTargetOut - Supplies a pointer that receives the IOTARGET created by
        UMDF.

Return Value:

    HRESULT

--*/

{

    IWDFMemory *pOutputMemory = NULL;
    IWDFMemory *pInputMemory = NULL;
    IRequestCallbackRequestCompletion *pICallback = NULL;
    IWDFIoRequest *pIoRequest = NULL;
    IWDFRemoteTarget *pRemoteTarget = NULL;
    UMDF_IO_TARGET_OPEN_PARAMS OpenParams;
    DWORD DesiredAccess;
    HRESULT hr = S_OK;
    IWDFRequestCompletionParams * FxComplParams = NULL;
    IWDFDriver *pWdfDriver = NULL;

    UNREFERENCED_PARAMETER(IoTargetOut);

    if (ReadOperation != FALSE) {
        DesiredAccess = FILE_GENERIC_READ;

    } else {
        DesiredAccess = FILE_GENERIC_WRITE;
    }

    //
    // QueryIUnknown references the IUnknown interface that it returns
    // (which is the same as referencing the CMyRemoteTarget).  We pass that
    // to the various Create* calls, which take their own reference if
    // everything works.
    //

    IUnknown * unknown = this->QueryIUnknown();


    IWDFDevice2 *pWdfDevice2 = NULL;
    hr = pWdfDevice->QueryInterface(IID_PPV_ARGS(&pWdfDevice2));

    // Create the IoTarget

    if (SUCCEEDED(hr)) {
        hr = pWdfDevice2->CreateRemoteTarget(
                 unknown,
                 NULL,
                 &pRemoteTarget);
    }

    //
    // Determine whether the request is a read or write
    //
    
    OpenParams.dwCreationDisposition = OPEN_EXISTING;
    OpenParams.dwFlagsAndAttributes = FILE_FLAG_OVERLAPPED;
    OpenParams.dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;


    if (SUCCEEDED(hr)) {
        hr = pRemoteTarget->OpenFileByName(RequestString,
                                           DesiredAccess,
                                           &OpenParams);
    }

    //
    //Create a new IO request
    //
    
    if (SUCCEEDED(hr)) {
        hr = pWdfDevice->CreateRequest(NULL, pWdfDevice, &pIoRequest);
    }

    if (SUCCEEDED(hr)) {
        hr = this->QueryInterface(__uuidof(IRequestCallbackRequestCompletion), (PVOID*)&pICallback);

        //
        //Set completion callback
        //
        
        if (SUCCEEDED(hr)){
            pIoRequest->SetCompletionCallback(pICallback, NULL);
        }

        pWdfDevice->GetDriver(&pWdfDriver);

        hr = pWdfDriver->CreatePreallocatedWdfMemory(Data,
                                                     Size,
                                                     NULL,          // no object event callback
                                                     pIoRequest,    // request object as parent
                                                     &pInputMemory);

         hr = pWdfDriver->CreatePreallocatedWdfMemory(Data,
                                                      Size,
                                                      NULL,          // no object event callback
                                                      pIoRequest,    // request object as parent
                                                      &pOutputMemory);

        //
        //Format IO request
        //
        
        if (ReadOperation != FALSE){
            if (SUCCEEDED(hr)) {
                hr = pRemoteTarget->FormatRequestForIoctl(pIoRequest,
                                                          IOCTL_GPIO_READ_PINS,
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          pOutputMemory,
                                                          NULL);
            }
        }
        else {
            if (SUCCEEDED(hr)) {
                hr = pRemoteTarget->FormatRequestForIoctl(pIoRequest,
                                                          IOCTL_GPIO_WRITE_PINS,
                                                          NULL,
                                                          pInputMemory,
                                                          NULL,
                                                          pOutputMemory,
                                                          NULL);
            }
        }

        //
        //Send down the request
        //
        if (SUCCEEDED(hr)) {
            hr = pIoRequest->Send(pRemoteTarget,
                                  WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
                                  0); //No timeout
        }

        if (SUCCEEDED(hr)) {
            
            //
            //Get IWDFRequestCompletionParams interface and then get completion status.
            //
            
            pIoRequest->GetCompletionParams(&FxComplParams);
            hr = FxComplParams->GetCompletionStatus();
        }

        if(FAILED(hr)) {
            pIoRequest->DeleteWdfObject();
            pIoRequest = NULL;
        }
    }

    //
    // Clean-up
    //
    
    if (pWdfDevice2 != NULL) {
        SAFE_RELEASE(pWdfDevice2);
    }

    if (pWdfDriver != NULL) {
        SAFE_RELEASE(pWdfDriver);
    }

    if (pRemoteTarget != NULL) {
        pRemoteTarget->Close();
    }

    return hr;
}


void
CSimdevice::OnSurpriseRemoval(
    _In_ IWDFDevice*  pWdfDevice
    )

/*++

Routine Description:

This method is called when a device is surprise removed

Parameters:

    pWdfDevice    pointer to a device object

Return Values:
    None
    
--*/

{
    UNREFERENCED_PARAMETER(pWdfDevice);
}


HRESULT
CSimdevice::OnQueryRemove(
    _In_ IWDFDevice*  pWdfDevice
    )
/*++

Routine Description:

This method is called when a device processes the query remove IRP

Parameters:

    pWdfDevice    pointer to a device object

Return Values:
    HRESULT
    
--*/


{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

HRESULT
CSimdevice::OnQueryStop(
    _In_ IWDFDevice*  pWdfDevice
    )
/*++

Routine Description:

This method is called when a device processes the query stop IRP

Parameters:

    pWdfDevice    pointer to a device object

Return Values:
    HRESULT
    
--*/
{
    UNREFERENCED_PARAMETER(pWdfDevice);
    return S_OK;
}

HRESULT 
CSimdevice::SimdeviceConnectInterrupt(
    _In_     IWDFDevice* pWdfDevice,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR RawResource,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedResource
    )

/*++

Routine Description:

    This is a utility routine to create an interrupt descriptor from the interrupt resource
    acquired from OnPrepareHardware and connect the device ISR routine.
Arguments:

    pWdfDevice - Supplies a pointer to the framework device object.
    
    pWdfResourcesRaw - Supplies a pointer to a collection of framework resource
        objects. This collection identifies the raw (bus-relative) hardware
        resources that have been assigned to the device.

    pWdfResourcesTranslated - Supplies a pointer to a collection of framework
        resource objects. This collection identifies the translated
        (system-physical) hardware resources that have been assigned to the
        device. The resources appear from the CPU's point of view.

Return Value:

    None.

--*/

{
    
    IWDFDevice3 * pIWDFDevice3 = NULL;
    IWDFInterrupt * spInterrupt = NULL;
    HRESULT hr = S_OK;

    if (pWdfDevice == NULL) {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        hr = pWdfDevice->QueryInterface(IID_PPV_ARGS(&pIWDFDevice3));
    }

    if (SUCCEEDED(hr)) {
        
        //
        // Create interrupt
        //
        
        WUDF_INTERRUPT_CONFIG config;
        WUDF_INTERRUPT_CONFIG_INIT(
            &config,
            &CSimdevice::OnInterruptIsr,
            &CSimdevice::OnInterruptWorkItem);
        
        config.InterruptRaw = RawResource;
        config.InterruptTranslated = TranslatedResource;

        hr = pIWDFDevice3->CreateInterrupt(&config, &spInterrupt);
    }

    //
    //   Associate the device context with the interrupt
    //
    
    if (SUCCEEDED(hr)) {
        hr = spInterrupt->AssignContext(NULL, (void*)this);
    }

    return hr;
}

BOOLEAN
CSimdevice::OnInterruptIsr(
    _In_ IWDFInterrupt* pInterrupt,
    _In_ ULONG MessageID,
    _In_ ULONG Reserved
    )

/* ++

Routine Description:

  This method is called when an interrupt occurs.  It determines if the
  driver owns the interrupt and queues a work item to defer processing
  of the data.

  Arguments:
  
      pInterrupt - pointer to the interrupt object
      MessageID - interrupt message ID
      Reserved - 

Return Values:
      TRUE if interrupt recognized, else FALSE.

-- */

{

    UNREFERENCED_PARAMETER(MessageID);
    UNREFERENCED_PARAMETER(Reserved);

    IWDFDevice* pWdfDevice = NULL;
    CSimdevice* pMyDevice = NULL;
    HRESULT hr;

    hr = pInterrupt->RetrieveContext((void**)&pMyDevice);

    if (FAILED(hr)) {        
        pWdfDevice = pInterrupt->GetDevice();
        pWdfDevice->SetPnpState(WdfPnpStateFailed, WdfTrue);
        pWdfDevice->CommitPnpState();
    }

    //
    //  The sample driver always returns TRUE (e.g. claiming the interrupt)
    //  from its ISR. In reality, the driver needs to do whatever necessary to
    //  quiesce the interrupt before claiming the interrupt. 
    //  If additional work needs to be done at , schedule a work item (as we do here).
    //

    pInterrupt->QueueWorkItemForIsr();

    return TRUE;
}

VOID
CSimdevice::OnInterruptWorkItem(
    _In_ IWDFInterrupt* pInterrupt,
    _In_ IWDFObject* AssociatedObject
    )
    
/* ++

Routine Description:

   This method is called on behalf of an interrupt to defer processing.  
   It retrieves latest data and posts it.
 
Arguments:
   pInterrupt - pointer to the interrupt object
   AssociatedObject - pointer to the associated object
   
Return Values:
       None.

--*/
{

    UNREFERENCED_PARAMETER(AssociatedObject);

    IWDFDevice* pWdfDevice = NULL;
    CSimdevice* pMyDevice;
    HRESULT hr;

    hr = pInterrupt->RetrieveContext((void**)&pMyDevice);

    if (FAILED(hr)) {        
        pWdfDevice = pInterrupt->GetDevice();
        pWdfDevice->SetPnpState(WdfPnpStateFailed, WdfTrue);
        pWdfDevice->CommitPnpState();
    }

    
    //
    //  The sample driver does nothing in the work item routine. The real driver
    //  can add code here to accomplish all that is required to complete the serving
    //  of the interrupt
    //

    return;
}


