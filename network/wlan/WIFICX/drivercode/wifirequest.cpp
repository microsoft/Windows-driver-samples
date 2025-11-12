// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "precomp.h"
#include "wifiHAL.h"
#include "wifirequest.h"
#include "wifirequest.tmh"

_Use_decl_annotations_
void EvtWifiDeviceSendCommand(WDFDEVICE Device, WIFIREQUEST SendRequest)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT InBufferLen = 0;
    UINT OutBufferLen = 0;
    UINT BytesWritten = 0;
    void* Buffer = WifiRequestGetInOutBuffer(SendRequest, &InBufferLen, &OutBufferLen);
    UINT16 MessageId = WifiRequestGetMessageId(SendRequest);

    Status = ProcessWifiRequest(Device, MessageId, Buffer, InBufferLen, OutBufferLen, &BytesWritten);

    if (Status != STATUS_PENDING)
    {
        WifiRequestComplete(SendRequest, Status, BytesWritten);
    }
    return;
}

_Use_decl_annotations_
void WifiIhvSendIndicationToOs(WDFDEVICE Device, PWDI_MESSAGE_HEADER pWdiHeader, UINT16 MessageId, UINT32 TransactionId, PUCHAR pTlvData, UINT32 TlvDataSize)
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

    RtlCopyMemory(pIndicationHeader, pWdiHeader, sizeof(WDI_MESSAGE_HEADER));
    pIndicationHeader->TransactionId = TransactionId;

    if (TlvDataSize > 0)
    {
        RtlCopyMemory(pIndicationBuffer + sizeof(WDI_MESSAGE_HEADER), pTlvData, TlvDataSize);
    }

    // Send the indication up to WifiCx
    WifiDeviceReceiveIndication(Device, MessageId, data);

    // Don't need to keep this around
    WdfObjectDelete(data);
}

_Use_decl_annotations_
static NTSTATUS ProcessWifiRequest(WDFDEVICE Device, UINT16 MessageId, void* Buffer, UINT InBufferLen, UINT OutBufferLen, UINT* pBytesWritten)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWIFI_IHV_DEVICE_CONTEXT deviceContext = WifiGetIhvDeviceContext(Device);
    PWDI_MESSAGE_HEADER pWdiHeader = static_cast<PWDI_MESSAGE_HEADER>(Buffer);
    UINT BytesWritten = sizeof(WDI_MESSAGE_HEADER);

    if (InBufferLen < sizeof(WDI_MESSAGE_HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (OutBufferLen < sizeof(WDI_MESSAGE_HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    auto const buffer = static_cast<UCHAR*>(Buffer);

    // Check for integer overflow
    if (!(buffer + InBufferLen > buffer))
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (MessageId)
    {
    case WDI_TASK_SET_RADIO_STATE:
    {
        WDI_SET_RADIO_STATE_PARAMETERS RadioStateParams = {};
        ULONG TLVStreamLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto TLVByteStream = buffer +sizeof(WDI_MESSAGE_HEADER);

        DumpMessageTlvByteStream(
            WDI_TASK_SET_RADIO_STATE, TRUE, deviceContext->TlvContext.PeerVersion, TLVStreamLength, TLVByteStream, 0, nullptr);

        WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
            Wifi::ConvertNDISSTATUSoNTSTATUS(
                ParseWdiTaskSetRadioState(
                    TLVStreamLength,
                    TLVByteStream,
                    &deviceContext->TlvContext,
                    &RadioStateParams)),
            "Set Radio State parsing failed");

        // No WX_RETURN here since we need to cleanup.
        ntStatus = deviceContext->wifiHAL->WifiIhvSetRadioState(
            Device, RadioStateParams, pWdiHeader, &deviceContext->TlvContext);
        if(!NT_SUCCESS(ntStatus)) 
        {
            WFCError("WifiIhvSetRadioState failed: %!STATUS!\n", ntStatus);
        }

        CleanupParsedWdiTaskSetRadioState(&RadioStateParams);
    }
    break;
    case WDI_TASK_SCAN:
    {
        ULONG scanDataLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto pScanData = buffer + sizeof(WDI_MESSAGE_HEADER);

        WDI_SCAN_PARAMETERS scanParams = {};

        WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
            Wifi::ConvertNDISSTATUSoNTSTATUS(
                ParseWdiTaskScan(
                    scanDataLength,
                    pScanData,
                    &deviceContext->TlvContext, 
                    &scanParams)),
                "Scan parameters parsing failed");

        // No WX_RETURN here since we need to cleanup.
        ntStatus = deviceContext->wifiHAL->WifiIhvScan(
            Device, scanParams, pWdiHeader, &deviceContext->TlvContext);
        if (!NT_SUCCESS(ntStatus))
        {
            WFCError("WifiIhvSetRadioState failed: %!STATUS!\n", ntStatus);
        }

        CleanupParsedWdiTaskScan(&scanParams);
    }
    break;

    case WDI_TASK_DOT11_RESET:
    {
        WDI_TASK_DOT11_RESET_PARAMETERS resetParameters;

        ULONG resetDataLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto pResetData = buffer + sizeof(WDI_MESSAGE_HEADER);

        WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
            Wifi::ConvertNDISSTATUSoNTSTATUS(ParseWdiTaskDot11Reset(resetDataLength, pResetData, &deviceContext->TlvContext, &resetParameters)),
                "DOT11 Reset parameters parsing failed");

        // No WX_RETURN here since we need to cleanup.
        ntStatus = deviceContext->wifiHAL->WifiIhvReset(
            Device, resetParameters, pWdiHeader, &deviceContext->TlvContext);
        if (!NT_SUCCESS(ntStatus))
        {
            WFCError("WifiIhvReset failed: %!STATUS!\n", ntStatus);
        }

        CleanupParsedWdiTaskDot11Reset(&resetParameters);
    }
    break;

    case WDI_TASK_CONNECT:
    {
        ULONG connectDataLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto pConnectData = buffer + sizeof(WDI_MESSAGE_HEADER);

        DumpMessageTlvByteStream(WDI_TASK_CONNECT, TRUE, deviceContext->TlvContext.PeerVersion, connectDataLength, pConnectData, 0, nullptr);

        WDI_TASK_CONNECT_PARAMETERS connectParams{};


        // We search for the BSSID pattern to figure out what AP we are trying to connect to
        WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
            Wifi::ConvertNDISSTATUSoNTSTATUS(ParseWdiTaskConnect(connectDataLength, pConnectData, &deviceContext->TlvContext, &connectParams)),
                "Connect parameters parsing failed");
        
        // No WX_RETURN here since we need to cleanup.
        ntStatus = deviceContext->wifiHAL->WifiIhvConnect(
            Device, connectParams, pWdiHeader, &deviceContext->TlvContext);
        if (!NT_SUCCESS(ntStatus))
        {
            WFCError("WifiIhvConnect failed: %!STATUS!\n", ntStatus);
        }

        CleanupParsedWdiTaskConnect(&connectParams);
    }
    break;

    case WDI_TASK_DISCONNECT:
    {
        deviceContext->wifiHAL->WifiIhvPerformDisassociation(Device, pWdiHeader, WDI_ASSOC_STATUS_DISASSOCIATED_BY_HOST);
    }
    break;

    case WDI_SET_PRIVACY_EXEMPTION_LIST:
    case WDI_SET_DEFAULT_KEY_ID:
    case WDI_SET_ADD_CIPHER_KEYS:
    case WDI_SET_DELETE_CIPHER_KEYS:
    case WDI_SET_RECEIVE_PACKET_FILTER:
    case WDI_SET_CONNECTION_QUALITY:
    case WDI_SET_ADAPTER_CONFIGURATION:
        // Doing Nothing special
        break;

    case WDI_SET_SAE_AUTH_PARAMS:
    {
        WDI_SET_SAE_AUTH_PARAMS_COMMAND setSAEAuthParams;
        
        WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
            Wifi::ConvertNDISSTATUSoNTSTATUS(ParseWdiSetSaeAuthParams(
            InBufferLen - sizeof(WDI_MESSAGE_HEADER), (PUINT8)buffer + sizeof(WDI_MESSAGE_HEADER), &deviceContext->TlvContext, &setSAEAuthParams)),
            "Failed to parse WDI_SET_SAE_AUTH_PARAMS");

        // No WX_RETURN here since we need to cleanup.
        ntStatus = deviceContext->wifiHAL->WifiIhvSetSaeAuthParams(
            Device, setSAEAuthParams, pWdiHeader, &deviceContext->TlvContext);
        if (!NT_SUCCESS(ntStatus))
        {
            WFCError("WifiIhvSetSaeAuthParams failed: %!STATUS!\n", ntStatus);
        }

        CleanupParsedWdiSetSaeAuthParams(&setSAEAuthParams);
    }
    break;

    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        break;
    }

    // Set the output length
    if (pBytesWritten)
    {
        *pBytesWritten = BytesWritten;
    }
    return ntStatus;
}
