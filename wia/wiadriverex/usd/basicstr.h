/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  Copyright (c) 2003 Microsoft Corporation.  All Rights Reserved.
*
*  DESCRIPTION: Simple string classes
*
*******************************************************************************/
#ifndef _SIMSTR_H_INCLUDED
#define _SIMSTR_H_INCLUDED

/*
* Simple string class.
*
* Template class:
*   CBasicStringBase<CharType>
* Implementations:
*   CBasicStringBase<wchar_t> CBasicStringWide
*   CBasicStringBase<char> CBasicStringAnsi
*   CBasicString = CBasicString[Ansi|Wide] depending on UNICODE macro
* Inline functions:
*   CBasicStringAnsi CBasicStringConvert::AnsiString(CharType n)
*   CBasicStringWide CBasicStringConvert::WideString(CharType n)
* Macros:
*   IS_CHAR(CharType)
*   IS_WCHAR(CharType)
*/

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

//
// Disable the "conditional expression is constant" warning that is caused by
// the IS_CHAR and IS_WCHAR macros
//
#pragma warning( push )
#pragma warning( disable : 4127 )

#define IS_CHAR(x)     (sizeof(x) & sizeof(char))
#define IS_WCHAR(x)    (sizeof(x) & sizeof(wchar_t))

#ifndef ARRAYSIZE
    #define ARRAYSIZE(x)   (sizeof(x) / sizeof(x[0]))
#endif

template <class CharType>
class CBasicStringBase
{
private:
    enum
    {
        c_nDefaultGranularity      = 16,   // Default number of extra characters to allocate when we have to grow
        c_nInitialLoadStringBuffer = 1024, // Initial length of .RC string
        c_nMaxAutoDataLength       = 128   // Length of non-dynamically allocated string
    };

private:
    //
    // If the string is less than c_nMaxAutoDataLength characters, it will be
    // stored here, instead of in a dynamically allocated buffer
    //
    CharType m_pstrAutoData[c_nMaxAutoDataLength];

    //
    // If we have to allocate data, it will be stored here
    //
    CharType *m_pstrData;

    //
    // Current maximum buffer size
    //
    size_t m_nMaxSize;

    //
    // Amount of extra space we allocate when we have to grow the buffer
    //
    size_t m_nGranularity;

    //
    // Error flag.  This is set if an allocation fails.
    //
    bool m_bError;

private:

    //
    // Min, in case it isn't already defined
    //
    template <class NumberType>
    static NumberType Min( const NumberType &a, const NumberType &b )
    {
        return (a < b) ? a : b;
    }

private:

    //
    // Replacements (in some cases just wrappers) for strlen, strcpy, ...
    //
    static inline CharType   *GenericCopyLength( CharType *pstrTarget, const CharType *pstrSource, size_t nSize );
    static inline size_t      GenericLength( const CharType *pstrStr );
    static inline CharType   *GenericConcatenate( CharType *pstrTarget, const CharType *pstrSource );
    static inline int         GenericCompare( const CharType *pstrTarget, const CharType *pstrSource );
    static inline int         GenericCompareNoCase( const CharType *pstrStrA, const CharType *pstrStrB );
    static inline int         GenericCompareLength( const CharType *pstrTarget, const CharType *pstrSource, size_t cchLength );
    static inline LPSTR       GenericCharNext( LPCSTR );
    static inline LPWSTR      GenericCharNext( LPCWSTR );

private:
    //
    // Internal only helpers
    //
    bool EnsureLength( size_t nMaxSize );
    void DeleteStorage();
    CharType *CreateStorage( size_t nCount, size_t &nAllocated );
    void Destroy();

public:
    //
    // Constructors and destructor
    //
    CBasicStringBase();
    CBasicStringBase( const CBasicStringBase & );
    CBasicStringBase( const CharType *szStr );
    CBasicStringBase( CharType ch );
    CBasicStringBase( UINT nResId, HMODULE hModule );
    virtual ~CBasicStringBase();

    //
    // String state
    //
    bool OK() const
    {
        return (!Error() && String());
    }
    bool IsValid() const
    {
        return (String() != NULL);
    }
    bool Error() const
    {
        return m_bError;
    }
    HRESULT Status() const
    {
        return OK() ? S_OK : E_OUTOFMEMORY;
    }
    void ClearError()
    {
        m_bError = false;
    }

private:
    void PropagateError( const CBasicStringBase &other )
    {
        if (!Error() && other.Error())
        {
            m_bError = true;
        }
    }

public:
#if defined(SIMSTR_UNIT_TEST)
    bool m_bForceError;
    void ForceError( bool bForceError ) { m_bForceError = bForceError; }
#endif

    //
    // Various helpers
    //
    size_t Length() const;
    void Concat( const CBasicStringBase &other );
    bool Assign( const CharType *szStr );
    bool Assign( const CBasicStringBase & );
    void SetAt( size_t nIndex, CharType chValue );
    CharType GetAt( size_t nIndex ) const;
    CharType &operator[](int index);
    const CharType &operator[](int index) const;

    //
    // Handy Win32 wrappers
    //
    CBasicStringBase &Format( const CharType *strFmt, ... );
    CBasicStringBase &Format( int nResId, HINSTANCE hInst, ... );
    bool LoadString( UINT nResId, HMODULE hModule );

    //
    // Operators
    //
    CBasicStringBase &operator=( const CBasicStringBase &other );
    CBasicStringBase &operator=( const CharType *other );
    CBasicStringBase &operator+=( const CBasicStringBase &other );


    //
    // Convert this string and return the converted string
    //
    CBasicStringBase ToUpper() const;
    CBasicStringBase ToLower() const;

    //
    // Convert in place
    //
    CBasicStringBase &MakeUpper();
    CBasicStringBase &MakeLower();

    //
    // Remove leading and trailing spaces
    //
    CBasicStringBase &TrimRight();
    CBasicStringBase &TrimLeft();
    CBasicStringBase &Trim();

    //
    // Searching
    //
    int Find( CharType cChar ) const;
    int Find( const CBasicStringBase &other, size_t nStart=0 ) const;
    int ReverseFind( CharType cChar ) const;
    int ReverseFind( const CBasicStringBase &other ) const;

    //
    // Substring copies
    //
    CBasicStringBase SubStr( size_t nStart, size_t nCount ) const;
    CBasicStringBase SubStr( size_t nStart ) const;

    CBasicStringBase Left( size_t nCount ) const
    {
        return SubStr( 0, (int)nCount );
    }
    CBasicStringBase Right( size_t nCount ) const
    {
        return SubStr( Length()-nCount );
    }

    //
    // Comparison functions
    //
    int CompareNoCase( const CBasicStringBase &other, int cchLength=-1 ) const;
    int Compare( const CBasicStringBase &other, int cchLength=-1 ) const;

    //
    // Direct manipulation
    //
    CharType *GetBuffer( size_t cchLength )
    {
        //
        // If we are able to allocate a string of the
        // requested length, return a pointer to the actual data.
        //
        return EnsureLength(cchLength+1) ? m_pstrData : NULL;
    }

    //
    // Useful inlines
    //
    const CharType *String() const
    {
        return m_pstrData;
    }
    size_t MaxSize() const
    {
        return m_nMaxSize;
    }
    size_t Granularity( size_t nGranularity )
    {
        if (nGranularity>0)
        {
            m_nGranularity = nGranularity;
        }
        return m_nGranularity;
    }
    size_t Granularity() const
    {
        return m_nGranularity;
    }

    //
    // Implicit cast operator
    //
    operator const CharType *() const
    {
        return String();
    }

    friend class CBasicStringBase;
};

template <>
inline LPSTR CBasicStringBase<CHAR>::GenericCharNext( LPCSTR pszStr )
{
    if (!pszStr)
    {
        return NULL;
    }
    return CharNextA(pszStr);
}

template <>
inline LPWSTR CBasicStringBase<WCHAR>::GenericCharNext( LPCWSTR pszStr )
{
    if (!pszStr)
    {
        return NULL;
    }
    return CharNextW(pszStr);
}

template <class CharType>
inline CharType *CBasicStringBase<CharType>::GenericCopyLength( CharType *pszTarget, const CharType *pszSource, size_t nCount )
{
    if (!pszTarget || !pszSource)
    {
        return NULL;
    }

    size_t nCopyLen = min( nCount, GenericLength(pszSource) + 1 );
    if (0 == nCopyLen)
    {
        return pszTarget;
    }

    CopyMemory( pszTarget, pszSource, nCopyLen * sizeof(CharType) );
    pszTarget[nCopyLen-1] = 0;
    return pszTarget;
}

template <>
inline size_t CBasicStringBase<CHAR>::GenericLength( LPCSTR pszString )
{
    if (!pszString)
    {
        return 0;
    }

    size_t nSize = 0;
    if (S_OK != StringCchLengthA( pszString, STRSAFE_MAX_CCH, &nSize ))
    {
        return 0;
    }

    return nSize;
}

template <>
inline size_t CBasicStringBase<WCHAR>::GenericLength( LPCWSTR pszString )
{
    if (!pszString)
    {
        return 0;
    }

    size_t nSize = 0;
    if (S_OK != StringCchLengthW( pszString, STRSAFE_MAX_CCH, &nSize ))
    {
        return 0;
    }

    return nSize;
}

template <class CharType>
inline CharType*CBasicStringBase<CharType>::GenericConcatenate( CharType *pszTarget, const CharType *pszSource )
{
    if (!pszTarget || !pszSource)
    {
        return NULL;
    }

    CharType *pszCurr = pszTarget;

    while (*pszCurr)
    {
        pszCurr++;
    }

    CopyMemory( pszCurr, pszSource, sizeof(CharType) * (GenericLength(pszSource) + 1) );

    return pszTarget;
}


template <class CharType>
inline int CBasicStringBase<CharType>::GenericCompare( const CharType *pszSource, const CharType *pszTarget )
{
#if defined(DBG) && !defined(UNICODE) && !defined(_UNICODE)
    if (sizeof(CharType) == sizeof(wchar_t))
    {
        OutputDebugString(TEXT("CompareStringW is not supported under win9x, so this call is going to fail!"));
    }
#endif
    int nRes = IS_CHAR(*pszSource) ?
               CompareStringA( LOCALE_USER_DEFAULT, 0, (LPCSTR)pszSource, -1, (LPCSTR)pszTarget, -1 ) :
               CompareStringW( LOCALE_USER_DEFAULT, 0, (LPCWSTR)pszSource, -1, (LPCWSTR)pszTarget, -1 );
    switch (nRes)
    {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
}



template <class CharType>
inline int CBasicStringBase<CharType>::GenericCompareNoCase( const CharType *pszSource, const CharType *pszTarget )
{
#if defined(DBG) && !defined(UNICODE) && !defined(_UNICODE)
    if (sizeof(CharType) == sizeof(wchar_t))
    {
        OutputDebugString(TEXT("CompareStringW is not supported under win9x, so this call is going to fail!"));
    }
#endif
    int nRes = IS_CHAR(*pszSource) ?
               CompareStringA( LOCALE_USER_DEFAULT, NORM_IGNORECASE, (LPCSTR)pszSource, -1, (LPCSTR)pszTarget, -1 ) :
               CompareStringW( LOCALE_USER_DEFAULT, NORM_IGNORECASE, (LPCWSTR)pszSource, -1, (LPCWSTR)pszTarget, -1 );
    switch (nRes)
    {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
}

template <class CharType>
inline int CBasicStringBase<CharType>::GenericCompareLength( const CharType *pszStringA, const CharType *pszStringB, size_t cchLength )
{
#if defined(DBG) && !defined(UNICODE) && !defined(_UNICODE)
    if (sizeof(CharType) == sizeof(wchar_t))
    {
        OutputDebugString(TEXT("CompareStringW is not supported under win9x, so this call is going to fail!"));
    }
#endif
    if (!cchLength)
        return(0);
    int nRes = IS_CHAR(*pszStringA) ?
               CompareStringA( LOCALE_USER_DEFAULT, 0, (LPCSTR)pszStringA, (int)Min(cchLength,CBasicStringBase<CHAR>::GenericLength((LPCSTR)pszStringA)), (LPCSTR)pszStringB, (int)Min(cchLength,CBasicStringBase<CHAR>::GenericLength((LPCSTR)pszStringB)) ) :
               CompareStringW( LOCALE_USER_DEFAULT, 0, (LPWSTR)pszStringA, (int)Min(cchLength,CBasicStringBase<WCHAR>::GenericLength((LPCWSTR)pszStringA)), (LPCWSTR)pszStringB, (int)Min(cchLength,CBasicStringBase<WCHAR>::GenericLength((LPCWSTR)pszStringB)) );
    switch (nRes)
    {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
}

template <class CharType>
bool CBasicStringBase<CharType>::EnsureLength( size_t nMaxSize )
{
    //
    // If the string is already long enough, just return true
    //
    if (m_nMaxSize >= nMaxSize)
    {
        return true;
    }

    // Get the new size
    //
    size_t nNewMaxSize = nMaxSize + m_nGranularity;

    //
    // Allocate the new buffer
    //
    size_t nAllocated = 0;
    CharType *pszTmp = CreateStorage(nNewMaxSize,nAllocated);

    //
    // Make sure the allocation succeeded
    //
    if (pszTmp)
    {
        //
        // If we have an existing string, copy it and delete it
        //
        if (m_pstrData)
        {
            GenericCopyLength( pszTmp, m_pstrData, Length()+1 );
            DeleteStorage();
        }

        //
        // Save the new max size
        //
        m_nMaxSize = nAllocated;

        //
        // Save this new string
        //
        m_pstrData = pszTmp;

        //
        // Return success
        //
        return true;
    }

    //
    // Couldn't allocate memory
    //
    return false;
}

template <class CharType>
CBasicStringBase<CharType>::CBasicStringBase()
  : m_pstrData(m_pstrAutoData),
    m_nMaxSize(ARRAYSIZE(m_pstrAutoData)),
    m_nGranularity(c_nDefaultGranularity),
    m_bError(false)
{
#if defined(SIMSTR_UNIT_TEST)
    m_bForceError = false;
#endif
    m_pstrAutoData[0] = 0;
}

template <class CharType>
CBasicStringBase<CharType>::CBasicStringBase( const CBasicStringBase &other )
  : m_pstrData(m_pstrAutoData),
    m_nMaxSize(ARRAYSIZE(m_pstrAutoData)),
    m_nGranularity(c_nDefaultGranularity),
    m_bError(false)
{
#if defined(SIMSTR_UNIT_TEST)
    m_bForceError = false;
#endif
    m_pstrAutoData[0] = 0;
    Assign(other);
}

template <class CharType>
CBasicStringBase<CharType>::CBasicStringBase( const CharType *szStr )
  : m_pstrData(m_pstrAutoData),
    m_nMaxSize(ARRAYSIZE(m_pstrAutoData)),
    m_nGranularity(c_nDefaultGranularity),
    m_bError(false)
{
#if defined(SIMSTR_UNIT_TEST)
    m_bForceError = false;
#endif
    m_pstrAutoData[0] = 0;
    Assign(szStr);
}

template <class CharType>
CBasicStringBase<CharType>::CBasicStringBase( CharType ch )
  : m_pstrData(m_pstrAutoData),
    m_nMaxSize(ARRAYSIZE(m_pstrAutoData)),
    m_nGranularity(c_nDefaultGranularity),
    m_bError(false)
{
#if defined(SIMSTR_UNIT_TEST)
    m_bForceError = false;
#endif
    m_pstrAutoData[0] = 0;
    CharType szTmp[2];
    szTmp[0] = ch;
    szTmp[1] = 0;
    Assign(szTmp);
}


template <class CharType>
CBasicStringBase<CharType>::CBasicStringBase( UINT nResId, HMODULE hModule )
  : m_pstrData(m_pstrAutoData),
    m_nMaxSize(ARRAYSIZE(m_pstrAutoData)),
    m_nGranularity(c_nDefaultGranularity),
    m_bError(false)
{
#if defined(SIMSTR_UNIT_TEST)
    m_bForceError = false;
#endif
    m_pstrAutoData[0] = 0;
    LoadString( nResId, hModule );
}

template <>
inline CBasicStringBase<WCHAR> &CBasicStringBase<WCHAR>::Format( const WCHAR *strFmt, ... )
{
    //
    // Initialize the string
    //
    Assign(NULL);

    //
    // Prepare the argument list
    //
    va_list ArgList;
    va_start( ArgList, strFmt );

    //
    // How many characters do we need?
    //
    int cchLength = _vscwprintf( strFmt, ArgList );

    //
    // Make sure we have a valid length
    //
    if (cchLength >= 0)
    {
        //
        // Get a pointer to the buffer
        //
        LPWSTR pszBuffer = GetBuffer(cchLength + 1);
        if (pszBuffer)
        {
            //
            // Print the string
            //
            StringCchVPrintfW( pszBuffer, cchLength + 1, strFmt, ArgList );
        }
    }

    //
    // Done with the argument list
    //
    va_end( ArgList );
    return *this;
}

template <>
inline CBasicStringBase<CHAR> &CBasicStringBase<CHAR>::Format( const CHAR *strFmt, ... )
{
    //
    // Initialize the string
    //
    Assign(NULL);

    //
    // Prepare the argument list
    //
    va_list ArgList;
    va_start( ArgList, strFmt );

    //
    // How many characters do we need?
    //
    int cchLength = _vscprintf( strFmt, ArgList );

    //
    // Make sure we have a valid length
    //
    if (cchLength >= 0)
    {
        //
        // Get a pointer to the buffer
        //
        LPSTR pszBuffer = GetBuffer(cchLength + 1);
        if (pszBuffer)
        {
            //
            // Print the string
            //
            StringCchVPrintfA( pszBuffer, cchLength + 1, strFmt, ArgList );
        }
    }

    //
    // Done with the argument list
    //
    va_end( ArgList );
    return *this;
}

template <>
inline CBasicStringBase<WCHAR> &CBasicStringBase<WCHAR>::Format( int nResId, HINSTANCE hInst, ... )
{
    //
    // Initialize the string
    //
    Assign(NULL);

    //
    // Load the format string
    //
    CBasicStringBase<WCHAR> strFmt;
    if (strFmt.LoadString( nResId, hInst ))
    {
        //
        // Prepare the argument list
        //
        va_list ArgList;
        va_start( ArgList, hInst );

        //
        // How many characters do we need?
        //
        int cchLength = _vscwprintf( strFmt, ArgList );

        //
        // Make sure we have a valid length
        //
        if (cchLength >= 0)
        {
            //
            // Get a pointer to the buffer
            //
            LPWSTR pszBuffer = GetBuffer(cchLength + 1);
            if (pszBuffer)
            {
                //
                // Print the string
                //
                StringCchVPrintfW( pszBuffer, cchLength + 1, strFmt, ArgList );
            }
        }

        //
        // Done with the argument list
        //
        va_end( ArgList );
    }
    return *this;
}

template <>
inline CBasicStringBase<CHAR> &CBasicStringBase<CHAR>::Format( int nResId, HINSTANCE hInst, ... )
{
    //
    // Initialize the string
    //
    Assign(NULL);

    //
    // Load the format string
    //
    CBasicStringBase<CHAR> strFmt;
    if (strFmt.LoadString(nResId,hInst))
    {
        //
        // Prepare the argument list
        //
        va_list ArgList;
        va_start( ArgList, hInst );

        //
        // How many characters do we need?
        //
        int cchLength = _vscprintf( strFmt, ArgList );

        //
        // Make sure we have a valid length
        //
        if (cchLength >= 0)
        {
            //
            // Get a pointer to the buffer
            //
            LPSTR pszBuffer = GetBuffer(cchLength + 1);
            if (pszBuffer)
            {
                //
                // Print the string
                //
                StringCchVPrintfA( pszBuffer, cchLength + 1, strFmt, ArgList );
            }
        }

        //
        // Done with the argument list
        //
        va_end(ArgList);
    }
    return *this;
}


template <>
inline bool CBasicStringBase<CHAR>::LoadString( UINT nResId, HMODULE hModule )
{
    //
    // Assume failure
    //
    bool bResult = false;

    //
    // Initialize the current string
    //
    Assign(NULL);

    //
    // If no hmodule was provided, use the current EXE's
    //
    if (!hModule)
    {
        hModule = GetModuleHandle(NULL);
    }

    //
    // Loop through, doubling the size of the string, until we get to 64K
    //
    for (int nSize = c_nInitialLoadStringBuffer;nSize < 0x0000FFFF; nSize <<= 1 )
    {
        //
        // Get a buffer to hold the string
        //
        LPSTR pszBuffer = GetBuffer(nSize);

        //
        // If we can't get a buffer, exit the loop
        //
        if (!pszBuffer)
        {
            break;
        }

        //
        // Make sure the string is NULL terminated.
        //
        *pszBuffer = '\0';

        //
        // Attempt to load the string
        //
        #pragma prefast(suppress:__WARNING_ANSI_APICALL, "Replace with LoadStringW if using for WCHAR; this instance is for CHAR; see CBasicStringBase<WCHAR>::LoadString below"
        int nResult = ::LoadStringA( hModule, nResId, pszBuffer, nSize );

        //
        // If the buffer was long enough, exit the loop, and set the success flag
        //
        if (nResult < (nSize - 1))
        {
            bResult = true;
            break;
        }

        //
        // If it was unsuccessful, exit the loop
        //
        if (!nResult)
        {
            break;
        }
    }
    return bResult;
}

template <>
inline bool CBasicStringBase<WCHAR>::LoadString( UINT nResId, HMODULE hModule )
{
    //
    // Assume failure
    //
    bool bResult = false;

    //
    // Initialize the current string
    //
    Assign(NULL);

    //
    // If no hmodule was provided, use the current EXE's
    //
    if (!hModule)
    {
        hModule = GetModuleHandle(NULL);
    }

    //
    // Loop through, doubling the size of the string, until we get to 64K
    //
    for (int nSize = c_nInitialLoadStringBuffer;nSize < 0x0000FFFF; nSize <<= 1 )
    {
        //
        // Get a buffer to hold the string
        //
        LPWSTR pszBuffer = GetBuffer(nSize);

        //
        // If we can't get a buffer, exit the loop
        //
        if (!pszBuffer)
        {
            break;
        }

        //
        // Make sure the string is NULL terminated.
        //
        *pszBuffer = L'\0';

        //
        // Attempt to load the string
        //
        int nResult = ::LoadStringW( hModule, nResId, pszBuffer, nSize );

        //
        // If the buffer was long enough, exit the loop, and set the success flag
        //
        if (nResult < (nSize - 1))
        {
            bResult = true;
            break;
        }

        //
        // If it was unsuccessful, exit the loop
        //
        if (!nResult)
        {
            break;
        }
    }
    return bResult;
}


template <class CharType>
CBasicStringBase<CharType>::~CBasicStringBase()
{
    Destroy();
}

template <class CharType>
void CBasicStringBase<CharType>::DeleteStorage()
{
    //
    // Only delete the string if it is non-NULL and not pointing to our non-dynamically allocated buffer
    //
    if (m_pstrData && m_pstrData != m_pstrAutoData)
    {
        delete[] m_pstrData;
    }
    m_pstrData = NULL;
}

template <class CharType>
CharType *CBasicStringBase<CharType>::CreateStorage( size_t nCount, size_t &nAllocated )
{
#if defined(SIMSTR_UNIT_TEST)
    if (m_bForceError)
    {
        m_bError = true;
        return NULL;
    }
#endif

    CharType *pResult = NULL;
    nAllocated = 0;

    //
    // If we are currently pointing to our fixed buffer, or the requested
    // size is greater than our fixed-length buffer, allocate using new.
    // Otherwise, return the address of our fixed-length buffer.
    //
    if (m_pstrData == m_pstrAutoData || nCount > ARRAYSIZE(m_pstrAutoData))
    {
        pResult = new CharType[nCount];
        if (pResult)
        {
            nAllocated = nCount;
        }
    }
    else
    {
        pResult = m_pstrAutoData;
        nAllocated = ARRAYSIZE(m_pstrAutoData);
    }
    if (!pResult)
    {
        m_bError = true;
    }

    return pResult;
}

template <class CharType>
void CBasicStringBase<CharType>::Destroy()
{
    DeleteStorage();
    m_nMaxSize = 0;
}

template <class CharType>
size_t CBasicStringBase<CharType>::Length() const
{
    return(m_pstrData ? GenericLength(m_pstrData) : 0);
}

template <class CharType>
CBasicStringBase<CharType> &CBasicStringBase<CharType>::operator=( const CBasicStringBase &other )
{
    if (&other != this)
    {
        Assign(other);
    }
    return *this;
}

template <class CharType>
CBasicStringBase<CharType> &CBasicStringBase<CharType>::operator=( const CharType *other )
{
    if (other != String())
    {
        Assign(other);
    }
    return *this;
}

template <class CharType>
CBasicStringBase<CharType> &CBasicStringBase<CharType>::operator+=( const CBasicStringBase &other )
{
    Concat(other.String());

    return *this;
}

template <class CharType>
bool CBasicStringBase<CharType>::Assign( const CharType *szStr )
{
    if (szStr && EnsureLength(GenericLength(szStr)+1))
    {
        GenericCopyLength(m_pstrData,szStr,GenericLength(szStr)+1);
    }
    else if (EnsureLength(1))
    {
        *m_pstrData = 0;
    }
    else Destroy();
    return OK();
}

template <class CharType>
bool CBasicStringBase<CharType>::Assign( const CBasicStringBase &other )
{
    Assign( other.String() );

    PropagateError( other );

    return OK();
}

template <class CharType>
void CBasicStringBase<CharType>::SetAt( size_t nIndex, CharType chValue )
{
    //
    // Make sure we don't go off the end of the string or overwrite the '\0'
    //
    if (Length() > nIndex)
    {
        m_pstrData[nIndex] = chValue;
    }
}


template <class CharType>
CharType CBasicStringBase<CharType>::GetAt( size_t nIndex ) const
{
    return m_pstrData[nIndex];
}


template <class CharType>
void CBasicStringBase<CharType>::Concat( const CBasicStringBase &other )
{
    if (EnsureLength( Length() + other.Length() + 1 ))
    {
        GenericConcatenate( m_pstrData, other.String() );

        PropagateError( other );
    }
}

template <class CharType>
CBasicStringBase<CharType> &CBasicStringBase<CharType>::MakeUpper()
{
    //
    // Make sure the string is not NULL
    //
    if (m_pstrData)
    {
        IS_CHAR(*m_pstrData) ? CharUpperBuffA( (LPSTR)m_pstrData, (DWORD)Length() ) : CharUpperBuffW( (LPWSTR)m_pstrData, (DWORD)Length() );
    }
    return *this;
}

template <class CharType>
CBasicStringBase<CharType> &CBasicStringBase<CharType>::MakeLower()
{
    //
    // Make sure the string is not NULL
    //
    if (m_pstrData)
    {
        IS_CHAR(*m_pstrData) ? CharLowerBuffA( (LPSTR)m_pstrData, (DWORD)Length() ) : CharLowerBuffW( (LPWSTR)m_pstrData, (DWORD)Length() );
    }
    return *this;
}

template <class CharType>
CBasicStringBase<CharType> CBasicStringBase<CharType>::ToUpper() const
{
    CBasicStringBase str(*this);
    str.MakeUpper();
    return str;
}

template <class CharType>
CBasicStringBase<CharType> CBasicStringBase<CharType>::ToLower() const
{
    CBasicStringBase str(*this);
    str.MakeLower();
    return str;
}

template <class CharType>
CharType &CBasicStringBase<CharType>::operator[](int nIndex)
{
    return m_pstrData[nIndex];
}

template <class CharType>
const CharType &CBasicStringBase<CharType>::operator[](int index) const
{
    return m_pstrData[index];
}

template <class CharType>
int CBasicStringBase<CharType>::Find( CharType cChar ) const
{
    CharType strTemp[2] = { cChar, 0};
    return Find(strTemp);
}


template <class CharType>
int CBasicStringBase<CharType>::Find( const CBasicStringBase &other, size_t nStart ) const
{
    if (!m_pstrData)
        return -1;
    if (nStart > Length())
        return -1;
    CharType *pstrCurr = m_pstrData+nStart, *pstrSrc, *pstrSubStr;
    while (*pstrCurr)
    {
        pstrSrc = pstrCurr;
        pstrSubStr = (CharType *)other.String();
        while (*pstrSrc && *pstrSubStr && *pstrSrc == *pstrSubStr)
        {
            pstrSrc = GenericCharNext(pstrSrc);
            pstrSubStr = GenericCharNext(pstrSubStr);
        }
        if (!*pstrSubStr)
            return static_cast<int>(pstrCurr-m_pstrData);
        pstrCurr = GenericCharNext(pstrCurr);
    }
    return -1;
}

template <class CharType>
int CBasicStringBase<CharType>::ReverseFind( CharType cChar ) const
{
    CharType strTemp[2] = { cChar, 0};
    return ReverseFind(strTemp);
}

template <class CharType>
int CBasicStringBase<CharType>::ReverseFind( const CBasicStringBase &srcStr ) const
{
    int nLastFind = -1, nFind=0;
    while ((nFind = Find( srcStr, nFind )) >= 0)
    {
        nLastFind = nFind;
        ++nFind;
    }
    return nLastFind;
}

template <class CharType>
CBasicStringBase<CharType> CBasicStringBase<CharType>::SubStr( size_t nStart, size_t nCount ) const
{
    if (nStart >= Length())
    {
        return CBasicStringBase<CharType>();
    }

    nCount = min( Length(), nCount );

    CBasicStringBase<CharType> strResult;
    CharType *pszBuffer = strResult.GetBuffer(nCount);
    if (pszBuffer)
    {
        GenericCopyLength( pszBuffer, m_pstrData+nStart, nCount+1 );
    }
    return strResult;
}

template <class CharType>
CBasicStringBase<CharType> CBasicStringBase<CharType>::SubStr( size_t nStart ) const
{
    return SubStr( nStart, Length() - nStart );
}


template <class CharType>
int CBasicStringBase<CharType>::CompareNoCase( const CBasicStringBase &other, int cchLength ) const
{
    if (cchLength < 0)
    {
        //
        // Make sure both strings are non-NULL
        //
        if (!String() && !other.String())
        {
            return 0;
        }
        else if (!String())
        {
            return -1;
        }
        else if (!other.String())
        {
            return 1;
        }
        else return GenericCompareNoCase(m_pstrData,other.String());
    }
    CBasicStringBase<CharType> strSrc(*this);
    CBasicStringBase<CharType> strTgt(other);
    strSrc.MakeUpper();
    strTgt.MakeUpper();
    //
    // Make sure both strings are non-NULL
    //
    if (!strSrc.String() && !strTgt.String())
    {
        return 0;
    }
    else if (!strSrc.String())
    {
        return -1;
    }
    else if (!strTgt.String())
    {
        return 1;
    }
    else return GenericCompareLength(strSrc.String(),strTgt.String(),cchLength);
}


template <class CharType>
int CBasicStringBase<CharType>::Compare( const CBasicStringBase &other, int cchLength ) const
{
    //
    // Make sure both strings are non-NULL
    //
    if (!String() && !other.String())
    {
        return 0;
    }
    else if (!String())
    {
        return -1;
    }
    else if (!other.String())
    {
        return 1;
    }

    if (cchLength < 0)
    {
        return GenericCompare(String(),other.String());
    }
    return GenericCompareLength(String(),other.String(),cchLength);
}

//
// Two main typedefs
//
typedef CBasicStringBase<char>     CBasicStringAnsi;
typedef CBasicStringBase<wchar_t>  CBasicStringWide;

//
// LPCTSTR equivalents
//
#if defined(UNICODE) || defined(_UNICODE)
typedef CBasicStringWide CBasicString;
#else
typedef CBasicStringAnsi CBasicString;
#endif

//
// Operators
//
inline bool operator<( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    return a.Compare(b) < 0;
}

inline bool operator<( const CBasicStringWide &a, const CBasicStringWide &b )
{
    return a.Compare(b) < 0;
}

inline bool operator<=( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    return a.Compare(b) <= 0;
}

inline bool operator<=( const CBasicStringWide &a, const CBasicStringWide &b )
{
    return a.Compare(b) <= 0;
}

inline bool operator==( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    return a.Compare(b) == 0;
}

inline bool operator==( const CBasicStringWide &a, const CBasicStringWide &b )
{
    return a.Compare(b) == 0;
}

inline bool operator!=( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    return a.Compare(b) != 0;
}

inline bool operator!=( const CBasicStringWide &a, const CBasicStringWide &b )
{
    return a.Compare(b) != 0;
}

inline bool operator>=( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    return a.Compare(b) >= 0;
}

inline bool operator>=( const CBasicStringWide &a, const CBasicStringWide &b )
{
    return a.Compare(b) >= 0;
}

inline bool operator>( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    return a.Compare(b) > 0;
}

inline bool operator>( const CBasicStringWide &a, const CBasicStringWide &b )
{
    return a.Compare(b) > 0;
}

inline CBasicStringWide operator+( const CBasicStringWide &a, const CBasicStringWide &b )
{
    CBasicStringWide strResult(a);
    strResult.Concat(b);
    return strResult;
}

inline CBasicStringAnsi operator+( const CBasicStringAnsi &a, const CBasicStringAnsi &b )
{
    CBasicStringAnsi strResult(a);
    strResult.Concat(b);
    return strResult;
}

//
// Restore the warning state
//
#pragma warning( pop )

#endif  // ifndef _SIMSTR_H_INCLUDED

