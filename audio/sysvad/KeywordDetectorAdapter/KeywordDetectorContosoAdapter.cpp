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
        BOOL* SupportsUserModels,
        KEYWORDID** KeywordIds,
        ULONG* NumKeywords,
        LANGID** LangIds,
        ULONG* NumLanguages,
        IMFMediaType* *ppMediaType)
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

        return hr;
    }

    STDMETHODIMP VerifyUserKeyword(
        IStream* ModelData,
        KEYWORDID KeywordId,
        LANGID LangId,
        LONG KeywordEndBytePos,
        IMFMediaBuffer* UserRecording)
    {
        UNREFERENCED_PARAMETER(ModelData);
        UNREFERENCED_PARAMETER(KeywordId);
        UNREFERENCED_PARAMETER(LangId);
        UNREFERENCED_PARAMETER(KeywordEndBytePos);
        UNREFERENCED_PARAMETER(UserRecording);

        return E_NOTIMPL;
    }

    STDMETHODIMP ComputeAndAddUserModelData(
        IStream* ModelData,
        KEYWORDSELECTOR KeywordSelector,
        LONG *KeywordEndBytePos,
        IMFMediaBuffer **UserRecordings,
        ULONG NumUserRecordings)
    {
        UNREFERENCED_PARAMETER(ModelData);
        UNREFERENCED_PARAMETER(KeywordSelector);
        UNREFERENCED_PARAMETER(KeywordEndBytePos);
        UNREFERENCED_PARAMETER(UserRecordings);
        UNREFERENCED_PARAMETER(NumUserRecordings);

        return E_NOTIMPL;
    }

    STDMETHODIMP BuildArmingPatternData(
        IStream *ModelData,
        KEYWORDSELECTOR *KeywordSelectors,
        ULONG NumKeywordSelectors,
        SOUNDDETECTOR_PATTERNHEADER **ppPatternData)
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
        IStream *ModelData, 
        SOUNDDETECTOR_PATTERNHEADER *Result,    // ISSUE-2015/1/14-frankye should be const
        KEYWORDID* KeywordId,
        LANGID* LangId,
        BOOL *pIsUserMatch,                  
        ULONG64 *KeywordStartPerformanceCounterValue,
        ULONG64 *KeywordEndPerformanceCounterValue)
    {
        const CONTOSO_KEYWORDDETECTIONRESULT *contosoResult;

        UNREFERENCED_PARAMETER(ModelData);

        if (Result->PatternType != CONTOSO_KEYWORDCONFIGURATION_IDENTIFIER || Result->Size < sizeof(CONTOSO_KEYWORDDETECTIONRESULT))
        {
            return E_INVALIDARG;
        }

        contosoResult = (CONTOSO_KEYWORDDETECTIONRESULT*)Result;

        // Do something with the result data to determine return parameters
        // PostProcessResult(contosoResult->ContosoDetectorResultData);

        *KeywordId = KwVoiceAssistant;
        *LangId = 0x0409;
        *pIsUserMatch = FALSE;

        *KeywordStartPerformanceCounterValue = 0;
        *KeywordEndPerformanceCounterValue = 0;

        return S_OK;
    }
};

CoCreatableClass(KeywordDetectorContosoAdapter);
