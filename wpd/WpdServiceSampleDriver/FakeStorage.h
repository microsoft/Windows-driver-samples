#pragma once

/**
 * This class represents an abstraction of a storage content object.
 * Driver implementors should replace this with their own
 * device I/O classes/libraries.
 */

#define STORAGE_OBJECT_ID                                 L"123ABC"
#define STORAGE_CAPACITY_VALUE                            1024 * 1024
#define STORAGE_FREE_SPACE_IN_BYTES_VALUE                 STORAGE_CAPACITY_VALUE
#define STORAGE_SERIAL_NUMBER_VALUE                       L"98765432109876-54321098765432"
#define STORAGE_OBJECT_NAME_VALUE                         L"Internal Memory"
#define STORAGE_FILE_SYSTEM_TYPE_VALUE                    L"FAT32"
#define STORAGE_DESCRIPTION_VALUE                         L"Phone Memory Storage System"
#define STORAGE_CONTAINER_FUNCTIONAL_OBJECT_ID            WPD_DEVICE_OBJECT_ID
#define STORAGE_TYPE                                      WPD_STORAGE_TYPE_FIXED_ROM

class FakeStorage : public FakeContent
{
public:
    FakeStorage()
    {
        ObjectID                    = STORAGE_OBJECT_ID;
        PersistentUniqueID          = STORAGE_OBJECT_ID;
        ParentID                    = WPD_DEVICE_OBJECT_ID;
        Name                        = STORAGE_OBJECT_NAME_VALUE;
        ContentType                 = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;
        Format                      = WPD_OBJECT_FORMAT_UNSPECIFIED;
        FunctionalCategory          = WPD_FUNCTIONAL_CATEGORY_STORAGE;
        ContainerFunctionalObjectID = WPD_DEVICE_OBJECT_ID;
        ParentPersistentUniqueID    = WPD_DEVICE_OBJECT_ID;

        Description                 = STORAGE_DESCRIPTION_VALUE;
        Capacity                    = STORAGE_CAPACITY_VALUE;
        FreeSpace                   = STORAGE_CAPACITY_VALUE;
        SerialNumber                = STORAGE_SERIAL_NUMBER_VALUE;
        FileSystemType              = STORAGE_FILE_SYSTEM_TYPE_VALUE;
        StorageType                 = STORAGE_TYPE;
    }

    FakeStorage(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~FakeStorage()
    {
    }

    virtual HRESULT GetSupportedProperties(
        _In_    IPortableDeviceKeyCollection* pKeys);
  
    virtual HRESULT GetValue(
        _In_    REFPROPERTYKEY                Key,
        _In_    IPortableDeviceValues*        pStore);

    virtual HRESULT WriteValue(
        _In_    REFPROPERTYKEY                Key,
        _In_    REFPROPVARIANT                Value);

    virtual HRESULT GetPropertyAttributes(
        _In_    REFPROPERTYKEY                Key,
        _In_    IPortableDeviceValues*        pAttributes);

public:    
    // Standard WPD properties
    CAtlStringW             Description;
    CAtlStringW             SerialNumber;
    CAtlStringW             FileSystemType;

    GUID                    FunctionalCategory;
    ULONGLONG               FreeSpace;
    ULONGLONG               Capacity;
    DWORD                   StorageType;
};
