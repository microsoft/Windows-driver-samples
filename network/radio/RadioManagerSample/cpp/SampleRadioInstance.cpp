#include "precomp.h"
#pragma hdrstop

HRESULT CSampleRadioInstance_CreateInstance(
    _In_                            PCWSTR pszKeyName,
    _In_                            ISampleRadioManagerInternal *pParentManager,
    _COM_Outptr_                    ISampleRadioInstanceInternal **ppRadioInstance)
{
	return CSampleRadioInstance::CreateInstance(pszKeyName, pParentManager, ppRadioInstance);
}

PCWSTR CSampleRadioInstance::s_pszInstanceName = L"Name";
PCWSTR CSampleRadioInstance::s_pszInstanceRadioState = L"RadioState";
PCWSTR CSampleRadioInstance::s_pszPreviousRadioState = L"PreviousRadioState";
PCWSTR CSampleRadioInstance::s_pszMultiComm = L"IsMultiComm";
PCWSTR CSampleRadioInstance::s_pszAssocatingDevice = L"IsAssociatingDevice";

CSampleRadioInstance::CSampleRadioInstance()
    : _pParentManager(nullptr),
      _fClosing(false),
      _fIsMultiCommDevice(false)
{
}

void CSampleRadioInstance::FinalRelease()
{
    if ((nullptr != _hRegChangeEvent) && (nullptr != _hEventThread))
    {
        {
            // Critical section scope.
            CComCritSecLock<CComAutoCriticalSection> lock(_criticalSection);
            _fClosing = true;
            SetEvent(_hRegChangeEvent);
        }
        WaitForSingleObject(_hEventThread, INFINITE);
    }

    _Cleanup();
}

// static
HRESULT CSampleRadioInstance::CreateInstance(
    _In_  PCWSTR pszKeyName,
    _In_  ISampleRadioManagerInternal *pParentManager,
    _COM_Outptr_ ISampleRadioInstanceInternal **ppRadioInstance)
{
    HRESULT hr;
    CComObject<CSampleRadioInstance> *pRadioInstance;
    CComPtr<ISampleRadioInstanceInternal> spRadioInstanceInternal;

    *ppRadioInstance = nullptr;

     hr = CComObject<CSampleRadioInstance>::CreateInstance(&pRadioInstance);
     if (SUCCEEDED(hr) && (pRadioInstance != nullptr))
     {
         hr = pRadioInstance->QueryInterface(IID_PPV_ARGS(&spRadioInstanceInternal));
     }
     if (SUCCEEDED(hr)  && (pRadioInstance != nullptr))
     {
         hr = pRadioInstance->_Init(pszKeyName, pParentManager);
     }
     if (SUCCEEDED(hr))
     {
         *ppRadioInstance = spRadioInstanceInternal.Detach();
     }

     return hr;
}

HRESULT CSampleRadioInstance::_Init(_In_ PCWSTR pszKeyName, _In_ ISampleRadioManagerInternal *pParentManager)
{
    HRESULT hr;
    DWORD   dwError;
    WCHAR   szInstanceName[128] = {};
    DWORD   cchInstanceName = ARRAYSIZE(szInstanceName);
	
    _ATLTRY
    {
        _strInstanceId = pszKeyName;
    }
    _ATLCATCH(e)
    {
        return e;
    }

    _pParentManager = pParentManager;

    dwError = _hKeyRoot.Open(HKEY_LOCAL_MACHINE, _strInstanceId, KEY_ALL_ACCESS);


    if (ERROR_SUCCESS == dwError)
    {
        dwError = _hKeyRoot.QueryStringValue(s_pszInstanceName, szInstanceName, &cchInstanceName);

        if (ERROR_SUCCESS == dwError)
        {
            _ATLTRY
            {
                _strInstanceName = szInstanceName;
            }
            _ATLCATCH(e)
            {
                UNREFERENCED_PARAMETER(e);
                dwError = ERROR_OUTOFMEMORY;
            }

            if (ERROR_SUCCESS == dwError)
            {
                DWORD dwTemp = 0;
                dwError = _hKeyRoot.QueryDWORDValue(s_pszMultiComm, dwTemp);
                // only when key read successfully with non zero value, we regard device as multicomm chip
                if ((ERROR_SUCCESS == dwError) && (0 != dwTemp))
                {
                    _fIsMultiCommDevice = true;
                }

				// ignore the case when there is no IsMultiComm value in the registry, set ERROR_SUCCESS to continue with _Init
				dwError = ERROR_SUCCESS;
            }            
        }
    }    

    // register the regkey value change event
    if (ERROR_SUCCESS == dwError)
    {
        HANDLE hRegChangeEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if ((NULL == hRegChangeEvent) || (INVALID_HANDLE_VALUE == hRegChangeEvent))
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
        HANDLE hThread= ::CreateThread(nullptr,
                                       0,
                                       CSampleRadioInstance::s_ThreadInstanceChange,
                                       reinterpret_cast<PVOID>(this),
                                       0,
                                       nullptr);

        if ((NULL == hThread) || (INVALID_HANDLE_VALUE == hThread))
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

void CSampleRadioInstance::_Cleanup()
{
    if (nullptr != _pParentManager)
    {
        _pParentManager = nullptr;
    }
}

IFACEMETHODIMP CSampleRadioInstance::GetRadioManagerSignature(_Out_ GUID *pguidSignature)
{
    if (nullptr == pguidSignature)
    {
        return E_INVALIDARG;
    }

    *pguidSignature = __uuidof(SampleRadioManager);

    return S_OK;
}

IFACEMETHODIMP CSampleRadioInstance::GetInstanceSignature(_Out_ BSTR *pbstrID)
{
    if (nullptr == pbstrID)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *pbstrID = _strInstanceId.AllocSysString();
    if (nullptr == *pbstrID)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

IFACEMETHODIMP CSampleRadioInstance::GetFriendlyName(_In_ LCID /* lcid*/, _Out_ BSTR *pbstrName)
{
    if (nullptr == pbstrName)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *pbstrName = _strInstanceName.AllocSysString();
    if (nullptr == *pbstrName)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

IFACEMETHODIMP CSampleRadioInstance::GetRadioState(_Out_ DEVICE_RADIO_STATE *pRadioState)
{
    if (nullptr == pRadioState)
    {
        return E_INVALIDARG;
    }

    DWORD              dwRadioState;
    DEVICE_RADIO_STATE radioState = DRS_RADIO_ON;

    *pRadioState = DRS_RADIO_ON;

    // Read radio state directly from device. we do not save radio state in our module to avoid out of sync
    DWORD dwError = _hKeyRoot.QueryDWORDValue(s_pszInstanceRadioState, dwRadioState);

    if (ERROR_SUCCESS == dwError)
    {
        if ((dwRadioState > DRS_RADIO_MAX) || (dwRadioState == DRS_RADIO_INVALID))
        {
            radioState = DRS_HW_RADIO_ON_UNCONTROLLABLE;
        }
        else
        {
            radioState = static_cast<DEVICE_RADIO_STATE>(dwRadioState);
        }

        *pRadioState = radioState;
    }    

    return HRESULT_FROM_WIN32(dwError);
}


IFACEMETHODIMP CSampleRadioInstance::SetRadioState(_In_ DEVICE_RADIO_STATE radioState, _In_ UINT32 uTimeoutSec)
{
    if ((radioState != DRS_RADIO_ON) && (radioState != DRS_SW_RADIO_OFF))
    {
        return E_INVALIDARG; // invalid input
    }

    DEVICE_RADIO_STATE drsCurrent;
    bool fRefAdded = false;
    HRESULT hr = GetRadioState(&drsCurrent);

    // fail to get current radio state or current radio state is uncontrollable
    if (FAILED(hr) || (drsCurrent & DRS_HW_RADIO_ON_UNCONTROLLABLE))
    {
        return E_FAIL;
    }

    if ((drsCurrent & DRS_SW_RADIO_OFF) == radioState)
    {
        return S_OK;    // current software radio is same as target software radio
    }

    // Keep all other bits same, only change the last bit of radio state to represent software radio change
    DEVICE_RADIO_STATE drsState = static_cast<DEVICE_RADIO_STATE>((drsCurrent & DRS_HW_RADIO_OFF) | radioState);

    CAutoPtr<SET_DEVICE_RADIO_JOB> spSetDeviceRadioJob;

    spSetDeviceRadioJob.Attach(new SET_DEVICE_RADIO_JOB);
    if (nullptr == spSetDeviceRadioJob)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        spSetDeviceRadioJob->hr = E_FAIL;
        spSetDeviceRadioJob->drsTarget = drsState;
        spSetDeviceRadioJob->pInstance = this;

        // Let working thread hold ref on object such that object won't be released before working thread return.
        this->AddRef();
        fRefAdded = true;

        HANDLE hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (nullptr == hEvent)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            spSetDeviceRadioJob->hEvent.Attach(hEvent);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!QueueUserWorkItem(CSampleRadioInstance::s_ThreadSetRadio, spSetDeviceRadioJob, WT_EXECUTEDEFAULT))
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
                                      reinterpret_cast<LPHANDLE>(&(spSetDeviceRadioJob->hEvent)),
                                      &dwIgnore);
        if (RPC_S_CALLPENDING == hr)
        {
            spSetDeviceRadioJob.Detach();
        }
        else
        {
            hr = spSetDeviceRadioJob->hr;
        }
    }

    if (fRefAdded)
    {
        this->Release();
    }
    return hr;
}

IFACEMETHODIMP_(BOOL) CSampleRadioInstance::IsMultiComm()
{
    return _fIsMultiCommDevice ? TRUE : FALSE;
}

IFACEMETHODIMP_(BOOL) CSampleRadioInstance::IsAssociatingDevice()
{
    BOOL   fIsAssociatingDevice = FALSE;
    DWORD  dwAssociatingDevice = 0;

    DWORD dwError = _hKeyRoot.QueryDWORDValue(s_pszAssocatingDevice, dwAssociatingDevice);

    // When key read successfully with non zero value, we regard device associating now.
    if ((ERROR_SUCCESS == dwError) && (0 != dwAssociatingDevice))
    {
        fIsAssociatingDevice = TRUE;
    }

    return fIsAssociatingDevice;
}

IFACEMETHODIMP CSampleRadioInstance::OnSysRadioChange(_In_ SYSTEM_RADIO_STATE sysRadioState)
{
    if ((sysRadioState != SRS_RADIO_ENABLED) && (sysRadioState != SRS_RADIO_DISABLED))
    {
        return E_INVALIDARG;
    }

    DEVICE_RADIO_STATE drsCurrent;
    DWORD              dwError = ERROR_SUCCESS;
    DEVICE_RADIO_STATE drsTarget = DRS_RADIO_ON;
    DEVICE_RADIO_STATE drsPrevious = DRS_RADIO_ON;
    DWORD              dwPreviousState;
    bool               fSetRadioState = false;

    HRESULT hr = GetRadioState(&drsCurrent);

    // fail to get current radio state or current radio state is uncontrollable, ignore this call
    if (FAILED(hr) || (drsCurrent & DRS_HW_RADIO_ON_UNCONTROLLABLE))
    {
        return S_OK;
    }

    if (SRS_RADIO_ENABLED == sysRadioState)
    {
        // If device current software radio is already on during system radio enabled,
        // we ignore the saved previous radio state and do nothing.
        // radio state will be updated only when current state is off during system radio enabling.
        if (DRS_RADIO_ON != (drsCurrent & DRS_SW_RADIO_OFF))
        {
            // System radio enable, need get previous radio state and set to previous radio state.
            // If fail to get previous radio state, assume previous radio state is software radio on.
            dwError = _hKeyRoot.QueryDWORDValue(s_pszPreviousRadioState, dwPreviousState);

            if (ERROR_SUCCESS == dwError)
            {
                if ((DRS_RADIO_ON == dwPreviousState) || (DRS_SW_RADIO_OFF == dwPreviousState))
                {
                    drsPrevious = static_cast<DEVICE_RADIO_STATE>(dwPreviousState);
                }
            }

            if ((drsCurrent & DRS_SW_RADIO_OFF) != drsPrevious)
            {
                drsTarget = static_cast<DEVICE_RADIO_STATE>((drsCurrent & DRS_HW_RADIO_OFF) | drsPrevious);
                fSetRadioState = true;
            }
        }
    }
    else
    {   // System radio disable, need record previous radio state.
        _hKeyRoot.SetDWORDValue(s_pszPreviousRadioState, static_cast<DWORD>(drsCurrent));

        if ((drsCurrent & DRS_SW_RADIO_OFF) != DRS_SW_RADIO_OFF)
        {
            drsTarget = static_cast<DEVICE_RADIO_STATE>((drsCurrent & DRS_HW_RADIO_OFF) | DRS_SW_RADIO_OFF);
            fSetRadioState = true;
        }
    }

    if (fSetRadioState)
    {
        dwError = _hKeyRoot.SetDWORDValue(s_pszInstanceRadioState, static_cast<DWORD>(drsTarget));
    }

    return HRESULT_FROM_WIN32(dwError);
}

HRESULT CSampleRadioInstance::_SetRadioState(_In_ DEVICE_RADIO_STATE radioState)
{
    DWORD dwError = _hKeyRoot.SetDWORDValue(s_pszInstanceRadioState, static_cast<DWORD>(radioState));

    return HRESULT_FROM_WIN32(dwError);
}

void CSampleRadioInstance::_OnInstanceUpdate()
{
    DWORD   dwError;
    DWORD   dwRadioState = 0;
    DEVICE_RADIO_STATE drsCurrent;

    dwError = _hKeyRoot.QueryDWORDValue(s_pszInstanceRadioState, dwRadioState);

    if (ERROR_SUCCESS == dwError)
    {
        if ((dwRadioState > DRS_RADIO_MAX) || (dwRadioState == DRS_RADIO_INVALID))
        {
            drsCurrent = DRS_HW_RADIO_ON_UNCONTROLLABLE;
        }
        else
        {
            drsCurrent = static_cast<DEVICE_RADIO_STATE>(dwRadioState);
        }
        CComBSTR sbstrID = _strInstanceId.AllocSysString();

        if (nullptr != sbstrID)
        {
            _pParentManager->OnInstanceRadioChange(sbstrID, drsCurrent);
        }
   }
}

DWORD WINAPI CSampleRadioInstance::s_ThreadInstanceChange(LPVOID pThis)
{
    CSampleRadioInstance *pRadioInstance = reinterpret_cast<CSampleRadioInstance *>(pThis);
    DWORD  dwError = ERROR_SUCCESS;

    {
        CComCritSecLock<CComAutoCriticalSection> lock(pRadioInstance->_criticalSection);
        if (pRadioInstance->_fClosing)
        {
            dwError = ERROR_APP_INIT_FAILURE;
        }
        else
        {
            dwError = pRadioInstance->_hKeyRoot.NotifyChangeKeyValue(FALSE,
                                                                     REG_NOTIFY_CHANGE_LAST_SET,
                                                                     pRadioInstance->_hRegChangeEvent,
                                                                     TRUE);
        }
    }

    if (ERROR_SUCCESS != dwError)
    {
        return dwError;
    }


    while (ERROR_SUCCESS == dwError)
    {
        WaitForSingleObject(pRadioInstance->_hRegChangeEvent, INFINITE);

        {
            CComCritSecLock<CComAutoCriticalSection> lock(pRadioInstance->_criticalSection);
            if (pRadioInstance->_fClosing)
            {
                break;
            }

            ResetEvent(pRadioInstance->_hRegChangeEvent);

            dwError = pRadioInstance->_hKeyRoot.NotifyChangeKeyValue(FALSE,
                                                                     REG_NOTIFY_CHANGE_LAST_SET,
                                                                     pRadioInstance->_hRegChangeEvent,
                                                                     TRUE);
        }
        pRadioInstance->_OnInstanceUpdate();
    }

    return dwError;
}

DWORD WINAPI CSampleRadioInstance::s_ThreadSetRadio(LPVOID pThis)
{
    SET_DEVICE_RADIO_JOB *pJob = reinterpret_cast<SET_DEVICE_RADIO_JOB *>(pThis);

    pJob->hr = pJob->pInstance->_SetRadioState(pJob->drsTarget);

    SetEvent(pJob->hEvent);
    return ERROR_SUCCESS;
}
