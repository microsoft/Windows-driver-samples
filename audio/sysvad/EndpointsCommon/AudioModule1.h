/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    AudioModule1.h

Abstract:

    Declaration of Audio Module 1.

--*/

#ifndef _SYSVAD_AUDIOMODULE1_H_
#define _SYSVAD_AUDIOMODULE1_H_

#include "AudioModuleHelper.h"

#define AUDIOMODULE1_MAJOR  0x2
#define AUDIOMODULE1_MINOR  0X1

static const GUID AudioModule1Id = 
{ 0x5ce55c8e, 0x84df, 0x4317, 0x98, 0xc7, 0x79, 0x2d, 0xe8, 0x33, 0x66, 0xcc };

enum AudioModule1_Parameter {
    AudioModule1Parameter1 = 0,
    AudioModule1Parameter2,
    AudioModule1Parameter3
};

typedef struct _AUDIOMODULE1_CUSTOM_COMMAND {
    ULONG                   Verb;       // get, set and support
    AudioModule1_Parameter  ParameterId;
} AUDIOMODULE1_CUSTOM_COMMAND, *PAUDIOMODULE1_CUSTOM_COMMAND;

enum AudioModule1_Notification_Type {
    AudioModule1ParameterChanged = 0,
};

typedef struct _AUDIOMODULE1_CUSTOM_NOTIFICATION {
    ULONG           Type;
    union {
        struct {
            ULONG   ParameterId;  
        } ParameterChanged;
    };
} AUDIOMODULE1_CUSTOM_NOTIFICATION, *PAUDIOMODULE1_CUSTOM_NOTIFICATION;

typedef struct _AUDIOMODULE1_NOTIFICATION {
    KSAUDIOMODULE_NOTIFICATION          Header;
    AUDIOMODULE1_CUSTOM_NOTIFICATION    CustomNotification;
} AUDIOMODULE1_NOTIFICATION, *PAUDIOMODULE1_NOTIFICATION;

typedef struct _AUDIOMODULE1_CONTEXT {
    BYTE                        Parameter1;
    ULONGLONG                   Parameter2;
    DWORD                       Parameter3;
    ULONG                       InstanceId;
    AUDIOMODULE1_NOTIFICATION   Notification;  
    PPORTCLSNOTIFICATIONS       PortNotifications;
} AUDIOMODULE1_CONTEXT, *PAUDIOMODULE1_CONTEXT;

static
BYTE AudioModule1_ValidParameter1List[] =
{
    0, 1, 2
};

static
AUDIOMODULE_PARAMETER_INFO  AudioModule1_ParameterInfo[] =
{
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE1_CONTEXT, Parameter1),
        VT_UI1,
        AudioModule1_ValidParameter1List,
        SIZEOF_ARRAY(AudioModule1_ValidParameter1List)
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        0,
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE1_CONTEXT, Parameter2),
        VT_UI8,
        NULL,
        0
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE1_CONTEXT, Parameter3),
        VT_UI4,
        NULL,
        0
    },
};

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule1_InitClass(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        KSAUDIOMODULE_NOTIFICATION * NotificationHeader,
    _In_opt_    PPORTCLSNOTIFICATIONS PortNotifications
    )
{
    PAUDIOMODULE1_CONTEXT ctx = (PAUDIOMODULE1_CONTEXT)Context;
        
    UNREFERENCED_PARAMETER(Module);

    PAGED_CODE();

    if (Context == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        return STATUS_INTERNAL_ERROR;
    }
    
    ASSERT(Size != 0);    
    RtlZeroMemory(Context, Size);
    
    ctx->Notification.Header = *NotificationHeader;
    ctx->PortNotifications = PortNotifications;
    if (ctx->PortNotifications != NULL)
    {
        ctx->PortNotifications->AddRef();
    }
        
    return STATUS_SUCCESS;
}

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule1_InitInstance(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _In_opt_    PVOID           TemplateContext,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        ULONG           InstanceId
    )
{
    PAUDIOMODULE1_CONTEXT ctx = (PAUDIOMODULE1_CONTEXT)Context;
    
    UNREFERENCED_PARAMETER(Module);

    PAGED_CODE();

    if (Context == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        return STATUS_INTERNAL_ERROR;
    }
    
    ASSERT(TemplateContext != NULL);
    ASSERT(Size != 0); 
    
    RtlCopyMemory(Context, TemplateContext, Size);
    
    ctx->InstanceId = InstanceId;
    ctx->Notification.Header.ProviderId.InstanceId = InstanceId;
    if (ctx->PortNotifications != NULL)
    {
        ctx->PortNotifications->AddRef();
    }
    
    return STATUS_SUCCESS;
}

inline
#pragma code_seg("PAGE")
VOID
AudioModule1_Cleanup(
    _In_  PVOID           Context
    )
{
    PAUDIOMODULE1_CONTEXT ctx = (PAUDIOMODULE1_CONTEXT)Context;
    
    PAGED_CODE();

    if (Context == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        return;
    }
    
    SAFE_RELEASE(ctx->PortNotifications);
}

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule1_Handler(
    _Inout_opt_                          PVOID   Context,
    _In_reads_bytes_(InBufferCb)         PVOID   InBuffer,
    _In_                                 ULONG   InBufferCb,
    _Out_writes_bytes_opt_(*OutBufferCb) PVOID   OutBuffer,
    _Inout_                              ULONG * OutBufferCb 
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    BOOL        fNewValue = FALSE;
    PVOID       currentValue;
    PVOID       inBuffer = NULL;
    ULONG       inBufferCb = 0;
        
    PAUDIOMODULE1_CONTEXT ctx = (PAUDIOMODULE1_CONTEXT)Context;
    AUDIOMODULE_PARAMETER_INFO * parameterInfo = NULL;
    AUDIOMODULE1_CUSTOM_COMMAND * command = NULL;
    
    PAGED_CODE();
    
    if (ctx == NULL)
    {
        ASSERT(FALSE);  // this should not happen.
        status = STATUS_INTERNAL_ERROR;
        goto exit;
    }

    //
    // Basic parameter validation (module specific).
    //
    if (InBuffer == NULL || InBufferCb == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (InBufferCb < sizeof(AUDIOMODULE1_CUSTOM_COMMAND))
    {
        return STATUS_INVALID_PARAMETER;
    }

    command = (AUDIOMODULE1_CUSTOM_COMMAND*)InBuffer;  
    
    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModule1Parameter1:
            currentValue = &ctx->Parameter1;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModule1Parameter1];
            break;
    
        case AudioModule1Parameter2:
            currentValue = &ctx->Parameter2;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModule1Parameter2];
            break;
            
        case AudioModule1Parameter3:
            currentValue = &ctx->Parameter3;
            parameterInfo = &AudioModule1_ParameterInfo[AudioModule1Parameter3];
            break;
    
        default:
            status = STATUS_INVALID_PARAMETER;
            goto exit;
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE1_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE1_CUSTOM_COMMAND);

    if (inBufferCb == 0)
    {
        inBuffer = NULL;
    }
    
    status = AudioModule_GenericHandler(
                command->Verb,
                command->ParameterId,
                parameterInfo,
                currentValue,
                inBuffer,
                inBufferCb,
                OutBuffer,
                OutBufferCb,
                &fNewValue);

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }
    
    if (fNewValue && ctx->PortNotifications &&
        (parameterInfo->Flags & AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION))
    {
        // This logic assumes that access to this function is serialized.
        ctx->Notification.CustomNotification.Type = AudioModule1ParameterChanged;
        ctx->Notification.CustomNotification.ParameterChanged.ParameterId = 
            command->ParameterId;

        AudioModule_SendNotification(ctx->PortNotifications,
                                     (PVOID)&ctx->Notification,
                                     (USHORT)sizeof(ctx->Notification));
    }

    // Normalize error code.
    status = STATUS_SUCCESS;
    
exit:
    return status;
}

#endif // _SYSVAD_AUDIOMODULE1_H_


