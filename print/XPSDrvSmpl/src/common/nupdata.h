/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupdata.h

Abstract:

   NUp data structure definition. This provides a convenient description
   of the PrintSchema JobNUpAllDocumentsContiguously and DocumentNUp features.

--*/

#pragma once

#include "nupschema.h"

namespace XDPrintSchema
{
    namespace NUp
    {
        struct NUpData
        {
            NUpData() :
                nUpFeature(JobNUpAllDocumentsContiguously),
                cNUp(1),
                nUpPresentDir(PresentationDirection::RightBottom)
            {
            }

            ENUpFeature                                nUpFeature;
            INT                                        cNUp;
            PresentationDirection::ENUpDirectionOption nUpPresentDir;
        };
    }
}

