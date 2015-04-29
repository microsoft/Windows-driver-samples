/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   xdstring.h

Abstract:

   A string class that emulates the CStringT class interface. The class uses an
   internal string of default length 128 characters. The string size is doubled
   on each resize.

--*/

#pragma once

#include <strsafe.h>
#include "xdexcept.h"

#define CCH_INITIAL_BUFFER 128

template< typename _T = CHAR >
class ChTraitsBaseXD
{
public:
    typedef CHAR    XCHARXD;
    typedef WCHAR   YCHARXD;
};

template<>
class ChTraitsBaseXD< WCHAR >
{
public:
    typedef WCHAR   XCHARXD;
    typedef CHAR    YCHARXD;
};

template <typename _T>
class CStringXDT
{
public:
    typedef typename ChTraitsBaseXD<_T>::XCHARXD  XCHARXD;
    typedef typename ChTraitsBaseXD<_T>::YCHARXD  YCHARXD;

public:
    /*++

    Routine Name:

        CStringXDT

    Routine Description:

        Class constructor

    Arguments:

        None

    Return Value:

        None

    --*/
    CStringXDT() :
        m_pszData(NULL)
    {
    }

    /*++

    Routine Name:

        CStringXDT

    Routine Description:

        Class constructor

    Arguments:

        cstrSrc - source CStringXDT class to construct from

    Return Value:

        None

    --*/
    CStringXDT(
        _In_ CONST CStringXDT<_T>& cstrSrc
        ) :
        m_pszData(NULL)
    {
        CreateString(cstrSrc.GetString());
    }

    /*++

    Routine Name:

        CStringXDT

    Routine Description:

        Class constructor

    Arguments:

        pszSrc - source string that matches the template argument to construct from

    Return Value:

        None

    --*/
    CStringXDT(
        _In_z_ CONST XCHARXD* pszSrc
        ) :
        m_pszData(NULL)
    {
        CreateString(pszSrc);
    }


    /*++

    Routine Name:

        CStringXDT

    Routine Description:

        Class constructor

    Arguments:

        pszSrc - source string to construct from. This is the "cross"
                 string, i.e. CHAR for WCHAR template and vice versa

    Return Value:

        None

    --*/
    CStringXDT(
        _In_z_ CONST YCHARXD* pszSrc
        ) :
        m_pszData(NULL)
    {
        CreateStringFromY(pszSrc);
    }

    /*++

    Routine Name:

        CStringXDT

    Routine Description:

        Class constructor

    Arguments:

        pszSrc - source string that matches the template argument to construct from
        cchSrc - the length of the source string to construct from

    Return Value:

        None

    --*/
    CStringXDT(
        _In_reads_(cchSrc) CONST XCHARXD* pszSrc,
        _In_                INT            cchSrc
        ) :
        m_pszData(NULL)
    {
        if (cchSrc < 0)
        {
            RIP("Negative string size requested.\n");
            throw CXDException(E_INVALIDARG);
        }

        CreateString(pszSrc, static_cast<size_t>(cchSrc));
    }

    /*++

    Routine Name:

        CStringXDT

    Routine Description:

        Class constructor

    Arguments:

        pszSrc - source string to construct from. This is the "cross"
                 string, i.e. CHAR for WCHAR template and vice versa
        cchSrc - the length of the source string to construct from

    Return Value:

        None

    --*/
    CStringXDT(
        _In_reads_(cchSrc) CONST YCHARXD* pszSrc,
        _In_                INT            cchSrc
        ) :
        m_pszData(NULL)
    {
        if (cchSrc < 0)
        {
            RIP("Negative string size requested.\n");
            throw CXDException(E_INVALIDARG);
        }

        CreateStringFromY(pszSrc, static_cast<size_t>(cchSrc));
    }

    /*++

    Routine Name:

        ~CStringXDT

    Routine Description:

        Class destructor

    Arguments:

        None

    Return Value:

        None

    --*/
    ~CStringXDT()
    {
        if (m_pszData != NULL)
        {
            HeapFree(GetProcessHeap(), 0, GetDataBuffer());
            m_pszData = NULL;
        }
    }

    //
    // Operators
    //
    /*++

    Routine Name:

        operator=

    Routine Description:

        Assignment operator

    Arguments:

        cstrSrc - const reference to the source CStringXDT

    Return Value:

        Reference to the newly assigned CStringXDT instance

    --*/
    CStringXDT<_T>&
    operator=(
        _In_ CONST CStringXDT<_T>& cstrSrc
        )
    {
        *this = cstrSrc.m_pszData;

        return *this;
    }

    /*++

    Routine Name:

        operator=

    Routine Description:

        Assignment operator

    Arguments:

        pszSrc - const pointer to the source string of the same type as the template

    Return Value:

        Reference to the newly assigned CStringXDT instance

    --*/
    CStringXDT<_T>&
    operator=(
        _In_z_ CONST XCHARXD* pszSrc
        )
    {
        if (this->operator!=(pszSrc))
        {
            Empty();
            CreateString(pszSrc);
        }

        return *this;
    }

    /*++

    Routine Name:

        operator=

    Routine Description:

        Assignment operator

    Arguments:

        pszSrc - const pointer to the source string of the opposite type to the template

    Return Value:

        Reference to the newly assigned CStringXDT instance

    --*/
    CStringXDT<_T>&
    operator=(
        _In_z_ CONST YCHARXD* pszSrc
        )
    {
        if (this->operator!=(pszSrc))
        {
            Empty();
            CreateStringFromY(pszSrc);
        }

        return *this;
    }

    /*++

    Routine Name:

        operator+=

    Routine Description:

        Addition assignment operator - appends a source string to the current underlying string

    Arguments:

        pszSrc - pointer to the native string to append

    Return Value:

        Reference to this instance

    --*/
    CStringXDT<_T>&
    operator+=(
        _In_z_ CONST XCHARXD* pszSrc
        )
    {
        size_t cchSrc = StringLength(pszSrc);
        size_t cchCur = StringLength(GetString());

        SetBufferSize(cchSrc + cchCur);
        CopyString(GetString() + cchCur, GetBufferCharCount() - cchCur, pszSrc, cchSrc);

        return *this;
    }

    /*++

    Routine Name:

        operator+=

    Routine Description:

        Addition assignment operator - appends a source string to the current underlying string

    Arguments:

        pszSrc - pointer to the opposite string type to append

    Return Value:

        Reference to this instance

    --*/
    CStringXDT<_T>&
    operator+=(
        _In_z_ CONST YCHARXD* pszSrc
        )
    {
        size_t cchSrc = StringLength(pszSrc);
        size_t cchCur = StringLength(GetString());

        SetBufferSize(cchSrc + cchCur);
        CopyYString(GetString() + cchCur, GetBufferCharCount() - cchCur, pszSrc, cchSrc);

        return *this;
    }

    /*++

    Routine Name:

        operator CONST _T*()

    Routine Description:

        const cast operator

    Arguments:

        None

    Return Value:

        Pointer to the underlying string

    --*/
    operator CONST _T*() CONST throw()
    {
        return GetString();
    }

    /*++

    Routine Name:

        operator!=

    Routine Description:

        Inequality operator - compares against the native string type

    Arguments:

        pszCompare - the string to compare to

    Return Value:

        true  - the strings do not match
        false - the strings match

    --*/
    bool
    operator!=(
        _In_z_ CONST XCHARXD* pszCompare
        ) CONST
    {
        return !operator==(pszCompare);
    }

    /*++

    Routine Name:

        operator==

    Routine Description:

        Equality operator - compares against the native string type

    Arguments:

        pszCompare - the string to compare to

    Return Value:

        true  - the strings match
        false - the strings do not match

    --*/
    bool
    operator==(
        _In_z_ CONST XCHARXD* pszCompare
        ) CONST
    {
        return CompareXDString(GetString(), pszCompare) == 0;
    }

    /*++

    Routine Name:

        operator!=

    Routine Description:

        Inequality operator - compares against the oposite string type

    Arguments:

        pszCompare - the string to compare to

    Return Value:

        true  - the strings do not match
        false - the strings match

    --*/
    bool
    operator!=(
        _In_z_ CONST YCHARXD* pszCompare
        ) CONST
    {
        return !operator==(pszCompare);
    }

    /*++

    Routine Name:

        operator==

    Routine Description:

        Equality operator - compares against the opposite string type

    Arguments:

        pszCompare - the string to compare to

    Return Value:

        true  - the strings match
        false - the strings do not match

    --*/
    bool
    operator==(
        _In_z_ CONST YCHARXD* pszCompare
        ) CONST
    {
        CStringXDT<XCHARXD> cstrCompare(pszCompare);
        return CompareXDString(GetString(), cstrCompare) == 0;
    }

    /*++

    Routine Name:

        operator<

    Routine Description:

        Less than operator

    Arguments:

        cstrCompare - The CStringXDT instance to compare to

    Return Value:

        true  - this string instance is less than the compare string
        false - this string instance is not less than the compare string

    --*/
    bool
    operator<(
        _In_ CONST CStringXDT<_T>& cstrCompare
        ) CONST
    {
        return CompareXDString(GetString(), cstrCompare.GetString()) < 0;
    }

    /*++

    Routine Name:

        operator<

    Routine Description:

        Less than operator

    Arguments:

        cstrCompare - pointer to a native string to compate with

    Return Value:

        true  - this string instance is less than the compare string
        false - this string instance is not less than the compare string

    --*/
    bool
    operator<(
        _In_z_ CONST XCHARXD* pszCompare
        ) CONST
    {
        return CompareXDString(GetString(), pszCompare) < 0;
    }

    /*++

    Routine Name:

        operator<

    Routine Description:

        Less than operator

    Arguments:

        cstrCompare - pointer to a opposite string type to compate with

    Return Value:

        true  - this string instance is less than the compare string
        false - this string instance is not less than the compare string

    --*/
    bool
    operator<(
        _In_z_ CONST YCHARXD* pszCompare
        ) CONST
    {
        CStringXDT<XCHARXD> cstrCompare(pszCompare);
        return CompareXDString(GetString(), cstrCompare) < 0;
    }

    /*++

    Routine Name:

        operator[]

    Routine Description:

        Array index operator

    Arguments:

        cchIndex - the character index into the string

    Return Value:

        The value of the character at the specified index

    --*/
    _T operator[](
        _In_ INT cchIndex
        ) CONST
    {
        if (cchIndex < 0 ||
            cchIndex > GetLength())
        {
            throw CXDException(E_INVALIDARG);
        }

        return GetString()[cchIndex];
    }

    //
    // Methods
    //
    /*++

    Routine Name:

        AllocSysString

    Routine Description:

        Allocates a BSTR from the underlying string

    Arguments:

        None

    Return Value:

        The newly allocated BSTR

    --*/
    BSTR
    AllocSysString()
    {
        return AllocSysString(GetString());
    }

    /*++

    Routine Name:

        Find

    Routine Description:

        Finds the location of a sub string in the underlying string

    Arguments:

        pszSub   - the sub string to search for
        cchStart - the point to start searching the underlying string from

    Return Value:

        -1 if the sub string is not found
        the index of the sub-string in the underlying strings

    --*/
    INT
    Find(
        _In_z_ CONST _T* pszSub,
        _In_   INT       cchStart = 0
        ) CONST throw()
    {
        return Find(GetString(), pszSub, cchStart);
    }

    /*++

    Routine Name:

        Delete

    Routine Description:

        Deletes one or more characters from a given offset

    Arguments:

        cchStart  - the starting character to delete from
        cchDelete - the number of characters to delete

    Return Value:

        The length of the remaining string

    --*/
    INT
    Delete(
        INT cchStart,
        INT cchDelete = 1
        )
    {
        if (cchStart < 0)
        {
            cchStart = 0;
        }

        if (cchDelete < 0)
        {
            cchDelete = 0;
        }

        size_t cchStr = StringLength(GetString());
        if (cchStr < static_cast<size_t>(cchStart + cchDelete))
        {
            throw CXDException(E_INVALIDARG);
        }

        size_t cchToMove = cchStr + 1 - static_cast<size_t>(cchDelete + cchStart);
        MoveMemory(GetString() + cchStart, m_pszData + cchStart + cchDelete, cchToMove * sizeof(_T));

        return GetLength();
    }

    /*++

    Routine Name:

        GetLength

    Routine Description:

        Retrieves the length of the string

    Arguments:

        None

    Return Value:

        The count of characters in the string

    --*/
    INT
    GetLength() CONST throw()
    {
        INT cch = 0;

        try
        {
            cch = static_cast<INT>(StringLength(GetString()));
        }
        catch (CXDException&)
        {
        }

        return cch;
    }

    /*++

    Routine Name:

        Format

    Routine Description:

        Writes formatted data to the string.

    Arguments:

        pszFormat - the format string
        ...       - Variable argument list

    Return Value:

        None

    --*/
    VOID
    Format(
        _In_z_ CONST _T* pszFormat,
        ...
        )
    {
        if (pszFormat == NULL)
        {
            throw CXDException(E_INVALIDARG);
        }

        va_list argList;
        va_start(argList, pszFormat);
        Format(pszFormat, argList);
        va_end(argList);
    }

    /*++

    Routine Name:

        Empty

    Routine Description:

        Sets the string to zero length

    Arguments:

        None

    Return Value:

        None

    --*/
    VOID
    Empty()
    {
        try
        {
            *GetString() = 0;
        }
        catch (CXDException&)
        {
        }
    }

    /*++

    Routine Name:

        Compare

    Routine Description:

        Compares the string data with a compare string passed in - case sensitive.
        This is the same functionality as strcmp.

    Arguments:

        pszCompare - the string to compare against

    Return Value:

        < 0 if the string data is less than the compare string
        0   if the strings are identical
        > 0 if the string data is more than the compare string

    --*/
    INT
    Compare(
       _In_z_ CONST _T* pszCompare
        ) CONST
    {
        return CompareXDString(GetString(), pszCompare);
    }

    /*++

    Routine Name:

        CompareNoCase

    Routine Description:

        Compares the string data with a compare string passed in - case insensitive.
        This is the same functionality as stricmp.

    Arguments:

        pszCompare - the string to compare against

    Return Value:

        < 0 if the string data is less than the compare string
        0   if the strings are identical
        > 0 if the string data is more than the compare string

    --*/
    INT
    CompareNoCase(
       _In_z_ CONST _T* pszCompare
        ) CONST throw()
    {
        return CompareXDStringNoCase(GetString(), pszCompare);
    }

    /*++

    Routine Name:

        Replace

    Routine Description:

        Replaces all instances of a target string with a new string

    Arguments:

        pszOld - the string to replace
        pszNew - the replacement string

    Return Value:

        The number of replaced instances

    --*/
    INT
    Replace(
       _In_z_ CONST _T* pszOld,
       _In_z_ CONST _T* pszNew
    )
    {
        //
        // Find the length of the old and new strings
        //
        size_t cchOld = StringLength(pszOld);
        size_t cchNew = StringLength(pszNew);

        //
        // Count the instances of the string to replace
        //
        _T* pszCurr = GetString();
        INT cReplace = 0;
        INT cchStart = Find(pszCurr, pszOld, 0);
        while (cchStart >= 0)
        {
            cReplace++;
            pszCurr += cchStart + cchOld;
            cchStart = Find(pszCurr, pszOld, 0);
        }

        if (cReplace > 0)
        {
            //
            // Resize the buffer to accomodate the replacements
            //
            INT cchDelta = static_cast<INT>(cchNew - cchOld);
            INT cchDeltaBuffer = cReplace * cchDelta;
            SetBufferSize(StringLength(GetString()) + cchDeltaBuffer);

            //
            // Replace all instances of the old string and replace with
            // the new string
            //
            pszCurr = GetString();
            cchStart = Find(pszCurr, pszOld, 0);
            while (cchStart >= 0)
            {
                //
                // Move the remainder of the string up by the length of the replacement
                // this makes room to insert the replacement string
                //
                pszCurr += cchStart;
                _T* pszOldEnd = pszCurr + cchOld;
                size_t cchToMove = StringLength(pszOldEnd) + 1;
                MoveMemory(pszCurr + cchNew, pszOldEnd, cchToMove * sizeof(_T));

                //
                // Fill the "gap" with the new string
                //
                CopyMemory(pszCurr, pszNew, cchNew * sizeof(_T));

                //
                // Look for the next string from the end of the current instance
                //
                pszCurr += cchNew;
                cchStart = Find(pszCurr, pszOld, 0);
            }
        }

        return cReplace;
    }

    /*++

    Routine Name:

        Append

    Routine Description:

        Appends a string to the current string data

    Arguments:

        pszSrc - the string to append

    Return Value:

        None

    --*/
    VOID
    Append(
        _In_z_ CONST _T* pszSrc
    )
    {
        *this += pszSrc;
    }

    /*++

    Routine Name:

        Insert

    Routine Description:

        Inserts a string to the current string data

    Arguments:

        cch    - the index at which to insert the string
        pszSrc - the string to insert

    Return Value:

        None

    --*/
    INT
    Insert(
        _In_   CONST INT& cch,
        _In_z_ CONST _T*  pszSrc
    )
    {
        if (cch >= 0 &&
            pszSrc != NULL)
        {
            _T* pszCurr    = GetString();
            size_t cchCurr = StringLength(pszCurr);

            if (cchCurr >= static_cast<size_t>(cch))
            {
                size_t cchSrc  = StringLength(pszSrc);
                SetBufferSize(cchCurr + cchSrc);

                //
                // Move the remainder of the string up by the length of the replacement
                // this makes room to insert the replacement string
                //
                pszCurr += cch;
                size_t cchToMove = StringLength(pszCurr) + 1;
                MoveMemory(pszCurr + cchSrc, pszCurr, cchToMove * sizeof(_T));

                //
                // Fill the "gap" with the new string
                //
                CopyMemory(pszCurr, pszSrc, cchSrc * sizeof(_T));
            }
        }
        else
        {
            throw CXDException(E_INVALIDARG);
        }

        return static_cast<INT>(StringLength(GetString()));
    }

    /*++

    Routine Name:

        IsEmpty

    Routine Description:

        Indicates whether the string is zero length

    Arguments:

        None

    Return Value:

        true  - the string is empty
        false - otherwise

    --*/
    bool
    IsEmpty() CONST throw()
    {
        return GetLength() == 0;
    }

    /*++

    Routine Name:

        GetBuffer

    Routine Description:

        Returns the underlying string buffer

    Arguments:

        None

    Return Value:

        The underlying string buffer

    --*/
    _T*
    GetBuffer()
    {
        return GetString();
    }

    /*++

    Routine Name:

        Tokenize

    Routine Description:

        Retrieves the position of the next token in a string

    Arguments:

        pszTokens - string defining the token
        cchStart  - the point at which the token search should start. This value is updated
                    by the call to the posiion following the end character of the token.

    Return Value:

        Returns a CStringXDT object containing the token value.

    --*/
    CStringXDT<_T>
    Tokenize(
        _In_z_  CONST _T* pszTokens,
        _Inout_ INT&      cchStart
        ) CONST
    {
        CStringXDT<_T> cstrResult;

        if (cchStart < 0)
        {
            throw CXDException(E_INVALIDARG);
        }

        INT cchTokens = static_cast<INT>(StringLength(pszTokens));
        if (cchTokens == 0)
        {
            if (cchStart < GetLength())
            {
                cstrResult = GetString() + cchStart;
            }
            else
            {
                //
                // There are no tokens or data
                //
                cchStart = -1;
            }
        }
        else
        {
            //
            // Skip leading tokens
            //
            while (Find(pszTokens, cchStart) == cchStart)
            {
                cchStart += cchTokens;
            }

            INT cchFirstToken = Find(pszTokens, cchStart);

            if (cchFirstToken > 0)
            {
                //
                // The result string runs from the start to the first token
                //
                cstrResult = Mid(cchStart, cchFirstToken - cchStart);
                cchStart = cchFirstToken + cchTokens;
            }
            else
            {
                if (cchStart < GetLength())
                {
                    //
                    // There are no more tokens but there is data left
                    //
                    cstrResult = GetString() + cchStart;
                    cchStart = GetLength() + cchTokens;
                }
                else
                {
                    //
                    // There are no more tokens or data
                    //
                    cchStart = -1;
                }
            }
        }

        return cstrResult;
    }

    /*++

    Routine Name:

        GetAt

    Routine Description:

        Retrieves character at the given index

    Arguments:

        cchAt - character count index of the character to return

    Return Value:

        The character value at the specified index

    --*/
    _T GetAt(
       _In_ INT cchAt
    ) CONST
    {
        if (cchAt < 0 ||
            cchAt > GetLength())
        {
            throw CXDException(E_INVALIDARG);
        }

        return GetString()[cchAt];
    }

    /*++

    Routine Name:

        Trim

    Routine Description:

        Trims the string of leading and trailing white space

    Arguments:

        None

    Return Value:

        Reference to the trimmed CStringXDT object

    --*/
    CStringXDT<_T>&
    Trim()
    {
        return TrimRight().TrimLeft();
    }

    /*++

    Routine Name:

        TrimRight

    Routine Description:

        Trims the string of trailing white space

    Arguments:

        None

    Return Value:

        Reference to the trimmed CStringXDT object

    --*/
    CStringXDT<_T>&
    TrimRight()
    {
        _T* pszFirstTrailing = NULL;
        _T* pszData = GetString();
        while (*pszData != 0)
        {
            if (IsSpace(*pszData))
            {
                if (pszFirstTrailing == NULL)
                {
                    pszFirstTrailing = pszData;
                }
            }
            else
            {
                pszFirstTrailing = NULL;
            }
            pszData++;
        }

        if (pszFirstTrailing != NULL)
        {
            Truncate(static_cast<INT>(pszFirstTrailing - GetString()));
        }

        return *this;
    }

    /*++

    Routine Name:

        Trim

    Routine Description:

        Trims the string of leading white space

    Arguments:

        None

    Return Value:

        Reference to the trimmed CStringXDT object

    --*/
    CStringXDT<_T>&
    TrimLeft()
    {
        _T* pszData = GetString();

        while (IsSpace(*pszData))
        {
            pszData++;
        }

        INT cchWhite = static_cast<INT>(pszData - GetString());
        if (cchWhite > 0)
        {
            Delete(0, cchWhite);
        }

        return *this;
    }

    /*++

    Routine Name:

        Left

    Routine Description:

        Returns a CStringXDT object containing the string of the specified length
        from the left of the string data

    Arguments:

        cchLeft - count of characters to return from the string data

    Return Value:

        The CStringXDT object containing the requested string

    --*/
    CStringXDT<_T>
    Left(
       _In_ INT cchLeft
    ) CONST
    {
        if (cchLeft < 0)
        {
            cchLeft = 0;
        }

        if (cchLeft > GetLength())
        {
            return *this;
        }

        return CStringXDT<_T>(GetString(), cchLeft);
    }

    /*++

    Routine Name:

        Mid

    Routine Description:

        Returns a CStringXDT object containing the string from the specified offset
        to the end of the string data

    Arguments:

        cchFirst - the index of the character to start the string

    Return Value:

        The CStringXDT object containing the requested string

    --*/
    CStringXDT<_T>
    Mid(
       _In_ INT cchFirst
    ) CONST
    {
        return Mid(cchFirst, GetLength() - cchFirst);
    }

    /*++

    Routine Name:

        Mid

    Routine Description:

        Returns a CStringXDT object containing the string from the specified offset
        of the requested length

    Arguments:

        cchFirst - the index of the character to start the string
        cchSize  - the count of characters to compose the return string from

    Return Value:

        The CStringXDT object containing the requested string

    --*/
    CStringXDT<_T>
    Mid(
       _In_ INT cchFirst,
       _In_ INT cchSize
    ) CONST
    {
        INT cchCurr = GetLength();

        if (cchFirst < 0 ||
            cchSize < 0 ||
            cchSize + cchFirst > cchCurr ||
            cchFirst > cchCurr)
        {
            throw CXDException(E_INVALIDARG);
        }

        if (cchFirst == 0 &&
            cchSize == cchCurr)
        {
            return *this;
        }

        return CStringXDT<_T>(GetString() + cchFirst, cchSize);
    }

    /*++

    Routine Name:

        MakeLower

    Routine Description:

        Makes all characters in the string lower case

    Arguments:

        None

    Return Value:

        Reference to the modified string object

    --*/
    CStringXDT<_T>&
    MakeLower()
    {
        MakeLower(GetString(), GetBufferCharCount());
        return *this;
    }

    /*++

    Routine Name:

        Truncate

    Routine Description:

        Truncates the string data to the specified size

    Arguments:

        cchNew - the length to which the string data is to be truncated

    Return Value:

        None

    --*/
    VOID
    Truncate(
        INT cchNew
        )
    {
        if (cchNew < 0)
        {
            cchNew = 0;
        }

        if (cchNew < GetLength())
        {
            GetString()[cchNew] = 0;
        }
    }

    /*++

    Routine Name:

        Preallocate

    Routine Description:

        Preallocate the string buffer to accomodate a string of length cChars
        Note: this function adds one for the NULL terminator

    Arguments:

        cChars - the count of chars to allocate for

    Return Value:

        None

    --*/
    VOID
    Preallocate(
       INT cChars
    )
    {
        if (cChars < 0)
        {
            cChars = 0;
        }

        SetBufferSize(static_cast<size_t>(cChars) + 1);
    }


private:
    /*++

    Routine Name:

        CreateString

    Routine Description:

        Creates a native string specifically from a WCHAR source string

    Arguments:

        pszSrc - the WCHAR source string

    Return Value:

        None

    --*/
    VOID
    CreateString(
        _In_z_ CONST WCHAR* pszSrc
        )
    {
        if (pszSrc != NULL)
        {
            CreateString(pszSrc, StringLength(pszSrc));
        }
    }

    /*++

    Routine Name:

        CreateString

    Routine Description:

        Creates a native string specifically from a CHAR source string

    Arguments:

        pszSrc - the CHAR source string

    Return Value:

        None

    --*/
    VOID
    CreateString(
        _In_z_ CONST CHAR* pszSrc
        )
    {
        if (pszSrc != NULL)
        {
            CreateString(pszSrc, StringLength(pszSrc));
        }
    }

    /*++

    Routine Name:

        CreateString

    Routine Description:

        Creates a snative tring from a native source string and character count

    Arguments:

        pszSrc - the source string
        cchSrc - count of characters to copy from the source string

    Return Value:

        None

    --*/
    VOID
    CreateString(
        _In_reads_(cchSrc) CONST _T*    pszSrc,
        _In_                size_t cchSrc
        )
    {
        if (pszSrc != NULL)
        {
            SetBufferSize(cchSrc);
            CopyString(GetString(), GetBufferCharCount(), pszSrc, cchSrc);
        }
    }

    /*++

    Routine Name:

        CreateStringFromY

    Routine Description:

        Creates a string from a source string of the opposite type.
        WCHAR specific implementation.

    Arguments:

        pszSrc - the source string
        cchSrc - count of characters to copy from the source string

    Return Value:

        None

    --*/
    VOID
    CreateStringFromY(
        _In_z_ CONST WCHAR* pszSrc
        )
    {
        if (pszSrc != NULL)
        {
            CreateStringFromY(pszSrc, StringLength(pszSrc));
        }
    }

    /*++

    Routine Name:

        CreateStringFromY

    Routine Description:

        Creates a string from a source string of the opposite type and character count.
        WCHAR specific implementation.

    Arguments:

        pszSrc - the source string
        cchSrc - count of characters to copy from the source string

    Return Value:

        None

    --*/
    VOID
    CreateStringFromY(
        _In_reads_(cchSrc) CONST WCHAR* pszSrc,
        _In_                size_t       cchSrc
        )
    {
        if (pszSrc != NULL &&
            cchSrc > 0)
        {
            SetBufferSize(cchSrc);
            CopyYString(GetString(), GetBufferCharCount(), pszSrc, cchSrc);
        }
    }

    /*++

    Routine Name:

        CreateStringFromY

    Routine Description:

        Creates a string from a source string of the opposite type and a chacter count.
        CHAR specific implementation.

    Arguments:

        pszSrc - the source string

    Return Value:

        None

    --*/
    VOID
    CreateStringFromY(
        _In_z_ CONST CHAR* pszSrc
        )
    {
        if (pszSrc != NULL)
        {
            CreateStringFromY(pszSrc, StringLength(pszSrc));
        }
    }

    /*++

    Routine Name:

        CreateStringFromY

    Routine Description:

        Creates a string from a source string of the opposite type.
        CHAR specific implementation.

    Arguments:

        pszSrc - the source string
        cchSrc - count of characters to copy from the source string

    Return Value:

        None

    --*/
    VOID
    CreateStringFromY(
        _In_reads_(cchSrc) CONST CHAR* pszSrc,
        _In_                size_t      cchSrc
        )
    {
        if (pszSrc != NULL &&
            cchSrc > 0)
        {
            SetBufferSize(cchSrc);
            CopyYString(GetString(), GetBufferCharCount(), pszSrc, cchSrc);
        }
    }

    /*++

    Routine Name:

        StringLength

    Routine Description:

        Returns the character count of the string. WCHAR specific.

    Arguments:

        pszSrc - source to string to retrieve the length of

    Return Value:

        The count of characters in the string

    --*/
    static size_t
    StringLength(
        _In_opt_z_ CONST WCHAR* pszSrc
        )
    {
        size_t cch = 0;
        if( pszSrc != NULL )
        {
            while( *pszSrc != 0 )
            {
                cch++;
                pszSrc++;
            }
        }

        return cch;
    }

    /*++

    Routine Name:

        StringLength

    Routine Description:

        Returns the character count of the string. CHAR specific.

    Arguments:

        pszSrc - source to string to retrieve the length of

    Return Value:

        The count of characters in the string

    --*/
    static size_t
    StringLength(
        _In_opt_z_ CONST CHAR* pszSrc
        )
    {
        size_t cch = 0;

        if( pszSrc != NULL )
        {
            while( *pszSrc != 0 )
            {
                cch++;
                pszSrc++;
            }
        }

        return cch;
    }

    /*++

    Routine Name:

        GetBufferCharCount

    Routine Description:

        Gets the total count of available characters in the buffer. This is the length
        of the buffer as opposed to the count of characters in the string (the buffer)
        can be larger.

    Arguments:

        None

    Return Value:

        The count of available characters in the data buffer

    --*/
    size_t
    GetBufferCharCount(
        VOID
        ) CONST
    {
        size_t cch = 0;

        if (m_pszData != NULL)
        {
            cch = *reinterpret_cast<size_t*>(reinterpret_cast<PBYTE>(m_pszData) - sizeof(size_t));
        }
        else
        {
            throw CXDException(E_PENDING);
        }

        return cch;
    }

    /*++

    Routine Name:

        GetDataBuffer

    Routine Description:

        Retrieves the pointer to the start of the data buffer. The size of the
        buffer is stored before the actual string data.

    Arguments:

        None

    Return Value:

        Pointer to the start of the data buffer

    --*/
    PVOID
    GetDataBuffer(
        VOID
        ) CONST
    {
        PVOID pData = NULL;

        if (m_pszData != NULL)
        {
            pData = reinterpret_cast<PVOID>(reinterpret_cast<PBYTE>(m_pszData) - sizeof(size_t));
        }
        else
        {
            throw CXDException(E_PENDING);
        }

        return pData;
    }

    /*++

    Routine Name:

        SetBufferSize

    Routine Description:

        Sets the size of the buffer according to the character count passed in. If
        the character count is less than that available no action is required. If it
        is larger the data buffer is doubled in size till it is sufficient.

    Arguments:

        cch - count of characters the buffer must accomodate

    Return Value:

        None

    --*/
    VOID
    SetBufferSize(
        size_t cch
        ) CONST
    {
        if (m_pszData == NULL)
        {
            //
            // Allocate a new buffer
            //
            size_t cchAllocate = CCH_INITIAL_BUFFER;

            //
            // Double the buffer size till we have enough room
            //
            while (cch >= cchAllocate)
            {
                cchAllocate *= 2;
            }

            LPVOID pData = HeapAlloc(GetProcessHeap(), 0, cchAllocate * sizeof(_T) + sizeof(size_t));

            if (pData != NULL)
            {
                *static_cast<size_t*>(pData) = cchAllocate;
                m_pszData = reinterpret_cast<_T*>(reinterpret_cast<PBYTE>(pData) + sizeof(size_t));
                *m_pszData = 0;
            }
            else
            {
                throw CXDException(E_OUTOFMEMORY);
            }
        }
        else
        {
            size_t cchAllocated = GetBufferCharCount();

            if (cch >= cchAllocated)
            {
                //
                // Double the buffer size till we have enough room
                //
                while (cch >= cchAllocated)
                {
                    cchAllocated *= 2;
                }

                //
                // Re-allocate the existing buffer
                //
                LPVOID pData = HeapReAlloc(GetProcessHeap(), 0, GetDataBuffer(), cchAllocated * sizeof(_T) + sizeof(size_t));

                if (pData != NULL)
                {
                    *static_cast<size_t*>(pData) = cchAllocated;
                    m_pszData = reinterpret_cast<_T*>(reinterpret_cast<PBYTE>(pData) + sizeof(size_t));
                }
                else
                {
                    throw CXDException(E_OUTOFMEMORY);
                }
            }
        }
    }

    /*++

    Routine Name:

        CopyString

    Routine Description:

        Copys a native string from one buffer to another. WCHAR specific implementation.

    Arguments:

        pszDst - pointer to the destination buffer to copy to
        cchDst - count of characters available in the destination buffer
        pszSrc - pointer to the source buffer
        cchSrc - count of characters to copy

    Return Value:

        None

    --*/
    static VOID
    CopyString(
        _Inout_updates_(cchDst) WCHAR*       pszDst,
        _In_                   size_t       cchDst,
        _In_reads_(cchSrc)    CONST WCHAR* pszSrc,
        _In_                   size_t       cchSrc
        )
    {
        HRESULT hr = S_OK;

        if (cchDst < cchSrc)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else if (pszDst == NULL)
        {
            hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr) &&
            pszSrc != NULL &&
            cchSrc > 0)
        {
            hr = StringCchCopyNW(pszDst, cchDst, pszSrc, cchSrc);
        }

        if (FAILED(hr))
        {
            throw CXDException(hr);
        }
    }

    /*++

    Routine Name:

        CopyString

    Routine Description:

        Copys a native string from one buffer to another. CHAR specific implementation.

    Arguments:

        pszDst - pointer to the destination buffer to copy to
        cchDst - count of characters available in the destination buffer
        pszSrc - pointer to the source buffer
        cchSrc - count of characters to copy

    Return Value:

        None

    --*/
    static VOID
    CopyString(
        _Inout_updates_(cchDst) CHAR*       pszDst,
        _In_                   size_t      cchDst,
        _In_reads_(cchSrc)    CONST CHAR* pszSrc,
        _In_                   size_t      cchSrc
        )
    {
        HRESULT hr = S_OK;

        if (cchDst < cchSrc)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else if (pszDst == NULL)
        {
            hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr) &&
            pszSrc != NULL &&
            cchSrc > 0)
        {
            hr = StringCchCopyNA(pszDst, cchDst, pszSrc, cchSrc);
        }

        if (FAILED(hr))
        {
            throw CXDException(hr);
        }
    }

    /*++

    Routine Name:

        CopyYString

    Routine Description:

        Copys a string of the opposite type from one buffer to another. CHAR specific implementation.

    Arguments:

        pszDst - pointer to the destination buffer to copy to
        cchDst - count of characters available in the destination buffer
        pszSrc - pointer to the source buffer
        cchSrc - count of characters to copy

    Return Value:

        None

    --*/
    static VOID
    CopyYString(
        _Inout_updates_(cchDst) CHAR*        pszDst,
        _In_                   size_t       cchDst,
        _In_reads_(cchSrc)    CONST WCHAR* pszSrc,
        _In_                   size_t       cchSrc
        )
    {
        HRESULT hr = S_OK;

        if (cchDst < cchSrc)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else if (pszDst == NULL)
        {
            hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr) &&
            pszSrc != NULL &&
            cchSrc > 0)
        {
            size_t cbWritten = WideCharToMultiByte(CP_ACP,
                                                   0,
                                                   pszSrc,
                                                   static_cast<INT>(cchSrc),
                                                   pszDst,
                                                   static_cast<INT>(cchDst * sizeof(_T)),
                                                   NULL,
                                                   NULL);

            if (cbWritten == cchSrc &&
                cchSrc < cchDst)
            {
                pszDst[cchSrc] = 0;
            }
            else
            {
                ERR("Failed to convert wide char to multibyte string\n");
                throw CXDException(E_FAIL);
            }
        }

        if (FAILED(hr))
        {
            throw CXDException(hr);
        }
    }

    /*++

    Routine Name:

        CopyYString

    Routine Description:

        Copys a string of the opposite type from one buffer to another. WCHAR specific implementation.

    Arguments:

        pszDst - pointer to the destination buffer to copy to
        cchDst - count of characters available in the destination buffer
        pszSrc - pointer to the source buffer
        cchSrc - count of characters to copy

    Return Value:

        None

    --*/
    static VOID
    CopyYString(
        _Inout_updates_(cchDst) WCHAR*      pszDst,
        _In_                   size_t      cchDst,
        _In_reads_(cchSrc)    CONST CHAR* pszSrc,
        _In_                   size_t      cchSrc
        )
    {
        HRESULT hr = S_OK;

        if (cchDst < cchSrc)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else if (pszDst == NULL)
        {
            hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr) &&
            pszSrc != NULL &&
            cchSrc > 0)
        {
            size_t cchWritten = MultiByteToWideChar(CP_ACP,
                                                    0,
                                                    pszSrc,
                                                    static_cast<INT>(cchSrc),
                                                    pszDst,
                                                    static_cast<INT>(cchDst));

            if (cchWritten == cchSrc &&
                cchSrc < cchDst)
            {
                pszDst[cchSrc] = 0;
            }
            else
            {
                ERR("Failed to convert multibyte to wide char string\n");
                throw CXDException(E_FAIL);
            }
        }

        if (FAILED(hr))
        {
            throw CXDException(hr);
        }
    }

    /*++

    Routine Name:

        CompareXDString

    Routine Description:

        Case sensitive comparison of a native string with another - WCHAR specific implementation.

    Arguments:

        psz    - first string in comparison
        pszCmp - second string in comparison

    Return Value:

        < 0 if the string data is less than the compare string
        0   if the strings are identical
        > 0 if the string data is more than the compare string

    --*/
    static INT
    CompareXDString(
        _In_z_ CONST WCHAR* psz,
        _In_z_ CONST WCHAR* pszCmp
        )
    {
        if (pszCmp == NULL)
        {
            throw CXDException(E_FAIL);
        }

        return wcscmp(psz, pszCmp);
    }

    /*++

    Routine Name:

        CompareXDString

    Routine Description:

        Case sensitive comparison of a native string with another - CHAR specific implementation.

    Arguments:

        psz    - first string in comparison
        pszCmp - second string in comparison

    Return Value:

        < 0 if the string data is less than the compare string
        0   if the strings are identical
        > 0 if the string data is more than the compare string

    --*/
    static INT
    CompareXDString(
        _In_z_ CONST CHAR* psz,
        _In_z_ CONST CHAR* pszCmp
        )
    {
        if (pszCmp == NULL)
        {
            throw CXDException(E_FAIL);
        }

        return strcmp(psz, pszCmp);
    }

    /*++

    Routine Name:

        CompareXDString

    Routine Description:

        Case insensitive comparison of a native string with another - WCHAR specific implementation.

    Arguments:

        psz    - first string in comparison
        pszCmp - second string in comparison

    Return Value:

        < 0 if the string data is less than the compare string
        0   if the strings are identical
        > 0 if the string data is more than the compare string

    --*/
    static INT
    CompareXDStringNoCase(
        _In_z_ CONST WCHAR* psz,
        _In_z_ CONST WCHAR* pszCmp
        )
    {
        if (pszCmp == NULL)
        {
            throw CXDException(E_FAIL);
        }

        return _wcsicmp(psz, pszCmp);
    }

    /*++

    Routine Name:

        CompareXDString

    Routine Description:

        Case insensitive comparison of a native string with another - CHAR specific implementation.

    Arguments:

        psz    - first string in comparison
        pszCmp - second string in comparison

    Return Value:

        < 0 if the string data is less than the compare string
        0   if the strings are identical
        > 0 if the string data is more than the compare string

    --*/
    static INT
    CompareXDStringNoCase(
        _In_z_ CONST CHAR* psz,
        _In_z_ CONST CHAR* pszCmp
        )
    {
        if (pszCmp == NULL)
        {
            throw CXDException(E_FAIL);
        }

        return _stricmp(psz, pszCmp);
    }

    /*++

    Routine Name:

        AllocSysString

    Routine Description:

        Allocates a system string from the string data. WCHAR specific implementation.

    Arguments:

        psz - The string to allocate from

    Return Value:

        The newly allocated BSTR

    --*/
    static BSTR
    AllocSysString(
        _In_z_ CONST WCHAR* psz
        )
    {
        BSTR bstr = ::SysAllocString(psz);

        if (bstr == NULL)
        {
            throw CXDException(E_OUTOFMEMORY);
        }

        return bstr;
    }

    /*++

    Routine Name:

        AllocSysString

    Routine Description:

        Allocates a system string from the string data. CHAR specific implementation.

    Arguments:

        psz - The string to allocate from

    Return Value:

        The newly allocated BSTR

    --*/
    static BSTR
    AllocSysString(
        _In_z_ CONST CHAR* psz
        )
    {
        CStringXDW cstrWide(psz);
        BSTR bstr = ::SysAllocString(cstrWide);

        if (bstr == NULL)
        {
            throw CXDException(E_OUTOFMEMORY);
        }

        return bstr;
    }

    /*++

    Routine Name:

        Find

    Routine Description:

        Retrieves the location of a sub string in another string. This is the
        CHAR spzecific implementation for native string type

    Arguments:

        pszSrc   - string to search in
        pszSub   - sub string to search for
        cchStart - starting point for the search

    Return Value:

        < 0 if the substring was not found
        Otherwise, the location of the substring

    --*/
    static INT
    Find(
        _In_z_ CONST CHAR* pszSrc,
        _In_z_ CONST CHAR* pszSub,
        _In_   INT         cchStart = 0
        ) throw()
    {
        INT    cchIndex = -1;
        size_t cchLen = StringLength(pszSrc);

        if (pszSrc != NULL &&
            pszSub != NULL &&
            cchStart >= 0 &&
            cchStart <= static_cast<INT>(cchLen))
        {
            CHAR* psz = strstr(const_cast<CHAR*>(pszSrc + cchStart), pszSub);

            if (psz != NULL)
            {
                cchIndex = static_cast<INT>(psz - pszSrc);
            }
        }

        return cchIndex;
    }

    /*++

    Routine Name:

        Find

    Routine Description:

        Retrieves the location of a sub string in another string. This is the
        WCHAR spzecific implementation for native string type

    Arguments:

        pszSrc   - string to search in
        pszSub   - sub string to search for
        cchStart - starting point for the search

    Return Value:

        < 0 if the substring was not found
        Otherwise, the location of the substring

    --*/
    static INT
    Find(
        _In_z_ CONST WCHAR* pszSrc,
        _In_z_ CONST WCHAR* pszSub,
        _In_   INT          cchStart = 0
        ) throw()
    {
        INT    cchIndex = -1;
        size_t cchLen = StringLength(pszSrc);

        if (pszSrc != NULL &&
            pszSub != NULL &&
            cchStart >= 0 &&
            cchStart <= static_cast<INT>(cchLen))
        {
            WCHAR* psz = wcsstr(const_cast<WCHAR*>(pszSrc + cchStart), pszSub);

            if (psz != NULL)
            {
                cchIndex = static_cast<INT>(psz - pszSrc);
            }
        }

        return cchIndex;
    }

    /*++

    Routine Name:

        Format

    Routine Description:

        Writes formatted data to the string. WCHAR specific implementation.

    Arguments:

        pszFormat - the format string
        argList   - Variable argument list

    Return Value:

        None

    --*/
    VOID
    Format(
        _In_z_ CONST WCHAR* pszFormat,
               va_list     argList
        )
    {
        INT cchFormatedLen = _vscwprintf(pszFormat, argList);

        if (cchFormatedLen >= 0)
        {
            SetBufferSize(cchFormatedLen);
            vswprintf_s(GetString(), GetBufferCharCount(), pszFormat, argList);
        }
        else
        {
            throw CXDException(E_INVALIDARG);
        }
    }

    /*++

    Routine Name:

        Format

    Routine Description:

        Writes formatted data to the string. CHAR specific implementation.

    Arguments:

        pszFormat - the format string
        argList   - Variable argument list

    Return Value:

        None

    --*/
    VOID
    Format(
        _In_z_ CONST CHAR* pszFormat,
               va_list     argList
        )
    {
        INT cchFormatedLen = _vscprintf(pszFormat, argList);

        if (cchFormatedLen >= 0)
        {
            SetBufferSize(cchFormatedLen);
            vsprintf_s(GetString(), GetBufferCharCount(), pszFormat, argList);
        }
        else
        {
            throw CXDException(E_INVALIDARG);
        }
    }

    /*++

    Routine Name:

        IsSpace

    Routine Description:

        Determinese if a character is white space character. WCHAR specific implementation.

    Arguments:

        szCandidate - candidate character value

    Return Value:

        true  - if the character is a white space character
        false - otherwise

    --*/
    static bool
    IsSpace(
        _In_ CONST WCHAR& szCandidate
        )
    {
        return iswspace(szCandidate) != 0;
    }

    /*++

    Routine Name:

        IsSpace

    Routine Description:

        Determinese if a character is white space character. CHAR specific implementation.

    Arguments:

        szCandidate - candidate character value

    Return Value:

        true  - if the character is a white space character
        false - otherwise

    --*/
    static bool
    IsSpace(
        _In_z_ CONST CHAR& szCandidate
        )
    {
        return isspace(szCandidate) != 0;
    }

    /*++

    Routine Name:

        MakeLower

    Routine Description:

        Converts all characters in the source string to lower case characters.
        WCHAR specific implementation.

    Arguments:

        pszSrc - string to be converted
        cchSrc - character count of the string

    Return Value:

        NULL if the conversion fails
        Pointer to the converted string on success

    --*/
    static PWSTR
    MakeLower(
        PWSTR  pszSrc,
        size_t cchSrc
        )
    {
        PWSTR pszRet = NULL;
        if (_wcslwr_s(pszSrc, cchSrc) == 0)
        {
            pszRet = pszSrc;
        }

        return pszRet;
    }

    /*++

    Routine Name:

        MakeLower

    Routine Description:

        Converts all characters in the source string to lower case characters.
        CHAR specific implementation.

    Arguments:

        pszSrc - string to be converted
        cchSrc - character count of the string

    Return Value:

        NULL if the conversion fails
        Pointer to the converted string on success

    --*/
    static PSTR
    MakeLower(
        PSTR   pszSrc,
        size_t cchSrc
        )
    {
        PSTR pszRet = NULL;
        if (_strlwr_s(pszSrc, cchSrc) == 0)
        {
            pszRet = pszSrc;
        }

        return pszRet;
    }

    /*++

    Routine Name:

        GetString

    Routine Description:

        Retrieves the underlying string.

    Arguments:

        None

    Return Value:

        The underlying string

    --*/
    _T*&
    GetString(
        VOID
        ) CONST
    {
        if (m_pszData == NULL)
        {
            SetBufferSize(1);
        }

        return m_pszData;
    }

private:
    mutable _T* m_pszData;
};

typedef CStringXDT<TCHAR> CStringXD;
typedef CStringXDT<WCHAR> CStringXDW;
typedef CStringXDT<CHAR>  CStringXDA;

