/*****************************************************************************
 *
 *  wiaitem.cpp
 *
 *  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
 *
 *  DESCRIPTION:
 *
 * wiaitem is a simply wrapper class used to read properties from an item
 * of interface IWiaItem2 
 *  
 *******************************************************************************/
#include "stdafx.h"
#include "wiaitem.h"

///
///  Constructor - sets m_pIWiaPropStg to NULL
///
CWiaItem::CWiaItem()
{
    m_pIWiaPropStg = NULL;
}

///
///  Destructor
///
CWiaItem::~CWiaItem()
{
    Release();
}

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @func STDMETHODIMP | CWiaItem::SetIWiaItem | Specifies the item to read from
 *  
 *  @parm   IWiaItem2 | pIWiaItem |
 *          The item we want to read properties from
 * 
 *  @comm
 *  SetIWiaItem simply QIs the passed in item for its IWiaPropertyStorage interface.
 *  This interface is later used in all calls to Read[Required]PropertyXXX
 *
 *  @rvalue S_OK    | 
 *              The function succeeded.
 *  @rvalue E_XXX   | 
 *              The function failed 
 * 
 *****************************************************************************/
HRESULT CWiaItem::SetIWiaItem(IWiaItem2 *pIWiaItem)
{
    HRESULT hr = S_OK;
    Release();

    if (!pIWiaItem)
    {
        return E_INVALIDARG;
    }

    hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(VOID**)&m_pIWiaPropStg);
    
    return hr;
}

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @func STDMETHODIMP | CWiaItem::ReadPropertyLong | Reads a LONG value from the
 *  currently set item.
 *  
 *  @parm   PROPID | PropertyID |
 *          Id of property to read
 *
 *  @parm   LONG* | plPropertyValue |
 *          Pointer where we store the result from the read operation. 
 * 
 *  @rvalue S_OK    | 
 *              The function succeeded.
 *  @rvalue E_XXX   | 
 *              The function failed 
 * 
 *****************************************************************************/
HRESULT CWiaItem::ReadRequiredPropertyLong(PROPID PropertyID, _Out_ LONG *plPropertyValue)
{
    if (!plPropertyValue)
    {
        return E_INVALIDARG;
    }

    if (!m_pIWiaPropStg)
    {
        return E_POINTER;
    }

    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];

    memset(PropVar, 0, sizeof(PropVar));
    PropVariantInit(PropVar);

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = PropertyID;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);

    //
    // This is a required property 
    //
    if (S_FALSE == hr)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *plPropertyValue = PropVar[0].lVal;
        PropVariantClear(PropVar);
    }

    return hr;
}

HRESULT CWiaItem::ReadRequiredPropertyBSTR(PROPID PropertyID, _Outptr_ BSTR *pbstrPropertyValue)
{
    if (!pbstrPropertyValue)
    {
        return E_INVALIDARG;
    }

    if (!m_pIWiaPropStg)
    {
        return E_POINTER;
    }

    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];

    memset(PropVar, 0, sizeof(PropVar));
    PropVariantInit(PropVar);

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = PropertyID;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);

    //
    // This is a required property 
    //
    if (S_FALSE == hr)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pbstrPropertyValue = SysAllocString(PropVar[0].bstrVal);
        if (!*pbstrPropertyValue )
        {
            hr = E_OUTOFMEMORY;
        }
        PropVariantClear(PropVar);
    }

    return hr;
}

HRESULT CWiaItem::ReadRequiredPropertyGUID(PROPID PropertyID, _Out_ GUID *pguidPropertyValue)
{
    if (!pguidPropertyValue)
    {
        return E_INVALIDARG;
    }

    if (!m_pIWiaPropStg)
    {
        return E_POINTER;
    }

    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];

    memset(PropVar, 0, sizeof(PropVar));
    PropVariantInit(PropVar);

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = PropertyID;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);

    if (S_FALSE == hr)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        memcpy(pguidPropertyValue,PropVar[0].puuid,sizeof(GUID));
        PropVariantClear(PropVar);
    }

    return hr;
}

///
/// Release releases the IPropertyStorage member variable
///
void CWiaItem::Release()
{
    if (m_pIWiaPropStg)
    {
        m_pIWiaPropStg->Release();
        m_pIWiaPropStg = NULL;
    }
}
