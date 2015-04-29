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

   PageICMRenderingIntent PrintSchema definition. This defines the features,
   options and enumerations that describe the PrintSchema PageICMRenderingIntent feature
   within a XDPrintSchema::PageICMRenderingIntent namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageColorManagementIntents elements described as Printschema keywords
    //
    namespace PageICMRenderingIntent
    {
        //
        // Feature name
        //
        extern LPCWSTR ICMINTENT_FEATURE;

        //
        // Who specified the profile to use
        //
        enum EICMIntentOption
        {
            AbsoluteColorimetric = 0, EICMIntentOptionMin = 0,
            RelativeColorimetric,
            Photographs,
            BusinessGraphics,
            EICMIntentOptionMax
        };

        extern LPCWSTR ICMINTENT_OPTIONS[EICMIntentOptionMax];
    }
}

