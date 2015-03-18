#pragma once

#define DEVICE_PROTOCOL_VALUE                             L"Multi-Transport Protocol ver 1.00"
#define DEVICE_FIRMWARE_VERSION_VALUE                     L"1.0.0.0"
#define DEVICE_POWER_LEVEL_VALUE                          100
#define DEVICE_MODEL_VALUE                                L"Multi-Transport"
#define DEVICE_FRIENDLY_NAME_VALUE                        L"Multi-Transport Hello World!"
#define DEVICE_MANUFACTURER_VALUE                         L"Windows Portable Devices Group"
#define DEVICE_SERIAL_NUMBER_VALUE                        L"01234567890123-45676890123456"
#define DEVICE_SUPPORTS_NONCONSUMABLE_VALUE               TRUE

#define STORAGE_OBJECT_ID                                 L"123ABC"
#define STORAGE_CAPACITY_VALUE                            1024 * 1024
#define STORAGE_FREE_SPACE_IN_BYTES_VALUE                 STORAGE_CAPACITY_VALUE
#define STORAGE_SERIAL_NUMBER_VALUE                       L"98765432109876-54321098765432"
#define STORAGE_OBJECT_NAME_VALUE                         L"Internal Memory"
#define STORAGE_FILE_SYSTEM_TYPE_VALUE                    L"FAT32"
#define STORAGE_DESCRIPTION_VALUE                         L"Hello World! Memory Storage System"

#define DOCUMENTS_FOLDER_OBJECT_ID                        L"XYZ456"
#define DOCUMENTS_FOLDER_OBJECT_NAME_VALUE                L"Documents Folder"
#define DOCUMENTS_FOLDER_OBJECT_ORIGINAL_FILE_NAME_VALUE  L"Documents"

#define README_FILE_OBJECT_ID                             L"6543210"
#define README_FILE_OBJECT_NAME_VALUE                     L"Sample ReadMe Text File"
#define README_FILE_OBJECT_ORIGINAL_FILE_NAME_VALUE       L"ReadMe.txt"
#define README_FILE_OBJECT_CONTENTS                       "Hello World!\r\nThis is a text file transferred from the WPD Multi-Transport Hello World sample driver.\r\n"

ULONGLONG GetObjectSize(_In_ LPCWSTR strObjectID);
GUID GetObjectFormat(_In_ LPCWSTR strObjectID);
GUID GetObjectContentType(_In_ LPCWSTR strObjectID);
HRESULT AddSupportedPropertyKeys(_In_ LPCWSTR                        wszObjectID,
                                 _In_ IPortableDeviceKeyCollection*  pKeys);

VOID AddCommonPropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);
VOID AddDevicePropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);
VOID AddStoragePropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);
VOID AddFilePropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);
VOID AddFolderPropertyKeys(_In_ IPortableDeviceKeyCollection* pKeys);

class WpdObjectProperties
{
public:
    WpdObjectProperties();
    virtual ~WpdObjectProperties();

    HRESULT Initialize();

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

private:
    HRESULT GetPropertyValuesForObject(_In_ LPCWSTR                        wszObjectID,
                                       _In_ IPortableDeviceKeyCollection*  pKeys,
                                       _In_ IPortableDeviceValues*         pValues);

    HRESULT GetPropertyAttributesForObject(_In_ LPCWSTR                wszObjectID,
                                           _In_ REFPROPERTYKEY         Key,
                                           _In_ IPortableDeviceValues* pAttributes);
};
