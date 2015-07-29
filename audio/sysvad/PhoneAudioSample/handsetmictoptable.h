/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    handsetmictopotable.h

Abstract:

    Declaration of topology table for the handset mic (internal)

--*/

#ifndef _SYSVAD_HANDSETMICTOPTABLE_H_
#define _SYSVAD_HANDSETMICTOPTABLE_H_

// Function declarations.
NTSTATUS
PropertyHandler_HandsetMicTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

//=============================================================================
static
KSDATARANGE HandsetMicTopoPinDataRangesBridge[] =
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
PKSDATARANGE HandsetMicTopoPinDataRangePointersBridge[] =
{
  &HandsetMicTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR HandsetMicTopoMiniportPins[] =
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
      SIZEOF_ARRAY(HandsetMicTopoPinDataRangePointersBridge),// DataRangesCount
      HandsetMicTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HANDSET,                              // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },

  // KSPIN - topology filter out-in
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
      SIZEOF_ARRAY(HandsetMicTopoPinDataRangePointersBridge),// DataRangesCount
      HandsetMicTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
static
KSJACK_DESCRIPTION HandsetMicJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    0x0000,               // no color
    eConnTypeUnknown,
    eGeoLocTop,
    eGenLocInternal,
    ePortConnIntegratedDevice,
    TRUE
};

//=============================================================================
// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION HandsetMicJackDescriptions[] =
{
    &HandsetMicJackDesc,
    NULL
};

//=============================================================================
static
PCPROPERTY_ITEM HandsetMicPropertiesVolume[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetMicVolume, HandsetMicPropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM HandsetMicPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetMicMute, HandsetMicPropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM HandsetMicPropertiesPeakMeter[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_PEAKMETER2,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  },
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_CPU_RESOURCES,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetMicPeakMeter, HandsetMicPropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR HandsetMicTopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                          // Flags
    &AutomationHandsetMicVolume,     // AutomationTable
    &KSNODETYPE_VOLUME,         // Type
    &KSAUDFNAME_MIC_VOLUME      // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                          // Flags
    &AutomationHandsetMicMute,       // AutomationTable
    &KSNODETYPE_MUTE,           // Type
    &KSAUDFNAME_MIC_MUTE        // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                          // Flags
    &AutomationHandsetMicPeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,      // Type
    &KSAUDFNAME_PEAKMETER       // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR HandsetMicMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
    {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM HandsetMicPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_HandsetMicTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_HandsetMicTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationHandsetMicTopoFilter, HandsetMicPropertiesTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR HandsetMicTopoMiniportFilterDescriptor =
{
  0,                                        // Version
  &AutomationHandsetMicTopoFilter,               // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                 // PinSize
  SIZEOF_ARRAY(HandsetMicTopoMiniportPins),      // PinCount
  HandsetMicTopoMiniportPins,                    // Pins
  sizeof(PCNODE_DESCRIPTOR),                // NodeSize
  SIZEOF_ARRAY(HandsetMicTopologyNodes),         // NodeCount
  HandsetMicTopologyNodes,                       // Nodes
  SIZEOF_ARRAY(HandsetMicMiniportConnections),   // ConnectionCount
  HandsetMicMiniportConnections,                 // Connections
  0,                                        // CategoryCount
  NULL                                      // Categories
};

#endif // _SYSVAD_HANDSETMICTOPTABLE_H_

