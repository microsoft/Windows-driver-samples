#pragma once

#define DEVICE_PROTOCOL_VALUE                 L"Sensor Protocol ver 1.00"
#define DEVICE_FIRMWARE_VERSION_VALUE         L"1.0.0.0"
#define DEVICE_POWER_LEVEL_VALUE              100
#define DEVICE_MODEL_VALUE                    L"RS232 Sensor"
#define DEVICE_FRIENDLY_NAME_VALUE            L"Parallax BS2 Sensor"
#define DEVICE_MANUFACTURER_VALUE             L"Windows Portable Devices Group"
#define DEVICE_SERIAL_NUMBER_VALUE            L"01234567890123-45676890123456"
#define DEVICE_SUPPORTS_NONCONSUMABLE_VALUE   FALSE

#define SENSOR_OBJECT_ID                      L"Sensor"
#define SENSOR_OBJECT_NAME_VALUE              L"Parallax Sensor"
#define COMPASS_SENSOR_OBJECT_ID              L"Compass"
#define COMPASS_SENSOR_OBJECT_NAME_VALUE      L"HM55B Compass Sensor"
#define PIR_SENSOR_OBJECT_ID                  L"PIR"
#define PIR_SENSOR_OBJECT_NAME_VALUE          L"Passive Infra-Red Sensor"
#define QTI_SENSOR_OBJECT_ID                  L"QTI"
#define QTI_SENSOR_OBJECT_NAME_VALUE          L"QTI Light Sensor"
#define FLEX_SENSOR_OBJECT_ID                 L"Flex"
#define FLEX_SENSOR_OBJECT_NAME_VALUE         L"Flex Force Sensor"
#define PING_SENSOR_OBJECT_ID                 L"Ping"
#define PING_SENSOR_OBJECT_NAME_VALUE         L"Ultrasonic Distance Sensor"
#define PIEZO_SENSOR_OBJECT_ID                L"Piezo"
#define PIEZO_SENSOR_OBJECT_NAME_VALUE        L"Piezo Vibration Sensor"
#define TEMP_SENSOR_OBJECT_ID                 L"TempHumidity"
#define TEMP_SENSOR_OBJECT_NAME_VALUE         L"Sensiron Temperature and Humidity Sensor"
#define MEMSIC_SENSOR_OBJECT_ID               L"Memsic"
#define MEMSIC_SENSOR_OBJECT_NAME_VALUE       L"Memsic Dual-Axis G-Force Sensor"
#define HITACHI_SENSOR_OBJECT_ID              L"Hitachi"
#define HITACHI_SENSOR_OBJECT_NAME_VALUE      L"Hitachi Tri-Axis G-Force Sensor"
// INSERT ID and NAME definitions for other sensors here!!


GUID GetObjectFormat(CAtlStringW strObjectID);
GUID GetObjectContentType(CAtlStringW strObjectID);
HRESULT AddSupportedPropertyKeys(_In_ LPCWSTR                        wszObjectID,
                                 _In_ IPortableDeviceKeyCollection*  pKeys);

VOID AddCommonPropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);
VOID AddDevicePropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);

VOID AddSensorPropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys); //Required for sensor props

class WpdObjectProperties
{
public:

    WpdObjectProperties();
    virtual ~WpdObjectProperties();

    HRESULT Initialize(_In_ WpdBaseDriver* pBaseDriver);

    HRESULT DispatchWpdMessage(_In_ REFPROPERTYKEY         Command,
                               _In_ IPortableDeviceValues* pParams,
                               _In_ IPortableDeviceValues* pResults);

    HRESULT OnGetSupportedProperties(_In_ IPortableDeviceValues*  pParams,
                                     _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetPropertyValues(_In_ IPortableDeviceValues*  pParams,
                                _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetAllPropertyValues(_In_ IPortableDeviceValues*  pParams,
                                   _In_ IPortableDeviceValues*  pResults);

    HRESULT OnSetPropertyValues(_In_ IPortableDeviceValues*  pParams,
                                _In_ IPortableDeviceValues*  pResults);

    HRESULT OnGetPropertyAttributes(_In_ IPortableDeviceValues*  pParams,
                                    _In_ IPortableDeviceValues*  pResults);

    HRESULT OnDeleteProperties(_In_ IPortableDeviceValues*  pParams,
                               _In_ IPortableDeviceValues*  pResults);

    VOID SetSensorReading(LONGLONG llNewReading);

    LONGLONG GetSensorReading();

    VOID SetUpdateInterval(DWORD dwNewInterval);

    DWORD GetUpdateInterval();

    HRESULT SendUpdateIntervalToDevice(DWORD dwNewInterval);

    BOOL IsValidUpdateInterval(DWORD dwInterval);

private:

    HRESULT GetPropertyValuesForObject(_In_ LPCWSTR                        wszObjectID,
                                       _In_ IPortableDeviceKeyCollection*  pKeys,
                                       _In_ IPortableDeviceValues*         pValues);

    HRESULT GetPropertyAttributesForObject(_In_ LPCWSTR                wszObjectID,
                                           _In_ REFPROPERTYKEY         Key,
                                           _In_ IPortableDeviceValues* pAttributes);


private:
    WpdBaseDriver*           m_pBaseDriver;

    // This critical section protects the sensor reading from concurrent access
    // by the application (which reads it) and the read request callback (which updates it) 
    CComAutoCriticalSection  m_SensorReadingCriticalSection;
    LONGLONG                 m_llSensorReading;

    // A critical section is not needed for the update interval because it is accessed
    // by the client application, and set/get requests from the application arrive serially 
    // through the sequential WDF queue. 
    DWORD                    m_dwUpdateInterval;
};
