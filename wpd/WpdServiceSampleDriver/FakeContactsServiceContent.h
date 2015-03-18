#pragma once

/**
 * This class represents an abstraction of a contacts service content object
 * Driver implementors should replace this with their own
 * device I/O classes/libraries.
 */

#define CONTACTS_SERVICE_OBJECT_ID                        L"789DEF"
#define CONTACTS_SERVICE_PERSISTENT_UNIQUE_ID             L"{95A95EA9-9904-430E-8FF6-70851F208478}"
#define CONTACTS_SERVICE_OBJECT_NAME_VALUE                NAME_ContactsSvc
#define CONTACTS_SERVICE_HUMAN_READABLE_NAME              L"Hello World Phone Contacts"
#define CONTACTS_SERVICE_PREFERRED_FORMAT                 WPD_OBJECT_FORMAT_ABSTRACT_CONTACT
#define CONTACTS_SERVICE_VERSION                          L"1.0"
#define CONTACTS_SERVICE_LOCAL_ONLY_DELETE                1

#define NUM_CONTACT_OBJECTS                               10

class FakeContactsServiceContent : public FakeContent
{
public:
    FakeContactsServiceContent()
    {
        ObjectID                    = CONTACTS_SERVICE_OBJECT_ID;
        PersistentUniqueID          = CONTACTS_SERVICE_PERSISTENT_UNIQUE_ID;
        ParentID                    = WPD_DEVICE_OBJECT_ID;
        Name                        = CONTACTS_SERVICE_OBJECT_NAME_VALUE;
        ContentType                 = WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT;
        Format                      = WPD_OBJECT_FORMAT_UNSPECIFIED;
        ParentPersistentUniqueID    = WPD_DEVICE_OBJECT_ID;
        ContainerFunctionalObjectID = WPD_DEVICE_OBJECT_ID;

        Version                     = CONTACTS_SERVICE_VERSION;
        FunctionalCategory          = SERVICE_Contacts;
        HumanReadableName           = CONTACTS_SERVICE_HUMAN_READABLE_NAME;
        PreferredSyncFormat         = CONTACTS_SERVICE_PREFERRED_FORMAT;
        RequiredScope               = CONTACTS_SERVICE_ACCESS;
        LocalOnlyDelete             = CONTACTS_SERVICE_LOCAL_ONLY_DELETE;
        FilterType                  = SYNCSVC_FILTER_CONTACTS_WITH_PHONE;
        CopyMemory(&ReplicaId, &MyFullEnumSyncReplicaId, sizeof(ReplicaId));
    }

    FakeContactsServiceContent(const FakeContent& src)
    {
        *this = src;
    }

    virtual ~FakeContactsServiceContent()
    {
    }

    virtual HRESULT InitializeContent(
        _Inout_ DWORD *pdwLastObjectID);

    virtual HRESULT CreatePropertiesOnlyObject(
        _In_                           IPortableDeviceValues* pObjectProperties,
        _Out_                          DWORD*                 pdwLastObjectID,
        _Outptr_result_nullonfailure_  FakeContent**          ppNewObject);

    virtual HRESULT GetSupportedProperties(
        _In_    IPortableDeviceKeyCollection *pKeys);

    virtual HRESULT GetPropertyAttributes(
        _In_    REFPROPERTYKEY         Key,
        _In_    IPortableDeviceValues* pAttributes);

    virtual HRESULT GetValue(
        _In_    REFPROPERTYKEY         Key,
        _In_    IPortableDeviceValues* pStore);

    virtual HRESULT WriteValue(
        _In_    REFPROPERTYKEY         Key,
        _In_    REFPROPVARIANT         Value);

private:
    HRESULT GetIconData(
        _In_    IPortableDeviceValues* pStore);

    HRESULT GetVICData(
        _In_    IPortableDeviceValues* pStore);

public:
    CAtlStringW Version;
    CAtlStringW HumanReadableName;

    GUID        FunctionalCategory;
    GUID        PreferredSyncFormat;
    GUID        ReplicaId;

    BYTE        LocalOnlyDelete;
    BYTE        FilterType;
};
