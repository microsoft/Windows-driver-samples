//**@@@*@@@****************************************************
//
// Microsoft Windows DevTopo.dll
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    Parts.h
//
// Abstract:    CPartsList is a list container of parts
//
// --------------------------------------------------------------------------------

#pragma once


class CPartsList : public IPartsList
{
private:

    // This is a REAL refcount.  When it goes to zero, the object is destroyed.
    LONG                m_refCount;
    TList<IPart>        m_lstParts;

    CCriticalSection    m_CritSection;

protected:
    ~CPartsList();          // Refcounted objects should have a private destructor.

public:
    CPartsList();

    STDMETHOD(AddPart)(IPart* pIPart);
    STDMETHOD(AddParts)(IPartsList* pIParts);

public:

    // IPartsList
    STDMETHOD(GetCount)(_Out_ UINT* pCount);
    STDMETHOD(GetPart)(_In_ UINT nIndex, _Out_ IPart** ppPart);

    // IUnknown
    HRESULT STDMETHODCALLTYPE   QueryInterface(const IID& iid, void** ppUnk);
    ULONG   STDMETHODCALLTYPE   AddRef(void);
    ULONG   STDMETHODCALLTYPE   Release(void);

    friend class CDeviceTopology;
};
