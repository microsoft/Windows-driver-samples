/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ptquerybld.cpp

Abstract:

   PrintTicket XPath query builder implementation. The CPTQueryBuilder class
   provides a means of constructing PrintTicket specific XPath queries for
   retrieving nodes from within the DOM document describing the PrintTicket.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "xdstring.h"
#include "ptquerybld.h"

//
// XML PrintTicket Query Builder Format Strings
//
static PCWSTR szPTQFeature               = L"//%sFeature[@name = \"%s\"]";
static PCWSTR szPTQProperty              = L"//%sProperty[@name = \"%s\"]";
static PCWSTR szPTQScoredProperty        = L"//%sScoredProperty[@name = \"%s\"]";
static PCWSTR szPTQParamRef              = L"//%sParameterRef[@name = \"%s\"]";
static PCWSTR szPTQOptionSH              = L"/%sOption";
static PCWSTR szPTQOptionLH              = L"/%sScoredProperty[@name = \"%sOptionName\"]/%sValue";
static PCWSTR szPTQOrNodes               = L" | ";

/*++

Routine Name:

    CPTQueryBuilder::CPTQueryBuilder

Routine Description:

    CPTQueryBuilder class default constructor

Arguments:

    bstrFrameworkNS - PrintTicket framework namespace prefix

Return Value:

    None

--*/
CPTQueryBuilder::CPTQueryBuilder(
    _In_z_ BSTR  bstrFrameworkNS
    ) :
    m_bstrFrameworkNS(bstrFrameworkNS)
{
}

/*++

Routine Name:

    CPTQueryBuilder::CPTQueryBuilder

Routine Description:

    CPTQueryBuilder class constructor

Arguments:

    bstrFrameworkNS - PrintTicket framework namespace prefix
    bstrQuery - String containing base query to use on construction.

Return Value:

    None

--*/
CPTQueryBuilder::CPTQueryBuilder(
    _In_z_ BSTR  bstrFrameworkNS,
    _In_z_ BSTR  bstrQuery
    ) :
    m_bstrFrameworkNS(bstrFrameworkNS),
    m_bstrQuery(bstrQuery)
{
}

/*++

Routine Name:

    CPTQueryBuilder::~CPTQueryBuilder

Routine Description:

    CPTQueryBuilder class destructor.

Arguments:

    None

Return Value:

    None

--*/
CPTQueryBuilder::~CPTQueryBuilder(
    )
{
}

/*++

Routine Name:

    CPTQueryBuilder::Clear

Routine Description:

    Clears the current query string.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::Clear(
    VOID
    )
{
    m_bstrQuery.Empty();

    return S_OK;
}

/*++

Routine Name:

    CPTQueryBuilder::GetQuery

Routine Description:

    Makes a copy of the current query string and returns it to the caller.

Arguments:

    bstrQuery - Address of a pointer that will be modified to point to a string
                that is filled out with a copy of the current query.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::GetQuery(
        _Inout_ _At_(*pbstrQuery, _Pre_maybenull_ _Post_valid_) BSTR*  pbstrQuery
        )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pbstrQuery, E_POINTER)))
    {
        SysFreeString(*pbstrQuery);
        hr = m_bstrQuery.CopyTo(pbstrQuery);
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTQueryBuilder::AddFeature

Routine Description:

    Appends a feature query to the query builder class.

Arguments:

    bstrKeywordNS - Optional parameter defining the keyword namespace to prefix.
    bstrFeature - Pointer to a string defining the feature name to be appended.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::AddFeature(
    _In_opt_z_ BSTR  bstrKeywordNS,
    _In_z_     BSTR  bstrFeature
    )
{
    HRESULT hr = S_OK;

    if (m_bstrFrameworkNS.Length() > 0)
    {
        try
        {
            CComBSTR bstrKeyword;

            if (SysStringLen(bstrKeywordNS) > 0)
            {
                hr = bstrKeyword.Append(bstrKeywordNS);
            }

            if (SUCCEEDED(hr))
            {
                hr = bstrKeyword.Append(bstrFeature);
            }

            if (SUCCEEDED(hr))
            {
                CStringXDW cstrFeatureQuery;
                cstrFeatureQuery.Format(szPTQFeature, m_bstrFrameworkNS, bstrKeyword);

                hr = m_bstrQuery.Append(cstrFeatureQuery);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTQueryBuilder::AddProperty

Routine Description:

    Appends a property query to the query builder class.

Arguments:

    bstrKeywordNS - Optional parameter defining the keyword namespace to prefix.
    bstrProperty - Pointer to a string defining the property name to be appended.


Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::AddProperty(
    _In_opt_z_ BSTR  bstrKeywordNS,
    _In_z_     BSTR  bstrProperty
    )
{
    HRESULT hr = S_OK;

    if (m_bstrFrameworkNS.Length() > 0)
    {
        try
        {
            CComBSTR bstrKeyword;

            if (SysStringLen(bstrKeywordNS) > 0)
            {
                hr = bstrKeyword.Append(bstrKeywordNS);
            }

            if (SUCCEEDED(hr))
            {
                hr = bstrKeyword.Append(bstrProperty);
            }

            if (SUCCEEDED(hr))
            {
                CStringXDW cstrPropertyQuery;
                cstrPropertyQuery.Format(szPTQProperty, m_bstrFrameworkNS, bstrKeyword);

                hr = m_bstrQuery.Append(cstrPropertyQuery);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTQueryBuilder::AddScoredProperty

Routine Description:

    Appends a scored property query to the query builder class.

Arguments:

    bstrKeywordNS - Optional parameter defining the keyword namespace to prefix.
    bstrScoredProperty - Pointer to a string defining the scored property name to be appended.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::AddScoredProperty(
    _In_opt_z_ BSTR  bstrKeywordNS,
    _In_z_     BSTR  bstrScoredProperty
    )
{
    HRESULT hr = S_OK;

    if (m_bstrFrameworkNS.Length() > 0)
    {
        try
        {
            CComBSTR bstrKeyword;

            if (SysStringLen(bstrKeywordNS) > 0)
            {
                hr = bstrKeyword.Append(bstrKeywordNS);
            }

            if (SUCCEEDED(hr))
            {
                hr = bstrKeyword.Append(bstrScoredProperty);
            }

            if (SUCCEEDED(hr))
            {
                CStringXDW cstrScoredPropertyQuery;
                cstrScoredPropertyQuery.Format(szPTQScoredProperty, m_bstrFrameworkNS, bstrKeyword);

                hr = m_bstrQuery.Append(cstrScoredPropertyQuery);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTQueryBuilder::AddScoredProperty

Routine Description:

    Appends a Parameter Reference query to the query builder class.

Arguments:

    bstrKeywordNS - Optional parameter defining the keyword namespace to prefix.
    bstrScoredProperty - Pointer to a string defining the scored property name to be appended.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::AddParamRef(
    _In_opt_z_    BSTR  bstrKeywordNS,
    _In_z_        BSTR  bstrParameterRef
    )
{
    HRESULT hr = S_OK;

    if (m_bstrFrameworkNS.Length() > 0)
    {
        try
        {
            CComBSTR bstrKeyword;

            if (SysStringLen(bstrKeywordNS) > 0)
            {
                hr = bstrKeyword.Append(bstrKeywordNS);
            }

            if (SUCCEEDED(hr))
            {
                hr = bstrKeyword.Append(bstrParameterRef);
            }

            if (SUCCEEDED(hr))
            {
                CStringXDW cstrParamRefQuery;
                cstrParamRefQuery.Format(szPTQParamRef, m_bstrFrameworkNS, bstrKeyword);

                hr = m_bstrQuery.Append(cstrParamRefQuery);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CPTQueryBuilder::AddOption

Routine Description:

    Appends the option query to the query builder class.

Arguments:

    None

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CPTQueryBuilder::AddOption(
    _In_opt_z_ BSTR  bstrKeywordNS
    )
{
    HRESULT hr = S_OK;

    if (m_bstrFrameworkNS.Length() > 0)
    {
        try
        {
            CStringXDW cstrQueryRoot(m_bstrQuery);
            CStringXDW cstrOptionQuery;
            cstrOptionQuery.Format(szPTQOptionSH, m_bstrFrameworkNS);

            if (SUCCEEDED(hr = m_bstrQuery.Append(cstrOptionQuery)) &&
                SUCCEEDED(hr = m_bstrQuery.Append(szPTQOrNodes)) &&
                SUCCEEDED(hr = m_bstrQuery.Append(cstrQueryRoot)))
            {
                cstrOptionQuery.Format(szPTQOptionLH, m_bstrFrameworkNS, bstrKeywordNS, m_bstrFrameworkNS);
                hr = m_bstrQuery.Append(cstrOptionQuery);
            }
        }
        catch (CXDException& e)
        {
            hr = e;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    ERR_ON_HR(hr);
    return hr;
}
