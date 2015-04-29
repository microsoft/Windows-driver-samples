/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscdata.h

Abstract:

   PageScaling data structure definition. This provides a convenient
   description of the PrintSchema PageScaling feature.

--*/

#pragma once

#include "pgscschema.h"

namespace XDPrintSchema
{
    namespace PageScaling
    {
        struct PageScalingData
        {
            PageScalingData() :
                pgscOption(None),
                offWidth(0),
                offHeight(0),
                scaleWidth(100),
                scaleHeight(100),
                offsetOption(OffsetAlignment::Center)
            {
            }

            EScaleOption                        pgscOption;
            INT                                 offWidth;
            INT                                 offHeight;
            INT                                 scaleWidth;
            INT                                 scaleHeight;
            OffsetAlignment::EScaleOffsetOption offsetOption;
        };
    }
}

