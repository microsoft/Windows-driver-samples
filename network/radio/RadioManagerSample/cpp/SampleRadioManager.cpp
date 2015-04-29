#include "precomp.h"
#pragma hdrstop

PCWSTR CSampleRadioManager::s_pszRootKey = L"SYSTEM\\CurrentControlSet\\Control\\RadioManagement\\Misc\\SampleRadioManager";

CSampleRadioManager::CSampleRadioManager()
    : _fClosing(false)
{
}

HRESULT CSampleRadioManager::FinalConstruct()
{
    DWORD    dwError;
    HRESULT  hr = S_OK;
    WCHAR    szSubKeyName[128];
    DWORD    cchSubKeyName;

    dwError = _hKeyRoot.Open(HKEY_LOCAL_MACHINE, s_pszRootKey, KEY_ALL_ACCESS);


    if (ERROR_SUCCESS == dwError)
    {

        // Enumerate all instances
        DWORD dwCount = 0;
        while (true)
        {
            cchSubKeyName = ARRAYSIZE(szSubKeyName);
            dwError = _hKeyRoot.EnumKey(dwCount, szSubKeyName, &cchSubKeyName, NULL);

            if (ERROR_SUCCESS == dwError)
            {
                CString strSubKeyName;
                _ATLTRY
                {
                    strSubKeyName.Format(L"%s\\%s", s_pszRootKey, szSubKeyName);
                }
                _ATLCATCH(e)
                {
                    hr = e;
                }
                if (SUCCEEDED(hr))
                {
                    hr = _AddInstance(strSubKeyName, nullptr);
                }
				
                dwCount++;
            }
            else
            {
                dwError = ERROR_SUCCESS;
                break;
            }
        }

    }
    
    // register the regkey subkey add/remove event
    if (ERROR_SUCCESS == dwError)
    {
        HANDLE hRegChangeEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (nullptr == hRegChangeEvent)
        {
            dwError = GetLastError();
        }
        else
        {
            _hRegChangeEvent.Attach(hRegChangeEvent);
        }
    }

    if (ERROR_SUCCESS == dwError)
    {
        HANDLE hThread = ::CreateThread(nullptr,
                                        0,
                                        CSampleRadioManager::s_ThreadRegChange,
                                        reinterpret_cast<PVOID>(this),
                                        0,
                                        nullptr);

        if (nullptr == hThread)
        {
            dwError = GetLastError();
        }
        else
        {
            _hEventThread.Attach(hThread);
        }
    }

    hr = HRESULT_FROM_WIN32(dwError);

    if (FAILED(hr))
    {
        _Cleanup();
    }

    return hr;
}

void CSampleRadioManager::FinalRelease()
{    
    if ((nullptr != _hRegChangeEvent) &&  (nullptr != _hEventThread))
    {
        {
            CComCritSecLock<CComAutoCriticalSection> lock(_criticalSection);

            _fClosing = true;
            SetEvent(_hRegChangeEvent); // inform reg listening thread obj is leaving
        }

        WaitForSingleObject(_hEventThread, INFINITE);  // wait reg listening thread finish
    }

    _Cleanup();
}

void CSampleRadioManager::_Cleanup()
{
    POSITION p;
    while (nullptr != (p = _listRadioInstances.GetHeadPosition()))
    {
        INSTANCE_LIST_OBJ *pListObj = _listRadioInstances.GetAt(p);
        if (nullptr != pListObj)
        {
            _listRadioInstances.SetAt(p, nullptr);
            _listRadioInstances.RemoveHeadNoReturn();
            delete pListObj;
        }
    }
}

IFACEMETHODIMP CSampleRadioManager::GetRadioInstances(_Out_ IRadioInstanceCollection **ppCollection)
{
    CAutoVectorPtr<IRadioInstance *> rgpIRadioInstance;
    HRESULT  hr = S_OK;
    DWORD    cInstance;
	
    if (nullptr == ppCollection)
    {
        return E_INVALIDARG;
    }

    *ppCollection = nullptr;

    CComCritSecLock<CComAutoCriticalSection> lock(_criticalSection);
    cInstance = static_cast<DWORD>(_listRadioInstances.GetCount());
    if (cInstance > 0)
    {
        if (!rgpIRadioInstance.Allocate(cInstance))
        {
            hr = E_OUTOFMEMORY;
        }
        if (SUCCEEDED(hr))
        {
            ZeroMemory(rgpIRadioInstance, sizeof(rgpIRadioInstance[0]) * cInstance);
            DWORD dwIndex = 0;

            for (POSITION p = _listRadioInstances.GetHeadPosition(); nullptr != p; _listRadioInstances.GetNext(p))
            {
                hr = (_listRadioInstances.GetAt(p))->spRadioInstance.QueryInterface(&(rgpIRadioInstance[dwIndex]));
                if (FAILED(hr))
                {
                     break;
                }
                else
                {
                    dwIndex++;
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = CRadioInstanceCollection_CreateInstance(cInstance, rgpIRadioInstance, ppCollection);
    }

    for (DWORD dwIndex = 0; dwIndex < cInstance; dwIndex++)
    {
        if (nullptr != rgpIRadioInstance[dwIndex])
        {
            rgpIRadioInstance[dwIndex]->Release();
        }
    }

    return hr;
}

IFACEMETHODIMP CSampleRadioManager::OnSystemRadioStateChange(
    _In_ SYSTEM_RADIO_STATE sysRadioState,
    _In_ UINT32 uTimeoutSec)
{
    HRESULT hr = S_OK;
    CAutoPtr<SET_SYS_RADIO_JOB> spSetSysRadioJob;
    bool fRefAdded = false;
	
    spSetSysRadioJob.Attach(new SET_SYS_RADIO_JOB);
    if (nullptr == spSetSysRadioJob)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        spSetSysRadioJob->hr = E_FAIL;
        spSetSysRadioJob->srsTarget = sysRadioState;
        spSetSysRadioJob->pSampleRM = this;

        // Add ref to object to avoid object release before working thread return
        this->AddRef();
        fRefAdded = true;

        HANDLE hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (nullptr == hEvent)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            spSetSysRadioJob->hEvent.Attach(hEvent);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!QueueUserWorkItem(CSampleRadioManager::s_ThreadSetSysRadio, spSetSysRadioJob, WT_EXECUTEDEFAULT))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwIgnore;
        hr = CoWaitForMultipleHandles(0,
                                      uTimeoutSec * 1000,
                                      1,
                                      reinterpret_cast<LPHANDLE>(&(spSetSysRadioJob->hEvent)),
                                      &dwIgnore);
        if (RPC_S_CALLPENDING == hr)
        {
             spSetSysRadioJob.Detach();
        }
        else
        {
            hr = spSetSysRadioJob->hr;
        }
    }

    if (fRefAdded)
    {
        this->Release();
    }

    return hr;
}

IFACEMETHODIMP CSampleRadioManager::OnInstanceRadioChange(
    _In_ BSTR bstrRadioInstanceID,
    _In_ DEVICE_RADIO_STATE radioState)
{
    return _FireEventOnInstanceRadioChange(bstrRadioInstanceID, radioState);
}

HRESULT CSampleRadioManager::_FireEventOnInstanceAdd(_In_ IRadioInstance *pRadioInstance)
{
    HRESULT hr;

    Lock();
    for (IUnknown** ppUnkSrc = m_vec.begin(); ppUnkSrc < m_vec.end(); ppUnkSrc++)
    {
        if ((nullptr != ppUnkSrc) && (nullptr != *ppUnkSrc))
        {
            CComPtr<IMediaRadioManagerNotifySink> spSink;

            hr = (*ppUnkSrc)->QueryInterface(IID_PPV_ARGS(&spSink));
            if (SUCCEEDED(hr))
            {
                spSink->OnInstanceAdd(pRadioInstance);
            }
        }
    }
    Unlock();

    return S_OK;
}

HRESULT CSampleRadioManager::_FireEventOnInstanceRemove(_In_ BSTR bstrRadioInstanceID)
{
    HRESULT hr;

    Lock();
    for (IUnknown** ppUnkSrc = m_vec.begin(); ppUnkSrc < m_vec.end(); ppUnkSrc++)
    {
        if ((nullptr != ppUnkSrc) && (nullptr != *ppUnkSrc))
        {
            CComPtr<IMediaRadioManagerNotifySink> spSink;

            hr = (*ppUnkSrc)->QueryInterface(IID_PPV_ARGS(&spSink));
            if (SUCCEEDED(hr))
            {
                spSink->OnInstanceRemove(bstrRadioInstanceID);
            }
        }
    }
    Unlock();

    return S_OK;
}

HRESULT CSampleRadioManager::_FireEventOnInstanceRadioChange(
    _In_ BSTR bstrRadioInstanceID,
    _In_ DEVICE_RADIO_STATE radioState
    )
{
    HRESULT hr;

    Lock();
    for (IUnknown** ppUnkSrc = m_vec.begin(); ppUnkSrc < m_vec.end(); ppUnkSrc++)
    {
        if ((nullptr != ppUnkSrc) && (nullptr != *ppUnkSrc))
        {
            CComPtr<IMediaRadioManagerNotifySink> spSink;

            hr = (*ppUnkSrc)->QueryInterface(IID_PPV_ARGS(&spSink));
            if (SUCCEEDED(hr))
            {
                spSink->OnInstanceRadioChange(bstrRadioInstanceID, radioState);
            }
        }
    }
    Unlock();

    return S_OK;
}


HRESULT CSampleRadioManager::_AddInstance(_In_ PCWSTR pszKeyName, _Out_opt_ IRadioInstance **ppRadioInstance)
{
    HRESULT hr = S_OK;
    CComPtr<ISampleRadioInstanceInternal> spRadioInstance;
    CAutoPtr<INSTANCE_LIST_OBJ> spInstanceObj;

    CComCritSecLock<CComAutoCriticalSection> lock(_criticalSection);

    spInstanceObj.Attach(new INSTANCE_LIST_OBJ);
    if (nullptr == spInstanceObj)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        CComPtr<ISampleRadioManagerInternal> spRMInternal = this;
        hr = CSampleRadioInstance_CreateInstance(pszKeyName, spRMInternal, &spRadioInstance);        
    }

    if (SUCCEEDED(hr))
    {
        spInstanceObj->fExisting = true;
        spInstanceObj->spRadioInstance = spRadioInstance;

        _ATLTRY
        {
            spInstanceObj->strRadioInstanceID = pszKeyName;
            _listRadioInstances.AddTail(spInstanceObj);
            spInstanceObj.Detach();
        }
        _ATLCATCH(e)
        {
            hr = e;
        }

        if (SUCCEEDED(hr))
        {
            if (ppRadioInstance != nullptr)
            {
                hr = spRadioInstance->QueryInterface(IID_PPV_ARGS(ppRadioInstance));
            }
        }
    }

    return hr;
}

void CSampleRadioManager::_OnInstanceAddRemove()
{
    DWORD    dwError;
    HRESULT  hr= S_OK;
    WCHAR    szSubKeyName[128];
    DWORD    cchSubKeyName;
    POSITION p;
	
    // Set all instances as invalid
    for (p = _listRadioInstances.GetHeadPosition(); p != nullptr; _listRadioInstances.GetNext(p))
    {
        INSTANCE_LIST_OBJ *pRadioInstanceObj = _listRadioInstances.GetAt(p);
        pRadioInstanceObj->fExisting = false;
    }

    DWORD dwCount = 0;
    while (true)
    {
        cchSubKeyName = ARRAYSIZE(szSubKeyName);
        dwError = _hKeyRoot.EnumKey(dwCount, szSubKeyName, &cchSubKeyName, NULL);

        if (ERROR_SUCCESS == dwError)
        {
            CString strKeyName;
            _ATLTRY
            {
                strKeyName.Format(L"%s\\%s",s_pszRootKey,szSubKeyName);
            }
            _ATLCATCH(e)
            {
                hr = e;
            }

            if (SUCCEEDED(hr))
            {
                for (p = _listRadioInstances.GetHeadPosition(); p != nullptr; _listRadioInstances.GetNext(p))
                {
                    INSTANCE_LIST_OBJ *pRadioInstanceObj = _listRadioInstances.GetAt(p);
                    if (pRadioInstanceObj->strRadioInstanceID == strKeyName)
                    {
                        break;
                    }
                }

                // find new instance
                if (nullptr == p)
                {
                    CComPtr<IRadioInstance> spRadioInstance;
                    hr = _AddInstance(strKeyName, &spRadioInstance);
                    if(SUCCEEDED(hr))
                    {
                        _FireEventOnInstanceAdd(spRadioInstance);
                    }                    
                }
                else
                {
                    // The instance still valid.
                    INSTANCE_LIST_OBJ *pRadioInstanceObj = _listRadioInstances.GetAt(p);
                    pRadioInstanceObj->fExisting = true;
                }
            }

            dwCount++;
        }
        else
        {
            dwError = ERROR_SUCCESS;
            break;
        }
    }

    // Remove deleted instance from list
    p = _listRadioInstances.GetHeadPosition();
    while (nullptr != p)
    {
        INSTANCE_LIST_OBJ *pRadioInstanceObj = _listRadioInstances.GetAt(p);
        if ((pRadioInstanceObj != nullptr) && !pRadioInstanceObj->fExisting)
        {
            POSITION pTmp = p;
            _listRadioInstances.GetPrev(pTmp);

            _listRadioInstances.RemoveAt(p);
            CComBSTR bstrTmp = pRadioInstanceObj->strRadioInstanceID.AllocSysString();
            if (nullptr != bstrTmp)
            {
               _FireEventOnInstanceRemove(bstrTmp);
            }
            delete pRadioInstanceObj;
            p = pTmp;
        }

        if (nullptr != p)
        {
            _listRadioInstances.GetNext(p);
        }
        else
        {
            p = _listRadioInstances.GetHeadPosition();
        }
    }
}

HRESULT CSampleRadioManager::_SetSysRadioState(_In_ SYSTEM_RADIO_STATE sysRadioState)
{
    // Since set regkey is fast enough, this implemtation is to set instance radio sequentially.
    // If set instance radio takes long time and there are multiple instance within one RM,
    // it is better to do it parallelly.
    HRESULT hr = S_OK;
    CComCritSecLock<CComAutoCriticalSection> lock(_criticalSection);
    for (POSITION p = _listRadioInstances.GetHeadPosition(); nullptr != p; _listRadioInstances.GetNext(p))
    {
        INSTANCE_LIST_OBJ *pInstanceObj = _listRadioInstances.GetAt(p);
        hr = pInstanceObj->spRadioInstance->OnSysRadioChange(sysRadioState);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

DWORD WINAPI CSampleRadioManager::s_ThreadRegChange(LPVOID pThis)
{
    CSampleRadioManager *pRadioMgr = reinterpret_cast<CSampleRadioManager *>(pThis);
    DWORD   dwError = ERROR_SUCCESS;

    {
        CComCritSecLock<CComAutoCriticalSection> lock(pRadioMgr->_criticalSection);
        if (pRadioMgr->_fClosing)
        {
            dwError = ERROR_APP_INIT_FAILURE;
        }
        else
        {
            dwError = pRadioMgr->_hKeyRoot.NotifyChangeKeyValue(FALSE,
                                                                REG_NOTIFY_CHANGE_NAME,
                                                                pRadioMgr->_hRegChangeEvent,
                                                                TRUE);
        }
    }

    if (ERROR_SUCCESS != dwError)
    {
        return dwError;
    }

    while (ERROR_SUCCESS == dwError)
    {
        if (WaitForSingleObject(pRadioMgr->_hRegChangeEvent, INFINITE) == WAIT_FAILED)
        {
            break;
        }

        {
            CComCritSecLock<CComAutoCriticalSection> lock(pRadioMgr->_criticalSection);
            if (pRadioMgr->_fClosing) // main thread is leaving
            {
                break;
            }

            ResetEvent(pRadioMgr->_hRegChangeEvent);

            dwError = pRadioMgr->_hKeyRoot.NotifyChangeKeyValue(FALSE,
                                                                REG_NOTIFY_CHANGE_NAME,
                                                                pRadioMgr->_hRegChangeEvent,
                                                                TRUE);
        }
        // Receive reg subkey add/remove event. update list
        pRadioMgr->_OnInstanceAddRemove();
    }

    return dwError;
}

DWORD WINAPI CSampleRadioManager::s_ThreadSetSysRadio(LPVOID pThis)
{
    SET_SYS_RADIO_JOB *pJob = reinterpret_cast<SET_SYS_RADIO_JOB *>(pThis);

    pJob->hr = pJob->pSampleRM->_SetSysRadioState(pJob->srsTarget);

    SetEvent(pJob->hEvent);
    return ERROR_SUCCESS;
}

