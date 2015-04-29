/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   schema.cpp

Abstract:

   PrintSchema implementation. This provides the definition of keywords common
   to all features within the PrintSchema.

--*/

#include "precomp.h"
#include "schema.h"

LPCWSTR XDPrintSchema::SCHEMA_INST_URI =
    L"http://www.w3.org/2001/XMLSchema-instance";

LPCWSTR XDPrintSchema::SCHEMA_DEF_URI =
    L"http://www.w3.org/2001/XMLSchema";

LPCWSTR XDPrintSchema::SCHEMA_TYPE    = L"type";
LPCWSTR XDPrintSchema::SCHEMA_INTEGER = L"integer";
LPCWSTR XDPrintSchema::SCHEMA_DECIMAL = L"decimal";
LPCWSTR XDPrintSchema::SCHEMA_STRING  = L"string";
LPCWSTR XDPrintSchema::SCHEMA_QNAME   = L"QName";
LPCWSTR XDPrintSchema::SCHEMA_CONDITIONAL = L"Conditional";

LPCWSTR XDPrintSchema::FRAMEWORK_URI =
    L"http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework";

LPCWSTR XDPrintSchema::KEYWORDS_URI =
    L"http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords";

LPCWSTR XDPrintSchema::FEATURE_ELEMENT_NAME     = L"Feature";
LPCWSTR XDPrintSchema::OPTION_ELEMENT_NAME      = L"Option";
LPCWSTR XDPrintSchema::PARAM_INIT_ELEMENT_NAME  = L"ParameterInit";
LPCWSTR XDPrintSchema::PARAM_REF_ELEMENT_NAME   = L"ParameterRef";
LPCWSTR XDPrintSchema::PARAM_DEF_ELEMENT_NAME   = L"ParameterDef";
LPCWSTR XDPrintSchema::SCORED_PROP_ELEMENT_NAME = L"ScoredProperty";
LPCWSTR XDPrintSchema::PROPERTY_ELEMENT_NAME    = L"Property";
LPCWSTR XDPrintSchema::VALUE_ELEMENT_NAME       = L"Value";
LPCWSTR XDPrintSchema::NAME_ATTRIBUTE_NAME      = L"name";


LPCWSTR XDPrintSchema::PICKONE_VALUE_NAME       = L"PickOne";
LPCWSTR XDPrintSchema::SELECTIONTYPE_VALUE_NAME = L"SelectionType";
LPCWSTR XDPrintSchema::DATATYPE_VALUE_NAME      = L"DataType";
LPCWSTR XDPrintSchema::DEFAULTVAL_VALUE_NAME    = L"DefaultValue";
LPCWSTR XDPrintSchema::MAX_VALUE_NAME           = L"MaxValue";
LPCWSTR XDPrintSchema::MIN_VALUE_NAME           = L"MinValue";
LPCWSTR XDPrintSchema::MAX_LENGTH_NAME          = L"MaxLength";
LPCWSTR XDPrintSchema::MIN_LENGTH_NAME          = L"MinLength";
LPCWSTR XDPrintSchema::MULTIPLE_VALUE_NAME      = L"Multiple";
LPCWSTR XDPrintSchema::MANDATORY_VALUE_NAME     = L"Mandatory";
LPCWSTR XDPrintSchema::UNITTYPE_VALUE_NAME      = L"UnitType";
LPCWSTR XDPrintSchema::DISPLAYNAME_VALUE_NAME   = L"DisplayName";

LPCWSTR XDPrintSchema::PRINTTICKET_NAME       = L"PrintTicket";
LPCWSTR XDPrintSchema::PRINTCAPABILITIES_NAME = L"PrintCapabilities";
