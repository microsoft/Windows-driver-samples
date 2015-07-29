/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarray1toptable.h

Abstract:

    Declaration of topology tables for the mic array (front).

--*/

#ifndef _SYSVAD_MICARRAY1TOPTABLE_H_
#define _SYSVAD_MICARRAY1TOPTABLE_H_

//
// {6ae81ff4-203e-4fe1-88aa-f2d57775cd4a}
DEFINE_GUID(MICARRAY1_CUSTOM_NAME, 
0x6ae81ff4, 0x203e, 0x4fe1, 0x88, 0xaa, 0xf2, 0xd5, 0x77, 0x75, 0xcd, 0x4a);

//=============================================================================
static
KSDATARANGE MicArray1TopoPinDataRangesBridge[] =
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
PKSDATARANGE MicArray1TopoPinDataRangePointersBridge[] =
{
  &MicArray1TopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicArray1TopoMiniportPins[] =
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
      SIZEOF_ARRAY(MicArray1TopoPinDataRangePointersBridge),     // DataRangesCount
      MicArray1TopoPinDataRangePointersBridge,                   // DataRanges
      KSPIN_DATAFLOW_IN,                            // DataFlow
      KSPIN_COMMUNICATION_NONE,                     // Communication
      &KSNODETYPE_MICROPHONE_ARRAY,                 // Category
      &MICARRAY1_CUSTOM_NAME,                       // Name
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
      SIZEOF_ARRAY(MicArray1TopoPinDataRangePointersBridge),     // DataRangesCount
      MicArray1TopoPinDataRangePointersBridge,                   // DataRanges
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
PCPROPERTY_ITEM MicArray1PropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray1Volume, MicArray1PropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM MicArray1PropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray1Mute, MicArray1PropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM MicArray1PropertiesPeakMeter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray1PeakMeter, MicArray1PropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR MicArray1TopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                              // Flags
    &AutomationMicArray1Volume,     // AutomationTable
    &KSNODETYPE_VOLUME,             // Type
    &KSAUDFNAME_MIC_VOLUME          // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                              // Flags
    &AutomationMicArray1Mute,       // AutomationTable
    &KSNODETYPE_MUTE,               // Type
    &KSAUDFNAME_MIC_MUTE            // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                              // Flags
    &AutomationMicArray1PeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,          // Type
    &KSAUDFNAME_PEAKMETER           // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicArray1TopoMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
    {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM MicArray1PropertiesTopoFilter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray1TopoFilter, MicArray1PropertiesTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicArray1TopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationMicArray1TopoFilter,               // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(MicArray1TopoMiniportPins),      // PinCount
  MicArray1TopoMiniportPins,                    // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  SIZEOF_ARRAY(MicArray1TopologyNodes),         // NodeCount
  MicArray1TopologyNodes,                       // Nodes
  SIZEOF_ARRAY(MicArray1TopoMiniportConnections),// ConnectionCount
  MicArray1TopoMiniportConnections,             // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_MICARRAY1TOPTABLE_H_

