/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  PropMan.cpp
*
*  Project:     Production Scanner Driver Sample
*
*  Description: Contains the class implementation of the
*               CWIAPropertyManager class that encapsulates
*               WIA property creation for this driver.
*
***************************************************************************/

#include "stdafx.h"

/**************************************************************************\
*
* CWIAPropertyManager constructor
*
\**************************************************************************/

CWIAPropertyManager::CWIAPropertyManager()
{
    return;
}

/**************************************************************************\
*
* CWIAPropertyManager destructor
*
\**************************************************************************/

CWIAPropertyManager::~CWIAPropertyManager()
{
    //
    // Cleanup any items contained in the property list before exiting:
    //
    for (INT i = 0; i < m_List.Size(); i++)
    {
        PWIA_PROPERTY_INFO_DATA pPropertyData = m_List[i];

        if (pPropertyData)
        {
            //
            // Delete contents:
            //
            DeletePropertyData(pPropertyData);

            //
            // Delete container:
            //
            delete pPropertyData;
        }
    }
}


/**************************************************************************\
*
* This function finds the specified property, and removes it
* from the list of properties
*
* Parameters:
*
*    lPropertyID - Property ID of the property to find
*
* Return Value:
*
*    Pointer to property information list, NULL if error
*
\**************************************************************************/

PWIA_PROPERTY_INFO_DATA CWIAPropertyManager::FindProperty(
    LONG lPropertyID)
{
    PWIA_PROPERTY_INFO_DATA pInfo = NULL;

    for (INT i = 0; i< m_List.Size(); i++)
    {
        PWIA_PROPERTY_INFO_DATA pPropertyData = m_List[i];

        if (pPropertyData->pid == (ULONG)lPropertyID)
        {
            pInfo = pPropertyData;
            break;
        }
    }

    return pInfo;
}

/**************************************************************************\
*
* This function deletes the contents of a WIA_PROPERTY_DATA structure
*
* Parameters:
*
*    pInfo - pointer containing the property data
*
* Return Value:
*
*    S_OK or a a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::DeletePropertyData(
    _Inout_ PWIA_PROPERTY_INFO_DATA pInfo)
{
    HRESULT hr = E_INVALIDARG;

    if (pInfo)
    {
        //
        // Delete any allocated lists:
        //
        if (pInfo->wpi.lAccessFlags & WIA_PROP_LIST)
        {
            if (pInfo->pv.vt & VT_I4)
            {
                if (pInfo->wpi.ValidVal.List.pList)
                {
                    LocalFree(pInfo->wpi.ValidVal.List.pList);
                    pInfo->wpi.ValidVal.List.pList = NULL;
                }
            }

            if (pInfo->pv.vt & VT_CLSID)
            {
                if (pInfo->wpi.ValidVal.ListGuid.pList)
                {
                    LocalFree(pInfo->wpi.ValidVal.ListGuid.pList);
                    pInfo->wpi.ValidVal.ListGuid.pList = NULL;
                }
            }
        }

        //
        // Free any allocated BSTRs:
        //

        if (VT_BSTR == pInfo->pv.vt)
        {
            SysFreeString(pInfo->pv.bstrVal);
            pInfo->pv.bstrVal = NULL;
        }

        //
        // Delete any allocated GUIDs:
        //

        if (VT_CLSID == pInfo->pv.vt)
        {
            delete pInfo->pv.puuid;
            pInfo->pv.puuid = NULL;
        }

        hr = S_OK;
    }
    return hr;
}

/**************************************************************************\
*
* This function allocates a WIA_PROPERTY_INFO_DATA strucuture
* and initializes the members.
*
* Parameters:
*
*    none
*
* Return Value:
*
*    Pointer to the new structure
*
\**************************************************************************/

PWIA_PROPERTY_INFO_DATA CWIAPropertyManager::AllocatePropertyData()
{

    PWIA_PROPERTY_INFO_DATA pInfo = NULL;

    pInfo = new WIA_PROPERTY_INFO_DATA;

    if (pInfo)
    {
        //
        // Erase all values in newly allocated property data structure:
        //
        memset(pInfo, 0, sizeof(WIA_PROPERTY_INFO_DATA));

        //
        // Properly initialize the property variant:
        //
        PropVariantInit(&pInfo->pv);
    }

    return pInfo;
}

/**************************************************************************\
*
* This function finds the property specified by lPropertyID and deletes the
* contents of the WIA_PROPERTY_INFO_DATA.
*
* Parameters:
*
*    lPropertyID - Property ID of the property to remove and delete
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::RemovePropertyAndDeleteData(
    LONG lPropertyID)
{

    //
    // Find any existing property with the same ID, and remove it from the list:
    //

    PWIA_PROPERTY_INFO_DATA pInfo = FindProperty(lPropertyID);

    if (pInfo)
    {
        //
        // Find and remove the property info from the list and delete the contents:
        //
        m_List.Delete(m_List.Find(pInfo));
        delete pInfo;
        pInfo = NULL;
    }

    return S_OK;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  lCurrValue   - Current Property Value
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG          lPropertyID,
    _In_ LPOLESTR pszName,
    LONG          lAccessFlags,
    LONG          lCurrValue)
{

    HRESULT hr = E_INVALIDARG;

    if (pszName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {

            //
            // Populate the data in the structure, and add it to the property list:
            //

            pInfo->pszName          = pszName;
            pInfo->pid              = lPropertyID;
            pInfo->pv.lVal          = lCurrValue;
            pInfo->pv.vt            = VT_I4;
            pInfo->ps.ulKind        = PRSPEC_PROPID;
            pInfo->ps.propid        = pInfo->pid;
            pInfo->wpi.lAccessFlags = lAccessFlags;
            pInfo->wpi.vt           = pInfo->pv.vt;

            m_List.Append(pInfo);

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new array-of-LONG single-value property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID      - Property ID
*  pszName          - Property NAME
*  lAccessFlags     - Property Access Flags
*  ulCurrValueItems - Number of VT_I4 items in the current value array
*  pCurrValue       - Current Property Value, as an array of VT_I4 (LONG)
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG           lPropertyID,
    _In_ LPOLESTR  pszName,
    LONG           lAccessFlags,
    ULONG          ulCurrValueItems,
    _In_reads_(ulCurrValueItems)
    LONG           *pCurrValue)
{

    HRESULT hr = E_INVALIDARG;

    if (pszName && pCurrValue && ulCurrValueItems)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {
            //
            // Populate the data in the structure, and add it to the property list
            //
            // For a VT_VECTOR | VT_I4 the correct PROPVARIANT member is: cal (type: CAL)
            //
            pInfo->pszName          = pszName;
            pInfo->pid              = lPropertyID;
            pInfo->pv.cal.cElems    = ulCurrValueItems;
            pInfo->pv.cal.pElems    = pCurrValue;
            pInfo->pv.vt            = VT_I4 | VT_VECTOR;
            pInfo->ps.ulKind        = PRSPEC_PROPID;
            pInfo->ps.propid        = pInfo->pid;
            pInfo->wpi.lAccessFlags = lAccessFlags;
            pInfo->wpi.vt           = pInfo->pv.vt;

            m_List.Append(pInfo);

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new VT_VECTOR | VT_UI1 (array-of-BYTEs single-value)
* property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  pbCurrValue  - Current Property Value (BYTE vector)
*  ulNumItems   - Number of items in the current propery value vector
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG          lPropertyID,
    _In_ LPOLESTR pszName,
    LONG          lAccessFlags,
    _In_reads_(ulNumItems)
    BYTE*         pbCurrValue,
    ULONG         ulNumItems)
{
    HRESULT hr = E_INVALIDARG;

    if (pszName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {
            //
            // Populate the data in the structure, and add it to the property list
            //
            // Note: for a VT_VECTOR | VT_UI1 the correct PROPVARIANT member is: caub (type: CAUB)
            //
            // From MSDN:
            //
            // "If the type indicator is combined with VT_VECTOR by using an OR operator, the value is one of the counted array values.
            // This creates a DWORD count of elements, followed by a pointer to the specified repetitions of the value.
            // For example, a type indicator of VT_LPSTR | VT_VECTOR has a DWORD element count, followed by a pointer to an array of LPSTR elements.
            // VT_VECTOR can be combined by an OR operator with the following types: VT_I1, VT_UI1, VT_I2, VT_UI2, VT_BOOL, VT_I4, VT_UI4, VT_R4,
            // VT_R8, VT_ERROR, VT_I8, VT_UI8, VT_CY, VT_DATE, VT_FILETIME, VT_CLSID, VT_CF, VT_BSTR, VT_LPSTR, VT_LPWSTR, and VT_VARIANT".
            //
            pInfo->pszName          = pszName;
            pInfo->pid              = lPropertyID;
            pInfo->pv.caub.cElems   = ulNumItems;
            pInfo->pv.caub.pElems   = pbCurrValue;
            pInfo->pv.vt            = VT_UI1 | VT_VECTOR;
            pInfo->ps.ulKind        = PRSPEC_PROPID;
            pInfo->ps.propid        = pInfo->pid;
            pInfo->wpi.lAccessFlags = lAccessFlags;
            pInfo->wpi.vt           = pInfo->pv.vt;

            m_List.Append(pInfo);
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  lCurrValue   - Current Property Value
*  lValidBits   - Valid bit values
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG lPropertyID,
    _In_ LPOLESTR pszName,
    LONG lAccessFlags,
    LONG lCurrValue,
    LONG lValidBits)
{

    HRESULT hr = E_INVALIDARG;
    if (pszName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // allocate a property info structure
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {

            //
            // populate the data in the structure, and add it to the property list
            //

            pInfo->pszName                     = pszName;
            pInfo->pid                         = lPropertyID;
            pInfo->pv.lVal                     = lCurrValue;
            pInfo->pv.vt                       = VT_I4;
            pInfo->ps.ulKind                   = PRSPEC_PROPID;
            pInfo->ps.propid                   = pInfo->pid;
            pInfo->wpi.lAccessFlags            = lAccessFlags;
            pInfo->wpi.vt                      = pInfo->pv.vt;
            pInfo->wpi.ValidVal.Flag.Nom       = lCurrValue;
            pInfo->wpi.ValidVal.Flag.ValidBits = lValidBits;
            m_List.Append(pInfo);
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  lCurrValue   - Current Property Value
*  lNomValue    - Property Nominal Value
*  lMinValue    - Property Minimum Value
*  lMaxValue    - Property Maximum Value
*  lInc         - Property Increment Value
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG          lPropertyID,
    _In_ LPOLESTR pszName,
    LONG          lAccessFlags,
    LONG          lCurrValue,
    LONG          lNomValue,
    LONG          lMinValue,
    LONG          lMaxValue,
    LONG          lInc)
{
    HRESULT hr = E_INVALIDARG;

    if (pszName &&
       (lMinValue  <=  lMaxValue) &&
       (lNomValue  >= lMinValue) &&
       (lNomValue  <= lMaxValue) &&
       (lCurrValue >= lMinValue) &&
       (lCurrValue <= lMaxValue))
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {
            //
            // Populate the data in the structure, and add it to the property list:
            //

            pInfo->pszName                = pszName;
            pInfo->pid                    = lPropertyID;
            pInfo->pv.lVal                = lCurrValue;
            pInfo->pv.vt                  = VT_I4;
            pInfo->ps.ulKind              = PRSPEC_PROPID;
            pInfo->ps.propid              = pInfo->pid;
            pInfo->wpi.lAccessFlags       = lAccessFlags;
            pInfo->wpi.vt                 = pInfo->pv.vt;
            pInfo->wpi.ValidVal.Range.Inc = lInc;
            pInfo->wpi.ValidVal.Range.Min = lMinValue;
            pInfo->wpi.ValidVal.Range.Max = lMaxValue;
            pInfo->wpi.ValidVal.Range.Nom = lNomValue;

            m_List.Append(pInfo);
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  lCurrValue   - Current Property Value
*  lNomValue    - Property Nominal Value
*  pValueList   - List of Valid Values
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG                           lPropertyID,
    _In_ LPOLESTR                  pszName,
    LONG                           lAccessFlags,
    LONG                           lCurrValue,
    LONG                           lNomValue,
    _In_ CBasicDynamicArray<LONG>* pValueList)
{

    HRESULT hr = E_INVALIDARG;

    if (pszName && pValueList && pValueList->Size())
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;
        LONG *pLongList = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        LONG lNumValues = (LONG)pValueList->Size();
        if (lNumValues)
        {
            pLongList = (LONG*)LocalAlloc(LPTR, (sizeof(LONG) * lNumValues));
            if (pLongList)
            {
                for(INT iIndex = 0; iIndex < lNumValues; iIndex++)
                {
                    pLongList[iIndex] = ((*pValueList)[iIndex]);
                }
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                //
                // Allocate a property info structure:
                //

                pInfo = AllocatePropertyData();
                if (pInfo)
                {

                    //
                    // Populate the data in the structure, and add it to the property list:
                    //

                    pInfo->pszName                    = pszName;
                    pInfo->pid                        = lPropertyID;
                    pInfo->pv.lVal                    = lCurrValue;
                    pInfo->pv.vt                      = VT_I4;
                    pInfo->ps.ulKind                  = PRSPEC_PROPID;
                    pInfo->ps.propid                  = pInfo->pid;
                    pInfo->wpi.lAccessFlags           = lAccessFlags;
                    pInfo->wpi.vt                     = pInfo->pv.vt;
                    pInfo->wpi.ValidVal.List.pList    = (BYTE*)pLongList;
                    pInfo->wpi.ValidVal.List.Nom      = lNomValue;
                    pInfo->wpi.ValidVal.List.cNumList = lNumValues;

                    m_List.Append(pInfo);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        if (FAILED(hr))
        {
            if (pLongList)
            {
                LocalFree(pLongList);
                pLongList = NULL;
            }
        }
    }
    return hr;
}

/**************************************************************************\
*
* This function adds a new VT_UI4 single value property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  ulCurrValue  - Current Property Value
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddPropertyUL(
    LONG           lPropertyID,
    _In_ LPOLESTR  pszName,
    LONG           lAccessFlags,
    ULONG          ulCurrValue)
{

    HRESULT hr = E_INVALIDARG;

    if (pszName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {

            //
            // Populate the data in the structure, and add it to the property list:
            //

            pInfo->pszName          = pszName;
            pInfo->pid              = lPropertyID;
            pInfo->pv.lVal          = ulCurrValue;
            pInfo->pv.vt            = VT_UI4;
            pInfo->ps.ulKind        = PRSPEC_PROPID;
            pInfo->ps.propid        = pInfo->pid;
            pInfo->wpi.lAccessFlags = lAccessFlags;
            pInfo->wpi.vt           = pInfo->pv.vt;

            m_List.Append(pInfo);

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new VT_UI4 range property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID  - Property ID
*  pszName      - Property NAME
*  lAccessFlags - Property Access Flags
*  ulCurrValue  - Current Property Value
*  ulNomValue   - Property Nominal Value
*  ulMinValue   - Property Minimum Value
*  ulMaxValue   - Property Maximum Value
*  ulInc        - Property Increment Value
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddPropertyUL(
    LONG          lPropertyID,
    _In_ LPOLESTR pszName,
    LONG          lAccessFlags,
    ULONG         ulCurrValue,
    ULONG         ulNomValue,
    ULONG         ulMinValue,
    ULONG         ulMaxValue,
    ULONG         ulInc)
{
    HRESULT hr = E_INVALIDARG;

    if (pszName &&
       (ulMinValue  <=  ulMaxValue) &&
       (ulNomValue  >= ulMinValue) &&
       (ulNomValue  <= ulMaxValue) &&
       (ulCurrValue >= ulMinValue) &&
       (ulCurrValue <= ulMaxValue))
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {
            //
            // Populate the data in the structure, and add it to the property list:
            //

            pInfo->pszName                = pszName;
            pInfo->pid                    = lPropertyID;
            pInfo->pv.lVal                = ulCurrValue;
            pInfo->pv.vt                  = VT_UI4;
            pInfo->ps.ulKind              = PRSPEC_PROPID;
            pInfo->ps.propid              = pInfo->pid;
            pInfo->wpi.lAccessFlags       = lAccessFlags;
            pInfo->wpi.vt                 = pInfo->pv.vt;
            pInfo->wpi.ValidVal.Range.Inc = ulInc;
            pInfo->wpi.ValidVal.Range.Min = ulMinValue;
            pInfo->wpi.ValidVal.Range.Max = ulMaxValue;
            pInfo->wpi.ValidVal.Range.Nom = ulNomValue;

            m_List.Append(pInfo);
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID   - Property ID
*  pszName       - Property NAME
*  lAccessFlags  - Property Access Flags
*  bstrCurrValue - Current Property Value
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG          lPropertyID,
    _In_ LPOLESTR pszName,
    LONG          lAccessFlags,
    _In_ BSTR     bstrCurrValue)
{
    HRESULT hr = E_INVALIDARG;

    if (pszName && bstrCurrValue)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {
            //
            // Populate the data in the structure, and add it to the property list:
            //

            pInfo->pszName          = pszName;
            pInfo->pid              = lPropertyID;
            pInfo->pv.bstrVal       = SysAllocString(bstrCurrValue);
            pInfo->pv.vt            = VT_BSTR;
            pInfo->ps.ulKind        = PRSPEC_PROPID;
            pInfo->ps.propid        = pInfo->pid;
            pInfo->wpi.lAccessFlags = lAccessFlags;
            pInfo->wpi.vt           = pInfo->pv.vt;

            m_List.Append(pInfo);
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

/**************************************************************************\
*
* This function adds a new array-of-BSTR single-value property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID      - Property ID
*  pszName          - Property NAME
*  lAccessFlags     - Property Access Flags
*  ulCurrValueItems - Number of VT_BSTR items in the current value array
*  pCurrValue       - Current Property Value, as an array of VT_BSTR
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG           lPropertyID,
    _In_ LPOLESTR  pszName,
    LONG           lAccessFlags,
    ULONG          ulCurrValueItems,
    _In_reads_(ulCurrValueItems)
    BSTR           *pCurrValue)
{

    HRESULT hr = E_INVALIDARG;

    if (pszName && pCurrValue && ulCurrValueItems)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate a property info structure:
        //

        pInfo = AllocatePropertyData();
        if (pInfo)
        {
            //
            // Populate the data in the structure, and add it to the property list
            //
            // For a VT_VECTOR | VT_BSTR the correct PROPVARIANT member is: cabstr (type: CABSTR)
            //
            pInfo->pszName          = pszName;
            pInfo->pid              = lPropertyID;
            pInfo->pv.cabstr.cElems = ulCurrValueItems;
            pInfo->pv.cabstr.pElems = pCurrValue;
            pInfo->pv.vt            = VT_VECTOR | VT_BSTR;
            pInfo->ps.ulKind        = PRSPEC_PROPID;
            pInfo->ps.propid        = pInfo->pid;
            pInfo->wpi.lAccessFlags = lAccessFlags;
            pInfo->wpi.vt           = pInfo->pv.vt;

            m_List.Append(pInfo);

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID   - Property ID
*  pszName       - Property NAME
*  lAccessFlags  - Property Access Flags
*  guidCurrValue - Current Property Value
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG          lPropertyID,
    _In_ LPOLESTR pszName,
    LONG          lAccessFlags,
    GUID          guidCurrValue)
{
    HRESULT hr = E_INVALIDARG;

    if (pszName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // Allocate memory for a new GUID value and copy the data to it.
        // This memory is going to be freed when DeletePropertyData will
        // be called for this VT_CLSID property:
        //
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "pguid is not leaked")
        GUID *pguid = new GUID;
        if (pguid)
        {
            memcpy(pguid, &guidCurrValue, sizeof(GUID));
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            //
            // Allocate a property info structure:
            //

            pInfo = AllocatePropertyData();
            if (pInfo)
            {
                //
                // Populate the data in the structure, and add it to the property list:
                //

                pInfo->pszName                = pszName;
                pInfo->pid                    = lPropertyID;
                pInfo->pv.puuid               = pguid;
                pInfo->pv.vt                  = VT_CLSID;
                pInfo->ps.ulKind              = PRSPEC_PROPID;
                pInfo->ps.propid              = pInfo->pid;
                pInfo->wpi.lAccessFlags       = lAccessFlags;
                pInfo->wpi.vt                 = pInfo->pv.vt;

                m_List.Append(pInfo);
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

/**************************************************************************\
*
* This function adds a new property to the property list.
*
* If a property exists with the same property ID:
*
* 1. The old property is removed from the list, and the contents  destroyed
* 2. The new property is added to the list.
*
* Parameters:
*
*  lPropertyID   - Property ID
*  pszName       - Property NAME
*  lAccessFlags  - Property Access Flags
*  guidCurrValue - Current Property Value
*  guidNomValue  - Property Nominal Value
*  pValueList    - List of Valid Values
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(
    LONG                           lPropertyID,
    _In_ LPOLESTR                  pszName,
    LONG                           lAccessFlags,
    GUID                           guidCurrValue,
    GUID                           guidNomValue,
    _In_ CBasicDynamicArray<GUID>* pValueList)
{

    HRESULT hr = E_INVALIDARG;

    if (pszName && pValueList)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;
        GUID *pguid = NULL;
        GUID *pguidList = NULL;

        //
        // When a property is being added, always remove any existing property that has the same
        // property ID. Any call to AddProperty() means that the property being added should be
        // treated as the latest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        LONG lNumValues = (LONG)pValueList->Size();
        if (lNumValues)
        {
            pguidList = (GUID*)LocalAlloc(LPTR,(sizeof(GUID) * lNumValues));
            if (pguidList)
            {
                for (INT iIndex = 0; iIndex < lNumValues; iIndex++)
                {
                    pguidList[iIndex] = ((*pValueList)[iIndex]);
                }

                hr = S_OK;

                if (SUCCEEDED(hr))
                {
#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pguid is not leaked")
                    pguid = new GUID;
                    if (pguid)
                    {
                        *pguid = guidCurrValue;
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                //
                // Allocate a property info structure:
                //

                pInfo = AllocatePropertyData();
                if (pInfo)
                {

                    //
                    // Populate the data in the structure, and add it to the property list:
                    //

                    pInfo->pszName                        = pszName;
                    pInfo->pid                            = lPropertyID;
                    pInfo->pv.puuid                       = pguid;
                    pInfo->pv.vt                          = VT_CLSID;
                    pInfo->ps.ulKind                      = PRSPEC_PROPID;
                    pInfo->ps.propid                      = pInfo->pid;
                    pInfo->wpi.lAccessFlags               = lAccessFlags;
                    pInfo->wpi.vt                         = pInfo->pv.vt;
                    pInfo->wpi.ValidVal.ListGuid.pList    = pguidList;
                    pInfo->wpi.ValidVal.ListGuid.Nom      = guidNomValue;
                    pInfo->wpi.ValidVal.ListGuid.cNumList = lNumValues;

                    m_List.Append(pInfo);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        if (FAILED(hr))
        {
            //
            // Free memory any allocated memory if failure occurs:
            //

            if (pguidList)
            {
                LocalFree(pguidList);
                pguidList = NULL;
            }

            if (pguid)
            {
                delete pguid;
                pguid = NULL;
            }
        }
    }
    return hr;
}

/**************************************************************************\
*
* This function removes a property from the property list.
*
* Parameters:
*
*  lPropertyID - Property ID
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::RemoveProperty(
    LONG lPropertyID)
{
    return RemovePropertyAndDeleteData(lPropertyID);
}

/**************************************************************************\
*
* This function uses WIA helper functions to upload the properties
* to the Application Item Tree item created by the WIA service.
*
* Parameters:
*
*  pWiasContext - WIA Context provided by the WIA service
*
* Return Value:
*
*    S_OK or a standard COM error code
*
\**************************************************************************/

HRESULT CWIAPropertyManager::SetItemProperties(
    _Inout_ BYTE* pWiasContext)
{
    HRESULT hr = E_INVALIDARG;

    if (pWiasContext)
    {
        hr = S_OK;

        //
        // Get current number of properties in the list:
        //

        LONG lNumProps = m_List.Size();
        if (lNumProps > 0)
        {
            LONG lIndex = 0;
            LPOLESTR *pszName = NULL;
            PROPID *ppid = NULL;
            PROPVARIANT *ppv = NULL;
            PROPSPEC *pps = NULL;
            WIA_PROPERTY_INFO *pwpi = NULL;

            //
            // Allocate arrays of structures needed to contain the property data:
            //

            #pragma prefast(suppress:__WARNING_MEMORY_LEAK_EXCEPTION, "When using a new operator that throws this can leak:")
            pszName = new LPOLESTR[lNumProps];
            #pragma prefast(suppress:__WARNING_MEMORY_LEAK_EXCEPTION, "When using a new operator that throws this can leak:")
            ppid    = new PROPID[lNumProps];
            #pragma prefast(suppress:__WARNING_MEMORY_LEAK_EXCEPTION, "When using a new operator that throws this can leak:")
            ppv     = new PROPVARIANT[lNumProps];
            #pragma prefast(suppress:__WARNING_MEMORY_LEAK_EXCEPTION, "When using a new operator that throws this can leak:")
            pps     = new PROPSPEC[lNumProps];
            #pragma prefast(suppress:__WARNING_MEMORY_LEAK_EXCEPTION, "When using a new operator that throws this can leak:")
            pwpi    = new WIA_PROPERTY_INFO[lNumProps];

            if (pszName && ppid && ppv && pps && pwpi)
            {
                //
                // Copy the property data into the proper structures:
                //
                for(INT i = 0; i < lNumProps; i++)
                {
                    PWIA_PROPERTY_INFO_DATA pPropertyData = m_List[i];
                    if (pPropertyData)
                    {
                        pszName[lIndex] = pPropertyData->pszName;
                        ppid[lIndex] = pPropertyData->pid;
                        memcpy(&ppv[lIndex], &pPropertyData->pv, sizeof(PROPVARIANT));
                        memcpy(&pps[lIndex], &pPropertyData->ps, sizeof(PROPSPEC));
                        memcpy(&pwpi[lIndex], &pPropertyData->wpi, sizeof(WIA_PROPERTY_INFO));
                        lIndex++;
                    }
                }

                //
                // Send the property names to the WIA service:
                //
                #pragma prefast(suppress:__WARNING_USING_UNINIT_VAR, "ppid, *ppid, pszName and *pszName are initialized above"
                hr = wiasSetItemPropNames(pWiasContext, lNumProps, ppid, pszName);
                if (SUCCEEDED(hr))
                {
                    //
                    // Send the property values to the WIA service:
                    //
                    #pragma prefast(suppress:__WARNING_USING_UNINIT_VAR, "pps and *pps are initialized above"
                    hr = wiasWriteMultiple(pWiasContext, lNumProps, pps, ppv);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Send the property valid values to the WIA service:
                        //
                        #pragma prefast(suppress:__WARNING_USING_UNINIT_VAR, "pwpi and *pvpi are initialized above"
                        hr = wiasSetItemPropAttribs(pWiasContext, lNumProps, pps, pwpi);
                        if (FAILED(hr))
                        {
                            WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties, wiasSetItemPropAttribs failed"));
                        }
                    }
                    else
                    {
                        WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties, wiasWriteMultiple failed"));
                    }
                }
                else
                {
                    WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties, wiasSetItemPropNames failed"));
                }
            }
            else
            {
                WIAEX_ERROR((g_hInst, "CWIAPropertyManager::SetItemProperties, failed to allocate memory for property arrays"));
                hr = E_OUTOFMEMORY;
            }

            //
            // Always delete any temporary memory allocated before exiting the function.
            // The WIA service makes a copy of the information during the "wias" helper calls.
            //

            if (pszName)
            {
                delete [] pszName;
                pszName = NULL;
            }

            if (ppid)
            {
                delete [] ppid;
                ppid = NULL;
            }

            if (ppv)
            {
                delete [] ppv;
                ppid = NULL;
            }

            if (pps)
            {
                delete [] pps;
                ppid = NULL;
            }

            if (pwpi)
            {
                delete [] pwpi;
                ppid = NULL;
            }
        }
    }

    return hr;
}
