// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

EVT_WIFI_DEVICE_SEND_COMMAND EvtWifiDeviceSendCommand;

static NTSTATUS ProcessWifiRequest(
    _In_ WDFDEVICE Device, _In_ UINT16 MessageId, _In_ void* Buffer, _In_ UINT InBufferLen, _In_ UINT OutBufferLen, _In_ UINT* pBytesWritten);

UINT WifiCxTestSetOutput(
    _In_ PWIFI_DEVICE_CONTEXT pDeviceContext,
    _Inout_ void* ReqBuffer,
    _In_ SIZE_T ReqBufferSize,
    _In_opt_bytecount_(TlvDataSize) PUCHAR pTlvData,
    _In_ UINT32 TlvDataSize);

void WifiCxTestSendIndication(
    _In_ WDFDEVICE Device,
    _In_ PWDI_MESSAGE_HEADER pWdiHeader,
    _In_ UINT16 MessageId,
    _In_ UINT32 TransactionId,
    _In_opt_bytecount_(TlvDataSize) PUCHAR pTlvData,
    _In_ UINT32 TlvDataSize);

NDIS_STATUS WifiCxTestGetAdapterCapabilities(_In_ PWIFI_DEVICE_CONTEXT pDeviceContext, _In_ void* ReqBuffer, _In_ SIZE_T ReqBufferSize);

NDIS_STATUS WifiCxTestSetAdapterConfiguration(_In_ PWIFI_DEVICE_CONTEXT pDeviceContext, _In_ void* ReqBuffer, _In_ SIZE_T ReqBufferSize);

NTSTATUS WifiCxTestSetDeviceCapabilities(_In_ WDFDEVICE Device);

ULONG g_SupportedBands = (WDI_BAND_ID_2400 | WDI_BAND_ID_5000 | WDI_BAND_ID_6000);