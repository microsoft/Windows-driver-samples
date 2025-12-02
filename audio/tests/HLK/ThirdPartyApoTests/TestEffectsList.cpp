//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#include <stdafx.h>
#include <memory>
#include <wil\resultmacros.h>
#include <wil\com.h>
#include <wil\winrt.h>
#include <wil\wrl.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>
#include <windows.media.effects.h>
#include "avdevqueryhelpers.h"
#include <AudioStreamCategoryConversions.h>

using namespace Windows::Media::Effects;

class CTestEffectsList
{
    BEGIN_TEST_CLASS(CTestEffectsList)
        START_APPVERIFIFER
        TEST_CLASS_PROPERTY(L"Owner", L"auddev")
        TEST_CLASS_PROPERTY(L"TestClassification", L"Feature")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"audiodg.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"audioengineextensionapo.h")
        TEST_CLASS_PROPERTY(L"RunAs", L"Elevated")
        TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        TEST_CLASS_PROPERTY(L"Ignore", L"false")
        TEST_CLASS_PROPERTY(L"EtwLogger:WPRProfileFile", L"Audio-Tests.wprp")
        TEST_CLASS_PROPERTY(L"EtwLogger:WPRProfile", L"MultimediaCategory.Verbose.File")
        END_APPVERIFIER
    END_TEST_CLASS()

protected:
    BEGIN_TEST_METHOD(TestGetEffectsList)
        COMMON_ONECORE_TEST_PROPERTIES
        TEST_METHOD_PROPERTY(L"Kits.TestId", L"7E8037CA-8670-4170-8EC8-C7B02B22A7B4")
        TEST_METHOD_PROPERTY(L"Kits.TestId2", L"FD1ED3C6-54E0-432B-B3C9-5EAC5DE2D3E1")
        TEST_METHOD_PROPERTY(L"Kits.TestName", L"Audio APO - Test IAudioSystemEffects2 and IAudioSystemEffects3 usage from APOs - TestEffectsList")
        TEST_METHOD_PROPERTY(L"Kits.ExpectedRuntime", L"30")
        TEST_METHOD_PROPERTY(L"Kits.TimeoutInMinutes", L"60")
        TEST_METHOD_PROPERTY(L"Kits.Description", L"Third Party APO Test: TestEffectsList")
        APO_TEST_PROPERTIES
    END_TEST_METHOD()
};

GUID AudioEffectTypeToGUID(Windows::Media::Effects::AudioEffectType type)
{
    switch(type)
    {
        case AudioEffectType_AcousticEchoCancellation:
            return AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION;
        case AudioEffectType_NoiseSuppression:
            return AUDIO_EFFECT_TYPE_NOISE_SUPPRESSION;
        case AudioEffectType_AutomaticGainControl:
            return AUDIO_EFFECT_TYPE_AUTOMATIC_GAIN_CONTROL;
        case AudioEffectType_BeamForming:
            return AUDIO_EFFECT_TYPE_BEAMFORMING;
        case AudioEffectType_ConstantToneRemoval:
            return AUDIO_EFFECT_TYPE_CONSTANT_TONE_REMOVAL;
        case AudioEffectType_Equalizer:
            return AUDIO_EFFECT_TYPE_EQUALIZER;
        case AudioEffectType_LoudnessEqualizer:
            return AUDIO_EFFECT_TYPE_LOUDNESS_EQUALIZER;
        case AudioEffectType_BassBoost:
            return AUDIO_EFFECT_TYPE_BASS_BOOST;
        case AudioEffectType_VirtualSurround:
            return AUDIO_EFFECT_TYPE_VIRTUAL_SURROUND;
        case AudioEffectType_VirtualHeadphones:
            return AUDIO_EFFECT_TYPE_VIRTUAL_HEADPHONES;
        case AudioEffectType_SpeakerFill:
            return AUDIO_EFFECT_TYPE_SPEAKER_FILL;
        case AudioEffectType_RoomCorrection:
            return AUDIO_EFFECT_TYPE_ROOM_CORRECTION;
        case AudioEffectType_BassManagement:
            return AUDIO_EFFECT_TYPE_BASS_MANAGEMENT;
        case AudioEffectType_EnvironmentalEffects:
            return AUDIO_EFFECT_TYPE_ENVIRONMENTAL_EFFECTS;
        case AudioEffectType_SpeakerProtection:
            return AUDIO_EFFECT_TYPE_SPEAKER_PROTECTION;
        case AudioEffectType_SpeakerCompensation:
            return AUDIO_EFFECT_TYPE_SPEAKER_COMPENSATION;
        case AudioEffectType_DynamicRangeCompression:
            return AUDIO_EFFECT_TYPE_DYNAMIC_RANGE_COMPRESSION;
        case AudioEffectType_FarFieldBeamForming:
            return AUDIO_EFFECT_TYPE_FAR_FIELD_BEAMFORMING;
        case AudioEffectType_DeepNoiseSuppression:
            return AUDIO_EFFECT_TYPE_DEEP_NOISE_SUPPRESSION;
    }
    return GUID_NULL;
}

void CTestEffectsList::TestGetEffectsList()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    wil::com_ptr_nothrow<IMMDeviceEnumerator> spEnumerator;
    VERIFY_SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spEnumerator)));

    EDataFlow dataFlows[] = { eRender, eCapture };

    for (EDataFlow dataFlow : dataFlows)
    {
        wil::com_ptr_nothrow<IMMDeviceCollection> spDevices;
        VERIFY_SUCCEEDED(spEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &spDevices));

        UINT cDevices = 0;
        VERIFY_SUCCEEDED(spDevices->GetCount(&cDevices));

        for (UINT i = 0; i < cDevices; i++)
        {
            wil::com_ptr_nothrow<IMMDevice> spEndpoint;
            VERIFY_SUCCEEDED(spDevices->Item(i, &spEndpoint));

            wil::unique_prop_variant var;
            wil::com_ptr_nothrow<IPropertyStore> spPropertyStore;
            VERIFY_SUCCEEDED(spEndpoint->OpenPropertyStore(STGM_READ, &spPropertyStore));
            VERIFY_SUCCEEDED(spPropertyStore->GetValue(PKEY_Device_FriendlyName, &var));
            VERIFY_ARE_EQUAL(VT_LPWSTR, var.vt);
            LOG_OUTPUT(L"Testing endpoint: %s", var.pwszVal);

            wil::unique_cotaskmem_string spDeviceId;
            VERIFY_SUCCEEDED(spEndpoint->GetId(&spDeviceId));
            wil::unique_cotaskmem_string spEndpointPnpInstanceId;
            VERIFY_SUCCEEDED(mmdDevGetInterfaceIdFromMMDeviceId(spDeviceId.get(), &spEndpointPnpInstanceId));
            HSTRING deviceId;
            VERIFY_SUCCEEDED(WindowsCreateString(spEndpointPnpInstanceId.get(), (UINT32)wcslen(spEndpointPnpInstanceId.get()), &deviceId));

            // Get effects list using IAudioSystemEffects2
            wil::com_ptr_nothrow<IAudioEffectsManagerStatics> audioEffectsManagerStatics =
                wil::GetActivationFactory<IAudioEffectsManagerStatics>(RuntimeClass_Windows_Media_Effects_AudioEffectsManager);

            wil::com_ptr_nothrow<Windows::Foundation::Collections::IVectorView<AudioEffect*>> effects;
            AUDIO_STREAM_CATEGORY audioCategory;

            if (dataFlow == eRender)
            {
                wil::com_ptr_nothrow<IAudioRenderEffectsManager> audioRenderEffectsManager;
                VERIFY_SUCCEEDED(audioEffectsManagerStatics->CreateAudioRenderEffectsManager(deviceId, Windows::Media::Render::AudioRenderCategory::AudioRenderCategory_Communications, &audioRenderEffectsManager));
                VERIFY_IS_NOT_NULL(audioRenderEffectsManager);
                VERIFY_SUCCEEDED(audioRenderEffectsManager->GetAudioRenderEffects(&effects));

                VERIFY_SUCCEEDED(MediaRenderCategory_To_AUDIO_STREAM_CATEGORY(Windows::Media::Render::AudioRenderCategory::AudioRenderCategory_Communications, &audioCategory));
            }
            else
            {
                wil::com_ptr_nothrow<IAudioCaptureEffectsManager> audioCaptureEffectsManager;
                VERIFY_SUCCEEDED(audioEffectsManagerStatics->CreateAudioCaptureEffectsManager(deviceId, Windows::Media::Capture::MediaCategory::MediaCategory_Communications, &audioCaptureEffectsManager));
                VERIFY_IS_NOT_NULL(audioCaptureEffectsManager);
                VERIFY_SUCCEEDED(audioCaptureEffectsManager->GetAudioCaptureEffects(&effects));

                VERIFY_SUCCEEDED(MediaCaptureCategory_To_AUDIO_STREAM_CATEGORY(Windows::Media::Capture::MediaCategory::MediaCategory_Communications, &audioCategory));
            }

            UINT32 numEffects = 0;
            effects->get_Size(&numEffects);

            // Get effects lists using IAudioSystemEffects3
            wil::com_ptr_nothrow<IAudioClient2> spAudioClient;
            VERIFY_SUCCEEDED(spEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(&spAudioClient)));

            AudioClientProperties clientProperties = {};
            clientProperties.cbSize = sizeof(AudioClientProperties);
            clientProperties.bIsOffload = FALSE;
            clientProperties.eCategory = audioCategory;
            clientProperties.Options = AUDCLNT_STREAMOPTIONS_NONE;
            VERIFY_SUCCEEDED(spAudioClient->SetClientProperties(&clientProperties));

            wil::unique_cotaskmem_ptr<WAVEFORMATEX> mixFormat;
            VERIFY_SUCCEEDED(spAudioClient->GetMixFormat(wil::out_param_ptr<WAVEFORMATEX**>(mixFormat)));

            VERIFY_SUCCEEDED(spAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 20 * 10000, 0, mixFormat.get(), nullptr));

            wil::com_ptr_nothrow<IAudioEffectsManager> audioEffectsManager;
            VERIFY_SUCCEEDED(spAudioClient->GetService(IID_PPV_ARGS(&audioEffectsManager)));
            VERIFY_IS_NOT_NULL(audioEffectsManager);

            wil::unique_cotaskmem_array_ptr<AUDIO_EFFECT> controllableEffects;
            UINT32 numControllableEffects;
            VERIFY_SUCCEEDED(audioEffectsManager->GetAudioEffects(&controllableEffects, &numControllableEffects));

            // Verify 'effects' is a subset of 'controllableEffects'
            VERIFY_IS_GREATER_THAN_OR_EQUAL(numControllableEffects, numEffects);
            for (UINT32 i = 0; i < numEffects; i++)
            {
                bool found = false;
                for (UINT32 j = 0; j < numControllableEffects && !found; j++)
                {
                    AudioEffectType effectType;
                    wil::com_ptr_nothrow<IAudioEffect> spEffect;
                    effects->GetAt(i, &spEffect);
                    spEffect->get_AudioEffectType(&effectType);
                    if (IsEqualGUID(AudioEffectTypeToGUID(effectType), controllableEffects[j].id))
                    {
                        found = true;
                    }
                }
                VERIFY_IS_TRUE(found);
            }

            // Verify effects not present in 'effects' are disabled
            for (UINT32 i = 0; i < numControllableEffects; i++)
            {
                bool found = false;
                for (UINT32 j = 0; j < numEffects && !found; j++)
                {
                    AudioEffectType effectType;
                    wil::com_ptr_nothrow<IAudioEffect> spEffect;
                    effects->GetAt(j, &spEffect);
                    spEffect->get_AudioEffectType(&effectType);
                    if (IsEqualGUID(controllableEffects[i].id, AudioEffectTypeToGUID(effectType)))
                    {
                        found = true;
                    }
                }
                if (!found)
                {
                    VERIFY_ARE_EQUAL(controllableEffects[i].state, AUDIO_EFFECT_STATE_OFF);
                }
            }
        }
    }
}
