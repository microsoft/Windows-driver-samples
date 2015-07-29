/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    bthhfpspeakertoptable.h

Abstract:

    Declaration of topology tables for the Bluetooth Hands-Free Profile speaker (external).

--*/

#ifndef _SYSVAD_BTHHFPSPEAKERTOPTABLE_H_
#define _SYSVAD_BTHHFPSPEAKERTOPTABLE_H_

#include "bthhfpspeakertopo.h"

//=============================================================================
static
KSDATARANGE BthHfpSpeakerTopoPinDataRangesBridge[] =
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
PKSDATARANGE BthHfpSpeakerTopoPinDataRangePointersBridge[] =
{
  &BthHfpSpeakerTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR BthHfpSpeakerTopoMiniportPins[] =
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
      SIZEOF_ARRAY(BthHfpSpeakerTopoPinDataRangePointersBridge),// DataRangesCount
      BthHfpSpeakerTopoPinDataRangePointersBridge,      // DataRanges
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
      SIZEOF_ARRAY(BthHfpSpeakerTopoPinDataRangePointersBridge),// DataRangesCount
      BthHfpSpeakerTopoPinDataRangePointersBridge,      // DataRanges
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
KSJACK_DESCRIPTION BthHfpSpeakerJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    0,                      // color
    eConnTypeOtherDigital,
    eGeoLocNotApplicable,
    eGenLocOther,
    ePortConnUnknown,
    FALSE                   // run-time code sets this value.
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION BthHfpSpeakerJackDescriptions[] =
{
    NULL,
    &BthHfpSpeakerJackDesc
};

//=============================================================================
static
PCPROPERTY_ITEM BthHfpSpeakerPropertiesVolume[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_BthHfpSpeakerTopoNode
  }
};

static 
PCEVENT_ITEM BthHfpSpeakerPropertiesVolumeEvent[] =
{
  {
    &KSEVENTSETID_AudioControlChange,               // Something changed!
    KSEVENT_CONTROL_CHANGE,                         // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_BthHfpSpeakerTopoNodeEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationBthHfpSpeakerVolumeWithEvent, 
                                     BthHfpSpeakerPropertiesVolume, 
                                     BthHfpSpeakerPropertiesVolumeEvent);

//=============================================================================
static
PCPROPERTY_ITEM BthHfpSpeakerPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationBthHfpSpeakerMute, BthHfpSpeakerPropertiesMute);

//=============================================================================
// The volume node should be implemented as follows:
//
// #1 If the Bluetooth headset supports volume control, then the audio driver should 
//  include a volume node in its KS topology. The audio driver's volume property 
//  handlers send the above IOCLTs to the Bluetooth HFP driver to handle the volume.
//
// #2 If the Bluetooth headset does not implement a hardware volume, and the codec 
//  (or DSP) has a hardware volume, the audio driver should handle the volume control
//  on the codec (or DSP).
//
// #3 If the Bluetooth headset and the audio device do not have hardware volume controls,
//  no volume node should be presented and Windows will insert a software volume 
//  control node.
// 
// This sample assumes #1 or #2, and it adds a valume node in its topology.
//
// The mute node is opitional. The driver shall implement the mute node if and only if 
// the DSP or audio codec provides the capability to mute the bypass PCM signal before 
// passing it to the Bluetooth controller. The mute nodes support 
// KSPROPSETID_Audio/KSPROPERTY_AUDIO_MUTE
//
static
PCNODE_DESCRIPTOR BthHfpSpeakerTopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                                          // Flags
    &AutomationBthHfpSpeakerVolumeWithEvent,    // AutomationTable
    &KSNODETYPE_VOLUME,                         // Type
    &KSAUDFNAME_LINE_VOLUME                     // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                                          // Flags
    &AutomationBthHfpSpeakerMute,               // AutomationTable
    &KSNODETYPE_MUTE,                           // Type
    &KSAUDFNAME_LINE_MUTE                       // Name
  },
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR BthHfpSpeakerTopoMiniportConnections[] =
{
  //  FromNode,             FromPin,                    ToNode,                 ToPin
  {   PCFILTER_NODE,        KSPIN_TOPO_WAVEOUT_SOURCE,  KSNODE_TOPO_VOLUME,     1 },
  {   KSNODE_TOPO_VOLUME,   0,                          KSNODE_TOPO_MUTE,       1 },
  {   KSNODE_TOPO_MUTE,     0,                          PCFILTER_NODE,          KSPIN_TOPO_LINEOUT_DEST }
};

//=============================================================================
static
PCPROPERTY_ITEM BthHfpSpeakerPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_CONTAINERID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_BtAudio,
        KSPROPERTY_ONESHOT_RECONNECT,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpSpeakerTopoFilter
    },
    {
        &KSPROPSETID_BtAudio,
        KSPROPERTY_ONESHOT_DISCONNECT,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_BthHfpSpeakerTopoFilter
    }
};

static 
PCEVENT_ITEM BthHfpSpeakerPropertiesTopoFilterEvent[] =
{
  {
    &KSEVENTSETID_PinCapsChange,                        // Something changed!
    KSEVENT_PINCAPS_JACKINFOCHANGE,                     // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_BthHfpSpeakerTopoFilterEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationBthHfpSpeakerTopoFilterWithEvent, 
                               BthHfpSpeakerPropertiesTopoFilter,
                               BthHfpSpeakerPropertiesTopoFilterEvent);

//=============================================================================
static
PCFILTER_DESCRIPTOR BthHfpSpeakerTopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationBthHfpSpeakerTopoFilterWithEvent,  // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(BthHfpSpeakerTopoMiniportPins),  // PinCount
  BthHfpSpeakerTopoMiniportPins,                // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  SIZEOF_ARRAY(BthHfpSpeakerTopologyNodes),     // NodeCount
  BthHfpSpeakerTopologyNodes,                   // Nodes
  SIZEOF_ARRAY(BthHfpSpeakerTopoMiniportConnections), // ConnectionCount
  BthHfpSpeakerTopoMiniportConnections,         // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_BTHHFPSPEAKERTOPTABLE_H_
    

