/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    xfilter.h

Abstract:

    Header file for the address filtering library for NDIS MAC's.

Author:

Environment:

Notes:

    None.

Revision History:

--*/

#ifndef _X_FILTER_DEFS_
#define _X_FILTER_DEFS_

#pragma once

#define ETH_LENGTH_OF_ADDRESS 6


//
// ZZZ This is a little-endian specific check.
//
#define ETH_IS_MULTICAST(Address) \
    (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x01))


//
// Check whether an address is broadcast.
//
#define ETH_IS_BROADCAST(Address)               \
    ((((PUCHAR)(Address))[0] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[1] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[2] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[3] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[4] == ((UCHAR)0xff)) && (((PUCHAR)(Address))[5] == ((UCHAR)0xff)))


//
// This macro will compare network addresses.
//
//  A - Is a network address.
//
//  B - Is a network address.
//
//  Result - The result of comparing two network address.
//
//  Result < 0 Implies the B address is greater.
//  Result > 0 Implies the A element is greater.
//  Result = 0 Implies equality.
//
// Note that this is an arbitrary ordering.  There is not
// defined relation on network addresses.  This is ad-hoc!
//
//
#define ETH_COMPARE_NETWORK_ADDRESSES(_A, _B, _Result)          \
{                                                               \
    if (*(ULONG UNALIGNED *)&(_A)[2] >                          \
         *(ULONG UNALIGNED *)&(_B)[2])                          \
    {                                                           \
        *(_Result) = 1;                                         \
    }                                                           \
    else if (*(ULONG UNALIGNED *)&(_A)[2] <                     \
                *(ULONG UNALIGNED *)&(_B)[2])                   \
    {                                                           \
        *(_Result) = (UINT)-1;                                  \
    }                                                           \
    else if (*(USHORT UNALIGNED *)(_A) >                        \
                *(USHORT UNALIGNED *)(_B))                      \
    {                                                           \
        *(_Result) = 1;                                         \
    }                                                           \
    else if (*(USHORT UNALIGNED *)(_A) <                        \
                *(USHORT UNALIGNED *)(_B))                      \
    {                                                           \
        *(_Result) = (UINT)-1;                                  \
    }                                                           \
    else                                                        \
    {                                                           \
        *(_Result) = 0;                                         \
    }                                                           \
}

//
// This macro will compare network addresses.
//
//  A - Is a network address.
//
//  B - Is a network address.
//
//  Result - The result of comparing two network address.
//
//  Result != 0 Implies inequality.
//  Result == 0 Implies equality.
//
//
#define ETH_COMPARE_NETWORK_ADDRESSES_EQ(_A,_B, _Result)        \
{                                                               \
    if ((*(ULONG UNALIGNED *)&(_A)[2] ==                        \
            *(ULONG UNALIGNED *)&(_B)[2]) &&                    \
         (*(USHORT UNALIGNED *)(_A) ==                          \
            *(USHORT UNALIGNED *)(_B)))                         \
    {                                                           \
        *(_Result) = 0;                                         \
    }                                                           \
    else                                                        \
    {                                                           \
        *(_Result) = 1;                                         \
    }                                                           \
}


//
// This macro is used to copy from one network address to
// another.
//
#define ETH_COPY_NETWORK_ADDRESS(_D, _S) \
{ \
    *((ULONG UNALIGNED *)(_D)) = *((ULONG UNALIGNED *)(_S)); \
    *((USHORT UNALIGNED *)((UCHAR *)(_D)+4)) = *((USHORT UNALIGNED *)((UCHAR *)(_S)+4)); \
}

#define TR_LENGTH_OF_FUNCTIONAL     4
#define TR_LENGTH_OF_ADDRESS        6


//
// Only the low 32 bits of the functional/group address
// are needed since the upper 16 bits is always c0-00.
//
typedef ULONG TR_FUNCTIONAL_ADDRESS;
typedef ULONG TR_GROUP_ADDRESS;


#define TR_IS_NOT_DIRECTED(_Address, _Result)                               \
{                                                                           \
    *(_Result) = (BOOLEAN)((_Address)[0] & 0x80);                           \
}

#define TR_IS_FUNCTIONAL(_Address, _Result)                                 \
{                                                                           \
    *(_Result) = (BOOLEAN)(((_Address)[0] & 0x80) &&                        \
                          !((_Address)[2] & 0x80));                         \
}

//
//
#define TR_IS_GROUP(_Address, _Result)                                      \
{                                                                           \
    *(_Result) = (BOOLEAN)((_Address)[0] & (_Address)[2] & 0x80);           \
}

//
//
#define TR_IS_SOURCE_ROUTING(_Address, _Result)                             \
{                                                                           \
    *(_Result) = (BOOLEAN)((_Address)[0] & 0x80);                           \
}

//
//  Check for NDIS_PACKET_TYPE_MAC_FRAME
//
#define TR_IS_MAC_FRAME(_PacketHeader)  ((((PUCHAR)_PacketHeader)[1] & 0xFC) == 0)


//
// Check whether an address is broadcast. This is a little-endian check.
//
#define TR_IS_BROADCAST(_Address, _Result)                                      \
{                                                                               \
    *(_Result) = (BOOLEAN)(((*(UNALIGNED USHORT *)&(_Address)[0] == 0xFFFF) ||  \
                            (*(UNALIGNED USHORT *)&(_Address)[0] == 0x00C0)) && \
                            (*(UNALIGNED ULONG  *)&(_Address)[2] == 0xFFFFFFFF));\
}


//
// This macro will compare network addresses.
//
//  A - Is a network address.
//
//  B - Is a network address.
//
//  Result - The result of comparing two network address.
//
//  Result < 0 Implies the B address is greater.
//  Result > 0 Implies the A element is greater.
//  Result = 0 Implies equality.
//
// Note that this is an arbitrary ordering.  There is not
// defined relation on network addresses.  This is ad-hoc!
//
//
#define TR_COMPARE_NETWORK_ADDRESSES(_A, _B, _Result)           \
{                                                               \
    if (*(ULONG UNALIGNED *)&(_A)[2] >                          \
        *(ULONG UNALIGNED *)&(_B)[2])                           \
    {                                                           \
        *(_Result) = 1;                                         \
    }                                                           \
    else if (*(ULONG UNALIGNED *)&(_A)[2] <                     \
             *(ULONG UNALIGNED *)&(_B)[2])                      \
    {                                                           \
        *(_Result) = (UINT)-1;                                  \
    }                                                           \
    else if (*(USHORT UNALIGNED *)(_A) >                        \
             *(USHORT UNALIGNED *)(_B))                         \
    {                                                           \
        *(_Result) = 1;                                         \
    }                                                           \
    else if (*(USHORT UNALIGNED *)(_A) <                        \
             *(USHORT UNALIGNED *)(_B))                         \
    {                                                           \
        *(_Result) = (UINT)-1;                                  \
    }                                                           \
    else                                                        \
    {                                                           \
        *(_Result) = 0;                                         \
    }                                                           \
}

//
// This macro will compare network addresses.
//
//  A - Is a network address.
//
//  B - Is a network address.
//
//  Result - The result of comparing two network address.
//
//  Result != 0 Implies inequality.
//  Result == 0 Implies equality.
//
//
#define TR_COMPARE_NETWORK_ADDRESSES_EQ(_A, _B, _Result)                    \
{                                                                           \
    if ((*(ULONG UNALIGNED  *)&(_A)[2] == *(ULONG UNALIGNED  *)&(_B)[2]) && \
        (*(USHORT UNALIGNED *)&(_A)[0] == *(USHORT UNALIGNED *)&(_B)[0]))   \
    {                                                                       \
        *(_Result) = 0;                                                     \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        *(_Result) = 1;                                                     \
    }                                                                       \
}


//
// This macro is used to copy from one network address to
// another.
//
#define TR_COPY_NETWORK_ADDRESS(_D, _S)                                     \
{                                                                           \
    *((ULONG UNALIGNED *)(_D)) = *((ULONG UNALIGNED *)(_S));                \
    *((USHORT UNALIGNED *)((UCHAR *)(_D)+4)) =                              \
                            *((USHORT UNALIGNED *)((UCHAR *)(_S)+4));       \
}

#endif // _X_FILTER_DEFS_
