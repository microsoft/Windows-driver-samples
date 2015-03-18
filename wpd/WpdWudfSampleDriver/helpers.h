#pragma once

#define FAKE_DATA_SIZE          (5 * 1024 * 1024 + 1831)
#define OPTIMAL_BUFFER_SIZE     (2 * 1024 * 1024)

// {9b2dce3f-cf02-4643-ae09-2bcf0012ac6d}
DEFINE_GUID(FakeContent_Format, 0x9b2dce3f, 0xcf02, 0x4643, 0xae, 0x09, 0x2b, 0xcf, 0x00, 0x12, 0xac, 0x6d);
// {8E829938-D838-479E-8489-5EC84986EE3B}
DEFINE_GUID(FakeDeviceContent_Format, 0x8E829938, 0xD838, 0x479E, 0x84, 0x89, 0x5E, 0xC8, 0x49, 0x86, 0xEE, 0x3B);
// {BDC7BBF8-3AAC-458F-92C9-7CD236186552}
DEFINE_GUID(FakeStorageContent_Format, 0xBDC7BBF8, 0x3AAC, 0x458F, 0x92, 0xC9, 0x7C, 0xD2, 0x36, 0x18, 0x65, 0x52);
//  We will define a custom format for memo objects: {C6F2ECC0-C351-42D6-AE20-837CE1EF433C}
DEFINE_GUID(FakeMemoContent_Format, 0xC6F2ECC0, 0xC351, 0x42D6, 0xAE, 0x20, 0x83, 0x7C, 0xE1, 0xEF, 0x43, 0x3C);

// {4DF6C8C7-2CE5-457C-9F53-EFCECAA95C04}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_CLIENT_CONTEXT_MAP, 0x4DF6C8C7, 0x2CE5, 0x457C, 0x9F, 0x53, 0xEF, 0xCE, 0xCA, 0xA9, 0x5C, 0x04, 2);
// {CDD18979-A7B0-4D5E-9EB2-0A826805CBBD}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WUDF_DEVICE_OBJECT, 0xCDD18979, 0xA7B0, 0x4D5E, 0x9E, 0xB2, 0x0A, 0x82, 0x68, 0x05, 0xCB, 0xBD, 2);
// {9BD949E5-59CF-41AE-90A9-BE1D044F578F}
DEFINE_PROPERTYKEY(PRIVATE_SAMPLE_DRIVER_WPD_SERIALIZER_OBJECT, 0x9BD949E5, 0x59CF, 0x41AE, 0x90, 0xA9, 0xBE, 0x1D, 0x04, 0x4F, 0x57, 0x8F, 2);

#ifndef SAFE_RELEASE
    #define SAFE_RELEASE(p) if( NULL != p ) { ( p )->Release(); p = NULL; }
#endif

typedef enum tagFakeDevicePropertyAttributesType
{
    UnspecifiedForm_CanRead_CanWrite_CannotDelete_Fast,
    UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast,
} FakeDevicePropertyAttributesType;

typedef struct tagKeyAndAttributesEntry
{
    const GUID*                         pFormat;
    const PROPERTYKEY*                  pKey;
    FakeDevicePropertyAttributesType    type;
} KeyAndAttributesEntry;

HRESULT AddFixedAttributesByType(
            FakeDevicePropertyAttributesType AttributesType,
    _In_    IPortableDeviceValues*           pAttributes);

HRESULT AddFixedPropertyAttributes(
    _In_    REFGUID                         guidObjectFormat,
    _In_    REFPROPERTYKEY                  key,
    _In_    IPortableDeviceValues*          pAttributes);

HRESULT AddSupportedProperties(
    _In_            REFGUID                         guidObjectFormatOrCategory,
    _COM_Outptr_    IPortableDeviceKeyCollection**  ppKeys);

HRESULT AddSupportedProperties(
    _In_    REFGUID                        guidObjectFormatOrCategory,
    _In_    IPortableDeviceKeyCollection*  pKeys);

HRESULT SetRenderingProfiles(
    _In_    IPortableDeviceValues*          pValues);

HRESULT AddExtraSupportedProperties(
    _In_    LPCWSTR                         pszObjectID,
    _In_    IPortableDeviceKeyCollection*   pKeys);

HRESULT AddExtraPropertyValues(
    _In_    LPCWSTR                         pszObjectID,
    _In_    IPortableDeviceValues*          pValues);

DWORD GetResourceSize(
    UINT uiResource);

PBYTE GetResourceData(
    UINT uiResource);

HRESULT IsValidContentType(
    _In_    REFGUID             guidObjectContentType,
    _In_    CAtlArray<GUID>&    RestrictedTypes);

HRESULT GetClientContext(
    _In_            IPortableDeviceValues*  pParams,
    _In_            LPCWSTR                 pszContextKey,
    _COM_Outptr_    IUnknown**              ppContext);

HRESULT GetClientEventCookie(
    _In_                      IPortableDeviceValues*  pParams,
    _Outptr_result_maybenull_ LPWSTR*       ppszEventCookie);

HRESULT PostWpdEvent(
    _In_    IPortableDeviceValues*  pCommandParams,
    _In_    IPortableDeviceValues*  pEventParams);

HRESULT PostWpdEventWithProgress(
    _In_    IPortableDeviceValues*  pCommandParams,
    _In_    IPortableDeviceValues*  pEventParams,
    _In_    REFGUID                 guidEvent,
            const DWORD             dwOperationState,
            const DWORD             dwOperationProgress);

BOOL ExistsInCollection(_In_ REFGUID guid, _In_ IPortableDevicePropVariantCollection* pCollection);

class PropVariantWrapper : public tagPROPVARIANT
{
public:
    PropVariantWrapper()
    {
        PropVariantInit(this);
    }

    PropVariantWrapper(_In_ LPCWSTR pszSrc)
    {
        PropVariantInit(this);

        *this = pszSrc;
    }

    virtual ~PropVariantWrapper()
    {
        Clear();
    }

    void Clear()
    {
        PropVariantClear(this);
    }

    PropVariantWrapper& operator= (ULONG ulValue)
    {
        Clear();
        vt      = VT_UI4;
        ulVal   = ulValue;

        return *this;
    }

    PropVariantWrapper& operator= (_In_ LPCWSTR pszSrc)
    {
        Clear();

        pwszVal = AtlAllocTaskWideString(pszSrc);
        if(pwszVal != NULL)
        {
            vt = VT_LPWSTR;
        }
        return *this;
    }

    PropVariantWrapper& operator= (_In_ IUnknown* punkSrc)
    {
        Clear();

        // Need to AddRef as PropVariantClear will Release
        if (punkSrc != NULL)
        {
            vt      = VT_UNKNOWN;
            punkVal = punkSrc;
            punkVal->AddRef();
        }
        return *this;
    }

    void SetErrorValue(HRESULT hr)
    {
        Clear();
        vt      = VT_ERROR;
        scode   = hr;
    }

    void SetBoolValue(bool bValue)
    {
        Clear();
        vt      = VT_BOOL;
        if(bValue)
        {
            boolVal = VARIANT_TRUE;
        }
        else
        {
            boolVal = VARIANT_FALSE;
        }
    }
};

HRESULT CreateVCard(_In_ IPortableDeviceValues* pValues, _Out_ CAtlStringA& strVCard);

HRESULT UpdateDeviceFriendlyName(
    _In_    IPortableDeviceClassExtension*  pPortableDeviceClassExtension,
    _In_    LPCWSTR                         wszDeviceFriendlyName);

HRESULT GetCommonResourceAttributes(
    _COM_Outptr_    IPortableDeviceValues** ppAttributes);
