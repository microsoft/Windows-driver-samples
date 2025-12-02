#pragma once 

#define IS_2_POW_N(X)           (((X)&(X-1)) == 0)

enum class aligned_size_t : size_t {};

void * AllocateAligned(size_t cb, aligned_size_t AlignmentMask)
{
    ASSERT(0 != cb);
    //ASSERT(IS_2_POW_N(AlignmentMask + 1));
    
    void *res = _aligned_malloc(cb, static_cast<size_t>(AlignmentMask) + 1);

    return res;
}

void DeallocateAligned(void * p, aligned_size_t /* AlignmentMask */)
{
    if (NULL != p)
    {
        _aligned_free(p);
    }
}

UINT32 CalculateAlignmentMask(APO_CONNECTION_DESCRIPTOR * /* pDescriptor */, UINT32 u32BytesPerFrame)
{
    const UINT32 u32AudioAlignment = sizeof(PVOID);
    UINT32 u32Alignment = u32AudioAlignment - 1;

    // Calculate the new alignment requirement for this buffer.
    if (u32BytesPerFrame > u32AudioAlignment)
    {
        // Use the frame size directly if it is a power of two.
        if (IS_2_POW_N(u32BytesPerFrame))
        {
            u32Alignment = u32BytesPerFrame - 1;
        }
        // The new alignment size should be greater than frame 
        // size and should be a power of 2.
        else
        {
            u32Alignment = u32AudioAlignment;
            while (u32BytesPerFrame / u32Alignment)
            {
                u32Alignment *= 2;
            }
            u32Alignment = u32Alignment - 1;
        }
    }

    // Use default alignment if the frame size is less than the default 
    // alignment.
    return u32Alignment; 
} // CalculateAlignmentMask

HRESULT
CreateConnection(APO_CONNECTION_DESCRIPTOR* pDescriptor, APO_CONNECTION_PROPERTY* pProperty)
{
    RETURN_HR_IF_NULL(E_POINTER, pDescriptor);
    RETURN_HR_IF_NULL(E_POINTER, pDescriptor->pFormat);
    RETURN_HR_IF_NULL(E_POINTER, pProperty);

    UINT32 u32BytesPerFrame;
    unsigned byteAlignmentMask;
    BYTE*  pAllocatedBuffer;
    UNCOMPRESSEDAUDIOFORMAT Format;

    RETURN_IF_FAILED(pDescriptor->pFormat->GetUncompressedAudioFormat(&Format));

    u32BytesPerFrame = 
        Format.dwSamplesPerFrame * Format.dwBytesPerSampleContainer;

    // Allocate buffer internally.
    if (NULL == pDescriptor->pBuffer)
    {
        byteAlignmentMask = CalculateAlignmentMask(pDescriptor, u32BytesPerFrame);

        pAllocatedBuffer = (BYTE *)AllocateAligned(pDescriptor->u32MaxFrameCount * u32BytesPerFrame, static_cast<aligned_size_t>(byteAlignmentMask));
        RETURN_HR_IF_NULL(E_OUTOFMEMORY, pAllocatedBuffer);

        pDescriptor->pBuffer = reinterpret_cast<UINT_PTR>(pAllocatedBuffer);
        pDescriptor->Type = APO_CONNECTION_BUFFER_TYPE_ALLOCATED;
    }
    // Use the given buffer.
    else
    {
        pDescriptor->Type = APO_CONNECTION_BUFFER_TYPE_EXTERNAL;
    }
    
    pProperty->pBuffer = pDescriptor->pBuffer;
    pProperty->u32ValidFrameCount = 0;
    pProperty->u32Signature = APO_CONNECTION_PROPERTY_SIGNATURE;

    return S_OK;
} // CreateConnection

HRESULT
DestroyConnection(APO_CONNECTION_DESCRIPTOR *pDescriptor)
{
    HRESULT hResult;
    UINT32 u32BytesPerFrame;
    size_t byteAlignmentMask;
    BYTE*  pAllocatedBuffer;
    UNCOMPRESSEDAUDIOFORMAT Format;

    hResult = pDescriptor->pFormat->GetUncompressedAudioFormat(&Format);
    //IF_FAILED_JUMP(hResult, Exit);

    if (APO_CONNECTION_BUFFER_TYPE_ALLOCATED == pDescriptor->Type)
    {
        u32BytesPerFrame = 
            Format.dwSamplesPerFrame * Format.dwBytesPerSampleContainer;

        byteAlignmentMask = CalculateAlignmentMask(pDescriptor, u32BytesPerFrame);

        if (pDescriptor->pBuffer)
        {
            pAllocatedBuffer = reinterpret_cast<BYTE*>(pDescriptor->pBuffer);
        	RETURN_HR_IF_NULL(E_OUTOFMEMORY, pAllocatedBuffer);

            DeallocateAligned((void *) pAllocatedBuffer, static_cast<aligned_size_t>(byteAlignmentMask));
        }
    }

    pDescriptor->pFormat->Release();
    pDescriptor->pFormat = NULL;

    return hResult;
} // DestroyConnection
