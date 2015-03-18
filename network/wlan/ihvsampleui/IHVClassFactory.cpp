//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

#include "precomp.h"
#include "ihvuiinc_i.c"

extern long g_serverLock; //lock count on server

//
// IUnknown Implementation
//
CIHVClassFactory::CIHVClassFactory() : m_refCount(1)
{
}

CIHVClassFactory::~CIHVClassFactory()
{
}

STDMETHODIMP_(ULONG) 
CIHVClassFactory::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) 
CIHVClassFactory::Release()
{
	ULONG refCount = InterlockedDecrement(&m_refCount);
	if (refCount == 0)
	{
		delete this;
	}
	return refCount;
}

STDMETHODIMP 
CIHVClassFactory::QueryInterface(
    REFIID riid, 
    void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;
    if (NULL != ppvObject)
    {
        hr = S_OK;
	    if (riid == IID_IUnknown)
	    {
		    *ppvObject = static_cast<IUnknown *>(this);
	    }
	    else if (riid == IID_IClassFactory)
	    {
		    *ppvObject = static_cast<IClassFactory *>(this);
	    }
	    else
	    {
		    *ppvObject = NULL;
		    return E_NOINTERFACE;
	    }
	    reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();	
    }

	return hr;
}


//
// IClassFactory Implementaion
//
STDMETHODIMP 
CIHVClassFactory::CreateInstance(
    IUnknown *pUnkOuter, 
    REFIID riid, 
    void **ppvObject
    )
{
   HRESULT hr = E_NOINTERFACE;

   // aggregation not supported
   if (pUnkOuter != NULL)
   {
      return CLASS_E_NOAGGREGATION;
   }

   if (NULL == ppvObject)
   {
       return E_INVALIDARG;
   }

   // Figure out which interface is wanted
   // the ui will call us only as IID_IDot11ExtUI
   if (IID_IDot11SampleExtUI == riid || 
       IID_IDot11ExtUI == riid ||
       IID_IWizardExtension == riid)
   {
       CDot11SampleExtUI *pExtUI = new(std::nothrow) CDot11SampleExtUI();
       if (NULL == pExtUI)
       {
           return E_OUTOFMEMORY;
       }

       pExtUI->AddRef();
       hr = pExtUI->QueryInterface(riid, ppvObject);

       pExtUI->Release();
   }
   else if (IID_IDot11SampleExtUIConProperty == riid)
   {
       CDot11SampleExtUIConProperty *pCDot11SampleExtUIConProperty = new(std::nothrow) CDot11SampleExtUIConProperty();
       if (NULL == pCDot11SampleExtUIConProperty)
       {
           return E_OUTOFMEMORY;
       }

       pCDot11SampleExtUIConProperty->AddRef();
       hr = pCDot11SampleExtUIConProperty->QueryInterface(riid, ppvObject);

       pCDot11SampleExtUIConProperty->Release();
   }
   else if (IID_IDot11SampleExtUISecProperty == riid)
   {
       CDot11SampleExtUISecProperty *pCDot11SampleExtUISecProperty = new(std::nothrow) CDot11SampleExtUISecProperty();
       if (NULL == pCDot11SampleExtUISecProperty)
       {
           return E_OUTOFMEMORY;
       }

       pCDot11SampleExtUISecProperty->AddRef();
       hr = pCDot11SampleExtUISecProperty->QueryInterface(riid, ppvObject);

       pCDot11SampleExtUISecProperty->Release();
   }
   else if (IID_IDot11SampleExtUIKeyProperty == riid)
   {
       CDot11SampleExtUIKeyProperty *pCDot11SampleExtUIKeyProperty = new(std::nothrow) CDot11SampleExtUIKeyProperty();
       if (NULL == pCDot11SampleExtUIKeyProperty)
       {
           return E_OUTOFMEMORY;
       }

       pCDot11SampleExtUIKeyProperty->AddRef();
       hr = pCDot11SampleExtUIKeyProperty->QueryInterface(riid, ppvObject);

       pCDot11SampleExtUIKeyProperty->Release();
   }

   return hr;
}


STDMETHODIMP 
CIHVClassFactory::LockServer(BOOL fLock)
{
	if (fLock)
	{
		InterlockedIncrement(&g_serverLock);
	}
	else
	{
		InterlockedDecrement(&g_serverLock);
	}

	return S_OK;
}
