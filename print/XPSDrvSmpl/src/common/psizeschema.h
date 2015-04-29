/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   psizeschema.h

Abstract:

   PageMediaSize PrintSchema definition. This defines the features, options
   and enumerations that describe the PrintSchema PageMediaSize feature within
   a XDPrintSchema::PageMediaSize namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageMediaSize elements described as Printschema keywords
    //
    namespace PageMediaSize
    {
        //
        // The feature name
        //
        extern LPCWSTR PAGESIZE_FEATURE;

        //
        // We don't actually ever use the page size option names so they are
        // omitted for brevity as there are many of them.
        //

        enum EPageSizeProps
        {
            MediaSizeWidth = 0, EPageSizePropsMin = 0,
            MediaSizeHeight,
            EPageSizePropsMax
        };

        extern LPCWSTR PAGESIZE_PROPS[EPageSizePropsMax];
    }
}

