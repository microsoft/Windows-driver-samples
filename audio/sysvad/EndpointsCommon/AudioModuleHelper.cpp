/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    AudioModuleHelper.cpp

Abstract:

    Simple helper for Audio Modules logic.

--*/

#pragma warning (disable : 4127)

#include <sysvad.h>
#include <limits.h>
#include <ntintsafe.h>
#include "SysVadShared.h"
#include "AudioModuleHelper.h"

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_BasicSupport(
    _In_ PAUDIOMODULE_PARAMETER_INFO            ParameterInfo,
    _Out_writes_bytes_opt_(*BufferCb) PVOID     Buffer,
    _Inout_ ULONG *                             BufferCb
    )
{
    NTSTATUS    ntStatus        = STATUS_SUCCESS;
    ULONG       cbFullProperty  = 0;
    ULONG       cbDataListSize  = 0;

    DPF_ENTER(("[AudioModule_GenericHandler_BasicSupport]"));

    PAGED_CODE();

    ASSERT(ParameterInfo);
    ASSERT(BufferCb);

    //
    // Compute total size of property.
    //
    ntStatus = RtlULongMult(ParameterInfo->Size, 
                            ParameterInfo->ValidSetCount, 
                            &cbDataListSize);
    if (!NT_SUCCESS(ntStatus))
    {
        ASSERT(FALSE);
        *BufferCb = 0;
        return ntStatus;
    }

    ntStatus = RtlULongAdd(cbDataListSize, 
                           (ULONG)(sizeof(KSPROPERTY_DESCRIPTION) +
                                sizeof(KSPROPERTY_MEMBERSHEADER)),
                           &cbFullProperty);

    if (!NT_SUCCESS(ntStatus))
    {
        ASSERT(FALSE);
        *BufferCb = 0;
        return ntStatus;
    }

    //
    // Return the info the caller is asking for.
    //
    if (*BufferCb == 0)
    {
        // caller wants to know the size of the buffer.
        *BufferCb = cbFullProperty;
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }
    else if (*BufferCb >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        PKSPROPERTY_DESCRIPTION propDesc = PKSPROPERTY_DESCRIPTION(Buffer);

        propDesc->AccessFlags       = ParameterInfo->AccessFlags;
        propDesc->DescriptionSize   = cbFullProperty;
        propDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        propDesc->PropTypeSet.Id    = ParameterInfo->VtType;
        propDesc->PropTypeSet.Flags = 0;
        propDesc->MembersListCount  = 1;
        propDesc->Reserved          = 0;

        // if return buffer can also hold a list description, return it too
        if(*BufferCb  >= cbFullProperty)
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER members = 
                PKSPROPERTY_MEMBERSHEADER(propDesc + 1);

            members->MembersFlags   = KSPROPERTY_MEMBER_VALUES;
            members->MembersSize    = ParameterInfo->Size;
            members->MembersCount   = ParameterInfo->ValidSetCount;
            members->Flags          = KSPROPERTY_MEMBER_FLAG_DEFAULT;

            // fill in valid array.
            BYTE* array = (BYTE*)(members + 1);

            RtlCopyMemory(array, ParameterInfo->ValidSet, cbDataListSize);

            // set the return value size
            *BufferCb  = cbFullProperty;
        } 
        else
        {
            *BufferCb  = sizeof(KSPROPERTY_DESCRIPTION);
        }
    } 
    else if(*BufferCb  >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG accessFlags = PULONG(Buffer);

        *BufferCb  = sizeof(ULONG);
        *accessFlags = ParameterInfo->AccessFlags;
    }
    else
    {
        *BufferCb  = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandlerBasicSupportVolume

//=============================================================================
#pragma code_seg("PAGE")
BOOLEAN
IsAudioModuleParameterValid(
    _In_ PAUDIOMODULE_PARAMETER_INFO        ParameterInfo,
    _In_reads_bytes_opt_(BufferCb) PVOID    Buffer,
    _In_  ULONG                             BufferCb
    )
{
    PAGED_CODE();

    DPF_ENTER(("[IsAudioModuleParameterValid]"));
    
    ULONG       i = 0;
    ULONG       j = 0;
    BOOLEAN     validParam = FALSE;

    //
    // Validate buffer ptr and size.
    //
    if (Buffer == NULL || BufferCb == 0)
    {
        validParam = FALSE;
        goto exit;
    }
    
    //
    // Check its size.
    //
    if (BufferCb < ParameterInfo->Size)
    {
        validParam = FALSE;
        goto exit;
    }

    //
    // Check the valid list.
    //
    if (ParameterInfo->ValidSet && ParameterInfo->ValidSetCount)
    {
        BYTE* buffer = (BYTE*)ParameterInfo->ValidSet;
        BYTE* pattern = (BYTE*)Buffer;

        //
        // Scan the valid list.
        //
        for (i = 0; i < ParameterInfo->ValidSetCount; ++i)
        {
            for (j=0; j < ParameterInfo->Size; ++j)
            {
                if (buffer[j] != pattern[j])
                {
                    break;
                }
            }

            if (j == ParameterInfo->Size)
            {
                // got a match.
                break;
            }

            buffer += ParameterInfo->Size;
        }
        
        //
        // If end of list, we didn't find the value.
        //
        if (i == ParameterInfo->ValidSetCount)
        {
            validParam = FALSE;
            goto exit;
        }
    }
    else
    {
        // 
        // Negative-testing support. Fail request if value is -1.
        //
        BYTE* buffer = (BYTE*)Buffer;

        for (i = 0; i < ParameterInfo->Size; ++i)
        {
            if (buffer[i] != 0xFF)
            {
                break;
            }
        }

        //
        // If value is -1, return error.
        //
        if (i == ParameterInfo->Size)
        {
            validParam = FALSE;
            goto exit;
        }
    }

    validParam = TRUE;
    
exit:
    return validParam;
}

//=============================================================================
#pragma code_seg("PAGE")
AUDIOMODULE *
AudioModule_FindModuleInList(
    _In_ AUDIOMODULE *          AudioModules,
    _In_ ULONG                  AudioModuleCount,
    _In_ GUID *                 ClassId,
    _In_ ULONG                  InstanceId
    )
{
    for (ULONG i=0; i<AudioModuleCount; ++i)
    {
        AUDIOMODULE * module = &AudioModules[i];
        
        if (IsEqualGUIDAligned(*(module->Descriptor->ClassId), *ClassId) &&
            module->InstanceId == InstanceId)
        {
            return module;
        }
    }

    return NULL;
}

//=============================================================================
#pragma code_seg("PAGE")
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
    )
{
    PAGED_CODE();

    DPF_ENTER(("[AudioModule_GenericHandler]"));

    UNREFERENCED_PARAMETER(ParameterId);
    
    *ParameterChanged = FALSE;

    // Handle KSPROPERTY_TYPE_BASICSUPPORT query
    if (Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        return AudioModule_GenericHandler_BasicSupport(ParameterInfo, OutBuffer, OutBufferCb);
    }

    ULONG cbMinSize = ParameterInfo->Size;
    
    if (Verb & KSPROPERTY_TYPE_GET)
    {
        // Verify module parameter supports 'get'.
        if (!(ParameterInfo->AccessFlags & KSPROPERTY_TYPE_GET))
        {
            *OutBufferCb = 0;
            return STATUS_INVALID_DEVICE_REQUEST;
        }
        
        // Verify value size
        if (*OutBufferCb == 0)
        {
            *OutBufferCb = cbMinSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        if (*OutBufferCb < cbMinSize)
        {
            *OutBufferCb = 0;
            return STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            RtlCopyMemory(OutBuffer, CurrentValue, ParameterInfo->Size);
            *OutBufferCb = cbMinSize;
            return STATUS_SUCCESS;
        }
    }
    else if (Verb & KSPROPERTY_TYPE_SET)
    {
        *OutBufferCb = 0;
    
        // Verify it is a write prop.
        if (!(ParameterInfo->AccessFlags & KSPROPERTY_TYPE_SET))
        {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        // Validate parameter.
        if (!IsAudioModuleParameterValid(ParameterInfo, InBuffer, InBufferCb))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if (ParameterInfo->Size != 
                RtlCompareMemory(CurrentValue, InBuffer, ParameterInfo->Size))
        {
            RtlCopyMemory(CurrentValue, InBuffer, ParameterInfo->Size);
            *ParameterChanged = TRUE;
        }
        
        return STATUS_SUCCESS;
    }
    
    return STATUS_INVALID_DEVICE_REQUEST;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_ModulesListRequest(
    _In_ PPCPROPERTY_REQUEST    PropertyRequest,
    _In_ AUDIOMODULE *          AudioModules,
    _In_ ULONG                  AudioModuleCount
    )
{
    NTSTATUS ntStatus;
    
    // This specific APO->driver communication example is mainly added to show how this communication is done.
    // The module list only lives on the wave filter and it can have modules that are for all pins and some that 
    // are only on specific pins.

    PAGED_CODE();

    DPF_ENTER(("[AudioModule_PropertyHandlerModulesListRequest]"));

    //
    // Note: return an empty list when no modules are present.
    //
    
    // Handle KSPROPERTY_TYPE_BASICSUPPORT query
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
        return PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        PKSMULTIPLE_ITEM ksMultipleItem;
        ULONG cbMinSize = 0;
        PKSAUDIOMODULE_DESCRIPTOR modules = NULL;
        
        //
        // Compute min value size requirements
        //
        ntStatus = RtlULongMult(AudioModuleCount, 
                                (ULONG)sizeof(KSAUDIOMODULE_DESCRIPTOR), 
                                &cbMinSize);
        if (!NT_SUCCESS(ntStatus))
        {
            ASSERT(FALSE);
            return ntStatus;
        }
        
        ntStatus = RtlULongAdd(cbMinSize, sizeof(KSMULTIPLE_ITEM), &cbMinSize);
        if (!NT_SUCCESS(ntStatus))
        {
            ASSERT(FALSE);
            return ntStatus;
        }

        // Verify value size
        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize < cbMinSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
        
        // Value is a KSMULTIPLE_ITEM followed by list of modules.
        ksMultipleItem = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
        modules = (PKSAUDIOMODULE_DESCRIPTOR)(ksMultipleItem + 1);

        // Copy modules  
        for (ULONG i = 0; i<AudioModuleCount; ++i, ++modules)
        {
            const AUDIOMODULE_DESCRIPTOR * moduleDesc = AudioModules[i].Descriptor;
            
            modules->ClassId    = *moduleDesc->ClassId;
            modules->InstanceId = AudioModules[i].InstanceId;
            
            modules->VersionMajor = moduleDesc->VersionMajor;
            modules->VersionMinor = moduleDesc->VersionMinor;

            
            RtlStringCchCopyW(modules->Name,     
                              AUDIOMODULE_MAX_NAME_CCH_SIZE, // includes null char
                              moduleDesc->Name);
        }
        
        // Miniport filled in the list of modules. Fill in the KSMULTIPLE_ITEM header.
        ksMultipleItem->Size = cbMinSize;
        ksMultipleItem->Count = AudioModuleCount;

        PropertyRequest->ValueSize = ksMultipleItem->Size;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_DEVICE_REQUEST;
} // AudioModule_GenericHandler_ModulesListRequest

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_ModuleCommand(
    _In_ PPCPROPERTY_REQUEST    PropertyRequest,
    _In_ AUDIOMODULE *          AudioModules,
    _In_ ULONG                  AudioModuleCount
    )
{
    AUDIOMODULE *   module = NULL;
    
    PAGED_CODE();

    DPF_ENTER(("[AudioModule_GenericHandler_ModuleCommand]"));
    
    //
    // Basic parameter validation.
    //
    if (AudioModules == NULL || AudioModuleCount == 0)
    {
        // endpoint/stream doesn't have any modules.
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    // Verify instance data stores at least KSAUDIOMODULE_PROPERTY fields 
    // beyond KSPPROPERTY.
    if (PropertyRequest->InstanceSize < (sizeof(KSAUDIOMODULE_PROPERTY) - 
            RTL_SIZEOF_THROUGH_FIELD(KSAUDIOMODULE_PROPERTY, Property)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Extract property descriptor from property request instance data
    PKSAUDIOMODULE_PROPERTY moduleProperty = CONTAINING_RECORD(
        PropertyRequest->Instance, KSAUDIOMODULE_PROPERTY, ClassId);

    // Get a ptr to the module.
    module = AudioModule_FindModuleInList(AudioModules, 
                                          AudioModuleCount, 
                                          &moduleProperty->ClassId,
                                          moduleProperty->InstanceId);
    //
    // Invoke the handler.
    //
    if (module == NULL || 
        module->Descriptor == NULL || 
        module->Descriptor->Handler == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Handle KSPROPERTY_TYPE_BASICSUPPORT query
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;

        return PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }

    // Only KS-get is supported.
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        NTSTATUS    status;
        ULONG       inBufferCb = 0;
        ULONG       outBufferCb = 0;
        PVOID       inBuffer = NULL;
        PVOID       outBuffer = NULL;

        outBufferCb = PropertyRequest->ValueSize;
        outBuffer = PropertyRequest->Value;
        
        ASSERT((outBufferCb != 0 && outBuffer != NULL) ||
               (outBufferCb == 0 && outBuffer == NULL));
        
        // this is guaranteed to work b/c of the validation above.
        inBufferCb = PropertyRequest->InstanceSize + 
                     sizeof(KSPROPERTY) -
                     sizeof(KSAUDIOMODULE_PROPERTY);

        if (inBufferCb != 0)
        {
            inBuffer = (PVOID)(moduleProperty+1);
        }

        status =  module->Descriptor->Handler(module->Context,
                                              inBuffer,
                                              inBufferCb,
                                              outBuffer,
                                              &outBufferCb);
        //
        // Set the size of the returned output data, or in the case of
        // buffer overflow error, return the expected buffer length.
        //
        PropertyRequest->ValueSize = outBufferCb;
        return status;
    }

    PropertyRequest->ValueSize = 0;
    return STATUS_INVALID_DEVICE_REQUEST;
} // AudioModule_GenericHandler_ModuleCommand

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
AudioModule_GenericHandler_ModuleNotificationDeviceId(
    _In_ PPCPROPERTY_REQUEST    PropertyRequest,
    _In_ const GUID *           NotificationDeviceId
    )
{
    PAGED_CODE();

    DPF_ENTER(("[AudioModule_GenericHandler_ModuleNotificationDeviceId]"));

    //
    // Basic parameter validation.
    //
    if (NotificationDeviceId == NULL)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Handle KSPROPERTY_TYPE_BASICSUPPORT query
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ULONG flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;

        return PropertyHandler_BasicSupport(PropertyRequest, flags, VT_ILLEGAL);
    }

    ULONG cbMinSize = sizeof(GUID);
    GUID *dataPtr = static_cast<GUID*>(PropertyRequest->Value);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        // Verify value size
        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize < cbMinSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            *dataPtr = *NotificationDeviceId;
            PropertyRequest->ValueSize = cbMinSize;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
AudioModule_SendNotification(
    _In_ PPORTCLSNOTIFICATIONS          PortNotifications,
    _In_ PVOID                          NotificationBuffer, 
    _In_ USHORT                         NotificationBufferCb
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    PPCNOTIFICATION_BUFFER  buffer = NULL;
    
    PAGED_CODE();

    DPF_ENTER(("[AudioModule_SendNotification]"));

    // Allocate a notification buffer.
    status = PortNotifications->AllocNotificationBuffer(PagedPool,
                                                        NotificationBufferCb,
                                                        &buffer);
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    // Notification buffer is only guaranteed to be LONG aligned,
    // it is received as ULONGLONG aligned on the receiving end.
    RtlCopyMemory(buffer, NotificationBuffer, NotificationBufferCb);    

    //
    // Generate notification (async).
    //
    PortNotifications->SendNotification(&KSNOTIFICATIONID_AudioModule, buffer);

exit:
    if (buffer != NULL)
    {
        // Free notification buffer.
        PortNotifications->FreeNotificationBuffer(buffer);
        buffer = NULL;
    }
}


