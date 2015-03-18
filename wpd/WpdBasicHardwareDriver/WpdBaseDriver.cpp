#include "stdafx.h"

#include "WpdBaseDriver.tmh"


WpdBaseDriver::WpdBaseDriver() :
    m_cRef(1)
{
}

WpdBaseDriver::~WpdBaseDriver()
{
}

ULONG __stdcall WpdBaseDriver::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

_At_(this, __drv_freesMem(Mem)) 
ULONG __stdcall WpdBaseDriver::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return ulRefCount;
}

HRESULT __stdcall WpdBaseDriver::QueryInterface(
    REFIID riid,
    void** ppv)
{
    HRESULT hr = S_OK;

    if(riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

RS232Target* WpdBaseDriver::GetRS232Target()
{
    return &m_Target;
}

HRESULT WpdBaseDriver::DispatchWpdMessage(_In_ IPortableDeviceValues* pParams,
                                          _In_ IPortableDeviceValues* pResults)
{
    HRESULT     hr                  = S_OK;
    GUID        guidCommandCategory = {0};
    DWORD       dwCommandID         = 0;
    PROPERTYKEY CommandKey          = WPD_PROPERTY_NULL;

    if (hr == S_OK)
    {
        hr = pParams->GetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, &guidCommandCategory);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_COMMAND_CATEGORY from input parameters");
    }

    if (hr == S_OK)
    {
        hr = pParams->GetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, &dwCommandID);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_COMMAND_ID from input parameters");
    }

    // If WPD_PROPERTY_COMMON_COMMAND_CATEGORY or WPD_PROPERTY_COMMON_COMMAND_ID could not be extracted
    // properly then we should return E_INVALIDARG to the client.
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_COMMAND_CATEGORY or WPD_PROPERTY_COMMON_COMMAND_ID from input parameters");
    }

    if (hr == S_OK)
    {
        CommandKey.fmtid = guidCommandCategory;
        CommandKey.pid   = dwCommandID;

        if (CommandKey.fmtid == WPD_CATEGORY_OBJECT_ENUMERATION)
        {
            hr = m_ObjectEnum.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_OBJECT_PROPERTIES)
        {
            hr = m_ObjectProperties.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (CommandKey.fmtid == WPD_CATEGORY_CAPABILITIES)
        {
            hr = m_Capabilities.DispatchWpdMessage(CommandKey, pParams, pResults);
        }
        else if (IsEqualPropertyKey(CommandKey, WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS))
        {
            hr = OnGetObjectIDsFromPersistentUniqueIDs(pParams, pResults);
        }
        else
        {
            hr = E_NOTIMPL;
            CHECK_HR(hr, "Unknown command %ws.%d received",CComBSTR(CommandKey.fmtid), CommandKey.pid);
        }
    }

    HRESULT hrTemp = pResults->SetErrorValue(WPD_PROPERTY_COMMON_HRESULT, hr);
    CHECK_HR(hrTemp, ("Failed to set WPD_PROPERTY_COMMON_HRESULT"));

    // Set to a success code, to indicate that the message was received.
    // the return code for the actual command's results is stored in the
    // WPD_PROPERTY_COMMON_HRESULT property.
    hr = S_OK;

    return hr;
}

/**
 * This method is called to initialize the driver object.
 * This is where the driver establishes a connection with the
 * COM port, creates the IoTarget to forward read/write requests to, and
 * starts the thread which monitors these events.
 */
HRESULT WpdBaseDriver::Initialize(_In_ IWDFDevice* pDevice)
{
    HRESULT hr    = S_OK;
    HANDLE  hPort = NULL;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DRIVER, "%!FUNC! Entry");

    if (pDevice == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "A NULL IWDFDevice parameter was received.");
        return hr;
    }

    // Save the WDF device instance
    m_pWDFDevice = pDevice;
    m_ObjectProperties.Initialize(this);
    m_ObjectEnum.Initialize(this);

    // Initialize the Wpd Serializer for posting events
    hr = CoCreateInstance(CLSID_WpdSerializer,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IWpdSerializer,
                          (VOID**)&m_pWpdSerializer);

    CHECK_HR(hr, "Failed to CoCreate the Wpd Serializer.");


    // Initialize a handle to the RS232 connection 
    hr = m_Connection.Connect(COM_PORT_NAME, &hPort);
    CHECK_HR(hr, "Failed to connect to port: %ws", COM_PORT_NAME);

    // Initialize the IoTarget to wrap the opened handle
    if (hr == S_OK)
    {
        hr = m_Target.Create(this, pDevice, hPort);
        CHECK_HR(hr, "Failed to create the port I/O target.");
    }

    return hr;
}

/**
 * This method is called to uninitialize the driver object.
 * This is where the driver disables the connection with the
 * COM port and performs the necessary cleanup.
 */
void WpdBaseDriver::Uninitialize()
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DRIVER, "%!FUNC! Entry");

    m_Target.Delete();
    m_Connection.Disconnect();

    m_pWDFDevice = NULL;
}

/**
 * This method is called to extract the sensor data and interval prop from the raw serial buffer
 * and to post a new reading PnP event if valid data is received
 */
HRESULT WpdBaseDriver::ProcessReadData(_In_reads_(cbData) BYTE* pData, size_t cbData)
{
    HRESULT     hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    LONGLONG    llSensorReading  = 0; // was originally DWORD but the accelerometers required addt'l bytes
    DWORD       dwUpdateInterval = 0;

    // Parse the serial data
    CHAR  szInterval[INTERVAL_DATA_LENGTH + 1]  = {0};  // last byte is always null
    CHAR* szReading                             = NULL; // buffer containing the reading

    const SENSOR_INFO c_SensorInfoTable [] = {
      {'1', COMPASS_DATA_LENGTH},   // Compass
      {'2', SENSIRON_DATA_LENGTH},  // Temp/humidity sensor
      {'3', FLEX_DATA_LENGTH},      // Flexiforce sensor
      {'4', PING_DATA_LENGTH},      // Ultrasonic ping
      {'5', PIR_DATA_LENGTH},       // Passive infrared
      {'6', MEMSIC_DATA_LENGTH},    // 2-axis accelerometer
      {'7', QTI_DATA_LENGTH},       // Light sensor
      {'8', PIEZO_DATA_LENGTH},     // Vibration Sensor
      {'9', HITACHI_DATA_LENGTH},   // 3-axis accelerometer
    };

    // Allocate the necessary bytes for collecting the sensor data.
    // We'll examine the DEVICE_ID field of the batched serial data and allocate
    // the necessary bytes accordingly...
    // This is also where we set the sensor identifier and related properties).

    if (cbData > DEVICE_ID)
    {
        switch (pData[DEVICE_ID]){
            case '1':
                m_SensorType = COMPASS;
                szReading = (CHAR *)malloc(COMPASS_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '2':
                m_SensorType = SENSIRON;
                szReading = (CHAR *)malloc(SENSIRON_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '3':
                m_SensorType = FLEX;
                szReading = (CHAR *)malloc(FLEX_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '4':
                m_SensorType = PING;
                szReading = (CHAR *)malloc(PING_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '5':
                m_SensorType = PIR;
                szReading = (CHAR *)malloc(PIR_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '6':
                m_SensorType = MEMSIC;
                szReading = (CHAR *)malloc(MEMSIC_AMOUNT_TO_READ + 1); // last byte is always null
                hr = S_OK;
                break;
            case '7':
                m_SensorType = QTI;
                szReading = (CHAR *)malloc(QTI_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '8':
                m_SensorType = PIEZO;
                szReading = (CHAR *)malloc(PIEZO_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            case '9':
                m_SensorType = HITACHI;
                szReading = (CHAR *)malloc(HITACHI_DATA_LENGTH + 1); // last byte is always null
                hr = S_OK;
                break;
            default:
                break;
        }
    }
    
    if ((hr == S_OK) && (szReading == NULL))
    {
        hr = E_OUTOFMEMORY;
    }
    
    // Ensure we have sufficient input buffer size, and, if we do
    // process the data for the given sensor. 
    if (hr == S_OK && (cbData >= (INTERVAL_DATA_LENGTH + MIN_DATA_LENGTH)))
    {
        for (int i=0; i<ARRAYSIZE(c_SensorInfoTable); i++)
        {
            if (pData[DEVICE_ID] == c_SensorInfoTable[i].deviceID)
            {
                memcpy((void*)szReading, (void*)pData, c_SensorInfoTable[i].dataLength);
                szReading[c_SensorInfoTable[i].dataLength] = '\0';  // ensure null termination
                llSensorReading = _atoi64(szReading);
                hr = (errno == ERANGE) ? HRESULT_FROM_WIN32(ERROR_INVALID_DATA) : S_OK;
                if (hr == S_OK)
                {
                    // Process the Interval data
                    memcpy((void*)szInterval, (void*)(pData+c_SensorInfoTable[i].dataLength), INTERVAL_DATA_LENGTH);
                    szInterval[INTERVAL_DATA_LENGTH] = '\0';  // ensure null termination
                    dwUpdateInterval = atoi(szInterval);
                    hr = (errno == ERANGE) ? HRESULT_FROM_WIN32(ERROR_INVALID_DATA) : S_OK;
                }
                break;
            }
        }
    }

    if (hr == S_OK)
    {
        // Post the Updated PnP event so applications receive the notification
        hr = PostSensorReadingEvent(llSensorReading, dwUpdateInterval);
        CHECK_HR(hr, "Failed to post a sensor reading event");
        
        // If the cached interval has not yet been initialized, set to the value returned from the device
        if (m_ObjectProperties.GetUpdateInterval() == 0)
        {
            m_ObjectProperties.SetUpdateInterval(dwUpdateInterval);
        }

        // Update the sensor reading property for the sensor functional object
        m_ObjectProperties.SetSensorReading(llSensorReading);
    }

    if (szReading)
    {
        // Free the bytes allocated with malloc()
        free(szReading);
    }

    return hr;
}

/**
 * This method is called to send a device event with 
 * the new sensor data
 */
HRESULT WpdBaseDriver::PostSensorReadingEvent(
    LONGLONG    llSensorData,
    DWORD       dwUpdateInterval)
{
    HRESULT     hr       = S_OK;
    BYTE*       pBuffer  = NULL;
    DWORD       cbBuffer = 0;

    CComPtr<IPortableDeviceValues> pEventParams;

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_FLAG_DRIVER, "%!FUNC! Reading: %I64d, Interval: %d", llSensorData, dwUpdateInterval);

    // Create the event parameters collection if it doesn't exist
    if (m_pEventParams == NULL)
    {
        hr = CoCreateInstance(CLSID_PortableDeviceValues,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDeviceValues,
                              (VOID**)&m_pEventParams);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDeviceValues");
    }

    if (hr == S_OK)
    {   
        // Initialize the event parameters
        m_pEventParams->Clear();

        // Populate the event parameters
        hr = m_pEventParams->SetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, EVENT_SENSOR_READING_UPDATED);
        CHECK_HR(hr, "Failed to set the WPD_EVENT_PARAMETER_EVENT_ID");
    }
    
    if (hr == S_OK)
    {
        //hr = m_pEventParams->SetUnsignedIntegerValue(SENSOR_READING, dwTemperatureData);
        hr = m_pEventParams->SetUnsignedLargeIntegerValue(SENSOR_READING, llSensorData);
        CHECK_HR(hr, "Failed to set the sensor reading");
    }

    if (hr == S_OK)
    {
        hr = m_pEventParams->SetUnsignedIntegerValue(SENSOR_UPDATE_INTERVAL, dwUpdateInterval);
        CHECK_HR(hr, "Failed to set the sensor update interval");
    }

    if (hr == S_OK)
    {
        // Create a buffer with the serialized parameters
        hr = m_pWpdSerializer->GetBufferFromIPortableDeviceValues(m_pEventParams, &pBuffer, &cbBuffer);
        CHECK_HR(hr, "Failed to get buffer from IPortableDeviceValues");
    }

    // Send the event
    if (hr == S_OK && pBuffer != NULL)
    {
        hr = m_pWDFDevice->PostEvent(WPD_EVENT_NOTIFICATION, WdfEventBroadcast, pBuffer, cbBuffer);
        CHECK_HR(hr, "Failed to post the WPD broadcast event");
    }

    // Free the memory
    CoTaskMemFree(pBuffer);
    pBuffer = NULL;

    return hr;
}

/**
 *  This method is called when we receive a WPD_COMMAND_COMMON_GET_OBJECT_IDS_FROM_PERSISTENT_UNIQUE_IDS
 *  command.
 *
 *  The parameters sent to us are:
 *  - WPD_PROPERTY_COMMON_PERSISTENT_UNIQUE_IDS: Contains an IPortableDevicePropVariantCollection of VT_LPWSTR,
 *    indicating the PersistentUniqueIDs.
 *
 *  The driver should:
 *  - Iterate through the PersistentUniqueIDs, and convert to a currently valid object id.
 *    This object ID list should be returned as an IPortableDevicePropVariantCollection of VT_LPWSTR
 *    in WPD_PROPERTY_COMMON_OBJECT_IDS.
 *    Order is implicit, i.e. the first element in the Persistent Unique ID list corresponds to the
 *    to the first element of the ObjectID list and so on.
 *
 *    For those elements where an existing ObjectID could not be found (e.g. the
 *    object is no longer present on the device), the element will contain the
 *    empty string (L"").
 */
HRESULT WpdBaseDriver::OnGetObjectIDsFromPersistentUniqueIDs(
    _In_ IPortableDeviceValues* pParams,
    _In_ IPortableDeviceValues* pResults)
{

    HRESULT hr      = S_OK;
    DWORD   dwCount = 0;
    CComPtr<IPortableDevicePropVariantCollection> pPersistentIDs;
    CComPtr<IPortableDevicePropVariantCollection> pObjectIDs;

    if((pParams    == NULL) ||
       (pResults   == NULL))
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    // Get the list of Persistent IDs
    if (hr == S_OK)
    {
        hr = pParams->GetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_COMMON_PERSISTENT_UNIQUE_IDS, &pPersistentIDs);
        CHECK_HR(hr, "Failed to get WPD_PROPERTY_COMMON_PERSISTENT_UNIQUE_IDS");
    }

    // Create the collection to hold the ObjectIDs
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IPortableDevicePropVariantCollection,
                              (VOID**) &pObjectIDs);
        CHECK_HR(hr, "Failed to CoCreate CLSID_PortableDevicePropVariantCollection");
    }

    // Iterate through the persistent ID list and add the equivalent object ID for each element.
    if (hr == S_OK)
    {
        hr = pPersistentIDs->GetCount(&dwCount);
        CHECK_HR(hr, "Failed to get count from persistent ID collection");

        if (hr == S_OK)
        {
            DWORD       dwIndex        = 0;
            PROPVARIANT pvPersistentID = {0};
            PROPVARIANT pvObjectID     = {0};

            PropVariantInit(&pvPersistentID);
            PropVariantInit(&pvObjectID);

            for(dwIndex = 0; dwIndex < dwCount; dwIndex++)
            {
                pvObjectID.vt = VT_LPWSTR;
                hr = pPersistentIDs->GetAt(dwIndex, &pvPersistentID);
                CHECK_HR(hr, "Failed to get persistent ID at index %d", dwIndex);

                // Since our persistent unique identifier are identical to our object
                // identifiers, we just return it back to the caller.
                if (hr == S_OK)
                {
                    pvObjectID.pwszVal = AtlAllocTaskWideString(pvPersistentID.pwszVal);
                }

                if (hr == S_OK)
                {
                    hr = pObjectIDs->Add(&pvObjectID);
                    CHECK_HR(hr, "Failed to add next Object ID");
                }

                PropVariantClear(&pvPersistentID);
                PropVariantClear(&pvObjectID);

                if(FAILED(hr))
                {
                    break;
                }
            }
        }
    }

    if (hr == S_OK)
    {
        hr = pResults->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_COMMON_OBJECT_IDS, pObjectIDs);
        CHECK_HR(hr, "Failed to set WPD_PROPERTY_COMMON_OBJECT_IDS");
    }

    return hr;
}
