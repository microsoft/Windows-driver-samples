// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

// The class for Wifi IHV device's HAL functionalities
class WifiHAL
{
public:
    static NTSTATUS _Create(_In_ WDFDEVICE Device);
    static void _OnCleanup(_In_ WDFOBJECT Object);

    // Default ctor so context memory can be zeroed by WDF without placement new
    WifiHAL() = default;

    // Initialization routine replacing the previous parameterized ctor usage
    void Initialize(_In_ WDFDEVICE Device, _In_ PCTLV_CONTEXT TlvContext);

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

    // Removed const so we can initialize without running a constructor via placement new
    WDI_MAC_ADDRESS m_LocalLinkAddresses[2];
    ULONG m_SupportedBands;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WifiHAL, GetWifiHalFromHandle);
