/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerhstoptable.h

Abstract:

    Declaration of topology tables for the speaker (external: headset).

--*/

#ifndef _SYSVAD_SPEAKERHSTOPTABLE_H_
#define _SYSVAD_SPEAKERHSTOPTABLE_H_

//=============================================================================
static
KSDATARANGE SpeakerHsTopoPinDataRangesBridge[] =
{
 {
   sizeof(KSDATARANGE),
   0,
   0,
   0,
   STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
   STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
   STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
 }
};

//=============================================================================
static
PKSDATARANGE SpeakerHsTopoPinDataRangePointersBridge[] =
{
  &SpeakerHsTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR SpeakerHsTopoMiniportPins[] =
{
  // KSPIN - topology filter in-pin
  {
    0,
    0,
    0,                                                  // InstanceCount
    NULL,                                               // AutomationTable
    {                                                   // KsPinDescriptor
      0,                                                // InterfacesCount
      NULL,                                             // Interfaces
      0,                                                // MediumsCount
      NULL,                                             // Mediums
      SIZEOF_ARRAY(SpeakerHsTopoPinDataRangePointersBridge),// DataRangesCount
      SpeakerHsTopoPinDataRangePointersBridge,          // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },
  // KSPIN - topology filter out-pin
  {
    0,
    0,
    0,                                                  // InstanceCount
    NULL,                                               // AutomationTable
    {                                                   // KsPinDescriptor
      0,                                                // InterfacesCount
      NULL,                                             // Interfaces
      0,                                                // MediumsCount
      NULL,                                             // Mediums
      SIZEOF_ARRAY(SpeakerHsTopoPinDataRangePointersBridge),// DataRangesCount
      SpeakerHsTopoPinDataRangePointersBridge,          // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HEADSET_SPEAKERS,                     // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

static
KSJACK_DESCRIPTION SpeakerHsJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    JACKDESC_RGB(179, 201, 140),
    eConnTypeCombination,
    eGeoLocRear,
    eGenLocPrimaryBox,
    ePortConnJack,
    FALSE               // NOTE: For convenience, wired headset jacks will be "unplugged" at boot.
                        // However, we need to introduce a test hook to toggle jack state of this and other endpoints.                                 
};

//=============================================================================
// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION SpeakerHsJackDescriptions[] =
{
    NULL,
    &SpeakerHsJackDesc
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR SpeakerHsTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_WAVEOUT_SOURCE,      PCFILTER_NODE,               KSPIN_TOPO_LINEOUT_DEST}
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpeakerHsTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpeakerHsTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpeakerHsTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHsTopoFilter, PropertiesSpeakerHsTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpeakerHsTopoMiniportFilterDescriptor =
{
  0,                                                // Version
  &AutomationSpeakerHsTopoFilter,                   // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                         // PinSize
  SIZEOF_ARRAY(SpeakerHsTopoMiniportPins),          // PinCount
  SpeakerHsTopoMiniportPins,                        // Pins
  sizeof(PCNODE_DESCRIPTOR),                        // NodeSize
  0,                                                // NodeCount
  NULL,                                             // Nodes
  SIZEOF_ARRAY(SpeakerHsTopoMiniportConnections),   // ConnectionCount
  SpeakerHsTopoMiniportConnections,                 // Connections
  0,                                                // CategoryCount
  NULL                                              // Categories
};

#endif // _SYSVAD_SPEAKERHSTOPTABLE_H_
    

