/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscschema.h

Abstract:

   PageScaling PrintSchema definition. This defines the features, options
   and enumerations that describe the PrintSchema PageScaling feature within
   a XDPrintSchema::PageScaling namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // PageScaling elements described as Printschema keywords
    //
    namespace PageScaling
    {
        //
        // The feature name
        //
        extern LPCWSTR SCALE_FEATURE;

        //
        // Option names
        //
        enum EScaleOption
        {
            Custom = 0, EScaleOptionMin = 0,
            CustomSquare,
            FitBleedToImageable,
            FitContentToImageable,
            FitMediaToImageable,
            FitMediaToMedia,
            None,
            EScaleOptionMax
        };

        extern LPCWSTR SCALE_OPTIONS[EScaleOptionMax];

        //
        // Custom scaling properties
        //
        enum ECustomScaleProps
        {
            CstOffsetWidth = 0, ECustomScalePropsMin = 0,
            CstOffsetHeight,
            CstScaleWidth,
            CstScaleHeight,
            ECustomScalePropsMax
        };

        extern LPCWSTR CUST_SCALE_PROPS[ECustomScalePropsMax];

        //
        // Custom square scaling properties
        //
        enum ECustomSquareScaleProps
        {
            CstSqOffsetWidth = 0, ECustomSquareScalePropsMin = 0,
            CstSqOffsetHeight,
            CstSqScale,
            ECustomSquareScalePropsMax
        };

        extern LPCWSTR CUST_SQR_SCALE_PROPS[ECustomSquareScalePropsMax];

        namespace OffsetAlignment
        {
            //
            // The feature name
            //
            extern LPCWSTR SCALE_OFFSET_FEATURE;

            //
            // Option names
            //
            enum EScaleOffsetOption
            {
                BottomCenter = 0, EScaleOffsetOptionMin = 0,
                BottomLeft,
                BottomRight,
                Center,
                LeftCenter,
                RightCenter,
                TopCenter,
                TopLeft,
                TopRight,
                EScaleOffsetOptionMax
            };

            extern LPCWSTR SCALE_OFFSET_OPTIONS[EScaleOffsetOptionMax];
        }
    }
}

