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
 *  SetIWiaItem QIs the passed in IWiaItem2 object for its IWiaPropertyStorage interface
 *  which it stores internally and uses in the read and write functions.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
HRESULT CWiaItem::SetIWiaItem(_In_ IWiaItem2 *pIWiaItem)
{
    if (!pIWiaItem)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    Release();

    //
    // Get WIA property storage interface and store into member variable
    //
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
HRESULT CWiaItem::ReadPropertyLong(PROPID PropertyID, _Out_ LONG *plPropertyValue)
{
    if (!plPropertyValue)
    {
        return E_INVALIDARG;
    }

    if (!m_pIWiaPropStg)
    {
        return E_POINTER;
    }

    *plPropertyValue = 0;

    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];

    memset(PropVar, 0, sizeof(PropVar));
    PropVariantInit(PropVar);

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = PropertyID;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (S_OK == hr)
    {
        *plPropertyValue = PropVar[0].lVal;
        PropVariantClear(PropVar);
    }
    return hr;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CWiaItem::ReadPropertyGUID | Reads a GUID value from the
 *  currently set item.
 *
 *  @parm   PROPID | PropertyID |
 *          Id of property to read
 *
 *  @parm   GUID* | pguidPropertyValue |
 *          Pointer where we store the result from the read operation.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
HRESULT CWiaItem::ReadPropertyGUID(PROPID PropertyID, _Out_ GUID *pguidPropertyValue)
{
    if (!pguidPropertyValue)
    {
        return E_INVALIDARG;
    }

    if (!m_pIWiaPropStg)
    {
        return E_POINTER;
    }

    memset(pguidPropertyValue, 0, sizeof(GUID));

    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];

    memset(PropVar, 0, sizeof(PropVar));
    PropVariantInit(PropVar);

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = PropertyID;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (hr == S_OK)
    {
        memcpy(pguidPropertyValue,PropVar[0].puuid,sizeof(GUID));
        PropVariantClear(PropVar);
    }
    return hr;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CWiaItem::ReadPropertyBSTR | Reads a BSTR value from the
 *  currently set item.
 *
 *  @parm   PROPID | PropertyID |
 *          Id of property to read
 *
 *  @parm   BSTR* | pbstrPropertyValue |
 *          Pointer where we store the result from the read operation.
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
HRESULT CWiaItem::ReadPropertyBSTR(PROPID PropertyID, _Out_ BSTR *pbstrPropertyValue)
{
    if (!pbstrPropertyValue)
    {
        return E_INVALIDARG;
    }

    if (!m_pIWiaPropStg)
    {
        return E_POINTER;
    }

    *pbstrPropertyValue = NULL;

    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];

    memset(PropVar, 0, sizeof(PropVar));
    PropVariantInit(PropVar);

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = PropertyID;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (hr == S_OK)
    {
        *pbstrPropertyValue = SysAllocString(PropVar[0].bstrVal);
        if (!*pbstrPropertyValue)
        {
            hr = E_OUTOFMEMORY;
        }
        PropVariantClear(PropVar);
    }
    return hr;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CWiaItem::WritePropertyLong | Writes a LONG value to the
 *  currently set item.
 *
 *  @parm   PROPID | PropertyID |
 *          Id of property to read
 *
 *  @parm   LONG | lPropertyValue |
 *          Value to write
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
HRESULT CWiaItem::WritePropertyLong(PROPID PropertyID, LONG lPropertyValue)
{
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
    PropVar[0].vt      = VT_I4;
    PropVar[0].lVal    = lPropertyValue;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->WriteMultiple(1, PropSpec, PropVar, MIN_PROPID);

    return hr;
}

/*****************************************************************************
 *
 *  @doc INTERNAL
 *
 *  @func STDMETHODIMP | CWiaItem::WritePropertyGUID | Writes a GUID value to the
 *  currently set item.
 *
 *  @parm   PROPID | PropertyID |
 *          Id of property to read
 *
 *  @parm   GUID | guidPropertyValue |
 *          Value to write
 *
 *  @rvalue S_OK    |
 *              The function succeeded.
 *  @rvalue E_XXX   |
 *              The function failed
 *
 *****************************************************************************/
HRESULT CWiaItem::WritePropertyGUID(PROPID PropertyID, GUID guidPropertyValue)
{
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
    PropVar[0].vt      = VT_CLSID;
    PropVar[0].puuid   = &guidPropertyValue;

    HRESULT hr = S_OK;
    hr = m_pIWiaPropStg->WriteMultiple(1, PropSpec, PropVar, MIN_PROPID);

    return hr;
}

///
/// Release releases the IWiaPropertyStorage member variable
///
void CWiaItem::Release()
{
    if (m_pIWiaPropStg)
    {
        m_pIWiaPropStg->Release();
        m_pIWiaPropStg = NULL;
    }
}
