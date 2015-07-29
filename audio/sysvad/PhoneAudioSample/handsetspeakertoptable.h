/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    handsetspeakertoptable.h

Abstract:

    Declaration of topology tables for the handsetspeaker endpoint.

--*/

#ifndef _SYSVAD_HANDSETSPEAKERTOPTABLE_H_
#define _SYSVAD_HANDSETSPEAKERTOPTABLE_H_

//=============================================================================
static
KSDATARANGE HandsetSpeakerTopoPinDataRangesBridge[] =
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
PKSDATARANGE HandsetSpeakerTopoPinDataRangePointersBridge[] =
{
  &HandsetSpeakerTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR HandsetSpeakerTopoMiniportPins[] =
{
  // KSPIN_TOPO_WAVEOUT_SOURCE
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
      SIZEOF_ARRAY(HandsetSpeakerTopoPinDataRangePointersBridge), // DataRangesCount
      HandsetSpeakerTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },
  // KSPIN_TOPO_LINEOUT_DEST
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
      SIZEOF_ARRAY(HandsetSpeakerTopoPinDataRangePointersBridge), // DataRangesCount
      HandsetSpeakerTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HANDSET,                              // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
static
KSJACK_DESCRIPTION HandsetSpeakerJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    0x0000,               // no color
    eConnTypeUnknown,
    eGeoLocBottom,
    eGenLocInternal,
    ePortConnIntegratedDevice,
    TRUE
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION HandsetSpeakerJackDescriptions[] =
{
    NULL,
    &HandsetSpeakerJackDesc
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR HandsetSpeakerTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_WAVEOUT_SOURCE,      PCFILTER_NODE,               KSPIN_TOPO_LINEOUT_DEST}
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesHandsetSpeakerTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_HandsetSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_HandsetSpeakerTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetSpeakerTopoFilter, PropertiesHandsetSpeakerTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR HandsetSpeakerTopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationHandsetSpeakerTopoFilter,                    // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(HandsetSpeakerTopoMiniportPins),           // PinCount
  HandsetSpeakerTopoMiniportPins,                         // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  0,                                            // NodeCount
  NULL,                                         // Nodes
  SIZEOF_ARRAY(HandsetSpeakerTopoMiniportConnections),    // ConnectionCount
  HandsetSpeakerTopoMiniportConnections,                  // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_HANDSETSPEAKERTOPTABLE_H_
    

