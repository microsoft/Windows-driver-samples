/*++

Copyright (c) Microsoft Corporation

Module Name:

    sample.h

Abstract:

    The shared header file for the sample TrEE miniport driver.

Environment:

    Kernel mode

--*/

//
// Device context
//

typedef struct _TREE_SAMPLE_DEVICE_CONTEXT {
    WDFDEVICE MasterDevice;
} TREE_SAMPLE_DEVICE_CONTEXT, *PTREE_SAMPLE_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(TREE_SAMPLE_DEVICE_CONTEXT);

TR_SECURE_SERVICE_CALLBACKS TestServiceCallbacks;
TR_SECURE_SERVICE_CALLBACKS Test2ServiceCallbacks;