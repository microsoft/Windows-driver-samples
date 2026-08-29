/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    AudioModule.cpp

Abstract:

    Implementation of general purpose audio module property handlers

Environment:

    Kernel mode

--*/

#include "private.h"
#include "audiomodule.h"
#include "stdunk.h"
#include <ks.h>

AUDIOMODULE_PARAMETER_INFO  AudioModule_ParameterInfo[] =
{
    {
        ACX_PROPERTY_ITEM_FLAG_GET | ACX_PROPERTY_ITEM_FLAG_SET | ACX_PROPERTY_ITEM_FLAG_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE_CONTEXT, Parameter1),
        VT_UI4,
        AudioModule_ValidParameterList,
        SIZEOF_ARRAY(AudioModule_ValidParameterList)
    },
    {
        ACX_PROPERTY_ITEM_FLAG_GET | ACX_PROPERTY_ITEM_FLAG_BASICSUPPORT,
        0, 
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE_CONTEXT, Parameter2),
        VT_UI1,
        NULL,
        0
    },
};

AUDIOMODULE_PARAMETER_INFO  AudioModule0_ParameterInfo[] =
{
    {
        ACX_PROPERTY_ITEM_FLAG_GET | ACX_PROPERTY_ITEM_FLAG_SET | ACX_PROPERTY_ITEM_FLAG_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE0_CONTEXT, Parameter1),
        VT_UI4,
        AudioModule0_ValidParameterList,
        SIZEOF_ARRAY(AudioModule0_ValidParameterList)
    },
    {
        ACX_PROPERTY_ITEM_FLAG_GET | ACX_PROPERTY_ITEM_FLAG_BASICSUPPORT,
        0, 
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE0_CONTEXT, Parameter2),
        VT_UI1,
        NULL,
        0
    },
};

AUDIOMODULE_PARAMETER_INFO  AudioModule1_ParameterInfo[] =
{
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE1_CONTEXT, Parameter1),
        VT_UI1,
        AudioModule1_ValidParameterList,
        SIZEOF_ARRAY(AudioModule1_ValidParameterList)
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        0,
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE1_CONTEXT, Parameter2),
        VT_UI8,
        NULL,
        0
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE1_CONTEXT, Parameter3),
        VT_UI4,
        NULL,
        0
    },
};

AUDIOMODULE_PARAMETER_INFO  AudioModule2_ParameterInfo[] =
{
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        AUDIOMODULE_PARAMETER_FLAG_CHANGE_NOTIFICATION,
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE2_CONTEXT, Parameter1),
        VT_UI4,
        AudioModule2_ValidParameterList,
        SIZEOF_ARRAY(AudioModule2_ValidParameterList)
    },
    {
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        0, 
        (ULONG)RTL_FIELD_SIZE(SDCAXU_AUDIOMODULE2_CONTEXT, Parameter2),
        VT_UI2,
        NULL,
        0
    },
};


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
} 

#pragma code_seg("PAGE")
BOOLEAN
IsAudioModuleParameterValid(
    _In_ PAUDIOMODULE_PARAMETER_INFO        ParameterInfo,
    _In_reads_bytes_opt_(BufferCb) PVOID    Buffer,
    _In_  ULONG                             BufferCb
    )
{
    PAGED_CODE();
    
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
