#include <stdafx.h>

class CApoDeviceTests;

class CAPODevice
{
    public:
        CAPODevice
        (
            IMMDevice          *pIEndpoint,
            LPCWSTR             pszClassId,
            LPCWSTR             pszEndpoint,
            IPropertyStore     *pIStoreDevice,
            IPropertyStore     *pIStoreFx,
            LPARAM              apoType,
            LPCWSTR             apoName,
            LPCWSTR             pszAttachedDevice,
            BOOL                bProxyAPO
        );
        CAPODevice
        (
            IAudioProcessingObject     *pIAPO,
            LPCWSTR                     pszClassId
        );
        ~CAPODevice(void);
        HRESULT InitializeAPO(IAudioProcessingObject *pIAPO);
        DWORD   GetDeviceType() { return (DWORD)(m_lpType); }
        BOOL    IsValid() { return (m_fValid); }
        void    GetClsID(GUID *pgClsID) { *pgClsID = m_gClsID; }
        HRESULT GetAPOInterfaces
        (
            IAudioProcessingObject                **pIAPO,
            IAudioProcessingObjectRT              **pIAPORT,
            IAudioProcessingObjectConfiguration   **pIAPOConfig
        );
        IUnknown *GetObject() { return ((IUnknown*)m_pIUnknown.get()); }
        IMMDevice *GetEndpoint() { return m_pIEndpoint.get(); }
        IPropertyStore *GetEndpointStore() { return m_pIEPStore.get(); }
        IPropertyStore *GetFxStore() { return m_pIFXStore.get(); }
        IMMDeviceCollection *GetDeviceCollection() { return m_pIDevCollection.get(); }
        UINT GetSoftwareIoDeviceInCollection() { return m_nSoftwareIoDeviceInCollection; }
        UINT GetSoftwareIoConnectorIndex() { return m_nSoftwareIoConnectorIndex; }
        PAPO_REG_PROPERTIES GetProperties() { return (m_pRegProps); }
        LPSTR GetAttachedDevice() { return (LPSTR)(m_szAttachedDevice.get()); }
        BOOL IsProxyApo() { return m_bProxyAPO; }

        BOOL    m_fSelected;      // if TRUE, this device will be selected
        wil::unique_cotaskmem_string                    m_szEndpoint;
        wil::unique_cotaskmem_string                    m_sApoName;
        wil::unique_cotaskmem_string                    m_sApoTypeName;

    private:
        BOOL                                            m_fValid;
        GUID                                            m_gClsID;
        wil::com_ptr_nothrow<IUnknown>                  m_pIUnknown;
        wil::com_ptr_nothrow<IAudioProcessingObject>    m_pIAPO;
        PAPO_REG_PROPERTIES                             m_pRegProps;
        
        //  These are only used for SysFx
        wil::com_ptr_nothrow<IMMDevice>                 m_pIEndpoint;
        wil::unique_cotaskmem_string                    m_szAttachedDevice;
        wil::com_ptr_nothrow<IPropertyStore>            m_pIEPStore;
        wil::com_ptr_nothrow<IPropertyStore>            m_pIFXStore;
        wil::com_ptr_nothrow<IMMDeviceCollection>       m_pIDevCollection;
        UINT                                            m_nSoftwareIoDeviceInCollection;
        UINT                                            m_nSoftwareIoConnectorIndex;
        BOOL                                            m_bProxyAPO;
        wil::unique_cotaskmem_string                    m_szPnPId;   // a string that represents the PnPId (or other description) of the device under test
        LPARAM                                          m_lpType;         // flags that will be intersected with test case flags
};


// ----------------------------------------------------------
// Stores a list of APO devices on an endpoint pDevice
// The list is populated using AddSysFxDevices and accessed
// using m_DeviceList
// ----------------------------------------------------------
class CAPODeviceList
{
public:
    CAPODeviceList();
    ~CAPODeviceList(VOID);

    HRESULT Initialize(wil::com_ptr_nothrow<IMMDevice> pDevice, LPWSTR deviceName);
    HRESULT AddSysFxDevices();

protected:
    friend class CApoDeviceTests;

    std::list<std::unique_ptr<CAPODevice>>  m_DeviceList;
    wil::com_ptr_nothrow<IMMDevice>         m_spDevice;
    WCHAR                                   m_deviceName[MAX_PATH] = { '\0' };
};

class CApoDeviceTests : public WEX::TestClass<CApoDeviceTests>
{
    CApoDeviceTests(){};
    ~CApoDeviceTests(){};

    BEGIN_TEST_CLASS(CApoDeviceTests)
        START_APPVERIFIFER
        TEST_CLASS_PROPERTY(L"Owner", L"auddev")
        TEST_CLASS_PROPERTY(L"TestClassification", L"Feature")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"audiodg.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"audioenginebaseapop.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"devicetopologyp.h")
        TEST_CLASS_PROPERTY(L"RunAs", L"Elevated")
        TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        TEST_CLASS_PROPERTY(L"Ignore", L"false")
        END_APPVERIFIER
    END_TEST_CLASS()

  public:
    TEST_METHOD_SETUP(setUpMethod);
    TEST_METHOD_CLEANUP(tearDownMethod);

  protected:
    BEGIN_TEST_METHOD(TestAPOInitialize)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"ED37B03B-F091-4C8D-9EDD-FE7EC803BC8E")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"5B0F542E-719A-4949-A9DF-F8BBAC0E9912")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Verify APO Initializes - TestAPOInitialize")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"1")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"5")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestAPOInitialize")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TestCustomFormatSupport)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"571BD021-64F5-4CD1-9BA4-8ABD0857025F")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"EB313E34-CD9D-410B-9C6F-E7032C74F4D1")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Verify All Formats on Device are Valid - TestCustomFormatSupport")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"1")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"5")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestCustomFormatSupport")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TestValidFrameCount)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"B9E9F61A-2882-4256-ABA8-D57A46E54B76")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"ABE4D880-7F1A-434C-A7B9-248FFA96D2CF")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Verify Frame Count is Valid - TestValidFrameCount")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"1")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"5")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestValidFrameCount")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TestAPODataStreaming)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"D327B019-99E4-41BD-BF66-AF1A92801DAC")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"9EBF7FC3-7ADB-49A1-B0E0-C7501841E05A")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Verify APO Streams Data - TestAPODataStreaming")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"1")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"5")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestAPODataStreaming")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TestActivateDeactivate)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"D1C21041-9080-4990-B6A2-5F718649A3F2")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"F0556EDA-4757-409F-AE89-C5C1B0E31A84")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Verify Proper APO Activation and Deactivation - TestActivateDeactivate")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"1")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"5")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestActivateDeactivate")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()

  private:
    std::list<std::unique_ptr<CAPODeviceList>> m_testDeviceWrapperList;
};

BEGIN_MODULE()
    MODULE_PROPERTY(L"EtwLogger:WPRProfileFile", L"Audio-Tests.wprp")
    MODULE_PROPERTY(L"EtwLogger:WPRProfile", L"MultimediaCategory.Verbose.File")
END_MODULE()
