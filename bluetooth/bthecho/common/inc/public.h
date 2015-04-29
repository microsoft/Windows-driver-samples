/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

ModuleName:

    public.h

Abstract:

    Contains definitions used both by driver and application


--*/

#pragma once

//
// Service GUID and name for the service published by our bth server device
//

/* c07508f2-b970-43ca-b5dd-cc4f2391bef4 */
DEFINE_GUID(BTHECHOSAMPLE_SVC_GUID, 0xc07508f2, 0xb970, 0x43ca, 0xb5, 0xdd, 0xcc, 0x4f, 0x23, 0x91, 0xbe, 0xf4);
extern __declspec(selectany) const PWSTR BthEchoSampleSvcName = L"BthEchoSampleSrv";

//
// Device interface exposed by our bth client device
//

/* fc71b33d-d528-4763-a86c-78777c7bcd7b */
DEFINE_GUID(BTHECHOSAMPLE_DEVICE_INTERFACE, 0xfc71b33d, 0xd528, 0x4763, 0xa8, 0x6c, 0x78, 0x77, 0x7c, 0x7b, 0xcd, 0x7b);


