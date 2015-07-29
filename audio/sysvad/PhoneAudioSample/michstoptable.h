/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    michstopotable.h

Abstract:

    Declaration of topology table for the mic (external: headset)

--*/

#ifndef _SYSVAD_MICHSTOPTABLE_H_
#define _SYSVAD_MICHSTOPTABLE_H_

// Function declarations.
NTSTATUS
PropertyHandler_MicHsTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

//=============================================================================
static
KSDATARANGE MicHsTopoPinDataRangesBridge[] =
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
PKSDATARANGE MicHsTopoPinDataRangePointersBridge[] =
{
  &MicHsTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicHsTopoMiniportPins[] =
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
      SIZEOF_ARRAY(MicHsTopoPinDataRangePointersBridge),// DataRangesCount
      MicHsTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HEADSET_MICROPHONE,                   // Category
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
      SIZEOF_ARRAY(MicHsTopoPinDataRangePointersBridge),// DataRangesCount
      MicHsTopoPinDataRangePointersBridge,              // DataRanges
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
KSJACK_DESCRIPTION MicHsJackDesc =
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
PKSJACK_DESCRIPTION MicHsJackDescriptions[] =
{
    &MicHsJackDesc,
    NULL
};

//=============================================================================
static
PCPROPERTY_ITEM MicHsPropertiesVolume[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicHsVolume, MicHsPropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM MicHsPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_Topology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicHsMute, MicHsPropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM MicHsPropertiesPeakMeter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicHsPeakMeter, MicHsPropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR MicHsTopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                          // Flags
    &AutomationMicHsVolume,     // AutomationTable
    &KSNODETYPE_VOLUME,         // Type
    &KSAUDFNAME_MIC_VOLUME      // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                          // Flags
    &AutomationMicHsMute,       // AutomationTable
    &KSNODETYPE_MUTE,           // Type
    &KSAUDFNAME_MIC_MUTE        // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                          // Flags
    &AutomationMicHsPeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,      // Type
    &KSAUDFNAME_PEAKMETER       // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicHsMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
    {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM MicHsPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicHsTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicHsTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicHsTopoFilter, MicHsPropertiesTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicHsTopoMiniportFilterDescriptor =
{
  0,                                        // Version
  &AutomationMicHsTopoFilter,               // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                 // PinSize
  SIZEOF_ARRAY(MicHsTopoMiniportPins),      // PinCount
  MicHsTopoMiniportPins,                    // Pins
  sizeof(PCNODE_DESCRIPTOR),                // NodeSize
  SIZEOF_ARRAY(MicHsTopologyNodes),         // NodeCount
  MicHsTopologyNodes,                       // Nodes
  SIZEOF_ARRAY(MicHsMiniportConnections),   // ConnectionCount
  MicHsMiniportConnections,                 // Connections
  0,                                        // CategoryCount
  NULL                                      // Categories
};

#endif // _SYSVAD_MICHSTOPTABLE_H_

