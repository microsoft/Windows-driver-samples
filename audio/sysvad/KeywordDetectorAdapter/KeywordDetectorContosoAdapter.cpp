// KeywordDetectorContosoAdapter.cpp : Defines the exported functions for the DLL application.
//

// ISSUE-2015/1/14-frankye review all error codes

#include "stdafx.h"

#include <strsafe.h>
#include <mfapi.h>

#include <initguid.h>
#include "KeywordDetectorContosoAdapter.h"
#include "ContosoKeywordDetector.h"
#include <wrl.h>

using namespace Microsoft::WRL;

class KeywordDetectorContosoAdapter : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IKeywordDetectorOemAdapter>
{
public:
    KeywordDetectorContosoAdapter()
    {
    }

    STDMETHODIMP KeywordDetectorContosoAdapter::GetCapabilities(
        _Out_ BOOL* SupportsUserModels,
        _Outptr_ KEYWORDID** KeywordIds,
        _Out_ ULONG* NumKeywords,
        _Outptr_ LANGID** LangIds,
        _Out_ ULONG* NumLanguages,
        _Outptr_ IMFMediaType* *ppMediaType)
    {
        HRESULT hr;
        ComPtr<IMFMediaType> spMFType;
        const WAVEFORMATEX waveFormat = { WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, 0 };

        *SupportsUserModels = FALSE;

        *KeywordIds = (KEYWORDID *)CoTaskMemAlloc(sizeof(KEYWORDID));
        if (*KeywordIds == nullptr)
        {
            return E_OUTOFMEMORY;
        }
        **KeywordIds = KwVoiceAssistant;
        *NumKeywords = 1;
        *LangIds = (LANGID *)CoTaskMemAlloc(sizeof(LANGID));
        if (*LangIds == nullptr)
        {
            CoTaskMemFree(*KeywordIds);
            return E_OUTOFMEMORY;
        }
        **LangIds = 0x409;
        *NumLanguages = 1;

        hr = MFCreateMediaType(spMFType.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            hr = MFInitMediaTypeFromWaveFormatEx(spMFType.Get(), &waveFormat, sizeof(waveFormat));
            if (SUCCEEDED(hr))
            {
                spMFType.CopyTo(ppMediaType);
                hr = S_OK;
            }
        }

        if (*ppMediaType == nullptr)
        {
            return E_FAIL;
        }

        return hr;
    }

    STDMETHODIMP VerifyUserKeyword(
        _In_ IStream* ModelData,
        _In_ KEYWORDID KeywordId,
        _In_ LANGID LangId,
        _In_ LONG KeywordEndBytePos,
        _In_ IMFMediaBuffer* UserRecording)
    {
        UNREFERENCED_PARAMETER(ModelData);
        UNREFERENCED_PARAMETER(KeywordId);
        UNREFERENCED_PARAMETER(LangId);
        UNREFERENCED_PARAMETER(KeywordEndBytePos);
        UNREFERENCED_PARAMETER(UserRecording);

        return E_NOTIMPL;
    }

    STDMETHODIMP ComputeAndAddUserModelData(
        _Inout_ IStream* ModelData,
        _In_ KEYWORDSELECTOR KeywordSelector,
        _In_ LONG *KeywordEndBytePos,
        _In_ IMFMediaBuffer **UserRecordings,
        _In_ ULONG NumUserRecordings)
    {
        UNREFERENCED_PARAMETER(ModelData);
        UNREFERENCED_PARAMETER(KeywordSelector);
        UNREFERENCED_PARAMETER(KeywordEndBytePos);
        UNREFERENCED_PARAMETER(UserRecordings);
        UNREFERENCED_PARAMETER(NumUserRecordings);

        return E_NOTIMPL;
    }

    STDMETHODIMP BuildArmingPatternData(
        _In_ IStream *ModelData,
        _In_ KEYWORDSELECTOR *KeywordSelectors,
        _In_ ULONG NumKeywordSelectors,
        _Outptr_ SOUNDDETECTOR_PATTERNHEADER **ppPatternData)
    {
        CONTOSO_KEYWORDCONFIGURATION *pPatternData = nullptr;

        UNREFERENCED_PARAMETER(ModelData);

        if (NumKeywordSelectors > 1)
        {
            return E_INVALIDARG;
        }
        if ((KeywordSelectors[0].KeywordId != KwVoiceAssistant) ||
            (KeywordSelectors[0].LangId != 0x0409))
        {
            return E_INVALIDARG;
        }

        pPatternData = (CONTOSO_KEYWORDCONFIGURATION*)CoTaskMemAlloc(sizeof(CONTOSO_KEYWORDCONFIGURATION));
        if (pPatternData == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        pPatternData->Header.Size = sizeof(*pPatternData);
        pPatternData->Header.PatternType = CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER;
        pPatternData->ContosoDetectorConfigurationData = 0x12345678;

        *ppPatternData = &pPatternData->Header;
        pPatternData = nullptr;

        return S_OK;
    }

    STDMETHODIMP ParseDetectionResultData(
        _In_ IStream *ModelData, 
        _In_ SOUNDDETECTOR_PATTERNHEADER *Result,    // ISSUE-2015/1/14-frankye should be const
        _Out_ KEYWORDID* KeywordId,
        _Out_ LANGID* LangId,
        _Out_ BOOL *pIsUserMatch,                  
        _Out_ ULONG64 *KeywordStartPerformanceCounterValue,
        _Out_ ULONG64 *KeywordEndPerformanceCounterValue)
    {
        // const CONTOSO_KEYWORDDETECTIONRESULT *contosoResult;

        UNREFERENCED_PARAMETER(ModelData);

        if (Result->PatternType != CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER || Result->Size < sizeof(CONTOSO_KEYWORDDETECTIONRESULT))
        {
            return E_INVALIDARG;
        }

        // contosoResult = (CONTOSO_KEYWORDDETECTIONRESULT*)Result;

        // Do something with the result data to determine return parameters
        // PostProcessResult(contosoResult->ContosoDetectorResultData, contosoResult->KeywordStartTimestamp, contosoResult->KeywordStopTimestamp);

        *KeywordId = KwVoiceAssistant;
        *LangId = 0x0409;
        *pIsUserMatch = FALSE;

        *KeywordStartPerformanceCounterValue = 0;
        *KeywordEndPerformanceCounterValue = 0;

        return S_OK;
    }
};

CoCreatableClass(KeywordDetectorContosoAdapter);

class EventDetectorContosoAdapter : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IEventDetectorOemAdapter>
{
public:
    EventDetectorContosoAdapter()
    {
    }

    STDMETHODIMP GetCapabilities(
        _Out_ EVENTFEATURES* GlobalFeatureSupport,
        _Outptr_ LANGID** LangIds,
        _Out_ ULONG* NumLanguages,
        _Out_ ULONG* NumUserRecordings,
        _Outptr_ WAVEFORMATEX** ppFormat)
    {
        const WAVEFORMATEX waveFormat = { WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, 0 };

        *ppFormat = (WAVEFORMATEX *)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        if (*ppFormat == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        memcpy(*ppFormat, &waveFormat, sizeof(WAVEFORMATEX));

        *GlobalFeatureSupport = EVENTFEATURES_NoEventFeatures;

        *LangIds = (LANGID *)CoTaskMemAlloc(sizeof(LANGID));

        if (*LangIds == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        **LangIds = 0x0409;
        *NumLanguages = 1;
        *NumUserRecordings = 0;

        return S_OK;
    }

    STDMETHODIMP GetCapabilitiesForLanguage(
        _In_ LANGID LangId,
        _Outptr_ DETECTIONEVENT** EventIds,
        _Out_ ULONG* NumEvents)
    {
        if (LangId == 0x0409)
        {
                DETECTIONEVENT events[] = { { CONTOSO_KEYWORD1, EVENTFEATURES_NoEventFeatures, {0}, L"Contoso 1", TRUE },
                                            { CONTOSO_KEYWORD2, EVENTFEATURES_NoEventFeatures, {0}, L"Contoso 2", TRUE } };

                *EventIds = (DETECTIONEVENT *)CoTaskMemAlloc(sizeof(events));
                if (*EventIds == nullptr)
                {
                    return E_OUTOFMEMORY;
                }

                memcpy(*EventIds, &events, sizeof(events));
                *NumEvents = 2;
        }
        else
        {
            return E_INVALIDARG;
        }

        return S_OK;
    }

    STDMETHODIMP VerifyUserEventData(
        _In_ IStream* ModelData,
        _In_ WAVEFORMATEX* UserRecording,
        _In_ DETECTIONEVENTSELECTOR EventSelector,
        _In_ LONG EventEndBytePos)
    {
        UNREFERENCED_PARAMETER(ModelData);
        UNREFERENCED_PARAMETER(UserRecording);
        UNREFERENCED_PARAMETER(EventSelector);
        UNREFERENCED_PARAMETER(EventEndBytePos);

        return E_NOTIMPL;
    }

    STDMETHODIMP ComputeAndAddUserModelData(
        _Inout_ IStream* ModelData,
        _In_ DETECTIONEVENTSELECTOR EventSelector,
        _In_ LONG* EventEndBytePos,
        _In_ WAVEFORMATEX** UserRecordings,
        _In_ ULONG NumUserRecordings)
    {
        UNREFERENCED_PARAMETER(ModelData);
        UNREFERENCED_PARAMETER(EventSelector);
        UNREFERENCED_PARAMETER(EventEndBytePos);
        UNREFERENCED_PARAMETER(UserRecordings);
        UNREFERENCED_PARAMETER(NumUserRecordings);

        return E_NOTIMPL;
    }

    STDMETHODIMP BuildArmingPatternData(
        _In_ IStream* UserModelData,
        _In_ DETECTIONEVENTSELECTOR* EventSelectors,
        _In_ ULONG NumEventSelectors,
        _Outptr_ SOUNDDETECTOR_PATTERNHEADER** ppPatternData)
    {
        CONTOSO_KEYWORDCONFIGURATION *pPatternData = nullptr;

        UNREFERENCED_PARAMETER(UserModelData);

        if (NumEventSelectors > 2)
        {
            return E_INVALIDARG;
        }

        if ((EventSelectors[0].Event.EventId != CONTOSO_KEYWORD1 && EventSelectors[0].Event.EventId != CONTOSO_KEYWORD2) || 
            (EventSelectors[0].UserId != 0) || (EventSelectors[0].LangId != 0x0409))
        {
            return E_INVALIDARG;
        }

        pPatternData = (CONTOSO_KEYWORDCONFIGURATION*)CoTaskMemAlloc(sizeof(CONTOSO_KEYWORDCONFIGURATION));
        if (pPatternData == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        pPatternData->Header.Size = sizeof(*pPatternData);
        pPatternData->Header.PatternType = CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2;
        pPatternData->ContosoDetectorConfigurationData = 0x12345678;

        *ppPatternData = &pPatternData->Header;
        pPatternData = nullptr;

        return S_OK;
    }

    STDMETHODIMP ParseDetectionResultData(
        _In_ IStream* UserModelData,
        _In_ SOUNDDETECTOR_PATTERNHEADER* Result,
        _Outptr_ SOUNDDETECTOR_PATTERNHEADER** AssistantContext,
        _Out_ DETECTIONEVENTSELECTOR* EventSelector,
        _Out_ EVENTACTION* EventAction,
        _Out_ ULONG64* EventStartPerformanceCounterValue,
        _Out_ ULONG64* EventEndPerformanceCounterValue,
        _Outptr_ WCHAR** DebugOutput)
    {
        const CONTOSO_KEYWORDDETECTIONRESULT *contosoResult;

        UNREFERENCED_PARAMETER(UserModelData);
        UNREFERENCED_PARAMETER(DebugOutput);
        UNREFERENCED_PARAMETER(AssistantContext);

        if (Result->PatternType != CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER2 || Result->Size < sizeof(CONTOSO_KEYWORDDETECTIONRESULT))
        {
            return E_INVALIDARG;
        }

        contosoResult = (CONTOSO_KEYWORDDETECTIONRESULT*)Result;

        if (CONTOSO_KEYWORD1 == contosoResult->EventId)
        {
            wcscpy_s(EventSelector->Event.DisplayName, L"Contoso 1");
        }
        else if (CONTOSO_KEYWORD2 == contosoResult->EventId)
        {
            wcscpy_s(EventSelector->Event.DisplayName, L"Contoso 2");
        }
        else
        {
            return E_INVALIDARG;
        }

        // Fill in event action information for the actual detection, based on what has been armed.
        EventSelector->Event.EventId = contosoResult->EventId;
        EventSelector->Armed = TRUE;
        EventSelector->UserId = 0;
        EventSelector->LangId = 0x0409;

        EventAction->EventdActionType = EVENTACTIONTYPE_Accept;
        EventAction->EventActionContextType = EVENTACTIONCONTEXTTYPE_None;

        // Retrieve the event start/stop times for the payload
        *EventStartPerformanceCounterValue = contosoResult->KeywordStartTimestamp;
        *EventEndPerformanceCounterValue = contosoResult->KeywordStopTimestamp;

        return S_OK;
    }

    STDMETHODIMP_(void) ReportOSDetectionResult(
        _In_ DETECTIONEVENTSELECTOR EventSelector,
        _In_ EVENTACTION EventAction)
    {
        UNREFERENCED_PARAMETER(EventSelector);
        UNREFERENCED_PARAMETER(EventAction);
    }
};

CoCreatableClass(EventDetectorContosoAdapter);

