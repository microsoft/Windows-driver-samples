/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    a2dphpspeakertoptable.h

Abstract:

    Declaration of topology tables for the A2DP Headphone speaker (external).

--*/

#ifndef _SYSVAD_A2DPHPSPEAKERTOPTABLE_H_
#define _SYSVAD_A2DPHPSPEAKERTOPTABLE_H_

#include "a2dphpspeakertopo.h"

//=============================================================================
static
KSDATARANGE A2dpHpSpeakerTopoPinDataRangesBridge[] =
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
PKSDATARANGE A2dpHpSpeakerTopoPinDataRangePointersBridge[] =
{
  &A2dpHpSpeakerTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR A2dpHpSpeakerTopoMiniportPins[] =
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
      SIZEOF_ARRAY(A2dpHpSpeakerTopoPinDataRangePointersBridge),// DataRangesCount
      A2dpHpSpeakerTopoPinDataRangePointersBridge,      // DataRanges
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
      SIZEOF_ARRAY(A2dpHpSpeakerTopoPinDataRangePointersBridge),// DataRangesCount
      A2dpHpSpeakerTopoPinDataRangePointersBridge,      // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HEADSET_SPEAKERS,                     // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
static
KSJACK_DESCRIPTION A2dpHpSpeakerJackDesc =
{
    KSAUDIO_SPEAKER_STEREO,
    0,                      // color
    eConnTypeOtherDigital,
    eGeoLocBottom,
    eGenLocPrimaryBox,
    ePortConnJack,
    FALSE                   // run-time code sets this value.
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION A2dpHpSpeakerJackDescriptions[] =
{
    NULL,
    &A2dpHpSpeakerJackDesc
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR A2dpHpSpeakerTopoMiniportConnections[] =
{
  //  FromNode,             FromPin,                    ToNode,                 ToPin
  {   PCFILTER_NODE,        KSPIN_TOPO_WAVEOUT_SOURCE,  PCFILTER_NODE,          KSPIN_TOPO_LINEOUT_DEST }
};

//=============================================================================
static
PCPROPERTY_ITEM A2dpHpSpeakerPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_CONTAINERID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_BtAudio,
        KSPROPERTY_ONESHOT_RECONNECT,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_BtAudio,
        KSPROPERTY_ONESHOT_DISCONNECT,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_A2dpHpSpeakerTopoFilter
    }
};

static 
PCEVENT_ITEM A2dpHpSpeakerPropertiesTopoFilterEvent[] =
{
  {
    &KSEVENTSETID_PinCapsChange,                        // Something changed!
    KSEVENT_PINCAPS_JACKINFOCHANGE,                     // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_A2dpHpSpeakerTopoFilterEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationA2dpHpSpeakerTopoFilterWithEvent, 
                               A2dpHpSpeakerPropertiesTopoFilter,
                               A2dpHpSpeakerPropertiesTopoFilterEvent);

//=============================================================================
static
PCFILTER_DESCRIPTOR A2dpHpSpeakerTopoMiniportFilterDescriptor =
{
  0,                                                    // Version
  &AutomationA2dpHpSpeakerTopoFilterWithEvent,          // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                             // PinSize
  SIZEOF_ARRAY(A2dpHpSpeakerTopoMiniportPins),          // PinCount
  A2dpHpSpeakerTopoMiniportPins,                        // Pins
  sizeof(PCNODE_DESCRIPTOR),                            // NodeSize
  0,                                                    // NodeCount
  NULL,                                                 // Nodes
  SIZEOF_ARRAY(A2dpHpSpeakerTopoMiniportConnections),   // ConnectionCount
  A2dpHpSpeakerTopoMiniportConnections,                 // Connections
  0,                                                    // CategoryCount
  NULL                                                  // Categories
};

#endif // _SYSVAD_A2DPHPSPEAKERTOPTABLE_H_
    

