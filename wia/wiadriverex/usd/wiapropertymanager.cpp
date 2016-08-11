/**************************************************************************
*
*  Copyright (c) 2003  Microsoft Corporation
*
*  Title:       wiapropertymanager.cpp
*
*  Date:
*
*  Description: This file contains the class implementation of the
*               CWIAPropertyManager class that encapsulates simple WIA
*               property creation.
*
***************************************************************************/
#include "stdafx.h"

CWIAPropertyManager::CWIAPropertyManager()
{
    ;
}

CWIAPropertyManager::~CWIAPropertyManager()
{

    //
    // cleanup any items contained in the property list
    // before exiting
    //

    for(INT i = 0; i < m_List.Size(); i++)
    {
        PWIA_PROPERTY_INFO_DATA pPropertyData = m_List[i];

        if(pPropertyData)
        {
            //
            // delete contents
            //

            DeletePropertyData(pPropertyData);

            //
            // delete container
            //

            delete pPropertyData;
        }
    }
}

/*****************************************************************************
   Function Name: FindProperty

   Arguments:

   LONG lPropertyID - Property ID of the property to find

   Description:

   This function finds the specified property, and removes it from the list
   of properties

 *****************************************************************************/

PWIA_PROPERTY_INFO_DATA CWIAPropertyManager::FindProperty(LONG lPropertyID)
{

    PWIA_PROPERTY_INFO_DATA pInfo = NULL;

    if(0 <= lPropertyID)
    {
        for(INT i = 0; i< m_List.Size(); i++)
        {
            PWIA_PROPERTY_INFO_DATA pPropertyData = m_List[i];
            if(pPropertyData->pid == (ULONG) lPropertyID)
            {
                pInfo = pPropertyData;
                break;
            }
        }
    }

    return pInfo;
}

/*****************************************************************************
   Function Name: DeletePropertyData

   Arguments:

   PWIA_PROPERTY_INFO_DATA pInfo - pointer containing the property data

   Description:

   This function deletes the contents of a WIA_PROPERTY_DATA structure.

 *****************************************************************************/

HRESULT CWIAPropertyManager::DeletePropertyData(_In_ PWIA_PROPERTY_INFO_DATA pInfo)
{

    HRESULT hr = E_INVALIDARG;
    if (pInfo)
    {

        //
        // delete any allocated LISTS
        //

        if (pInfo->wpi.lAccessFlags & WIA_PROP_LIST)
        {
            if(pInfo->pv.vt & VT_I4)
            {
                WIAS_TRACE((g_hInst,"Freeing LONG List for %d",pInfo->pid));
                if (pInfo->wpi.ValidVal.List.pList)
                {
                    LocalFree(pInfo->wpi.ValidVal.List.pList);
                    pInfo->wpi.ValidVal.List.pList = NULL;
                }
            }

            if(pInfo->pv.vt & VT_CLSID)
            {
                WIAS_TRACE((g_hInst,"Freeing GUID List for %d",pInfo->pid));
                if (pInfo->wpi.ValidVal.ListGuid.pList)
                {
                    LocalFree(pInfo->wpi.ValidVal.ListGuid.pList);
                    pInfo->wpi.ValidVal.ListGuid.pList = NULL;
                }
            }
        }

        //
        // free any allocated BSTRS
        //

        if (pInfo->pv.vt == VT_BSTR)
        {
            SysFreeString(pInfo->pv.bstrVal);
            pInfo->pv.bstrVal = NULL;
        }

        //
        // delete any allocated GUIDS
        //

        if (pInfo->pv.vt == VT_CLSID)
        {
            delete pInfo->pv.puuid;
            pInfo->pv.puuid = NULL;
        }

        hr = S_OK;
    }
    return hr;
}

/*****************************************************************************
   Function Name: AllocatePropertyData

   Arguments:

   NONE

   Description:

   This function allocates a WIA_PROPERTY_INFO_DATA strucuture, and initializes
   the members.

 *****************************************************************************/

PWIA_PROPERTY_INFO_DATA CWIAPropertyManager::AllocatePropertyData()
{

    PWIA_PROPERTY_INFO_DATA pInfo = NULL;
    pInfo = new WIA_PROPERTY_INFO_DATA;
    if (pInfo)
    {
        //
        // erase all values in newly allocated property data structure
        //

        memset(pInfo,0,sizeof(WIA_PROPERTY_INFO_DATA));

        //
        // properly initialize the property variant
        //

        PropVariantInit(&pInfo->pv);
    }
    return pInfo;
}

/*****************************************************************************
   Function Name: RemovePropertyAndDeleteData

   Arguments:

   LONG lPropertyID - Property ID of the property to remove and delete

   Description:

   This function finds the property specified by lPropertyID and deletes the
   contents of the WIA_PROPERTY_INFO_DATA.

 *****************************************************************************/

HRESULT CWIAPropertyManager::RemovePropertyAndDeleteData(LONG lPropertyID)
{

    //
    // find any existing property with the same ID, and remove it from the list
    //

    PWIA_PROPERTY_INFO_DATA pInfo = FindProperty(lPropertyID);
    if (pInfo)
    {

        //
        // find and remove the property info from the list and delete the
        // contents
        //

        m_List.Delete(m_List.Find(pInfo));
        delete pInfo;
        pInfo = NULL;
    }
    return S_OK;
}

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID  - Property ID
   LPOLESTR szName   - Property NAME
   LONG lAccessFlags - Property Access Flags
   LONG lCurrValue   - Current Property Value

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue)
{

    HRESULT hr = E_INVALIDARG;
    if(szName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // allocate a property info structure
        //

        pInfo = AllocatePropertyData();
        if(pInfo)
        {

            //
            // populate the data in the structure, and add it to the property list
            //

            pInfo->szName           = szName;
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

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID  - Property ID
   LPOLESTR szName   - Property NAME
   LONG lAccessFlags - Property Access Flags
   BYTE *pbCurrValue - Current Property Value (BYTE vector)
   ULONG ulNumItems   - Number of items in the current propery value vector

   Description:

   This function adds a new VT_UI1 | VT_VECTOR property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID,
                                         _In_ LPOLESTR szName,
                                         LONG lAccessFlags,
                                         _In_ BYTE *pbCurrValue,
                                         ULONG ulNumItems)
{

    HRESULT hr = E_INVALIDARG;
    if(szName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // allocate a property info structure
        //

        pInfo = AllocatePropertyData();
        if(pInfo)
        {

            //
            // populate the data in the structure, and add it to the property list
            //
            // Note: for a VT_VECTOR | VT_UI1 the correct PROPVARIANT member is: caub (type: CAUB)
            //
            // From MSDN:
            //
            // "If the type indicator is combined with VT_VECTOR by using an OR operator, the value is one of the counted array values.
            // This creates a DWORD count of elements, followed by a pointer to the specified repetitions of the value.
            // For example, a type indicator of VT_LPSTR|VT_VECTOR has a DWORD element count, followed by a pointer to an array of LPSTR elements.
            // VT_VECTOR can be combined by an OR operator with the following types: VT_I1, VT_UI1, VT_I2, VT_UI2, VT_BOOL, VT_I4, VT_UI4, VT_R4,
            // VT_R8, VT_ERROR, VT_I8, VT_UI8, VT_CY, VT_DATE, VT_FILETIME, VT_CLSID, VT_CF, VT_BSTR, VT_LPSTR, VT_LPWSTR, and VT_VARIANT".
            //
            pInfo->szName           = szName;
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

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID  - Property ID
   LPOLESTR szName   - Property NAME
   LONG lAccessFlags - Property Access Flags
   LONG lCurrValue   - Current Property Value
   LONG lValidBits   - Valid bit values

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/


HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID,
                                         _In_ LPOLESTR szName,
                                         LONG lAccessFlags,
                                         LONG lCurrValue,
                                         LONG lValidBits)
{

    HRESULT hr = E_INVALIDARG;
    if(szName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // allocate a property info structure
        //

        pInfo = AllocatePropertyData();
        if(pInfo)
        {

            //
            // populate the data in the structure, and add it to the property list
            //

            pInfo->szName                      = szName;
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

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID  - Property ID
   LPOLESTR szName   - Property NAME
   LONG lAccessFlags - Property Access Flags
   LONG lCurrValue   - Current Property Value
   LONG lNomValue    - Property Nominal Value
   LONG lMinValue    - Property Minimum Value
   LONG lMaxValue    - Property Maximum Value
   LONG lInc         - Property Increment Value

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/


HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID,
                                         _In_ LPOLESTR szName,
                                         LONG lAccessFlags,
                                         LONG lCurrValue,
                                         LONG lNomValue,
                                         LONG lMinValue,
                                         LONG lMaxValue,
                                         LONG lInc)
{

    HRESULT hr = E_INVALIDARG;
    if((szName)&&
       (lMinValue  <= lMaxValue) &&
       (lNomValue  >= lMinValue) &&
       (lNomValue  <= lMaxValue) &&
       (lCurrValue >= lMinValue) &&
       (lCurrValue <= lMaxValue))  // TODO: validate lInc value???
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // allocate a property info structure
        //

        pInfo = AllocatePropertyData();
        if(pInfo)
        {

            //
            // populate the data in the structure, and add it to the property list
            //

            pInfo->szName                 = szName;
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
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID  - Property ID
   LPOLESTR szName   - Property NAME
   LONG lAccessFlags - Property Access Flags
   LONG lCurrValue   - Current Property Value
   LONG lNomValue    - Property Nominal Value
   CBasicDynamicArray<LONG> - LONG Array

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, LONG lCurrValue,
                                         LONG lNomValue, _In_ CBasicDynamicArray<LONG> *pValueList)
{

    HRESULT hr = E_INVALIDARG;
    if((szName)&&(pValueList)&&(pValueList->Size()))
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;
        LONG *pLongList = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        if(pValueList)
        {
            LONG lNumValues = (LONG)pValueList->Size();
            if(lNumValues)
            {
                pLongList = (LONG*)LocalAlloc(LPTR,(sizeof(LONG)*lNumValues));
                if(pLongList)
                {
                    for(INT iIndex = 0; iIndex < pValueList->Size(); iIndex++)
                    {
                        pLongList[iIndex] = ((*pValueList)[iIndex]);
                    }
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if(SUCCEEDED(hr))
                {

                    //
                    // allocate a property info structure
                    //

                    pInfo = AllocatePropertyData();
                    if(pInfo)
                    {

                        //
                        // populate the data in the structure, and add it to the property list
                        //

                        pInfo->szName                     = szName;
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
        }
        else
        {
            hr = E_INVALIDARG;
        }

        if(FAILED(hr))
        {
            if(pLongList)
            {
                LocalFree(pLongList);
                pLongList = NULL;
            }
        }
    }
    return hr;
}

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID   - Property ID
   LPOLESTR szName    - Property NAME
   LONG lAccessFlags  - Property Access Flags
   BSTR bstrCurrValue - Current Property Value

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, _In_ BSTR bstrCurrValue)
{

    HRESULT hr = E_INVALIDARG;
    if((szName)&&(bstrCurrValue))
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        //
        // allocate a property info structure
        //

        pInfo = AllocatePropertyData();
        if(pInfo)
        {

            //
            // populate the data in the structure, and add it to the property list
            //

            pInfo->szName                 = szName;
            pInfo->pid                    = lPropertyID;
            pInfo->pv.bstrVal             = SysAllocString(bstrCurrValue);
            pInfo->pv.vt                  = VT_BSTR;
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
    return hr;
}

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID   - Property ID
   LPOLESTR szName    - Property NAME
   LONG lAccessFlags  - Property Access Flags
   GUID guidCurrValue - Current Property Value

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, GUID guidCurrValue)
{

    HRESULT hr = E_INVALIDARG;
    if(szName)
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pguid is freed by DeletePropertyData() when m_List is destroyed.")
        GUID *pguid = new GUID;
        if(pguid)
        {
            *pguid = guidCurrValue;
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {

            //
            // allocate a property info structure
            //

            pInfo = AllocatePropertyData();
            if(pInfo)
            {
                //
                // populate the data in the structure, and add it to the property list
                //

                pInfo->szName                 = szName;
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
                //
                //  Cleanup locally allocated memory
                //
                delete pguid;
                pguid = NULL;

                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

/*****************************************************************************
   Function Name: AddProperty

   Arguments:

   LONG lPropertyID   - Property ID
   LPOLESTR szName    - Property NAME
   LONG lAccessFlags  - Property Access Flags
   GUID guidCurrValue - Current Property Value
   GUID guidNomValue  - Property Nominal Value
   CBasicDynamicArray<GUID> - GUID List

   Description:

   This function adds a new property to the property list.

   Remarks:
   If a property exists with the same property ID:
        1. The old property is removed from the list, and the contents
           destroyed
        2. The new property is added to the list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::AddProperty(LONG lPropertyID, _In_ LPOLESTR szName, LONG lAccessFlags, GUID guidCurrValue,
                                         GUID guidNomValue, _In_ CBasicDynamicArray<GUID> *pValueList)
{

    HRESULT hr = E_INVALIDARG;
    if((szName)&&(pValueList))
    {
        PWIA_PROPERTY_INFO_DATA pInfo = NULL;
        GUID *pguid = NULL;
        GUID *pguidList = NULL;

        //
        // when a property is being added, always remove any existing property that has the same
        // property ID.  Any call to AddProperty() means that the property being added should be
        // treated as the lastest.
        //

        RemovePropertyAndDeleteData(lPropertyID);

        if(pValueList)
        {
            LONG lNumValues = (LONG)pValueList->Size();
            if(lNumValues)
            {
                pguidList = (GUID*)LocalAlloc(LPTR,(sizeof(GUID)*lNumValues));
                if(pguidList)
                {
                    for(INT iIndex = 0; iIndex < pValueList->Size(); iIndex++)
                    {
                        pguidList[iIndex] = ((*pValueList)[iIndex]);
                    }

                    hr = S_OK;

                    if(SUCCEEDED(hr))
                    {
#pragma prefast(suppress:__WARNING_ALIASED_MEMORY_LEAK, "pguid is freed by DeletePropertyData() when m_List is destroyed.")
                        pguid = new GUID;
                        if(pguid)
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

                if(SUCCEEDED(hr))
                {

                    //
                    // allocate a property info structure
                    //

                    pInfo = AllocatePropertyData();
                    if(pInfo)
                    {

                        //
                        // populate the data in the structure, and add it to the property list
                        //

                        pInfo->szName                         = szName;
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
        }
        else
        {
            hr = E_INVALIDARG;
        }

        if(FAILED(hr))
        {
            // free memory any allocated memory if failure occurs
            if(pguidList)
            {
                LocalFree(pguidList);
                pguidList = NULL;
            }

            if(pguid)
            {
                delete pguid;
                pguid = NULL;
            }
        }
    }
    return hr;
}

/*****************************************************************************
   Function Name: RemoveProperty

   Arguments:

   LONG lPropertyID   - Property ID

   Description:

   This function removes a property from the property list.

 *****************************************************************************/

HRESULT CWIAPropertyManager::RemoveProperty(LONG lPropertyID)
{

    return RemovePropertyAndDeleteData(lPropertyID);
}

/*****************************************************************************
   Function Name: SetItemProperties

   Arguments:

   BYTE *pWiasContext   - WIA Context provided by the WIA service

   Description:

   This function uses WIA helper functions to upload the properties to the
   WIA service.

 *****************************************************************************/

HRESULT CWIAPropertyManager::SetItemProperties(_Inout_ BYTE *pWiasContext)
{

    HRESULT hr = E_INVALIDARG;
    if(pWiasContext)
    {
        hr = S_OK;

        //
        // get current number of properties in the list
        //

        LONG lNumProps = m_List.Size();
        if(lNumProps)
        {
            LONG lIndex = 0;

            LPOLESTR *pszName = (LPOLESTR*) LocalAlloc(LPTR,sizeof(LPOLESTR)*lNumProps);
            PROPID *ppid = (PROPID*) LocalAlloc(LPTR,sizeof(PROPID)*lNumProps);
            PROPVARIANT *ppv = (PROPVARIANT*) LocalAlloc(LPTR,sizeof(PROPVARIANT)*lNumProps);
            PROPSPEC *pps = (PROPSPEC*) LocalAlloc(LPTR,sizeof(PROPSPEC)*lNumProps);
            WIA_PROPERTY_INFO *pwpi = (WIA_PROPERTY_INFO*) LocalAlloc(LPTR,sizeof(WIA_PROPERTY_INFO)*lNumProps);

            if((pszName)&&(ppid)&&(ppv)&&(pps)&&(pwpi))
            {

                //
                // copy the property data into the proper structures
                //

                for(INT i = 0; i < m_List.Size(); i++)
                {
                    PWIA_PROPERTY_INFO_DATA pPropertyData = m_List[i];
                    if(pPropertyData)
                    {
                        pszName[lIndex] = pPropertyData->szName;
                        ppid[lIndex] = pPropertyData->pid;
                        memcpy(&ppv[lIndex],&pPropertyData->pv,sizeof(PROPVARIANT));
                        memcpy(&pps[lIndex],&pPropertyData->ps, sizeof(PROPSPEC));
                        memcpy(&pwpi[lIndex],&pPropertyData->wpi,sizeof(WIA_PROPERTY_INFO));
                        lIndex++;
                    }
                }

                //
                // send the property names to the WIA service
                //

                hr = wiasSetItemPropNames(pWiasContext,lNumProps,ppid,pszName);
                if(SUCCEEDED(hr))
                {

                    //
                    // send the property values to the WIA service
                    //

                    hr = wiasWriteMultiple(pWiasContext,lNumProps,pps,ppv);
                    if(SUCCEEDED(hr))
                    {

                        //
                        // send the property valid values to the WIA service
                        //

                        hr = wiasSetItemPropAttribs(pWiasContext,lNumProps,pps,pwpi);
                        if(FAILED(hr))
                        {
                            WIAS_ERROR((g_hInst, "CWIAPropertyManager_SetItemProperties - wiasSetItemPropAttribs failed"));
                        }
                    }
                    else
                    {
                        WIAS_ERROR((g_hInst, "CWIAPropertyManager_SetItemProperties - wiasWriteMultiple failed"));
                    }
                }
                else
                {
                    WIAS_ERROR((g_hInst, "CWIAPropertyManager_SetItemProperties - wiasSetItemPropNames failed"));
                }
            }
            else
            {
                WIAS_ERROR((g_hInst, "CWIAPropertyManager_SetItemProperties - failed to allocate memory for property arrays"));
                hr = E_OUTOFMEMORY;
            }

            //
            // always delete any temporary memory allocated before exiting the function.  The WIA
            // service makes a copy of the information during the "wias" helper calls.
            //

            if(pszName)
            {
                LocalFree(pszName);
                pszName = NULL;
            }

            if(ppid)
            {
                LocalFree(ppid);
                ppid = NULL;
            }

            if(ppv)
            {
                LocalFree(ppv);
                ppv = NULL;
            }

            if(pps)
            {
                LocalFree(pps);
                pps = NULL;
            }

            if(pwpi)
            {
                LocalFree(pwpi);
                pwpi = NULL;
            }
        }
    }
    return hr;
}
