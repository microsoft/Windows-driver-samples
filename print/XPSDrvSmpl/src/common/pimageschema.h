/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pimageschema.h

Abstract:

   PageImageableSize PrintSchema definition. This defines the features, options
   and enumerations that describe the PrintSchema PageImageableSize feature within
   a XDPrintSchema::PageImageableSize namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageImageSize elements described as Printschema keywords
    //
    namespace PageImageableSize
    {
        //
        // The property name
        //
        extern LPCWSTR PAGE_IMAGEABLE_PROPERTY;

        enum EPageImageableProps
        {
            ImageableSizeWidth = 0, EPageImageablePropsMin = 0,
            ImageableSizeHeight,
            ImageableArea,
            EPageImageablePropsMax
        };

        extern LPCWSTR PAGE_IMAGEABLE_PROPS[EPageImageablePropsMax];

        enum EPageImageablePropsArea
        {
            OriginWidth = 0, EPageImageablePropsAreaMin = 0,
            OriginHeight,
            ExtentWidth,
            ExtentHeight,
            EPageImageablePropsAreaMax
        };

        extern LPCWSTR PAGE_IMAGEABLE_PROPS_AREA[EPageImageablePropsAreaMax];
    }
}

