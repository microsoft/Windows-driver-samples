/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    usbhsmictopotable.h

Abstract:

    Declaration of topology table for the USB Headset mic (external)

--*/

#ifndef _SYSVAD_USBHSMICTOPTABLE_H_
#define _SYSVAD_USBHSMICTOPTABLE_H_

#include "usbhsmictopo.h"

// Function declarations.
NTSTATUS
PropertyHandler_UsbHsMicTopoFilter( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
    );

//=============================================================================
static
KSDATARANGE UsbHsMicTopoPinDataRangesBridge[] =
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
PKSDATARANGE UsbHsMicTopoPinDataRangePointersBridge[] =
{
  &UsbHsMicTopoPinDataRangesBridge[0]
};

//=============================================================================
static
PCPIN_DESCRIPTOR UsbHsMicTopoMiniportPins[] =
{
  // KSPIN_TOPO_MIC_ELEMENTS
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
      SIZEOF_ARRAY(UsbHsMicTopoPinDataRangePointersBridge),// DataRangesCount
      UsbHsMicTopoPinDataRangePointersBridge,              // DataRanges
      KSPIN_DATAFLOW_IN,                                // DataFlow
      KSPIN_COMMUNICATION_NONE,                         // Communication
      &KSNODETYPE_HEADSET_MICROPHONE,                   // Category
      NULL,                                             // Name
      0                                                 // Reserved
    }
  },

  // KSPIN_TOPO_BRIDGE
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
      SIZEOF_ARRAY(UsbHsMicTopoPinDataRangePointersBridge),// DataRangesCount
      UsbHsMicTopoPinDataRangePointersBridge,              // DataRanges
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
KSJACK_DESCRIPTION UsbHsMicJackDesc =
{
    KSAUDIO_SPEAKER_MONO,
    JACKDESC_RGB(0, 0, 0),
    eConnTypeOtherDigital,
    eGeoLocBottom,
    eGenLocPrimaryBox,
    ePortConnJack,
    FALSE               // NOTE: For convenience, wired headset jacks will be "unplugged" at boot.
};

//=============================================================================
// Only return a KSJACK_DESCRIPTION for the physical bridge pin.
static 
PKSJACK_DESCRIPTION UsbHsMicJackDescriptions[] =
{
    &UsbHsMicJackDesc,
    NULL
};

//=============================================================================
static
PCPROPERTY_ITEM UsbHsMicPropertiesVolume[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_UsbHsMicTopoNode
  }
};

static
PCEVENT_ITEM UsbHsMicPropertiesVolumeEvent[] =
{
  // This is a generic event for nearly every node property.
  {
    &KSEVENTSETID_AudioControlChange,               // Something changed!
    KSEVENT_CONTROL_CHANGE,                         // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_UsbHsMicTopoNodeEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationUsbHsMicVolumeWithEvent, 
                                     UsbHsMicPropertiesVolume, 
                                     UsbHsMicPropertiesVolumeEvent);


//=============================================================================
static
PCPROPERTY_ITEM UsbHsMicPropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_UsbHsMicTopoNode
  }
};

static
PCEVENT_ITEM UsbHsMicPropertiesMuteEvent[] =
{
  // This is a generic event for nearly every node property.
  {
    &KSEVENTSETID_AudioControlChange,               // Something changed!
    KSEVENT_CONTROL_CHANGE,                         // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_UsbHsMicTopoNodeEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationUsbHsMicMuteWithEvent, 
                                     UsbHsMicPropertiesMute, 
                                     UsbHsMicPropertiesMuteEvent);

//=============================================================================
static
PCPROPERTY_ITEM UsbHsMicPropertiesPeakMeter[] =
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

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationUsbHsMicPeakMeter, UsbHsMicPropertiesPeakMeter);

//=============================================================================
static
PCNODE_DESCRIPTOR UsbHsMicTopologyNodes[] =
{
  // KSNODE_TOPO_VOLUME
  {
    0,                                      // Flags
    &AutomationUsbHsMicVolumeWithEvent,     // AutomationTable
    &KSNODETYPE_VOLUME,                     // Type
    &KSAUDFNAME_MIC_VOLUME                  // Name
  },
  // KSNODE_TOPO_MUTE
  {
    0,                                      // Flags
    &AutomationUsbHsMicMuteWithEvent,       // AutomationTable
    &KSNODETYPE_MUTE,                       // Type
    &KSAUDFNAME_MIC_MUTE                    // Name
  },
  // KSNODE_TOPO_PEAKMETER
  {
    0,                          // Flags
    &AutomationUsbHsMicPeakMeter,  // AutomationTable
    &KSNODETYPE_PEAKMETER,      // Type
    &KSAUDFNAME_PEAKMETER       // Name
  }
};

C_ASSERT( KSNODE_TOPO_VOLUME  == 0 );
C_ASSERT( KSNODE_TOPO_MUTE    == 1 );
C_ASSERT( KSNODE_TOPO_PEAKMETER == 2 );

//=============================================================================
static
PCCONNECTION_DESCRIPTOR UsbHsMicMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          KSNODE_TOPO_PEAKMETER,  1 },
    {   KSNODE_TOPO_PEAKMETER,    0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};


//=============================================================================
static
PCPROPERTY_ITEM UsbHsMicPropertiesTopoFilter[] =
{
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_UsbHsMicTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_DESCRIPTION2,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_UsbHsMicTopoFilter
    },
    {
        &KSPROPSETID_Jack,
        KSPROPERTY_JACK_CONTAINERID,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_UsbHsMicTopoFilter
    }

};

static 
PCEVENT_ITEM UsbHsMicTopoPropertiesFilterEvent[] =
{
  {
    &KSEVENTSETID_PinCapsChange,                        // Something changed!
    KSEVENT_PINCAPS_JACKINFOCHANGE,                     // The only event-property defined.
    KSEVENT_TYPE_ENABLE | KSEVENT_TYPE_BASICSUPPORT,
    PropertyHandler_UsbHsMicTopoFilterEvent
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationUsbHsMicTopoFilterWithEvent, 
                               UsbHsMicPropertiesTopoFilter,
                               UsbHsMicTopoPropertiesFilterEvent);


//=============================================================================
static
PCFILTER_DESCRIPTOR UsbHsMicTopoMiniportFilterDescriptor =
{
  0,                                        // Version
  &AutomationUsbHsMicTopoFilterWithEvent,  // AutomationTable
  sizeof(PCPIN_DESCRIPTOR),                 // PinSize
  SIZEOF_ARRAY(UsbHsMicTopoMiniportPins),  // PinCount
  UsbHsMicTopoMiniportPins,                // Pins
  sizeof(PCNODE_DESCRIPTOR),                // NodeSize
  SIZEOF_ARRAY(UsbHsMicTopologyNodes),     // NodeCount
  UsbHsMicTopologyNodes,                   // Nodes
  SIZEOF_ARRAY(UsbHsMicMiniportConnections),// ConnectionCount
  UsbHsMicMiniportConnections,             // Connections
  0,                                        // CategoryCount
  NULL                                      // Categories
};

#endif // _SYSVAD_USBHSMICTOPTABLE_H_

