/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmschema.h

Abstract:

   PageWatermark PrintSchema definition. This defines the features, options
   and enumerations that describe the PrintSchema PageWatermark feature within
   a XDPrintSchema::PageWatermark namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageWatermark elements described as Printschema keywords
    //
    namespace PageWatermark
    {
        //
        // Feature name
        //
        extern LPCWSTR WATERMARK_FEATURE;

        //
        // Option names
        //
        enum EWatermarkOption
        {
            NoWatermark = 0, EWatermarkOptionMin = 0,
            TextWatermark,
            BitmapWatermark,
            VectorWatermark,
            EWatermarkOptionMax
        };

        extern LPCWSTR WATERMARK_OPTIONS[EWatermarkOptionMax];

        //
        // Common watermark properties
        //
        enum ECommonWatermarkProps
        {
            WidthOrigin = 0, ECommonWatermarkPropsMin = 0,
            HeightOrigin,
            Transparency,
            Angle,
            ECommonWatermarkPropsMax
        };

        extern LPCWSTR CMN_WATERMARK_PROPS[ECommonWatermarkPropsMax];

        //
        // Text watermark properties
        //
        enum ETextWatermarkProps
        {
            FontColor = 0, ETextWatermarkPropsMin = 0,
            FontSize,
            Text,
            ETextWatermarkPropsMax
        };

        extern LPCWSTR TXT_WATERMARK_PROPS[ETextWatermarkPropsMax];

        //
        // Vector / Bitmap common properties
        //
        enum EVectBmpWatermarkProps
        {
            WidthExtent = 0, EVectBmpWatermarkPropsMin = 0,
            HeightExtent,
            EVectBmpWatermarkPropsMax
        };

        extern LPCWSTR VECTBMP_WATERMARK_PROPS[EVectBmpWatermarkPropsMax];

        //
        // Layering sub feature
        //
        namespace Layering
        {
            extern LPCWSTR LAYERING_FEATURE;

            //
            // Layering options
            //
            enum ELayeringOption
            {
                Underlay = 0, ELayeringOptionMin = 0,
                Overlay,
                ELayeringOptionMax
            };

            extern LPCWSTR LAYERING_OPTIONS[ELayeringOptionMax];
        }
    }
}

