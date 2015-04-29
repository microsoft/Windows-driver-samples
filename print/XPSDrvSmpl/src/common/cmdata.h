/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmdata.h

Abstract:

   PageColorManagement data structure definition. This provides a convenient
   description of the PrintSchema PageColorManagement feature.

--*/

#pragma once

#include "cmschema.h"

namespace XDPrintSchema
{
    namespace PageColorManagement
    {
        struct ColorManagementData
        {
            ColorManagementData() :
                cmOption(None)
            {
            }

            EPCMOption cmOption;
        };
    }
}

