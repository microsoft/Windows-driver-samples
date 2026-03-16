// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
#pragma once

// header files for imported files
#include "propidl.h"

#ifdef DEFINE_PROPERTYKEY
#undef DEFINE_PROPERTYKEY
#endif

#ifdef INITGUID
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY name
#endif // INITGUID

// ----------------------------------------------------------------------
//
// PKEY_Endpoint_Enable_Channel_Swap_SFX: When value is 0x00000001, Channel Swap local effect is enabled
// {A44531EF-5377-4944-AE15-53789A9629C7},2
// vartype = VT_UI4
DEFINE_PROPERTYKEY(PKEY_Endpoint_Enable_Channel_Swap_SFX, 0xa44531ef, 0x5377, 0x4944, 0xae, 0x15, 0x53, 0x78, 0x9a, 0x96, 0x29, 0xc7, 2);

// PKEY_Endpoint_Enable_Channel_Swap_MFX: When value is 0x00000001, Channel Swap global effect is enabled
// {A44531EF-5377-4944-AE15-53789A9629C7},3
// vartype = VT_UI4
DEFINE_PROPERTYKEY(PKEY_Endpoint_Enable_Channel_Swap_MFX, 0xa44531ef, 0x5377, 0x4944, 0xae, 0x15, 0x53, 0x78, 0x9a, 0x96, 0x29, 0xc7, 3);

// PKEY_Endpoint_Enable_Delay_SFX: When value is 0x00000001, Delay local effect is enabled
// {A44531EF-5377-4944-AE15-53789A9629C7},4
// vartype = VT_UI4
DEFINE_PROPERTYKEY(PKEY_Endpoint_Enable_Delay_SFX, 0xa44531ef, 0x5377, 0x4944, 0xae, 0x15, 0x53, 0x78, 0x9a, 0x96, 0x29, 0xc7, 4);

// PKEY_Endpoint_Enable_Delay_MFX: When value is 0x00000001, Delay global effect is enabled
// {A44531EF-5377-4944-AE15-53789A9629C7},5
// vartype = VT_UI4
DEFINE_PROPERTYKEY(PKEY_Endpoint_Enable_Delay_MFX, 0xa44531ef, 0x5377, 0x4944, 0xae, 0x15, 0x53, 0x78, 0x9a, 0x96, 0x29, 0xc7, 5);
