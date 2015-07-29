/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    spdiftoptable.h

Abstract:

    Declaration of topology tables for the SPDIF endpoint.

--*/

#ifndef _SYSVAD_SPDIFTOPTABLE_H_
#define _SYSVAD_SPDIFTOPTABLE_H_

//=============================================================================
static
KSDATARANGE SpdifTopoPinDataRangesBridge[] =
{
  {
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
  },
  {
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
  }
};

//=============================================================================
static
PKSDATARANGE SpdifTopoPinDataRangePointersBridge[] =
{
  &SpdifTopoPinDataRangesBridge[0],
  &SpdifTopoPinDataRangesBridge[1]
};

//=============================================================================
static
PCPIN_DESCRIPTOR SpdifTopoMiniportPins[] =
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
      SIZEOF_ARRAY(SpdifTopoPinDataRangePointersBridge), // DataRangesCount
      SpdifTopoPinDataRangePointersBridge,               // DataRanges
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
      SIZEOF_ARRAY(SpdifTopoPinDataRangePointersBridge), // DataRangesCount
      SpdifTopoPinDataRangePointersBridge,               // DataRanges
      KSPIN_DATAFLOW_OUT,                               // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_SPDIF_INTERFACE,                      // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  }
};

//=============================================================================
static
KSJACK_DESCRIPTION SpdifJackDesc =
{
    KSAUDIO_SPEAKER_STEREO,
    0x0000,               // no color
    eConnTypeOtherDigital,
    eGeoLocRear,
    eGenLocPrimaryBox,
    ePortConnJack,
    TRUE
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION SpdifJackDescriptions[] =
{
    NULL,
    &SpdifJackDesc
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR SpdifTopoMiniportConnections[] =
{
  //  FromNode,                     FromPin,                        ToNode,                      ToPin
  {   PCFILTER_NODE,                KSPIN_TOPO_WAVEOUT_SOURCE,      PCFILTER_NODE,               KSPIN_TOPO_LINEOUT_DEST}
};


//=============================================================================
static
PCPROPERTY_ITEM PropertiesSpdifTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpdifTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_SpdifTopoFilter
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSpdifTopoFilter, PropertiesSpdifTopoFilter);

//=============================================================================
static
PCFILTER_DESCRIPTOR SpdifTopoMiniportFilterDescriptor =
{
  0,                                            // Version
  &AutomationSpdifTopoFilter,                   // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                     // PinSize
  SIZEOF_ARRAY(SpdifTopoMiniportPins),          // PinCount
  SpdifTopoMiniportPins,                        // Pins
  sizeof(PCNODE_DESCRIPTOR),                    // NodeSize
  0,                                            // NodeCount
  NULL,                                         // Nodes
  SIZEOF_ARRAY(SpdifTopoMiniportConnections),   // ConnectionCount
  SpdifTopoMiniportConnections,                 // Connections
  0,                                            // CategoryCount
  NULL                                          // Categories
};

#endif // _SYSVAD_SPDIFTOPTABLE_H_
    

