/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmintentsdata.h

Abstract:

   PageICMRenderingIntent data structure definition. This provides a more
   convenient description of the PrintSchema PageICMRenderingIntent feature.

--*/

#pragma once

#include "cmintentsschema.h"

namespace XDPrintSchema
{
    namespace PageICMRenderingIntent
    {
        struct PageICMRenderingIntentData
        {
            PageICMRenderingIntentData() :
                cmOption(AbsoluteColorimetric)
            {
            }

            EICMIntentOption cmOption;
        };
    }
}

