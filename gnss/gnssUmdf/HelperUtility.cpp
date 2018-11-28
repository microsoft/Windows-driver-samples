/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    HelperUtility.cpp

Abstract:

    This file contains the helper functions.

Environment:

    Windows User-Mode Driver Framework

--*/

#include "precomp.h"
#include "Trace.h"
#include "HelperUtility.h"

#include "HelperUtility.tmh"


_Use_decl_annotations_
BOOL
HelperUtility::GetDataFromCommand(
    _In_ PGNSS_DRIVERCOMMAND_PARAM Command,
    _In_ size_t DataLen,
    _Out_ void* Data
)
{
    ZeroMemory(Data, DataLen);

    if (!Command || (Command->CommandDataSize != DataLen))
    {
        return FALSE;
    }

    memcpy_s(Data, DataLen, Command->CommandData, DataLen);
    return TRUE;
}
