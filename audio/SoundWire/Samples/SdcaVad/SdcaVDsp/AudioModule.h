/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    AudioModule.h

Abstract:

    Contains audio modules definitions and function prototypes private to
    the driver.

Environment:

    Kernel mode

--*/

#ifndef _AUDIOMODULE_H_
#define _AUDIOMODULE_H_

/* make prototypes usable from C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Audio module definitions

//
// Audio module instance defintion.
// This sample driver generates an instance id by combinding the
// configuration set # for a class module id with the instance of that 
// configuration. Real driver should use a more robust scheme, such as
// an indirect mapping from/to an instance id to/from a configuration set
// + location in the pipeline + any other info the driver needs.
//
// top 8 bits reserved for use by aggregation
// next 12 bits are the config id mask
// bottom 12 bits instance id
#define AUDIOMODULE_CLASS_CFG_ID_MASK           0xFFF
#define AUDIOMODULE_CLASS_CFG_INSTANCE_ID_MASK  0xFFF
    
#define AUDIOMODULE_INSTANCE_ID(ClassCfgId, ClassCfgInstanceId) \
        ((ULONG(ClassCfgId & AUDIOMODULE_CLASS_CFG_ID_MASK) << 12) | \
         (ULONG(ClassCfgInstanceId & AUDIOMODULE_CLASS_CFG_INSTANCE_ID_MASK)))
    
#define AUDIOMODULE_GET_CLASSCFGID(InstanceId) \
        (ULONG(InstanceId) >> 12 & AUDIOMODULE_CLASS_CFG_ID_MASK)

enum AudioModule_Parameter {
    AudioModuleParameter1 = 0,
    AudioModuleParameter2,
    AudioModuleParameter3
};

typedef struct _AUDIOMODULE_CUSTOM_COMMAND {
    ULONG                   Verb;       // get, set and support
    AudioModule_Parameter  ParameterId;
} AUDIOMODULE_CUSTOM_COMMAND, *PAUDIOMODULE_CUSTOM_COMMAND;

enum AudioModule_Notification_Type {
    AudioModuleParameterChanged = 0,
};

typedef struct _AUDIOMODULE_CUSTOM_NOTIFICATION {
    ULONG           Type;
    union {
        struct {
            ULONG   ParameterId;  
        } ParameterChanged;
    };
} AUDIOMODULE_CUSTOM_NOTIFICATION, *PAUDIOMODULE_CUSTOM_NOTIFICATION;

#define AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION  0x00000001

typedef struct _DSP_AUDIOMODULE0_CONTEXT {
    ACXPNPEVENT                 Event;
    ULONG                       Parameter1;
    BYTE                        Parameter2;
    ULONG                       InstanceId;
} DSP_AUDIOMODULE0_CONTEXT, *PDSP_AUDIOMODULE0_CONTEXT;

typedef struct _DSP_AUDIOMODULE1_CONTEXT {
    ACXPNPEVENT                 Event;
    BYTE                        Parameter1;
    ULONGLONG                   Parameter2;
    DWORD                       Parameter3;
    ULONG                       InstanceId;
} DSP_AUDIOMODULE1_CONTEXT, *PDSP_AUDIOMODULE1_CONTEXT;

typedef struct _DSP_AUDIOMODULE2_CONTEXT {
    ACXPNPEVENT                 Event;
    ULONG                       Parameter1;
    USHORT                      Parameter2;
    ULONG                       InstanceId;
} DSP_AUDIOMODULE2_CONTEXT, *PDSP_AUDIOMODULE2_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_AUDIOMODULE0_CONTEXT, GetDspAudioModule0Context);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_AUDIOMODULE1_CONTEXT, GetDspAudioModule1Context);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DSP_AUDIOMODULE2_CONTEXT, GetDspAudioModule2Context);

typedef struct _AUDIOMODULE_PARAMETER_INFO
{
    USHORT      AccessFlags;        // get/set/basic-support attributes.
    USHORT      Flags;        
    ULONG       Size;
    DWORD       VtType;
    PVOID       ValidSet;
    ULONG       ValidSetCount;
} AUDIOMODULE_PARAMETER_INFO, *PAUDIOMODULE_PARAMETER_INFO;

//
// Module 0 definitions
//
#define AUDIOMODULE0DESCRIPTION L"Generic system module"
#define AUDIOMODULE0_MAJOR  0x1
#define AUDIOMODULE0_MINOR  0X0

// {BD7CDC7F-F52E-4A95-B026-586926056128}
static const GUID AudioModule0Id = 
{ 0xbd7cdc7f, 0xf52e, 0x4a95, { 0xb0, 0x26, 0x58, 0x69, 0x26, 0x5, 0x61, 0x28 } };

EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  DspR_EvtProcessCommand0;

static
ULONG AudioModule0_ValidParameterList[] =
{
    1, 2, 5
};

extern AUDIOMODULE_PARAMETER_INFO AudioModule0_ParameterInfo[2];

//
// Module 1 definitions
//
static
BYTE AudioModule1_ValidParameterList[] =
{
    0, 1, 2
};

extern AUDIOMODULE_PARAMETER_INFO AudioModule1_ParameterInfo[3];

#define AUDIOMODULE1DESCRIPTION L"Module 1"
#define AUDIOMODULE1_MAJOR  0x2
#define AUDIOMODULE1_MINOR  0X1

// {2803D255-6175-40A4-A572-ECF9FF6F07A9}
static const GUID AudioModule1Id = 
{ 0x2803d255, 0x6175, 0x40a4, { 0xa5, 0x72, 0xec, 0xf9, 0xff, 0x6f, 0x7, 0xa9 } };

EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  DspR_EvtProcessCommand1;

//
// Module 2 definitions
//
static
ULONG AudioModule2_ValidParameterList[] =
{
    1, 0xfffffffe
};

extern AUDIOMODULE_PARAMETER_INFO AudioModule2_ParameterInfo[2];

#define AUDIOMODULE2DESCRIPTION L"Module 2"
#define AUDIOMODULE2_MAJOR  0x2
#define AUDIOMODULE2_MINOR  0X0

// {2225578F-DF3B-40D8-BE80-031E1649DCC4}
static const GUID AudioModule2Id = 
{ 0x2225578f, 0xdf3b, 0x40d8, { 0xbe, 0x80, 0x3, 0x1e, 0x16, 0x49, 0xdc, 0xc4 } };


EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  DspR_EvtProcessCommand2;

// General purpose helper functions

NTSTATUS
AudioModule_GenericHandler_BasicSupport(
    _In_ PAUDIOMODULE_PARAMETER_INFO            ParameterInfo,
    _Out_writes_bytes_opt_(*BufferCb) PVOID     Buffer,
    _Inout_ ULONG *                             BufferCb
    );

BOOLEAN
IsAudioModuleParameterValid(
    _In_ PAUDIOMODULE_PARAMETER_INFO        ParameterInfo,
    _In_reads_bytes_opt_(BufferCb) PVOID    Buffer,
    _In_  ULONG                             BufferCb
    );

NTSTATUS
AudioModule_GenericHandler(
    _In_  ULONG                                         Verb,
    _In_  ULONG                                         ParameterId,
    _In_  PAUDIOMODULE_PARAMETER_INFO                   ParameterInfo,
    _Inout_updates_bytes_(ParameterInfo->Size) PVOID    CurrentValue,
    _In_reads_bytes_opt_(InBufferCb) PVOID              InBuffer,
    _In_  ULONG                                         InBufferCb,
    _Out_writes_bytes_opt_(*OutBufferCb) PVOID          OutBuffer,
    _Inout_ ULONG *                                     OutBufferCb, 
    _In_ BOOL *                                         ParameterChanged
    );

/* make internal prototypes usable from C++ */
#ifdef __cplusplus
}
#endif

#endif // _AUDIOMODULE_H_
