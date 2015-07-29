/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    cellulartoptable.h

Abstract:

    Declaration of topology tables for the cellular endpoint.

--*/

#ifndef _SYSVAD_CELLULARTOPTABLE_H_
#define _SYSVAD_CELLULARTOPTABLE_H_

// {896BC350-BF57-4039-A6B6-64DCEC3AEEB8}
DEFINE_GUID(CELLULARBIDI1_CUSTOM_NAME, 
0x896bc350, 0xbf57, 0x4039, 0xa6, 0xb6, 0x64, 0xdc, 0xec, 0x3a, 0xee, 0xb8);

// {80CC64DA-166A-4A2C-ADF7-1464C49D3B0E}
DEFINE_GUID(CELLULARBIDI2_CUSTOM_NAME, 
0x80cc64da, 0x166a, 0x4a2c, 0xad, 0xf7, 0x14, 0x64, 0xc4, 0x9d, 0x3b, 0xe);

//=============================================================================
static
KSDATARANGE CellularTopoPinDataRangesBridge[] =
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
PKSDATARANGE CellularTopoPinDataRangePointersBridge[] =
{
  &CellularTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR CellularTopoMiniportPins[] =
{
  // KSPIN_TOPO_BIDI1
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
      SIZEOF_ARRAY(CellularTopoPinDataRangePointersBridge), // DataRangesCount
      CellularTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_TELEPHONY_BIDI,                       // Category
      &CELLULARBIDI1_CUSTOM_NAME,                       // Name
      0                                                 // Reserved
    }
  },
  // KSPIN_TOPO_BIDI1_BRIDGE
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
      SIZEOF_ARRAY(CellularTopoPinDataRangePointersBridge), // DataRangesCount
      CellularTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_OUT,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },
  // KSPIN_TOPO_BIDI2
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
      SIZEOF_ARRAY(CellularTopoPinDataRangePointersBridge), // DataRangesCount
      CellularTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_TELEPHONY_BIDI,                       // Category
      &CELLULARBIDI2_CUSTOM_NAME,                       // Name
      0                                                 // Reserved
    }
  },
  // KSPIN_TOPO_BIDI2_BRIDGE
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
      SIZEOF_ARRAY(CellularTopoPinDataRangePointersBridge), // DataRangesCount
      CellularTopoPinDataRangePointersBridge,               // DataRanges
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
KSJACK_DESCRIPTION CellularJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    0x0000,               // no color
    eConnTypeUnknown,
    eGeoLocNotApplicable,
    eGenLocInternal,
    ePortConnUnknown,
    TRUE                // NOTE: cellular jacks should be "unplugged" at boot, and only changed to active during a call.
                        // However, for convienience during development, the jack state is set to active at boot for this milestone.
};

static
KSJACK_DESCRIPTION CellularJackDesc2 =
{
    KSAUDIO_SPEAKER_MONO,
    0x0000,               // no color
    eConnTypeUnknown,
    eGeoLocNotApplicable,
    eGenLocInternal,
    ePortConnUnknown,
    TRUE                // NOTE: cellular jacks should be "unplugged" at boot, and only changed to active during a call.
                        // However, for convienience during development, the jack state is set to active at boot for this milestone.
};


// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION CellularJackDescriptions[] =
{
    &CellularJackDesc,
    NULL,
    &CellularJackDesc2,
    NULL
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR CellularTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_BIDI1,                PCFILTER_NODE,               KSPIN_TOPO_BIDI1_BRIDGE},
  {   PCFILTER_NODE,                KSPIN_TOPO_BIDI2,                PCFILTER_NODE,               KSPIN_TOPO_BIDI2_BRIDGE}
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesCellularTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularTopoFilter
    },
    {
        &KSPROPSETID_TelephonyTopology,
        KSPROPERTY_TELEPHONY_ENDPOINTIDPAIR,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularTopoFilter
    },
    {
        &KSPROPSETID_TelephonyTopology,
        KSPROPERTY_TELEPHONY_VOLUME,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CellularTopoFilter
    }
};

NTSTATUS CCellularMiniportWaveRT_EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
);

static PCEVENT_ITEM JackInfoChangeEvent[] =
{
  {
    &KSEVENTSETID_PinCapsChange,   // Something changed
    KSEVENT_PINCAPS_JACKINFOCHANGE,  // Jack Info Changes
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    CCellularMiniportWaveRT_EventHandler_JackState
  }
};

/*****************************************************************************
 * PropertiesTopologyFilter
 *****************************************************************************
 * Automation table for jack descripton/detection.
 */
DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationCellularTopoFilter, PropertiesCellularTopoFilter, JackInfoChangeEvent);


//=============================================================================
static
PCFILTER_DESCRIPTOR CellularTopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationCellularTopoFilter,                    // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(CellularTopoMiniportPins),           // PinCount
  CellularTopoMiniportPins,                         // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  0,                                            // NodeCount
  NULL,                                         // Nodes
  SIZEOF_ARRAY(CellularTopoMiniportConnections),    // ConnectionCount
  CellularTopoMiniportConnections,                  // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_CELLULARTOPTABLE_H_
    

