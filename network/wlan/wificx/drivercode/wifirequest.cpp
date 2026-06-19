// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "precomp.h"
#include "wifitransition.h"
#include "wifirequest.h"
#include "wifirequest.tmh"

_Use_decl_annotations_
void EvtWifiDeviceSendCommand(WDFDEVICE Device, WIFIREQUEST SendRequest)
{
    UINT InBufferLen = 0;
    UINT OutBufferLen = 0;
    void* Buffer = WifiRequestGetInOutBuffer(SendRequest, &InBufferLen, &OutBufferLen);
    UINT16 MessageId = WifiRequestGetMessageId(SendRequest);

    TransitionContext tctx{
        Device,
        WifiGetIhvDeviceContext(Device),
        SendRequest,
        static_cast<PWDI_MESSAGE_HEADER>(Buffer),
        Buffer,
        InBufferLen,
        OutBufferLen
    };

    if(!NT_SUCCESS(RunTransitionByMessage(tctx, MessageId)))
    {
        WFCError("RunTransitionByMessage failed for MessageId: 0x%04X", MessageId);
        return;
    }
}

void WifiIhvSendIndicationToOs(
    _In_ WDFDEVICE Device, 
    _In_ const PWDI_MESSAGE_HEADER pOriginalWdiHeader, 
    _In_ UINT16 WifiRequestMessageId, 
    _In_ UINT32 WifiRequestTransactionId, 
    _In_ NTSTATUS WifiRquestM4Status,
    _In_opt_bytecount_(TlvDataSize) PUCHAR pTlvData,
    _In_ UINT32 TlvDataSize)
{
    WDFMEMORY data = WDF_NO_HANDLE;
    PUCHAR pIndicationBuffer = nullptr;
    PWDI_MESSAGE_HEADER pIndicationHeader = nullptr;
    SIZE_T indicationSize = sizeof(WDI_MESSAGE_HEADER) + TlvDataSize;
    WDF_OBJECT_ATTRIBUTES objectAttribs;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
    objectAttribs.ParentObject = Device;

    if(!NT_SUCCESS(WdfMemoryCreate(&objectAttribs, NonPagedPoolNx, 0, indicationSize, &data, reinterpret_cast<void**>(&pIndicationBuffer))))
    {
        WFCError("Failed to create indication buffer");
        return;
    }

    RtlZeroMemory(pIndicationBuffer, indicationSize);
    pIndicationHeader = reinterpret_cast<PWDI_MESSAGE_HEADER>(pIndicationBuffer);
    pIndicationHeader->PortId = pOriginalWdiHeader->PortId;
    pIndicationHeader->Reserved = pOriginalWdiHeader->Reserved;
    pIndicationHeader->Status = Wifi::ConvertNDISSTATUSToNTSTATUS(WifiRquestM4Status);
    pIndicationHeader->TransactionId = WifiRequestTransactionId;
    pIndicationHeader->IhvSpecificId = pOriginalWdiHeader->IhvSpecificId;

    if (TlvDataSize > 0 && pTlvData != nullptr)
    {
        RtlCopyMemory(pIndicationBuffer + sizeof(WDI_MESSAGE_HEADER), pTlvData, TlvDataSize);
    }

    // Send the indication up to WifiCx
    WifiDeviceReceiveIndication(Device, WifiRequestMessageId, data);

    // Don't need to keep this around
    WdfObjectDelete(data);
}

_Use_decl_annotations_
void WifiIhvSendUnsolicitedIndicationToOs(WDFDEVICE Device, PWDI_MESSAGE_HEADER pWdiHeader, UINT16 MessageId, PUCHAR pTlvData, UINT32 TlvDataSize)
{
    WifiIhvSendIndicationToOs(Device, pWdiHeader, MessageId, 0, STATUS_SUCCESS, pTlvData, TlvDataSize); //TransactionId required to be 0 for unsolicited indications.
}
_Use_decl_annotations_
void WifiIhvNotifyM3Completion(WIFIREQUEST Request, NTSTATUS WifiRequestM3Status, UINT BytesWritten)
{
    if(!NT_SUCCESS(WifiRequestM3Status))
    {
        WFCError("WifiRequest M3 failed: %!STATUS!, BytesWritten: %d\n", WifiRequestM3Status, BytesWritten);
    }   
    // Report the M3 status back to OS
    // OS expects M3 before the M4
    WifiRequestComplete(Request, WifiRequestM3Status, BytesWritten);
}

_Use_decl_annotations_
void WifiIhvSendM4IndicationToOs(WDFDEVICE Device, UINT16 WifiRequestMessageId, const PWDI_MESSAGE_HEADER pWdiHeader, NTSTATUS WifiRequestM4Status)
{
    WifiIhvSendIndicationToOs(Device, pWdiHeader, WifiRequestMessageId, pWdiHeader->TransactionId, WifiRequestM4Status, nullptr, 0);
}
