/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    hw.h

Abstract:

    This module contains the function definitions for
    the hardware registers.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _HW_H_
#define _HW_H_

template<typename T> struct HWREG
{
private: 

    //
    // Only one data member - this has to fit in the same space as the underlying type.
    //

    T m_Value;

public:

    T Read(void);
    T Write(_In_ T value);

    VOID 
    ReadBuffer(
        _In_                   ULONG BufferCe,
        _Out_writes_(BufferCe) T     Buffer[]
        )
    {
        for(ULONG i = 0; i < BufferCe; i++)
        {
            Buffer[i] = Read();
        }
    }

    VOID 
    WriteBuffer(
        _In_                   ULONG BufferCe,
        _Out_writes_(BufferCe) T     Buffer[]
        )
    {
        for(ULONG i = 0; i < BufferCe; i++)
        {
            Write(Buffer[i]);
        }
    }

    //
    // Operators with standard meanings.
    //

    T operator= (_In_ T value) {return Write(value);}
    T operator|=(_In_ T value) {return Write(Read() | value);}
    T operator&=(_In_ T value) {return Write(value | Read());}
      operator T()             {return Read();}

    //
    // Override the meaning of exclusive OR to mean clear
    //
    // Added this because x &= ~foo requires a cast of ~foo from signed int
    // back to the underlying (typically unsigned) type.  I would prefer x ~= foo 
    // but that's not a real C++ operator.
    //

    T operator^=(_In_ T value) {return Write(((T) ~value) & Read());}

    T SetBits  (_In_ T Flags) {return (*this |= Flags);}
    T ClearBits(_In_ T Flags) {return (*this &= ~Flags);}
    bool TestBits (_In_ T Flags) {return ((Read() & Flags) != 0)};
};

#endif
