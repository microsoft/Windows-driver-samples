/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupschema.h

Abstract:

   NUp PrintSchema definition. This defines the features, options and
   enumerations that describe the PrintSchema JobNUpAllDocumentsContiguously and DocumentNUp
   features within a XDPrintSchema::NUp namespace.

--*/

#pragma once

#include "schema.h"

namespace XDPrintSchema
{
    //
    // NUp elements described as Printschema keywords
    //
    namespace NUp
    {
        //
        // JobNUpAllDocumentsContiguously and DocumentNUp share identical options so we define two
        // features within the NUp namespace.
        //
        enum ENUpFeature
        {
            JobNUpAllDocumentsContiguously = 0, ENUpFeatureMin = 0,
            DocumentNUp,
            ENUpFeatureMax
        };

        extern LPCWSTR NUP_FEATURES[ENUpFeatureMax];

        extern LPCWSTR NUP_PROP;

        namespace PresentationDirection
        {
            //
            // Feature name
            //
            extern LPCWSTR NUP_DIRECTION_FEATURE;

            //
            // Option names
            //
            enum ENUpDirectionOption
            {
                RightBottom = 0, ENUpDirectionOptionMin = 0,
                BottomRight,
                LeftBottom,
                BottomLeft,
                RightTop,
                TopRight,
                LeftTop,
                TopLeft,
                ENUpDirectionOptionMax
            };

            extern LPCWSTR NUP_DIRECTION_OPTIONS[ENUpDirectionOptionMax];
        }
    }
}

