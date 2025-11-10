//-------------------------------------------------------------------------------
// Net Adapter source file
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "precomp.h"
#include "testdata.h"
#include <ndis\status.h>
#include "wifirequest.h"
#include "wifirequest.tmh"

// clang-format off


WdiTestConnectEntry g_ConnectEntries[] =
{
    // 0
    {                                               // 0th Entry is disconnected state
        0,
        nullptr,
        nullptr, 0,
        nullptr, 0,
    },

    // 1 - WFC_OPEN
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr,
        s_TLV_BSS_Entry_1, sizeof(s_TLV_BSS_Entry_1),
        s_TLV_SuccessOpenAssociationResult, sizeof(s_TLV_SuccessOpenAssociationResult),
    },

    // 2 - WFC_OPEN
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_2_Open,
        s_TLV_BSS_Entry_2_Open, sizeof(s_TLV_BSS_Entry_2_Open),
        s_TLV_Success_AssociationResult_2_Open, sizeof(s_TLV_Success_AssociationResult_2_Open),
    },

    // 3 - WDI__WEP
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_3_WEP,
        s_TLV_BSS_Entry_3_WEP, sizeof(s_TLV_BSS_Entry_3_WEP),
        s_TLV_Success_AssociationResult_3_WEP, sizeof(s_TLV_Success_AssociationResult_3_WEP),
    },

    // 4 - WDI_SECURE
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_4_RSNA_CCMP,
        s_TLV_BSS_Entry_4_RSNA_CCMP, sizeof(s_TLV_BSS_Entry_4_RSNA_CCMP),
        s_TLV_Success_AssociationResult_4_RSNA_CCMP, sizeof(s_TLV_Success_AssociationResult_4_RSNA_CCMP),
    },

    // 5 - WDI__IHV
    {
        TESTMP_BAND_IHV,
        &s_Connect_Addr_5_IHV,
        s_TLV_BSS_Entry_5_IHV, sizeof(s_TLV_BSS_Entry_5_IHV),
        s_TLV_Success_AssociationResult_5_IHV, sizeof(s_TLV_Success_AssociationResult_5_IHV),
    },

    // 6 - WDI__FT
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_6_FT_CCMP,
        s_TLV_BSS_Entry_6_FT_CCMP, sizeof(s_TLV_BSS_Entry_6_FT_CCMP),
        s_TLV_Success_AssociationResult_6_FT_CCMP, sizeof(s_TLV_Success_AssociationResult_6_FT_CCMP),
    },

    // 7 - WDI__FTPSK
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_7_FT_PSK_CCMP,
        s_TLV_BSS_Entry_7_FT_PSK_CCMP, sizeof(s_TLV_BSS_Entry_7_FT_PSK_CCMP),
        s_TLV_Success_AssociationResult_7_FT_PSK_CCMP, sizeof(s_TLV_Success_AssociationResult_7_FT_PSK_CCMP),
    },

    // 8 -
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_8_Hidden,
        s_TLV_BSS_Entry_Beacon_8_Hidden, sizeof(s_TLV_BSS_Entry_Beacon_8_Hidden),
        s_TLV_Success_AssociationResult_8_Hidden, sizeof(s_TLV_Success_AssociationResult_8_Hidden),
    },

    // 9 - WDI_adPSK
    {
        WDI_BAND_ID_60000,
        &s_Connect_Addr_9_11ad_PSK,
#ifdef DMG_CAPABILITIES
        s_TLV_BSS_Entry_9_11ad_ProbeResponse_Dmg_PSK, sizeof(s_TLV_BSS_Entry_9_11ad_ProbeResponse_Dmg_PSK),
        s_TLV_Success_AssociationResult_9_11ad_Dmg_PSK, sizeof(s_TLV_Success_AssociationResult_9_11ad_Dmg_PSK),
#else
        s_TLV_BSS_Entry_9_11ad_ProbeResponse_PSK, sizeof(s_TLV_BSS_Entry_9_11ad_ProbeResponse_PSK),
        s_TLV_Success_AssociationResult_9_11ad_PSK, sizeof(s_TLV_Success_AssociationResult_9_11ad_PSK),
#endif // DMG_CAPABILITIES
    },

    // 10 - WDI_ad_1x
    {
        WDI_BAND_ID_60000,
        &s_Connect_Addr_10_11ad_1x,
#ifdef DMG_CAPABILITIES
        s_TLV_BSS_Entry_10_11ad_ProbeResponse_Dmg_1x, sizeof(s_TLV_BSS_Entry_10_11ad_ProbeResponse_Dmg_1x),
        s_TLV_Success_AssociationResult_10_11ad_Dmg_1x, sizeof(s_TLV_Success_AssociationResult_10_11ad_Dmg_1x),
#else
        s_TLV_BSS_Entry_10_11ad_ProbeResponse_1x, sizeof(s_TLV_BSS_Entry_10_11ad_ProbeResponse_1x),
        s_TLV_Success_AssociationResult_10_11ad_1x, sizeof(s_TLV_Success_AssociationResult_10_11ad_1x),
#endif // DMG_CAPABILITIES
    },

    // 11 - WDI_ad_ON
    {
        WDI_BAND_ID_60000,
        &s_Connect_Addr_11_11ad_Open,
#ifdef DMG_CAPABILITIES
        s_TLV_BSS_Entry_11_11ad_ProbeResponse_Dmg_Open, sizeof(s_TLV_BSS_Entry_11_11ad_ProbeResponse_Dmg_Open),
        s_TLV_Success_AssociationResult_11_11ad_Dmg_Open, sizeof(s_TLV_Success_AssociationResult_11_11ad_Dmg_Open),
#else
        s_TLV_BSS_Entry_11_11ad_ProbeResponse_Open, sizeof(s_TLV_BSS_Entry_11_11ad_ProbeResponse_Open),
        s_TLV_Success_AssociationResult_11_11ad_Open, sizeof(s_TLV_Success_AssociationResult_11_11ad_Open),
#endif // DMG_CAPABILITIES
    },

    // 12 - WDI_OPEN_11ax.2.4
    {
        WDI_BAND_ID_60000,
        &s_Connect_Addr_12_11ax_24_Open,
        s_TLV_BSS_Entry_12_11ax_24_Open, sizeof(s_TLV_BSS_Entry_12_11ax_24_Open),
        s_TLV_Success_AssociationResult_12_11ax_24_Open, sizeof(s_TLV_Success_AssociationResult_12_11ax_24_Open),
    },

    // 13 - WDI_OPEN_11ax.5
    {
        WDI_BAND_ID_5000,
        &s_Connect_Addr_13_11ax_5_Open,
        s_TLV_BSS_Entry_13_11ax_5_Open, sizeof(s_TLV_BSS_Entry_13_11ax_5_Open),
        s_TLV_Success_AssociationResult_13_11ax_5_Open, sizeof(s_TLV_Success_AssociationResult_13_11ax_5_Open),
    },

    // 14 - WDI_WPA3-SAE
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_14_WPA3_SAE_CCMP,
        s_TLV_BSS_Entry_14_WPA3_SAE_CCMP, sizeof(s_TLV_BSS_Entry_14_WPA3_SAE_CCMP),
        s_TLV_Success_AssociationResult_14_WPA3_SAE_CCMP, sizeof(s_TLV_Success_AssociationResult_14_WPA3_SAE_CCMP),
    },

    // 15 - WDI_SHA256
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_15_WPA2PSK_SHA256,
        s_TLV_BSS_Entry_15_WPA2PSK_SHA256, sizeof(s_TLV_BSS_Entry_15_WPA2PSK_SHA256),
        s_TLV_Success_AssociationResult_15_WPA2PSK_SHA256, sizeof(s_TLV_Success_AssociationResult_15_WPA2PSK_SHA256),
    },

    // 16 - WDI_WPA3-SUITE_B
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_16_WPA3_SUITEB,
        s_TLV_BSS_Entry_16_WPA3_SUITEB, sizeof(s_TLV_BSS_Entry_16_WPA3_SUITEB),
        s_TLV_Success_AssociationResult_16_WPA3_SUITEB, sizeof(s_TLV_Success_AssociationResult_16_WPA3_SUITEB),
    },

    // 17 - 6E__1
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_17_6G_S1_2_4_Ghz,
        s_TLV_BSS_Entry_17_6G_S1_2_4_Ghz, sizeof(s_TLV_BSS_Entry_17_6G_S1_2_4_Ghz),
        s_TLV_Success_AssociationResult_17_6G_S1_2_4_Ghz, sizeof(s_TLV_Success_AssociationResult_17_6G_S1_2_4_Ghz),
    },

    // 18 - 6E__1
    {
        WDI_BAND_ID_5000,
        &s_Connect_Addr_18_6G_S1_5_Ghz,
        s_TLV_BSS_Entry_18_6G_S1_5_Ghz, sizeof(s_TLV_BSS_Entry_18_6G_S1_5_Ghz),
        s_TLV_Success_AssociationResult_18_6G_S1_5_Ghz, sizeof(s_TLV_Success_AssociationResult_18_6G_S1_5_Ghz),
    },

    // 19 - 6E__1
    {
        WDI_BAND_ID_6000,
        &s_Connect_Addr_19_6G_S1a_6_Ghz,
        s_TLV_BSS_Entry_19_6G_S1a_6_Ghz, sizeof(s_TLV_BSS_Entry_19_6G_S1a_6_Ghz),
        s_TLV_Success_AssociationResult_19_6G_S1a_6_Ghz, sizeof(s_TLV_Success_AssociationResult_19_6G_S1a_6_Ghz),
    },

    // 20 - 6E__1
    {
        WDI_BAND_ID_6000,
        &s_Connect_Addr_20_6G_S1b_6_Ghz,
        s_TLV_BSS_Entry_20_6G_S1b_6_Ghz, sizeof(s_TLV_BSS_Entry_20_6G_S1b_6_Ghz),
        s_TLV_Success_AssociationResult_20_6G_S1b_6_Ghz, sizeof(s_TLV_Success_AssociationResult_20_6G_S1b_6_Ghz),
    },

    // 21 - 6E__2
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_21_6G_S2_2_4_Ghz,
        s_TLV_BSS_Entry_21_6G_S2_2_4_Ghz, sizeof(s_TLV_BSS_Entry_21_6G_S2_2_4_Ghz),
        s_TLV_Success_AssociationResult_21_6G_S2_2_4_Ghz, sizeof(s_TLV_Success_AssociationResult_21_6G_S2_2_4_Ghz),
    },

    // 22 - 6E__2
    {
        WDI_BAND_ID_5000,
        &s_Connect_Addr_22_6G_S2_5_Ghz,
        s_TLV_BSS_Entry_22_6G_S2_5_Ghz, sizeof(s_TLV_BSS_Entry_22_6G_S2_5_Ghz),
        s_TLV_Success_AssociationResult_22_6G_S2_5_Ghz, sizeof(s_TLV_Success_AssociationResult_22_6G_S2_5_Ghz),
    },

    // 23 - 6E__2
    {
        WDI_BAND_ID_6000,
        &s_Connect_Addr_23_6G_S2_6_Ghz,
        s_TLV_BSS_Entry_23_6G_S2_6_Ghz, sizeof(s_TLV_BSS_Entry_23_6G_S2_6_Ghz),
        s_TLV_Success_AssociationResult_23_6G_S2_6_Ghz, sizeof(s_TLV_Success_AssociationResult_23_6G_S2_6_Ghz),
    },

    // 24 - 6E__3
    {
        WDI_BAND_ID_2400,
        &s_Connect_Addr_24_6G_S3_2_4_Ghz,
        s_TLV_BSS_Entry_24_6G_S3_2_4_Ghz, sizeof(s_TLV_BSS_Entry_24_6G_S3_2_4_Ghz),
        s_TLV_Success_AssociationResult_24_6G_S3_2_4_Ghz, sizeof(s_TLV_Success_AssociationResult_24_6G_S3_2_4_Ghz),
    },

    // 25 - 6E__4
    {
        WDI_BAND_ID_6000,
        &s_Connect_Addr_25_6G_S4_6_Ghz,
        s_TLV_BSS_Entry_25_6G_S4_6_Ghz, sizeof(s_TLV_BSS_Entry_25_6G_S4_6_Ghz),
        s_TLV_Success_AssociationResult_25_6G_S4_6_Ghz, sizeof(s_TLV_Success_AssociationResult_25_6G_S4_6_Ghz),
    },

    // 26 - WDI_OWE_RNR
    {
        WDI_BAND_ID_2400,
        &s_ConnectAddr_26_OWE_With_RNR,
        s_TLV_BSS_Entry_26_OWE_With_RNR, sizeof(s_TLV_BSS_Entry_26_OWE_With_RNR),
        s_TLV_Failure_AssociationResult_26_OWE_With_RNR, sizeof(s_TLV_Failure_AssociationResult_26_OWE_With_RNR)
    },
    // 27 - <WDI_OWE_TM_OPEN>
    {
        WDI_BAND_ID_2400,
        &s_ConnectAddr_27_OWE_TM_OWE,
        s_TLV_BSS_Entry_27_OWE_TM_OWE, sizeof(s_TLV_BSS_Entry_27_OWE_TM_OWE),
        s_TLV_Failure_AssociationResult_27_OWE_TM_OWE, sizeof(s_TLV_Failure_AssociationResult_27_OWE_TM_OWE)
    },
    // 28 - WDI_OWE_TM_OPEN
    {
        WDI_BAND_ID_2400,
        &s_ConnectAddr_28_OWE_TM_Open,
        s_TLV_BSS_Entry_28_OWE_TM_Open_Beacon, sizeof(s_TLV_BSS_Entry_28_OWE_TM_Open_Beacon),
        s_TLV_Failure_AssociationResult_28_OWE_TM_Open, sizeof(s_TLV_Failure_AssociationResult_28_OWE_TM_Open)
    },

    // 29 is only MLD address for Wi-Fi 7
    // 30 - Wi-Fi 7 - Mixed
    {
        WDI_BAND_ID_5000,
        &s_Connect_Addr_30_WiFi7_Mixed_Link_1,
        s_TLV_BSS_Entry_30_WiFi7_Mixed_Link_1, sizeof(s_TLV_BSS_Entry_30_WiFi7_Mixed_Link_1),
        s_TLV_Success_AssociationResult_30_WiFi7_Mixed_Link_1, sizeof(s_TLV_Success_AssociationResult_30_WiFi7_Mixed_Link_1),
    },
    // 34 - Wi-Fi 7 - Open
    {
        WDI_BAND_ID_5000,
        &s_Connect_Addr_34_WiFi7_Open_Link_1,
        s_TLV_BSS_Entry_34_WiFi7_Open_Link_1, sizeof(s_TLV_BSS_Entry_34_WiFi7_Open_Link_1),
        s_TLV_Success_AssociationResult_34_WiFi7_Open_Link_1, sizeof(s_TLV_Success_AssociationResult_34_WiFi7_Open_Link_1),
    },
    // 38 - Wi-Fi 7 - Only
    {
        WDI_BAND_ID_6000,
        &s_Connect_Addr_38_WiFi7_Only_Link_1,
        s_TLV_BSS_Entry_38_WiFi7_Only_Link_1, sizeof(s_TLV_BSS_Entry_38_WiFi7_Only_Link_1),
        s_TLV_Success_AssociationResult_38_WiFi7_Only_Link_1, sizeof(s_TLV_Success_AssociationResult_38_WiFi7_Only_Link_1),
    },

    // 41 - Dual-Sta
    {
        WDI_BAND_ID_5000,
        &s_Connect_Addr_41_DualSta_5Ghz,
        s_TLV_BSS_Entry_41_DualSta_5Ghz, sizeof(s_TLV_BSS_Entry_41_DualSta_5Ghz),
        s_TLV_SuccessOpenAssociationResult_41_DualSta_5Ghz, sizeof(s_TLV_SuccessOpenAssociationResult_41_DualSta_5Ghz),
    },

    // 42 - Dual-Sta
    {
        WDI_BAND_ID_6000,
        &s_Connect_Addr_42_DualSta_6Ghz,
        s_TLV_BSS_Entry_42_DualSta_6Ghz, sizeof(s_TLV_BSS_Entry_42_DualSta_6Ghz),
        s_TLV_SuccessOpenAssociationResult_42_DualSta_6Ghz, sizeof(s_TLV_SuccessOpenAssociationResult_42_DualSta_6Ghz),
    },
};

// clang-format on

#define ConnectEntryId_MAX ARRAYSIZE(g_ConnectEntries)

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
void WifiCxTestSendIndication(WDFDEVICE Device, PWDI_MESSAGE_HEADER pWdiHeader, UINT16 MessageId, UINT32 TransactionId, PUCHAR pTlvData, UINT32 TlvDataSize)
{
    NTSTATUS ntStatus;
    WDFMEMORY data;
    PUCHAR pIndicationBuffer = nullptr;
    PWDI_MESSAGE_HEADER pIndicationHeader = nullptr;
    SIZE_T indicationSize = sizeof(WDI_MESSAGE_HEADER) + TlvDataSize;
    WDF_OBJECT_ATTRIBUTES objectAttribs;

    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
    objectAttribs.ParentObject = Device;

    ntStatus = WdfMemoryCreate(&objectAttribs, NonPagedPoolNx, 0, indicationSize, &data, reinterpret_cast<void**>(&pIndicationBuffer));

    if (!NT_SUCCESS(ntStatus))
    {
        WFCError("%!FUNC!: Failed to initialize indication buffer %!STATUS!", ntStatus);
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
UINT WifiCxTestSetOutput(PWIFI_DEVICE_CONTEXT pDeviceContext, void* ReqBuffer, SIZE_T ReqBufferSize, PUCHAR pTlvData, UINT32 TlvDataSize)
{
    UNREFERENCED_PARAMETER(pDeviceContext);
    PWDI_MESSAGE_HEADER pWdiHeader = static_cast<PWDI_MESSAGE_HEADER>(ReqBuffer);
    UINT BytesWritten = 0;
    pWdiHeader->Status = NDIS_STATUS_SUCCESS; // TODO Need to set it to actual output

    if (ReqBufferSize >= (TlvDataSize + sizeof(WDI_MESSAGE_HEADER)))
    {
        if (TlvDataSize > 0)
        {
            RtlCopyMemory((static_cast<UCHAR*>(ReqBuffer)) + sizeof(WDI_MESSAGE_HEADER), pTlvData, TlvDataSize);
        }
        BytesWritten = static_cast<UINT16>(TlvDataSize);
    }
    else
    {
        pWdiHeader->Status = NDIS_STATUS_BUFFER_OVERFLOW;
    }
    return BytesWritten;
}

_Use_decl_annotations_
NDIS_STATUS WifiCxTestGetAdapterCapabilities(PWIFI_DEVICE_CONTEXT pDeviceContext, void* ReqBuffer, SIZE_T ReqBufferSize)
{
    NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
    PUCHAR pOutput = nullptr;
    ULONG length = 0;

    // Generate TLV
    ndisStatus =
        GenerateWdiGetAdapterCapabilities(&pDeviceContext->AdapterCapabilities, 0, &pDeviceContext->TlvContext, &length, &pOutput);

    if (ndisStatus == NDIS_STATUS_SUCCESS)
    {
        // Set the information in the wifirequest
        WifiCxTestSetOutput(pDeviceContext, ReqBuffer, ReqBufferSize, pOutput, length);
    }

    FreeGenerated(pOutput);

    return ndisStatus;
}

_Use_decl_annotations_
NDIS_STATUS WifiCxTestSetAdapterConfiguration(PWIFI_DEVICE_CONTEXT pDeviceContext, void* ReqBuffer, SIZE_T ReqBufferSize)
{
    NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
    WDI_SET_FIRMWARE_CONFIGURATION_PARAMETERS fwConfig = {};

    ULONG TLVStreamLength = static_cast<ULONG>(ReqBufferSize - sizeof(WDI_MESSAGE_HEADER));
    auto TLVByteStream = static_cast<UCHAR*>(ReqBuffer) + sizeof(WDI_MESSAGE_HEADER);

    DumpMessageTlvByteStream(
        WDI_SET_ADAPTER_CONFIGURATION, TRUE, pDeviceContext->TlvContext.PeerVersion, TLVStreamLength, TLVByteStream, 0, nullptr);
    ndisStatus = ParseWdiSetAdapterConfiguration(TLVStreamLength, TLVByteStream, &pDeviceContext->TlvContext, &fwConfig);

    if (ndisStatus == NDIS_STATUS_SUCCESS)
    {
        if (fwConfig.Optional.MacAddress_IsPresent)
        {
            pDeviceContext->AdapterCapabilities.InterfaceAttributes.InterfaceCapabilities.Address = fwConfig.MacAddress;
        }
    }

    CleanupParsedWdiSetAdapterConfiguration(&fwConfig);

    return ndisStatus;
}


extern WDI_MAC_ADDRESS STAAddress;

WDI_MAC_ADDRESS LocalLinkAddresses[] = {
    {0x11, 0x01, 0x02, 0x03, 0x04, 0x21},
    {0x11, 0x01, 0x02, 0x03, 0x04, 0x22},
};

static NTSTATUS WifiCxTestSendLinkStateIndication(_In_ WDFDEVICE Device, _In_ PWDI_MESSAGE_HEADER pWdiHeader, ULONG numLinks)
{
    // Report link quality
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWIFI_DEVICE_CONTEXT pDeviceContext = WifiGetDeviceContext(Device);
    WDI_INDICATION_LINK_STATE_CHANGE_PARAMETERS linkStateChangeParameters = {};
    WDI_LINK_INFO_CONTAINER pLinkInfo[2] = {};
    UINT8* pOutput = nullptr;
    ULONG cbOutput = 0;

    RtlCopyMemory(
        &linkStateChangeParameters.LinkStateChangeParameters.PeerMACAddress, &pDeviceContext->ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
    linkStateChangeParameters.LinkStateChangeParameters.TxLinkSpeed = 30000;
    linkStateChangeParameters.LinkStateChangeParameters.RxLinkSpeed = 30000;
    linkStateChangeParameters.LinkStateChangeParameters.LinkQuality = 56;

    pLinkInfo[0].LinkID = 1;
    RtlCopyMemory(&pLinkInfo[0].LocalLinkMACAddress, &LocalLinkAddresses[0], sizeof(DOT11_MAC_ADDRESS));
    RtlCopyMemory(&pLinkInfo[0].PeerLinkMACAddress, &pDeviceContext->ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
    pLinkInfo[0].ChannelNumber = 6;
    pLinkInfo[0].BandId = WDI_BAND_ID_2400;
    pLinkInfo[0].RSSI = -50;
    pLinkInfo[0].Bandwidth = 40;
    pLinkInfo[0].TxMCS = 3;
    pLinkInfo[0].RxMCS = 4;

    if (numLinks > 1)
    {
        pLinkInfo[1].LinkID = 2;
        RtlCopyMemory(&pLinkInfo[1].LocalLinkMACAddress, &LocalLinkAddresses[1], sizeof(DOT11_MAC_ADDRESS));
        RtlCopyMemory(&pLinkInfo[1].PeerLinkMACAddress, &pDeviceContext->ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
        pLinkInfo[1].ChannelNumber = 36;
        pLinkInfo[1].BandId = WDI_BAND_ID_5000;
        pLinkInfo[1].RSSI = -30;
        pLinkInfo[1].Bandwidth = 160;
        pLinkInfo[1].TxMCS = 8;
        pLinkInfo[1].RxMCS = 9;
    }

    linkStateChangeParameters.LinkInfo.pElements = pLinkInfo;
    linkStateChangeParameters.LinkInfo.ElementCount = numLinks > 1 ? 2 : 1;

    ntStatus =
        GenerateWdiIndicationLinkStateChangeFromIhv(&linkStateChangeParameters, 0, &pDeviceContext->TlvContext, &cbOutput, &pOutput);
    if (STATUS_SUCCESS == ntStatus)
    {
        WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_LINK_STATE_CHANGE, 0, pOutput, cbOutput);
        FreeGenerated(pOutput);
    }
    else
    {
        WFCError("Failed to generate WDI_INDICATION_LINK_STATE_CHANGE - 0x%08x\n", ntStatus);
    }

    return ntStatus;
}

#define TEST_RSN_EAPOL_SHA384_KCK_LENGTH (192 / 8) // 24 bytes
#define TEST_RSN_EAPOL_SHA384_KEK_LENGTH (256 / 8) // 32 bytes
#define TEST_RSN_CCMP_KEY_LENGTH 16
#define TEST_RSN_BIP_GMAC_256_KEY_LENGTH 32

#define TEST_NUM_LINKS 3
#define TEST_NUM_CIPHER_KEY_TYPES 3


static NTSTATUS WifiCxTestPerformAssociation(
    _In_ WDFDEVICE Device,
    _In_ struct ArrayOfElements<WDI_CONNECT_BSS_ENTRY_CONTAINER>* pPreferredBSSEntryList,
    _In_ struct ArrayOfElements<WDI_AUTH_ALGORITHM>* pAuthenticationAlgorithms,
    _In_ PWDI_MESSAGE_HEADER pWdiHeader)
{

    ULONG bssIndex = 0;
    ULONG connectEntry = ConnectEntryId_MAX;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 NewConnectEntryId = 0; // Disconnected State
    WDI_AUTH_ALGORITHM NewAuthAlgo = pAuthenticationAlgorithms->pElements[0];
    UCHAR pucData[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10};
    PWIFI_DEVICE_CONTEXT pDeviceContext = WifiGetDeviceContext(Device);
    ULONG assocStatus = WDI_ASSOC_STATUS_SUCCESS;

    do
    {
        // We search for the BSSID pattern to figure out what AP we are trying to connect to
        for (bssIndex = 0; bssIndex < pPreferredBSSEntryList->ElementCount; bssIndex++)
        {
            for (connectEntry = 1; connectEntry < ConnectEntryId_MAX; connectEntry++)
            {
                if (RtlCompareMemory(
                        pPreferredBSSEntryList->pElements[bssIndex].BSSID.Address,
                        g_ConnectEntries[connectEntry].pMacAddress,
                        sizeof(DOT11_MAC_ADDRESS)) == sizeof(DOT11_MAC_ADDRESS))
                {
                    PUCHAR pAssociationResult = g_ConnectEntries[connectEntry].pTlvAssociationResult;

                    NewConnectEntryId = connectEntry;
                    pDeviceContext->LastConnectTransactionId = pWdiHeader->TransactionId;

                    // TODO NetAdapter: datapath not yet implemented
#ifdef WIFICX_TEST_DATAPATH
                    // add peer on datapath
                    AddDatapathPeer(0x33, *g_ConnectEntries[connectEntry].pMacAddress);

                    // example inject frame
                    InjectDatapathFrame(0x33);

                    // Pretend to recieve M1 on datapath before, the association complete has made it up the control path.
                    RecieveDatapathFrame(0x33, sizeof(pucData), pucData);

#endif
                    if ((&s_Connect_Addr_14_WPA3_SAE_CCMP == g_ConnectEntries[connectEntry].pMacAddress) ||
                        (&s_Connect_Addr_30_WiFi7_Mixed_Link_1 == g_ConnectEntries[connectEntry].pMacAddress) ||
                        (&s_Connect_Addr_38_WiFi7_Only_Link_1 == g_ConnectEntries[connectEntry].pMacAddress))
                    {
                        WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED_PARAMETERS SAEAuthParamsNeeded;
                        UINT8* pOutput = nullptr;
                        ULONG cbOutput = 0;
                        NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;

                        //
                        // Handle special case for WPA3-SAE
                        // Send the Indication to request additonal SAE params
                        //
                        g_dwSaeResendConfirmRequested = 0;
                        SAEAuthParamsNeeded.SAEIndicationType = WDI_SAE_INDICATION_TYPE_COMMIT_REQUEST_PARAMS_NEEDED;
                        RtlCopyMemory(
                            SAEAuthParamsNeeded.BssId.Address, g_ConnectEntries[connectEntry].pMacAddress, sizeof(DOT11_MAC_ADDRESS));

                        ndisStatus = GenerateWdiIndicationSaeAuthParamsNeeded(
                            &SAEAuthParamsNeeded, 0, &pDeviceContext->TlvContext, &cbOutput, &pOutput);
                        if (ndisStatus == NDIS_STATUS_SUCCESS)
                        {
                            WFCInfo("[SAE] Indicating request for COMMIT_REQUEST_PARAMS_NEEDED ...");

                            WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED, 0, pOutput, cbOutput);

                            FreeGenerated(pOutput);

                            break;
                        }
                    }

                    // Get the assoc status
                    RtlCopyMemory(&assocStatus, &pAssociationResult[18], sizeof(ULONG));

                    // Send the association indication
                    WifiCxTestSendIndication(
                        Device,
                        pWdiHeader,
                        WDI_INDICATION_ASSOCIATION_RESULT,
                        0,
                        g_ConnectEntries[connectEntry].pTlvAssociationResult,
                        g_ConnectEntries[connectEntry].TlvAssociationResultSize);

                    break;
                }
            }

            //
            // If we found a matching BSSID entry, we are done
            //
            if (connectEntry < ConnectEntryId_MAX)
            {
                break;
            }
        }

    } while (FALSE);

    if (WDI_ASSOC_STATUS_SUCCESS != assocStatus || 0 == NewConnectEntryId || bssIndex >= pPreferredBSSEntryList->ElementCount)
    {
        NewConnectEntryId = 0; // Disconnected State
        NewAuthAlgo = WDI_AUTH_ALGO_80211_OPEN;
        ntStatus = STATUS_UNSUCCESSFUL;
    }
    else
    {
        RtlCopyMemory(&pDeviceContext->ConnectedPeer, pPreferredBSSEntryList->pElements[bssIndex].BSSID.Address, sizeof(DOT11_MAC_ADDRESS));
    }

    pDeviceContext->LastConnectEntryId = NewConnectEntryId;
    pDeviceContext->LastAuthAlgo = NewAuthAlgo;

    if (STATUS_SUCCESS == ntStatus && (WDI_AUTH_ALGO_WPA3_SAE != pDeviceContext->LastAuthAlgo))
    {
        // Report link quality
        ntStatus = WifiCxTestSendLinkStateIndication(Device, pWdiHeader, 2);
    }

    return ntStatus;
}

static void WifiCxTestPerformDisassociation(_In_ WDFDEVICE Device, _In_ PWDI_MESSAGE_HEADER pWdiHeader, _In_ WDI_ASSOC_STATUS DisassocStatus)
{
    // clang-format off
    UCHAR s_TLV_Disassociation[] =
    {
        // WDI_TLV_ASSOCIATION_RESULT
        0xBC, 0x00,
        0x0A, 0x00,

            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,

        // WDI_TLV Need Peer Cleanup Params
        0xb4, 0x00,
        0x01, 0x00,

        0x00
    };
    // clang-format on

    PWIFI_DEVICE_CONTEXT pDeviceContext = WifiGetDeviceContext(Device);

    // TODO NetAdapter: datapath not yet implemented
#ifdef WIFICX_TEST_DATAPATH
    // example inject frame
    InjectDatapathFrame(0x33);
#endif

    RtlCopyMemory(&s_TLV_Disassociation[4], &pDeviceContext->ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
    RtlCopyMemory(&s_TLV_Disassociation[10], &DisassocStatus, sizeof(ULONG));

    // Send the disassociation indication
    WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_DISASSOCIATION, 0, s_TLV_Disassociation, sizeof(s_TLV_Disassociation));

    pDeviceContext->LastConnectEntryId = 0; // Disconnected State
}

_Use_decl_annotations_
static NTSTATUS ProcessWifiRequest(WDFDEVICE Device, UINT16 MessageId, void* Buffer, UINT InBufferLen, UINT OutBufferLen, UINT* pBytesWritten)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
    PWIFI_DEVICE_CONTEXT deviceContext = WifiGetDeviceContext(Device);
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
        auto TLVByteStream = buffer + sizeof(WDI_MESSAGE_HEADER);

        DumpMessageTlvByteStream(
            WDI_TASK_SET_RADIO_STATE, TRUE, deviceContext->TlvContext.PeerVersion, TLVStreamLength, TLVByteStream, 0, nullptr);
        ParseWdiTaskSetRadioState(TLVStreamLength, TLVByteStream, &deviceContext->TlvContext, &RadioStateParams);

        if (RadioStateParams.SoftwareRadioState != deviceContext->CurrentRadioState)
        {
            // Change the radio state
            deviceContext->CurrentRadioState = RadioStateParams.SoftwareRadioState;

            // Send the radio state indication
            WDI_INDICATION_RADIO_STATUS_PARAMETERS RadioStatusParams = {};
            UINT8* pOutput = nullptr;
            ULONG cbOutput = 0;

            RadioStatusParams.RadioState.HardwareState = TRUE;
            RadioStatusParams.RadioState.SoftwareState = deviceContext->CurrentRadioState;
            GenerateWdiIndicationRadioStatus(&RadioStatusParams, 0, &deviceContext->TlvContext, &cbOutput, &pOutput);
            WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_RADIO_STATUS, 0, pOutput, cbOutput);

            FreeGenerated(pOutput);
        }

        WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_SET_RADIO_STATE_COMPLETE, pWdiHeader->TransactionId, nullptr, 0);

        CleanupParsedWdiTaskSetRadioState(&RadioStateParams);
    }
    break;

    case WDI_SET_ADAPTER_CONFIGURATION:
    {
        ndisStatus = WifiCxTestSetAdapterConfiguration(deviceContext, Buffer, InBufferLen);
    }
    break;

    case WDI_TASK_SCAN:
    {
        ULONG scanDataLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto pScanData = buffer + sizeof(WDI_MESSAGE_HEADER);

        WDI_SCAN_PARAMETERS scanParams = {};

        ndisStatus = ParseWdiTaskScan(scanDataLength, pScanData, &deviceContext->TlvContext, &scanParams);

        if (NDIS_STATUS_SUCCESS != ndisStatus)
        {
            WFCTrace("Scan parameters parsing failed: %d\n", ndisStatus);
            // ntStatus = NdisConvertNdisStatusToNtStatus(parseStatus);
            break;
        }

        for (UINT8 connectEntry = 1; connectEntry < ConnectEntryId_MAX; connectEntry++)
        {
            if (g_SupportedBands & g_ConnectEntries[connectEntry].BandId)
            {

                PUCHAR pBssEntry = g_ConnectEntries[connectEntry].pTlvBssEntry;

                //
                // If currently connected, see if the connected entry should be skipped
                //
                if ((deviceContext->LastConnectEntryId != 0) &&
                    (RtlCompareMemory(&deviceContext->ConnectedPeer, &pBssEntry[8], sizeof(DOT11_MAC_ADDRESS)) == sizeof(DOT11_MAC_ADDRESS)))
                {
                    // Already connected, don't report this entry
                    continue;
                }

                // TODO: Skip the IHV band as it is not being reported in capabilities
                if (g_ConnectEntries[connectEntry].BandId == TESTMP_BAND_IHV)
                {
                    continue;
                }

                // Send the BSS entry indication
                WifiCxTestSendIndication(
                    Device,
                    pWdiHeader,
                    WDI_INDICATION_BSS_ENTRY_LIST,
                    0,
                    g_ConnectEntries[connectEntry].pTlvBssEntry,
                    g_ConnectEntries[connectEntry].TlvBssEntrySize);
            }
        }

        //
        // See if the hidden network needs to be indicated
        //
        if ((scanParams.SSIDList.ElementCount > 0) && (scanParams.SSIDList.pElements[0].ElementCount == 8) &&
            (scanParams.SSIDList.pElements[0].pElements[4] == 'H') && (scanParams.SSIDList.pElements[0].pElements[5] == 'I') &&
            (scanParams.SSIDList.pElements[0].pElements[6] == 'D') && (scanParams.SSIDList.pElements[0].pElements[7] == 'E'))
        {
            WifiCxTestSendIndication(
                Device, pWdiHeader, WDI_INDICATION_BSS_ENTRY_LIST, 0, s_TLV_BSS_Entry_ProbeResponse_8_Hidden, sizeof(s_TLV_BSS_Entry_ProbeResponse_8_Hidden));
        }

        CleanupParsedWdiTaskScan(&scanParams);

        WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_SCAN_COMPLETE, pWdiHeader->TransactionId, nullptr, 0);
    }
    break;

    case WDI_TASK_DOT11_RESET:
    {
        WDI_TASK_DOT11_RESET_PARAMETERS resetParameters;

        ULONG resetDataLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto pResetData = buffer + sizeof(WDI_MESSAGE_HEADER);

        ndisStatus = ParseWdiTaskDot11Reset(resetDataLength, pResetData, &deviceContext->TlvContext, &resetParameters);

        if (NDIS_STATUS_SUCCESS != ndisStatus)
        {
            WFCTrace("DOT11 Reset parameters parsing failed: %d\n", ndisStatus);
        }
        else if (0 == resetParameters.Optional.ResetMACAddress_IsPresent)
        {
            WFCTrace("DOT11 Reset, set default MIB = %d, no MAC Address specified\n", resetParameters.Dot11ResetParameters.SetDefaultMIB);
        }
        else
        {
            WFCTrace(
                "DOT11 Reset, set default MIB = %d, MAC Address = %2x:%2x:%2x:%2x:%2x:%2x\n",
                resetParameters.Dot11ResetParameters.SetDefaultMIB,
                resetParameters.ResetMACAddress.Address[0],
                resetParameters.ResetMACAddress.Address[1],
                resetParameters.ResetMACAddress.Address[2],
                resetParameters.ResetMACAddress.Address[3],
                resetParameters.ResetMACAddress.Address[4],
                resetParameters.ResetMACAddress.Address[5]);
        }

        WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_DOT11_RESET_COMPLETE, pWdiHeader->TransactionId, nullptr, 0);

        CleanupParsedWdiTaskDot11Reset(&resetParameters);
    }
    break;

    case WDI_TASK_CONNECT:
    {
        ULONG connectDataLength = static_cast<ULONG>(InBufferLen - sizeof(WDI_MESSAGE_HEADER));
        auto pConnectData = buffer + sizeof(WDI_MESSAGE_HEADER);

        DumpMessageTlvByteStream(WDI_TASK_CONNECT, TRUE, deviceContext->TlvContext.PeerVersion, connectDataLength, pConnectData, 0, nullptr);

        WDI_TASK_CONNECT_PARAMETERS connectParams;

        if (deviceContext->LastConnectEntryId != 0) // Not Disconnected State
        {
            ASSERT(FALSE);
            // TODO NetAdapter: datapath not yet implemented
#ifdef WIFICX_TEST_DATAPATH
            DeleteDatapathPeer(0x33);
#endif
        }

        // We search for the BSSID pattern to figure out what AP we are trying to connect to
        ndisStatus = ParseWdiTaskConnect(connectDataLength, pConnectData, &deviceContext->TlvContext, &connectParams);
        if (NDIS_STATUS_SUCCESS != ndisStatus)
        {
            //   ntStatus = NdisConvertNdisStatusToNtStatus(parseStatus);
            WFCTrace("ParseWdiTaskConnect FAIled %d\n", ndisStatus);
            break;
        }

        // Call the helper
        ndisStatus = WifiCxTestPerformAssociation(
            Device, &connectParams.PreferredBSSEntryList, &connectParams.ConnectParameters.AuthenticationAlgorithms, pWdiHeader);
        CleanupParsedWdiTaskConnect(&connectParams);

        //
        // WPA3-SAE requires the SAE Exchange, so do not complete the Connection request until the SAE exchange is complete
        //
        if (WDI_AUTH_ALGO_WPA3_SAE != deviceContext->LastAuthAlgo)
        {
            WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_CONNECT_COMPLETE, pWdiHeader->TransactionId, nullptr, 0);
        }
    }
    break;

    case WDI_TASK_DISCONNECT:
    {
        WifiCxTestPerformDisassociation(Device, pWdiHeader, WDI_ASSOC_STATUS_DISASSOCIATED_BY_HOST);

        WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_DISCONNECT_COMPLETE, pWdiHeader->TransactionId, nullptr, 0);

#ifdef WIFICX_TEST_DATAPATH
        // TODO NetAdapter: datapath not yet implemented
        DeleteDatapathPeer(0x33);
#endif
    }
    break;

    case WDI_SET_PRIVACY_EXEMPTION_LIST:
    case WDI_SET_DEFAULT_KEY_ID:
    case WDI_SET_ADD_CIPHER_KEYS:
    case WDI_SET_DELETE_CIPHER_KEYS:
    case WDI_SET_RECEIVE_PACKET_FILTER:
    case WDI_SET_CONNECTION_QUALITY:
        // Doing Nothing special
        break;

    case WDI_SET_SAE_AUTH_PARAMS:
    {
        WDI_SET_SAE_AUTH_PARAMS_COMMAND setSAEAuthParams;
        WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED_PARAMETERS SAEAuthParamsNeeded;
        UINT8* pOutput = nullptr;
        ULONG cbOutput = 0;

        // Commit frame when Status = 0:    FiniteCyclicGroup + Scalar + Element
        // Commit frame when Status = 76:   FiniteCyclicGroup + AntiCloggingToken
        WFCInfo("WDI_SET_SAE_AUTH_PARAMS called!!!\n");

        ndisStatus = ParseWdiSetSaeAuthParams(
            InBufferLen - sizeof(WDI_MESSAGE_HEADER), (PUINT8)buffer + sizeof(WDI_MESSAGE_HEADER), &deviceContext->TlvContext, &setSAEAuthParams);
        if (ndisStatus == NDIS_STATUS_SUCCESS)
        {
            if (WDI_SAE_REQUEST_TYPE_COMMIT_PARAMS == setSAEAuthParams.SAERequestType)
            {
                WFCInfo("[SAE] WDI_SET_SAE_AUTH_PARAMS has Commit request for Tx, Setting CommitResponse for Rx\n");

                SAEAuthParamsNeeded.SAEIndicationType = WDI_SAE_INDICATION_TYPE_COMMIT_FRAME;
                SAEAuthParamsNeeded.Optional.SAECommitFrame_IsPresent = 1;

                //
                // Send reflection attack first
                //
                SAEAuthParamsNeeded.SAECommitFrame.ElementCount = sizeof(pucSAECommitResponseReflection);
                SAEAuthParamsNeeded.SAECommitFrame.pElements = pucSAECommitResponseReflection;
            }
            else if (
                (WDI_SAE_REQUEST_TYPE_FAILURE == setSAEAuthParams.SAERequestType) &&
                (WDI_SAE_STATUS_COMMIT_MESSAGE_REFLECTION_ATTACK_DETECTED == setSAEAuthParams.SAEStatus))
            {
                WFCInfo("[SAE] WDI_SET_SAE_AUTH_PARAMS returned Reflection error -- Inidicate proper CommitResponse for Rx\n");

                SAEAuthParamsNeeded.SAEIndicationType = WDI_SAE_INDICATION_TYPE_CONFIRM_FRAME;
                SAEAuthParamsNeeded.Optional.SAECommitFrame_IsPresent = 1;
                SAEAuthParamsNeeded.SAECommitFrame.ElementCount = sizeof(pucSAECommitResponse);
                SAEAuthParamsNeeded.SAECommitFrame.pElements = pucSAECommitResponse;
            }
            else if (WDI_SAE_REQUEST_TYPE_CONFIRM_PARAMS == setSAEAuthParams.SAERequestType)
            {
                WFCInfo("[SAE] WDI_SET_SAE_AUTH_PARAMS has Confirm request for Tx, Setting ConfirmResponse for Rx\n");

                if (g_dwSaeResendConfirmRequested)
                {
                    SAEAuthParamsNeeded.SAEIndicationType = WDI_SAE_INDICATION_TYPE_CONFIRM_FRAME;
                    SAEAuthParamsNeeded.Optional.SAECommitFrame_IsPresent = 1;
                    SAEAuthParamsNeeded.SAECommitFrame.ElementCount = sizeof(pucSAEConfirmResponse);
                    SAEAuthParamsNeeded.SAECommitFrame.pElements = pucSAEConfirmResponse;
                }
                else
                {
                    g_dwSaeResendConfirmRequested = 1;

                    SAEAuthParamsNeeded.SAEIndicationType = WDI_SAE_INDICATION_TYPE_CONFIRM_REQUEST_RESEND_REQUEST;
                }
            }
            else
            {
                if (WDI_SAE_REQUEST_TYPE_FAILURE == setSAEAuthParams.SAERequestType)
                {
                    WFCInfo("[SAE] OID_WDI_SET_SAE_AUTH_PARAMS has indicated error - %d\n", setSAEAuthParams.SAEStatus);
                }
                else if (WDI_SAE_REQUEST_TYPE_SUCCESS == setSAEAuthParams.SAERequestType)
                {
                    WFCInfo("[SAE] OID_WDI_SET_SAE_AUTH_PARAMS has indicated SAE success - %d\n", setSAEAuthParams.SAEStatus);
                }
                else
                {
                    WFCInfo("[SAE] OID_WDI_SET_SAE_AUTH_PARAMS has set INVALID Request type = %d\n", setSAEAuthParams.SAERequestType);
                }
#ifdef WIFICX_TEST_DATAPATH
                // example inject frame
                InjectDatapathFrame(0x33);
#endif
                // Send the association indication
                if (WDI_AUTH_ALGO_WPA3_SAE == deviceContext->LastAuthAlgo)
                {
                    s_TLV_Success_AssociationResult_14_WPA3_SAE_CCMP[27] = (UCHAR)WDI_AUTH_ALGO_WPA3_SAE;
                }
                else
                {
                    s_TLV_Success_AssociationResult_14_WPA3_SAE_CCMP[27] = (UCHAR)WDI_AUTH_ALGO_RSNA_PSK;
                }

                WifiCxTestSendIndication(
                    Device,
                    pWdiHeader,
                    WDI_INDICATION_ASSOCIATION_RESULT,
                    0,
                    s_TLV_Success_AssociationResult_14_WPA3_SAE_CCMP,
                    sizeof(s_TLV_Success_AssociationResult_14_WPA3_SAE_CCMP));
                RtlCopyMemory(&deviceContext->ConnectedPeer, &s_Connect_Addr_14_WPA3_SAE_CCMP, sizeof(DOT11_MAC_ADDRESS));

                // Report link quality
                ntStatus = WifiCxTestSendLinkStateIndication(Device, pWdiHeader, 1);

                WifiCxTestSendIndication(
                    Device, pWdiHeader, WDI_INDICATION_CONNECT_COMPLETE, deviceContext->LastConnectTransactionId, nullptr, 0);

#ifdef WIFICX_TEST_DATAPATH
                //
                // Receive M1 frame of 4-way handshake
                //
                RecieveDatapathFrame(0x33, sizeof(pucM1SaeFrame), pucM1SaeFrame);

                //
                // Receive M3 frame of 4-way handshake
                //
                RecieveDatapathFrame(0x33, sizeof(pucM3SaeFrame), pucM3SaeFrame);
#endif
                break;
            }

            RtlCopyMemory(SAEAuthParamsNeeded.BssId.Address, &s_Connect_Addr_14_WPA3_SAE_CCMP, sizeof(DOT11_MAC_ADDRESS));

            ndisStatus =
                GenerateWdiIndicationSaeAuthParamsNeeded(&SAEAuthParamsNeeded, 0, &deviceContext->TlvContext, &cbOutput, &pOutput);
            if (ndisStatus == NDIS_STATUS_SUCCESS)
            {
                WifiCxTestSendIndication(Device, pWdiHeader, WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED, 0, pOutput, cbOutput);

                FreeGenerated(pOutput);
            }
            else
            {
                WFCError("Failed to generate WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED - 0x%08x\n", ndisStatus);
            }
        }
        else
        {
            WFCError("[SAE] Failed to parse WDI_SET_SAE_AUTH_PARAMS data - 0x%08x\n", ndisStatus);
        }
        CleanupParsedWdiSetSaeAuthParamsToIhv(&setSAEAuthParams);
    }
    break;

    case WDI_SET_LOCATION_PRIVACY:
    {
        WDI_SET_LOCATION_PRIVACY_PARAMETERS privacyParams;

        ndisStatus = ParseWdiSetLocationPrivacyToIhv(
            InBufferLen - sizeof(WDI_MESSAGE_HEADER), (PUINT8)buffer + sizeof(WDI_MESSAGE_HEADER), &deviceContext->TlvContext, &privacyParams);
        if (NDIS_STATUS_SUCCESS == ndisStatus)
        {
            WFCInfo("[WDI_SET_LOCATION_PRIVACY]: Got privacy setting = %x *****\n", privacyParams.LocationAllowed);
            CleanupParsedWdiSetLocationPrivacy(&privacyParams);
        }
        else
        {
            WFCError("[WDI_SET_LOCATION_PRIVACY]: ParseWdiSetLocationPrivacyToIhv FAIled %x0x *****\n", ndisStatus);
        }
    }
    break;

    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        break;
    }

    // Set the ndis Status in the WDI header
    pWdiHeader->Status = ndisStatus;

    // Set the output length
    if (pBytesWritten)
    {
        *pBytesWritten = BytesWritten;
    }
    return ntStatus;
}

_Use_decl_annotations_
NTSTATUS WifiCxTestSetDeviceCapabilities(WDFDEVICE Device)
{
    WIFI_DEVICE_CAPABILITIES deviceCaps = {};
    WIFI_DEVICE_CAPABILITIES_INIT(&deviceCaps);

    deviceCaps.HardwareRadioState = TRUE;
    deviceCaps.SoftwareRadioState = TRUE;
    deviceCaps.FirmwareVersion[MAX_FIRMWARE_VERSION_LENGTH];
    deviceCaps.ActionFramesSupported = TRUE;
    deviceCaps.NumRxStreams = 1;
    deviceCaps.NumTxStreams = 1;
    deviceCaps.Support_eCSA = FALSE;
    deviceCaps.MACAddressRandomization = FALSE;
    deviceCaps.MACAddressRandomizationMask.Address[0] = 0;
    deviceCaps.MACAddressRandomizationMask.Address[1] = 0;
    deviceCaps.MACAddressRandomizationMask.Address[2] = 0;
    deviceCaps.MACAddressRandomizationMask.Address[3] = 0xFF;
    deviceCaps.MACAddressRandomizationMask.Address[4] = 0;
    deviceCaps.MACAddressRandomizationMask.Address[5] = 0;
    deviceCaps.BluetoothCoexistenceSupport = WDI_BLUETOOTH_COEXISTENCE_PERFORMANCE_MAINTAINED;
    deviceCaps.SupportsNonWdiOidRequests = FALSE;
    deviceCaps.FastTransitionSupported = TRUE;
    deviceCaps.MU_MIMOSupported = FALSE;
    deviceCaps.SAEAuthenticationSupported = TRUE;
    deviceCaps.BSSTransitionSupported = TRUE;
    deviceCaps.MBOSupported = FALSE;
    deviceCaps.BeaconReportsImplemented = FALSE;

    WifiDeviceSetDeviceCapabilities(Device, &deviceCaps);

    WIFI_STATION_CAPABILITIES StationCaps = {};
    WIFI_STATION_CAPABILITIES_INIT(&StationCaps);

    StationCaps.ScanSSIDListSize = 4;
    StationCaps.DesiredSSIDListSize = 1;
    StationCaps.PrivacyExemptionListSize = 1;
    StationCaps.KeyMappingTableSize = 32;
    StationCaps.DefaultKeyTableSize = 4;
    StationCaps.WEPKeyValueMaxLength = 0x20;
    StationCaps.MaxNumPerSTA = 4;
    StationCaps.SupportedQOSFlags = 0;
    StationCaps.HostFIPSModeImplemented = FALSE;
    StationCaps.MFPCapable = TRUE;
    StationCaps.AutoPowerSaveMode = FALSE;
    StationCaps.BSSListCachemanagement = FALSE;
    StationCaps.ConnectBSSSelectionOverride = FALSE;
    StationCaps.MaxNetworkOffloadListSize = 0;
    StationCaps.HESSIDConnectionSupported = FALSE;
    StationCaps.FTMAsInitiatorSupport = FALSE;
    StationCaps.FTMNumberOfSupportedTargets = 0;

    const DOT11_AUTH_CIPHER_PAIR UnicastAlgos[] = {
        {DOT11_AUTH_ALGO_80211_OPEN, DOT11_CIPHER_ALGO_NONE},
        {DOT11_AUTH_ALGO_80211_OPEN, DOT11_CIPHER_ALGO_WEP},
        {DOT11_AUTH_ALGO_WPA_PSK, DOT11_CIPHER_ALGO_CCMP},
        {DOT11_AUTH_ALGO_RSNA, DOT11_CIPHER_ALGO_CCMP},
        {DOT11_AUTH_ALGO_RSNA_PSK, DOT11_CIPHER_ALGO_TKIP},
        {DOT11_AUTH_ALGO_RSNA_PSK, DOT11_CIPHER_ALGO_CCMP},
        {DOT11_AUTH_ALGO_WPA3_ENT_192, DOT11_CIPHER_ALGO_GCMP_256},
        {DOT11_AUTH_ALGO_WPA3_ENT, DOT11_CIPHER_ALGO_CCMP},
        {DOT11_AUTH_ALGO_WPA3_SAE, DOT11_CIPHER_ALGO_GCMP_256},
        {DOT11_AUTH_ALGO_WPA3_SAE, DOT11_CIPHER_ALGO_CCMP},
        {DOT11_AUTH_ALGO_OWE, DOT11_CIPHER_ALGO_GCMP_256},
        {DOT11_AUTH_ALGO_OWE, DOT11_CIPHER_ALGO_CCMP},
    };

    const DOT11_AUTH_CIPHER_PAIR McastMgmtAlgos[] = {
        {DOT11_AUTH_ALGO_80211_OPEN, DOT11_CIPHER_ALGO_NONE},
        {DOT11_AUTH_ALGO_RSNA, DOT11_CIPHER_ALGO_BIP},
        {DOT11_AUTH_ALGO_RSNA_PSK, DOT11_CIPHER_ALGO_BIP},
        {DOT11_AUTH_ALGO_WPA3_SAE, DOT11_CIPHER_ALGO_BIP},
        {DOT11_AUTH_ALGO_WPA3_ENT, DOT11_CIPHER_ALGO_BIP},
        {DOT11_AUTH_ALGO_WPA3_ENT_192, DOT11_CIPHER_ALGO_BIP_GMAC_256},
    };

    StationCaps.NumSupportedUnicastAlgorithms = ARRAYSIZE(UnicastAlgos);
    StationCaps.UnicastAlgorithmsList = const_cast<PDOT11_AUTH_CIPHER_PAIR>(UnicastAlgos);
    StationCaps.NumSupportedMulticastDataAlgorithms = ARRAYSIZE(UnicastAlgos);
    StationCaps.MulticastDataAlgorithmsList = const_cast<PDOT11_AUTH_CIPHER_PAIR>(UnicastAlgos);
    StationCaps.NumSupportedMulticastMgmtAlgorithms = ARRAYSIZE(McastMgmtAlgos);
    StationCaps.MulticastMgmtAlgorithmsList = const_cast<PDOT11_AUTH_CIPHER_PAIR>(McastMgmtAlgos);

    WIFI_STA_BANDS_COMBINATION SecondaryStaBandsCombinations[] = {
        {2, {WDI_BAND_ID_2400, WDI_BAND_ID_5000, WDI_BAND_ID_UNKNOWN, WDI_BAND_ID_UNKNOWN}},
        {2, {WDI_BAND_ID_2400, WDI_BAND_ID_6000, WDI_BAND_ID_UNKNOWN, WDI_BAND_ID_UNKNOWN}},
        {2, {WDI_BAND_ID_5000, WDI_BAND_ID_6000, WDI_BAND_ID_UNKNOWN, WDI_BAND_ID_UNKNOWN}},
        {3, {WDI_BAND_ID_2400, WDI_BAND_ID_5000, WDI_BAND_ID_6000, WDI_BAND_ID_UNKNOWN}},
    };

    StationCaps.NumSecondaryStaBandCombinations = ARRAYSIZE(SecondaryStaBandsCombinations);
    StationCaps.SecondaryStaBandsCombinations = SecondaryStaBandsCombinations;

    WDI_MAC_ADDRESS MLOAddresses[] = {
        {0x11, 0x01, 0x02, 0x03, 0x04, 0x21},
        {0x11, 0x01, 0x02, 0x03, 0x04, 0x22},
    };
    StationCaps.MaxMLOLinksSupported = ARRAYSIZE(MLOAddresses);
    StationCaps.MLOAddressesList = MLOAddresses;

    RSNA_AKM_SUITE AkmsList[] = {
        rsna_akm_1x,
        rsna_akm_psk,
        rsna_akm_ft_1x_sha256,
        rsna_akm_ft_psk_sha256,
        rsna_akm_1x_sha256,
        rsna_akm_psk_sha256,
        rsna_akm_sae_pmk256,
        rsna_akm_1x_suite_b_sha384,
        rsna_akm_owe,
        rsna_akm_1x_sha384,
        rsna_akm_sae_pmk384,
    };
    StationCaps.NumAkmsSupported = ARRAYSIZE(AkmsList);
    StationCaps.AkmsList = AkmsList;

    if (WIFI_IS_FIELD_AVAILABLE(WIFI_STATION_CAPABILITIES, MSCSSupported))
    {
        StationCaps.MSCSSupported = true;
    }
    if (WIFI_IS_FIELD_AVAILABLE(WIFI_STATION_CAPABILITIES, DSCPToUPMappingSupported))
    {
        StationCaps.DSCPToUPMappingSupported = true;
    }
#ifdef _KERNEL_MODE
    if (!KdRefreshDebuggerNotPresent())
    {
        DbgPrintEx(
            DPFLTR_DEFAULT_ID,
            DPFLTR_ERROR_LEVEL,
            "WIFI_IS_FIELD_AVAILABLE:WIFI_STATION_CAPABILITIES::MSCSSupported %u\n",
            WIFI_IS_FIELD_AVAILABLE(WIFI_STATION_CAPABILITIES, MSCSSupported));
        DbgPrintEx(
            DPFLTR_DEFAULT_ID,
            DPFLTR_ERROR_LEVEL,
            "WIFI_IS_FIELD_AVAILABLE:WIFI_STATION_CAPABILITIES::DSCPToUPMappingSupported %u\n",
            WIFI_IS_FIELD_AVAILABLE(WIFI_STATION_CAPABILITIES, DSCPToUPMappingSupported));
        DbgPrintEx(
            DPFLTR_DEFAULT_ID,
            DPFLTR_ERROR_LEVEL,
            "WIFI_IS_FIELD_AVAILABLE:WIFI_STATION_CAPABILITIES::MaxNumConfigurableActionFrameWakePatterns %u\n",
            WIFI_IS_FIELD_AVAILABLE(WIFI_STATION_CAPABILITIES, MaxNumConfigurableActionFrameWakePatterns));
        DbgPrintEx(
            DPFLTR_DEFAULT_ID,
            DPFLTR_ERROR_LEVEL,
            "WIFI_IS_FUNCTION_AVAILABLE:WifiPowerOffloadGetActionFrameWakePatternParameters %u\n",
            WIFI_IS_FUNCTION_AVAILABLE(WifiPowerOffloadGetActionFrameWakePatternParameters));
    }
#endif
    WifiDeviceSetStationCapabilities(Device, &StationCaps);

    WIFI_BAND_CAPABILITIES BandCaps = {};
    WIFI_BAND_CAPABILITIES_INIT(&BandCaps);

    const WDI_PHY_TYPE Phy24GHz[] = { WDI_PHY_TYPE_ERP, WDI_PHY_TYPE_HE };                   // g, ax
    const WDI_PHY_TYPE Phy5GHz[] = { WDI_PHY_TYPE_OFDM, WDI_PHY_TYPE_HE, WDI_PHY_TYPE_EHT }; // a, ax, be
    const WDI_PHY_TYPE Phy6GHz[] = { WDI_PHY_TYPE_HE, WDI_PHY_TYPE_EHT };                    // ax, be
    const WDI_PHY_TYPE pPhy60GHz[] = { WDI_PHY_TYPE_DMG };                                   // ad
    const WDI_PHY_TYPE phyIHV[2] = { WDI_PHY_TYPE_OFDM, static_cast<WDI_PHY_TYPE>(WDI_PHY_TYPE_IHV_START + 1) };

    // clang-format off

    const WDI_CHANNEL_MAPPING_ENTRY ChannelMap24[] = {
        {1, 2412},
        {2, 2417},
        {3, 2422},
        {4, 2427},
        {5, 2432},
        {6, 2437},
        {7, 2442},
        {8, 2447},
        {9, 2452},
        {10, 2457},
        {11, 2462},
        {12, 2467}, // Not used in US
        {13, 2472}, // Not used in US
        {14, 2484}, // Allowed in Japan only
    };

    const WDI_CHANNEL_MAPPING_ENTRY ChannelMap5[] = {
        {7, 5035},  // Not used in US
        {8, 5040},  // Not used in US
        {9, 5045},  // Not used in US
        {11, 5055}, // Not used in US
        {12, 5060}, // Not used in US
        {16, 5080}, // Not used in US
        {32, 5160}, // Unknown status
        {34, 5170}, // Not used in US
        {36, 5180},
        {38, 5190},
        {40, 5200},
        {42, 5210},
        {44, 5220},
        {46, 5230},
        {48, 5240},
        {50, 5250},  // DFS
        {52, 5260},  // DFS
        {54, 5270},  // DFS
        {56, 5280},  // DFS
        {58, 5290},  // DFS
        {60, 5300},  // DFS
        {62, 5310},  // DFS
        {64, 5320},  // DFS
        {68, 5340},  // Unknown status
        {96, 5480},  // Unknown status
        {100, 5500}, // DFS
        {102, 5510}, // DFS
        {104, 5520}, // DFS
        {106, 5530}, // DFS
        {108, 5540}, // DFS
        {110, 5550}, // DFS
        {112, 5560}, // DFS
        {114, 5570}, // DFS
        {116, 5580}, // DFS
        {118, 5590}, // DFS
        {120, 5600}, // DFS
        {122, 5610}, // DFS
        {124, 5620}, // DFS
        {126, 5630}, // DFS
        {128, 5640}, // DFS
        {132, 5660}, // DFS
        {134, 5670}, // DFS
        {136, 5680}, // DFS
        {138, 5690}, // DFS
        {140, 5700}, // DFS
        {142, 5710}, // DFS
        {142, 5720}, // DFS
        {144, 5730}, // DFS
        {149, 5745},
        {151, 5755},
        {153, 5765},
        {155, 5785},
        {157, 5785},
        {159, 5795},
        {161, 5805},
        {165, 5825},
        {169, 5845}, // Not used in US
        {173, 5865}, // Not used in US
        {183, 4915}, // Not used in US
        {184, 4920}, // Not used in US
        {185, 4925}, // Not used in US
        {187, 4935}, // Not used in US
        {188, 4940}, // Not used in US
        {189, 4945}, // Not used in US
        {192, 4960}, // Not used in US
        {196, 4980}, // Not used in US
    };

    const WDI_CHANNEL_MAPPING_ENTRY channelMap6[] = {
        {1, 5955},
        {5, 5975}, // PSC Channel (1)
        {9, 5995},

        {13, 6015},
        {17, 6035},
        {21, 6055}, // PSC Channel (2)
        {25, 6075},
        {29, 6095},

        {33, 6115},
        {37, 6135}, // PSC Channel (3)
        {41, 6155},
        {45, 6175},
        {49, 6195},

        {53, 6215}, // PSC Channel (4)
        {57, 6235},
        {61, 6255},
        {65, 6275},
        {69, 6295}, // PSC Channel (5)

        {73, 6315},
        {77, 6335},
        {81, 6355},
        {85, 6375}, // PSC Channel (6)
        {89, 6395},

        {93, 6415},
        {97, 6435},
        {101, 6455}, // PSC Channel (7)
        {105, 6475},
        {109, 6495},

        {113, 6515},
        {117, 6535}, // PSC Channel (8)
        {121, 6555},
        {125, 6575},
        {129, 6595},

        {133, 6615}, // PSC Channel (9)
        {137, 6635},
        {141, 6655},
        {145, 6675},
        {149, 6695}, // PSC Channel (10)

        {153, 6715},
        {157, 6735},
        {161, 6755},
        {165, 6775}, // PSC Channel (11)
        {169, 6795},

        {173, 6815},
        {177, 6835},
        {181, 6855}, // PSC Channel (12)
        {185, 6875},
        {189, 6895},

        {193, 6915},
        {197, 6935}, // PSC Channel (13)
        {201, 6955},
        {205, 6975},
        {209, 6995},

        {213, 7015}, // PSC Channel (14)
        {217, 7035},
        {221, 7055},
        {225, 7075},
        {229, 7095}, // PSC Channel (15)

        {233, 7115},
        {237, 7135},
        {241, 7155},
        {245, 7175},
        {249, 7195},

        {253, 7215},
    };

    const WDI_CHANNEL_MAPPING_ENTRY channelMap60[] = {
        {1, 58320},
        {2, 60480},
        {3, 62640},
        {4, 64800},
        {5, 66960},
        {6, 69120},
    };

    // clang-format on

    UINT32 ChannelWidth10Mhz = 10;
    UINT32 ChannelWidth20Mhz = 20;
    UINT32 channelWidth2160Mhz = 2160;
    UINT32 pChannelWidth6Ghz[] = { 20, 40, 80, 160, 320 };

    WIFI_BAND_INFO BandInfo[4] = {}; // Upto 4 bands
    UINT32 bandInfoCount = 0;

    if (g_SupportedBands & WDI_BAND_ID_2400)
    {
        BandInfo[bandInfoCount].BandID = WDI_BAND_ID_2400;
        BandInfo[bandInfoCount].BandState = TRUE;
        BandInfo[bandInfoCount].NumValidPhyTypes = ARRAYSIZE(Phy24GHz);
        BandInfo[bandInfoCount].ValidPhyTypeList = const_cast<WDI_PHY_TYPE*>(Phy24GHz);
        BandInfo[bandInfoCount].NumValidChannelTypes = ARRAYSIZE(ChannelMap24);
        BandInfo[bandInfoCount].ValidChannelTypes = const_cast<WDI_CHANNEL_MAPPING_ENTRY*>(ChannelMap24);
        BandInfo[bandInfoCount].NumChannelWidths = 1;
        BandInfo[bandInfoCount].ChannelWidthList = &ChannelWidth10Mhz;
        bandInfoCount++;
    }
    NT_ASSERT(bandInfoCount <= 1);

    if (g_SupportedBands & WDI_BAND_ID_5000)
    {
        BandInfo[bandInfoCount].BandID = WDI_BAND_ID_5000;
        BandInfo[bandInfoCount].BandState = TRUE;
        BandInfo[bandInfoCount].NumValidPhyTypes = ARRAYSIZE(Phy5GHz);
        BandInfo[bandInfoCount].ValidPhyTypeList = const_cast<WDI_PHY_TYPE*>(Phy5GHz);
        BandInfo[bandInfoCount].NumValidChannelTypes = ARRAYSIZE(ChannelMap5);
        BandInfo[bandInfoCount].ValidChannelTypes = const_cast<WDI_CHANNEL_MAPPING_ENTRY*>(ChannelMap5);
        BandInfo[bandInfoCount].NumChannelWidths = 1;
        BandInfo[bandInfoCount].ChannelWidthList = &ChannelWidth20Mhz;
        bandInfoCount++;
    }
    NT_ASSERT(bandInfoCount <= 2);

    if (g_SupportedBands & WDI_BAND_ID_6000)
    {
        BandInfo[bandInfoCount].BandID = WDI_BAND_ID_6000; // 6
        BandInfo[bandInfoCount].BandState = TRUE;
        BandInfo[bandInfoCount].NumValidPhyTypes = ARRAYSIZE(Phy6GHz);
        BandInfo[bandInfoCount].ValidPhyTypeList = const_cast<WDI_PHY_TYPE*>(Phy6GHz);
        BandInfo[bandInfoCount].NumValidChannelTypes = ARRAYSIZE(channelMap6);
        BandInfo[bandInfoCount].ValidChannelTypes = const_cast<WDI_CHANNEL_MAPPING_ENTRY*>(channelMap6);
        BandInfo[bandInfoCount].NumChannelWidths = ARRAYSIZE(pChannelWidth6Ghz);
        BandInfo[bandInfoCount].ChannelWidthList = pChannelWidth6Ghz;
        bandInfoCount++;
    }

    NT_ASSERT(bandInfoCount <= 3);
    if (g_SupportedBands & WDI_BAND_ID_60000)
    {
        BandInfo[bandInfoCount].BandID = WDI_BAND_ID_60000; // 60
        BandInfo[bandInfoCount].BandState = TRUE;
        BandInfo[bandInfoCount].NumValidPhyTypes = ARRAYSIZE(pPhy60GHz);
        BandInfo[bandInfoCount].ValidPhyTypeList = const_cast<WDI_PHY_TYPE*>(pPhy60GHz);
        BandInfo[bandInfoCount].NumValidChannelTypes = ARRAYSIZE(channelMap60);
        BandInfo[bandInfoCount].ValidChannelTypes = const_cast<WDI_CHANNEL_MAPPING_ENTRY*>(channelMap60);
        BandInfo[bandInfoCount].NumChannelWidths = 1;
        BandInfo[bandInfoCount].ChannelWidthList = &channelWidth2160Mhz;
        bandInfoCount++;
    }
    NT_ASSERT(bandInfoCount <= 4);

    BandCaps.NumBands = bandInfoCount;
    BandCaps.BandInfoList = BandInfo;

    WifiDeviceSetBandCapabilities(Device, &BandCaps);

    WIFI_PHY_CAPABILITIES PhyCaps = {};
    WIFI_PHY_CAPABILITIES_INIT(&PhyCaps);

    WIFI_PHY_INFO PhyInfoList[3];

    const WDI_DATA_RATE_ENTRY DataRateListErp[] = {
        {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2},
        {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 4},
        {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 22},
        {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 108} };
    const WDI_DATA_RATE_ENTRY DataRateListOfdm[] = {
        {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2}, {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 108} };
    const WDI_DATA_RATE_ENTRY DataRateListEht[] = {
        {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2}, {WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 108} };

    PhyInfoList[0].PhyType = WDI_PHY_TYPE_ERP;
    PhyInfoList[0].NumberDataRateEntries = ARRAYSIZE(DataRateListErp);
    RtlCopyMemory(&PhyInfoList[0].DataRateList, &DataRateListErp, sizeof(DataRateListErp));

    PhyInfoList[1].PhyType = WDI_PHY_TYPE_HE;
    PhyInfoList[1].NumberDataRateEntries = ARRAYSIZE(DataRateListOfdm);
    RtlCopyMemory(&PhyInfoList[1].DataRateList, &DataRateListOfdm, sizeof(DataRateListOfdm));

    PhyInfoList[2].PhyType = WDI_PHY_TYPE_EHT;
    PhyInfoList[2].NumberDataRateEntries = ARRAYSIZE(DataRateListEht);
    RtlCopyMemory(&PhyInfoList[2].DataRateList, &DataRateListEht, sizeof(DataRateListEht));

    PhyCaps.NumPhyTypes = ARRAYSIZE(PhyInfoList);
    PhyCaps.PhyInfoList = PhyInfoList;

    WifiDeviceSetPhyCapabilities(Device, &PhyCaps);

    return STATUS_SUCCESS;
}