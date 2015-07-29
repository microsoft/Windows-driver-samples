/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    fmtoptable.h

Abstract:

    Declaration of topology tables for the fm endpoint.

--*/

#ifndef _SYSVAD_FMTOPTABLE_H_
#define _SYSVAD_FMTOPTABLE_H_

// {344EC7D7-F209-4FC4-B6C1-B324E8309E8D}
DEFINE_GUID(FMRX_CUSTOM_NAME,
    0x344ec7d7, 0xf209, 0x4fc4, 0xb6, 0xc1, 0xb3, 0x24, 0xe8, 0x30, 0x9e, 0x8d);

//=============================================================================
static
KSDATARANGE FmTopoPinDataRangesBridge[] =
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
PKSDATARANGE FmTopoPinDataRangePointersBridge[] =
{
  &FmTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR FmTopoMiniportPins[] =
{
  // KSPIN_TOPO_FMRX
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
      SIZEOF_ARRAY(FmTopoPinDataRangePointersBridge),   // DataRangesCount
      FmTopoPinDataRangePointersBridge,                 // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_FM_RX,                                // Category
      &FMRX_CUSTOM_NAME,                                // Name
      0                                                 // Reserved
    }
  },
  // KSPIN_TOPO_FMRX_BRIDGE
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
      SIZEOF_ARRAY(FmTopoPinDataRangePointersBridge), // DataRangesCount
      FmTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSCATEGORY_AUDIO,                                // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },
};

//=============================================================================
static
KSJACK_DESCRIPTION FmRxJackDesc =
{
    KSAUDIO_SPEAKER_STEREO,
    0x0000,               // no color
    eConnTypeOtherAnalog,
    eGeoLocNotApplicable,
    eGenLocInternal,
    ePortConnUnknown,
    FALSE
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION FmJackDescriptions[] =
{
    &FmRxJackDesc,
    NULL,
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR FmTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_FMRX,                  PCFILTER_NODE,               KSPIN_TOPO_FMRX_BRIDGE},
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesFmTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_FmRxTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_FmRxTopoFilter
    },
    {
        &KSPROPSETID_FMRXTopology,
        KSPROPERTY_FMRX_ENDPOINTID,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_FmRxTopoFilter
    },
    {
        &KSPROPSETID_FMRXTopology,
        KSPROPERTY_FMRX_VOLUME,
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_FmRxTopoFilter
    },
    {
        &KSPROPSETID_FMRXTopology,
        KSPROPERTY_FMRX_ANTENNAENDPOINTID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_FmRxTopoFilter
    },
};

NTSTATUS CFmMiniportWaveRT_EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
);

static PCEVENT_ITEM FmRxJackInfoChangeEvent[] =
{
  {
    &KSEVENTSETID_PinCapsChange,   // Something changed
    KSEVENT_PINCAPS_JACKINFOCHANGE,  // Jack Info Changes
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    CFmMiniportWaveRT_EventHandler_JackState
  }
};

/*****************************************************************************
 * PropertiesTopologyFilter
 *****************************************************************************
 * Automation table for jack descripton/detection.
 */
DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationFmTopoFilter, PropertiesFmTopoFilter, FmRxJackInfoChangeEvent);


//=============================================================================
static
PCFILTER_DESCRIPTOR FmTopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationFmTopoFilter,                    // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(FmTopoMiniportPins),           // PinCount
  FmTopoMiniportPins,                         // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  0,                                            // NodeCount
  NULL,                                         // Nodes
  SIZEOF_ARRAY(FmTopoMiniportConnections),    // ConnectionCount
  FmTopoMiniportConnections,                  // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_FMTOPTABLE_H_
    

