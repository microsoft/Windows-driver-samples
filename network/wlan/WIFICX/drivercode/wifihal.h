// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

// The class for Wifi IHV device's HAL functionalities
class WifiHAL
{
public:
    WifiHAL(_In_ WDFDEVICE Device);

    // Wifi request M3 working condition verification function
    NTSTATUS WifiIhvIsDeviceReadyForRequest();

    // Wifi request pending transition status check function
    // for example, during SAE authentication, the connect request is pending until SAE exchange is complete
    NTSTATUS WifiIhvGetPendingTransitionStatus();

    NTSTATUS WifiIhvSetDeviceCapabilities();
    NTSTATUS WifiIhvReset(_In_ const WDI_TASK_DOT11_RESET_PARAMETERS& ResetParameters, _In_ const PWDI_MESSAGE_HEADER pWdiHeader, _In_ UINT BytesWritten);
    NTSTATUS WifiIhvSetRadioState(_In_ const WDI_SET_RADIO_STATE_PARAMETERS& RadioState, _In_ const PWDI_MESSAGE_HEADER pWdiHeader, _In_ UINT BytesWritten);
    NTSTATUS WifiIhvScan(_In_ const WDI_SCAN_PARAMETERS& ScanParameters, _In_ const PWDI_MESSAGE_HEADER pWdiHeader, _In_ UINT BytesWritten);
    NTSTATUS WifiIhvConnect(_In_ const WDI_TASK_CONNECT_PARAMETERS& ConnectParameters, _In_ const PWDI_MESSAGE_HEADER pWdiHeader, _In_ UINT BytesWritten);
    NTSTATUS WifiIhvSetSaeAuthParams(_In_ const WDI_SET_SAE_AUTH_PARAMS_COMMAND& setSAEAuthParams, _In_ const PWDI_MESSAGE_HEADER pWdiHeader, _In_ UINT BytesWritten);
    NTSTATUS WifiIhvDisconnect(_In_ const WDI_TASK_DISCONNECT_PARAMETERS& disconnectParameters, _In_ const PWDI_MESSAGE_HEADER pWdiHeader, _In_ UINT BytesWritten);
private:
    NTSTATUS WifiIhvPerformAssociation(_In_ const struct ArrayOfElements<WDI_CONNECT_BSS_ENTRY_CONTAINER>* pPreferredBSSEntryList, _In_ const struct ArrayOfElements<WDI_AUTH_ALGORITHM>* pAuthenticationAlgorithms, _In_ const PWDI_MESSAGE_HEADER pWdiHeader);
    NTSTATUS WifiIhvSendLinkStateIndication(_In_ const PWDI_MESSAGE_HEADER pWdiHeader, ULONG numLinks);

    WDFDEVICE m_Device{};
    PCTLV_CONTEXT m_TlvContext{};
    
    UCHAR m_CurrentRadioState{};
    UINT32 m_LastConnectEntryId{};
    UINT32 m_LastConnectTransactionId{};
    WDI_AUTH_ALGORITHM m_LastAuthAlgo{};
    DOT11_MAC_ADDRESS m_ConnectedPeer{};

    const WDI_MAC_ADDRESS m_LocalLinkAddresses[2] = {
        {0x11, 0x01, 0x02, 0x03, 0x04, 0x21},
        {0x11, 0x01, 0x02, 0x03, 0x04, 0x22},
    };

    const ULONG m_SupportedBands = (WDI_BAND_ID_2400 | WDI_BAND_ID_5000 | WDI_BAND_ID_6000);
};
