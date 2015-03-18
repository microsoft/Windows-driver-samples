/*++

Copyright (C) Microsoft Corporation, All Rights Reserved.

Module Name:

    Device.cpp

Abstract:

    This module contains the implementation of the UMDF VirtualSerial sample 
    driver's device callback object.

    The VirtualSerial sample device does very little.  It does not implement 
    either of the PNP interfaces so once the device is setup, it won't ever get
    any callbacks until the device is removed.

Environment:

   Windows User-Mode Driver Framework (WUDF)

--*/

#include "internal.h"
#include "device.tmh"

HRESULT
CMyDevice::CreateInstance(
    _In_ IWDFDriver *FxDriver,
    _In_ IWDFDeviceInitialize * FxDeviceInit,
    _Out_ PCMyDevice *Device
    )
/*++

  Routine Description:

    This method creates and initializs an instance of the VirtualSerial driver's
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
    IWDFDevice *fxDevice;
    HRESULT hr;

    //
    // Configure things like the locking model before we go to create our
    // partner device.
    //
    FxDeviceInit->SetLockingConstraint(WdfDeviceLevel);

    //
    // Create a new FX device object and assign the new callback object to
    // handle any device level events that occur.
    //

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
    IWDFPropertyStoreFactory *pPropertyStoreFactory = NULL;
    IWDFNamedPropertyStore2 * pHardwarePropertyStore = NULL;
    IWDFNamedPropertyStore2 * pLegacyHardwarePropertyStore = NULL;
    WDF_PROPERTY_STORE_ROOT RootSpecifier;
    PROPVARIANT comPortPV;
    WCHAR portName[] = L"PortName";
    size_t comPortSuffixCch = 0;
    WCHAR *comPortFullName = NULL;
    size_t comPortFullNameCch = 0;
    WCHAR *pdoName = NULL;
    PCMyQueue defaultQueue;

#ifdef _FAKE_MODEM
    //
    // Compiled as fake modem
    //
    LPGUID pGuid = (LPGUID) &GUID_DEVINTERFACE_MODEM;
#else
    //
    // Compiled as virtual serial port
    //
    LPGUID pGuid = (LPGUID) &GUID_DEVINTERFACE_COMPORT;
#endif

    HRESULT hr;

    PropVariantInit(&comPortPV);

    //
    // Create device interface
    //
    hr = m_FxDevice->CreateDeviceInterface(pGuid,
                                           NULL);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Cannot create device interface (%!GUID!)",
            pGuid
            );

        goto Exit;
    }

    hr = m_FxDevice->AssignDeviceInterfaceState(pGuid,
                                                NULL,
                                                TRUE);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Cannot enable device interface (%!GUID!)",
            pGuid
            );

        goto Exit;
    }

    //
    // Create Symbolic Link
    //

    //
    // First we need to read the COM number from the registry
    //

    hr = m_FxDevice->QueryInterface(IID_PPV_ARGS(&pPropertyStoreFactory));

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: QI for IWDFPropertyStoreFactory failed"
            );

        goto Exit;
    }

    RootSpecifier.LengthCb = sizeof(WDF_PROPERTY_STORE_ROOT);
    RootSpecifier.RootClass = WdfPropertyStoreRootClassHardwareKey;
    RootSpecifier.Qualifier.HardwareKey.ServiceName =
                                           WDF_PROPERTY_STORE_HARDWARE_KEY_ROOT;

    hr = pPropertyStoreFactory->RetrieveDevicePropertyStore(
                                           &RootSpecifier,
                                           WdfPropertyStoreNormal,
                                           KEY_QUERY_VALUE,
                                           NULL,
                                           &pHardwarePropertyStore,
                                           NULL
                                           );

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Failed to retrieve device hardware key root"
            );
        goto Exit;
    }

    hr = pHardwarePropertyStore->GetNamedValue(portName, &comPortPV);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Failed to read value %ws",
            portName
            );
        goto Exit;
    }

    //
    // Then we need to construct the COM port name
    //
    hr = StringCchLength(comPortPV.pwszVal,
                         STRSAFE_MAX_CCH,
                         &comPortSuffixCch);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Overflow while calculating COM port suffix length"
            );
        goto Exit;
    }

    hr = SizeTAdd(ARRAYSIZE(SYMBOLIC_LINK_NAME_PREFIX),
                            comPortSuffixCch,
                            &comPortFullNameCch);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Overflow while calculating symbolic link length"
            );
        goto Exit;
    }

    comPortFullName = (WCHAR*) new WCHAR[comPortFullNameCch];

    if (NULL == comPortFullName)
    {
        hr = E_OUTOFMEMORY;
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Unable to allocate memory for full port name"
            );
        goto Exit;
    }

    hr = StringCchPrintf(comPortFullName,
                         comPortFullNameCch,
                         L"%ws%ws",
                         SYMBOLIC_LINK_NAME_PREFIX,
                         comPortPV.pwszVal);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Cannot create full name for COM port"
            );
        goto Exit;
    }

    //
    // Finally we create the symbolic link
    //
    hr = m_FxDevice->CreateSymbolicLink(comPortFullName);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Cannot create symbolic link %ws",
            comPortFullName
            );
        goto Exit;
    }

    //
    // Write the com name to the legacy hardware key
    //
    hr = GetPdoName(&pdoName);

    if (FAILED(hr) || (pdoName == NULL))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Cannot retrieve PDO name"
            );
        goto Exit;
    }

    //
    // Write the name to the legacy hardware key
    //
    RootSpecifier.LengthCb = sizeof(WDF_PROPERTY_STORE_ROOT);
    RootSpecifier.RootClass = WdfPropertyStoreRootClassLegacyHardwareKey;
    RootSpecifier.Qualifier.LegacyHardwareKey.LegacyMapName = L"SERIALCOMM";

    hr = pPropertyStoreFactory->RetrieveDevicePropertyStore(
                                           &RootSpecifier,
                                           WdfPropertyStoreCreateVolatile,
                                           KEY_SET_VALUE,
                                           NULL,
                                           &pLegacyHardwarePropertyStore,
                                           NULL
                                           );

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Failed to retrieve device legacy hardware key"
            );
        goto Exit;
    }

    hr = pLegacyHardwarePropertyStore->SetNamedValue(pdoName,
                                                     &comPortPV);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Failed to write device name to legacy hardware key"
            );
        goto Exit;
    }

    m_CreatedLegacyHardwareKey = TRUE;
    m_LegacyHardwarePropertyStore = pLegacyHardwarePropertyStore;
    m_PdoName = pdoName;
    
    //
    // Create and configure the queues
    //
    hr = CMyQueue::CreateInstance(this, m_FxDevice, &defaultQueue);

    if (FAILED(hr))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Failed to create default queue"
            );        
        goto Exit;
    }

    defaultQueue->Release();    
    
    
Exit:

    if (m_CreatedLegacyHardwareKey == FALSE)
    {
        //
        // If the legacy hardware key has been written, then the cleanup
        // will happen, when the device is unloaded
        // 
        SAFE_RELEASE(pLegacyHardwarePropertyStore);
        if (pdoName != NULL)
        {
            delete[] pdoName;
        }        
    }
    
    SAFE_RELEASE(pHardwarePropertyStore);
    SAFE_RELEASE(pPropertyStoreFactory);

    PropVariantClear(&comPortPV);

    if (comPortFullName != NULL)
    {
        delete[] comPortFullName;
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

  Arguments:

    InterfaceId - the interface being requested

    Object - a location to store the interface pointer if successful

  Return Value:

    S_OK or E_NOINTERFACE

--*/
{
    HRESULT hr;

    if(IsEqualIID(InterfaceId, __uuidof(IObjectCleanup))) 
    {    
        *Object = QueryIObjectCleanup();
        hr = S_OK;  
    }
    else
    {
        hr = CUnknown::QueryInterface(InterfaceId, Object);
    }
    
    return hr;
}

VOID 
CMyDevice::OnCleanup(
    IWDFObject*  pWdfObject
)
{
    UNREFERENCED_PARAMETER(pWdfObject);
    
    if ((m_CreatedLegacyHardwareKey == TRUE) && 
        (m_LegacyHardwarePropertyStore != NULL))
    {
        m_LegacyHardwarePropertyStore->DeleteNamedValue(m_PdoName);
        SAFE_RELEASE(m_LegacyHardwarePropertyStore);
        delete[] m_PdoName;
    }    
}

HRESULT
CMyDevice::GetPdoName(
    _Out_ LPWSTR *PdoName
    )
{
    HRESULT hr = S_OK;
    WCHAR *devInstId = NULL;
    ULONG devInstIdLen;
    HDEVINFO hDevInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA devInfoData;
    WCHAR *pdoName = NULL;
    DWORD pdoNameCb = 0;

    WUDF_TEST_DRIVER_ASSERT(m_FxDevice);

    //
    // First let's get the device instance ID
    //
    devInstIdLen = 0;
    hr = m_FxDevice->RetrieveDeviceInstanceId(NULL, &devInstIdLen);
    if (SUCCEEDED(hr))
    {
        devInstId = (WCHAR*) new WCHAR[devInstIdLen];
        if (NULL == devInstId)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = m_FxDevice->RetrieveDeviceInstanceId(devInstId, &devInstIdLen);
    }

    //
    // Now use the SetupDiXxx functions to get a handle to the device's 
    // hardware key
    //
    if (SUCCEEDED(hr))
    {
        //
        // Create a new device information set
        //
        hDevInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
        if (INVALID_HANDLE_VALUE == hDevInfoSet)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Add our device to this device information set
        //
        ZeroMemory(&devInfoData, sizeof(devInfoData));
        devInfoData.cbSize = sizeof(devInfoData);
        if (!SetupDiOpenDeviceInfo(hDevInfoSet,
                                   devInstId,
                                   NULL,
                                   0,
                                   &devInfoData))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the length of the PDO name
        //
        if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet,
                                              &devInfoData,
                                              SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                              NULL,
                                              (PBYTE)pdoName,
                                              0,
                                              &pdoNameCb))
        {
            //
            // The only reason for this call is to get the length of the
            // buffer. The only non acceptable reason for failure is, if we
            // asked for invalid data.
            //
            if (GetLastError() == ERROR_INVALID_DATA)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    
    //
    // Get the PDO name                                 
    //
    if (SUCCEEDED(hr))
    {    
        pdoName = (WCHAR *)new BYTE[pdoNameCb];
        
        if (pdoName == NULL)
        {
            hr = E_OUTOFMEMORY;    
        }
    }
        
    if (SUCCEEDED(hr))
    {
        if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet,
                                              &devInfoData,
                                              SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                              NULL,
                                              (PBYTE)pdoName,
                                              pdoNameCb,
                                              NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }                   
                                         
    }

    if (SUCCEEDED(hr))
    {
        *PdoName = pdoName;
    }

    //
    // clean up to be done regardless of success or failure
    //
    if (NULL != devInstId)
    {
        delete[] devInstId;
    }

    if (INVALID_HANDLE_VALUE != hDevInfoSet)
    {
        SetupDiDestroyDeviceInfoList(hDevInfoSet);
    }

    //
    // clean up to be done in case of failure only
    //
    if (FAILED(hr))
    {
        if (NULL != pdoName)
        {
            delete[] pdoName;
            pdoName = NULL;
        }
    }

    return hr;
}
