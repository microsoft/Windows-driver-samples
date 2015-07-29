/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarray3toptable.h

Abstract:

    Declaration of topology tables for the mic array (front/back).

--*/

#ifndef _SYSVAD_MICARRAY3TOPTABLE_H_
#define _SYSVAD_MICARRAY3TOPTABLE_H_

//
// {c04bdb7c-2138-48da-9dd4-2af9ff2e58c2}
DEFINE_GUID(MICARRAY3_CUSTOM_NAME, 
0xc04bdb7c, 0x2138, 0x48da, 0x9d, 0xd4, 0x2a, 0xf9, 0xff, 0x2e, 0x58, 0xc2);

//=============================================================================
static
KSDATARANGE MicArray3TopoPinDataRangesBridge[] =
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
PKSDATARANGE MicArray3TopoPinDataRangePointersBridge[] =
{
  &MicArray3TopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicArray3TopoMiniportPins[] =
{
  // KSPIN_TOPO_MIC_ELEMENTS
  {
    0,
    0,
    0,                                              // InstanceCount
    NULL,                                           // AutomationTable
    {                                               // KsPinDescriptor
      0,                                            // InterfacesCount
      NULL,                                         // Interfaces
      0,                                            // MediumsCount
      NULL,                                         // Mediums
      SIZEOF_ARRAY(MicArray3TopoPinDataRangePointersBridge),     // DataRangesCount
      MicArray3TopoPinDataRangePointersBridge,                   // DataRanges
      KSPIN_DATAFLOW_IN,                            // DataFlow
      KSPIN_COMMUNICATION_NONE,                     // Communication
      &KSNODETYPE_MICROPHONE_ARRAY,                 // Category
      &MICARRAY3_CUSTOM_NAME,                       // Name
      0                                             // Reserved
    }
  },

  // KSPIN_TOPO_BRIDGE
  {
    0,
    0,
    0,                                              // InstanceCount
    NULL,                                           // AutomationTable
    {                                               // KsPinDescriptor
      0,                                            // InterfacesCount
      NULL,                                         // Interfaces
      0,                                            // MediumsCount
      NULL,                                         // Mediums
      SIZEOF_ARRAY(MicArray3TopoPinDataRangePointersBridge),     // DataRangesCount
      MicArray3TopoPinDataRangePointersBridge,                   // DataRanges
      KSPIN_DATAFLOW_OUT,                           // DataFlow
      KSPIN_COMMUNICATION_NONE,                     // Communication
      &KSCATEGORY_AUDIO,                            // Category
      NULL,                                         // Name
      0                                             // Reserved
    }
  }
};

//=============================================================================
static
PCPROPERTY_ITEM MicArray3PropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray3Volume, MicArray3PropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM MicArray3PropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray3Mute, MicArray3PropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM MicArray3PropertiesPeakMeter[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_PEAKMETER2,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
  },
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_CPU_RESOURCES,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray3PeakMeter, MicArray3PropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR MicArray3TopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                              // Flags
    &AutomationMicArray3Volume,     // AutomationTable
    &KSNODETYPE_VOLUME,             // Type
    &KSAUDFNAME_MIC_VOLUME          // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                              // Flags
    &AutomationMicArray3Mute,       // AutomationTable
    &KSNODETYPE_MUTE,               // Type
    &KSAUDFNAME_MIC_MUTE            // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                              // Flags
    &AutomationMicArray3PeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,          // Type
    &KSAUDFNAME_PEAKMETER           // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicArray3TopoMiniportConnections[] =
{
  //  FromNode,                 FromPin,                    ToNode,                 ToPin
  {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
  {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
  {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
  {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM MicArray3PropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicArrayTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicArrayTopoFilter
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MIC_ARRAY_GEOMETRY,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_MicArrayTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray3TopoFilter, MicArray3PropertiesTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicArray3TopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationMicArray3TopoFilter,               // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(MicArray3TopoMiniportPins),      // PinCount
  MicArray3TopoMiniportPins,                    // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  SIZEOF_ARRAY(MicArray3TopologyNodes),         // NodeCount
  MicArray3TopologyNodes,                       // Nodes
  SIZEOF_ARRAY(MicArray3TopoMiniportConnections),// ConnectionCount
  MicArray3TopoMiniportConnections,             // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_MICARRAY3TOPTABLE_H_

