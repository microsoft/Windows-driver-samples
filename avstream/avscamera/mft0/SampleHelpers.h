/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2013, Microsoft Corporation.

    File:

        SampleHelpers.h

    Abstract:

        Helper classes for the MFT0.

    History:

        created 4/26/2013

**************************************************************************/

#pragma once

/***************************************************************************\
*****************************************************************************
*
* class CAutoLock
*
* Locks a critical section at construction time and unlocks it automatically
* when the object goes out of scope
*
*****************************************************************************
\***************************************************************************/

class CAutoLock
{
private:
    // Make copy constructor and assignment operator inaccessible
    CAutoLock(const CAutoLock &refAutoLock);
    CAutoLock &operator=(const CAutoLock &refAutoLock);

    // CAutoLock should not be created on the heap, so declare
    // (but do not implement) private operator new/delete.
    static void *operator new(size_t);
    static void  operator delete(void *);
    static void *operator new[](size_t);
    static void  operator delete[](void *);

protected:
    CRITICAL_SECTION *m_pLock;

public:
    _Acquires_lock_(m_pLock)
    explicit CAutoLock(CRITICAL_SECTION *pLock):
        m_pLock(pLock)
    {
        EnterCriticalSection(m_pLock);
    }

    _Releases_lock_(m_pLock)
    ~CAutoLock()
    {
        LeaveCriticalSection(m_pLock);
    }
};

class MediaBufferLock
{
public:
    MediaBufferLock(_In_ IMFMediaBuffer *pBuffer):
        m_bLocked(false)
    {
        m_spBuffer = pBuffer;
    }

    HRESULT LockBuffer(
        _Outptr_result_bytebuffer_to_(*pcbMaxLength, *pcbCurrentLength)  BYTE **ppbBuffer,
        _Out_opt_  DWORD *pcbMaxLength,
        _Out_opt_  DWORD *pcbCurrentLength)
    {
        if(!m_spBuffer)
        {
            return E_INVALIDARG;
        }

        HRESULT hr = m_spBuffer->Lock(ppbBuffer, pcbMaxLength, pcbCurrentLength);
        if(FAILED(hr))
        {
            return hr;
        }
        m_bLocked = true;
        return S_OK;
    }

    ~MediaBufferLock()
    {
        if(m_spBuffer && m_bLocked)
        {
            //Unlock fails only if we did not lock it first
            (void) m_spBuffer->Unlock();
        }
    }

private:
    ComPtr<IMFMediaBuffer> m_spBuffer;
    bool m_bLocked;
};
