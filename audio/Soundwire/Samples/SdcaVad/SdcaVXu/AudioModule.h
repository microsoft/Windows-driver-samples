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
    AudioModule_Parameter   ParameterId;
} AUDIOMODULE_CUSTOM_COMMAND, *PAUDIOMODULE_CUSTOM_COMMAND;

enum AudioModule_Notification_Type {
    AudioModuleParameterChanged = 0,
};

typedef struct _AUDIOMODULE_CUSTOM_NOTIFICATION {
    ULONG Type;
    union
    {
        struct
        {
            ULONG ParameterId;  
        } ParameterChanged;
    };
} AUDIOMODULE_CUSTOM_NOTIFICATION, *PAUDIOMODULE_CUSTOM_NOTIFICATION;

#define AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION  0x00000001

typedef struct _SDCAXU_AUDIOMODULE_CONTEXT {
    ACXPNPEVENT                 Event;
    ULONG                       Parameter1;
    BYTE                        Parameter2;
    ULONG                       InstanceId;
    ACXCIRCUIT                  Circuit;
} SDCAXU_AUDIOMODULE_CONTEXT, *PSDCAXU_AUDIOMODULE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_AUDIOMODULE_CONTEXT, GetSdcaXuAudioModuleContext);

typedef struct _SDCAXU_AUDIOMODULE0_CONTEXT {
    ACXPNPEVENT                 Event;
    ULONG                       Parameter1;
    BYTE                        Parameter2;
    ULONG                       InstanceId;
    ACXCIRCUIT                  Circuit;
} SDCAXU_AUDIOMODULE0_CONTEXT, *PSDCAXU_AUDIOMODULE0_CONTEXT;

typedef struct _SDCAXU_AUDIOMODULE1_CONTEXT {
    ACXPNPEVENT                 Event;
    BYTE                        Parameter1;
    ULONGLONG                   Parameter2;
    DWORD                       Parameter3;
    ULONG                       InstanceId;
    ACXCIRCUIT                  Circuit;
} SDCAXU_AUDIOMODULE1_CONTEXT, *PSDCAXU_AUDIOMODULE1_CONTEXT;

typedef struct _SDCAXU_AUDIOMODULE2_CONTEXT {
    ACXPNPEVENT                 Event;
    ULONG                       Parameter1;
    USHORT                      Parameter2;
    ULONG                       InstanceId;
    ACXCIRCUIT                  Circuit;
} SDCAXU_AUDIOMODULE2_CONTEXT, *PSDCAXU_AUDIOMODULE2_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_AUDIOMODULE0_CONTEXT, GetSdcaXuAudioModule0Context);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_AUDIOMODULE1_CONTEXT, GetSdcaXuAudioModule1Context);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SDCAXU_AUDIOMODULE2_CONTEXT, GetSdcaXuAudioModule2Context);

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
// factory Module definitions
//
#define AUDIOMODULEDESCRIPTION L"Generic system module"
#define AUDIOMODULE_MAJOR  0x1
#define AUDIOMODULE_MINOR  0X0
static const GUID AudioModuleId = 
{ 0xe24c8b6f, 0xede7, 0x4255, 0x8b, 0x39, 0x77, 0x97, 0x60, 0x15, 0xd5, 0x93 };

EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  SdcaXu_EvtProcessCommand;

static
ULONG AudioModule_ValidParameterList[] =
{
    1, 2, 5
};

extern AUDIOMODULE_PARAMETER_INFO AudioModule_ParameterInfo[2];


//
// render Module 0 definitions
//
#define AUDIOMODULE0DESCRIPTION L"Generic system module"
#define AUDIOMODULE0_MAJOR  0x1
#define AUDIOMODULE0_MINOR  0X0

// {D96F901A-BFDD-46FD-9B1E-4FCD9D693360}
static const GUID AudioModule0Id = 
{ 0xd96f901a, 0xbfdd, 0x46fd, { 0x9b, 0x1e, 0x4f, 0xcd, 0x9d, 0x69, 0x33, 0x60 } };

EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  SdcaXu_EvtProcessCommand0;

static
ULONG AudioModule0_ValidParameterList[] =
{
    1, 2, 5
};

extern AUDIOMODULE_PARAMETER_INFO AudioModule0_ParameterInfo[2];

//
// render Module 1 definitions
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

// {631A7961-DED6-4405-9A37-E4C4380918E4}
static const GUID AudioModule1Id = 
{ 0x631a7961, 0xded6, 0x4405, { 0x9a, 0x37, 0xe4, 0xc4, 0x38, 0x9, 0x18, 0xe4 } };

EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  SdcaXu_EvtProcessCommand1;

//
// render Module 2 definitions
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

// {3471D6C5-6322-4730-8FD3-177061B52BB5}
static const GUID AudioModule2Id = 
{ 0x3471d6c5, 0x6322, 0x4730, { 0x8f, 0xd3, 0x17, 0x70, 0x61, 0xb5, 0x2b, 0xb5 } };

EVT_ACX_AUDIOMODULE_PROCESSCOMMAND  SdcaXu_EvtProcessCommand2;

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
