/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   schema.h

Abstract:

   PrintSchema definition. This provides the definition of keywords common
   to all features within the PrintSchema. The description is placed within a
   XDPrintSchema namespace.

--*/

#pragma once

enum EPrintschemaVersion
{
    PRINTSCHEMA_VERSION_NUMBER =  1
};

namespace XDPrintSchema
{
    //
    // W3 namespaces
    //
    extern LPCWSTR SCHEMA_INST_URI;
    extern LPCWSTR SCHEMA_DEF_URI;

    //
    // W3 type prefixes
    //
    extern LPCWSTR SCHEMA_TYPE;
    extern LPCWSTR SCHEMA_INTEGER;
    extern LPCWSTR SCHEMA_DECIMAL;
    extern LPCWSTR SCHEMA_STRING;
    extern LPCWSTR SCHEMA_QNAME;
    extern LPCWSTR SCHEMA_CONDITIONAL;

    //
    // Namespaces
    //
    extern LPCWSTR FRAMEWORK_URI;
    extern LPCWSTR KEYWORDS_URI;

    //
    // Element and attribute types defined in Printschema framework
    //
    extern LPCWSTR FEATURE_ELEMENT_NAME;
    extern LPCWSTR OPTION_ELEMENT_NAME;
    extern LPCWSTR PARAM_INIT_ELEMENT_NAME;
    extern LPCWSTR PARAM_REF_ELEMENT_NAME;
    extern LPCWSTR PARAM_DEF_ELEMENT_NAME;
    extern LPCWSTR SCORED_PROP_ELEMENT_NAME;
    extern LPCWSTR VALUE_ELEMENT_NAME;
    extern LPCWSTR NAME_ATTRIBUTE_NAME;
    extern LPCWSTR PROPERTY_ELEMENT_NAME;

    //
    // Value types defined in the PrintSchema Keywords
    //
    extern LPCWSTR PICKONE_VALUE_NAME;
    extern LPCWSTR SELECTIONTYPE_VALUE_NAME;
    extern LPCWSTR DATATYPE_VALUE_NAME;
    extern LPCWSTR DEFAULTVAL_VALUE_NAME;
    extern LPCWSTR MAX_VALUE_NAME;
    extern LPCWSTR MIN_VALUE_NAME;
    extern LPCWSTR MAX_LENGTH_NAME;
    extern LPCWSTR MIN_LENGTH_NAME;
    extern LPCWSTR MULTIPLE_VALUE_NAME;
    extern LPCWSTR MANDATORY_VALUE_NAME;
    extern LPCWSTR UNITTYPE_VALUE_NAME;
    extern LPCWSTR DISPLAYNAME_VALUE_NAME;

    //
    // Root PrintTicket element
    //
    extern LPCWSTR PRINTTICKET_NAME;

    //
    // Root PrintCapabilities element
    //
    extern LPCWSTR PRINTCAPABILITIES_NAME;
}

