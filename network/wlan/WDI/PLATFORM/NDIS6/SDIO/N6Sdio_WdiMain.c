#include "Mp_Precomp.h"
#include "CustomOid.h"

#if WPP_SOFTWARE_TRACE
#include "N6Sdio_WdiMain.tmh"
#endif

typedef struct _WDI_FRAME_MYWORKSPCE
{   //
    // example work space
    //
    UINT32  Signature; // 'Meta'
    WDI_PEER_ID PeerId;
    WDI_EXTENDED_TID ExTid;
    UINT32  Whatever;
    LIST_ENTRY ListEntry;
} WDI_FRAME_MYWORKSPACE, *PWDI_FRAME_MYWORKSPACE;

static
VOID
n6sdioWdi_FillAPAttr(
	_In_  WDI_AP_ATTRIBUTES_CONTAINER						*attr,
	_In_  ADAPTER											*pAdapter
    )
{
    static WDI_ALGO_PAIRS UcastAlgoList[] = 
    {
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_NONE},	// 0
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_CCMP},	//14
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP40},	// 1
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP40}, 	// 2
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP104},	// 3
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP104},	// 4
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP},	// 5
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP},	// 6
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_TKIP},	// 7
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_TKIP},	// 8
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_TKIP},	// 9
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_TKIP},	//10
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_CCMP},	//11
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_CCMP},	//12
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_CCMP},	//13

        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_TKIP	},    //15
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_CCMP},   //16

        {DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPCCMP	},    //17
        {DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPTKIP},   //18

        {DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPCCMP},   //19
        {DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPTKIP},   //20

        {DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
        {DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
    };

    static WDI_ALGO_PAIRS McastAlgoList[] = 
    {
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_NONE},	// 0
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_CCMP},	//14
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP40},	// 1
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP40}, 	// 2
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP104},	// 3
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP104},	// 4
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP},	// 5
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP},	// 6
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_TKIP},	// 7
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_TKIP},	// 8
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_TKIP},	// 9
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_TKIP},	//10
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_CCMP},	//11
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_CCMP},	//12
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_CCMP},	//13

        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_WEP40},  //15
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_WEP104}, //16
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_TKIP},     //17
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_CCMP},   //18

        {DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
        {DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
    };

    // WIFI_AP_CAPABILITIES_CONTAINER APCapabilities;

    // AP cap, ref N62CSet80211Attributes
    attr->APCapabilities.ScanSSIDListSize = AP_SCAN_SSID_LIST_MAX_SIZE;
    attr->APCapabilities.DesiredSSIDListSize = AP_DESIRED_SSID_LIST_MAX_SIZE;
    attr->APCapabilities.PrivacyExemptionListSize = NATIVE_802_11_MAX_PRIVACY_EXEMPTION;
    attr->APCapabilities.AssociationTableSize = AP_DEFAULT_ALLOWED_ASSOCIATION_COUNT;
    attr->APCapabilities.KeyMappingTableSize = DOT11_MAX_NUM_DEFAULT_KEY; // TODO: which val to fill?
    attr->APCapabilities.DefaultKeyTableSize = DOT11_MAX_NUM_DEFAULT_KEY;
    attr->APCapabilities.WEPKeyValueMaxLength = (104 / 8);
    attr->APCapabilities.RadarDetectionSupported = 0;

    // Unicast algo, ref N62CSet80211Attributes
    attr->UnicastAlgorithms.ElementCount = ARRAYSIZE(UcastAlgoList);
    attr->UnicastAlgorithms.pElements = UcastAlgoList;
    attr->UnicastAlgorithms.MemoryInternallyAllocated = FALSE;

    // Multicast algo 
    attr->MulticastDataAlgorithms.ElementCount = ARRAYSIZE(McastAlgoList);
    attr->MulticastDataAlgorithms.pElements = McastAlgoList;
    attr->MulticastDataAlgorithms.MemoryInternallyAllocated = FALSE;
}

static
VOID
n6sdioWdi_FillWiFiVirtualizationParam(
	_In_  WDI_VIRTUALIZATION_ATTRIBUTES_CONTAINER			*attr,
	_In_  ADAPTER											*pAdapter
    )
{
    attr->VirtualizationCapabilities.ExtSTACount = 1;
    attr->VirtualizationCapabilities.WiFiDirectGroupCount = 1;
    attr->VirtualizationCapabilities.APCount = 1;
    attr->VirtualizationCapabilities.SimultaneousGoCount = 1;
    attr->VirtualizationCapabilities.SimultaneousMultiChannelOperationLimit = 1;
    attr->VirtualizationCapabilities.SimultaneousSTAWFDClientCount = 1;
}


static
VOID
n6sdioWdi_FillP2PAttr(
	_In_  WDI_P2P_ATTRIBUTES_CONTAINER						*attr,
	_In_  ADAPTER											*pAdapter
    )
{

    u1Byte	bHwSupportMACRandom = FALSE;

    // P2P attr
    cpMacAddr(attr->P2PCapabilities.DeviceAddress, pAdapter->CurrentAddress);
	
    pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_MAC_ADRESS_COX_CAP, &bHwSupportMACRandom);
    RT_TRACE(COMP_INIT, DBG_LOUD, ("n6sdioWdi_FillP2PAttr() : bHwSupportMACRandom( %d ) RegSupportMACRandom(%d)\n" , bHwSupportMACRandom , pAdapter->MgntInfo.RegSupportMACRandom));
    if(  !pAdapter->MgntInfo.RegSupportMACRandom ||bHwSupportMACRandom )
    {
		attr->P2PCapabilities.DeviceAddress[0] |= BIT1;
    }
    attr->P2PCapabilities.ConcurrentGOCount = 1;
    attr->P2PCapabilities.ConcurrentClientCount = 1;
    attr->P2PCapabilities.WPSVersionSupport = 3;
    attr->P2PCapabilities.ServiceDiscoverySupported = 1;
    attr->P2PCapabilities.ServiceNameDiscoverySupported = 1;
	attr->P2PCapabilities.ServiceNameAdvertisementsMaxBytesSupported = 500;
    attr->P2PCapabilities.ServiceInformationDiscoverySupported = 1;
    attr->P2PCapabilities.ServiceInformationAdvertisementsMaxBytesSupported = 5000;
    attr->P2PCapabilities.BackgroundDiscoverySupported = 0;
    attr->P2PCapabilities.ClientDiscoverabilitySupported = 0;
    attr->P2PCapabilities.InfrastructureManagementSupported = 0;
    attr->P2PCapabilities.MaxSecondaryAdapterTypeListSize = 8;	
    attr->P2PCapabilities.DiscoveryFilterListSize = 2;
    attr->P2PCapabilities.GOClientTableSize = 2;
    attr->P2PCapabilities.MaxVendorSpecificExtensionIESize = 255;
    attr->P2PCapabilities.PassiveAvailabilityListenStateSupported = 0;
    attr->P2PCapabilities.OperatingChannelUpdatesSupported = 0;

    // Addr list
    attr->InterfaceAddressList.ElementCount = 1;
    attr->InterfaceAddressList.pElements = (WDI_MAC_ADDRESS *)pAdapter->CurrentAddress;
}

static
VOID
n6sdioWdi_FillInterfaceAttr(
	_In_  WDI_INTERFACE_ATTRIBUTES_CONTAINER					*attr,
	_In_  ADAPTER												*pAdapter
    )
{
	PMGNT_INFO					pMgntInfo = &(pAdapter->MgntInfo);	
	PRT_HIGH_THROUGHPUT			pHTInfo = GET_HT_INFO(pMgntInfo);
	BOOLEAN						bAntDivSupport = FALSE;

	static CHAR FirmwareVersion[] = {'r', 't', 'w', 'l', 'a', 'n', 's', 0};
	static u1Byte	MACAddrRandomMask[6]={0xff,0xff,0xff,0xff,0xff,0x00};
	
	attr->InterfaceCapabilities.MTUSize = WDI_802_11_MTU_SIZE;
	attr->InterfaceCapabilities.MaxMultiCastListSize = WDI_MAX_MCAST_LIST_NUM;
	attr->InterfaceCapabilities.BackFillSize = WDI_802_11_DATA_BACK_FILL_SIZE;
	PlatformMoveMemory(
		attr->InterfaceCapabilities.Address.Address,
		pAdapter->PermanentAddress,
		ETH_LENGTH_OF_ADDRESS);
	attr->InterfaceCapabilities.MaxTxRate = (WDI_802_11_MAX_XMIT_LINK_SPEED/1000);	// kbps
	attr->InterfaceCapabilities.MaxRxRate = (WDI_802_11_MAX_RCV_LINK_SPEED/1000);	// kbps
	attr->InterfaceCapabilities.HardwareRadioState = (pMgntInfo->RfOffReason & RF_CHANGE_BY_HW)?FALSE:TRUE;
	attr->InterfaceCapabilities.SoftwareRadioState = (pMgntInfo->RegRfOff == TRUE)?FALSE:TRUE;
	attr->InterfaceCapabilities.SupportPLR = TRUE;
	attr->InterfaceCapabilities.SupportFLR = TRUE;
	attr->InterfaceCapabilities.ActionFramesSupported = TRUE;

	attr->InterfaceCapabilities.NumRxStreams = (pHTInfo->nRxSPStream > 0)?pHTInfo->nRxSPStream:1;
	attr->InterfaceCapabilities.NumTxStreams = (pHTInfo->nTxSPStream > 0)?pHTInfo->nTxSPStream:1;
	attr->InterfaceCapabilities.NumChannels = (pAdapter->MgntInfo.RegMultiChannelFcsMode > 0)?2:1;// TODO: check

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_ANT_DIV, &bAntDivSupport);
	attr->InterfaceCapabilities.AntennaDiversity = bAntDivSupport;
	attr->InterfaceCapabilities.Support_eCSA = pMgntInfo->RegSupportECSA;

	// MAC Address Randomization contraints for some hardware.
	RT_TRACE(COMP_INIT, DBG_LOUD, ("n6sdioWdi_FillInterfaceAttr() : RegSupportMACRandom(%d)\n"  , pMgntInfo->RegSupportMACRandom));
	if(pMgntInfo->RegSupportMACRandom)
	{
		attr->InterfaceCapabilities.MACAddressRandomization = 1;
		cpMacAddr(attr->InterfaceCapabilities.MACAddressRandomizationMask.Address , MACAddrRandomMask);
	}

	attr->InterfaceCapabilities.BluetoothCoexistenceSupport = WDI_BLUETOOTH_COEXISTENCE_UNKNOWN;
	attr->InterfaceCapabilities.SupportsNonWdiOidRequests = TRUE;

	attr->InterfaceCapabilities.FastTransitionSupported = pMgntInfo->RegSupportFT;

	// TODO: MU-MIMO Support

	attr->InterfaceCapabilities.CannotSupportMiracastSink = FALSE;

	attr->FirmwareVersion.ElementCount = ARRAYSIZE( FirmwareVersion );
	attr->FirmwareVersion.pElements = FirmwareVersion;
}


static
VOID
n6sdioWdi_FillStationAttr(
	_In_  WDI_STATION_ATTRIBUTES_CONTAINER					*attr,
	_In_  ADAPTER											*pAdapter
    )
{
    static WDI_ALGO_PAIRS UcastAlgoList[] = 
    {
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_NONE},	// 0
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_CCMP},	//14
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP40},	// 1
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP40}, 	// 2
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP104},	// 3
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP104},	// 4
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP},	// 5
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP},	// 6
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_TKIP},	// 7
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_TKIP},	// 8
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_TKIP},	// 9
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_TKIP},	//10
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_CCMP},	//11
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_CCMP},	//12
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_CCMP},	//13

        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_TKIP	},    //15
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_CCMP},   //16

        {DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPCCMP	},    //17
        {DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPTKIP},   //18

        {DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPCCMP},   //19
        {DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPTKIP},   //20

        {DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
        {DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
    };
	static WDI_ALGO_PAIRS UcastAlgoListWiFiCfg[] = 
	{
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_NONE},	// 0
		{WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_CCMP},	//14
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP40},	// 1
		{WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP40}, 	// 2
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP104},	// 3
		{WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP104},	// 4
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP},	// 5
		{WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP},	// 6
		//{WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_TKIP},	// 7
		//{WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_TKIP},	// 8
		//{WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_TKIP},	// 9
		//{WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_TKIP},	//10
		//{WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_CCMP},	//11
		//{WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_CCMP},	//12
		{WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_CCMP},	//13
		
		{DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_TKIP	},    //15
		{DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_CCMP},   //16

		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPCCMP	},    //17
		{DOT11_AUTH_ALGO_CCKM,				DOT11_CIPHER_ALGO_MFPTKIP},   //18
		
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPCCMP},   //19
		{DOT11_AUTH_ALGO_RSNA,					DOT11_CIPHER_ALGO_MFPTKIP},   //20
		
		{DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
		{DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
	};
    static WDI_ALGO_PAIRS McastAlgoList[] = 
    {
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_NONE},	// 0
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_CCMP},	//14
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP40},	// 1
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP40}, 	// 2
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP104},	// 3
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP104},	// 4
        {WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP},	// 5
        {WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP},	// 6
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_TKIP},	// 7
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_TKIP},	// 8
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_TKIP},	// 9
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_TKIP},	//10
        {WDI_AUTH_ALGO_WPA,					WDI_CIPHER_ALGO_CCMP},	//11
        {WDI_AUTH_ALGO_WPA_PSK,				WDI_CIPHER_ALGO_CCMP},	//12
        {WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_CCMP},	//13

        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_WEP40},  //15
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_WEP104}, //16
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_TKIP},     //17
        {DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_CCMP},   //18

        {DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
        {DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},   //for WAPI IHV Support add by ylb  20111114
    };
	static WDI_ALGO_PAIRS McastAlgoListWiFiCfg[] = 
	{
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_NONE},	// 0
		{WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_CCMP},	//14
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP40}, // 1
		{WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP40}, 	// 2
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP104},	// 3
		{WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP104},	// 4
		{WDI_AUTH_ALGO_80211_OPEN,			WDI_CIPHER_ALGO_WEP},	// 5
		{WDI_AUTH_ALGO_80211_SHARED_KEY,	WDI_CIPHER_ALGO_WEP},	// 6
		//{WDI_AUTH_ALGO_WPA, 				WDI_CIPHER_ALGO_TKIP},	// 7
		//{WDI_AUTH_ALGO_WPA_PSK, 			WDI_CIPHER_ALGO_TKIP},	// 8
		//{WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_TKIP},	// 9
		//{WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_TKIP},	//10
		//{WDI_AUTH_ALGO_WPA, 				WDI_CIPHER_ALGO_CCMP},	//11
		//{WDI_AUTH_ALGO_WPA_PSK, 			WDI_CIPHER_ALGO_CCMP},	//12
		{WDI_AUTH_ALGO_RSNA,					WDI_CIPHER_ALGO_CCMP},	//13
		
		{DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_WEP40},  //15
		{DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_WEP104}, //16
		{DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_TKIP},	   //17
		{DOT11_AUTH_ALGO_CCKM,				WDI_CIPHER_ALGO_CCMP},	 //18
		
		{DOT11_AUTH_ALGO_WAPI_PSK,			DOT11_CIPHER_ALGO_WAPI_SMS4},	//for WAPI IHV Support add by ylb  20111114
		{DOT11_AUTH_ALGO_WAPI_CERTIFICATE,	DOT11_CIPHER_ALGO_WAPI_SMS4},	//for WAPI IHV Support add by ylb  20111114
	};
    static WDI_ALGO_PAIRS  McastMFPCipherList[] = 
    {
        {WDI_AUTH_ALGO_RSNA_PSK,			WDI_CIPHER_ALGO_BIP},	// 0
        {WDI_AUTH_ALGO_RSNA,				WDI_CIPHER_ALGO_BIP},	// 1
    };

    attr->StationCapabilities.ScanSSIDListSize = WDI_802_11_MAX_SCAN_SSID;
    attr->StationCapabilities.DesiredBSSIDListSize = WDI_802_11_MAX_DESIRED_BSSID;
    attr->StationCapabilities.DesiredSSIDListSize = WDI_802_11_MAX_DESIRED_SSID;
    attr->StationCapabilities.PrivacyExemptionListSize = WDI_802_11_MAX_PRIVACY_EXEMPTION;
    attr->StationCapabilities.KeyMappingTableSize = WDI_802_11_MAX_KEY_MAPPING_ENTRY;
    attr->StationCapabilities.DefaultKeyTableSize = WDI_802_11_MAX_DEFAULT_KEY_ENTRY;
    attr->StationCapabilities.WEPKeyValueMaxLength = WDI_802_11_MAX_WEP_KEY_LENGTH;
    attr->StationCapabilities.MaxNumPerSTA = WDI_802_11_MAX_PER_STA_DEFAULT_KEY;
    attr->StationCapabilities.SupportedQOSFlags = 0;
    attr->StationCapabilities.HostFIPSModeImplemented = TRUE;
    attr->StationCapabilities.MFPCapable = TRUE;
    attr->StationCapabilities.AutoPowerSaveMode = TRUE;
	
	//Note: it is recommended LE do not cache BSS list to reduce the effort
#if BSS_LIST_CACHE
    attr->StationCapabilities.BSSListCachemanagement = TRUE;
#else
    attr->StationCapabilities.BSSListCachemanagement = FALSE;
#endif

	//Note: it is recommended LE do not override BSS selection set by UE
#if CONNECT_BSS_SELECTION_OVERRIDE
    attr->StationCapabilities.ConnectBSSSelectionOverride = TRUE;
#else
    attr->StationCapabilities.ConnectBSSSelectionOverride = FALSE;
#endif
    
    attr->StationCapabilities.uMaxNetworkOffloadListSize = WDI_802_11_MAX_NETWORKOFFLOAD_SIZE;
    attr->StationCapabilities.HESSIDConnectionSupported = FALSE;
    // Following capabilities is added after 0.90_1
    // TODO: [WDI] Fill following capabilities according to hw support
    //attr->StationCapabilities.HESSIDConnectionOffloadSupported = FALSE;
    attr->StationCapabilities.DisconnectedStandbySupported = FALSE;

    //4 Unicast Algorithm
    attr->Optional.UnicastAlgorithms_IsPresent = TRUE;
	if(pAdapter->MgntInfo.bWiFiConfg)
	{
		attr->UnicastAlgorithms.ElementCount = ARRAYSIZE( UcastAlgoListWiFiCfg );
		attr->UnicastAlgorithms.pElements = UcastAlgoListWiFiCfg;
	}
	else
	{
		attr->UnicastAlgorithms.ElementCount = ARRAYSIZE( UcastAlgoList );
		attr->UnicastAlgorithms.pElements = UcastAlgoList;
	}

    //4 Multicast Data Algorithm
    attr->Optional.MulticastDataAlgorithms_IsPresent = TRUE;
	if(pAdapter->MgntInfo.bWiFiConfg)
	{
		attr->MulticastDataAlgorithms.ElementCount = ARRAYSIZE( McastAlgoListWiFiCfg );
		attr->MulticastDataAlgorithms.pElements = McastAlgoListWiFiCfg;
	}
	else
	{
		attr->MulticastDataAlgorithms.ElementCount = ARRAYSIZE( McastAlgoList );
		attr->MulticastDataAlgorithms.pElements = McastAlgoList;
	}

    //4 Multicast Management Algorithm
    attr->Optional.MulticastManagementAlgorithms_IsPresent = TRUE;
    attr->MulticastManagementAlgorithms.ElementCount = ARRAYSIZE( McastMFPCipherList );
    attr->MulticastManagementAlgorithms.pElements = McastMFPCipherList;
}


static
VOID
n6sdioWdi_FillDatapathAttr(
	_In_  WDI_DATAPATH_ATTRIBUTES_CONTAINER					*attr,
	_In_  ADAPTER											*pAdapter
    )
{
    attr->Optional.DataPathCapabilities_IsPresent = TRUE;
    //8723BS is SDIO Interface, for PCIe interface, use WDI_INTERCONNECT_MEMORY_MAPPED
    attr->DataPathCapabilities.InterconnectType = WDI_INTERCONNECT_MESSAGE_BASED;
    attr->DataPathCapabilities.MaxNumPeers = (u1Byte)MAX_PEER_NUM;
    attr->DataPathCapabilities.TxTargetPriorityQueueing = TRUE;
    attr->DataPathCapabilities.TxMaxScatterGatherElementsPerFrame = 0x00ff;
    attr->DataPathCapabilities.TxExplicitSendCompleteFlagRequired = TRUE;
    attr->DataPathCapabilities.TxMinEffectiveFrameSize = 0;
    attr->DataPathCapabilities.TxFrameSizeGranularity = 1;
    attr->DataPathCapabilities.RxTxForwarding = FALSE;
    attr->DataPathCapabilities.RxMaxThroughput = 300; // (150 Mbps)
}


static
VOID
n6sdioWdi_FillBandInfoAttr(
	_In_  WDI_GET_ADAPTER_CAPABILITIES_PARAMETERS				*pAdapterCap,
	_In_  ADAPTER												*pAdapter
    )
{
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	u4Byte				WirelessModeCapa=0;

	static WDI_CHANNEL_MAPPING_ENTRY channelMap24[] = {
		{1, 0x96C},
		{2, 0x971},
		{3, 0x976},
		{4, 0x97B},
		{5, 0x980},
		{6, 0x985},
		{7, 0x98A},
		{8, 0x98F},
		{9, 0x994},
		{10, 0x999},
		{11, 0x99E},
		{12, 0x9A3},
		{13, 0x9A8},
		{14, 0x9B4}
		};
	// 8723BS only support 2.4GHz
	static WDI_CHANNEL_MAPPING_ENTRY channelMap5[] = {
		{36, 5180 },
		{40, 5200 },
		{44, 5220 },
		{48, 5240 },
		{52, 5260},
		{56, 5280},
		{60, 5300},
		{64, 5320},
		{100, 5500},
		{104, 5520},
		{108, 5540},
		{112, 5560},
		{116, 5580},
		{120, 5600},
		{124, 5620},
		{128, 5640},
		{132, 5660},
		{136, 5680},
		{140, 5700},
		{149, 5745},
		{153, 5765},
		{157, 5785},
		{161, 5805},
		{165, 5825}
	};
	// Following parameter is added after 0.90_1
	static UINT32		WidthList[] = {20, 40, 80};

	WirelessModeCapa = HalGetSupportedWirelessMode(pAdapter);

	pNdisCommon->BandInfoCount = 0;
	//3 If Support 2.4G band 
	if((WirelessModeCapa & WIRELESS_MODE_N_24G) || (WirelessModeCapa & WIRELESS_MODE_AC_24G) ||(WirelessModeCapa & WIRELESS_MODE_G)||(WirelessModeCapa & WIRELESS_MODE_B))
	{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("==>Fill band info 2.4GHz\n")); 
	//4 BandCapabilities
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].BandCapabilities.BandID = WDI_BAND_ID_2400;
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].BandCapabilities.BandState = TRUE;
	//4 ValidPhyTypes
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.pElements = (WDI_PHY_TYPE*)pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType;
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount = 0;
	if((WirelessModeCapa & WIRELESS_MODE_N_24G) || (WirelessModeCapa & WIRELESS_MODE_AC_24G))
	{
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount++;
	}
	if(WirelessModeCapa & WIRELESS_MODE_G)
	{
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount++;
	}
	if(WirelessModeCapa & WIRELESS_MODE_B) 
	{
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount++;
	}
	//4 ValidChannelTypes
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidChannelTypes.ElementCount = ARRAYSIZE( channelMap24 );
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidChannelTypes.pElements = channelMap24;
	//Following definition is added after 0.90_1
	//4 ChannelWidthList
	// TODO: [WDI] Determine channel width
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ChannelWidthList.ElementCount = 2;
	pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ChannelWidthList.pElements = WidthList;
	pNdisCommon->BandInfoCount++;
	}
	//3 Support 5G band if hardware capable
	if( (WirelessModeCapa & WIRELESS_MODE_A) || (WirelessModeCapa & WIRELESS_MODE_N_5G) ||
		(WirelessModeCapa & WIRELESS_MODE_AC_5G) || (WirelessModeCapa & WIRELESS_MODE_AC_ONLY) )
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("==>Fill band info 5GHz\n")); 
		//4 BandCapabilities
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].BandCapabilities.BandID = WDI_BAND_ID_5000;
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].BandCapabilities.BandState = TRUE;
		//4 ValidPhyTypes
		if(pNdisCommon->BandInfoCount==1)
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.pElements = (WDI_PHY_TYPE*)&(pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType[pNdisCommon->BandInfo[0].ValidPhyTypes.ElementCount]);
		}
		else
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.pElements = (WDI_PHY_TYPE*)pNdisCommon->pDot11SupportedPhyTypes->dot11PHYType;
		}
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount = 0;
		if(WirelessModeCapa & WIRELESS_MODE_AC_5G)
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount++;
		}
		if(WirelessModeCapa & WIRELESS_MODE_N_5G)
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount++;
		}
		if(WirelessModeCapa & WIRELESS_MODE_A)
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidPhyTypes.ElementCount++;
		}
		//4 ValidChannelTypes
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidChannelTypes.ElementCount = ARRAYSIZE( channelMap5 );
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ValidChannelTypes.pElements = channelMap5;
		// Following define is added after 0.90_1
		//4 ChannelWidthList
		// TODO: [WDI] Determine channel width
		if((WirelessModeCapa & WIRELESS_MODE_AC_5G) || (WirelessModeCapa & WIRELESS_MODE_AC_ONLY))
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ChannelWidthList.ElementCount = 3;
		}
		else if(WirelessModeCapa & WIRELESS_MODE_N_5G)
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ChannelWidthList.ElementCount = 2;
		}
		else	// if(WirelessModeCapa & WIRELESS_MODE_A)
		{
			pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ChannelWidthList.ElementCount = 2;
		}
		pNdisCommon->BandInfo[pNdisCommon->BandInfoCount].ChannelWidthList.pElements = WidthList;
		pNdisCommon->BandInfoCount++;
	}
	RT_TRACE(COMP_INIT, DBG_LOUD, ("==>pNdisCommon->BandInfoCount=%d\n",pNdisCommon->BandInfoCount)); 
	pAdapterCap->BandInfo.ElementCount = pNdisCommon->BandInfoCount;
	pAdapterCap->BandInfo.pElements = pNdisCommon->BandInfo;
}


static
VOID
n6sdioWdi_FillPhyInfoAttr(
	_In_  WDI_GET_ADAPTER_CAPABILITIES_PARAMETERS				*pAdapterCap,
	_In_  ADAPTER												*pAdapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	u1Byte				count = 0;

	static WDI_DATA_RATE_LIST DataRateListHrDsss[] = {
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 4 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 11 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 22 }
	};
	static WDI_DATA_RATE_LIST DataRateListErp[] = {
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 4 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 11 },
 		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 22 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 12 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 18 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 24 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 36 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 48 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 72 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 96 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 108 }
	};
	static WDI_DATA_RATE_LIST DataRateListOfdm[] = {
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 12 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 18 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 24 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 36 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 48 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 72 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 96 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 108 }
	};
 	static WDI_DATA_RATE_LIST DataRateListHt[] = {
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 30 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 60 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 90 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 120 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 180 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 240 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 270 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 300 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 360 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 480 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 600 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 720 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 900 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 960 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1200 }
	};
 	static WDI_DATA_RATE_LIST DataRateListVht[] = {
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 65 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 130 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 195 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 260 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 390 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 520 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 585 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 650 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 780 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 867 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 130 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 260 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 390 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 520 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 780 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1040 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1170 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1300 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1560 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1734 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 195 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 390 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 585 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 780 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1170 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1560 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1755 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 1950 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2340 },
		{ WDI_DATA_RATE_RX_RATE | WDI_DATA_RATE_TX_RATE, 2340 }
 	};
	
	for (count = 0; count < pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries; count ++)
	{
		//4 PhyCapabilities
		pNdisCommon->PhyInfo[count].PhyCapabilities.PhyType = pNdisCommon->pDot11PhyMIBs[count].PhyType;
		pNdisCommon->PhyInfo[count].PhyCapabilities.SupportsCFPoll = MgntActQuery_CfPollable(pAdapter);
		pNdisCommon->PhyInfo[count].PhyCapabilities.MPDUMaxLength = WDI_802_11_MTU_SIZE;
		pNdisCommon->PhyInfo[count].PhyCapabilities.TemperatureClass = dot11_temp_type_unknown;
		pNdisCommon->PhyInfo[count].PhyCapabilities.DiversitySupport = dot11_diversity_support_dynamic;
		//4 TxPowerLevelList
		pNdisCommon->PhyInfo[count].TxPowerLevelList.ElementCount = pNdisCommon->dot11SupportedPowerLevels.uNumOfSupportedPowerLevels;
		pNdisCommon->PhyInfo[count].TxPowerLevelList.pElements = pNdisCommon->dot11SupportedPowerLevels.uTxPowerLevelValues;
		//4 DataRateList
		switch(pNdisCommon->PhyInfo[count].PhyCapabilities.PhyType)
		{
			case WDI_PHY_TYPE_UNKNOWN:
			default:
				break;

			case WDI_PHY_TYPE_HRDSSS:
			{
				pNdisCommon->PhyInfo[count].DataRateList.ElementCount = ARRAYSIZE( DataRateListHrDsss );
				pNdisCommon->PhyInfo[count].DataRateList.pElements = DataRateListHrDsss;
			}
			break;

			case WDI_PHY_TYPE_ERP:
			{
				pNdisCommon->PhyInfo[count].DataRateList.ElementCount = ARRAYSIZE( DataRateListErp );
				pNdisCommon->PhyInfo[count].DataRateList.pElements = DataRateListErp;
			}
			break;

			case WDI_PHY_TYPE_OFDM:
			{
				pNdisCommon->PhyInfo[count].DataRateList.ElementCount = ARRAYSIZE( DataRateListOfdm );
				pNdisCommon->PhyInfo[count].DataRateList.pElements = DataRateListOfdm;
			}
			break;

			case WDI_PHY_TYPE_HT:
			{
				pNdisCommon->PhyInfo[count].DataRateList.ElementCount = ARRAYSIZE( DataRateListHt );
				pNdisCommon->PhyInfo[count].DataRateList.pElements = DataRateListHt;
			}
			break;

			case WDI_PHY_TYPE_VHT:
			{
				pNdisCommon->PhyInfo[count].DataRateList.ElementCount = ARRAYSIZE( DataRateListVht );
				pNdisCommon->PhyInfo[count].DataRateList.pElements = DataRateListVht;
			}
			break;
		}
	}

	pAdapterCap->PhyInfo.ElementCount = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries;
	pAdapterCap->PhyInfo.pElements = pNdisCommon->PhyInfo;
}


static
VOID
n6sdioWdi_FillPMCapabilities(
	_In_  WDI_PM_CAPABILITIES_CONTAINER						*cap,
	_In_  ADAPTER											*pAdapter
	)
{
	BOOLEAN					bEnableWoLCapabilities;
	PMGNT_INFO  			pMgntInfo = &pAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	bEnableWoLCapabilities = MgntIsWoWLANCapabilityEnable(pAdapter);

	if(bEnableWoLCapabilities)
	{
		cap->Flags = NDIS_PM_WAKE_PACKET_INDICATION_SUPPORTED;
		cap->SupportedWoLPacketPatterns = 
			( NDIS_PM_WOL_BITMAP_PATTERN_SUPPORTED |
			NDIS_PM_WOL_MAGIC_PACKET_SUPPORTED |
			NDIS_PM_WOL_IPV4_TCP_SYN_SUPPORTED |
			NDIS_PM_WOL_IPV6_TCP_SYN_SUPPORTED |
			NDIS_PM_WOL_IPV4_DEST_ADDR_WILDCARD_SUPPORTED |
			NDIS_PM_WOL_IPV6_DEST_ADDR_WILDCARD_SUPPORTED |
			NDIS_PM_WOL_EAPOL_REQUEST_ID_MESSAGE_SUPPORTED );
		cap->NumTotalWoLPatterns = MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter);
		cap->MaxWoLPatternSize = MAX_WOL_PATTERN_SIZE;
		cap->MaxWoLPatternOffset = 256;
		cap->MaxWoLPacketSaveBuffer = (MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter))*MAX_WOL_PATTERN_SIZE;
		cap->SupportedProtocolOffloads =  NDIS_PM_PROTOCOL_OFFLOAD_NS_SUPPORTED;
		cap->NumArpOffloadIPv4Addresses = 0;
		cap->NumNSOffloadIPv6Addresses = 2; // WHQL requirment at least 2
		if(pPSC->RegARPOffloadEnable)
		{
			cap->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_ARP_SUPPORTED;
			cap->NumArpOffloadIPv4Addresses = 1;
		}
		if(pPSC->RegNSOffloadEnable)
		{
			cap->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_NS_SUPPORTED;
			cap->NumNSOffloadIPv6Addresses =2;
		}
		if(pPSC->RegGTKOffloadEnable)
			cap->SupportedProtocolOffloads |= NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_SUPPORTED;
		//
		// Set type of WOL supported.
		//
		cap->MinMagicPacketWakeUp   = NdisDeviceStateD3; // 070201, rcnjko: magic packet tought me a lesson, we should call one's name 16 times to awake him. So, be patient.
		cap->MinPatternWakeUp       = NdisDeviceStateD3;
		cap->MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;
		if(pPSC->WoWLANMode == eWakeOnMagicPacketOnly)	//by tynli.
			cap->MinPatternWakeUp       = NdisDeviceStateUnspecified;
		else if (pPSC->WoWLANMode == eWakeOnPatternMatchOnly)	
			cap->MinMagicPacketWakeUp	= NdisDeviceStateUnspecified;

		//
		// Set new attributes for Win8
		//
		// Temporarily mark some attributes because we cannot support now. 
		// They will be enable in the feature. 2012.03.12. by tynli.
		cap->SupportedWakeUpEvents = 0
			/*NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_SUPPORTED |
			NDIS_PM_WAKE_ON_MEDIA_CONNECT_SUPPORTED*/;
		cap->MediaSpecificWakeUpEvents = 
			NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_SUPPORTED |
			NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_SUPPORTED |
			NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_SUPPORTED |
			NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_SUPPORTED;

	}
	else
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("[WDI], n6pciWdi_FillPMCapabilities(): NOT support wake-on-wlan\n"));

		//
		// Not support WOL
		//
		cap->MinMagicPacketWakeUp   = NdisDeviceStateUnspecified;
		cap->MinPatternWakeUp       = NdisDeviceStateUnspecified;
		cap->MinLinkChangeWakeUp    = NdisDeviceStateUnspecified;
	}
}

static
VOID
n6sdioWdi_FillReceiveCoalescingCap(
	_In_  WDI_RECEIVE_COALESCING_CAPABILITIES_CONTAINER		*RecvCoalescingCap,
	_In_  ADAPTER											*pAdapter
	)
{
	//3 The value of "WDI_RECEIVE_FILTER_XXX" defined by WDI are same with defined by NDIS.
	RecvCoalescingCap->EnableFilterTypes = NDIS_RECEIVE_FILTER_PACKET_COALESCING_FILTERS_ENABLED;
	RecvCoalescingCap->EnabledQueueTypes = 0;//!WDI_RECEIVE_FILTER_VM_QUEUES_ENABLE
	RecvCoalescingCap->NumQueues = 0;
	RecvCoalescingCap->SupportedQueueProperties = NDIS_RECEIVE_FILTER_PACKET_COALESCING_SUPPORTED_ON_DEFAULT_QUEUE;
	RecvCoalescingCap->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED|
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_NOT_EQUAL_SUPPORTED;
	RecvCoalescingCap->SupportedHeaders = NDIS_RECEIVE_FILTER_ARP_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV4_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV6_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_UDP_HEADER_SUPPORTED;
	RecvCoalescingCap->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PACKET_TYPE_SUPPORTED;
	RecvCoalescingCap->MaxMacHeaderFilters = 2;
	RecvCoalescingCap->MaxQueueGroups = 0;
	RecvCoalescingCap->MaxQueuesPerQueueGroup = 0;
	RecvCoalescingCap->MinLookaheadSplitSize = 0;
	RecvCoalescingCap->MaxLookaheadSplitSize = 0;
	RecvCoalescingCap->SupportedARPHeaderFields = NDIS_RECEIVE_FILTER_ARP_HEADER_OPERATION_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_SPA_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_TPA_SUPPORTED;
	RecvCoalescingCap->SupportedIPv4HeaderFields = NDIS_RECEIVE_FILTER_IPV4_HEADER_PROTOCOL_SUPPORTED;
	RecvCoalescingCap->SupportedIPv6HeaderFields = NDIS_RECEIVE_FILTER_IPV6_HEADER_PROTOCOL_SUPPORTED;
	RecvCoalescingCap->SupportedUdpHeaderFields = NDIS_RECEIVE_FILTER_UDP_HEADER_DEST_PORT_SUPPORTED;
	RecvCoalescingCap->MaxFieldTestsPerPacketCoalescingFilter = 5;
	RecvCoalescingCap->MaxPacketCoalescingFilters = 10;

}

VOID
n6sdioWdi_InitEvents(
	_In_  PRT_SDIO_DEVICE 	sdiodevice
	)
{	
	// Initialize event.
	NdisInitializeEvent(&sdiodevice->evtSendingNBLCompleted);
	
    //
    // <Roger_Notes> Initialize event to make sure PnP action could be followed by Miniport CheckForHang routine
    // to prevent unexpect bug check code 0x1d.
    // 2009.09.30.
    //
	NdisInitializeEvent(&sdiodevice->SetPnpChkForHangEvent);

	//2Init Event for canceling Asyn_Io pending IRP when unload	
    NdisInitializeEvent(&sdiodevice->AllAsynIoIrpReturnedEvent);

	
    //2Init Event for all SDIO cmd52/cmd53 returned when unload	
    NdisInitializeEvent(&sdiodevice->AllSdioCmdReturnedEvent);

	
    NdisInitializeEvent(&sdiodevice->FwPsClockOffEvent);
    NdisInitializeEvent(&sdiodevice->AllSdioRxTransCompleteEvent);
}

RT_STATUS
n6sdioWdi_GenAdapterCaps(
	_In_  PADAPTER			pAdapter,
	_In_  PWDI_GET_ADAPTER_CAPABILITIES_PARAMETERS	pAdapterCap
)
{
	PMGNT_INFO			pMgntInfo = &pAdapter->MgntInfo;

    // TODO: adjust parameters in N6InitializeNative80211MIBs for SDIO
	N6InitializeNative80211MIBs(pAdapter);
	PlatformZeroMemory(pAdapterCap, sizeof(WDI_GET_ADAPTER_CAPABILITIES_PARAMETERS));

	//4 1. Fill Communication Configuration Attribute
	pAdapterCap->Optional.CommunicationAttributes_IsPresent = TRUE;
	pAdapterCap->CommunicationAttributes.Optional.CommunicationCapabilities_IsPresent = TRUE;
	pAdapterCap->CommunicationAttributes.CommunicationCapabilities.MaxCommandSize = 0x00000800; // 2K

	//4 2. Fill Interface Attribute
	n6sdioWdi_FillInterfaceAttr(&pAdapterCap->InterfaceAttributes, pAdapter);
	
	//4 3. Fill Station Attribute
	n6sdioWdi_FillStationAttr(&pAdapterCap->StationAttributes, pAdapter);

	//4 4. Fill AP Attribute
	pAdapterCap->Optional.APAttributes_IsPresent = TRUE;
	n6sdioWdi_FillAPAttr(&pAdapterCap->APAttributes, pAdapter);
	
	//4 5. Fill Vritualization Attribute
	pAdapterCap->Optional.VirtualizationAttributes_IsPresent = TRUE;
	n6sdioWdi_FillWiFiVirtualizationParam(&pAdapterCap->VirtualizationAttributes, pAdapter);

	// P2P attr
	pAdapterCap->Optional.P2PAttributes_IsPresent = TRUE;
	n6sdioWdi_FillP2PAttr(&pAdapterCap->P2PAttributes, pAdapter);
	
	//4 6. Fill Data Path Attribute
	pAdapterCap->Optional.DatapathAttributes_IsPresent = TRUE;
	n6sdioWdi_FillDatapathAttr(&pAdapterCap->DatapathAttributes, pAdapter);
	
	//4 7. Fill Band Info
	pAdapterCap->Optional.BandInfo_IsPresent = TRUE;
	n6sdioWdi_FillBandInfoAttr(pAdapterCap, pAdapter);
	
	//4 8. Fill Phy Info
	pAdapterCap->Optional.PhyInfo_IsPresent = TRUE;
	n6sdioWdi_FillPhyInfoAttr(pAdapterCap, pAdapter);
	
	//4 9. Fill PM Capability
	// TODO: [WDI] add power management capability
	pAdapterCap->Optional.PmCapabilities_IsPresent = TRUE;
	n6sdioWdi_FillPMCapabilities(&pAdapterCap->PmCapabilities, pAdapter);

	//4 10. CountryRegionList

	//4 11. ReceiveCoalescingCapabilities
	if(TRUE == pMgntInfo->bSupportPacketCoalescing)
	{
		pAdapterCap->Optional.ReceiveCoalescingCapabilities_IsPresent = TRUE;
		n6sdioWdi_FillReceiveCoalescingCap(&pAdapterCap->ReceiveCoalescingCapabilities,pAdapter);
	}

	return RT_STATUS_SUCCESS;
}

NDIS_STATUS
N6SdioWdi_AllocateAdapter(
	_In_  NDIS_HANDLE										NdisMiniportHandle,
	_In_  NDIS_HANDLE										MiniportDriverContext,
	_In_  PNDIS_MINIPORT_INIT_PARAMETERS					MiniportInitParameters,
	_In_  PNDIS_WDI_INIT_PARAMETERS           				NdisWdiInitParameters,
	_Inout_ PNDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES	RegistrationAttributes
	)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
    NDIS_STATUS         ndisStatus = NDIS_STATUS_SUCCESS;
    RT_STATUS           RtStatus = RT_STATUS_SUCCESS;
    PADAPTER            Adapter = NULL;
    PRT_SDIO_DEVICE     sdiodevice = NULL;
    PMGNT_INFO          pMgntInfo = NULL;
    PRT_NDIS6_COMMON    pNdisCommon = NULL;
    PWDI_DATA_STRUCT    pWdi = NULL;
    GUID                PowerSetting_GUID = GUID_ACTIVE_POWERSCHEME;
    GUID                WLANPowerMode_GUID = GUID_WLAN_POWER_MODE;
    NDIS_HANDLE         MiniportAdapterHandle = NdisMiniportHandle;

    RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], ==> N6SdioWdi_AllocateAdapter()\n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Allocate Adapter",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingHexUInt32(NdisWdiInitParameters->WdiVersion, "UE WDI Version"));
#endif

	// Allocate ADAPTER structure
	ndisStatus = N6sdioAllocateAdapter(&Adapter, MiniportAdapterHandle);
	if( ndisStatus != NDIS_STATUS_SUCCESS )
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6sdioAllocateAdapter() failed! ndisStatus=0x%x\n", ndisStatus));
		Adapter = NULL;
		goto error;
	}
	else
	{
		// Invoke resource checking mechanism.
		INIT_RES_MON_OBJ(Adapter->ResMonObjWdi);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6sdioAllocateAdapter() successfully.\n"));
		ADD_RES_TO_MON(Adapter->ResMonObjWdi, InitRM_AllocAdapter); // Add to resource monitor.
	}

    sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
    sdiodevice->pAdapter = Adapter;

    pMgntInfo = &(Adapter->MgntInfo);
    pNdisCommon = Adapter->pNdisCommon;
    pNdisCommon->NdisVersion  = NdisGetVersion();

    pNdisCommon->WDISupport = TRUE;
    pWdi = &(Adapter->pPortCommonInfo->WdiData);
    pWdi->pWdiInitParameters = NdisWdiInitParameters;

    pWdi->bCommandHangHappened = FALSE;

    pWdi->TlvContext.AllocationContext = 0;
	
    pWdi->TlvContext.PeerVersion = NdisWdiInitParameters->WdiVersion;

    if(pNdisCommon->NdisVersion >= NDIS_VERSION_BASE_6_50)
        pNdisCommon->NdisVersion = NDIS_VERSION_BASE_6_50;

    NdisMGetDeviceProperty(MiniportAdapterHandle,
        &(sdiodevice->pPhysDevObj), // PhysicalDeviceObject 
        &(sdiodevice->FunctionalDeviceObject), // FunctionalDeviceObject 
        &(sdiodevice->pSdioDevObj), // NextDeviceObject
        NULL,
        NULL);

    sdiodevice->Sdbusinterface.Size = sizeof(SDBUS_INTERFACE_STANDARD);
    sdiodevice->Sdbusinterface.Version = SDBUS_INTERFACE_VERSION;
    sdiodevice->NextDeviceStackSize = (CHAR)sdiodevice->pSdioDevObj->StackSize + 1;

    //2 Save the related NDIS handles
    sdiodevice->hNdisAdapter = MiniportAdapterHandle;
    pNdisCommon->hNdisAdapter = MiniportAdapterHandle;

    //
    // Fill RegistrationAttributes for NdisMSetMiniportAttributes() called by WDI
    //
    N6_ASSIGN_OBJECT_HEADER(
        RegistrationAttributes->Header,
        NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES,
        NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2,
        NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2
        );

    RegistrationAttributes->AttributeFlags = 
		NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK | 
		NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM |
		NDIS_MINIPORT_ATTRIBUTES_NO_PAUSE_ON_SUSPEND;
	
    RegistrationAttributes->InterfaceType = NdisInterfacePNPBus;
    RegistrationAttributes->MiniportAdapterContext = Adapter;

    RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_AllocateAdapter(), status 0x%x\n", ndisStatus));

    return ndisStatus;

error:
	RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], <== N6SdioWdi_AllocateAdapter(), Initialize failed, clean up resources. ndisStatus(%#x)\n", ndisStatus));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Allocate Adapter Failure",
		TraceLoggingLevel(TRACE_LEVEL_FATAL),
		TraceLoggingHexUInt32(ndisStatus, "Status"));
#endif

	if( Adapter != NULL )
		CLEANUP_RES_IN_MON(Adapter->ResMonObjWdi, Adapter);
	
	return ndisStatus;
}

VOID
N6SdioWdi_FreeAdapter(
	_In_  NDIS_HANDLE						MiniportAdapterContext
	)
{
	PADAPTER			pAdapter = (PADAPTER) MiniportAdapterContext;
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("==========> N6SdioWdi_FreeAdapter\n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Free Adapter",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(pAdapter, "pAdapter"));
#endif

	WDICommandHandleDeinit(pAdapter);

	N6SdioCleanUpInitializedResourcesEx(pAdapter, pAdapter->ResMonObjWdi);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<========== N6SdioWdi_FreeAdapter\n"));
}

NDIS_STATUS
N6SdioWdi_OpenAdapter(
	_In_  NDIS_HANDLE                         MiniportAdapterContext,
	_In_  PNDIS_MINIPORT_INIT_PARAMETERS      MiniportInitParameters
	)
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER) MiniportAdapterContext;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_OpenAdapter() ==>\n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Open Adapter",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(pAdapter, "pAdapter"));
#endif

	pWdi->pWdiInitParameters->OpenAdapterCompleteHandler(pAdapter->pNdisCommon->hNdisAdapter, 
		N6SdioWdi_TaskOpen(pAdapter, MiniportInitParameters));

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], <== N6SdioWdi_OpenAdapter() \n"));
    
	return ndisStatus;
}

NDIS_STATUS
N6SdioWdi_TaskOpen(
	_In_  PADAPTER			pAdapter,
	_In_  PNDIS_MINIPORT_INIT_PARAMETERS      MiniportInitParameters
	)
{
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS				RtStatus = RT_STATUS_SUCCESS;
	GUID                	PowerSetting_GUID = GUID_ACTIVE_POWERSCHEME;
	GUID                	WLANPowerMode_GUID = GUID_WLAN_POWER_MODE;
	PRT_SDIO_DEVICE			sdiodevice = NULL;
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PWDI_DATA_STRUCT		pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	NDIS_HANDLE				MiniportAdapterHandle = NULL;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TaskOpen() ==>\n"));

	sdiodevice = GET_RT_SDIO_DEVICE(pAdapter);
	MiniportAdapterHandle = sdiodevice->hNdisAdapter;

	//
	// Allocate Tx semaphore before create corresponding Tx thread.
	//
	PlatformInitializeSemaphore(&sdiodevice->TxSemaphore, 0);

#if RTL8723_SDIO_IO_THREAD_ENABLE 
	//
	// Allocate IO semaphore before create corresponding IO Handling thread.
	//
	PlatformInitializeSemaphore(&sdiodevice->IOSemaphore, 0);

#endif

	//
	// Set up synchronization mechanism for NdisTimer objects.
	//
	N6InitTimerSync(pAdapter);

	// Allocate spin lock.
	N6CInitializeSpinLocks(pAdapter);

	// Initialize Ndis events.
	n6sdioWdi_InitEvents(sdiodevice);

	//----------------------------------------------------------------------------
	// Initialize variables related to New IO method, 2005.01.06, by rcnjko.
	// 
	NdisAllocateSpinLock( &(sdiodevice->IrpSpinLock) );
	NdisOIDHistoryInit(pAdapter);	
	ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_SpinLock); // Add to resource monitor.

#if (RK_PLATFORM_SUPPORT == 1)
	PlatformInitializeMutex(&sdiodevice->RxHandleIntMutex);
#endif

	// SyncIo Method 2.  
	KeInitializeEvent( &(sdiodevice->SyncIoEvent), NotificationEvent, TRUE);

	if (WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
		pNdisCommon->MaxPktSize=NIC_MAX_PACKET_SIZE;

	sdiodevice->nIrpPendingCnt = 0;
	RTInitializeListHead( &(sdiodevice->AwbIdleQueue) );
	RTInitializeListHead( &(sdiodevice->AwbWaitQueue) );
	sdiodevice->NumAwb = MAX_AWB_DATA_SIZE;
	PrepareSdioAWBs(sdiodevice);

	ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_AWBs); // Add to resource monitor.

	N6WdmSdio_Enable(pAdapter);

	sdiodevice->AsynIoIrpPendingCount++;

	// Read the registry parameters
	ndisStatus= N6SdioReadRegParameters(sdiodevice);
	if (ndisStatus != NDIS_STATUS_SUCCESS) 
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6SdioReadRegParameters(): Read Registry Parameter Failed!\n"));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioReadRegParameters() successfully.\n"));
	}

	//
	// <Roger_Notes> We set default hardware type to unknown type to associate common NIC handler first and 
	// then re-assign correct HW type association handler when the type is recognized.
	// 2011.12.30.
	//
	ndisStatus = NicIFAssociateNIC(pAdapter, HARDWARE_TYPE_MAX);	

	//
	// Perform SDIO WDM initialization
	//
	if( !NT_SUCCESS(N6WdmSdio_Initialize(pAdapter) ))
	{
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6WdmSdio_Initialize() failed!\n") );
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6WdmSdio_Initialize() successfully.\n"));
	}

	if(pAdapter->HardwareType == HARDWARE_TYPE_MAX)
	{
		if(HAL_ReadTypeID(pAdapter) == RT_STATUS_FAILURE)
			goto error;
	}

	HAL_SetInterfaceIndex(pAdapter, 0);

	// We should set the correct hardware type to the following function
	ndisStatus = NicIFAssociateNIC(pAdapter, pAdapter->HardwareType);	
	if (ndisStatus != NDIS_STATUS_SUCCESS) 
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], NicIFAssociateNIC() Failed!\n"));
		goto error;
	} 
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], NicIFAssociateNIC() successfully.\n"));
		ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_AsocNIC); // Add to resource monitor.
	}

	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], NicIFInitResource().\n"));
	NicIFInitResource(pAdapter);

	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], N6UpdateDefaultSetting().\n"));
	N6UpdateDefaultSetting(pAdapter);

	N6SdioUpdateDefaultSetting(pAdapter);

	WDI_UpdateDefaultSetting(pAdapter);

	Dot11_UpdateDefaultSetting(pAdapter);

	//
	// Initialize SDIO Data transfer related Tx context buffer.
	//
	if( !NT_SUCCESS(N6SdioInitTxQueue(pAdapter) ) ) 
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6SdioInitTxQueue() Failed!\n"));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioInitTxQueue() successfully.\n"));
		ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_SdioTxRes); // Add to resource monitor.
	}

	//
	// Set SDIO Tx queue mapping
	//
	if(!pAdapter->HalFunc.HalSdioSetQueueMappingHandler(
			pAdapter, (u1Byte)sdiodevice->RtNumRxQueue, (u1Byte)sdiodevice->RtNumTxQueue))
	{
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6SdioWdi_AllocateAdapter(), Set SDIO Tx Queue mapping failed!\n") );
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_AllocateAdapter(), Set SDIO Tx Queue mapping successfully.\n"));
	}

	// Enable all Tx Queues.
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], N6WdmSdioTx_Enable().\n"));
	N6WdmSdioTx_Enable(pAdapter);

	ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_SdioInit); // Add to resource monitor.

	// Read adapter information such as MAC address from EEPROM
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], NicIFReadAdapterInfo().\n"));
	NicIFReadAdapterInfo(pAdapter);

	NDBG_Init(pAdapter);
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], HT_UpdateDefaultSetting().\n"));
	HT_UpdateDefaultSetting(pAdapter);
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], VHT_UpdateDefaultSetting().\n"));
	VHT_UpdateDefaultSetting(pAdapter);

	//Initialize  Ndis 6.2, Ndis 6.0 only return success.
	ndisStatus = NDIS_6_2_INITIALIZE_EXTENSION_COMPONENT(pAdapter,MiniportAdapterHandle,MiniportInitParameters);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N62CInitialize() failed\n"));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N62CInitialize() successfully.\n"));
		ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_N62CInit);
	}

	//
	// Update default setting to HalData.
	//
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_LED, &(sdiodevice->bRegLedCtrl));

	//
	// Support WoW for future use. Added by Roger, 2008.12.30.
	//
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_WOWLAN, &(sdiodevice->bSupportRemoteWakeUp));

	// Settings for HctTest.
	if(pAdapter->bInHctTest)
	{
		// For NDIS6 DOT11_STATISTICS test, we must open CRC, ICV error.
		// Revise this handler for DTM test, 2008.12.16, added by Roger.
		pAdapter->HalFunc.AllowErrorPacketHandler(pAdapter, TRUE, FALSE);
	}

	//
	// For usb selective suspend configuration. Added by Roger, 2010.04.16.
	//
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_USB_SELECTIVE_SUSPEND, &(sdiodevice->RegUsbSS));

	//
	// 2011/09/07 MH Add TX power for different channel plan.
	//
	RT_TRACE(COMP_INIT, DBG_TRACE, 
		("[WDI], power offset MKK = %08x/%08x/%08x ETSI = %08x/%08x/%08x\n", 
		sdiodevice->RegGainOffsetMKKLow, sdiodevice->RegGainOffsetMKK, sdiodevice->RegGainOffsetMKKHigh,
		sdiodevice->RegGainOffsetETSILow, sdiodevice->RegGainOffsetETSI, sdiodevice->RegGainOffsetETSIHigh));

	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_FCC_LOW, &(sdiodevice->RegGainOffsetFCCLow));
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_FCC, &(sdiodevice->RegGainOffsetFCC));
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_FCC_HIGH, &(sdiodevice->RegGainOffsetFCCHigh));

	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_MKK_LOW, &(sdiodevice->RegGainOffsetMKKLow));
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_MKK, &(sdiodevice->RegGainOffsetMKK));
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_MKK_HIGH, &(sdiodevice->RegGainOffsetMKKHigh));

	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_ETSI_LOW, &(sdiodevice->RegGainOffsetETSILow));
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_ETSI, &(sdiodevice->RegGainOffsetETSI));
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_GAIN_OFFSET_ETSI_HIGH, &(sdiodevice->RegGainOffsetETSIHigh));

	N6SdioAllocRes(pAdapter);
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], N6SdioAllocRes() successfully.\n"));

	ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_AllocSdioRes); // Add Usb related resource monitoring.

	sdiodevice->CurrentPowerState = NdisDeviceStateD0;

	ndisStatus=NicIFAllocateMemory(pAdapter);
	if(ndisStatus!=NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], NicIFAllocateMemory failed\n"));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], NicIFAllocateMemory()\n"));
		ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_AllocMem); // Add to resource monitor.
	}

	pAdapter->bInitComplete = TRUE;
	N6CInitThread((PVOID)pAdapter);

	ADAPTER_SET_STATUS_FLAG(pAdapter, ADAPTER_STATUS_FIRST_INIT);
	ndisStatus = NicIFInitializeAdapter(pAdapter);
	if( ndisStatus != NDIS_STATUS_SUCCESS )
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], NicIFInitializeAdapter() failed!!\n"));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], NicIFInitializeAdapter() successfully.\n"));
	}
	ADAPTER_CLEAR_STATUS_FLAG(pAdapter, ADAPTER_STATUS_FIRST_INIT);

	//
	// HCT12.0 2c_AddressChange.
	// Note that, Adapter->CurrentAddress and Adapter->PermanentAddress had been  
	// set up in NicIFInitializeAdapter().
	//
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], Set MAC Address\n"));
	if(pNdisCommon->bOverrideAddress)
	{
		NicIFSetMacAddress(pAdapter, pNdisCommon->CurrentAddress);
	}
	else
	{
		ETH_COPY_NETWORK_ADDRESS(pNdisCommon->CurrentAddress, pAdapter->PermanentAddress);
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], MAC Address: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
		pNdisCommon->CurrentAddress[0],
		pNdisCommon->CurrentAddress[1],
		pNdisCommon->CurrentAddress[2],
		pNdisCommon->CurrentAddress[3],
		pNdisCommon->CurrentAddress[4],
		pNdisCommon->CurrentAddress[5]));

	//
	// Before setting of attributes, update the native 802.11 related variable.
	// 2006.10.09, by shien chang.
	//
	ndisStatus = N6SdioAllocateNative80211MIBs(pAdapter);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6SdioAllocateNative80211MIBs() failed\n"));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioAllocateNative80211MIBs() successfully.\n"));
	}

	ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_AllocNWifi); // Add to resource monitor.

	N6InitializeNative80211MIBs(pAdapter);

	//Get Ndis Version
	pMgntInfo->NdisVersion = MgntTranslateNdisVersionToRtNdisVersion(pNdisCommon->NdisVersion);

	// At the moment, if HW is unpluged, fail the initialization immediate.
	if (pAdapter->bSurpriseRemoved)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], Adapter is unpluged, fail the init process.\n"));
		ndisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
		goto error;
	}

	//
	// Set Power config callback, 2009.03.18, by lanhsin.
	//
	ntStatus = PoRegisterPowerSettingCallback(
				NULL,
				(LPGUID)(&PowerSetting_GUID), 
				(PPOWER_SETTING_CALLBACK)&N6SdioPowerSettingCallback,
				(PVOID)pAdapter,
				&pNdisCommon->N6PowerSettingHandle
				);
	if (ntStatus != STATUS_SUCCESS)
	{
		ndisStatus = NDIS_STATUS_FAILURE;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], register N6SdioPowerSettingCallback() Failed, ntStatus = 0x%x\n", ntStatus));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], register N6SdioPowerSettingCallback() successfully.\n"));
	}

	ntStatus = PoRegisterPowerSettingCallback(
				NULL,
				(LPGUID)(&WLANPowerMode_GUID), 
				(PPOWER_SETTING_CALLBACK)&N6SdioWLANPowerModeCallback,
				(PVOID)pAdapter,
				&pNdisCommon->N6WLANPowerModeHandle
				);
	if (ntStatus != STATUS_SUCCESS)
	{
		ndisStatus = NDIS_STATUS_FAILURE;
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], register N6SdioWLANPowerModeCallback() Failed, ntStatus = 0x%x\n", ntStatus));
		goto error;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], register N6SdioWLANPowerModeCallback() successfully.\n"));
	}

	InitNdis6CommonResources(pAdapter);
	ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_CommonRS);

	// <Roger_Notes> We disable Rx aggregation for WiFi test. 2008.11.25.
	if(pMgntInfo->bWiFiConfg || pAdapter->bInHctTest)
		pAdapter->HalFunc.HalRxAggrHandler(pAdapter, FALSE);

	//This is sw initialize ready.
	pAdapter->bSWInitReady=TRUE;

	if(NDIS_STATUS_SUCCESS == ndisStatus) 
	{
		// After this adapter is initialzied ready, insert this adapter to the list.
		INSERT_GLOBAL_ADAPTER_LIST(pAdapter);

		//
		// Register the Device object for this adapter!
		// Note:
		//	No matter the returned status of registration is success or not, the result has no
		//	effect on the status for Miniportinitialize.
		// 	So, the resiteration process should be placed in the success location of Initialization.
		// By Bruce, 2011-07-07.
		//	
		if(RT_STATUS_SUCCESS != (RtStatus = N6C_RegisterIoDevice(pAdapter)))
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6C_RegisterIoDevice() failed for status = %d\n", RtStatus));
		}
		ADD_RES_TO_MON(pAdapter->ResMonObjWdi, InitRM_BTIODevice);
	}

	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSED);

	WDICommandHandleInit(pAdapter);

	pAdapter->initfinish = TRUE;
	pAdapter->bInitComplete = TRUE;	
	DrvIFIndicateCurrentPhyStatus(pAdapter);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_AllocateAdapter(), status 0x%x\n", ndisStatus));

	return ndisStatus;

error:
	RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], <== N6SdioWdi_AllocateAdapter(), Initialize failed, clean up resources. ndisStatus(%#x)\n", ndisStatus));

	if( pAdapter != NULL )
		CLEANUP_RES_IN_MON(pAdapter->ResMonObjWdi, pAdapter);

	return ndisStatus;
}

NDIS_STATUS
N6SdioWdi_CloseAdapter(
	_In_  NDIS_HANDLE             MiniportAdapterContext
	)
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER) MiniportAdapterContext;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_CloseAdapter() ==>\n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Close Adapter",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(pAdapter, "pAdapter"));
#endif

	pWdi->pWdiInitParameters->CloseAdapterCompleteHandler(pAdapter->pNdisCommon->hNdisAdapter, N6SdioWdi_TaskClose(pAdapter));

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_CloseAdapter() <==\n"));

	return ndisStatus;
}

NDIS_STATUS
N6SdioWdi_TaskClose(
	_In_  PADAPTER			pAdapter
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS				RtStatus = RT_STATUS_SUCCESS;
	PWDI_DATA_STRUCT		pWdi = &(pAdapter->pPortCommonInfo->WdiData);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TaskClose() ==> \n"));

	do
	{
		u1Byte				RFInProgressTimeOut = 0;

		PlatformAcquireSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);
		while( pAdapter->MgntInfo.RFChangeInProgress)
		{
			PlatformReleaseSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);

			RT_TRACE(COMP_POWER,DBG_LOUD,
				("[WDI], RF is in progress, need to wait until rf chang is done.\n"));
			delay_ms(1);
			RFInProgressTimeOut ++;
			PlatformAcquireSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);
		}
		pAdapter->MgntInfo.RFChangeInProgress = TRUE;
		PlatformReleaseSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);

		pAdapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;

		NicIFHaltAdapter(pAdapter, FALSE);

		PlatformAcquireSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);	
		pAdapter->MgntInfo.RFChangeInProgress = FALSE;
		PlatformReleaseSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);

		RT_SET_DRV_STATE(pAdapter, DrvStateHwHalted);

		pAdapter->HalFunc.WaitForH2CQueueEmptyHandler(pAdapter);

		pAdapter->bDriverStopped = TRUE;

		//
		// Mark the miniport driver as halted state.
		//
		//RT_ASSERT( (N6C_GET_MP_DRIVER_STATE(pAdapter) == MINIPORT_PAUSED), 
		//	("MiniportHalt(): DriverState(%d) != PAUSED !!!\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));
		N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_HALTED);

		//double check to avoid race condition that causes double complete here
		if( pWdi->TaskHandle.Status & RT_OID_HANDLER_STATUS_SET )
		{
			OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
		}

		if( pWdi->PropertyHandle.Status & RT_OID_HANDLER_STATUS_SET )
		{
			OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_PROPERTY);
		}
	}while(FALSE);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TaskClose() <== \n"));

	return ndisStatus;
}

NDIS_STATUS
N6SdioWdi_StartOperation(
	_In_  NDIS_HANDLE             MiniportAdapterContext
	)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER) MiniportAdapterContext;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	PPORT_COMMON_INFO	pPortCommonInfo = pAdapter->pPortCommonInfo;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_StartOperation() ==> \n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Start Operation",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(pAdapter, "pAdapter"));
#endif

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], NicIFClearInterrupt()\n"));
	NicIFClearInterrupt(pAdapter);
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], NicIFEnableInterrupt()\n"));
	NicIFEnableInterrupt(pAdapter);

	PlatformInitializeSemaphore(&(pNdisCommon->RxNotifySemaphore), 0);

	PlatformInitializeThread(
		pAdapter,
		&(pAdapter->RxNotifyThread),
		(RT_THREAD_CALL_BACK)RxNotifyThreadCallback,
		"RxIndicationThread",
		TRUE,
		0,
		NULL);		

	PlatformRunThread(
		pAdapter, 
		&(pAdapter->RxNotifyThread), 
		PASSIVE_LEVEL,
		NULL);

	MgntActSet_802_11_PowerSaveMode(pAdapter, pAdapter->MgntInfo.dot11PowerSaveMode);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_StartOperation() <== \n"));

	return ndisStatus;
}

void
N6SdioWdi_StopOperation(
	_In_  NDIS_HANDLE             MiniportAdapterContext
	)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER) MiniportAdapterContext;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_StopOperation ==>\n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Stop Operation",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(pAdapter, "pAdapter"));
#endif

	//1 leave all power save mode before bDriverIsGoingToUnload, otherwise driver will disable interrupt.
	LeaveAllPowerSaveMode(pAdapter);

	pAdapter->bDriverIsGoingToUnload = TRUE;

	RT_SET_DRV_STATE(pAdapter, DrvStateHwHalting);

	MgntResetJoinProcess(pAdapter);
	N6C_DeregisterIoDevice(pAdapter);
	REMOVE_GLOBAL_ADAPTER_LIST(pAdapter);

	//1 Stop Watchdog timer
	//Add bDriverIsGoingToUnload checking at watchdog timer to stop dynamic architecture and will be released later.

	//1 Disable BulkIn/BulkOut to avoid incoming data during mac/port deletion.	
	//1 MUST be the bottom of StopOperation
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], NicIFDisableInterrupt()!!\n"));
	NicIFDisableInterrupt(pAdapter);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_StopOperation  <==\n"));    
}

NDIS_STATUS
N6SdioWdi_HangDiagnose(
    _In_  NDIS_HANDLE         MiniportDriverContext,
    _In_  eDiagnoseLevel      DiagnoseLevel,
    _In_  UINT32              BufferSize,
    _Out_writes_bytes_to_( BufferSize, *pOutputSize )
          UINT8 *             FirmwareBlob,
    _Out_ UINT32*             pOutputSize
    )
{
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER) MiniportDriverContext;
	UINT32				BufferSizeDumped = 0;
	UINT8*				pBufferDumped = NULL;

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample HangDiagnose",
		TraceLoggingLevel(TRACE_LEVEL_ERROR),
		TraceLoggingUInt32(DiagnoseLevel, "DiagnoseLevel"),
		TraceLoggingUInt32(BufferSize, "BufferSize"));
#endif

	if(MiniportDriverContext == NULL ||
		FirmwareBlob == NULL ||
		pOutputSize == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI] N6SdioWdi_HangDiagnose() Invalid parameter!\n"));
		return NDIS_STATUS_INVALID_PARAMETER;
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI] N6SdioWdi_HangDiagnose() Miniport:%p level:%d BufferSize:%d\n", 
		MiniportDriverContext,
		DiagnoseLevel,
		BufferSize));

	RT_TRACE(COMP_INIT, DBG_LOUD, ("HangDiagnose Level:%d BufferSize:%d\n", DiagnoseLevel, BufferSize));

	switch (DiagnoseLevel)
	{
		case DiagnoseLevelHardwareRegisters:
			//dump register here
			break;
		case DiagnoseLevelFirmwareImageDump:
			//dump fw image
			break;
		case DiagnoseLevelDriverStateDump:
			//dump driver state
			break;
		default:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI] N6SdioWdi_HangDiagnose() DiagnoseLevel %d invalid.\n", DiagnoseLevel));
			break;
	}

	*pOutputSize = BufferSizeDumped;
	FirmwareBlob = pBufferDumped;

	//do the WDICommandHangCleanup in surprise remove to complete the hanging Command
	pAdapter->pPortCommonInfo->WdiData.bCommandHangHappened = TRUE;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI] N6SdioWdi_HangDiagnose() OutputSize:%d Status:0x%x\n", 
		*pOutputSize, 
		Status));

	return Status;
}

NDIS_STATUS
N6SdioWdi_TalTxRxInitialize(
	_In_ NDIS_HANDLE		MiniportAdapterContext,
	_In_ NDIS_HANDLE		NdisMiniportDataPathHandle,
	_In_ PNDIS_WDI_DATA_API	NdisWdiDataPathApi,
	_Out_ PTAL_TXRX_HANDLE	pMiniportTalTxRxContext,
	_Inout_ PNDIS_MINIPORT_WDI_DATA_HANDLERS pMiniportDataHandlers,
	_Out_ UINT32 *			pMiniportWdiFrameMetaDataExtraSpace ) // request exta bytes at the tail
{
	NDIS_STATUS			Status = NDIS_STATUS_FAILURE;
	PADAPTER			Adapter = (PADAPTER) MiniportAdapterContext;
	PWDI_DATA_STRUCT	pWdi = NULL;
	NDIS_MINIPORT_WDI_DATA_HANDLERS     WdiDataHandlers = {0};

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TalTxRxInitialize() ==> \n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample TalTxRxInitialize",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(Adapter, "Adapter"));
#endif

	do
	{
		RtlZeroMemory(&GLWdiTxRxStatistics, sizeof(WDI_TXRX_STATISTICS));		

		// Store Handle for Wdi Data Path
		pWdi = &(Adapter->pPortCommonInfo->WdiData);
		pWdi->DataPathHandle = NdisMiniportDataPathHandle;

		*pMiniportWdiFrameMetaDataExtraSpace = 0;
		*pMiniportTalTxRxContext = NULL;

		//
		// Init Rx NetBufferListPool,
		//
		Status = N6SdioAllocateRxNetBufferListPool(Adapter);
		if(Status != NDIS_STATUS_SUCCESS) 
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6SdioAllocateRxNetBufferListPool() failed Status: %#X!\n", Status));
			break;
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioAllocateRxNetBufferListPool() successfully.\n"));
			ADD_RES_TO_MON(Adapter->ResMonObjWdi, InitRM_AllocRxNBLPool); // Add to resource monitor.
		}

		// Link Wdi Data Api
		RtlCopyMemory(&pWdi->WdiDataApi, NdisWdiDataPathApi,sizeof(NDIS_WDI_DATA_API));

		*pMiniportTalTxRxContext = Adapter;

		// Initialize Wdi Data Path Handlers
		RtlZeroMemory(&WdiDataHandlers, sizeof(NDIS_MINIPORT_WDI_DATA_HANDLERS));

		N6_ASSIGN_OBJECT_HEADER(
			WdiDataHandlers.Header,
			NDIS_OBJECT_TYPE_MINIPORT_WDI_DATA_HANDLERS,
			NDIS_OBJECT_TYPE_MINIPORT_WDI_DATA_HANDLERS_REVISION_1,
			NDIS_SIZEOF_MINIPORT_WDI_DATA_HANDLERS_REVISION_1);

		WdiDataHandlers.TxAbortHandler = N6SdioWdi_TxAbort;
		WdiDataHandlers.TxTargetDescInitHandler = N6SdioWdi_TxTargetDescInit;
		WdiDataHandlers.TxTargetDescDeInitHandler = N6SdioWdi_TxTargetDescDeInit;
		WdiDataHandlers.TxDataSendHandler = N6SdioWdi_TxDataSend;
		WdiDataHandlers.TxTalSendHandler = N6SdioWdi_TxTalSend;
		WdiDataHandlers.TxTalSendCompleteHandler = N6SdioWdi_TxTalSendComplete;
		WdiDataHandlers.TxTalQueueInOrderHandler = N6SdioWdi_TxTalQueueInOrder;
		WdiDataHandlers.TxPeerBacklogHandler = N6SdioWdi_TxPeerBacklog;

		WdiDataHandlers.RxStopHandler = N6SdioWdi_RxStop;
		WdiDataHandlers.RxFlushHandler = N6SdioWdi_RxFlush;
		WdiDataHandlers.RxRestartHandler = N6SdioWdi_RxRestart;
		WdiDataHandlers.RxGetMpdusHandler = N6SdioWdi_RxGetMpdus;
		WdiDataHandlers.RxReturnFramesHandler = N6SdioWdi_RxReturnFrames;
		WdiDataHandlers.RxResumeHandler = N6SdioWdi_RxResume;
		WdiDataHandlers.RxThrottleHandler = N6SdioWdi_RxThrottle;
		WdiDataHandlers.RxPpduRssiHandler = N6SdioWdi_RxPpduRssi;

		WdiDataHandlers.TalTxRxStartHandler = N6SdioWdi_TalTxRxStart;
		WdiDataHandlers.TalTxRxStopHandler = N6SdioWdi_TalTxRxStop;
		WdiDataHandlers.TalTxRxAddPortHandler = N6SdioWdi_TalTxRxAddPort;
		WdiDataHandlers.TalTxRxDeletePortHandler = N6SdioWdi_TalTxRxDeletePort;
		WdiDataHandlers.TalTxRxSetPortOpModeHandler = N6SdioWdi_TalTxRxSetPortOpMode;
		WdiDataHandlers.TalTxRxResetPortHandler = N6SdioWdi_TalTxRxResetPort;
		WdiDataHandlers.TalTxRxPeerConfigHandler = N6SdioWdi_TalTxRxPeerConfig;
		WdiDataHandlers.TalTxRxPeerDeleteConfirmHandler = N6SdioWdi_TalTxRxPeerDeleteConfirm;

		RtlCopyMemory(pMiniportDataHandlers,&WdiDataHandlers,sizeof(NDIS_MINIPORT_WDI_DATA_HANDLERS));

		if(Adapter->bUseThreadHandleInterrupt)
		{
			PlatformSetEventTrigerThread(
				Adapter,
				&Adapter->InterruptThread,
				PASSIVE_LEVEL,
				NULL);	
		}

		N6C_SET_MP_DRIVER_STATE(Adapter, MINIPORT_RUNNING);
	}while(FALSE);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TalTxRxInitialize() <== \n"));

	return Status;
}


VOID
N6SdioWdi_TalTxRxDeinitialize(
    _In_ TAL_TXRX_HANDLE MiniportTalTxRxContext
    )
{
	NDIS_STATUS     ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
	PADAPTER        Adapter = (PADAPTER) MiniportTalTxRxContext;
	PRT_SDIO_DEVICE pDevice = &(Adapter->NdisSdioDev);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TalTxRxDeinitialize ==>\n"));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample TalTxRxDeinitialize",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingPointer(Adapter, "Adapter"));
#endif

	RT_DISABLE_SDIO_TX_TRANSFERS(Adapter);	// Disable Tx
	RT_DISABLE_SDIO_RX_TRANSFERS(Adapter);	// Disable Rx

	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], Before ReleaseDataFrameQueued()\n"));
	N6SdioReleaseTxQueuePending(Adapter);
	ReleaseDataFrameQueued(Adapter);
	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], After ReleaseDataFrameQueued()\n"));
	WDI_DeInitRxQueue(Adapter);	

	if (pDevice->RxNetBufferListPool)
	{
		N6SdioFreeRxNetBufferListPool(Adapter);
	}

	NicIFClearInterrupt(Adapter);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TalTxRxDeinitialize <==\n"));
}


//
//   Description:
//		This function is called when WDI send NBLs
//
//	Output:
//		Return corresponding RT_STATUS for processing NBLs.
//
//	Assumption:
//		1. RT_TX_SPINLOCK is acquired.
//
RT_STATUS
N6SdioWdi_DataSend(
	_In_  PADAPTER			pAdapter,
	_In_  PNET_BUFFER_LIST	pNBLs
	)
{
	RT_STATUS			RtStatus = RT_STATUS_SUCCESS;
	PRT_SDIO_DEVICE		pDevice = &(pAdapter->NdisSdioDev);
	PRT_TCB				pTcb = NULL;
	PNET_BUFFER_LIST	pCurrNetBufferList = NULL, pNextNetBufferList = NULL;
	PNET_BUFFER			pNetBuffer = NULL, pNextNetBuffer = NULL;

	PDOT11_EXTSTA_SEND_CONTEXT		pDot11SendContext = NULL;
	u2Byte							ExemptionActionType = 0; // 0:Auto 1:encrypt 2:no encrypt 3: key UNAVAILABLE
	NDIS_NET_BUFFER_LIST_8021Q_INFO NBL8021qInfo;
	u1Byte							Priority = 0;
	u4Byte							numNBLCnt=0;

	for (pCurrNetBufferList = pNBLs;
		pCurrNetBufferList != NULL;
		pCurrNetBufferList = pNextNetBufferList)
	{
		numNBLCnt++;
		GLWdiTxRxStatistics.numWdiTxNBLs++;
		pDevice->SendingNetBufferList = pCurrNetBufferList;

		pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);

		//
		// Parse per-NBL information.
		//
		pDot11SendContext = (PDOT11_EXTSTA_SEND_CONTEXT) NET_BUFFER_LIST_INFO(pCurrNetBufferList, MediaSpecificInformation);	 
		switch( pDot11SendContext->usExemptionActionType )
		{
		case DOT11_EXEMPT_NO_EXEMPTION :
			ExemptionActionType = 1;
			break;
		case DOT11_EXEMPT_ALWAYS :
			ExemptionActionType = 2;
			break;
		case DOT11_EXEMPT_ON_KEY_MAPPING_KEY_UNAVAILABLE :
			ExemptionActionType = 3;
			break;
		default :
			ExemptionActionType = 0;
			break;
		}

		//
		// Parse WMM info
		//
		//if(pStaQos->CurrentQosMode != QOS_DISABLE) // this only for OP mode in EXTSTA. //chedu default port one//?? need modify Neo
		{
			NBL8021qInfo.Value = NET_BUFFER_LIST_INFO(pCurrNetBufferList, Ieee8021QNetBufferListInfo);
			Priority = (u1Byte)NBL8021qInfo.WLanTagHeader.WMMInfo;		
			if (Priority > 7)
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("[WDI], N6PciWdiTargetDescInit(): WMMInfo should less or equal than 7 (%d)\n", Priority));
				Priority = 0;
			}
		}

		for(pNetBuffer = NET_BUFFER_LIST_FIRST_NB(pCurrNetBufferList);
			pNetBuffer != NULL;
			pNetBuffer = pNextNetBuffer)
		{
			//
			// Out of Tx resource.
			//
			if( RTIsListEmpty(GET_TCB_IDLE_QUEUE(pAdapter)) )
			{
				RT_ASSERT(FALSE, ("[WDI], N6SdioWdi_DataSend(): GET_TCB_IDLE_QUEUE is empty! SHOULD NOT REACH HERE!!!\n"));
				pAdapter->TxStats.NumOutOfTCB++;
				RtStatus = RT_STATUS_RESOURCE;
				break;
			}

			pNextNetBuffer = NET_BUFFER_NEXT_NB(pNetBuffer);

			//
			// We have enough Tx resource to send this NB.
			//
			pAdapter->TxStats.numTcbOk++;
			pTcb = (PRT_TCB)RTRemoveHeadListWithCnt(GET_TCB_IDLE_QUEUE(pAdapter),Get_NUM_IDLE_TCB(pAdapter));
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_DataSend(), get pTcb=0x%x, Adapter->NumIdleTcb = %d\n", (u4Byte)pTcb, *Get_NUM_IDLE_TCB(pAdapter)));
			GLWdiTxRxStatistics.numWdiTxTcbs++;

			pTcb->sysTime[0] = PlatformGetCurrentTime();
			pTcb->Reserved = pCurrNetBufferList; // <Roger_TODO> Should we need to remove this variable later?

			//
			// Add per-NBL info into TCB.
			//			
			pTcb->priority = Priority;	// Qos information. 2007.01.05, by shien chang.
			pTcb->EncInfo.ExemptionActionType = ExemptionActionType; // by CCW. 
			//pTcb->TcpOffloadMode = TcpCheckSumValue;
			pTcb->pAdapter = pAdapter;

			//RT_TRACE(COMP_SEND, DBG_LOUD, ("[WDI], N6SdioWdi_DataSend(), pTcb 0x%x to go!!!\n", pTcb));

			// this is a NB associated with a TCB
			pAdapter->MgntInfo.OutstandingNdisPackets++;

			if(N6SdioGetPacketBuffers(pAdapter, pNetBuffer, pTcb) &&
				!pAdapter->bInHctTest)
			{ 
				//
				// Set up TCB.
				//
				pTcb->ProtocolType = RT_PROTOCOL_802_11;
				pTcb->BufferType = RT_TCB_BUFFER_TYPE_SYSTEM;
				if ( IsDataFrame(pTcb->BufferList[0].VirtualAddress) )
				{
					pTcb->SpecifiedQueueID = UNSPECIFIED_QUEUE_ID;	//LOW_QUEUE;	// Rewrited by Annie, 2005-12-05.
				}
				else
				{
					pTcb->SpecifiedQueueID = NORMAL_QUEUE;
				}
				pTcb->DataRate = UNSPECIFIED_DATA_RATE;
				pTcb->bFromUpperLayer = TRUE;
				pTcb->FragCount = 1;
				pTcb->TSID = DEFAULT_TSID;

				//
				// Send TCB down.
				//
				if(ACTING_AS_AP(pAdapter) == FALSE)
				{
					BOOLEAN bCompleteAfterSend;

					bCompleteAfterSend = NicIFSendPacket(pAdapter, pTcb);

					if (bCompleteAfterSend)
					{
						//
						// Complete the NB ASAP to enhance throughput for some application, such as iPerf.
						// Note that, we can complete the NB here because it 
						// had been coalesced in PreTransmitTCB().
						//

						// In WDI, if we need to set pTcb->Reserved to NULL, we have to handle the completion
						// of the NBL when reference cnt = 0. later
						// Currently we don't set the pTcb->Reserved to NULL as design such that the completion 
						// can be handled in ReturnTcb().

						//pTcb->Reserved = NULL;
					}
				}
				else
				{
					//
					// 07525, rcnjko: 
					// AP_PS_SendPacket() will queue the packet to send in proper queue if necessary.
					// So, we must gurantee the buffers referenced in the TCB is still valid, 
					// that is, not yet compelted to upper layer.
					//
					NicIFSendPacket(pAdapter, pTcb);
				}

				GLWdiTxRxStatistics.numWdiTxNicSend++;
			}
			else
			{
				//
				// Case 2.2 Too many buffers in the NB, send it in coalesced way.
				//
				N6SdioSendCoalescedNetBuffer( pAdapter, pNetBuffer, pTcb);
				GLWdiTxRxStatistics.numWdiTxCoalesce++;
			}
		}
	}

	if(GLWdiTxRxStatistics.maxDeqNBL < numNBLCnt)
		GLWdiTxRxStatistics.maxDeqNBL = numNBLCnt;

	//
	// We are now ready to process another NBL.
	//
	pDevice->SendingNetBufferList = NULL;

	return RtStatus;
}

void N6SdioWdi_TxAbort(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_PEER_ID		PeerId,
	_Out_ PNDIS_STATUS		pWifiStatus)
{
	PADAPTER		pAdapter = (PADAPTER) MiniportTalTxRxContext;
	NDIS_STATUS 	ndisStatus = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_INIT);

	ReleaseDataFrameQueued(pAdapter);

	//complete inline
	*pWifiStatus = NDIS_STATUS_SUCCESS;

	FunctionOut(COMP_INIT);
}

//
//   Assumption: WDI interface independent
//
void N6SdioWdi_TxTargetDescInit(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ PNET_BUFFER_LIST	pNBL,
	_Out_ PNDIS_STATUS		pWifiStatus)
{
	PADAPTER				pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PWDI_FRAME_METADATA     pFrameMeta = NULL;
	PNET_BUFFER				pNetBuffer = NULL;
	u2Byte					NetBufferCount = 0;

	GLWdiTxRxStatistics.numWdiTxDescInit++;

	FunctionIn(COMP_SEND);

	//Fill in cost
	pFrameMeta = (PWDI_FRAME_METADATA)pNBL->MiniportReserved[0];

	NetBufferCount = 0;
	for(pNetBuffer = NET_BUFFER_LIST_FIRST_NB(pNBL);
		pNetBuffer != NULL;
		pNetBuffer = NET_BUFFER_NEXT_NB(pNetBuffer))
	{
		NetBufferCount ++; 
	}

	*pWifiStatus = NDIS_STATUS_SUCCESS;

	RT_WDI_NBL_SET_REF_CNT(pNBL, (NetBufferCount));
	pFrameMeta->u.txMetaData.TxCost = NetBufferCount;

	if(NetBufferCount > GLWdiTxRxStatistics.maxTxNBInNBL)
		GLWdiTxRxStatistics.maxTxNBInNBL = NetBufferCount;

	FunctionOut(COMP_SEND);
}


void N6SdioWdi_TxTargetDescDeInit(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ PNET_BUFFER_LIST	pNBL)
{
	FunctionIn(COMP_SEND);

	GLWdiTxRxStatistics.numWdiTxDescDeInit++;

	FunctionOut(COMP_SEND);
}


void N6SdioWdi_TxDataSend(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_PEER_ID		PeerId,
	_In_ WDI_EXTENDED_TID	ExTid,
	_In_ UINT16				NumQueueFrames,
#ifndef WDI_83_COMPLIANT
	_In_ UINT32				NumActiveFrames,
#endif // WDI_83_COMPLIANT
	_In_ BOOLEAN			bRobustnessFlag)
{
	PADAPTER			pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PNET_BUFFER_LIST	pDequeuedNbls = NULL;
	PADAPTER			pTargetAdapter = NULL;
	u2Byte				txCredit=0;

	FunctionIn(COMP_SEND);

	UNREFERENCED_PARAMETER(PeerId);
	UNREFERENCED_PARAMETER(ExTid);
	UNREFERENCED_PARAMETER(NumQueueFrames);
	UNREFERENCED_PARAMETER(bRobustnessFlag);

	do
	{
		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)PortId);

		PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
		txCredit = *Get_NUM_IDLE_TCB(pAdapter);
		if(txCredit)
		{
			// Dequeue NBLs from WDI.
			pWdi->WdiDataApi.TxDequeueIndication(pWdi->DataPathHandle,
													WDI_QUANTUM_INVALID,	// quantum: don't care total length of packets
													WDI_TX_MAX_FRAME_COUNT_INVALID,	// max frame count: dequeue as much as we can.
													txCredit,				// credit: number of idle Tcb
													&pDequeuedNbls);
		}
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

		// can not dequeue any frames, indicate PauseIndication to upper layer.
		// here we assume that only credit not enough that we can not dequeue any frames.
		// so when sendPauseIndication, we set the Tx pause reason by credit.
		if(!pDequeuedNbls)
		{
			GLWdiTxRxStatistics.numWdiTxPauseIndicate++;			
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TxDataSend(), txCredit=%d\n", txCredit));
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TxDataSend(), PortId=0x%x, PeerId=0x%x\n", PortId, PeerId));
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TxDataSend(), pTargetAdapter=0x%p, pWdi=0x%p\n", pTargetAdapter, pWdi));
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TxDataSend(): TxSendPauseIndication, count=%d!!!\n",
				GLWdiTxRxStatistics.numWdiTxPauseIndicate));

			pWdi->WdiDataApi.TxSendPauseIndication(pWdi->DataPathHandle, 
													WDI_PORT_ANY, 
													WDI_PEER_ANY,
													WDI_EXT_TID_UNKNOWN,
													WDI_TX_PAUSE_REASON_CREDIT);
			//PlatformAtomicExchange(&pWdi->txPauseReason, pWdi->txPauseReason|WDI_TX_PAUSE_REASON_CREDIT);
			// here we use tx spinlock to protect txPauseReason.
			PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
			pWdi->txPauseReason |= WDI_TX_PAUSE_REASON_CREDIT;
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], N6SdioWdi_TxDataSend(): pWdi->txPauseReason=0x%x!!!\n", pWdi->txPauseReason));
			PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
			break;
		}

		// can not process Tx, complete to WDI immediately.
		if( N6SDIO_CANNOT_TX(pTargetAdapter) || (!N6_INIT_READY(pTargetAdapter)))
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("[WDI], N6SdioWdi_TxDataSend(): N6SDIO_CANNOT_TX, drop the packets.\n"));
			GLWdiTxRxStatistics.numWdiTxDataSendDrop++;
			pWdi->WdiDataApi.TxTransferCompleteIndication(pWdi->DataPathHandle,WDI_TxFrameStatus_TransferFailed,pDequeuedNbls);
			break;
		}
		GLWdiTxRxStatistics.numWdiTxDataSend++;

		// have enough credit(Tcb), can process NBLs.
		PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);	
		N6SdioWdi_DataSend(pTargetAdapter, pDequeuedNbls);
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
	}	while(FALSE);

	FunctionOut(COMP_SEND);
}

//
//  Interface to send frame injected(TxInjectFrameIndication) by LE through data path.
//  8723BS do not inject mgnt frame because there is specific handling in lower level.
//
void N6SdioWdi_TxTalSend(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_PEER_ID		PeerId,
	_In_ WDI_EXTENDED_TID	ExTid,
	_In_ UINT16				NumQueueFrames,
#ifndef WDI_83_COMPLIANT
	_In_ UINT32				NumActiveFrames,
#endif // WDI_83_COMPLIANT
	_In_ BOOLEAN			bRobustnessFlag)
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
	PADAPTER			Adapter = (PADAPTER) MiniportTalTxRxContext;
	PWDI_DATA_STRUCT	pWdi = &(Adapter->pPortCommonInfo->WdiData);
	PNET_BUFFER_LIST	pDequeuedNbls = NULL;

	FunctionIn(COMP_SEND);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);
	UNREFERENCED_PARAMETER(PeerId);
	UNREFERENCED_PARAMETER(ExTid);
	UNREFERENCED_PARAMETER(NumQueueFrames);
	UNREFERENCED_PARAMETER(bRobustnessFlag);

	pWdi->WdiDataApi.TxDequeueIndication(pWdi->DataPathHandle, 20000, 1, 8, &pDequeuedNbls);

	pWdi->WdiDataApi.TxTransferCompleteIndication(pWdi->DataPathHandle,WDI_TxFrameStatus_Ok,pDequeuedNbls);

	FunctionOut(COMP_SEND);
}


void N6SdioWdi_TxTalSendComplete(
	_In_ TAL_TXRX_HANDLE		MiniportTalTxRxContext,
	_In_ PNET_BUFFER_LIST		pNBL,
	_In_ WDI_TX_FRAME_STATUS	TxFrameStatus)
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
	PNET_BUFFER_LIST	pCurNbl = NULL;
	PNET_BUFFER_LIST	pNextNbl = NULL;

	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);

	RT_TRACE(COMP_SEND, DBG_LOUD, ("[WDI], Frame status = %x", TxFrameStatus));

	pCurNbl = pNBL;

	while (pCurNbl != NULL)
	{
		pNextNbl = NET_BUFFER_LIST_NEXT_NBL(pCurNbl);
		NET_BUFFER_LIST_NEXT_NBL(pCurNbl) = NULL;

		NdisFreeNetBufferList(pCurNbl);

		pCurNbl = pNextNbl;
	}

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TxTalQueueInOrder(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PEER_ID		PeerId,
	_In_ UINT32				ExTidBitmask)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_NOT_SUPPORTED;

	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PeerId);
	UNREFERENCED_PARAMETER(ExTidBitmask);

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TxPeerBacklog(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_PEER_ID		PeerId,
	_In_ BOOLEAN			bBacklogged)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_NOT_SUPPORTED;

	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);
	UNREFERENCED_PARAMETER(PeerId);
	UNREFERENCED_PARAMETER(bBacklogged);

	FunctionOut(COMP_INIT);
}

void N6SdioWdi_RxStop(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_Out_ PNDIS_STATUS		pWifiStatus)
{
	PADAPTER			pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PADAPTER			pAdapterByPortNum = NULL;
	PADAPTER			pTempAdapter = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = NULL;
	BOOLEAN				bNotifying = FALSE;

	FunctionIn(COMP_INIT);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], stop rx at Port_ID %d\n", PortId));

	PlatformAcquireSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	if( PortId != WDI_PORT_ANY )
	{
		pAdapterByPortNum = GetAdapterByPortNum(pAdapter, (u1Byte)PortId);
		if( pAdapterByPortNum == NULL )
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("[WDI], Cannot find Port Id(%d)\n", PortId));
			*pWifiStatus = NDIS_STATUS_FAILURE;
		}
		else
		{
			pNdisCommon = pAdapterByPortNum->pNdisCommon;
			if( pNdisCommon->bRxControlState == RT_RX_NOTIFYING )
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Driver change Rx control state to Prepare Stop mode for port %d and will be stopped later\n", PortId));
				pNdisCommon->bRxControlState = RT_RX_PREPARE_STOP;
				*pWifiStatus = NDIS_STATUS_PENDING;
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Driver change Rx control state to stop mode for port %d\n", PortId));
				pNdisCommon->bRxControlState = RT_RX_STOP;
				GLWdiTxRxStatistics.numWdiRxStop++;
				*pWifiStatus = NDIS_STATUS_SUCCESS;
			}
		}
	}
	else
	{
		pTempAdapter = GetDefaultAdapter(pAdapter);

		do
		{
			pNdisCommon = pTempAdapter->pNdisCommon;
			if( pNdisCommon->bRxControlState == RT_RX_NOTIFYING )
			{
				bNotifying = TRUE;
				break;
			}
			pTempAdapter = GetNextExtAdapter(pTempAdapter);
		}while( pTempAdapter != NULL );

		pTempAdapter = GetDefaultAdapter(pAdapter);
		do
		{
			pNdisCommon = pTempAdapter->pNdisCommon;
			if( bNotifying == TRUE )
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Driver change Rx control state to Prepare Stop mode for all ports and will be stopped later\n"));
				pNdisCommon->bRxControlState = RT_RX_PREPARE_STOP;
				*pWifiStatus = NDIS_STATUS_PENDING;
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("Driver change Rx control state to stop mode for all ports\n"));
				pNdisCommon->bRxControlState = RT_RX_STOP;
				GLWdiTxRxStatistics.numWdiRxStop++;
				*pWifiStatus = NDIS_STATUS_SUCCESS;
			}
			pTempAdapter = GetNextExtAdapter(pTempAdapter);
		}while( pTempAdapter != NULL );
	}
	PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_RxFlush(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId)
{
	PADAPTER			pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PNET_BUFFER_LIST	pNetBufferList = NULL;

	FunctionIn(COMP_INIT);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], flush rx at Port_ID %d\n", PortId));
	WDI_FetchNBLByPort(pAdapter, (u2Byte)PortId, &pNetBufferList);

	if( pNetBufferList != NULL )
		WDI_FreeRxFrame(pAdapter, pNetBufferList);

	pWdi->WdiDataApi.RxFlushConfirm(pWdi->DataPathHandle);

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_RxRestart(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId)
{
	PADAPTER				pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PADAPTER				pAdapterByPortNum = NULL;
	PADAPTER				pTempAdapter = NULL;
	PRT_NDIS6_COMMON		pNdisCommon = NULL;

	FunctionIn(COMP_INIT);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], restart rx at Port_ID %d\n", PortId));

	PlatformAcquireSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	if( PortId != WDI_PORT_ANY )
	{
		pAdapterByPortNum = GetAdapterByPortNum(pAdapter, (u1Byte)PortId);
		if( pAdapterByPortNum == NULL )
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("[WDI], Cannot find Port Id(%d)\n", PortId));
		}
		else
		{
			pNdisCommon = pAdapterByPortNum->pNdisCommon;
			pNdisCommon->bRxControlState = RT_RX_NORMAL;
			GLWdiTxRxStatistics.numWdiRxRestart++;
		}
	}
	else
	{
		pTempAdapter = GetDefaultAdapter(pAdapter);
		do
		{
			pNdisCommon = pTempAdapter->pNdisCommon;
			pNdisCommon->bRxControlState = RT_RX_NORMAL;
			GLWdiTxRxStatistics.numWdiRxRestart++;
			pTempAdapter = GetNextExtAdapter(pTempAdapter);
		}while( pTempAdapter != NULL );
	}
	PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	
	FunctionOut(COMP_INIT);
}


void N6SdioWdi_RxGetMpdus(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PEER_ID		PeerId,
	_In_ WDI_EXTENDED_TID	ExTid,
	_Out_ PNET_BUFFER_LIST	*ppNBL)
{
	PADAPTER	pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_SINGLE_LIST_ENTRY	pFirst = NULL, pTemp = NULL;
	u1Byte		index = 0;

	FunctionIn(COMP_RECV);

	RT_TRACE(COMP_RECV, DBG_LOUD, ("PeerId %d, ExTid %d\n", PeerId, ExTid));

	if( (PeerId >= MAX_PEER_NUM) && (PeerId != WDI_PEER_ANY) )
	{
		RT_TRACE(COMP_RECV, DBG_WARNING, ("Incorrect peer\n"));
		KeBugCheckEx(
			OID_RT_FORCED_BUG_CHECK, 
			(ULONG)PeerId,
			(ULONG)ExTid,
			(ULONG)PeerId,
			(ULONG)0);
	}
	else
	{
		*ppNBL = NULL;

		if( PeerId == WDI_PEER_ANY)
		{
			RT_TRACE(COMP_RECV, DBG_LOUD, ("WDI wants to fetch all frames of all peers.\n"));

			PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
			for(index = 0; index < MAX_PEER_NUM; index++)
			{
				if( (pDefaultAdapter->RxPeerTable[index].bUsed == TRUE) )
				{
					if(N6CIsNblWaitQueueEmpty(pDefaultAdapter->RxPeerQueue[index]))
						continue;

					pTemp = RTGetHeadSList(&pDefaultAdapter->RxPeerQueue[index]);
					if( pFirst == NULL )
					{
						pFirst = pTemp;
					}
					else
					{
						N6CConcatenateTwoList(pFirst, pTemp);
					}
					RTInitializeSListHead(&pDefaultAdapter->RxPeerQueue[index]);
				}
			}

			if( pFirst != NULL )
			{
				GLWdiTxRxStatistics.numRxIndicateNBLToUE++;
				for(pTemp = pFirst; pTemp->Next != NULL; pTemp = pTemp->Next)
				{
					GLWdiTxRxStatistics.numRxIndicateNBLToUE++;
				}
				*ppNBL = RT_GET_NBL_FROM_QUEUE_LINK(pFirst);
			}
			PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		}
		else
		{
			PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
			if( (pDefaultAdapter->RxPeerTable[PeerId].bUsed == TRUE) )
			{
				if(N6CIsNblWaitQueueEmpty(pDefaultAdapter->RxPeerQueue[PeerId]))
				{
					RT_TRACE(COMP_RECV, DBG_TRACE, ("Queue %d: no frames in queue\n", PeerId));
				}
				else
				{
					pFirst = RTGetHeadSList(&pDefaultAdapter->RxPeerQueue[PeerId]);
					GLWdiTxRxStatistics.numRxIndicateNBLToUE++;
					for(pTemp = pFirst; pTemp->Next != NULL; pTemp = pTemp->Next)
					{
						GLWdiTxRxStatistics.numRxIndicateNBLToUE++;
					}
					*ppNBL = (PNET_BUFFER_LIST)N6CGetHeadNblWaitQueue(pDefaultAdapter->RxPeerQueue[PeerId]);
					RTInitializeSListHead(&pDefaultAdapter->RxPeerQueue[PeerId]);
				}
			}
			PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		}
	}

	FunctionOut(COMP_RECV);
}


void N6SdioWdi_RxReturnFrames(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ PNET_BUFFER_LIST	pNBL)
{
	PADAPTER	pAdapter = (PADAPTER) MiniportTalTxRxContext;

	FunctionIn(COMP_RECV);

	WDI_FreeRxFrame(pAdapter, pNBL);

	FunctionOut(COMP_RECV);
}


void N6SdioWdi_RxResume(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext)
{
	PADAPTER			pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_NDIS6_COMMON	pDefaultNdisCommon = pDefaultAdapter->pNdisCommon;

	FunctionIn(COMP_INIT);

	PlatformAcquireSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	if( pDefaultNdisCommon->bRxDataPathState == RT_RX_PAUSE )
	{
		pDefaultNdisCommon->bRxDataPathState = RT_RX_NORMAL;
		GLWdiTxRxStatistics.numWdiRxResume++;
	}
	PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);

	PlatformReleaseSemaphore(&(GetDefaultAdapter(pAdapter)->pNdisCommon->RxNotifySemaphore));

	FunctionOut(COMP_INIT);
}



void N6SdioWdi_RxThrottle(
	_In_ TAL_TXRX_HANDLE		MiniportTalTxRxContext,
	_In_ WDI_RX_THROTTLE_LEVEL	RxThrottleLevel)
{
	FunctionIn(COMP_RECV);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(RxThrottleLevel);

	FunctionOut(COMP_RECV);
}

void N6SdioWdi_RxPpduRssi(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ PNET_BUFFER_LIST	pNBL,
	_Out_ UINT8				*pRssi)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
	PADAPTER	pAdapter = (PADAPTER) MiniportTalTxRxContext;

	FunctionIn(COMP_RECV);

	*pRssi = (UINT8)pAdapter->RxStats.SignalStrength;

	FunctionOut(COMP_RECV);
}

NDIS_STATUS N6SdioWdi_TalTxRxStart(
	_In_ TAL_TXRX_HANDLE		MiniportTalTxRxContext,
	_In_ PWDI_TXRX_TARGET_CONFIGURATION	pWifiTxRxConfiguration,
	_Out_ PTAL_TXRX_PARAMETERS	pTalTxRxParameters)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(pWifiTxRxConfiguration);

	if (pTalTxRxParameters != NULL)
	{
		pTalTxRxParameters->MaxOutstandingTransfers = 100;
	}

	//
	// test a few cycles
	//

	#if DEV_TEST
	{
		PWDI_FRAME_METADATA pWdiFrameMeta;
		PWDI_FRAME_MYWORKSPACE pWdiFrameMyWorkSpace;
		int i;

		for (i=0; i < 7; i++ ) {
			__debugbreak();
			pWdiFrameMeta = g_NdisWdiDataApi.AllocateWiFiFrameMetaData( g_NdisMiniportDataPathHandle );
			if ( pWdiFrameMeta ) {
				pWdiFrameMeta->Linkage.Flink =  (PLIST_ENTRY)'f00d';
				pWdiFrameMyWorkSpace = (PWDI_FRAME_MYWORKSPACE) ((UINT8*)pWdiFrameMeta+sizeof(WIFI_FRAME_METADATA));
				pWdiFrameMyWorkSpace->Signature = 'ffdd';

				g_NdisWdiDataApi.FreeWiFiFrameMetaData( g_NdisMiniportDataPathHandle, pWdiFrameMeta );
			} else {
				ASSERT( 0 );
			}
		}
	}
	#endif // DEV_TEST

	FunctionOut(COMP_INIT);

	return ndisStatus;
}


void N6SdioWdi_TalTxRxStop(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext)
{
	PADAPTER			pAdapter = (PADAPTER) MiniportTalTxRxContext;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	
	FunctionIn(COMP_INIT);	

	//1 Move DisableInterrupt to bottom of StopOperation handler to avoid incoming data during mac/port deletion
	
	//1 Wait for return packets then release/free semaphore for RxNotifyThread
	N6CWaitForReturnPacket(pAdapter);
	PlatformReleaseSemaphore(&(pNdisCommon->RxNotifySemaphore));
	PlatformFreeSemaphore(&(pNdisCommon->RxNotifySemaphore));

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TalTxRxAddPort(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_OPERATION_MODE	OpMode)
{	
	PADAPTER	pAdapter = (PADAPTER) MiniportTalTxRxContext;

	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(OpMode);

	WDI_AddGroupPeer(pAdapter, (u2Byte)PortId);

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TalTxRxDeletePort(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId)
{
	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);
	
	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TalTxRxSetPortOpMode(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_OPERATION_MODE	Opmode)
{
	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);
	UNREFERENCED_PARAMETER(Opmode);

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TalTxRxResetPort(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId)
{
	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);

	FunctionOut(COMP_INIT);
}


void N6SdioWdi_TalTxRxPeerConfig(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_PEER_ID		PeerId,
	_In_ PWDI_TXRX_PEER_CFG	pPeerCfg)
{
	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);
	UNREFERENCED_PARAMETER(PeerId);
	UNREFERENCED_PARAMETER(pPeerCfg);

	FunctionOut(COMP_INIT);
}

void N6SdioWdi_TalTxRxPeerDeleteConfirm(
	_In_ TAL_TXRX_HANDLE	MiniportTalTxRxContext,
	_In_ WDI_PORT_ID		PortId,
	_In_ WDI_PEER_ID		PeerId)
{
	FunctionIn(COMP_INIT);

	UNREFERENCED_PARAMETER(MiniportTalTxRxContext);
	UNREFERENCED_PARAMETER(PortId);
	UNREFERENCED_PARAMETER(PeerId);

	FunctionOut(COMP_INIT);
}

NDIS_STATUS
N6SdioWdi_AdapterCapabilities(
	_In_  PADAPTER				pAdapter,
	_In_  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	WDI_GET_ADAPTER_CAPABILITIES_PARAMETERS	adapterCap = {0};
	PUCHAR			pOutput = NULL;
	ULONG			length = 0;

	PWDI_MESSAGE_HEADER pOidHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	n6sdioWdi_GenAdapterCaps(pAdapter, &adapterCap);

	ndisStatus = GenerateWdiGetAdapterCapabilities(
					&adapterCap,
					0,
					&pAdapter->pPortCommonInfo->WdiData.TlvContext,
					&length,
					&pOutput );

	RT_TRACE(COMP_INIT, DBG_TRACE, ("[WDI], GenerateWdiGetAdapterCapabilities() return length = %d\n", length));
	RT_PRINT_DATA(COMP_INIT, DBG_TRACE, ("buffer\n"), pOutput, length);

	if(NDIS_STATUS_SUCCESS != ndisStatus)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("[WDI], GenerateWdiGetAdapterCapabilities failed for status: 0x%08X\n", ndisStatus));
		return ndisStatus;
	}

	if (pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength >= 
		(length + sizeof(WDI_MESSAGE_HEADER)))
	{
		PlatformMoveMemory(
			((UCHAR *)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer) + sizeof(WDI_MESSAGE_HEADER),
			pOutput,
			length);
		pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += length;
	}
	else
	{
		ndisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
		pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += (length + sizeof(WDI_MESSAGE_HEADER));
		RT_TRACE(COMP_INIT, DBG_WARNING, ("[WDI], GenerateWdiGetAdapterCapabilities() failed with status(%x)\n", ndisStatus));       
	}
	
	if(pOutput)
	{
		FreeGenerated(pOutput);
		pOutput = NULL;
	}

	pOidHeader->Status = ndisStatus;
	
	return ndisStatus;
}

VOID
N6SdioWdi_PnPSetPower(
	_In_  PADAPTER			Adapter
    )
{
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_SDIO_DEVICE			pDevice 		= GET_RT_SDIO_DEVICE(Adapter);
	PRT_POWER_SAVE_CONTROL	pPSC 			= GET_POWER_SAVE_CONTROL(&(Adapter->MgntInfo));
	PRT_NDIS_COMMON			pNdisCommon 	= Adapter->pNdisCommon;
	PMGNT_INFO				pMgntInfo		= &Adapter->MgntInfo;
	PRT_NLO_INFO			pNLOInfo 		= &(pMgntInfo->NLOInfo);		
	BOOLEAN					bWakeReconnect	= TRUE;
	BOOLEAN 				bMacPwrCtrlOn;	
	BOOLEAN 				bSupportRemoteWakeUp;
	BOOLEAN					bEnableWoLCapabilities;
	RT_RF_POWER_STATE 		rfState;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("[WDI], N6SdioWdi_PnPSetPower() ==>\n"));

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);
	bEnableWoLCapabilities = MgntIsWoWLANCapabilityEnable(Adapter);
	
	if(pDevice->CurrentPowerState == NdisDeviceStateD0)
	{ // wake up. 
		
		//
		// <Roger_Notes> Reset corresponding PnP event to monitor MiniportCheckForHang routine.
		// 2009.09.30.
		//
		pDevice->bChkForHangAfterPnP = FALSE;
		NdisResetEvent(&pDevice->SetPnpChkForHangEvent);
	
		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			PMGNT_INFO		pExtMgntInfo = NULL;
			PRT_POWER_SAVE_CONTROL	pExtPSC = NULL;
			PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;
			PRT_POWER_SAVE_CONTROL pDefualtPSC = GET_POWER_SAVE_CONTROL(pDefaultMgntInfo);

			pDefaultAdapter->bEnterPnpSleep = FALSE;
			pDefaultAdapter->bWakeFromPnpSleep = TRUE;
			pDefaultAdapter->bDriverIsGoingToPnpSetPowerSleep = FALSE;
		
			while(pExtAdapter != NULL)
			{
				pExtMgntInfo = &pExtAdapter->MgntInfo;
				pExtPSC = GET_POWER_SAVE_CONTROL(pExtMgntInfo);
			
				pExtAdapter->bEnterPnpSleep = FALSE;
				pExtAdapter->bWakeFromPnpSleep = TRUE;
				pExtAdapter->bDriverIsGoingToPnpSetPowerSleep = FALSE;
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}

		// Clear the flag after resuming from device low low power state. We expect that
		// before the system sets OID_PNP_CAPABILITIES to put the device to low PS state, it will
		// set OID_PM_PARAMETERS first. In order to seperate FSS mode from S3/S4/Selective suspend
		// mode, we use "bSetPMParameters" flag to decide some behaviors. 2012.09.13, by tynli.
		pPSC->bSetPMParameters = FALSE; 

		if(Adapter->MgntInfo.RegSuspendTimerInLowPwr)
			N6CTimerResourceAction(Adapter, RT_TIMER_RESOURCE_ACTION_RESUME);

		RT_ENABLE_SDIO_TRANSFERS(Adapter);

		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

			while(pTargetAdapter != NULL)
			{
				pTargetAdapter->bInitializeInProgress=TRUE;
				pTargetAdapter->bSWInitReady=TRUE;

				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}

#if USE_WDF_SDIO
		WdfSdioTx_Enable(Adapter);
#else
		N6WdmSdio_Enable(Adapter);
		N6WdmSdioTx_Enable(Adapter);
#endif
		N6SdioDummyIO(pDevice);
		
		// Control SDIO Clock rate settings
		if(pDevice->bRegSdioSpeedSDR25)
		{
			N6SdioConfigureBusCLKByWorkAround(pDevice);
		}

		if(bEnableWoLCapabilities)
		{
			N6SdioEnableDeviceInterrupt(Adapter);
		}
	
		//
		// 070106, rcnjko:
		// Re-initialize H/W for USB Bus is suspend.
		// Besides, we also issue bulk IN transfers at InitializeAdapterHandler().
		//
		// <tynli_note> On Intel Bay Trail low power platform, "D2 -> D0 -> miniport shutdown" function will be 
		// called by user initialiated Restart. It has site effect to 8723BS card lost issue. So this is a work around
		// to add the condition to prevent from acting HaltAdapter in PNP D2 and D0 under Restart flow. 2013.11.18.
		if((pMgntInfo->bIntelPatchPNP && !pMgntInfo->bReceiveSystemPSOID) || (!pMgntInfo->bIntelPatchPNP))
		{
			Adapter->HalFunc.InitializeAdapterHandler(Adapter, Adapter->MgntInfo.dot11CurrentChannelNumber);
		}

		//
		// <tynli_Note> Reconnect to AP in the following conditions for AOAC on connected standby platform.
		// (1) The media state is connected before PNP sleep and HW is re-initiated after resupmtion.
		// (2) The media state is connected before PNP sleep and wake reason is AP lost, on disassoc, or on deauth.
		// (3) According to the variable "bPnpKeepConnectToAP", we should keep connection (do not perform reconnect
		//      mechanism) if "bPnpKeepConnectToAP" is TRUE. It will be set to TRUE while remote wake up function
		//      is decided to be operation in PNP sleep flow. 2014.11.12, by tynli.
		//
		if(pMgntInfo->bRegPnpKeepLink)
		{
			if((Adapter->MgntInfo.bMediaConnect && (Adapter->bReInitHW || pPSC->WakeUpReason == WOL_REASON_AP_LOST ||
				pPSC->WakeUpReason == WOL_REASON_DISASSOC ||pPSC->WakeUpReason == WOL_REASON_DEAUTH)) || 
				(!Adapter->pNdisCommon->bPnpKeepConnectToAP))
			{
				pMgntInfo->bPerformPnpReconnect = TRUE;
			}
			else
			{
				pMgntInfo->bPerformPnpReconnect = FALSE;
			}
	
			RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower(): bPerformPnpReconnect=%d, bReInitHW=%d\n", 
				pMgntInfo->bPerformPnpReconnect, Adapter->bReInitHW));
		}
		else
		{
			pMgntInfo->bPerformPnpReconnect = TRUE;
		}

		if(bSupportRemoteWakeUp) // Need to be set before N6RestoreLastInitSettingAterWakeUP().
		{							
			Adapter->HalFunc.EnableHWSecCfgHandler(Adapter); //For HW Security. by tynli. 2009.06.24.
		}	
		
		PlatformSetCheckForHangTimer(Adapter);

		PlatformStartWorkItem(&(Adapter->pPortCommonInfo->pPortHelper->CreateDeleteMacWorkitem));
		
		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

			while(pTargetAdapter != NULL)
			{
				pTargetAdapter->bInitializeInProgress = FALSE;
				
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
		
		//ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE)->bInitializeInProgress=FALSE;
		//pDefaultAdapter->bInitializeInProgress=FALSE;

		//fix some InitilaAdapter error under sleep
		if( !Adapter->bInHctTest )
		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);
				
			while(pTargetAdapter != NULL)
			{
				RT_TRACE(COMP_AP, DBG_LOUD, ("Adapter on Port: %d, APType: %d\n", pTargetAdapter->pNdis62Common->PortNumber, MgntActQuery_ApType(pTargetAdapter)));
				if(MgntActQuery_ApType(pTargetAdapter) == RT_AP_TYPE_VWIFI_AP)
				{
					// Force vWifi being OFF(STATE: not available), then reset it(STATE: not started)
					pTargetAdapter->bvWifiStopBeforeSleep = TRUE;
					N62CApIndicateStopAp(pTargetAdapter);
				}
					
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
		
		Adapter->pNdisCommon->bWakeupAutoLinkInProgressing = TRUE;

		if(pMgntInfo->bPerformPnpReconnect)
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	N6SdioWdi_PnPSetPower()  try to connect to %s\n", Adapter->MgntInfo.Ssid.Octet));
		}
		N6RestoreLastInitSettingAterWakeUP(Adapter);

		// Indicate disassociation state when RF is off becuase we do not disconnect before sleep in WoWLAN mode.		
		// 2011.10.27. by tynli.
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));
		if(rfState == eRfOff)
		{
			if(Adapter->pNdisCommon->bPnpKeepConnectToAP)
			{
				DrvIFIndicateDisassociation(Adapter, unspec_reason, pMgntInfo->Bssid);
			}
			pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
		}	

		/*if(OS_SUPPORT_WDI(Adapter))
		{
			// TODO: WoWLAN

			// TODO: NLO
		}
		else */if(pNdisCommon->bWakeupAutoLinkInProgressing)
		{
			if( pNLOInfo->NumDot11OffloadNetwork != 0 && Adapter->bInHctTest)
			{
				u1Byte i = 0;
				BOOLEAN bTargetInNLO = FALSE;
				
				if(Adapter->MgntInfo.bMediaConnect || pNdisCommon->bDissociateBeforeSleep)
				{				
					for(i=0; i<pNLOInfo->NumDot11OffloadNetwork; i++)
					{
						if(pMgntInfo->Ssid.Length && (pMgntInfo->Ssid.Length == pNLOInfo->dDot11OffloadNetworkList[i].ssidlen))
							if(!PlatformCompareMemory(pMgntInfo->Ssid.Octet, pNLOInfo->dDot11OffloadNetworkList[i].ssidbuf, pMgntInfo->Ssid.Length))
								bTargetInNLO = TRUE;
					}

					if(!bTargetInNLO)
					{
						RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower(): LinkTargetSSID is not in Dot11OffloadNetworks!\n"));
						pNdisCommon->ScanPeriod = 0;
						PlatformHandleNLOnScanRequest(Adapter);
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
					else
					{
						RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower(): FFFFind LinkTargetSSID in Dot11OffloadNetworks!\n"));
						if(!(ACTING_AS_AP(Adapter) || IsExtAPModeExist(Adapter)))
						{
							if(pMgntInfo->Ssid.Length != 0)
							{
								RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower()	try to connect to %s\n", Adapter->MgntInfo.Ssid.Octet));

								// Set Roam flag 
								MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_SLEEP, ROAMINGSTATE_SCANNING);

								DrvIFIndicateRoamingStart(Adapter);
								pNdisCommon->bDissociateBeforeSleep = FALSE;

								MgntActSet_802_11_SSID(Adapter, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, TRUE );
							}
						}
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
				}
				else
				{
					//if(pPSC->DxNLOEnable && (pPSC->WakeUpReason == WOL_REASON_NLO_SSID_MATCH))
					{ // FW NLO SSID matched
						// We should execute scan here because after indicating NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED,
						// OS will not set scan request OID (different with msdn's mention) and then query BSS list OID. So if we
						// do not keep NLO matched SSID in BSS list, the state machine will keep in disconnect then HCK test will be failed.
						// This may be a workaround because we do not know the behavior expected by MSFT. 2014.03.06. Add by tynli.
						RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower(): Set Scan Request OID after NLO issue for HctTest.\n"));
						pNdisCommon->ScanPeriod = 0;
						PlatformHandleNLOnScanRequest(Adapter);
						pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
					}
				}
//				pMgntInfo->bPwrSaveState = FALSE;
//				Adapter->bWakeFromPnpSleep = FALSE;
			}
			else
			{
				// NLO enabled by the upper layer and in disconnect state, we should make sure that we will connect
				// to the expected AP by indicating NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED
				// to declare that NLO scan is finished.
				if((pNLOInfo->NumDot11OffloadNetwork != 0) && (!Adapter->MgntInfo.bMediaConnect) &&
					(!pNdisCommon->bDissociateBeforeSleep)) 
				{
					// We should execute scan here because after indicating NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED,
					// OS will not set scan request OID (different with msdn's mention) and then query BSS list OID. So if we
					// do not keep NLO matched SSID in BSS list, the state machine will keep in disconnect then HCK test will be failed.
					// This may be a workaround because we do not know the behavior expected by MSFT. 2014.03.06. Add by tynli.
					RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower(): Set Scan Request OID after NLO issue.\n"));
					pNdisCommon->ScanPeriod = 0;
					PlatformHandleNLOnScanRequest(Adapter);
					pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
				}
				else
				{
					
					{
						if((!ACTING_AS_AP(Adapter) || (!IsExtAPModeExist(Adapter) && bEnableWoLCapabilities)) 
							&& (pMgntInfo->Ssid.Length != 0) && !(IsSSIDDummy(pMgntInfo->Ssid)) && pMgntInfo->bPerformPnpReconnect
							)
						{
							RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower()	try to connect to %s ACTING_AS_AP(Adapter)	%d IsExtAPModeExist(Adapter) %d\n", 
									Adapter->MgntInfo.Ssid.Octet, ACTING_AS_AP(Adapter) , IsExtAPModeExist(Adapter)));
							bWakeReconnect = TRUE;
						}
						else
						{
							bWakeReconnect = FALSE;
						}

						if(bWakeReconnect)
						{
							// Set Roam flag 
							MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_SLEEP, ROAMINGSTATE_SCANNING);

							pNdisCommon->PNPconnentCout = 0;
							// Try to connent AP !!
							// Set timer to 10ms to prevent from being returned by bSetPnpPwrInProgress in MgntLinkRetry function
							// or we could call PNPReConnectTimer directly.
							// 1. Indicate roaming start. 2. Reconnect to AP in PNPReConnectTimer().
							PlatformSetTimer(Adapter, &pNdisCommon->PNPReConnentTimer, 10);
						}
						else
						{
							RT_TRACE(COMP_POWER, DBG_LOUD, ("N6SdioWdi_PnPSetPower(): do not try to connect to AP\n"));
							pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
							pNdisCommon->bDissociateBeforeSleep = FALSE;

							//
							// <tynli_note> If we keep connection after PNP resumption, we should reset Rx TS to avoid 
							// packet droped by the sequence number is over Rx reorder window size because we will not
							// update the Rx TS sequence number during PNP sleep stage. 2015.01.22.
							//
							if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE && pMgntInfo->mAssoc)
							{
								QosResetRxTS(Adapter);
								MgntRecoverFWOffloadMechanism(Adapter);
							}
						}
					}
				}
			}
			// Move the flag to be cleared in PNPReConnentTimerCallBack().
			//pNdisCommon->bWakeupAutoLinkInProgressing = FALSE;
		}
		Adapter->bWakeFromPnpSleep = FALSE;

#if USE_WORKITEM
		{
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	Start check for hang workitem\n"));
			PlatformStartWorkItem( &(pHalData->RtCheckForHangWorkItem) );
		}
#endif		
	}
	else if(pDevice->CurrentPowerState > NdisDeviceStateD0)
	{ // sleep.
		PlatformIndicateCustomStatus(
				Adapter,
				RT_CUSTOM_EVENT_IRP_UNLOAD,
				RT_CUSTOM_INDI_TARGET_IRP,
				NULL,
				0);
		
		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			PMGNT_INFO		pExtMgntInfo = NULL;
			PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;

			pDefaultMgntInfo->bStartApDueToWakeup=TRUE;	

			pDefaultAdapter->bEnterPnpSleep = TRUE;
		
			while(pExtAdapter != NULL)
			{
				pExtMgntInfo = &pExtAdapter->MgntInfo;
			
				pExtMgntInfo->bStartApDueToWakeup=TRUE;	

				pExtAdapter->bEnterPnpSleep = TRUE;
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}
		pMgntInfo->bInToSleep = TRUE;

		//Moved to PNP wake flow. 2013.04.
		//MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_SLEEP, ROAMINGSTATE_SCANNING);
		
		// 20100721 Joseph: Modified for Velocity Suspend test.
		// Reset IPS state only.
		// Since NIC is going to Halt for sleep, driver does not need to initialize the HW again.
		// This shall be revised in the future and also take NIC Disable/Radio-off into consideration.
		if((!bSupportRemoteWakeUp) &&
			(pPSC->IPSState==eIPSDozed) &&
			/*(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC) &&*/ 
			RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVL_HALT_NIC))
		{
			LPSLeaveAndCheckReady(Adapter);

			NicIFDisableInterrupt(Adapter); 
		
			pPSC->eInactivePowerState = eRfOn;
			pPSC->IPSState = eIPSAwake;
			pMgntInfo->RfOffReason &= (~RF_CHANGE_BY_IPS);
		}
		else
		{
			LeaveAllPowerSaveMode(Adapter);		
		}
		NicIFClearInterrupt(Adapter);
		
		// Recover the BW to 40MHz for a specific case. To see the description of 
		// function HTRecoverBWTo40MHz().  2012.12.20, by tynli.
		if(bSupportRemoteWakeUp && 
			(Adapter->MgntInfo.OpMode == RT_OP_MODE_INFRASTRUCTURE))
		{
			HTRecoverBWTo40MHz(Adapter);
		}
		
		PlatformAcquireSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
		while( pMgntInfo->RFChangeInProgress)
		{
			PlatformReleaseSpinLock(Adapter,RT_RF_STATE_SPINLOCK);

			RT_TRACE(COMP_POWER,DBG_LOUD,
				("RF is in progress, need to wait until rf chang is done.\n"));
			delay_ms(1);
			
			PlatformAcquireSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
		}	
		PlatformReleaseSpinLock(Adapter,RT_RF_STATE_SPINLOCK);
//		Adapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;

		//In Win7 for WoWLAN, we should write CAM to allow Hw to encrypt/decrypt packets when S3/S4 because
		//there are some type of packets are en/decrypted by Sw when wake up. by tynli.
		if(bSupportRemoteWakeUp)
		{
			PRT_SECURITY_T	pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
			u1Byte	KeyIndex;
			u1Byte CAM_CONST_BROAD[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
			
			Adapter->bHWSecurityInWoL = TRUE;
			
			Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);

			if(pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 ||
				pSecInfo->PairwiseEncAlgorithm ==RT_ENC_ALG_WEP104)
			{	//Set WEP key to hw (but WEP 1.x cannot work)
				for(KeyIndex=0; KeyIndex<4; KeyIndex++)
				{
					if(pSecInfo->KeyLen[KeyIndex] != 0)
					{
						Adapter->HalFunc.SetKeyHandler(Adapter, 
								KeyIndex, 
								CAM_CONST_BROAD, 
								FALSE, 
								pSecInfo->PairwiseEncAlgorithm, 
								TRUE, 
								FALSE);
					}
				}
			}
			else if( pSecInfo->PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER )
			{	//Set group key for other encrypyion mode (TKIP, AES)
				Adapter->HalFunc.SetKeyHandler(Adapter, 
							pSecInfo->GroupTransmitKeyIdx, //KeyIndex,
							CAM_CONST_BROAD,
							TRUE, //IsGroup,
							pSecInfo->GroupEncAlgorithm,
							FALSE,
							FALSE);
			}
			Adapter->bHWSecurityInWoL = FALSE;
		}
		//-Write CAM End------------------------------------------
		
		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			pDefaultAdapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;

			while(pExtAdapter != NULL)
			{
				pExtAdapter->bDriverIsGoingToPnpSetPowerSleep = TRUE;
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}

		// <tynli_note> On Intel Bay Trail low power platform, "D2 -> D0 -> miniport shutdown" function will be 
		// called by user initialiated Restart. It has site effect to 8723BS card lost issue. So this is a work around
		// to add the condition to prevent from acting HaltAdapter in PNP D2 and D0 under Restart flow. 2013.11.18.
		if((pMgntInfo->bIntelPatchPNP && !pMgntInfo->bReceiveSystemPSOID) || (!pMgntInfo->bIntelPatchPNP))
		{
			if(bSupportRemoteWakeUp && IS_WOWLAN_OPERATING_MODE(pMgntInfo))
				Adapter->HalFunc.SleepAdapterHandler(Adapter);
			else
				Adapter->HalFunc.HaltAdapterHandler(Adapter, FALSE);
		}

		// We may set H2C to Fw during HaltAdapter so release H2C queue after Halt. 2011.10.04. by tynli.
		Adapter->HalFunc.WaitForH2CQueueEmptyHandler(Adapter);

		if((pMgntInfo->RfOffReason & (RF_CHANGE_BY_IPS|RF_CHANGE_BY_SW|RF_CHANGE_BY_HW)) &&
			RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K))
		{
			// Wait FW 32K Event
			// Prefast warning C28121: The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
			// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
			// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
			if(!NdisWaitEvent(&pDevice->FwPsClockOffEvent, 20))
			{
				// Force Hw clock off to 32K
				RT_TRACE(COMP_POWER, DBG_LOUD, ("Force Hw clock off to 32K!\n"));
				SetFwPSRFOffLowPower(Adapter);
			}
		}

		if(pDevice->CurrentPowerState == NdisDeviceStateD2 && bEnableWoLCapabilities)
		{
			N6SdioDisableDeviceInterrupt(Adapter);
		}
		// Wait until all H2C cmd finished than disable cmd53 to prevent from cmd53 lost issue.
		// To disable cmd53 operation here to avoid that system reset NIC power during sleep or hibernate mode
		// but driver does not call card disable flow to disable cmd53 operation, it will cause driver use cmd53
		// to access non-power on regsiter then I/O fail after resume.
		// Dsiable CMD53 R/W operation
		bMacPwrCtrlOn = FALSE;// Disable CMD53 R/W
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));

		if(pDevice->bRegSdioSpeedSDR25)
		{
			N6SdioConfigureBusCLK_SDR12(pDevice);
		}

		RT_DISABLE_SDIO_TRANSFERS(Adapter);

#if USE_WDF_SDIO
		WdfSdio_Disable(Adapter);
#else
		N6WdmSdio_Disable(Adapter);	
#endif

		//
		// 070531, rcnjko: although NDIS6 had made driver stack go to paused state 
		// before sleeping, it is safer to return pending tx to upper layer.
		//
		N6SdioReturnAllPendingTxPackets(Adapter);

		// Release Tx Queue buffered context if needed. added by Roger, 2014.04.29.
		N6SdioReleaseTxQueuePending(Adapter);	

		// 2011/08/23 MH For check for hang test only. Prevent check for HANG IQK write incorrect BB register under
		// RF off state.
		{
			HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("	Stop check for hang workitem\n"));
			PlatformStopWorkItem( &(pHalData->RtCheckForHangWorkItem) );
		}

		PlatformStopWorkItem(&(Adapter->pPortCommonInfo->pPortHelper->CreateDeleteMacWorkitem));

		//
		// As devices enter low power states, the driver managing the devices should cancel any programmed timers. 
		// Added by Roger, 2016.01.18
		//
		if(Adapter->MgntInfo.RegSuspendTimerInLowPwr){
			N6CTimerResourceAction(Adapter, RT_TIMER_RESOURCE_ACTION_SUSPEND);
			N6WaitTimerSync(Adapter);
		}

		{
			PADAPTER pExtAdapter = GetNextExtAdapter(pDefaultAdapter);
			PMGNT_INFO		pExtMgntInfo = NULL;
			PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;

			pDefaultMgntInfo->bDriverIsGoingToSleep = FALSE; 
		
			while(pExtAdapter != NULL)
			{
				pExtMgntInfo = &pExtAdapter->MgntInfo;
			
				pExtMgntInfo->bDriverIsGoingToSleep = FALSE; 
				
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
		}

		RT_TRACE(COMP_POWER, DBG_LOUD, ("	N6SdioWdi_PnPSetPower()  sleep\n"));
	}
	else
	{
		RT_TRACE(COMP_POWER, DBG_WARNING, ("N6SdioWdi_PnPSetPower(): unexpected CurrentPowerState: %#X\n", pDevice->CurrentPowerState));
	}

	Adapter->bInSetPower = FALSE;	
	pMgntInfo->bSetPnpPwrInProgress = FALSE;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("[WDI], N6SdioWdi_PnPSetPower() <==\n"));
}

NDIS_STATUS
N6SdioWdi_Mgnt_SetPower(
	_In_  PADAPTER			pAdapter,
	_In_  PRT_OID_HANDLER 	pOidHandle
	)
{
	WDI_SET_POWER_PARAMETERS	*Params 				= pOidHandle->tlvParser.parsedTlv.paramSetPower;
	PMGNT_INFO					pMgntInfo				= &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL		pPSC					= GET_POWER_SAVE_CONTROL(pMgntInfo);		
	PRT_SDIO_DEVICE 			pDevice 				= GET_RT_SDIO_DEVICE(pAdapter); 		
	PRT_NDIS6_COMMON			pNdisCommon 			= pAdapter->pNdisCommon;
	BOOLEAN 					bEnableWoLCapabilities	= FALSE;		
	NDIS_STATUS					ndisStatus 				= NDIS_STATUS_SUCCESS;
	NDIS_DEVICE_POWER_STATE 	NewPowerState;	
	OCTET_STRING				savedssid;
	u1Byte						ssidbuf[33];
	u1Byte						bssidbuf[6];

	LARGE_INTEGER	StartTime, EndTime;
	LARGE_INTEGER	Freq;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("[WDI], Set OID_PNP_SET_POWER() ==>\n"));

	if( Params )
	{
		NewPowerState = Params->PowerState;
		
#if SOFTWARE_TRACE_LOGGING
		TraceLoggingWrite(
			g_hProvider,
			"WDI Sample Set Power",
			TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
			TraceLoggingUInt32(NewPowerState, "NewPowerState"),
			TraceLoggingUInt32(pDevice->CurrentPowerState, "CurrentPowerState"));
#endif
		
		if(pDevice->CurrentPowerState == NewPowerState)
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Wdi_Set_Power_State(): already in this power state %d\n", NewPowerState));
			return ndisStatus;
		}

		// For Working Time Calculation
		pAdapter->bCtrlPnPTime = TRUE;
		pAdapter->PnPTotalTime.QuadPart = 0;
		pAdapter->PnPIOTime.QuadPart = 0;
		pDevice->PnPSdBusWorkTime.QuadPart = 0;
					
		StartTime = KeQueryPerformanceCounter(&Freq);
		//

		pMgntInfo->bSetPnpPwrInProgress = TRUE;

		if(NewPowerState == NdisDeviceStateD0)
		{ // Wake up.

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Set OID_PNP_SET_POWER: wake up from %d to D0\n", pDevice->CurrentPowerState));

			pDevice->CurrentPowerState = NewPowerState;

			pPSC->PnpWakeD0Cnt++;	// For Dbgmon record
			pPSC->LastPnpWakeTime = PlatformGetCurrentTime();

			pAdapter->bSurpriseRemoved = FALSE;

			pAdapter->bInSetPower = TRUE;

			MgntStopBeacon(pAdapter);
	
			//
			// Immediately disable adapter in this irql level, then return success. By Bruce, 2008-10-29.
			//
			N6SdioWdi_PnPSetPower(pAdapter);

		}
		else if((NewPowerState == NdisDeviceStateD2) || (NewPowerState == NdisDeviceStateD3))
		{
			bEnableWoLCapabilities = MgntIsWoWLANCapabilityEnable(pAdapter);

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Wdi_Set_Power_State(): going to sleep: %d\n", NewPowerState));

			{
				PADAPTER		pExtAdapter 		= GetNextExtAdapter(pAdapter);
				PMGNT_INFO		pExtMgntInfo		= NULL;
				PMGNT_INFO		pDefaultMgntInfo	= &pAdapter->MgntInfo;

				pDefaultMgntInfo->bDriverIsGoingToSleep = TRUE; 
			
				while(pExtAdapter != NULL)
				{
					pExtMgntInfo = &pExtAdapter->MgntInfo;
				
					pExtMgntInfo->bDriverIsGoingToSleep = TRUE; 
					
					pExtAdapter = GetNextExtAdapter(pExtAdapter);
				}
			}
			
			MgntResetJoinProcess(pAdapter);

			// For Intel Bay trail platform, we do not support WoWLAN during hibernate (S4). 2013.10.30, by tynli.
			if(NewPowerState != NdisDeviceStateD2)
			{
				pPSC->bPnpEnterD2 = FALSE;
				if(NewPowerState == NdisDeviceStateD3)
					pPSC->PnpSleepEnterD3Cnt++;
				else
					pPSC->PnpSleepEnterUnknownDxCnt++;
			}
			else
			{
				pPSC->bPnpEnterD2 = TRUE;
				pPSC->PnpSleepEnterD2Cnt++;
			}
			
			pPSC->LastPnpSleepTime = PlatformGetCurrentTime();

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Before Stop Watchdog\n"));

	// Cancel watchdog workitem
#if USE_WORKITEM
			{
				HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
				RT_TRACE(COMP_POWER, DBG_LOUD, ("	Stop check for hang workitem\n"));
				PlatformStopWorkItem( &(pHalData->RtCheckForHangWorkItem) );
			}
#endif
			RT_TRACE(COMP_POWER, DBG_LOUD, ("After Stop Watchdog\n"));

			if( !pAdapter->bInHctTest )
			{	// Those in vWifi mode now disassociate all STA before sleep, and will be reset after waking up.
			
				PADAPTER pTargetAdapter = GetDefaultAdapter(pAdapter);
					
				while(pTargetAdapter != NULL)
				{
					if(MgntActQuery_ApType(pTargetAdapter) == RT_AP_TYPE_VWIFI_AP)
					{
						AP_DisassociateAllStation(pTargetAdapter, unspec_reason);
						SecSetSwEncryptionDecryption(pTargetAdapter, FALSE, FALSE);
			
						RT_TRACE(COMP_AP, DBG_LOUD,("vWifi mode on Port: %d disassociates all STA.\n", pTargetAdapter->pNdis62Common->PortNumber));
					}		
			
						pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
				}
			}
			else
			{	// For OS before Win7 still go turning OFF directly.
				NDIS_6_2_AP_CLEAR_BEFORE_SLEEP(pAdapter);	
			}

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Stop vWifi Service\n"));

#if P2P_SUPPORT == 1
			if(P2P_ENABLED(GET_P2P_INFO(pAdapter)))
			{
				if(P2P_DOING_DEVICE_DISCOVERY(GET_P2P_INFO(pAdapter)))
				{// Doing P2P Device Discovery
					P2PScanListCeaseScan(GET_P2P_INFO(pAdapter));
					P2PDeviceDiscoveryComplete(GET_P2P_INFO(pAdapter), TRUE); // P2P State is restored in this function.
				}
			
	#if (MULTIPORT_SUPPORT == 1)
				if(GetFirstDevicePort(pAdapter))
				{
					(GET_P2P_INFO(GetFirstDevicePort(pAdapter)))->uListenStateDiscoverability = 0;
				}
	#endif		
			}	
#endif		

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Handle P2P relative function\n"));

			//
			// Stop scan, roaming and link operation.
			//
			if(MgntScanInProgress(pMgntInfo) || MgntIsLinkInProgress(pMgntInfo) || MgntRoamingInProgress(pMgntInfo))
			{
				RT_TRACE(COMP_POWER, DBG_WARNING, ("@!!!!!Mgnt_SetPower we are scanning.....\n"));
			
				if(MgntScanInProgress(pMgntInfo))
					MgntResetScanProcess(pAdapter);
				PlatformCancelTimer(pAdapter, &pMgntInfo->ScanTimer);
				{
					PADAPTER pLoopAdapter = GetDefaultAdapter(pAdapter);
				
					while(pLoopAdapter)
					{
						pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
						pLoopAdapter->MgntInfo.bDualModeScanStep = 0;
						
						pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
					}
				}
			
				if( MgntIsLinkInProgress(pMgntInfo) || MgntRoamingInProgress(pMgntInfo))
				{
					pMgntInfo->bJoinInProgress = FALSE;
					if(MgntRoamingInProgress(pMgntInfo))
						DrvIFIndicateRoamingStart(pAdapter);
					else
						DrvIFIndicateConnectionStart(pAdapter);
					
					DrvIFIndicateAssociationStart(pAdapter);
					DrvIFIndicateAssociationComplete(pAdapter, RT_STATUS_FAILURE);
					
					if(MgntRoamingInProgress(pMgntInfo))
						DrvIFIndicateRoamingComplete(pAdapter, RT_STATUS_FAILURE);
					else
						DrvIFIndicateConnectionComplete(pAdapter, RT_STATUS_FAILURE);		
					MgntActSet_802_11_DISASSOCIATE( pAdapter, unspec_reason);	
				}			
			}

			RT_TRACE(COMP_POWER, DBG_LOUD, ("After Stop Scan\n"));
			
			MgntResetRoamingState(pMgntInfo);

			//MgntDisconnectAP(Adapter , unspec_reason);
			if(pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
			{
				RT_RF_POWER_STATE eRfPowerState;

				RT_TRACE(COMP_POWER, DBG_LOUD, ("@!!!!!Mgnt_SetPower before we save the ssid, the ssid is %s bMediaConnect %d bIbssStarter %d\n", pMgntInfo->Ssid.Octet, pMgntInfo->bMediaConnect, pMgntInfo->bIbssStarter));

				//save SSID
				savedssid.Octet = ssidbuf;
				CopySsidOS(savedssid, pMgntInfo->Ssid); 
				CopyMem(bssidbuf,pMgntInfo->Bssid, 6);

				//Add for DTM 1.0c test.
				if(!pMgntInfo->bHiddenSSIDEnable)
					MgntRemoveSsidsToScan(pAdapter, pMgntInfo->Ssid);			

				//
				// CCW: don't indicate dissociation event on infra. mode client.
				// 070125, rcnjko: don't indicate disassociation event
				//		

				if(!bEnableWoLCapabilities ||
					(pPSC->FSSDetection && (!pPSC->bSetPMParameters || (pPSC->bSetPMParameters && !pPSC->bOSSupportProtocolOffload)))) 
				{	// Do not disasso to AP when WoWLAN. by tynli.
					if( pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE )
					{
						pNdisCommon->bDissociateBeforeSleep = TRUE;
					}
				
					MgntActSet_802_11_DISASSOCIATE(pAdapter , unspec_reason);
					pNdisCommon->bPnpKeepConnectToAP = FALSE;

					//delay 50ms to let hw send disassociation packet successful
					//fix s3 fail with Netgear3500 v1
					//by sherry 20101124
					PlatformStallExecution(50000);
				}
				else
				{ // WoWLAN
					if( pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE && pPSC->bPnpEnterD2)
						pNdisCommon->bPnpKeepConnectToAP = TRUE;
					else
						pNdisCommon->bPnpKeepConnectToAP = FALSE;
				}

				//restore SSID
				CopySsidOS(pMgntInfo->Ssid, savedssid); 
				CopyMem(pMgntInfo->Bssid, bssidbuf, 6);
			}
			else
			{
				pNdisCommon->bPnpKeepConnectToAP = FALSE;
			}

			RT_TRACE(COMP_POWER, DBG_LOUD, ("After Link Operation\n"));

			{
				PADAPTER pTargetAdapter = GetDefaultAdapter(pAdapter);

				while(pTargetAdapter != NULL)
				{
					if(MgntActQuery_ApType(pTargetAdapter) == RT_AP_TYPE_VWIFI_AP)
					{
						// stop beaconing. for NDISTest preview 3 SoftAP_Power_ext.
						PMGNT_INFO pTargetMgntInfo = &pTargetAdapter->MgntInfo;
					
						MgntStopBeacon(pTargetAdapter);

						// Configure the HW to be No Link.
						pTargetMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
						pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pTargetMgntInfo->OpMode));
						pTargetMgntInfo->OpMode = RT_OP_MODE_AP;
					}
					
					pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
				}
			}

			RT_TRACE(COMP_POWER, DBG_LOUD, ("vWifi other config\n"));
				
			pDevice->CurrentPowerState = NewPowerState;
			pAdapter->bInSetPower = TRUE;

			N6SdioWdi_PnPSetPower(pAdapter);

		}
		else
		{	
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Wdi_Set_Power_State(): Invalid Power State(%u)\n", NewPowerState));
			ndisStatus = NDIS_STATUS_INVALID_DATA;
		}
		
		// For Working Time Calculation 		
		EndTime = KeQueryPerformanceCounter(NULL);
		pAdapter->bCtrlPnPTime = FALSE;

		pAdapter->PnPTotalTime.QuadPart = (EndTime.QuadPart - StartTime.QuadPart) * 1000000;
		pAdapter->PnPTotalTime.QuadPart = pAdapter->PnPTotalTime.QuadPart / Freq.QuadPart;
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[PnP Time] Wake Up Total Cost Time: %llu us\n", pAdapter->PnPTotalTime.QuadPart));

		pAdapter->PnPIOTime.QuadPart = pAdapter->PnPIOTime.QuadPart * 1000000;
		pAdapter->PnPIOTime.QuadPart = pAdapter->PnPIOTime.QuadPart / Freq.QuadPart;
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[PnP Time] IO Cost Time: %llu us\n", pAdapter->PnPIOTime.QuadPart));
		
		pDevice->PnPSdBusWorkTime.QuadPart = pDevice->PnPSdBusWorkTime.QuadPart * 1000000;
		pDevice->PnPSdBusWorkTime.QuadPart = pDevice->PnPSdBusWorkTime.QuadPart / Freq.QuadPart;
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[PnP Time] SdBus Submit Cost Time: %llu us\n", pDevice->PnPSdBusWorkTime.QuadPart));
		//
	}
	else
	{
		RT_TRACE(COMP_POWER, DBG_WARNING, ("Wdi_Set_Power_State(): failed to ParseWdiSetPowerState!\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
	}

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample Set Power",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingHexUInt32(ndisStatus, "ndisStatus"));
#endif

	RT_TRACE(COMP_POWER, DBG_LOUD, ("[WDI], Set OID_PNP_SET_POWER()return status:0x%x\n", ndisStatus));

	return ndisStatus;
}

