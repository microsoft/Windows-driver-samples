#pragma once

/**
 * This class represents an abstraction of a real device.
 * Driver implementors should replace this with their own
 * device I/O classes/libraries.
 */

class FakeDevice
{
public:
    FakeDevice() : m_dwLastObjectID(0)
    {
    }

    ~FakeDevice()
    {
    }

    HRESULT InitializeContent();

    FakeContactsService* GetContactsService();

    ACCESS_SCOPE GetAccessScope(
        _In_    IPortableDeviceValues*                pParams);

    // Device Capabilities
    // These are legacy commands that apply to the whole device, no access scope is required
    HRESULT GetSupportedCommands(
        _In_    IPortableDeviceKeyCollection*         pCommands);
    
    HRESULT GetCommandOptions(
        _In_    REFPROPERTYKEY                        Command,
        _In_    IPortableDeviceValues*                pOptions);

    HRESULT GetSupportedFunctionalCategories(
        _In_    IPortableDevicePropVariantCollection* pFunctionalCategories);
   
    HRESULT GetFunctionalObjects(
        _In_    REFGUID                               guidFunctionalCategory,
        _In_    IPortableDevicePropVariantCollection* pFunctionalObjects);
   
    HRESULT GetSupportedContentTypes(
        _In_    REFGUID                              guidFunctionalCategory,
        _In_    IPortableDevicePropVariantCollection* pContentTypes);
   
    HRESULT GetSupportedFormats(
        _In_    REFGUID                              guidContentType,
        _In_    IPortableDevicePropVariantCollection* pFormats);
    
    HRESULT GetSupportedFormatProperties(
        _In_    REFGUID                              guidObjectFormat, 
        _In_    IPortableDeviceKeyCollection*         pKeys);
    
    HRESULT GetFixedPropertyAttributes(
        _In_    REFGUID                              guidObjectFormat, 
        _In_    REFPROPERTYKEY                        Key, 
        _In_    IPortableDeviceValues* pAttributes);
    
    HRESULT GetSupportedEvents(
        _In_    IPortableDevicePropVariantCollection* pEvents);
    
    HRESULT GetEventOptions(
        _In_    IPortableDeviceValues*                pOptions);

    // Enumeration
    // Depending on the access scope, the driver can display only objects within the current
    // scoped hierarchy tree
    void InitializeEnumerationContext(
                ACCESS_SCOPE                          Scope, 
        _In_    LPCWSTR                               wszParentID, 
        _In_    WpdObjectEnumeratorContext*           pEnumContext);
  
    HRESULT FindNext(
                    const DWORD                           dwNumObjectsRequested, 
        _In_        WpdObjectEnumeratorContext*           pEnumContext, 
        _In_        IPortableDevicePropVariantCollection* pObjectIDCollection, 
        _Out_opt_   DWORD*                                pdwNumObjectsEnumerated);

    HRESULT GetObjectIDsByFormat(
                ACCESS_SCOPE                          Scope,
        _In_    REFGUID                              guidObjectFormat,
        _In_    LPCWSTR                               wszParentObjectID,
                const DWORD                           dwDepth,
        _In_    IPortableDevicePropVariantCollection* pObjectIDs);

    HRESULT GetObjectIDsFromPersistentUniqueIDs(
                ACCESS_SCOPE                          Scope,
        _In_    IPortableDevicePropVariantCollection* pPersistentIDs,
        _In_    IPortableDevicePropVariantCollection* pObjectIDs);

    // Property Management
    // Depending on the access scope, the driver can allow access to properties of objects within the current
    // scoped hierarchy tree
    HRESULT GetSupportedProperties(
                ACCESS_SCOPE                          Scope, 
        _In_    LPCWSTR                               wszObjectID, 
        _In_    IPortableDeviceKeyCollection*         pKeys);

    HRESULT GetAllPropertyValues(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    IPortableDeviceValues*                pValues);

    HRESULT GetPropertyValues(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    IPortableDeviceKeyCollection*         pKeys,
        _In_    IPortableDeviceValues*                pValues);

    HRESULT SetPropertyValues(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    IPortableDeviceValues*                pValues,
        _In_    IPortableDeviceValues*                pResults, 
        _In_    IPortableDeviceValues*                pEventParams,
        _Out_   bool*                                 pbObjectChanged);

    HRESULT GetPropertyAtributes(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    REFPROPERTYKEY                        Key,
        _In_    IPortableDeviceValues*                pAttributes);

    // Object Management
    // Depending on the access scope, the driver can limit access only to objects within the current
    // scoped hierarchy tree
    HRESULT CreatePropertiesOnlyObject(
                    ACCESS_SCOPE                      Scope,
        _In_        IPortableDeviceValues*            pObjectProperties,
        _In_        IPortableDeviceValues*            pEventParams,
        _Outptr_result_nullonfailure_    LPWSTR*      ppszNewObjectID);

    HRESULT DeleteObject(
                ACCESS_SCOPE                          Scope,
                const DWORD                           dwDeleteOptions,
        _In_    LPCWSTR                               wszObjectID,
        _In_    IPortableDeviceValues*                pEventParams);

    // Resources
    HRESULT GetSupportedResources(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    IPortableDeviceKeyCollection*         pResources);

    HRESULT GetResourceAttributes(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    REFPROPERTYKEY                        Resource,
        _In_    IPortableDeviceValues*                pAttributes);

    HRESULT OpenResource(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _In_    REFPROPERTYKEY                        Resource,
                const DWORD                           dwMode,
        _In_    WpdObjectResourceContext*             pResourceContext); 

    HRESULT ReadResourceData(
        _In_                            WpdObjectResourceContext*   pResourceContext,
        _Out_writes_to_(dwNumBytesToRead, *pdwNumBytesRead)   BYTE* pBuffer,
                                        const DWORD                 dwNumBytesToRead,
        _Out_                           DWORD*                      pdwNumBytesRead);

private:
    HRESULT GetContent(
                ACCESS_SCOPE                          Scope,
        _In_    LPCWSTR                               wszObjectID,
        _Outptr_result_nullonfailure_   FakeContent** ppContent);

    HRESULT RemoveObjectsMarkedForDeletion();

private:

    // Simulates content on the device
    FakeDeviceContent   m_DeviceContent;

    // Simulates contacts service functionality
    FakeContactsService m_ContactsService;

    DWORD               m_dwLastObjectID;
};
