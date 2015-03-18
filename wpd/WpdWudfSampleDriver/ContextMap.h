#pragma once

class ContextMap : public IUnknown
{
public:
    ContextMap() :
        m_cRef(1)
    {

    }

    ~ContextMap()
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);

        IUnknown*   pUnk            = NULL;
        POSITION    elementPosition = NULL;
        elementPosition = m_Map.GetStartPosition();
        while(elementPosition != NULL)
        {
            pUnk = m_Map.GetNextValue(elementPosition);

            if(pUnk != NULL)
            {
                pUnk->Release();
            }
        }
    }

public: // IUnknown
    ULONG __stdcall AddRef()
    {
        InterlockedIncrement((long*) &m_cRef);
        return m_cRef;
    }

    _At_(this, __drv_freesMem(Mem)) 
    ULONG __stdcall Release()
    {
        ULONG ulRefCount = m_cRef - 1;

        if (InterlockedDecrement((long*) &m_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return ulRefCount;
    }

    HRESULT __stdcall QueryInterface(
        REFIID riid,
        void** ppv)
    {
        HRESULT hr = S_OK;

        if(riid == IID_IUnknown)
        {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
        }
        else
        {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }

        return hr;
    }


public: // Context accessor methods

    // If successfull, this method AddRef's the context
    HRESULT Add(
        _In_ const CAtlStringW&  key,
        _In_ IUnknown*           pContext)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        HRESULT hr = S_OK;

        // Insert this into the map
        POSITION    elementPosition = m_Map.SetAt(key, pContext);
        if(elementPosition != NULL)
        {
            // AddRef since we are holding onto it
            pContext->AddRef();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        return hr;
    }

    void Remove(
        _In_ const CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        // Get the element
        IUnknown* pContext = NULL;

        if (m_Map.Lookup(key, pContext) == true)
        {
            // Remove the entry for it
            m_Map.RemoveKey(key);

            // Release it
            pContext->Release();
        }
    }

    // Returns the context pointer.  If not found, return value is NULL.
    // If non-NULL, caller is responsible for Releasing when it is done,
    // since this method will AddRef the context.
    IUnknown* GetContext(
        _In_ const CAtlStringW&  key)
    {
        CComCritSecLock<CComAutoCriticalSection> Lock(m_CriticalSection);
        // Get the element
        IUnknown* pContext = NULL;

        if (m_Map.Lookup(key, pContext) == true)
        {
            // AddRef
            pContext->AddRef();
        }
        return pContext;
    }

private:
    CComAutoCriticalSection         m_CriticalSection;
    CAtlMap<CAtlStringW, IUnknown*> m_Map;
    DWORD                           m_cRef;
};
