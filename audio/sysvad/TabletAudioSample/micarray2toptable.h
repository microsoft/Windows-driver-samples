/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarray2toptable.h

Abstract:

    Declaration of topology tables for the mic array (back).

--*/

#ifndef _SYSVAD_MICARRAY2TOPTABLE_H_
#define _SYSVAD_MICARRAY2TOPTABLE_H_

//
// {3fe0e3e1-ad16-4772-8382-4129169018ce}
DEFINE_GUID(MICARRAY2_CUSTOM_NAME, 
0x3fe0e3e1, 0xad16, 0x4772, 0x83, 0x82, 0x41, 0x29, 0x16, 0x90, 0x18, 0xce);

//=============================================================================
static
KSDATARANGE MicArray2TopoPinDataRangesBridge[] =
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
PKSDATARANGE MicArray2TopoPinDataRangePointersBridge[] =
{
  &MicArray2TopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR MicArray2TopoMiniportPins[] =
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
      SIZEOF_ARRAY(MicArray2TopoPinDataRangePointersBridge),     // DataRangesCount
      MicArray2TopoPinDataRangePointersBridge,                   // DataRanges
      KSPIN_DATAFLOW_IN,                            // DataFlow
      KSPIN_COMMUNICATION_NONE,                     // Communication
      &KSNODETYPE_MICROPHONE_ARRAY,                 // Category
      &MICARRAY2_CUSTOM_NAME,                       // Name
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
      SIZEOF_ARRAY(MicArray2TopoPinDataRangePointersBridge),     // DataRangesCount
      MicArray2TopoPinDataRangePointersBridge,                   // DataRanges
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
PCPROPERTY_ITEM MicArray2PropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray2Volume, MicArray2PropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM MicArray2PropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray2Mute, MicArray2PropertiesMute);

//=============================================================================
static
PCPROPERTY_ITEM MicArray2PropertiesPeakMeter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray2PeakMeter, MicArray2PropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR MicArray2TopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                              // Flags
    &AutomationMicArray2Volume,     // AutomationTable
    &KSNODETYPE_VOLUME,             // Type
    &KSAUDFNAME_MIC_VOLUME          // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                              // Flags
    &AutomationMicArray2Mute,       // AutomationTable
    &KSNODETYPE_MUTE,               // Type
    &KSAUDFNAME_MIC_MUTE            // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                              // Flags
    &AutomationMicArray2PeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,          // Type
    &KSAUDFNAME_PEAKMETER           // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );


//=============================================================================
static
PCCONNECTION_DESCRIPTOR MicArray2TopoMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
    {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM MicArray2PropertiesTopoFilter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray2TopoFilter, MicArray2PropertiesTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR MicArray2TopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationMicArray2TopoFilter,               // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(MicArray2TopoMiniportPins),      // PinCount
  MicArray2TopoMiniportPins,                    // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  SIZEOF_ARRAY(MicArray2TopologyNodes),         // NodeCount
  MicArray2TopologyNodes,                       // Nodes
  SIZEOF_ARRAY(MicArray2TopoMiniportConnections),// ConnectionCount
  MicArray2TopoMiniportConnections,             // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_MICARRAY2TOPTABLE_H_

