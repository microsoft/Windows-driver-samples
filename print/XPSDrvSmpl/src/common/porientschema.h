/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   porientschema.h

Abstract:

   PageOrientation PrintSchema definition. This defines the features, options
   and enumerations that describe the PrintSchema PageOrientation feature within
   a XDPrintSchema::PageOrientation namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageOrientation elements described as Printschema keywords
    //
    namespace PageOrientation
    {
        //
        // The feature name
        //
        extern LPCWSTR ORIENTATION_FEATURE;

        //
        // Paper orientation options
        //
        enum EOrientationOption
        {
            Landscape = 0, EOrientationOptionMin = 0,
            Portrait,
            ReverseLandscape,
            ReversePortrait,
            EOrientationOptionMax
        };

        extern LPCWSTR ORIENTATION_OPTIONS[EOrientationOptionMax];
    }
}

