/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  PropMan.h
*
*  Project:     Production Scanner Driver Sample
*
*  Description: This file contains the class definition of the
*               CWIAPropertyManager class that encapsulates
*               WIA property creation for this driver.
*
***************************************************************************/

#pragma once


//
// Structure definitions:
//

typedef struct _WIA_PROPERTY_INFO_DATA{
    LPOLESTR          pszName;  // property name
    PROPID            pid;      // property id
    PROPVARIANT       pv;       // property variant
    PROPSPEC          ps;       // property spec
    WIA_PROPERTY_INFO wpi;      // property info
} WIA_PROPERTY_INFO_DATA,*PWIA_PROPERTY_INFO_DATA;

//
// WIA access flag combinations:
//

#define RN   (WIA_PROP_READ | WIA_PROP_NONE)
#define RF   (WIA_PROP_READ | WIA_PROP_FLAG)
#define RW   (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_NONE)
#define RWL  (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_LIST)
#define RWR  (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_RANGE)
#define RWF  (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_FLAG)
#define RWLC (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_LIST|WIA_PROP_CACHEABLE)
#define RWRC (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_RANGE|WIA_PROP_CACHEABLE)
#define RWFC (WIA_PROP_READ | WIA_PROP_WRITE | WIA_PROP_FLAG|WIA_PROP_CACHEABLE)

//
// CWIAPropertyManager class:
//

class CWIAPropertyManager
{
private:
    CBasicDynamicArray<PWIA_PROPERTY_INFO_DATA> m_List;

    PWIA_PROPERTY_INFO_DATA
    FindProperty(
        LONG lPropertyID);

    HRESULT
    DeletePropertyData(
        _Inout_ PWIA_PROPERTY_INFO_DATA pInfo);

    PWIA_PROPERTY_INFO_DATA
    AllocatePropertyData();

    HRESULT
    RemovePropertyAndDeleteData(
        LONG lPropertyID);

public:
    CWIAPropertyManager();
    ~CWIAPropertyManager();

    //
    // LONG type property creation:
    //

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        LONG lCurrValue);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        ULONG ulCurrValueItems,
        _In_reads_(ulCurrValueItems) LONG *pCurrValue);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        _In_reads_(ulNumItems) BYTE *pbCurrValue,
        ULONG ulNumItems);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        LONG lCurrValue,
        LONG lNomValue,
        LONG lMinValue,
        LONG lMaxValue,
        LONG lInc);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        LONG lCurrValue,
        LONG lNomValue,
        _In_ LONG *plValues,
        LONG lNumValues);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        LONG lCurrValue,
        LONG lNomValue,
        _In_ CBasicDynamicArray<LONG> *pValueList);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        LONG lCurrValue,
        LONG lValidBits);

    //
    // ULONG type property creation
    //

    HRESULT
    AddPropertyUL(
        LONG  lPropertyID,
        _In_  LPOLESTR pszName,
        LONG  lAccessFlags,
        ULONG ulCurrValue);

    HRESULT
    AddPropertyUL(
        LONG  lPropertyID,
        _In_  LPOLESTR pszName,
        LONG  lAccessFlags,
        ULONG ulCurrValue,
        ULONG ulNomValue,
        ULONG ulMinValue,
        ULONG ulMaxValue,
        ULONG ulInc);

    //
    // BSTR type property creation:
    //

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        _In_ BSTR bstrCurrValue);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        ULONG ulCurrValueItems,
        _In_reads_(ulCurrValueItems) BSTR *pCurrValue);

    //
    // GUID type property creation:
    //

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        GUID guidCurrValue);

    HRESULT
    AddProperty(
        LONG lPropertyID,
        _In_ LPOLESTR pszName,
        LONG lAccessFlags,
        GUID guidCurrValue,
        GUID guidNomValue,
        _In_ CBasicDynamicArray<GUID> *pValueList);

    HRESULT
    RemoveProperty(
        LONG lPropertyID);

    HRESULT
    SetItemProperties(
        _Inout_ BYTE *pWiasContext);
};
