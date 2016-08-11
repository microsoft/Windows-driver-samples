/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Driver.cpp

Abstract:

    Driver object callbacks, functions, and types.

Environment:

    Kernel-mode only.

--*/

#pragma once

#define TAG_UCSI 'iscU'

#define TEST_BIT(value, bitNumber) ((value) & (1<<(bitNumber))) ? true : false
