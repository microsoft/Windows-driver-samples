
//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#pragma once

// The class factory
class CIHVClassFactory : public IClassFactory
{
public:
   // Constructor
   CIHVClassFactory();
   ~CIHVClassFactory();

   // IUnknown
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);

   // IClassFactory
   STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
   STDMETHODIMP LockServer(BOOL fLock);

private:
   long m_refCount;
};
