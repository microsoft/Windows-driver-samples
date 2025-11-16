// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once
#include "device.h"

EVT_WIFI_DEVICE_SEND_COMMAND EvtWifiDeviceSendCommand;

void WifiIhvSendUnsolicitedIndicationToOs(
    _In_ WDFDEVICE Device,
    _In_ PWDI_MESSAGE_HEADER pWdiHeader,
    _In_ UINT16 MessageId,
    _In_opt_bytecount_(TlvDataSize) PUCHAR pTlvData,
    _In_ UINT32 TlvDataSize);

void WifiIhvNotifyM3Completion(
    _In_ WIFIREQUEST Request,
    _In_ NTSTATUS WifiRequestM3Status,
    _In_ UINT BytesWritten);

void WifiIhvSendM4IndicationToOs(
    _In_ WDFDEVICE Device,
    _In_ UINT16 WifiRequestMessageId,
    _In_ const PWDI_MESSAGE_HEADER pOriginalWdiHeader,
    _In_ NTSTATUS WifiRequestM4Status);