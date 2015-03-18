//
// Copyright (C) Microsoft Corporation 2005
// IHV UI Extension sample
//

// Common macro based implementation of IUnknown
// using a interface table approach

#define IMPLEMENT_REFCOUNT()\
    ULONG m_crefCount;\
    \
    STDMETHODIMP_(ULONG) AddRef(void)\
    {\
        return InterlockedIncrement((PLONG)&m_crefCount);\
    }\
    \
    _At_(this, __drv_aliasesMem)\
    STDMETHODIMP_(ULONG) Release(void)\
    {\
        ULONG res = InterlockedDecrement((PLONG)&m_crefCount);\
        if (res == 0)\
        {\
           delete this;\
        }\
        return res;\
    }

#define BEGIN_INTERFACE_TABLE()\
    IMPLEMENT_REFCOUNT()\
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject)\
    {\
        if (riid == IID_IUnknown)\
        {\
            *ppvObject = reinterpret_cast<IUnknown*>(this);\
        }

#define IMPLEMENTS_INTERFACE(Itf)\
        else if (riid == IID_ ## Itf)\
        {\
            *ppvObject = static_cast<Itf*>(this);\
        }

#define END_INTERFACE_TABLE()\
        else \
        {\
           *ppvObject = NULL;\
            return E_NOINTERFACE;\
        }\
        \
        reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();\
        \
        return S_OK;\
    }

