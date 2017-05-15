/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    AudioModule2.h

Abstract:

    Declaration of Audio Module 2.

--*/

#ifndef _SYSVAD_AUDIOMODULE2_H_
#define _SYSVAD_AUDIOMODULE2_H_

#include "AudioModuleHelper.h"

#define AUDIOMODULE2_MAJOR  0x2
#define AUDIOMODULE2_MINOR  0X0

static const GUID AudioModule2Id = 
{ 0xcc2a9527, 0x19d9, 0x4784, 0x8d, 0xd4, 0x6c, 0x1f, 0xe0, 0x1e, 0x37, 0x26 };

enum AudioModule2_Parameter {
    AudioModule2Parameter1 = 0,
    AudioModule2Parameter2
};

typedef struct _AUDIOMODULE2_CUSTOM_COMMAND {
    ULONG                   Verb;       // get, set and support
    AudioModule2_Parameter  ParameterId;
} AUDIOMODULE2_CUSTOM_COMMAND, *PAUDIOMODULE2_CUSTOM_COMMAND;

enum AudioModule2_Notification_Type {
    AudioModule2ParameterChanged = 0,
};

typedef struct _AUDIOMODULE2_CUSTOM_NOTIFICATION {
    ULONG           Type;
    union {
        struct {
            ULONG   ParameterId;  
        } ParameterChanged;
    };
} AUDIOMODULE2_CUSTOM_NOTIFICATION, *PAUDIOMODULE2_CUSTOM_NOTIFICATION;

typedef struct _AUDIOMODULE2_NOTIFICATION {
    KSAUDIOMODULE_NOTIFICATION          Header;
    AUDIOMODULE2_CUSTOM_NOTIFICATION    CustomNotification;
} AUDIOMODULE2_NOTIFICATION, *PAUDIOMODULE2_NOTIFICATION;

typedef struct _AUDIOMODULE2_CONTEXT {
    ULONG                       Parameter1;
    USHORT                      Parameter2;
    ULONG                       InstanceId;
    AUDIOMODULE2_NOTIFICATION   Notification;  
    PPORTCLSNOTIFICATIONS       PortNotifications;
} AUDIOMODULE2_CONTEXT, *PAUDIOMODULE2_CONTEXT;

static
ULONG AudioModule2_ValidParameter1List[] =
{
    1, 0xfffffffe
};

static
AUDIOMODULE_PARAMETER_INFO  AudioModule2_ParameterInfo[] =
{
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE2_CONTEXT, Parameter1),
        VT_UI4,
        AudioModule2_ValidParameter1List,
        SIZEOF_ARRAY(AudioModule2_ValidParameter1List)
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        0, 
        (ULONG)RTL_FIELD_SIZE(AUDIOMODULE2_CONTEXT, Parameter2),
        VT_UI2,
        NULL,
        0
    },
};

inline
#pragma code_seg("PAGE")
NTSTATUS
AudioModule2_InitClass(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        KSAUDIOMODULE_NOTIFICATION * NotificationHeader,
    _In_opt_    PPORTCLSNOTIFICATIONS PortNotifications
    )
{
    PAUDIOMODULE2_CONTEXT ctx = (PAUDIOMODULE2_CONTEXT)Context;
        
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
AudioModule2_InitInstance(
    _In_  const AUDIOMODULE_DESCRIPTOR * Module,
    _In_opt_    PVOID           TemplateContext,
    _Inout_opt_ PVOID           Context,
    _In_        size_t          Size,
    _In_        ULONG           InstanceId
    )
{
    PAUDIOMODULE2_CONTEXT ctx = (PAUDIOMODULE2_CONTEXT)Context;

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
AudioModule2_Cleanup(
    _In_  PVOID           Context
    )
{
    PAUDIOMODULE2_CONTEXT ctx = (PAUDIOMODULE2_CONTEXT)Context;
    
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
AudioModule2_Handler(
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
        
    PAUDIOMODULE2_CONTEXT ctx = (PAUDIOMODULE2_CONTEXT)Context;
    AUDIOMODULE_PARAMETER_INFO * parameterInfo = NULL;
    AUDIOMODULE2_CUSTOM_COMMAND * command = NULL;
    
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

    if (InBufferCb < sizeof(AUDIOMODULE2_CUSTOM_COMMAND))
    {
        return STATUS_INVALID_PARAMETER;
    }

    command = (AUDIOMODULE2_CUSTOM_COMMAND*)InBuffer;  
    
    //
    // Validate the parameter referenced in the command.
    //
    switch (command->ParameterId)
    {
        case AudioModule2Parameter1:
            currentValue = &ctx->Parameter1;
            parameterInfo = &AudioModule2_ParameterInfo[AudioModule0Parameter1];
            break;

        case AudioModule2Parameter2:
            currentValue = &ctx->Parameter2;
            parameterInfo = &AudioModule2_ParameterInfo[AudioModule2Parameter2];
            break;
            
        default:
            status = STATUS_INVALID_PARAMETER;
            goto exit;
    }

    //
    // Update input buffer ptr/size.
    //
    inBuffer = (PVOID)((ULONG_PTR)InBuffer + sizeof(AUDIOMODULE2_CUSTOM_COMMAND));
    inBufferCb = InBufferCb - sizeof(AUDIOMODULE2_CUSTOM_COMMAND);

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
        ctx->Notification.CustomNotification.Type = AudioModule2ParameterChanged;
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

#endif // _SYSVAD_AUDIOMODULE2_H_

