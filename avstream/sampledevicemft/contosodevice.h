// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//  The Contoso device comes with an SDK
//  that defines various Contoso-specific types,
//  all of which have CONTOSO in their names.

#pragma once

//  This payload is used when sending a KSPROPERTY_CONTOSODEVICE_PROCESSBUFFER.
struct CONTOSODEVICE_PROCESSBUFFER_PAYLOAD
{
    GUID        identifier;
    DWORD       size;
};

//  Define a custom property set for driver communications.
//  {C5175EE2-583E-43A2-8C26-765287BCEB9D}
#define STATIC_PROPSETID_CONTOSODEVICE\
    0xc5175ee2, 0x583e, 0x43a2, 0x8c, 0x26, 0x76, 0x52, 0x87, 0xbc, 0xeb, 0x9d
DEFINE_GUIDSTRUCT("C5175EE2-583E-43A2-8C26-765287BCEB9D", PROPSETID_CONTOSODEVICE);
#define PROPSETID_CONTOSODEVICE DEFINE_GUIDNAMED(PROPSETID_CONTOSODEVICE)
#define KSPROPERTY_CONTOSODEVICE_PROCESSBUFFER 0       // Send a frame buffer ID to the driver.

