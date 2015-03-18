#pragma once

class FakeContent
{
public:
    FakeContent() :
        CanDelete(false), 
        RequiredScope(FULL_DEVICE_ACCESS),
        MarkedForDeletion(false)
    {
        Format = WPD_OBJECT_FORMAT_UNSPECIFIED;
        ContentType = WPD_CONTENT_TYPE_UNSPECIFIED;
    }

    FakeContent(const FakeContent& src) :
        CanDelete(false)
    {
        *this = src;
    }

    virtual ~FakeContent()
    {
        for(size_t index = 0; index < m_Children.GetCount(); index++)
        {
            if (m_Children[index])
            {
                delete(m_Children[index]);
                m_Children[index] = NULL;
            }
        }
        m_Children.RemoveAll();
    }

    virtual FakeContent& operator= (const FakeContent& src)
    {
        ObjectID                    = src.ObjectID;
        PersistentUniqueID          = src.PersistentUniqueID;
        ParentID                    = src.ParentID;
        Name                        = src.Name;
        ContentType                 = src.ContentType;
        Format                      = src.Format;
        CanDelete                   = src.CanDelete;
        RequiredScope               = src.RequiredScope;
        ParentPersistentUniqueID    = src.ParentPersistentUniqueID;
        ContainerFunctionalObjectID = src.ContainerFunctionalObjectID;

        return *this;
    }

    virtual HRESULT InitializeContent(
        _Inout_ DWORD *pdwLastObjectID);
        
    virtual HRESULT InitializeEnumerationContext(
                ACCESS_SCOPE                Scope,
        _In_    WpdObjectEnumeratorContext* pEnumeratorContext);

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

    virtual HRESULT CreatePropertiesOnlyObject(
        _In_                          IPortableDeviceValues* pObjectProperties,
        _Out_                         DWORD*                 pdwLastObjectID,
        _Outptr_result_nullonfailure_ FakeContent**          ppNewObject);

    virtual HRESULT GetSupportedResources(
        _In_    IPortableDeviceKeyCollection* pResources);

    virtual HRESULT GetResourceAttributes(
        _In_    REFPROPERTYKEY         Resource,
        _In_    IPortableDeviceValues* pAttributes);
    
    virtual HRESULT OpenResource(
        _In_    REFPROPERTYKEY            Resource,
                const DWORD               dwMode,
        _In_    WpdObjectResourceContext* pResourceContext); 

    virtual HRESULT ReadResourceData(
        _In_                            WpdObjectResourceContext*   pResourceContext,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead)   BYTE* pBuffer,
                                        const DWORD                 dwNumBytesToRead,
        _Out_                           DWORD*                      pdwNumBytesRead);

    virtual HRESULT WriteValues(
        _In_    IPortableDeviceValues* pValues,
        _In_    IPortableDeviceValues* pResults,
        _Out_   bool*                  pbObjectChanged); 

public:
    bool CanAccess(
                ACCESS_SCOPE Scope);

    HRESULT GetAllValues(
        _In_    IPortableDeviceValues* pStore);

    _Success_(return)
    bool FindNext(
                            ACCESS_SCOPE  Scope,
                            const DWORD   dwIndex,
        _Outptr_result_nullonfailure_     FakeContent** ppChild);

    HRESULT GetContent(
                            ACCESS_SCOPE  Scope,
        _In_                LPCWSTR       wszObjectID,
        _Outptr_result_nullonfailure_     FakeContent**  ppContent);

    HRESULT GetObjectIDsByFormat(
                ACCESS_SCOPE                          Scope,
        _In_    REFGUID                               Format,
                const DWORD                           dwDepth,
        _In_    IPortableDevicePropVariantCollection* pObjectIDs);

    HRESULT GetObjectIDByPersistentID(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszPersistentID,
        _In_    IPortableDevicePropVariantCollection* pObjectIDs);

    HRESULT MarkForDelete(
                const DWORD dwOptions);

    HRESULT RemoveObjectsMarkedForDeletion(
                ACCESS_SCOPE Scope);

public:
    CAtlStringW     ObjectID;
    CAtlStringW     PersistentUniqueID;
    CAtlStringW     ParentID;
    CAtlStringW     Name;
    CAtlStringW     ParentPersistentUniqueID;
    CAtlStringW     ContainerFunctionalObjectID;
    GUID            ContentType;
    GUID            Format;
    bool            CanDelete;
    bool            MarkedForDeletion;
    
    // A bitmask of all the required scopes in order to access this object
    ACCESS_SCOPE    RequiredScope;

    CAtlArray<FakeContent*> m_Children;
};
