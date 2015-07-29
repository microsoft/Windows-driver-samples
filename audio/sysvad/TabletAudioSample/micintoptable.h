/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micintopotable.h

Abstract:

    Declaration of topology table for the mic (external: headphone)

--*/

#ifndef _SYSVAD_MICINTOPTABLE_H_
#define _SYSVAD_MICINTOPTABLE_H_

// Function declarations.
NTSTATUS
PropertyHandler_MicInTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

//
// {d48deb08-fd1c-4d1e-b821-9064d49ae96e}
DEFINE_GUID(MICIN_CUSTOM_NAME, 
0xd48deb08, 0xfd1c, 0x4d1e, 0xb8, 0x21, 0x90, 0x64, 0xd4, 0x9a, 0xe9, 0x6e);

//=============================================================================
static
KSJACK_DESCRIPTION MicInJackDesc =
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
KSDATARANGE MicInTopoPinDataRangesBridge[] =
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
PKSDATARANGE MicInTopoPinDataRangePointersBridge[] =
{
  &MicInTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicInTopoMiniportPins[] =
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
      SIZEOF_ARRAY(MicInTopoPinDataRangePointersBridge),// DataRangesCount
      MicInTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_MICROPHONE,                           // Category
      &MICIN_CUSTOM_NAME,                               // Name
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
      SIZEOF_ARRAY(MicInTopoPinDataRangePointersBridge),// DataRangesCount
      MicInTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION MicInJackDescriptions[] =
{
    &MicInJackDesc,
    NULL
};

//=============================================================================
static
PCPROPERTY_ITEM MicInPropertiesVolume[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInVolume, MicInPropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM MicInPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInMute, MicInPropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM MicInPropertiesPeakMeter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInPeakMeter, MicInPropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR MicInTopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                          // Flags
    &AutomationMicInVolume,     // AutomationTable
    &KSNODETYPE_VOLUME,         // Type
    &KSAUDFNAME_MIC_VOLUME      // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                          // Flags
    &AutomationMicInMute,       // AutomationTable
    &KSNODETYPE_MUTE,           // Type
    &KSAUDFNAME_MIC_MUTE        // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                          // Flags
    &AutomationMicInPeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,      // Type
    &KSAUDFNAME_PEAKMETER       // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicInMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
    {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM MicInPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicInTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicInTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicInTopoFilter, MicInPropertiesTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicInTopoMiniportFilterDescriptor =
{
  0,                                        // Version
  &AutomationMicInTopoFilter,               // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                 // PinSize
  SIZEOF_ARRAY(MicInTopoMiniportPins),      // PinCount
  MicInTopoMiniportPins,                    // Pins
  sizeof(PCNODE_DESCRIPTOR),                // NodeSize
  SIZEOF_ARRAY(MicInTopologyNodes),         // NodeCount
  MicInTopologyNodes,                       // Nodes
  SIZEOF_ARRAY(MicInMiniportConnections),   // ConnectionCount
  MicInMiniportConnections,                 // Connections
  0,                                        // CategoryCount
  NULL                                      // Categories
};

#endif // _SYSVAD_MICINTOPTABLE_H_

