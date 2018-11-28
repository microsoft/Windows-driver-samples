/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    HelperUtility.h

Abstract:

    This module contains the Helper class used by other classes within the Umdf Gnss Driver.

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

class HelperUtility
{
public:
    static BOOL GetDataFromCommand(_In_ PGNSS_DRIVERCOMMAND_PARAM Command,
                                   _In_ size_t DataLen,
                                   _Out_ void* Data);
};

template <class DataT>
BOOL GetDataFromCommand(
    _In_ PGNSS_DRIVERCOMMAND_PARAM Command,
    _Inout_ DataT* Data)
{
    return HelperUtility::GetDataFromCommand(Command, sizeof(*Data), Data);
}

template <class DataT>
void INIT_POD_GNSS_STRUCT(
    _Out_ DataT* Data
)
{
    RtlZeroMemory(Data, sizeof(*Data));

    Data->Size = sizeof(*Data);
    Data->Version = GNSS_DRIVER_DDK_VERSION;
}
