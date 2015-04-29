/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ptquerybld.h

Abstract:

   PrintTicket XPath query builder definition. The CPTQueryBuilder class
   provides a means of constructing PrintTicket specific XPath queries for
   retrieving nodes from within the DOM document describing the PrintTicket.

--*/

#pragma once

class CPTQueryBuilder
{
public:
    CPTQueryBuilder(
        _In_z_ BSTR bstrFrameworkNS
        );

    CPTQueryBuilder(
        _In_z_ BSTR bstrFrameworkNS,
        _In_z_ BSTR bstrQuery
        );

    virtual ~CPTQueryBuilder();

    HRESULT
    Clear(
        VOID
        );

    HRESULT
    GetQuery(
        _Inout_ _At_(*pbstrQuery, _Pre_maybenull_ _Post_valid_) BSTR*  pbstrQuery
        );

    HRESULT
    AddFeature(
        _In_opt_z_ BSTR  bstrKeywordNS,
        _In_z_     BSTR  bstrPropertyName
        );

    HRESULT
    AddProperty(
        _In_opt_z_ BSTR  bstrKeywordNS,
        _In_z_     BSTR  bstrPropertyName
        );

    HRESULT
    AddScoredProperty(
        _In_opt_z_ BSTR  bstrKeywordNS,
        _In_z_     BSTR  bstrPropertyName
        );

    HRESULT
    AddParamRef(
        _In_opt_z_ BSTR  bstrKeywordNS,
        _In_z_     BSTR  bstrParameterRef
        );

    HRESULT
    AddOption(
        _In_opt_z_ BSTR  bstrKeywordNS
        );

private:
    CComBSTR    m_bstrQuery;

    CComBSTR    m_bstrFrameworkNS;
};

