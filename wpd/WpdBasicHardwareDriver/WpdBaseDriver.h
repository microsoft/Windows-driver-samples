#pragma once

typedef struct tagSENSOR_INFO{
    unsigned char   deviceID;
    unsigned short  dataLength;
} SENSOR_INFO;

class WpdBaseDriver :
    public IUnknown
{
public:
    WpdBaseDriver();
    virtual ~WpdBaseDriver();

    HRESULT Initialize(_In_ IWDFDevice *pDevice);
    void    Uninitialize();

    HRESULT DispatchWpdMessage(_In_ IPortableDeviceValues* pParams,
                               _In_ IPortableDeviceValues* pResults);

    HRESULT ProcessReadData(_In_reads_(cbData) BYTE* pData, size_t cbData);

    RS232Target* GetRS232Target();

public: // IUnknown
    ULONG __stdcall AddRef();

    _At_(this, __drv_freesMem(Mem)) 
    ULONG __stdcall Release();

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);

private:
    HRESULT OnGetObjectIDsFromPersistentUniqueIDs(_In_ IPortableDeviceValues* pParams,
                                                  _In_ IPortableDeviceValues* pResults);

    HRESULT PostSensorReadingEvent(LONGLONG llSensorData, DWORD dwUpdateInterval);

public:
        
    enum SensorType{
      UNKNOWN,  // Unknown
      COMPASS,  // Compass
      SENSIRON, // Temp/humidity sensor 
      FLEX,     // Flexiforce sensor
      PING,     // Ultrasonic ping 
      PIR,      // Passive infrared
      MEMSIC,   // 2-axis accelerometer 
      QTI,      // Light sensor
      PIEZO,    // Vibration Sensor 
      HITACHI,  // 3-axis accelerometer
    };

    SensorType   m_SensorType;  // enum value specifying sensor type

private:
    WpdObjectEnumerator            m_ObjectEnum;
    WpdObjectProperties            m_ObjectProperties;
    WpdCapabilities                m_Capabilities;

    ULONG                          m_cRef;
    RS232Connection                m_Connection;
    RS232Target                    m_Target;

    CComPtr<IWDFDevice>            m_pWDFDevice;
    CComPtr<IWpdSerializer>        m_pWpdSerializer;
    CComPtr<IPortableDeviceValues> m_pEventParams;
};

