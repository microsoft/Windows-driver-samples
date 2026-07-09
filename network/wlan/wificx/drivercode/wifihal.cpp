// Copyright (C) Microsoft Corporation. All rights reserved.

#include "precomp.h"
#include "wifirequest.h"
#include "wifiHALtestdata.h"
#include "WifiHal.h"
#include "WifiHal.tmh"

_Use_decl_annotations_
NTSTATUS WifiHAL::_Create(WDFDEVICE Device)
{
    // Download firmware, initialize hardware, etc.

    // Create WifiHAL object and associate it with Device context after FW ready
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WifiHAL);
    attributes.EvtCleanupCallback = WifiHAL::_OnCleanup;

    void* memory = nullptr;
    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WdfObjectAllocateContext(Device, &attributes, &memory), "Failed to allocate WifiHAL context. Device=%p", Device);

    // Obtain the context and initialize it
    auto* wifiHal = reinterpret_cast<WifiHAL*>(memory);
    wifiHal->Initialize(Device, &WifiGetIhvDeviceContext(Device)->TlvContext);

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        wifiHal->WifiIhvSetDeviceCapabilities(),
        "Failed to set device capabilities. Device=%p", Device);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void WifiHAL::_OnCleanup(WDFOBJECT Object)
{
    UNREFERENCED_PARAMETER(Object);
}

_Use_decl_annotations_
void WifiHAL::Initialize(WDFDEVICE Device, PCTLV_CONTEXT TlvContext)
{
    m_Device = Device;
    m_TlvContext = TlvContext;
    m_CurrentRadioState = 1; // On by default
    m_LastConnectEntryId = 0;
    m_LastConnectTransactionId = 0;
    m_LastAuthAlgo = WDI_AUTH_ALGO_UNKNOWN;
    RtlZeroMemory(&m_ConnectedPeer, sizeof(m_ConnectedPeer));
    // Initialize link addresses and supported bands (previously in-class const init)
    m_LocalLinkAddresses[0] = { 0x11, 0x01, 0x02, 0x03, 0x04, 0x21 };
    m_LocalLinkAddresses[1] = { 0x11, 0x01, 0x02, 0x03, 0x04, 0x22 };
    m_SupportedBands = (WDI_BAND_ID_2400 | WDI_BAND_ID_5000 | WDI_BAND_ID_6000);
}

NTSTATUS WifiHAL::WifiIhvIsDeviceReadyForRequest()
{
    NTSTATUS status = 
         ((m_Device != WDF_NO_HANDLE) // Make sure device is initialized (since this is hardware abstraction layer, IHV can replace with firmware state)
        && (WifiGetIhvDeviceContext(m_Device)->netAdapters[0] != WDF_NO_HANDLE) ? STATUS_SUCCESS : STATUS_DEVICE_NOT_READY);// In WIFICX, the logic sits on top of primary STA adapter, make sure it is initialized
    if(NT_SUCCESS(status) == FALSE)
    {
        WFCError(
            "Device not ready for request. Device=%p, primaryStaAdapter=%p",
            m_Device,
            (m_Device != WDF_NO_HANDLE) ? WifiGetIhvDeviceContext(m_Device)->netAdapters[0] : WDF_NO_HANDLE);
    }
    return status;
}

NTSTATUS WifiHAL::WifiIhvGetPendingTransitionStatus()
{
    return m_LastConnectTransactionId ==0 ? STATUS_SUCCESS : STATUS_PENDING;
}

NTSTATUS WifiHAL::WifiIhvSetDeviceCapabilities()
{
    WIFI_DEVICE_CAPABILITIES deviceCaps = {};
    WIFI_DEVICE_CAPABILITIES_INIT(&deviceCaps);

    deviceCaps.HardwareRadioState = TRUE;
    deviceCaps.SoftwareRadioState = TRUE;
    RtlCopyMemory(deviceCaps.FirmwareVersion, "1.0.0", sizeof("1.0.0"));
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

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiDeviceSetDeviceCapabilities(m_Device, &deviceCaps), 
        "Failed to set device capabilities");

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

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiDeviceSetStationCapabilities(m_Device, &StationCaps), 
        "Failed to set station capabilities");

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

    if (m_SupportedBands & WDI_BAND_ID_2400)
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

    if (m_SupportedBands & WDI_BAND_ID_5000)
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

    if (m_SupportedBands & WDI_BAND_ID_6000)
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
    if (m_SupportedBands & WDI_BAND_ID_60000)
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

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiDeviceSetBandCapabilities(m_Device, &BandCaps), 
        "Failed to set band capabilities");

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

    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(
        WifiDeviceSetPhyCapabilities(m_Device, &PhyCaps), 
        "Failed to set PHY capabilities");

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvReset(const WDI_TASK_DOT11_RESET_PARAMETERS& ResetParameters, const PWDI_MESSAGE_HEADER, UINT)
{
    if (0 == ResetParameters.Optional.ResetMACAddress_IsPresent)
    {
        WFCTrace("DOT11 Reset, set default MIB = %d, no MAC Address specified\n", ResetParameters.Dot11ResetParameters.SetDefaultMIB);
    }
    else
    {
        WFCTrace(
            "DOT11 Reset, set default MIB = %d, MAC Address = %2x:%2x:%2x:%2x:%2x:%2x\n",
            ResetParameters.Dot11ResetParameters.SetDefaultMIB,
            ResetParameters.ResetMACAddress.Address[0],
            ResetParameters.ResetMACAddress.Address[1],
            ResetParameters.ResetMACAddress.Address[2],
            ResetParameters.ResetMACAddress.Address[3],
            ResetParameters.ResetMACAddress.Address[4],
            ResetParameters.ResetMACAddress.Address[5]);
    }

    // Reset the connection ID in case the previous connection attempt did not complete
    m_LastConnectEntryId = 0;
    
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvSetRadioState(const WDI_SET_RADIO_STATE_PARAMETERS& RadioState, const PWDI_MESSAGE_HEADER pWdiHeader, UINT)
{
    WFCInfo("Setting OS requested Radio State: SoftwareRadioState=%u\n", RadioState.SoftwareRadioState);
    if (RadioState.SoftwareRadioState != m_CurrentRadioState)
    {
        // Change the radio state
        m_CurrentRadioState = RadioState.SoftwareRadioState;

        // Send the radio state indication
        WDI_INDICATION_RADIO_STATUS_PARAMETERS RadioStatusParams = {};
        UINT8* pOutput = nullptr;
        ULONG cbOutput = 0;

        RadioStatusParams.RadioState.HardwareState = TRUE;
        RadioStatusParams.RadioState.SoftwareState = m_CurrentRadioState;
        if (GenerateWdiIndicationRadioStatus(&RadioStatusParams, 0, m_TlvContext, &cbOutput, &pOutput) == NDIS_STATUS_SUCCESS)
        {
            WFCInfo("Indicate OS with Radio State: SoftwareRadioState=%u\n", m_CurrentRadioState);
            WifiIhvSendUnsolicitedIndicationToOs(m_Device, pWdiHeader, WDI_INDICATION_RADIO_STATUS, pOutput, cbOutput);
            FreeGenerated(pOutput);
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvScan(const WDI_SCAN_PARAMETERS& ScanParameters, const PWDI_MESSAGE_HEADER pWdiHeader, UINT)
{
    for (UINT8 connectEntry = 1; connectEntry < ConnectEntryId_MAX; connectEntry++)
    {
        if (m_SupportedBands & g_ConnectEntries[connectEntry].BandId)
        {
            PUCHAR pBssEntry = g_ConnectEntries[connectEntry].pTlvBssEntry;
            //
            // If currently connected, see if the connected entry should be skipped
            //
            if ((m_LastConnectEntryId != 0) &&
                (RtlCompareMemory(&m_ConnectedPeer, &pBssEntry[8], sizeof(DOT11_MAC_ADDRESS)) == sizeof(DOT11_MAC_ADDRESS)))
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
            WifiIhvSendUnsolicitedIndicationToOs(
                m_Device,
                pWdiHeader,
                WDI_INDICATION_BSS_ENTRY_LIST,
                g_ConnectEntries[connectEntry].pTlvBssEntry,
                g_ConnectEntries[connectEntry].TlvBssEntrySize);
        }
    }

    //
    // See if the hidden network needs to be indicated
    //
    if ((ScanParameters.SSIDList.ElementCount > 0) && (ScanParameters.SSIDList.pElements[0].ElementCount == 8) &&
        (ScanParameters.SSIDList.pElements[0].pElements[4] == 'H') && (ScanParameters.SSIDList.pElements[0].pElements[5] == 'I') &&
        (ScanParameters.SSIDList.pElements[0].pElements[6] == 'D') && (ScanParameters.SSIDList.pElements[0].pElements[7] == 'E'))
    {
        WifiIhvSendUnsolicitedIndicationToOs(
            m_Device, pWdiHeader, WDI_INDICATION_BSS_ENTRY_LIST, s_TLV_BSS_Entry_ProbeResponse_8_Hidden, sizeof(s_TLV_BSS_Entry_ProbeResponse_8_Hidden));
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvConnect(const WDI_TASK_CONNECT_PARAMETERS& ConnectParameters, const PWDI_MESSAGE_HEADER pWdiHeader, UINT)
{
    NT_ASSERT(m_LastConnectEntryId == 0);
#ifdef NETV_SUPPORT_TX_DEMUXING
    if (m_LastConnectEntryId != 0) // Not Disconnected State
    {
        WifiAdapterRemovePeer(
            WifiGetIhvDeviceContext(m_Device)->netAdapters[pWdiHeader->PortId],
            reinterpret_cast<NET_EUI48_ADDRESS*>(&m_ConnectedPeer));
    }
#endif //NETV_SUPPORT_TX_DEMUXING
    WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(WifiIhvPerformAssociation(
        &ConnectParameters.PreferredBSSEntryList, &ConnectParameters.ConnectParameters.AuthenticationAlgorithms, pWdiHeader),
        "Failed to perform association");

    //
    // WPA3-SAE requires the SAE Exchange, so do not complete the Connection request until the SAE exchange is complete
    //
    if (WDI_AUTH_ALGO_WPA3_SAE == m_LastAuthAlgo)
    {
        m_LastConnectTransactionId = pWdiHeader->TransactionId;
    }
    else 
    {
        m_LastConnectTransactionId = 0;
    }
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvSendLinkStateIndication(_In_ PWDI_MESSAGE_HEADER pWdiHeader, ULONG numLinks)
{
    // Report link quality
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWIFI_IHV_DEVICE_CONTEXT pDeviceContext = WifiGetIhvDeviceContext(m_Device);
    WDI_INDICATION_LINK_STATE_CHANGE_PARAMETERS linkStateChangeParameters = {};
    WDI_LINK_INFO_CONTAINER pLinkInfo[2] = {};
    UINT8* pOutput = nullptr;
    ULONG cbOutput = 0;

    RtlCopyMemory(
        &linkStateChangeParameters.LinkStateChangeParameters.PeerMACAddress, &m_ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
    linkStateChangeParameters.LinkStateChangeParameters.TxLinkSpeed = 30000;
    linkStateChangeParameters.LinkStateChangeParameters.RxLinkSpeed = 30000;
    linkStateChangeParameters.LinkStateChangeParameters.LinkQuality = 56;

    // Default linkId for non-Mlo connections is 0
    pLinkInfo[0].LinkID = 0;
    RtlCopyMemory(&pLinkInfo[0].LocalLinkMACAddress, &m_LocalLinkAddresses[0], sizeof(DOT11_MAC_ADDRESS));
    RtlCopyMemory(&pLinkInfo[0].PeerLinkMACAddress, &m_ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
    pLinkInfo[0].ChannelNumber = 6;
    pLinkInfo[0].BandId = WDI_BAND_ID_2400;
    pLinkInfo[0].RSSI = -50;
    pLinkInfo[0].Bandwidth = 40;
    pLinkInfo[0].TxMCS = 3;
    pLinkInfo[0].RxMCS = 4;

    if (numLinks > 1)
    {
        // For Mlo connections, set the link ID for the first link to 1
        pLinkInfo[0].LinkID = 1;

        pLinkInfo[1].LinkID = 2;
        RtlCopyMemory(&pLinkInfo[1].LocalLinkMACAddress, &m_LocalLinkAddresses[1], sizeof(DOT11_MAC_ADDRESS));
        RtlCopyMemory(&pLinkInfo[1].PeerLinkMACAddress, &m_ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
        pLinkInfo[1].ChannelNumber = 36;
        pLinkInfo[1].BandId = WDI_BAND_ID_5000;
        pLinkInfo[1].RSSI = -30;
        pLinkInfo[1].Bandwidth = 160;
        pLinkInfo[1].TxMCS = 8;
        pLinkInfo[1].RxMCS = 9;
    }

    linkStateChangeParameters.LinkInfo.pElements = pLinkInfo;
    linkStateChangeParameters.LinkInfo.ElementCount = numLinks > 1 ? 2 : 1;

    ntStatus = GenerateWdiIndicationLinkStateChangeFromIhv(&linkStateChangeParameters, 0, &pDeviceContext->TlvContext, &cbOutput, &pOutput);
    if (STATUS_SUCCESS == ntStatus)
    {
        WifiIhvSendUnsolicitedIndicationToOs(m_Device, pWdiHeader, WDI_INDICATION_LINK_STATE_CHANGE, pOutput, cbOutput);
        FreeGenerated(pOutput);
    }
    else
    {
        WFCError("Failed to generate WDI_INDICATION_LINK_STATE_CHANGE - 0x%08x\n", ntStatus);
    }

    return ntStatus;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvPerformAssociation(
    const struct ArrayOfElements<WDI_CONNECT_BSS_ENTRY_CONTAINER>* pPreferredBSSEntryList,
    const struct ArrayOfElements<WDI_AUTH_ALGORITHM>* pAuthenticationAlgorithms,
    const PWDI_MESSAGE_HEADER pWdiHeader)
{

    ULONG bssIndex = 0;
    ULONG connectEntry = ConnectEntryId_MAX;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 NewConnectEntryId = 0; // Disconnected State
    WDI_AUTH_ALGORITHM NewAuthAlgo = pAuthenticationAlgorithms->pElements[0];
    UCHAR pucData[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10 };

    PWIFI_IHV_DEVICE_CONTEXT pDeviceContext = WifiGetIhvDeviceContext(m_Device);

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
                    m_LastConnectTransactionId = pWdiHeader->TransactionId;
#ifdef NETV_SUPPORT_TX_DEMUXING
                    // add peer on datapath
                    WifiAdapterAddPeer(pDeviceContext->netAdapters[pWdiHeader->PortId],
                        reinterpret_cast<NET_EUI48_ADDRESS*>(g_ConnectEntries[connectEntry].pMacAddress));
#endif // NETV_SUPPORT_TX_DEMUXING
#ifdef  WIFI_IHV_HANDSHAKE
                    // Pretend to recieve M1 on datapath before, the association complete has made it up the control path.
                    RecieveDatapathFrame(0x33, sizeof(pucData), pucData);
#endif //  WIFI_IHV_HANDSHAKE

                    if (NewAuthAlgo == WDI_AUTH_ALGO_WPA3_SAE)
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

                            WifiIhvSendUnsolicitedIndicationToOs(m_Device, pWdiHeader, WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED, pOutput, cbOutput);

                            FreeGenerated(pOutput);

                            break;
                        }
                    }

                    // Get the assoc status
                    RtlCopyMemory(&assocStatus, &pAssociationResult[18], sizeof(ULONG));

                    // Send the association indication
                    WifiIhvSendUnsolicitedIndicationToOs(
                        m_Device,
                        pWdiHeader,
                        WDI_INDICATION_ASSOCIATION_RESULT,
                        g_ConnectEntries[connectEntry].pTlvAssociationResult,
                        g_ConnectEntries[connectEntry].TlvAssociationResultSize);

                    break;
                }
            }

            // If we found a matching BSSID entry, we are done
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
        RtlCopyMemory(&m_ConnectedPeer, pPreferredBSSEntryList->pElements[bssIndex].BSSID.Address, sizeof(DOT11_MAC_ADDRESS));
    }

    m_LastConnectEntryId = NewConnectEntryId;
    m_LastAuthAlgo = NewAuthAlgo;

    if (STATUS_SUCCESS == ntStatus && (WDI_AUTH_ALGO_WPA3_SAE != m_LastAuthAlgo))
    {
        // Report link quality
        ntStatus = WifiIhvSendLinkStateIndication(pWdiHeader, 2);
    }

    return ntStatus;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvSetSaeAuthParams(const WDI_SET_SAE_AUTH_PARAMS_COMMAND& setSAEAuthParams, const PWDI_MESSAGE_HEADER pWdiHeader, UINT)
{
    //Since this is DIRECT OID, need to check the m_LastConnectTransactionId match
    if (pWdiHeader->TransactionId != m_LastConnectTransactionId) 
    {
        WFCError("WDI_SET_SAE_AUTH_PARAMS called with invalid TransactionId: %llu, expected: %llu\n",
            pWdiHeader->TransactionId, m_LastConnectTransactionId);
        return STATUS_INVALID_DEVICE_REQUEST;   
    }

    WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED_PARAMETERS SAEAuthParamsNeeded{};
    UINT8* pOutput = nullptr;
    ULONG cbOutput = 0;

    // Commit frame when Status = 0:    FiniteCyclicGroup + Scalar + Element
    // Commit frame when Status = 76:   FiniteCyclicGroup + AntiCloggingToken
    WFCInfo("WDI_SET_SAE_AUTH_PARAMS called!!!\n");
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

        // Send the association indication
        // This assumes that the association result fields are in the right order
        if (WDI_AUTH_ALGO_WPA3_SAE == m_LastAuthAlgo)
        {
            g_ConnectEntries[m_LastConnectEntryId].pTlvAssociationResult[27] = (UCHAR)WDI_AUTH_ALGO_WPA3_SAE;
        }
        else
        {
            g_ConnectEntries[m_LastConnectEntryId].pTlvAssociationResult[27] = (UCHAR)WDI_AUTH_ALGO_RSNA_PSK;
        }

        WifiIhvSendUnsolicitedIndicationToOs(
            m_Device,
            pWdiHeader,
            WDI_INDICATION_ASSOCIATION_RESULT,
            g_ConnectEntries[m_LastConnectEntryId].pTlvAssociationResult,
            g_ConnectEntries[m_LastConnectEntryId].TlvAssociationResultSize);
        RtlCopyMemory(&m_ConnectedPeer, &g_ConnectEntries[m_LastConnectEntryId].pMacAddress, sizeof(DOT11_MAC_ADDRESS));

        // Report link quality
        WX_RETURN_NTSTATUS_IF_NOT_NT_SUCCESS_MSG(WifiIhvSendLinkStateIndication(pWdiHeader, 1),
            "Failed WifiIhvSendLinkStateIndication");

        // Complete the transaction to let the M4 hanlder WifiIhvGetPendingTransitionStatus know that association is complete
        m_LastConnectTransactionId = 0;

#ifdef WIFI_IHV_HANDSHAKE
        //
        // Receive M1 frame of 4-way handshake
        //
        RecieveDatapathFrame(0x33, sizeof(pucM1SaeFrame), pucM1SaeFrame);

        //
        // Receive M3 frame of 4-way handshake
        //
        RecieveDatapathFrame(0x33, sizeof(pucM3SaeFrame), pucM3SaeFrame);
#endif
        return STATUS_SUCCESS;
    }

    RtlCopyMemory(SAEAuthParamsNeeded.BssId.Address, &g_ConnectEntries[m_LastConnectEntryId].pMacAddress, sizeof(DOT11_MAC_ADDRESS));

    auto ndisStatus =
        GenerateWdiIndicationSaeAuthParamsNeeded(&SAEAuthParamsNeeded, 0, m_TlvContext, &cbOutput, &pOutput);
    if (ndisStatus == NDIS_STATUS_SUCCESS)
    {
        WifiIhvSendUnsolicitedIndicationToOs(m_Device, pWdiHeader, WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED, pOutput, cbOutput);

        FreeGenerated(pOutput);
    }
    else
    {
        WFCError("Failed to generate WDI_INDICATION_SAE_AUTH_PARAMS_NEEDED - 0x%08x\n", ndisStatus);
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvDisconnect(const WDI_TASK_DISCONNECT_PARAMETERS&, const PWDI_MESSAGE_HEADER pWdiHeader, UINT)
{
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
    WDI_ASSOC_STATUS DisassocStatus = WDI_ASSOC_STATUS_DISASSOCIATED_BY_HOST;

    RtlCopyMemory(&s_TLV_Disassociation[4], &m_ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));
    RtlCopyMemory(&s_TLV_Disassociation[10], &DisassocStatus, sizeof(ULONG));

    // Send the disassociation indication
    WifiIhvSendUnsolicitedIndicationToOs(m_Device, pWdiHeader, WDI_INDICATION_DISASSOCIATION, s_TLV_Disassociation, sizeof(s_TLV_Disassociation));

    m_LastConnectEntryId = 0; // Disconnected State

#ifdef NETV_SUPPORT_TX_DEMUXING
    WifiAdapterRemovePeer(
        WifiGetIhvDeviceContext(m_Device)->netAdapters[pWdiHeader->PortId],
        reinterpret_cast<NET_EUI48_ADDRESS*>(&m_ConnectedPeer));
#endif
    RtlZeroMemory(&m_ConnectedPeer, sizeof(DOT11_MAC_ADDRESS));

    return STATUS_SUCCESS;
}

// -------- WDI_GET_SUPPORTED_DEVICE_SERVICES (OID_WDI_GET_SUPPORTED_DEVICES) --------
// Property GET: the request has no input (Inputs is empty). Builds the
// WDI_TLV_DEVICE_SERVICE_GUID_LIST result advertising GUID_OEM_SAMPLE_DEVICE_SERVICE and
// serializes it via the generated TLV generator, so the OS learns which device services
// this driver supports.
// OutBuffer receives the full WDI message (header + TLVs); BytesWritten = total length.
_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvGetSupportedDeviceServices(const WDI_GET_SUPPORTED_DEVICE_SERVICES_INPUTS& Inputs, void* OutBuffer, ULONG OutBufferLen, ULONG& BytesWritten)
{
    UNREFERENCED_PARAMETER(Inputs); // GET request carries no input data

    BytesWritten = sizeof(WDI_MESSAGE_HEADER);

    if (OutBuffer == nullptr || OutBufferLen < sizeof(WDI_MESSAGE_HEADER))
    {
        WFCError("GetSupportedDeviceServices: invalid out buffer (OutBufferLen=%u)", OutBufferLen);
        return STATUS_INVALID_PARAMETER;
    }

    // WDI_TLV_DEVICE_SERVICE_GUID_LIST: a list containing our single device service GUID.
    // WDI_GUID_LIST_CONTAINER is ArrayOfElements<GUID>; SimpleAssign points it at our
    // stack array (the generator copies the data while serializing the TLV).
    GUID supportedServices[] = { GUID_OEM_SAMPLE_DEVICE_SERVICE };

    WDI_GET_SUPPORTED_DEVICE_SERVICES_PARAMETERS results{};
    results.DeviceServiceGUIDList.SimpleAssign(supportedServices, ARRAYSIZE(supportedServices));

    // Generate the TLV byte stream. ReservedHeaderLength reserves room for the
    // WDI_MESSAGE_HEADER at the front of the produced buffer.
    ULONG generatedLength = 0;
    UINT8* pGenerated = nullptr;

    NDIS_STATUS genStatus = GenerateWdiGetSupportedDeviceServices(
        &results, sizeof(WDI_MESSAGE_HEADER), m_TlvContext, &generatedLength, &pGenerated);

    NTSTATUS ntStatus = Wifi::ConvertNDISSTATUSToNTSTATUS(genStatus);
    if (!NT_SUCCESS(ntStatus) || pGenerated == nullptr)
    {
        WFCError("GetSupportedDeviceServices: Generate failed, status=%!STATUS!", ntStatus);
        return ntStatus;
    }

    if (OutBufferLen < generatedLength)
    {
        WFCError("GetSupportedDeviceServices: out buffer too small (have=%u need=%u)",
            OutBufferLen, generatedLength);
        FreeGenerated(pGenerated);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(OutBuffer, pGenerated, generatedLength);
    BytesWritten = generatedLength;
    FreeGenerated(pGenerated);

    WFCInfo("GetSupportedDeviceServices: advertised %u device service(s), %u bytes",
        ARRAYSIZE(supportedServices), BytesWritten);
    return STATUS_SUCCESS;
}

// -------- OEM Device Service Command (OID_WDI_DEVICE_SERVICE_COMMAND) --------
// Reads the request data blob (WDI_TLV_DEVICE_SERVICE_PARAMS_DATA_BLOB) parsed into
// Inputs.Params, expects "Hello, My Driver", and returns "Nice to meet you, My OEM"
// as the response data blob, serialized via the generated TLV generator.
// OutBuffer receives the full WDI message (header + TLVs); BytesWritten = total length.
_Use_decl_annotations_
NTSTATUS WifiHAL::WifiIhvDeviceServiceCommand(const WDI_DEVICE_SERVICE_COMMAND_INPUTS& Inputs, void* OutBuffer, ULONG OutBufferLen, ULONG& BytesWritten)
{
    BytesWritten = sizeof(WDI_MESSAGE_HEADER);

    if (OutBuffer == nullptr || OutBufferLen < sizeof(WDI_MESSAGE_HEADER))
    {
        WFCError("OEM device service: invalid out buffer (OutBufferLen=%u)", OutBufferLen);
        return STATUS_INVALID_PARAMETER;
    }

    // Log the request data blob ("Hello, My Driver"), if present.
    if (Inputs.Optional.Params_IsPresent &&
        Inputs.Params.ElementCount > 0 &&
        Inputs.Params.pElements[0].ElementCount > 0 &&
        Inputs.Params.pElements[0].pElements != nullptr)
    {
        WFCInfo("OEM device service: opcode=0x%08X, received %u-byte data blob: %hs",
            Inputs.Opcode,
            Inputs.Params.pElements[0].ElementCount,
            reinterpret_cast<const char*>(Inputs.Params.pElements[0].pElements));
    }
    else
    {
        WFCInfo("OEM device service: opcode=0x%08X, no input data blob", Inputs.Opcode);
    }

    // Build the response data blob ("Nice to meet you, My OEM"), including the null terminator.
    // SimpleAssign points the blob at this buffer (no copy); the generator copies the bytes
    // while serializing, and the buffer outlives that call.
    UINT8 responseBytes[] = OEM_DEVICE_SERVICE_RESPONSE_STRING;

    WDI_BYTE_BLOB responseBlob{};
    responseBlob.SimpleAssign(responseBytes, static_cast<UINT32>(sizeof(responseBytes)));

    WDI_DEVICE_SERVICE_COMMAND_PARAMETERS params{};
    params.Optional.Params_IsPresent = TRUE;
    params.Params.SimpleAssign(&responseBlob, 1);

    // Serialize WDI_TLV_DEVICE_SERVICE_PARAMS_DATA_BLOB into the response message.
    ULONG generatedLength = 0;
    UINT8* pGenerated = nullptr;

    NDIS_STATUS genStatus = GenerateWdiDeviceServiceCommand(
        &params, sizeof(WDI_MESSAGE_HEADER), m_TlvContext, &generatedLength, &pGenerated);

    NTSTATUS ntStatus = Wifi::ConvertNDISSTATUSToNTSTATUS(genStatus);
    if (!NT_SUCCESS(ntStatus) || pGenerated == nullptr)
    {
        WFCError("OEM device service: Generate failed, status=%!STATUS!", ntStatus);
        return ntStatus;
    }

    if (OutBufferLen < generatedLength)
    {
        WFCError("OEM device service: out buffer too small (have=%u need=%u)",
            OutBufferLen, generatedLength);
        FreeGenerated(pGenerated);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(OutBuffer, pGenerated, generatedLength);
    BytesWritten = generatedLength;
    FreeGenerated(pGenerated);

    WFCInfo("OEM device service: responded with \"%hs\" (%u bytes)",
        reinterpret_cast<const char*>(responseBytes), BytesWritten);
    return STATUS_SUCCESS;
}
