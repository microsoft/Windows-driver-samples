/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   transform.cpp

Abstract:

   CTransform class implementation. This class creates and manages color transforms providing
   limited caching functionality based off the source profile keys.

   This file also contains a utility class for handling profile lists.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "xdexcept.h"
#include "transform.h"

class CProfileList
{
public:
    /*++

    Routine Name:

        CProfileList

    Routine Description:

        CProfileList constructor

    Arguments:

        pProfiles - pointer to a vector of profiles

    Return Value:

        None
        Throws an exception on error.

    --*/
    CProfileList(
        _In_ ProfileList* pProfiles
        ) :
        m_cProfiles(0),
        m_phProfiles(NULL)
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pProfiles, E_POINTER)))
        {
            hr = CreateHandleBuffer(pProfiles);
        }

        if (FAILED(hr))
        {
            throw CXDException(hr);
        }
    }

    /*++

    Routine Name:

        ~CProfileList

    Routine Description:

        CProfileList destructor

    Arguments:

        None

    Return Value:

        None

    --*/
    ~CProfileList()
    {
        FreeHandleBuffer();
    }

    /*++

    Routine Name:

        GetProfileData

    Routine Description:

        Retrieves

    Arguments:

        pphProfiles - Pointer to pointer that recieves the address of an array of profile handles
                      Note: the buffer is only valid for the lifetime of the CProfileList object.
        pcProfiles  - Pointer to storage that recieves the count of profiles in the array

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    GetProfileData(
        _When_(*pcProfiles >  0, _Outptr_result_buffer_(*pcProfiles)) 
        _When_(*pcProfiles == 0, _Outptr_result_maybenull_)
                                                                HPROFILE** pphProfiles,
        _Out_                                                   DWORD*     pcProfiles
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pphProfiles, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pcProfiles, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(m_phProfiles, E_PENDING)))
        {
            *pphProfiles = 0;
            *pcProfiles = 0;

            if (m_cProfiles > 0)
            {
                *pphProfiles = m_phProfiles;
                *pcProfiles = m_cProfiles;
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }

private:
    /*++

    Routine Name:

        CreateHandleBuffer

    Routine Description:

        Creates the buffer that holds the profile handles

    Arguments:

        pProfiles - Pointer to the vector of CProfile objects that have the individual profile data

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT
    CreateHandleBuffer(
        _In_  ProfileList* pProfiles
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pProfiles, E_POINTER)))
        {
            try
            {
                FreeHandleBuffer();

                m_cProfiles = static_cast<DWORD>(pProfiles->size());
                if (m_cProfiles > 0)
                {
                    m_phProfiles = new(std::nothrow) HPROFILE[m_cProfiles];

                    if (SUCCEEDED(hr = CHECK_POINTER(m_phProfiles, E_OUTOFMEMORY)))
                    {
                        ProfileList::iterator iterProfiles = pProfiles->begin();

                        for (UINT cProfile = 0;
                             SUCCEEDED(hr) && iterProfiles != pProfiles->end() && cProfile < m_cProfiles;
                             iterProfiles++, cProfile++)
                        {
                            hr = (*iterProfiles)->GetProfileHandle(&m_phProfiles[cProfile]);
                        }
                    }
                }
            }
            catch (exception& DBG_ONLY(e))
            {
                ERR(e.what());
                hr = E_FAIL;
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        FreeHandleBuffer

    Routine Description:

        Frees up the profile handle buffer

    Arguments:

        None

    Return Value:

        None

    --*/
    VOID
    FreeHandleBuffer(
        VOID
        )
    {
        if (m_phProfiles != NULL)
        {
            delete[] m_phProfiles;
            m_phProfiles = NULL;
        }

        m_cProfiles = 0;
    }

private:
    HPROFILE*   m_phProfiles;

    DWORD       m_cProfiles;
};

/*++

Routine Name:

    CTransform::CTransform

Routine Description:

    CTransform constructor

Arguments:

    None

Return Value:

    None

--*/
CTransform::CTransform() :
    m_hColorTrans(NULL),
    m_intents(INTENT_ABSOLUTE_COLORIMETRIC),
    m_renderFlags(0),
    m_pcstrProfileKeys(NULL),
    m_cProfiles(0)
{
}

/*++

Routine Name:

    CTransform::~CTransform

Routine Description:

    CTransform destructor

Arguments:

    None

Return Value:

    None

--*/
CTransform::~CTransform()
{
    FreeTransform();
}

/*++

Routine Name:

    CTransform::CreateTransform

Routine Description:

    Creates the transform from a vector of CProfile objects

Arguments:

    pProfiles   - Pointer to the vector of CProfile objects that have the individual profile data
    intent      - Intent flags to be applied when creating transform
    renderFlags - Render flags to be applied when creating transform

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CTransform::CreateTransform(
    _In_ ProfileList* pProfiles,
    _In_ CONST DWORD  intent,
    _In_ CONST DWORD  renderFlags
    )
{
    HRESULT hr = S_OK;

    if (intent <= INTENT_ABSOLUTE_COLORIMETRIC)
    {
        BOOL bFreeTransform = FALSE;

        if (m_intents != intent)
        {
            m_intents = intent;
            bFreeTransform = TRUE;
        }

        if (m_renderFlags != renderFlags)
        {
            m_renderFlags = renderFlags;
            bFreeTransform = TRUE;
        }

        if (bFreeTransform)
        {
            FreeTransform();
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    BOOL bUpdate = FALSE;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = CHECK_POINTER(pProfiles, E_POINTER)) &&
        SUCCEEDED(hr = UpdateProfiles(pProfiles, &bUpdate)))
    {
        try
        {
            if (bUpdate)
            {
                HPROFILE* phProfiles = NULL;
                DWORD     cProfiles = 0;
                CProfileList profileList(pProfiles);

                if (SUCCEEDED(hr = profileList.GetProfileData(&phProfiles, &cProfiles)))
                {
                    m_hColorTrans = CreateMultiProfileTransform(phProfiles,
                                                                cProfiles,
                                                                &m_intents,
                                                                1,
                                                                m_renderFlags,
                                                                INDEX_DONT_CARE);

                    if (m_hColorTrans == NULL)
                    {
                        hr = GetLastErrorAsHResult();
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CTransform::GetTransformHandle

Routine Description:

    Retrieves the current transform handle

Arguments:

    phTrans - Pointer to a HTRANSFORM that recieves the transform handle

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CTransform::GetTransformHandle(
    _Out_ HTRANSFORM* phTrans
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(phTrans, E_POINTER)))
    {
        *phTrans = NULL;

        if (SUCCEEDED(hr = CHECK_HANDLE(m_hColorTrans, E_PENDING)))
        {
            *phTrans = m_hColorTrans;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CTransform::FreeTransform

Routine Description:

    Free the current transform

Arguments:

    None

Return Value:

    None

--*/
VOID
CTransform::FreeTransform(
    VOID
    )
{
    FreeProfileKeysBuffer();
    if (m_hColorTrans != NULL)
    {
        DeleteColorTransform(m_hColorTrans);
        m_hColorTrans = NULL;
    }
}

/*++

Routine Name:

    CTransform::CreateProfileKeysBuffer

Routine Description:

    Creates a buffer to recieve the keys to the profiles that compse the current
    trnasform

Arguments:

    cProfiles - the count of profiles that compose the transform

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CTransform::CreateProfileKeysBuffer(
    _In_ CONST UINT& cProfiles
    )
{
    HRESULT hr = S_OK;

    FreeProfileKeysBuffer();

    if (cProfiles > 0)
    {
        m_pcstrProfileKeys = new(std::nothrow) CStringXDW[cProfiles];
        if (SUCCEEDED(hr = CHECK_POINTER(m_pcstrProfileKeys, E_OUTOFMEMORY)))
        {
            m_cProfiles = cProfiles;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CTransform::FreeProfileKeysBuffer

Routine Description:

    Frees the profile key buffer

Arguments:

    None

Return Value:

    None

--*/
VOID
CTransform::FreeProfileKeysBuffer(
    VOID
    )
{
    if (m_pcstrProfileKeys != NULL)
    {
        delete[] m_pcstrProfileKeys;
        m_pcstrProfileKeys = NULL;
    }

    m_cProfiles = 0;
}

/*++

Routine Name:

    CTransform::UpdateProfiles

Routine Description:

    Updates the profile list and checks whether the transform needs to be generated
    (i.e. is the current transform the same as the requested transform)

Arguments:

    pProfiles - Pointer to the vector of CProfile objects that have the individual profile data
    pbUpdate  - Pointer to BOOL that recieves whether the transform requires updating

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CTransform::UpdateProfiles(
    _In_  ProfileList* pProfiles,
    _Out_ BOOL*        pbUpdate
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pProfiles, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pbUpdate, E_POINTER)))
    {
        *pbUpdate = FALSE;

        try
        {
            //
            // If we already have a transform, a list of profile keys and matching
            // number of profiles, check our keys against the incoming list of profiles
            // and note if we need to create a new transform
            //
            CStringXDW* pcstrProfileKeys = NULL;
            if (m_hColorTrans != NULL &&
                m_pcstrProfileKeys != NULL &&
                m_cProfiles > 0 &&
                m_cProfiles == static_cast<UINT>(pProfiles->size()))
            {
                pcstrProfileKeys = m_pcstrProfileKeys;
                ProfileList::iterator iterProfiles = pProfiles->begin();

                for (;
                     iterProfiles != pProfiles->end() && !*pbUpdate;
                     iterProfiles++)
                {
                    if (*(*iterProfiles) != *pcstrProfileKeys++)
                    {
                        *pbUpdate = TRUE;
                    }
                }
            }
            else
            {
                *pbUpdate = TRUE;
            }

            if (*pbUpdate)
            {
                //
                // We need to create a new transform. Free any current transform and cache the keys
                // to the profiles that constitute the transform
                //
                FreeTransform();
                if (SUCCEEDED(hr = CreateProfileKeysBuffer(static_cast<UINT>(pProfiles->size()))))
                {
                    pcstrProfileKeys = m_pcstrProfileKeys;
                    ProfileList::iterator iterProfiles = pProfiles->begin();

                    for (;
                         SUCCEEDED(hr) && iterProfiles != pProfiles->end();
                         iterProfiles++, pcstrProfileKeys++)
                    {
                        //
                        // If CreateProfileKeysBuffer succeeds, it fills
                        // in m_pcstrProfileKeys with pProfiles->size()
                        // elements. Hence the _Analysis_assume_.
                        //
                        _Analysis_assume_(pcstrProfileKeys != NULL);
                        hr = (*iterProfiles)->GetProfileKey(pcstrProfileKeys);
                    }
                }
            }
        }
        catch (exception& DBG_ONLY(e))
        {
            ERR(e.what());
            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

