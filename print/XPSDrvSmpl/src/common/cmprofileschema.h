/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmintentsschema.h

Abstract:

   PageSourceColorProfile PrintSchema definition. This defines the features,
   options and enumerations that describe the PrintSchema PageSourceColorProfile
   feature within a XDPrintSchema::PageSourceColorProfile namespace.


--*/

#pragma once

#include "schema.h"
#include "globals.h"
#include "privatedefs.h"

namespace XDPrintSchema
{
    //
    // PageColorManagement elements described as Printschema keywords
    //
    namespace PageSourceColorProfile
    {
        //
        // Feature name
        //
        extern LPCWSTR PROFILE_FEATURE;

        //
        // Who specified the profile to use
        //
        enum EProfileOption
        {
            RGB = 0, EProfileOptionMin = 0,
            CMYK,
            EProfileOptionMax
        };

        extern LPCWSTR PROFILE_OPTIONS[EProfileOptionMax];

        extern LPCWSTR PROFILE_URI_PROP;
        extern LPCWSTR PROFILE_URI_REF;
        extern PRIVATE_DEF_STRINGS PROFILE_PARAM_DEF;
    }
}

