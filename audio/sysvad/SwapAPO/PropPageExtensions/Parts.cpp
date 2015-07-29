//**@@@*@@@****************************************************
//
// Microsoft Windows DevTopo.dll
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    Parts.cpp
//
// Abstract:    Implementation of CPart and derived classes.
//
// --------------------------------------------------------------------------------

#include "stdafx.h"
#include <DeviceTopology.h>
#include "Parts.h"
   
_Analysis_mode_(_Analysis_code_type_user_driver_)

// ----------------------------------------------------------------------
// Function:
//      CPartsList::CPartsList
//
// Description:
//      CPartsList constructor
// ----------------------------------------------------------------------
CPartsList::CPartsList() :
    m_refCount(1)
{
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::~CPartsList
//
// Description:
//      CPartsList destructor
// ----------------------------------------------------------------------
CPartsList::~CPartsList()
{
    ATLASSERT(m_refCount == 0);

    IPart*  pIPart;
    while (m_lstParts.RemoveHead(&pIPart))
    {
        SAFE_RELEASE(pIPart);
    }
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::QueryInterface
//
// Description:
//      Implementation of IUnknown::QueryInterface
//
// Parameters:
//      riid - [in] Identifier of the interface being queried
//      ppvObj - [in] Pointer to a buffer that receives a pointer to the
//               object whose interface is queried
//
// Return:
//      S_OK if successful
// ----------------------------------------------------------------------
STDMETHODIMP CPartsList::QueryInterface
(
    const IID& iid,
    void** ppUnk
)
{
    if (!ppUnk)
        return E_POINTER;

    HRESULT hr = E_NOINTERFACE;
    *ppUnk = NULL;

    if (iid == __uuidof(IPartsList))
    {
        *ppUnk = (IPartsList*)this;
        hr = S_OK;
        AddRef();
    }
    else
    if (iid == __uuidof(IUnknown))
    {
        *ppUnk = (IUnknown*)this;
        hr = S_OK;
        AddRef();
    }
        
    return hr;
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::AddRef
//
// Description:
//      Implementation of IUnknown::AddRef
//
// Return:
//      New refcount
// ----------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CPartsList::AddRef(void)
{
    return InterlockedIncrement(&m_refCount);
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::Release
//
// Description:
//      Implementation of IUnknown::Release
//
// Return:
//      New refcount
// ----------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CPartsList::Release(void)
{
    ATLASSERT(m_refCount > 0);
    LONG lRef = InterlockedDecrement(&m_refCount);
    ATLASSERT(lRef >= 0);

    if (lRef == 0)
    {
        delete this;
    }
    return lRef;
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::AddPart
//
// Description: 
//      Adds an item to the list of IPart interfaces
//
// Parameters:
//      pIPart - [in] Part to add to the list
//
// Return:
//      S_OK if successful
// ----------------------------------------------------------------------
STDMETHODIMP CPartsList::AddPart
(
    IPart* pIPart
)
{
    if (pIPart == NULL)
        return E_POINTER;

    // Protect m_lstParts access
    m_CritSection.Enter();

    pIPart->AddRef();
    m_lstParts.AddTail(pIPart);

    m_CritSection.Leave();

    return S_OK;
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::AddPart
//
// Description: 
//      Adds the PartsList Parts to the list of IPart interfaces
//
// Parameters:
//      pIParts - [in] List of Parts to be added to the list
//
// Return:
//      S_OK if successful
// ----------------------------------------------------------------------
STDMETHODIMP CPartsList::AddParts
(
    IPartsList* pIParts
)
{
    ATLASSERT(pIParts != NULL);

    HRESULT hr = S_OK;
    UINT i, cParts = 0;

    // Protect m_lstParts
    m_CritSection.Enter();

    // Loop through specified parts list
    hr = pIParts->GetCount(&cParts);
    IF_FAILED_JUMP(hr, Exit);

    for (i = 0; (i < cParts) && (hr == S_OK); i++)
    {
        IPart*  pIPart;

        // ... and copy reference to each part
        hr = pIParts->GetPart(i, &pIPart);
        IF_FAILED_JUMP(hr, Exit);

        // ... to this list
        m_lstParts.AddTail(pIPart);
    }

Exit:
    m_CritSection.Leave();

    return hr;
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::GetCount
//
// Description:
//      Returns the count of items in the list of IParts
//
// Parameters:
//      pCount - [out] the count of parts in this list
//
// Return:
//      S_OK if successful
//
// ----------------------------------------------------------------------
_Use_decl_annotations_
STDMETHODIMP CPartsList::GetCount
(
    UINT* pCount
)
{
    if (pCount == NULL)
        return E_POINTER;

    // Protect m_lstParts access
    m_CritSection.Enter();

    *pCount = m_lstParts.GetCount();

    m_CritSection.Leave();

    return S_OK;
}


// ----------------------------------------------------------------------
// Function:
//      CPartsList::GetPart
//
// Description:
//      Returns the nIndex-th item in the list of IParts
//
// Parameters:
//      nIndex - [in] index of the part to access
//      uMsg - [out] the part at the specified index
//
// Return:
//      S_OK if successful
//
// ----------------------------------------------------------------------
_Use_decl_annotations_
STDMETHODIMP CPartsList::GetPart
(
    UINT nIndex,
    IPart** ppPart
)
{
    HRESULT hr;
    IPart*  pIPart = NULL;

    // Protect m_lstParts access
    m_CritSection.Enter();

    IF_TRUE_ACTION_JUMP((ppPart == NULL), hr = E_POINTER, Exit);

    *ppPart = NULL;

    IF_TRUE_ACTION_JUMP((nIndex >= m_lstParts.GetCount()), hr = E_INVALIDARG, Exit);

    IF_TRUE_ACTION_JUMP((!m_lstParts.GetAt(nIndex, &pIPart)), hr = E_NOTFOUND, Exit);

    // Take care of AddRef and ensure correct interface in one shot
    hr = pIPart->QueryInterface(__uuidof(IPart), reinterpret_cast<void**>(ppPart));
    IF_FAILED_JUMP(hr, Exit);

Exit:
    m_CritSection.Leave();

    return hr;
}
