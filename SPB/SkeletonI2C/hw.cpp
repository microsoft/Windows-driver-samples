/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    hw.cpp

Abstract:

    This module contains the functions for accessing
    the hardware registers.

Environment:

    kernel-mode only

Revision History:

--*/

#include "internal.h"
#include "hw.tmh"

ULONG
HWREG<ULONG>::Read(
    VOID
    )
{
    volatile ULONG *addr = &m_Value;
    ULONG v = READ_REGISTER_ULONG((PULONG)addr);
    return v;
}

ULONG
HWREG<ULONG>::Write(
    _In_ ULONG Value
    )
{
    volatile ULONG *addr = &m_Value;
    WRITE_REGISTER_ULONG((PULONG)addr, Value);
    return Value;
}

USHORT
HWREG<USHORT>::Read(
    VOID
    )
{
    volatile USHORT *addr = &m_Value;
    USHORT v = READ_REGISTER_USHORT((PUSHORT)addr);
    return v;
}

USHORT
HWREG<USHORT>::Write(
    _In_ USHORT Value
    )
{
    volatile USHORT *addr = &m_Value;
    WRITE_REGISTER_USHORT((PUSHORT)addr, Value);
    return Value;
}

UCHAR
HWREG<UCHAR>::Read(
    VOID
    )
{
    volatile UCHAR *addr = &m_Value;
    UCHAR v = READ_REGISTER_UCHAR((PUCHAR)addr);
    return v;
}

UCHAR
HWREG<UCHAR>::Write(
    _In_ UCHAR Value
    )
{
    volatile UCHAR *addr = &m_Value;
    WRITE_REGISTER_UCHAR((PUCHAR)addr, Value);
    return Value;
}
