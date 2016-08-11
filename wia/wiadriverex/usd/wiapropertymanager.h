/**************************************************************************
*
*  Copyright (c) 2003  Microsoft Corporation
*
*  Title:       wiapropertymanager.h
*
*  Description: This file contains the class definition of the
*               CWIAPropertyManager class that encapsulates simple WIA
*               property creation.
*
***************************************************************************/
#pragma once

/////////////////////////////////////////////////////////////////////////////
// structure definitions

typedef struct _WIA_PROPERTY_INFO_DATA{
    LPOLESTR          szName;   // property name
    PROPID            pid;      // property id
    PROPVARIANT       pv;       // property variant
    PROPSPEC          ps;       // property spec
    WIA_PROPERTY_INFO wpi;      // property info
}WIA_PROPERTY_INFO_DATA,*PWIA_PROPERTY_INFO_DATA;

/////////////////////////////////////////////////////////////////////////////
// #define WIA flags to make shorter arguments

#define RN   WIA_PROP_READ|WIA_PROP_NONE
#define RF   WIA_PROP_READ|WIA_PROP_FLAG
#define RWL  WIA_PROP_READ|WIA_PROP_WRITE|WIA_PROP_LIST
#define RWR  WIA_PROP_READ|WIA_PROP_WRITE|WIA_PROP_RANGE
#define RWF  WIA_PROP_READ|WIA_PROP_WRITE|WIA_PROP_FLAG
#define RWLC WIA_PROP_READ|WIA_PROP_WRITE|WIA_PROP_LIST|WIA_PROP_CACHEABLE
#define RWRC WIA_PROP_READ|WIA_PROP_WRITE|WIA_PROP_RANGE|WIA_PROP_CACHEABLE
#define RWFC WIA_PROP_READ|WIA_PROP_WRITE|WIA_PROP_FLAG|WIA_PROP_CACHEABLE

class CWIAPropertyManager {
private:
    CBasicDynamicArray<PWIA_PROPERTY_INFO_DATA> m_List;
    PWIA_PROPERTY_INFO_DATA FindProperty(LONG lPropertyID);
    HRESULT DeletePropertyData(_In_ PWIA_PROPERTY_INFO_DATA pInfo);
    PWIA_PROPERTY_INFO_DATA AllocatePropertyData();
    HRESULT RemovePropertyAndDeleteData(LONG lPropertyID);
public:
    CWIAPropertyManager();
    ~CWIAPropertyManager();

    //
    // LONG type properties
    //

    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue);
    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, _In_ BYTE *pbCurrValue, ULONG ulNumItems);
    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue,
                        LONG lNomValue,
                        LONG lMinValue,
                        LONG lMaxValue,
                        LONG lInc);
    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue,
                        LONG lNomValue, _In_ LONG *plValues, LONG lNumValues);
    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue,
                        LONG lNomValue, _In_ CBasicDynamicArray<LONG> *pValueList);
    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue, LONG lValidBits);

    //
    // BSTR type properties
    //

    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, _In_ BSTR bstrCurrValue);

    //
    // GUID type properties
    //

    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, GUID guidCurrValue);
    HRESULT AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, GUID guidCurrValue,
                        GUID guidNomValue, _In_ CBasicDynamicArray<GUID> *pValueList);


    HRESULT RemoveProperty(LONG lPropertyID);
    HRESULT SetItemProperties(_Inout_ BYTE *pWiasContext);
};
