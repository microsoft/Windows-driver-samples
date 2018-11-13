/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsspeakertoptable.h

Abstract:

    Declaration of topology tables for the USB Headset speaker (external).

--*/

#ifndef _SYSVAD_USBHSSPEAKERTOPTABLE_H_
#define _SYSVAD_USBHSSPEAKERTOPTABLE_H_

#include "usbhsspeakertopo.h"

//=============================================================================
static
KSDATARANGE UsbHsSpeakerTopoPinDataRangesBridge[] =
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
PKSDATARANGE UsbHsSpeakerTopoPinDataRangePointersBridge[] =
{
  &UsbHsSpeakerTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR UsbHsSpeakerTopoMiniportPins[] =
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
      SIZEOF_ARRAY(UsbHsSpeakerTopoPinDataRangePointersBridge),// DataRangesCount
      UsbHsSpeakerTopoPinDataRangePointersBridge,      // DataRanges
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
      SIZEOF_ARRAY(UsbHsSpeakerTopoPinDataRangePointersBridge),// DataRangesCount
      UsbHsSpeakerTopoPinDataRangePointersBridge,      // DataRanges
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
KSJACK_DESCRIPTION UsbHsSpeakerJackDesc =
{
    KSAUDIO_SPEAKER_STEREO,
    0,                      // color
    eConnTypeOtherDigital,
    eGeoLocBottom,
    eGenLocPrimaryBox,
    ePortConnJack,
    FALSE                   // run-time code sets this value.
};

// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION UsbHsSpeakerJackDescriptions[] =
{
    NULL,
    &UsbHsSpeakerJackDesc
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR UsbHsSpeakerTopoMiniportConnections[] =
{
  //  FromNode,             FromPin,                    ToNode,                 ToPin
  {   PCFILTER_NODE,        KSPIN_TOPO_WAVEOUT_SOURCE,  PCFILTER_NODE,          KSPIN_TOPO_LINEOUT_DEST }
};

//=============================================================================
static
PCPROPERTY_ITEM UsbHsSpeakerPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_UsbHsSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_UsbHsSpeakerTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_CONTAINERID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_UsbHsSpeakerTopoFilter
    }
};

static 
PCEVENT_ITEM UsbHsSpeakerPropertiesTopoFilterEvent[] =
{
  {
    &KSEVENTSETID_PinCapsChange,                        // Something changed!
    KSEVENT_PINCAPS_JACKINFOCHANGE,                     // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_UsbHsSpeakerTopoFilterEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationUsbHsSpeakerTopoFilterWithEvent, 
                               UsbHsSpeakerPropertiesTopoFilter,
                               UsbHsSpeakerPropertiesTopoFilterEvent);

//=============================================================================
static
PCFILTER_DESCRIPTOR UsbHsSpeakerTopoMiniportFilterDescriptor =
{
  0,                                                    // Version
  &AutomationUsbHsSpeakerTopoFilterWithEvent,           // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                             // PinSize
  SIZEOF_ARRAY(UsbHsSpeakerTopoMiniportPins),           // PinCount
  UsbHsSpeakerTopoMiniportPins,                         // Pins
  sizeof(PCNODE_DESCRIPTOR),                            // NodeSize
  0,                                                    // NodeCount
  NULL,                                                 // Nodes
  SIZEOF_ARRAY(UsbHsSpeakerTopoMiniportConnections),    // ConnectionCount
  UsbHsSpeakerTopoMiniportConnections,                  // Connections
  0,                                                    // CategoryCount
  NULL                                                  // Categories
};

#endif // _SYSVAD_USBHSSPEAKERTOPTABLE_H_
    

