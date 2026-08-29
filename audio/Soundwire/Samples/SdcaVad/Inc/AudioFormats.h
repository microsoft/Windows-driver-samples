/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    AudioFormats.h

Abstract:

    Contains Audio formats supported for the SDCAVad Device

Environment:

    Kernel mode

--*/

#pragma once

//
// Basic-testing formats.
//
static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm44100c2 =
{ 
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            44100,
            176400,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
        },
        16,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm44100c2nomask =
{ 
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            44100,
            176400,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)
        },
        16,
        0,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm48000c2 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            48000,
            192000,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        16,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm44100c2_24in32 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            44100,
            352800,
            8,
            32,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        24,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm44100c2_24in32_nomask =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            44100,
            352800,
            8,
            32,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        24,
        0,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm48000c2_24in32 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            48000,
            384000,
            8,
            32,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        24,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static 
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm48000c2_24in32_nomask =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            48000,
            384000,
            8,
            32,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        24,
        0,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

// No Mask version is used for 2ch Capture, where the mask is not meaningful
static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm48000c2nomask =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            48000,
            192000,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        16,
        0,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};


static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm96000c2_24in32 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            96000,
            768000,
            8,
            32,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        24,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm96000c2 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            96000,
            384000,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        16,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm192000c2_24in32 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            192000,
            1536000,
            8,
            32,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        24,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm192000c2 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            192000,
            768000,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
        16,
        KSAUDIO_SPEAKER_STEREO,
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};



static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm44100c1 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            1,
            44100,
            88200,
            2,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
    16,
    KSAUDIO_SPEAKER_MONO,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm48000c1 =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            1,
            48000,
            96000,
            2,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
    16,
    KSAUDIO_SPEAKER_MONO,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm48000c4nomask =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            4,
            48000,
            384000,
            8,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
    16,
    0,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm16000c2nomask =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            2,
            16000,
            64000,
            4,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
    16,
    0,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

static
KSDATAFORMAT_WAVEFORMATEXTENSIBLE Pcm16000c4nomask =
{
    {
        sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    {
        {
            WAVE_FORMAT_EXTENSIBLE,
            4,
            16000,
            128000,
            8,
            16,
            sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)
        },
    16,
    0,
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)
    }
};

PAGED_CODE_SEG
inline
NTSTATUS
SdcaVad_RetrieveOrCreateDataFormatList(
    _In_    ACXPIN              Pin,
    _In_    PGUID               Mode,
    _Out_   ACXDATAFORMATLIST * FormatList
)
{
    PAGED_CODE();

    // Note: AcxPinGetRawDataFormatList will do the same thing as AcxPinRetrieveModeDataFormatList(RAW)
    NTSTATUS status = AcxPinRetrieveModeDataFormatList(Pin, Mode, FormatList);
    if (!NT_SUCCESS(status))
    {
        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = Pin;

        ACX_DATAFORMAT_LIST_CONFIG config;
        ACX_DATAFORMAT_LIST_CONFIG_INIT(&config);

        status = AcxDataFormatListCreate(AcxCircuitGetWdfDevice(AcxPinGetCircuit(Pin)), &attributes, &config, FormatList);

        if (NT_SUCCESS(status))
        {
            status = AcxPinAssignModeDataFormatList(Pin, Mode, *FormatList);
        }
    }
    return status;
}

PAGED_CODE_SEG
inline
NTSTATUS
SdcaVad_ClearDataFormatList(
    _In_ ACXDATAFORMATLIST FormatList
)
{
    PAGED_CODE();

    ACX_DATAFORMAT_LIST_ITERATOR    formatIter;
    ACXDATAFORMAT                   format;
    ACXDATAFORMAT                   formatToDelete = nullptr;
    NTSTATUS                        status = STATUS_SUCCESS;

    // The AcxDataFormatListRemoveDataFormats API is not available in ACX 1.0
    ACX_DATAFORMAT_LIST_ITERATOR_INIT(&formatIter);
    AcxDataFormatListBeginIteration(FormatList, &formatIter);
    status = AcxDataFormatListRetrieveNextFormat(FormatList, &formatIter, &format);
    while (NT_SUCCESS(status) && format != nullptr)
    {
        // We can delete this format after we've retrieved the next format
        formatToDelete = format;
        status = AcxDataFormatListRetrieveNextFormat(FormatList, &formatIter, &format);
        if (!NT_SUCCESS(status))
        {
            format = nullptr;
        }

        status = AcxDataFormatListRemoveDataFormat(FormatList, formatToDelete);
        if (!NT_SUCCESS(status))
        {
            break;
        }

    }
    AcxDataFormatListEndIteration(FormatList, &formatIter);

    if (status == STATUS_NO_MORE_ENTRIES)
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

PAGED_CODE_SEG
inline
NTSTATUS
SdcaVad_CopyFormats(
    _In_    ACXDATAFORMATLIST SourceList,
    _In_    ACXDATAFORMATLIST DestinationList,
    _Out_   PULONG FormatCount
)
{
    PAGED_CODE();

    ACX_DATAFORMAT_LIST_ITERATOR    formatIter;
    ACXDATAFORMAT                   format;
    NTSTATUS                        status = STATUS_SUCCESS;

    *FormatCount = 0;

    // Now copy over all formats from the target pin
    ACX_DATAFORMAT_LIST_ITERATOR_INIT(&formatIter);
    AcxDataFormatListBeginIteration(SourceList, &formatIter);
    while (NT_SUCCESS(status) && NT_SUCCESS(AcxDataFormatListRetrieveNextFormat(SourceList, &formatIter, &format)))
    {
        ++*FormatCount;

        // The DataFormatList adds a reference to the format object
        status = AcxDataFormatListAddDataFormat(DestinationList, format);
    }
    AcxDataFormatListEndIteration(SourceList, &formatIter);

    // Then finally assign the default format
    ACXDATAFORMAT defaultFormat;
    if (NT_SUCCESS(status) && NT_SUCCESS(AcxDataFormatListRetrieveDefaultDataFormat(SourceList, &defaultFormat)))
    {
        status = AcxDataFormatListAssignDefaultDataFormat(DestinationList, defaultFormat);
    }

    return status;
}
