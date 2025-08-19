/*++

Copyright (c) 1997-2008 Microsoft Corporation

Module Name:

DISPAUD.C

Abstract:

This source file contains routines which update the edit control
to display information about USB Audio descriptors.

Environment:

user mode

Revision History:

03-07-1998 : created

--*/

/*****************************************************************************
 I N C L U D E S
*****************************************************************************/

#include "uvcview.h"

/*****************************************************************************
 G L O B A L S    P R I V A T E    T O    T H I S    F I L E
*****************************************************************************/

//
// USB Device Class Definition for Terminal Types  0.9 Draft Revision
//
STRINGLIST slAudioTerminalTypes [] =
{
    //
    // 2.1 USB Terminal Types
    //
    {0x0100, "USB Undefined",       ""},
    {0x0101, "USB streaming",       ""},
    {0x01FF, "USB vendor specific", ""},
    //
    // 2.2 Input Terminal Types
    //
    {0x0200, "Input Undefined",             ""},
    {0x0201, "Microphone",                  ""},
    {0x0202, "Desktop microphone",          ""},
    {0x0203, "Personal microphone",         ""},
    {0x0204, "Omni-directional microphone", ""},
    {0x0205, "Microphone array",            ""},
    {0x0206, "Processing microphone array", ""},
    //
    // 2.3 Output Terminal Types
    //
    {0x0300, "Output Undefined",              ""},
    {0x0301, "Speaker",                       ""},
    {0x0302, "Headphones",                    ""},
    {0x0303, "Head Mounted Display Audio",    ""},
    {0x0304, "Desktop speaker",               ""},
    {0x0305, "Room speaker",                  ""},
    {0x0306, "Communication speaker",         ""},
    {0x0307, "Low frequency effects speaker", ""},
    //
    // 2.4 Bi-directional Terminal Types
    //
    {0x0400, "Bi-directional Undefined",        ""},
    {0x0401, "Handset",                         ""},
    {0x0402, "Headset",                         ""},
    {0x0403, "Speakerphone, no echo reduction", ""},
    {0x0404, "Echo-suppressing speakerphone",   ""},
    {0x0405, "Echo-canceling speakerphone",     ""},
    //
    // 2.5 Telephony Terminal Types
    //
    {0x0500, "Telephony Undefined", ""},
    {0x0501, "Phone line",          ""},
    {0x0502, "Telephone",           ""},
    {0x0503, "Down Line Phone",     ""},
    //
    // 2.6 External Terminal Types
    //
    {0x0600, "External Undefined",                    ""},
    {0x0601, "Analog connector",                      ""},
    {0x0602, "Digital audio interface",               ""},
    {0x0603, "Line connector",                        ""},
    {0x0604, "Legacy audio connector",                ""},
    {0x0605, "S/PDIF interface",                      ""},
    {0x0606, "1394 DA stream",                        ""},
    {0x0607, "1394 DV stream soundtrack",             ""},
    {0x0608, "ADAT Lightpipe",                        ""},
    {0x0609, "Tascam Digital Interface",              ""},
    {0x060A, "Multi-channel Audio Digital Interface", ""},
    //
    // Embedded Function Terminal Types
    //
    {0x0700, "Embedded Undefined",             ""},
    {0x0701, "Level Calibration Noise Source", ""},
    {0x0702, "Equalization Noise",             ""},
    {0x0703, "CD player",                      ""},
    {0x0704, "DAT",                            ""},
    {0x0705, "DCC",                            ""},
    {0x0706, "MiniDisk",                       ""},
    {0x0707, "Analog Tape",                    ""},
    {0x0708, "Phonograph",                     ""},
    {0x0709, "VCR Audio",                      ""},
    {0x070A, "Video Disc Audio",               ""},
    {0x070B, "DVD Audio",                      ""},
    {0x070C, "TV Tuner Audio",                 ""},
    {0x070D, "Satellite Receiver Audio",       ""},
    {0x070E, "Cable Tuner Audio",              ""},
    {0x070F, "DSS Audio",                      ""},
    {0x0710, "Radio Receiver",                 ""},
    {0x0711, "Radio Transmitter",              ""},
    {0x0712, "Multi-track Recorder",           ""},
    {0x0713, "Synthesizer",                    ""},
    {0x0714, "Piano",                          ""},
    {0x0715, "Guitar",                         ""},
    {0x0716, "Drums/Rythm",                    ""},
    {0x0717, "Other Musical Instrument",       ""},
};
STRINGLIST slAudioFormatTypes [] =
{
    //
    // A.1.1 Audio Data Format Type I Codes
    //
    {0x0000, "TYPE_I_UNDEFINED", ""},
    {0x0001, "PCM",              ""},
    {0x0002, "PCM8",             ""},
    {0x0003, "IEEE_FLOAT",       ""},
    {0x0004, "ALAW",             ""},
    {0x0005, "MULAW",            ""},
    //
    // A.1.2 Audio Data Format Type II Codes
    //
    {0x1000, "TYPE_II_UNDEFINED", ""},
    {0x1001, "MPEG",              ""},
    {0x1002, "AC-3",              ""},
    //
    // A.1.3 Audio Data Format Type III Codes
    //
    {0x2000, "TYPE_III_UNDEFINED",                              ""},
    {0x2001, "IEC1937_AC-3",                                    ""},
    {0x2002, "IEC1937_MPEG-1_Layer1",                           ""},
    {0x2003, "IEC1937_MPEG-1_Layer2/3 or IEC1937_MPEG-2_NOEXT", ""},
    {0x2004, "IEC1937_MPEG-2_EXT",                              ""},
    {0x2005, "IEC1937_MPEG-2_Layer1_LS",                        ""},
    {0x2006, "IEC1937_MPEG-2_Layer2/3_LS",                      ""},
};

STRINGLIST slFormatTypeCode [] =
{
    {0x00, "FORMAT_TYPE_UNDEFINED", ""},
    {0x01, "FORMAT_TYPE_I",         ""},
    {0x02, "FORMAT_TYPE_II",        ""},
    {0x03, "FORMAT_TYPE_III",       ""},
};


STRINGLIST slProcessingUnitTypes [] =
{
    {0x00, "Processing undefined",      ""},
    {0x01, "Up/downmix",                ""},
    {0x02, "Dolby Prologic",            ""},
    {0x03, "3D Stereo Extender",        ""},
    {0x04, "Reverberation",             ""},
    {0x05, "Chorus",                    ""},
    {0x06, "Dynamic range compression", ""},
};

STRINGLIST slFeatureUnitTypes [] =
{
    {0x001, "Mute",              ""},
    {0x002, "Volume",            ""},
    {0x004, "Bass",              ""},
    {0x008, "Mid",               ""},
    {0x010, "Treble",            ""},
    {0x020, "Graphic equalizer", ""},
    {0x040, "Automatic gain",    ""},
    {0x080, "Delay",             ""},
    {0x100, "Bass boost",        ""},
    {0x200, "Loudness",          ""},
};

STRINGLIST slChannelConfig [] =
{
    {0x001, "Left Front (L)",                  ""},
    {0x002, "Right Ront (R)",                  ""},
    {0x004, "Center Front (C)",                ""},
    {0x008, "Low Frequency Enhancement (LFE)", ""},
    {0x010, "Left Surround (Ls)",              ""},
    {0x020, "Right Surround (Rs)",             ""},
    {0x040, "Left of Center (Lc)",             ""},
    {0x080, "Right of Center (Rc)",            ""},
    {0x100, "Surround (S)",                    ""},
    {0x200, "Side Left (Sl)",                  ""},
    {0x400, "Side Right (Sr)",                 ""},
    {0x800, "Top (T)",                         ""},
};

STRINGLIST slIsocAttributes [] =
{
    {0x01, "Sampling Frequency control", ""},
    {0x02, "Pitch control",              ""},
    {0x80, "MaxPacketsOnly",             ""},
};

STRINGLIST slLockDelayUnits [] =
{
    {0x00, "Undefined",           ""},
    {0x01, "Milliseconds",        ""},
    {0x02, "Decoded PCM samples", ""},
};

/*****************************************************************************
 L O C A L    F U N C T I O N    P R O T O T Y P E S
*****************************************************************************/

BOOL
DisplayACHeader (
    PUSB_AUDIO_AC_INTERFACE_HEADER_DESCRIPTOR HeaderDesc
);

BOOL
DisplayACInputTerminal (
    PUSB_AUDIO_INPUT_TERMINAL_DESCRIPTOR ITDesc
);

BOOL
DisplayACOutputTerminal (
    PUSB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR OTDesc
);

BOOL
DisplayACMixerUnit (
    PUSB_AUDIO_MIXER_UNIT_DESCRIPTOR MixerDesc
);

BOOL
DisplayACSelectorUnit (
    PUSB_AUDIO_SELECTOR_UNIT_DESCRIPTOR SelectorDesc
);

BOOL
DisplayACFeatureUnit (
    PUSB_AUDIO_FEATURE_UNIT_DESCRIPTOR FeatureDesc
);

BOOL
DisplayACProcessingUnit (
    PUSB_AUDIO_PROCESSING_UNIT_DESCRIPTOR ProcessingDesc
);

BOOL
DisplayACExtensionUnit (
    PUSB_AUDIO_EXTENSION_UNIT_DESCRIPTOR ExtensionDesc
);

BOOL
DisplayASGeneral (
    PUSB_AUDIO_GENERAL_DESCRIPTOR GeneralDesc
);

BOOL
DisplayAudioStreamingEndpoint (
    PUSB_AUDIO_AS_ENDPOINT_DESCRIPTOR EndpointDesc
);

BOOL
DisplayASFormatType (
    PUSB_AUDIO_COMMON_FORMAT_DESCRIPTOR FormatDesc
);

BOOL
DisplayASFormatSpecific (
    PUSB_AUDIO_COMMON_DESCRIPTOR CommonDesc
);

BOOL
DisplayMSInterfaceHeader (
    PUSB_AUDIO_MS_INTERFACE_HEADER_DESCRIPTOR HeaderDesc
);

BOOL
DisplayMSMIDIInJack (
    PUSB_AUDIO_MS_MIDI_IN_JACK_DESCRIPTOR JackDesc,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
);

BOOL
DisplayMSMIDIOutJack (
    PUSB_AUDIO_MS_MIDI_OUT_JACK_DESCRIPTOR JackDesc,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
);

BOOL
DisplayMSElement (
    PUSB_AUDIO_MS_ELEMENT_DESCRIPTOR ElementDesc,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
);

BOOL
DisplayMidiStreamingEndpoint(
    PUSB_AUDIO_MS_ENDPOINT_DESCRIPTOR EndpointDesc
);

VOID
DisplayBytes (
    PUCHAR Data,
    USHORT Len
);

VOID
DisplayChannelConfig (
    DWORD dwChannelConfig
)
{
    for (int i = 0; i < ARRAYSIZE(slChannelConfig); i++)
    {
        if (dwChannelConfig & slChannelConfig[i].ulFlag)
        {
            AppendTextBuffer("                                 (%s)\r\n", slChannelConfig[i].pszString);
        }
    }
}


/*****************************************************************************
 L O C A L    F U N C T I O N S
*****************************************************************************/

/*****************************************************************************

 DisplayAudioDescriptor()

 CommonDesc - An Audio Class Descriptor

 bInterfaceSubClass - The SubClass of the Interface containing the descriptor

 info - The device which supplied the descriptor

*****************************************************************************/

BOOL
DisplayAudioDescriptor (
    PUSB_AUDIO_COMMON_DESCRIPTOR CommonDesc,
    UCHAR                        bInterfaceSubClass,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
)
{
    switch (CommonDesc->bDescriptorType)
    {
        case USB_AUDIO_CS_INTERFACE:
            switch (bInterfaceSubClass)
            {
                case USB_AUDIO_SUBCLASS_AUDIOCONTROL:
                    switch (CommonDesc->bDescriptorSubtype)
                    {
                        case USB_AUDIO_AC_HEADER:
                            return DisplayACHeader((PUSB_AUDIO_AC_INTERFACE_HEADER_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_INPUT_TERMINAL:
                            return DisplayACInputTerminal((PUSB_AUDIO_INPUT_TERMINAL_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_OUTPUT_TERMINAL:
                            return DisplayACOutputTerminal((PUSB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_MIXER_UNIT:
                            return DisplayACMixerUnit((PUSB_AUDIO_MIXER_UNIT_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_SELECTOR_UNIT:
                            return DisplayACSelectorUnit((PUSB_AUDIO_SELECTOR_UNIT_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_FEATURE_UNIT:
                            return DisplayACFeatureUnit((PUSB_AUDIO_FEATURE_UNIT_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_PROCESSING_UNIT:
                            return DisplayACProcessingUnit((PUSB_AUDIO_PROCESSING_UNIT_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AC_EXTENSION_UNIT:
                            return DisplayACExtensionUnit((PUSB_AUDIO_EXTENSION_UNIT_DESCRIPTOR)CommonDesc);

                        default:
                            break;
                    }
                    break;

                case USB_AUDIO_SUBCLASS_AUDIOSTREAMING:
                    switch (CommonDesc->bDescriptorSubtype)
                    {
                        case USB_AUDIO_AS_GENERAL:
                            return DisplayASGeneral((PUSB_AUDIO_GENERAL_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_AS_FORMAT_TYPE:
                            return DisplayASFormatType((PUSB_AUDIO_COMMON_FORMAT_DESCRIPTOR)CommonDesc);
                            break;

                        case USB_AUDIO_AS_FORMAT_SPECIFIC:
                            return DisplayASFormatSpecific(CommonDesc);

                        default:
                            break;
                    }
                    break;

                case USB_AUDIO_SUBCLASS_MIDISTREAMING:
                    switch (CommonDesc->bDescriptorSubtype)
                    {
                        case USB_AUDIO_CS_INTERFACE_MS_HEADER:
                            return DisplayMSInterfaceHeader((PUSB_AUDIO_MS_INTERFACE_HEADER_DESCRIPTOR)CommonDesc);

                        case USB_AUDIO_CS_INTERFACE_MIDI_IN_JACK:
                            return DisplayMSMIDIInJack((PUSB_AUDIO_MS_MIDI_IN_JACK_DESCRIPTOR)CommonDesc, StringDescs, LatestDevicePowerState);

                        case USB_AUDIO_CS_INTERFACE_MIDI_OUT_JACK:
                            return DisplayMSMIDIOutJack((PUSB_AUDIO_MS_MIDI_OUT_JACK_DESCRIPTOR)CommonDesc, StringDescs, LatestDevicePowerState);

                        case USB_AUDIO_CS_INTERFACE_ELEMENT:
                            return DisplayMSElement((PUSB_AUDIO_MS_ELEMENT_DESCRIPTOR)CommonDesc, StringDescs, LatestDevicePowerState);
                    default:
                        break;
                    }
                    break;

                default:
                    break;
            }
            break;

        case USB_AUDIO_CS_ENDPOINT:
            switch (bInterfaceSubClass)
            {
            case USB_AUDIO_SUBCLASS_AUDIOSTREAMING:
                return DisplayAudioStreamingEndpoint((PUSB_AUDIO_AS_ENDPOINT_DESCRIPTOR)CommonDesc);

            case USB_AUDIO_SUBCLASS_MIDISTREAMING:
                return DisplayMidiStreamingEndpoint((PUSB_AUDIO_MS_ENDPOINT_DESCRIPTOR)CommonDesc);
            }

        default:
            break;
    }

    return FALSE;
}


/*****************************************************************************

 DisplayACHeader()

*****************************************************************************/

BOOL
DisplayACHeader (
    PUSB_AUDIO_AC_INTERFACE_HEADER_DESCRIPTOR HeaderDesc
)
{
    UINT i = 0;

    if (HeaderDesc->bLength < sizeof(USB_AUDIO_AC_INTERFACE_HEADER_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Interface Header Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     HeaderDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     HeaderDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (HEADER)\r\n",
                     HeaderDesc->bDescriptorSubtype);

    AppendTextBuffer("bcdADC:                          0x%04X\r\n",
                     HeaderDesc->bcdADC);

    AppendTextBuffer("wTotalLength:                    0x%04X\r\n",
                     HeaderDesc->wTotalLength);

    AppendTextBuffer("bInCollection:                     0x%02X\r\n",
                     HeaderDesc->bInCollection);

    for (i=0; i<HeaderDesc->bInCollection; i++)
    {
        AppendTextBuffer("baInterfaceNr[%d]:                  0x%02X\r\n",
                         i+1,
                         HeaderDesc->baInterfaceNr[i]);
    }

    return TRUE;
}


/*****************************************************************************

 DisplayACInputTerminal()

*****************************************************************************/

BOOL
DisplayACInputTerminal (
    PUSB_AUDIO_INPUT_TERMINAL_DESCRIPTOR ITDesc
)
{
    PCHAR pStr = NULL;

    if (ITDesc->bLength != sizeof(USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Input Terminal Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     ITDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     ITDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (INPUT_TERMINAL)\r\n",
                     ITDesc->bDescriptorSubtype);

    AppendTextBuffer("bTerminalID:                       0x%02X\r\n",
                     ITDesc->bTerminalID);

    AppendTextBuffer("wTerminalType:                   0x%04X",
                     ITDesc->wTerminalType);
    pStr = GetStringFromList(slAudioTerminalTypes, 
            sizeof(slAudioTerminalTypes) / sizeof(STRINGLIST),
            ITDesc->wTerminalType, 
            "Invalid AC Input Terminal Type");
    AppendTextBuffer(" (%s)\r\n", pStr);

    AppendTextBuffer("bAssocTerminal:                    0x%02X\r\n",
                     ITDesc->bAssocTerminal);

    AppendTextBuffer("bNrChannels:                       0x%02X\r\n",
                     ITDesc->bNrChannels);

    AppendTextBuffer("wChannelConfig:                  0x%04X\r\n",
                     ITDesc->wChannelConfig);

    DisplayChannelConfig((DWORD) ITDesc->wChannelConfig);

    AppendTextBuffer("iChannelNames:                     0x%02X\r\n",
                     ITDesc->iChannelNames);

    AppendTextBuffer("iTerminal:                         0x%02X\r\n",
                     ITDesc->iTerminal);


    return TRUE;
}


/*****************************************************************************

 DisplayACOutputTerminal()

*****************************************************************************/

BOOL
DisplayACOutputTerminal (
    PUSB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR OTDesc
)
{
    PCHAR pStr = NULL;

    if (OTDesc->bLength != sizeof(USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Output Terminal Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     OTDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     OTDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (OUTPUT_TERMINAL)\r\n",
                     OTDesc->bDescriptorSubtype);

    AppendTextBuffer("bTerminalID:                       0x%02X\r\n",
                     OTDesc->bTerminalID);

    AppendTextBuffer("wTerminalType:                   0x%04X",
                     OTDesc->wTerminalType);

    pStr = GetStringFromList(slAudioTerminalTypes, 
            sizeof(slAudioTerminalTypes) / sizeof(STRINGLIST),
            OTDesc->wTerminalType, 
            "Invalid AC Output Terminal Type");
    AppendTextBuffer(" (%s)\r\n", pStr);

    AppendTextBuffer("bAssocTerminal:                    0x%02X\r\n",
                     OTDesc->bAssocTerminal);

    AppendTextBuffer("bSourceID:                         0x%02X\r\n",
                     OTDesc->bSourceID);

    AppendTextBuffer("iTerminal:                         0x%02X\r\n",
                     OTDesc->iTerminal);


    return TRUE;
}


/*****************************************************************************

 DisplayACMixerUnit()

*****************************************************************************/

BOOL
DisplayACMixerUnit (
    PUSB_AUDIO_MIXER_UNIT_DESCRIPTOR MixerDesc
)
{
    UCHAR  i = 0;
    PUCHAR data = NULL;

    if (MixerDesc->bLength < 10)
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Mixer Unit Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     MixerDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     MixerDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (MIXER_UNIT)\r\n",
                     MixerDesc->bDescriptorSubtype);

    AppendTextBuffer("bUnitID:                           0x%02X\r\n",
                     MixerDesc->bUnitID);

    AppendTextBuffer("bNrInPins:                         0x%02X\r\n",
                     MixerDesc->bNrInPins);

    for (i=0; i<MixerDesc->bNrInPins; i++)
    {
        AppendTextBuffer("baSourceID[%d]:                     0x%02X\r\n",
                        i+1,
                        MixerDesc->baSourceID[i]);
    }

    data = &MixerDesc->baSourceID[MixerDesc->bNrInPins];

    AppendTextBuffer("bNrChannels:                       0x%02X\r\n",
                     *data++);

    AppendTextBuffer("wChannelConfig:                  0x%04X\r\n",
                     *(PUSHORT)data);

    DisplayChannelConfig((DWORD) *data);

    data = (PUCHAR) ((PUSHORT) data + 1);

    AppendTextBuffer("iChannelNames:                     0x%02X\r\n",
                     *data++);

    AppendTextBuffer("bmControls:\r\n");

    i = MixerDesc->bLength - 10 - MixerDesc->bNrInPins;

    DisplayBytes(data, i);

    data += i;

    AppendTextBuffer("iMixer:                            0x%02X\r\n",
                     *data);

    return TRUE;
}


/*****************************************************************************

 DisplayACSelectorUnit()

*****************************************************************************/

BOOL
DisplayACSelectorUnit (
    PUSB_AUDIO_SELECTOR_UNIT_DESCRIPTOR SelectorDesc
)
{
    UCHAR  i = 0;
    PUCHAR data = NULL;

    if (SelectorDesc->bLength < 6)
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Selector Unit Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     SelectorDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     SelectorDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (SELECTOR_UNIT)\r\n",
                     SelectorDesc->bDescriptorSubtype);

    AppendTextBuffer("bUnitID:                           0x%02X\r\n",
                     SelectorDesc->bUnitID);

    AppendTextBuffer("bNrInPins:                         0x%02X\r\n",
                     SelectorDesc->bNrInPins);

    for (i=0; i<SelectorDesc->bNrInPins; i++)
    {
        AppendTextBuffer("baSourceID[%d]:                     0x%02X\r\n",
                        i+1,
                        SelectorDesc->baSourceID[i]);
    }

    data = &SelectorDesc->baSourceID[SelectorDesc->bNrInPins];

    AppendTextBuffer("iSelector:                         0x%02X\r\n",
                     *data);

    return TRUE;
}


/*****************************************************************************

 DisplayACFeatureUnit()

*****************************************************************************/

BOOL
DisplayACFeatureUnit (
    PUSB_AUDIO_FEATURE_UNIT_DESCRIPTOR FeatureDesc
)
{
    UCHAR  i = 0;
    UCHAR  n = 0;
    UCHAR  ch = 0;
    PUCHAR data = NULL;

    AppendTextBuffer("\r\n          ===>Audio Control Feature Unit Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     FeatureDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     FeatureDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (FEATURE_UNIT)\r\n",
                     FeatureDesc->bDescriptorSubtype);

    AppendTextBuffer("bUnitID:                           0x%02X\r\n",
                     FeatureDesc->bUnitID);

    AppendTextBuffer("bSourceID:                         0x%02X\r\n",
                     FeatureDesc->bSourceID);

    AppendTextBuffer("bControlSize:                      0x%02X\r\n",
                     FeatureDesc->bControlSize);


    if (FeatureDesc->bLength < 7)
    {
        AppendTextBuffer("*!*WARNING:    bLength is invalid (< 7)\r\n");
        OOPS();
        return FALSE;
    }
    else if(FeatureDesc->bLength == 7)
    {
        AppendTextBuffer("Audio controls are not available (bLength = 7)\r\n");
        return TRUE;
    }

    n = FeatureDesc->bControlSize;

    if(n == 0)
    {
        AppendTextBuffer("Audio controls are not available (bControlSize = 0)\r\n");
        return TRUE;
    }

    ch = ((FeatureDesc->bLength - 7) / n) - 1;

    // Check if there are extra bytes in descriptor based on formula in Spec
    if (FeatureDesc->bLength != (7 + (ch + 1) * n))
    {
        // The descriptor length is greater than number of bmaControls
        AppendTextBuffer("*!*WARNING:    bLength is greater than number of bmaControls (bLength > ( 7 + (ch + 1) * n)\r\n");
    }

    data = &FeatureDesc->bmaControls[0];

    if (ch == (UCHAR) -1)
    {
        // This should not happen, but this check is put in place so we don't loop for a long time below
        AppendTextBuffer("*!*WARNING:    Either bLength or bControlSize are invalid. The calculated logical channel count is -1. ((bLength - 7)/ n) - 1\r\n");
        OOPS();
        return FALSE;
    }

    for (i=0; i<=ch; i++)
    {
        if (0 == i)
        {
            AppendTextBuffer("bmaControls[master]:               ");
        }
        else
        {
            AppendTextBuffer("bmaControls[channel %d]:            ", i-1);
        }
        DisplayBytes(data, n);

        if (n == 1)
        {
            for (int j = 0; j < ARRAYSIZE(slFeatureUnitTypes); j++)
            {
                DWORD dwData = (DWORD) *data;
                if (dwData & slFeatureUnitTypes[j].ulFlag)
                {
                    AppendTextBuffer("                                   (%s)\r\n", slFeatureUnitTypes[j].pszString);
                }
            }
        }

        data += n;
    }


    AppendTextBuffer("iFeature:                          0x%02X\r\n",
                     *data);

    return TRUE;
}


/*****************************************************************************

 DisplayACProcessingUnit()

*****************************************************************************/

BOOL
DisplayACProcessingUnit (
    PUSB_AUDIO_PROCESSING_UNIT_DESCRIPTOR ProcessingDesc
)
{
    UCHAR  i = 0;
    PUCHAR data = NULL;
    PCHAR pStr = NULL;

    if (ProcessingDesc->bLength < sizeof(USB_AUDIO_PROCESSING_UNIT_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Processing Unit Descriptor<===\r\n");

    AppendTextBuffer("bLength:                          0x%02X\r\n",
                     ProcessingDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     ProcessingDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (PROCESSING_UNIT)\r\n",
                     ProcessingDesc->bDescriptorSubtype);

    AppendTextBuffer("bUnitID:                           0x%02X\r\n",
                     ProcessingDesc->bUnitID);

    AppendTextBuffer("wProcessType:                    0x%04X",
                     ProcessingDesc->wProcessType);

    pStr = GetStringFromList(slProcessingUnitTypes, 
            sizeof(slProcessingUnitTypes) / sizeof(STRINGLIST),
            ProcessingDesc->wProcessType, 
            "Invalid Processing Unit Type");
    AppendTextBuffer("                                    (%s)\r\n", pStr);

    AppendTextBuffer("bNrInPins:                         0x%02X\r\n",
                     ProcessingDesc->bNrInPins);

    for (i=0; i<ProcessingDesc->bNrInPins; i++)
    {
        AppendTextBuffer("baSourceID[%d]:                     0x%02X\r\n",
                        i+1,
                        ProcessingDesc->baSourceID[i]);
    }

    data = &ProcessingDesc->baSourceID[ProcessingDesc->bNrInPins];

    AppendTextBuffer("bNrChannels:                       0x%02X\r\n",
                     *data++);

    AppendTextBuffer("wChannelConfig:                  0x%04X\r\n",
                     *(PUSHORT)data);

    DisplayChannelConfig((DWORD) *data);

    data = (PUCHAR) ((PUSHORT) data + 1);

    AppendTextBuffer("iChannelNames:                     0x%02X\r\n",
                     *data++);

    i = *data++;

    AppendTextBuffer("bControlSize:                      0x%02X\r\n",
                     i);

    AppendTextBuffer("bmControls:\r\n");

    DisplayBytes(data, i);

    data += i;

    AppendTextBuffer("iProcessing:                       0x%02X\r\n",
                     *data++);


    i = ProcessingDesc->bLength - 13 - ProcessingDesc->bNrInPins - i;

    if (i)
    {
        AppendTextBuffer("Process Specific:\r\n");

        DisplayBytes(data, i);
    }

    return TRUE;
}


/*****************************************************************************

 DisplayACExtensionUnit()

*****************************************************************************/

BOOL
DisplayACExtensionUnit (
    PUSB_AUDIO_EXTENSION_UNIT_DESCRIPTOR ExtensionDesc
)
{
    UCHAR  i = 0;
    PUCHAR data = NULL;

    if (ExtensionDesc->bLength < 13)
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Control Extension Unit Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     ExtensionDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     ExtensionDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (EXTENSION_UNIT)\r\n",
                     ExtensionDesc->bDescriptorSubtype);

    AppendTextBuffer("bUnitID:                           0x%02X\r\n",
                     ExtensionDesc->bUnitID);

    AppendTextBuffer("wExtensionCode:                  0x%04X\r\n",
                     ExtensionDesc->wExtensionCode);


    AppendTextBuffer("bNrInPins:                         0x%02X\r\n",
                     ExtensionDesc->bNrInPins);

    for (i=0; i<ExtensionDesc->bNrInPins; i++)
    {
        AppendTextBuffer("baSourceID[%d]:                     0x%02X\r\n",
                        i+1,
                        ExtensionDesc->baSourceID[i]);
    }

    data = &ExtensionDesc->baSourceID[ExtensionDesc->bNrInPins];

    AppendTextBuffer("bNrChannels:                       0x%02X\r\n",
                     *data++);

    AppendTextBuffer("wChannelConfig:                  0x%04X\r\n",
                     *(PUSHORT)data);

    DisplayChannelConfig((DWORD) *data);

    data = (PUCHAR) ((PUSHORT) data + 1);

    AppendTextBuffer("iChannelNames:                     0x%02X\r\n",
                     *data++);

    i = *data++;

    AppendTextBuffer("bControlSize:                      0x%02X\r\n",
                     i);

    AppendTextBuffer("bmControls:\r\n");

    DisplayBytes(data, i);

    data += i;

    AppendTextBuffer("iExtension:                        0x%02X\r\n",
                     *data);
    return TRUE;
}


/*****************************************************************************

 DisplayASGeneral()

*****************************************************************************/

BOOL
DisplayASGeneral (
    PUSB_AUDIO_GENERAL_DESCRIPTOR GeneralDesc
)
{
    PCHAR pStr = NULL;

    if (GeneralDesc->bLength != sizeof(USB_AUDIO_GENERAL_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Streaming Class Specific Interface Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     GeneralDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     GeneralDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (AS_GENERAL)\r\n",
                     GeneralDesc->bDescriptorSubtype);

    AppendTextBuffer("bTerminalLink:                     0x%02X\r\n",
                     GeneralDesc->bTerminalLink);

    AppendTextBuffer("bDelay:                            0x%02X\r\n",
                     GeneralDesc->bDelay);

    AppendTextBuffer("wFormatTag:                      0x%04X",
                     GeneralDesc->wFormatTag);

    pStr = GetStringFromList(slAudioFormatTypes, 
            sizeof(slAudioFormatTypes) / sizeof(STRINGLIST),
            GeneralDesc->wFormatTag, 
            "Invalid AC Format Type");
    AppendTextBuffer(" (%s)\r\n", pStr);

    return TRUE;
}


/*****************************************************************************

 DisplayAudioStreamingEndpoint()

*****************************************************************************/

BOOL
DisplayAudioStreamingEndpoint (
    PUSB_AUDIO_AS_ENDPOINT_DESCRIPTOR EndpointDesc
)
{
    PCHAR pStr = NULL;

    if (EndpointDesc->bLength != sizeof(USB_AUDIO_AS_ENDPOINT_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Streaming Class Specific Audio Data Endpoint Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     EndpointDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_ENDPOINT)\r\n",
                     EndpointDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (EP_GENERAL)\r\n",
                     EndpointDesc->bDescriptorSubtype);

    AppendTextBuffer("bmAttributes:                      0x%02X\r\n",
                     EndpointDesc->bmAttributes);

    for (int j = 0; j < ARRAYSIZE(slIsocAttributes); j++)
    {
        DWORD dwData = (DWORD) EndpointDesc->bmAttributes;
        if (dwData & slIsocAttributes[j].ulFlag)
        {
            AppendTextBuffer("                                   (%s)\r\n", slIsocAttributes[j].pszString);
        }
    }

    AppendTextBuffer("bLockDelayUnits:                   0x%02X",
                     EndpointDesc->bLockDelayUnits);

    pStr = GetStringFromList(slLockDelayUnits, 
            sizeof(slLockDelayUnits) / sizeof(STRINGLIST),
            EndpointDesc->bLockDelayUnits, 
            "Invalid Lock Delay Units");
    AppendTextBuffer(" (%s)\r\n", pStr);


    AppendTextBuffer("wLockDelay:                      0x%04X\r\n",
                     EndpointDesc->wLockDelay);

    return TRUE;
}


/*****************************************************************************

 DisplayASFormatType()

*****************************************************************************/

BOOL
DisplayASFormatType (
    PUSB_AUDIO_COMMON_FORMAT_DESCRIPTOR FormatDesc
)
{
    UCHAR  i = 0;
    UCHAR  n = 0;
    ULONG  freq = 0;
    PUCHAR data = NULL;
    PCHAR  pStr = NULL;

    if (FormatDesc->bLength < sizeof(USB_AUDIO_COMMON_FORMAT_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>Audio Streaming Format Type Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     FormatDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     FormatDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (FORMAT_TYPE)\r\n",
                     FormatDesc->bDescriptorSubtype);

    AppendTextBuffer("bFormatType:                       0x%02X",
                     FormatDesc->bFormatType);

    pStr = GetStringFromList(slFormatTypeCode, 
            sizeof(slFormatTypeCode) / sizeof(STRINGLIST),
            FormatDesc->bFormatType, 
            "Invalid AC Format Type");
    AppendTextBuffer(" (%s)\r\n", pStr);

    if (FormatDesc->bFormatType == 0x01 ||
        FormatDesc->bFormatType == 0x03)
    {
        PUSB_AUDIO_TYPE_I_OR_III_FORMAT_DESCRIPTOR FormatI_IIIDesc;

        FormatI_IIIDesc = (PUSB_AUDIO_TYPE_I_OR_III_FORMAT_DESCRIPTOR)FormatDesc;

        AppendTextBuffer("bNrChannels:                       0x%02X\r\n",
                         FormatI_IIIDesc->bNrChannels);

        AppendTextBuffer("bSubframeSize:                     0x%02X\r\n",
                         FormatI_IIIDesc->bSubframeSize);

        AppendTextBuffer("bBitResolution:                    0x%02X (%d)\r\n",
                         FormatI_IIIDesc->bBitResolution, FormatI_IIIDesc->bBitResolution);

        AppendTextBuffer("bSamFreqType:                      0x%02X",
                         FormatI_IIIDesc->bSamFreqType);

        if (0 == FormatI_IIIDesc->bSamFreqType)
        {
            AppendTextBuffer(" (Continuous)\r\n", pStr);
        }
        else
        {
            AppendTextBuffer(" (Discrete)\r\n", pStr);
        }

        data = (PUCHAR)(FormatI_IIIDesc + 1);

        n = FormatI_IIIDesc->bSamFreqType;

    }
    else if (FormatDesc->bFormatType == 0x02)
    {
        PUSB_AUDIO_TYPE_II_FORMAT_DESCRIPTOR FormatIIDesc;

        FormatIIDesc = (PUSB_AUDIO_TYPE_II_FORMAT_DESCRIPTOR)FormatDesc;

        AppendTextBuffer("wMaxBitRate:                     0x%04X\r\n",
                         FormatIIDesc->wMaxBitRate);

        AppendTextBuffer("wSamplesPerFrame:                0x%04X\r\n",
                         FormatIIDesc->wSamplesPerFrame);

        AppendTextBuffer("bSamFreqType:                      0x%02X",
                         FormatIIDesc->bSamFreqType);

        if (0 == FormatIIDesc->bSamFreqType)
        {
            AppendTextBuffer(" (Continuous)\r\n", pStr);
        }
        else
        {
            AppendTextBuffer(" (Discrete)\r\n", pStr);
        }

        data = (PUCHAR)(FormatIIDesc + 1);

        n = FormatIIDesc->bSamFreqType;
    }
    else
    {
        data = NULL;
    }

    if (data != NULL)
    {
        if (n == 0)
        {
            freq = (data[0]) + (data[1] << 8) + (data[2] << 16);
            data += 3;

            AppendTextBuffer("tLowerSamFreq:                 0x%06X (%d Hz)\r\n",
                             freq,
                             freq);

            freq = (data[0]) + (data[1] << 8) + (data[2] << 16);
            data += 3;

            AppendTextBuffer("tUpperSamFreq:                 0x%06X (%d Hz)\r\n",
                             freq,
                             freq);
        }
        else
        {
            for (i=0; i<n; i++)
            {
                freq = (data[0]) + (data[1] << 8) + (data[2] << 16);
                data += 3;

                AppendTextBuffer("tSamFreq[%d]:                   0x%06X (%d Hz)\r\n",
                                 i+1,
                                 freq,
                                 freq);
            }
        }
    }

    return TRUE;
}


/*****************************************************************************

 DisplayASFormatSpecific()

*****************************************************************************/

BOOL
DisplayASFormatSpecific (
    PUSB_AUDIO_COMMON_DESCRIPTOR CommonDesc
)
{
    AppendTextBuffer("\r\n          ===>Audio Streaming Format Specific Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
                     CommonDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
                     CommonDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (FORMAT_SPECIFIC)\r\n",
                     CommonDesc->bDescriptorSubtype);

    DisplayBytes((PUCHAR)(CommonDesc + 1),
                 CommonDesc->bLength);

    return TRUE;
}

BOOL
DisplayMSInterfaceHeader(
    PUSB_AUDIO_MS_INTERFACE_HEADER_DESCRIPTOR HeaderDesc
)
{
    AppendTextBuffer("\r\n          ===>MIDI Streaming Interface Header Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
        HeaderDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
        HeaderDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (MS_HEADER)\r\n",
        HeaderDesc->bDescriptorSubtype);

    AppendTextBuffer("bcdMSC:                            0x%04X\r\n",
        HeaderDesc->bcdMSC);

    AppendTextBuffer("wTotalLength:                      0x%04X\r\n",
        HeaderDesc->wTotalLength);
    return TRUE;
}

PCHAR
DescribeMidiJackType(
    UCHAR JackType
)
{
    if (JackType == USB_AUDIO_MS_JACK_TYPE_EMBEDDED)
    {
        return "EMBEDDED";
    }
    if (JackType == USB_AUDIO_MS_JACK_TYPE_EXTERNAL)
    {
        return "EXTERNAL";
    }
    return "JACK_TYPE_UNDEFINED";
}

BOOL
DisplayMSMIDIInJack(
    PUSB_AUDIO_MS_MIDI_IN_JACK_DESCRIPTOR JackDesc,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
)
{
    AppendTextBuffer("\r\n          ===>MIDI IN Jack Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
        JackDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
        JackDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (MIDI_IN_JACK)\r\n",
        JackDesc->bDescriptorSubtype);

    AppendTextBuffer("bJackType:                         0x%02X (%s)\r\n",
        JackDesc->bJackType,
        DescribeMidiJackType(JackDesc->bJackType));

    AppendTextBuffer("bJackID:                           0x%02X\r\n",
        JackDesc->bJackType);

    if (JackDesc->iJack)
    {
        DisplayStringDescriptor(JackDesc->iJack, StringDescs, LatestDevicePowerState);
    }
    return TRUE;
}

BOOL
DisplayMSMIDIOutJack(
    PUSB_AUDIO_MS_MIDI_OUT_JACK_DESCRIPTOR JackDesc,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
)
{
    UCHAR i, iJack;
    PUSB_AUDIO_MS_MIDI_OUT_JACK_PIN_DESCRIPTOR PinDesc = (PUSB_AUDIO_MS_MIDI_OUT_JACK_PIN_DESCRIPTOR)(JackDesc + 1);

    AppendTextBuffer("\r\n          ===>MIDI OUT Jack Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
        JackDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
        JackDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (MIDI_OUT_JACK)\r\n",
        JackDesc->bDescriptorSubtype);

    AppendTextBuffer("bJackType:                         0x%02X (%s)\r\n",
        JackDesc->bJackType,
        DescribeMidiJackType(JackDesc->bJackType));

    AppendTextBuffer("bJackID:                           0x%02X\r\n",
        JackDesc->bJackID);

    AppendTextBuffer("bNrInputPins:                      0x%02X\r\n",
        JackDesc->bNrInputPins);

    for (i = 0; i < JackDesc->bNrInputPins; i++)
    {
        AppendTextBuffer("baSourceID(%d):                     0x%02X\r\n",
            i + 1, PinDesc->baSourceID);

        AppendTextBuffer("baSourcePin(%d):                    0x%02X\r\n",
            i + 1, PinDesc->baSourcePin);

        PinDesc++;
    }

    iJack = *((UCHAR *)PinDesc);

    if (iJack)
    {
        DisplayStringDescriptor(iJack, StringDescs, LatestDevicePowerState);
    }
    return TRUE;
}

BOOL
DisplayMSElement(
    PUSB_AUDIO_MS_ELEMENT_DESCRIPTOR ElementDesc,
    PSTRING_DESCRIPTOR_NODE      StringDescs,
    DEVICE_POWER_STATE           LatestDevicePowerState
)
{
    UCHAR i, iElement;
    PUSB_AUDIO_MS_ELEMENT_SOURCE_DESCRIPTOR SourceDesc = (PUSB_AUDIO_MS_ELEMENT_SOURCE_DESCRIPTOR)(ElementDesc + 1);
    PUSB_AUDIO_MS_ELEMENT_FOOTER_DESCRIPTOR FooterDesc;

    AppendTextBuffer("\r\n          ===>MIDI Element Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
        ElementDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_INTERFACE)\r\n",
        ElementDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (ELEMENT)\r\n",
        ElementDesc->bDescriptorSubtype);

    AppendTextBuffer("bElementID:                        0x%02X\r\n",
        ElementDesc->bElementID);

    AppendTextBuffer("bNrInputPins:                      0x%02X\r\n",
        ElementDesc->bNrInputPins);

    for (i = 0; i < ElementDesc->bNrInputPins; i++)
    {
    AppendTextBuffer("baSourceID(%d):                    0x%02X\r\n",
            i, SourceDesc->baSourceID);

    AppendTextBuffer("baSourcePin(%d):                   0x%02X\r\n",
            i, SourceDesc->baSourcePin);

        SourceDesc++;
    }

    FooterDesc = (PUSB_AUDIO_MS_ELEMENT_FOOTER_DESCRIPTOR) SourceDesc;

    AppendTextBuffer("bNrOutputPins:                      0x%02X\r\n",
        FooterDesc->bNrOutputPins);

    AppendTextBuffer("bInTerminalLink:                      0x%02X\r\n",
        FooterDesc->bInTerminalLink);

    AppendTextBuffer("bOutTerminalLink:                      0x%02X\r\n",
        FooterDesc->bOutTerminalLink);

    AppendTextBuffer("bElCapsSize:                      0x%02X\r\n",
        FooterDesc->bInTerminalLink);

    FooterDesc++;
    iElement = *((UCHAR*)FooterDesc);

    if (iElement)
    {
        DisplayStringDescriptor(iElement, StringDescs, LatestDevicePowerState);
    }
    return TRUE;
}

BOOL
DisplayMidiStreamingEndpoint(
    PUSB_AUDIO_MS_ENDPOINT_DESCRIPTOR EndpointDesc
)
{
    UCHAR i, *j;
    if (EndpointDesc->bLength < sizeof(USB_AUDIO_MS_ENDPOINT_DESCRIPTOR))
    {
        OOPS();
        return FALSE;
    }

    AppendTextBuffer("\r\n          ===>MIDI Streaming Class Specific Audio Data Endpoint Descriptor<===\r\n");

    AppendTextBuffer("bLength:                           0x%02X\r\n",
        EndpointDesc->bLength);

    AppendTextBuffer("bDescriptorType:                   0x%02X (CS_ENDPOINT)\r\n",
        EndpointDesc->bDescriptorType);

    AppendTextBuffer("bDescriptorSubtype:                0x%02X (EP_GENERAL)\r\n",
        EndpointDesc->bDescriptorSubtype);

    AppendTextBuffer("bNumEmbMIDIJack:                   0x%02X\r\n",
        EndpointDesc->bNumEmbMIDIJack);

    j = (PUCHAR)(EndpointDesc + 1);

    for (i = 0; i < EndpointDesc->bNumEmbMIDIJack; i++)
    {
    AppendTextBuffer("baAssocJackID(%d):                  0x%02X\r\n",
        i + 1,
        *(j + i));

    }

    return TRUE;
}

/*****************************************************************************

 DisplayBytes()

*****************************************************************************/

VOID
DisplayBytes (
    PUCHAR Data,
    USHORT Len
)
{
    USHORT i;

    for (i = 0; i < Len; i++)
    {
        AppendTextBuffer("%02X ", Data[i]);

        if (i % 16 == 15)
        {
            AppendTextBuffer("\r\n");
        }
    }

    if (i % 16 != 0)
    {
        AppendTextBuffer("\r\n");
    }
}


