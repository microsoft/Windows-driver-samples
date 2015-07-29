/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerhptoptable.h

Abstract:

    Declaration of topology tables for the speaker (external: headphone).

--*/

#ifndef _SYSVAD_SPEAKERHPTOPTABLE_H_
#define _SYSVAD_SPEAKERHPTOPTABLE_H_

//=============================================================================
static
KSJACK_DESCRIPTION SpeakerHpJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    JACKDESC_RGB(179, 201, 140),
    eConnTypeCombination,
    eGeoLocRear,
    eGenLocPrimaryBox,
    ePortConnJack,
    TRUE
};

//=============================================================================
static
KSDATARANGE SpeakerHpTopoPinDataRangesBridge[] =
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
PKSDATARANGE SpeakerHpTopoPinDataRangePointersBridge[] =
{
  &SpeakerHpTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR SpeakerHpTopoMiniportPins[] =
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
      SIZEOF_ARRAY(SpeakerHpTopoPinDataRangePointersBridge),// DataRangesCount
      SpeakerHpTopoPinDataRangePointersBridge,          // DataRanges
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
      SIZEOF_ARRAY(SpeakerHpTopoPinDataRangePointersBridge),// DataRangesCount
      SpeakerHpTopoPinDataRangePointersBridge,          // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HEADPHONES,                           // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION SpeakerHpJackDescriptions[] =
{
    NULL,
    &SpeakerHpJackDesc
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR SpeakerHpTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_WAVEOUT_SOURCE,      PCFILTER_NODE,               KSPIN_TOPO_LINEOUT_DEST}
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpeakerHpTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpeakerHpTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpeakerHpTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpeakerHpTopoFilter, PropertiesSpeakerHpTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpeakerHpTopoMiniportFilterDescriptor =
{
  0,                                                // Version
  &AutomationSpeakerHpTopoFilter,                   // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                         // PinSize
  SIZEOF_ARRAY(SpeakerHpTopoMiniportPins),          // PinCount
  SpeakerHpTopoMiniportPins,                        // Pins
  sizeof(PCNODE_DESCRIPTOR),                        // NodeSize
  0,                                                // NodeCount
  NULL,                                             // Nodes
  SIZEOF_ARRAY(SpeakerHpTopoMiniportConnections),   // ConnectionCount
  SpeakerHpTopoMiniportConnections,                 // Connections
  0,                                                // CategoryCount
  NULL                                              // Categories
};

#endif // _SYSVAD_SPEAKERHPTOPTABLE_H_
    

