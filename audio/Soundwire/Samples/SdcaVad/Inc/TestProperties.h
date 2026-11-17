/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    TestProperties.h

Abstract:

    Contains Test property values

Environment:

    Kernel mode

--*/

#pragma once

const ULONG _DSP_STREAM_PROPERTY_UI4_VALUE = 1;

//
// VENDOR SPECIFIC DATA
// These are made up placeholders to demonstrate how the KSPROPERTY_SDCA_VENDOR_SPECIFIC works
typedef struct _VIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL
{
    ULONG VendorSpecificId;
    ULONG VendorSpecificSize;
    union
    {
        struct TestData
        {
            ULONG EndpointId;
            ULONG DataPort;
        } Data;
        struct TestConfig
        {
            BOOLEAN IsScatterGather;
        } Config;
    };
} VIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL, * PVIRTUAL_STACK_VENDOR_SPECIFIC_CONTROL;

typedef struct _VIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA
{
    ULONG Test1;
    ULONG Test2;
} VIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA, * PVIRTUAL_STACK_VENDOR_SPECIFIC_VALUE_TEST_DATA;

enum VIRTUAL_STACK_VENDOR_SPECIFIC_REQUEST
{
    VirtualStackVendorSpecificRequestGetTestData,
    VirtualStackVendorSpecificRequestSetTestConfig
};

