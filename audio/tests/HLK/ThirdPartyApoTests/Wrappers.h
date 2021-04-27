//
// Wrappers.h -- Copyright (c) Microsoft Corporation
//
// Author: zaneh
//
// Description:
//
//   Wrapper methods for APIs which can throw
//

// Collects exception data and sends it to Watson without crashing Audio DG.
// This method can only be called from inside the __except condition statement
// as a filter function.
inline DWORD CollectExceptionDataAndContinue(LPEXCEPTION_POINTERS exceptionInfo)
{
    LONG lResult;
    DWORD dwType, dwVal, dwSize=sizeof(DWORD);

    // Check to see if we should allow the process create a crash dump
    lResult = RegGetValue(HKEY_LOCAL_MACHINE, REGKEY_AUDIO_SOFTWARE, PREVENT_APOTEST_CRASH_OR_REPORT_ON_APO_EXCEPTION, RRF_RT_DWORD, &dwType, &dwVal, &dwSize);

    // If we are allowing dumps, report an exception to Watson before continuing
    if ((lResult != ERROR_SUCCESS) || (dwVal == 0))
    {
        RtlReportException(exceptionInfo->ExceptionRecord, exceptionInfo->ContextRecord, RTL_WER_NO_DEBUG | RTL_WER_NON_FATAL);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

HRESULT QIInternal(IAudioProcessingObject* pIAPO, _In_ REFIID riid, _COM_Outptr_ void** ppvObject)
{
    __try
    {
        return pIAPO->QueryInterface(riid, ppvObject);
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return __HRESULT_FROM_WIN32(GetExceptionCode());
    }
}

HRESULT UnknownQIInternal(IUnknown* pIAPO, _In_ REFIID riid, _COM_Outptr_ void** ppvObject)
{
    __try
    {
        return pIAPO->QueryInterface(riid, ppvObject);
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return __HRESULT_FROM_WIN32(GetExceptionCode());
    }
}

HRESULT IsOutputFormatSupported(IAudioProcessingObject* pIAPO, IAudioMediaType* pInputFormat, IAudioMediaType* pRequestedOutputFormat, IAudioMediaType** ppSupportedOutputFormat)
{
    __try
    {
        return pIAPO->IsOutputFormatSupported(pInputFormat, pRequestedOutputFormat, ppSupportedOutputFormat);
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return __HRESULT_FROM_WIN32(GetExceptionCode());
    }
}

HRESULT LockForProcess(IAudioProcessingObjectConfiguration* pIAPOConfig,
        _In_ UINT32 u32NumInputConnections, 
        _In_reads_(u32NumInputConnections) APO_CONNECTION_DESCRIPTOR** ppInputConnections,
        _In_ UINT32 u32NumOutputConnections, 
        _In_reads_(u32NumOutputConnections) APO_CONNECTION_DESCRIPTOR** ppOutputConnections)
{
    __try
    {
        return pIAPOConfig->LockForProcess(u32NumInputConnections, ppInputConnections, u32NumOutputConnections, ppOutputConnections);
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return __HRESULT_FROM_WIN32(GetExceptionCode());
    }
}

HRESULT UnlockForProcess(IAudioProcessingObjectConfiguration* pIAPOConfig)
{
    __try
    {
        return pIAPOConfig->UnlockForProcess();
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return __HRESULT_FROM_WIN32(GetExceptionCode());
    }
}

void APOProcess(IAudioProcessingObjectRT* pIAPORT,
        _In_ UINT32 u32NumInputConnections,
        _In_reads_(u32NumInputConnections) APO_CONNECTION_PROPERTY** ppInputConnections,
        _In_ UINT32 u32NumOutputConnections,
        _Inout_updates_(u32NumOutputConnections) APO_CONNECTION_PROPERTY** ppOutputConnections)
{
    __try
    {
        pIAPORT->APOProcess(u32NumInputConnections, ppInputConnections, u32NumOutputConnections, ppOutputConnections);
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return;
    }
}

HRESULT GetRegistrationProperties(IAudioProcessingObject* pIAPO, APO_REG_PROPERTIES** ppRegProps)
{
    __try
    {
        return pIAPO->GetRegistrationProperties(ppRegProps);
    }
    __except (CollectExceptionDataAndContinue(GetExceptionInformation()))
    {
        return __HRESULT_FROM_WIN32(GetExceptionCode());
    }
}