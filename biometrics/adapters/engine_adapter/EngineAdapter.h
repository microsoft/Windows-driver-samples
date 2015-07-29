/*++
 
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    EngineAdapter.h

Abstract:

    This module contains a stub implementation of an Engine Adapter
    plug-in for the Windows Biometric service.

Author:

    -

Environment:

    Win32, user mode only.

Revision History:

NOTES:

    (None)

--*/
#pragma once

#include "winbio_adapter.h"

///////////////////////////////////////////////////////////////////////////////
//
// The WINIBIO_ENGINE_CONTEXT structure is privately-defined by each
// Engine Adapter. Its purpose is to maintain any information that
// should persist across Engine Adapter API calls. 
//
// The Adapter allocates and initializes one of these structures in its
// 'Attach' routine and saves its address in the Pipeline->EngineContext 
// field.
//
// The Engine Adapter's 'Detach' routine cleans up and deallocates the
// structure and sets the PipelineContext->EngineContext field to NULL.
//
///////////////////////////////////////////////////////////////////////////////
typedef struct _WINIBIO_ENGINE_CONTEXT {
    //
    // The following fields illustrate the kind of information 
    // the Engine Adapter needs to keep in this structure:
    //
    //      FeatureSet      - A processed description of a biometric
    //                        sample.
    //
    //      Enrollment      - An object that tracks the current state
    //                        of an in-progress enrollment operation.
    //
    //      Template        - A template either created from the Feature
    //                        Set or from the Enrollment object.
    //
    //      Comparison      - An object that tracks the result of a
    //                        one-to-one comparison between the Template
    //                        and the Feature Set.
    //
    PVOID x;
    PVOID y;
    PVOID z;

} WINIBIO_ENGINE_CONTEXT, *PWINIBIO_ENGINE_CONTEXT;

