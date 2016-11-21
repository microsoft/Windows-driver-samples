#ifndef __INC_MGNTACTSETPARAM_H
#define __INC_MGNTACTSETPARAM_H

#define ADD_KEY_IDX_AS	0x10000000
#define ADD_KEY_IDX_RSC	0x20000000


//
// For switching AutoTurbo by 8186 or our original turbo mode mechanism. Added by Roger, 2006.12.07.
// Note: bit0 and bit1 should NOT both be 1 at the same time.
//

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef	union _TURBOMODE_TYPE{
	u1Byte	charData;		
	struct
	{
		u1Byte		SupportTurboMode:1;
		u1Byte		AutoTurboBy8186:1;
		u1Byte		Reserved:6;
	}field;
}TURBOMODE_TYPE, *PTURBOMODE_TYPE;

typedef enum _HT_CAP_OP_CODE{
	//------------------------------------------// sigma spec
	HT_CAP_OP_RESET_DEFAULT = 0,
	HT_CAP_OP_40_INTOLERANT = 1,
	HT_CAP_OP_ADDBA_REJECT = 2,
	HT_CAP_OP_AMPDU = 3,
	HT_CAP_OP_AMSDU = 4,
	HT_CAP_OP_GREENFIELD = 5,
	HT_CAP_OP_SGI20 = 6,
	HT_CAP_OP_STBC = 7,
	HT_CAP_OP_WIDTH = 8,
	HT_CAP_OP_MCS_FIXEDRATE = 9,
	HT_CAP_OP_MCS32 = 10,
	HT_CAP_OP_SMPS = 11,
	HT_CAP_OP_VHT_TXSP_STREAM = 12,
	HT_CAP_OP_VHT_RXSP_STREAM = 13,
	HT_CAP_OP_BAND = 14,
	HT_CAP_OP_DYN_BW_SGNL = 15,
	HT_CAP_OP_SGI80 = 16,
	HT_CAP_OP_TXBF = 17,
	HT_CAP_OP_LDPC = 18,
	HT_CAP_OP_OPT_MD_NOTIF_IE = 19,
	HT_CAP_OP_NSS_MCS_CAP = 20,
	HT_CAP_OP_TX_LGI_RATE = 21,
	HT_CAP_OP_ZERO_CRC = 22,
	HT_CAP_OP_VHT_TKIP = 23,
	HT_CAP_OP_VHT_WEP = 24,
	HT_CAP_OP_BW_SGNL = 25,
	HT_CAP_OP_CTS_ENABLE = 26,
	//------------------------------------------// realtek custom
	HT_CAP_OP_SEND_ADDBA = 0x81,		
	HT_CAP_OP_RTS_THRESHOLD = 0x82,
	HT_CAP_OP_FRAGMENT = 0x83,
	HT_CAP_OP_PREAMBLE = 0x84,
	HT_CAP_OP_WIRELESS_MODE = 0x85,
	HT_CAP_OP_POWERSAVE = 0x86,
	HT_CAP_OP_NOACK = 0x87,
	HT_CAP_OP_OPMODE_NOTIF = 0x88,
	HT_CAP_OP_BW_SIGNALING_CTRL = 0x89,
	HT_CAP_OP_HT_TXSP_STREAM = 0x8b,
	HT_CAP_OP_HT_RXSP_STREAM = 0x8c,
	HT_CAP_OP_BEAMFORMING_CAP = 0x8d,
	HT_CAP_OP_RESET_OPMODE = 0x8e,
}HT_CAP_OP_CODE, *PHT_CAP_OP_CODE;

#define	BENoAck		BIT0
#define	BKNoAck		BIT1
#define	VINoAck		BIT2
#define	VONoAck	BIT3

#ifdef REMOVE_PACK
#pragma pack()
#endif


//------------------------------------------
//Name : MgntActSet_RSNA_REMOVE_DEAULT_KEY
//Function : Remove Key from PerStable
//Input : Keyindex = Perstation DefKeybuf index.
//           MacAddress = Per-station's MAC address 
//------------------------------------------
BOOLEAN
MgntActSet_RSNA_REMOVE_DEAULT_KEY(
	PADAPTER		Adapter,
	u4Byte			KeyIndex,
	pu1Byte			MacAddress
);

//------------------------------------------
//Name : MgntActSet_RSNA_ADD_DEAULT_KEY
//Function : Remove Key from PerStable
//Input : Keyindex = Perstation DefKeybuf index.
//           pKeyMaterial = conent of key
//           MacAddress = Per-station's MAC address 
//------------------------------------------
//vivi added for new cam search flow, 20091028
BOOLEAN
MgntActSet_RSNA_ADD_DEAULT_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLen,
	pu1Byte               pKeyMaterial,
	pu1Byte		      MacAddress
);
//------------------------------------------
//Name : MgntActSet_RSNA_REMOVE_MAPPING_KEY
//Function : Remove Key from PerStable
//Input : 
//           MacAddress = Per-station's MAC address 
//------------------------------------------
BOOLEAN
MgntActSet_RSNA_REMOVE_MAPPING_KEY(
	PADAPTER		Adapter,
	pu1Byte			MacAddress
);

//------------------------------------------
//Name : MgntActSet_RSNA_ADD_MAPPING_KEY
//Function : Remove Key from PerStable
//Input : pKeyMaterial = conent of key
//           MacAddress = Per-station's MAC address 
//------------------------------------------
//vivi added for new cam search flow, 20091028
BOOLEAN
MgntActSet_RSNA_ADD_MAPPING_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte		       KeyLen,
	pu1Byte               pKeyMaterial,
	pu1Byte			MacAddress
);

BOOLEAN
MgntActSet_802_11_REMOVE_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	pu1Byte			BSSID,
	BOOLEAN			IsGroup
);
BOOLEAN
MgntActSet_802_11_ADD_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLength,
	pu1Byte			KeyMaterial,
	pu1Byte			MacAddress,
	BOOLEAN			IsGroupTransmitKey,
	BOOLEAN			IsGroup,
	u8Byte			KeyRSC
);


BOOLEAN
MgntActSet_802_3_MAC_ADDRESS(
	PADAPTER		Adapter,
	pu1Byte          	addrbuf
);


//Sets the MAC address of the desired AP.
BOOLEAN
MgntActSet_802_11_BSSID(
	PADAPTER		Adapter,
	pu1Byte          	bssidbuf
);





// Sets the SSID to a specified value
// Select a Basic Service Set to join?
BOOLEAN
MgntActSet_802_11_SSID(
	PADAPTER		Adapter,
	pu1Byte          ssidbuf,
	u2Byte	          ssidlen,
	BOOLEAN			JoinAfterSetSSID
);



BOOLEAN
MgntActSet_802_11_INFRASTRUCTURE_MODE(
	PADAPTER			Adapter,
	RT_JOIN_NETWORKTYPE networktype
);



BOOLEAN
MgntActSet_802_11_ADD_WEP(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLength,
	pu1Byte			KeyMaterial,
	BOOLEAN			IsDefaultKeyId
);




BOOLEAN
MgntActSet_802_11_REMOVE_WEP(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLength
);



BOOLEAN
MgntActSet_802_11_TX_POWER_LEVEL(
	PADAPTER		Adapter,
	u4Byte	          	powerlevel
);




// Sets the fragmentation threshold in bytes.
BOOLEAN
MgntActSet_802_11_FRAGMENTATION_THRESHOLD(
	PADAPTER		Adapter,
	u2Byte	          	fragthres
);


//Sets the RTS threshold.
BOOLEAN
MgntActSet_802_11_RTS_THRESHOLD(
	PADAPTER		Adapter,
	u2Byte	          	RtsThres
);

BOOLEAN
MgntActSet_802_11_RATE(
	PADAPTER		Adapter,
	u1Byte	          	rate			// 0x02 := 1Mbps, ....
);

NDIS_STATUS
MgntActSet_802_11_DISASSOCIATE(
	PADAPTER		Adapter,
	u1Byte			asRsn
);

BOOLEAN
MgntActSet_802_11_DEAUTHENTICATION(
	PADAPTER		Adapter,
	u1Byte			asRsn
);

BOOLEAN
MgntActSet_802_11_AUTHENTICATION_MODE(
	PADAPTER		Adapter,
	RT_AUTH_MODE	authmode
);



BOOLEAN
MgntActSet_802_11_BSSID_LIST_SCAN(
	PADAPTER		Adapter
);


BOOLEAN
MgntActSet_802_11_WIRELESS_MODE(
	PADAPTER		Adapter,
	WIRELESS_MODE  	WirelessMode
);

BOOLEAN
MgntActSet_802_11_RETRY_LIMIT(
	PADAPTER Adapter, 
	u2Byte ShortRetryLimit, 
	u2Byte LongRetryLimit
);

// Use this interface to swith channel and bandwidth in MAC layer --------------
VOID
MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
	PADAPTER			pAdapter, 
	u1Byte 				Primary20MhzChannel,
	CHANNEL_WIDTH		BandWidthMode,
	EXTCHNL_OFFSET		ExtChnlOffsetOf40MHz,
	EXTCHNL_OFFSET		ExtChnlOffsetOf80MHz,
	u1Byte				Secondary80MhzChannelCenterFrequency
);
//-------------------------------------------------------------------

// Use this interface to change Reg 20Mhz channel ------------------------------
BOOLEAN
MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(
	PADAPTER	Adapter, 
	u1Byte 		channel
);
// ----------------------------------------------------------------------

// Annie, 2004-12-27
BOOLEAN
MgntActSet_802_11_CHANNELPLAN(
	PADAPTER	Adapter, 
	u2Byte		ChannelPlan
);

BOOLEAN
MgntActSet_802_11_PMKID(
	IN	PADAPTER     		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	ULONG			InformationBufferLength
);


VOID
MgntActSet_802_3_MULTICAST_LIST(
	PADAPTER		Adapter,
	pu1Byte			MCListbuf,
	u4Byte			MCListlen,
	BOOLEAN			bAcceptAllMulticast
);

VOID
MgntActSet_802_11_PREAMBLE_MODE(
	PADAPTER		Adapter,
	u1Byte	PreambleMode
);

BOOLEAN
MgntActSet_802_11_CONFIGURATION(
	PADAPTER		Adapter,
	u2Byte			BeaconPeriod,
	u1Byte			ChannelNumber
);

BOOLEAN 
MgntActSet_802_11_PowerSaveMode(
	PADAPTER		Adapter,
	RT_PS_MODE		rtPsMode
);

BOOLEAN
MgntActSet_ApMode(
	PADAPTER		Adapter,
	BOOLEAN			bApMode
);

VOID
MgntActSet_802_11_DtimPeriod(
	PADAPTER		Adapter,
	u1Byte			u1DtimPeriod
);

VOID
MgntActSet_802_11_BeaconInterval(
	PADAPTER		Adapter,
	u2Byte			u2BeaconPeriod
);

VOID 
MgntActSet_802_11_ScanWithMagicPacket(
	PADAPTER		Adapter,
	pu1Byte			pDstAddr
);

VOID
MgntActSet_Passphrase(
	PADAPTER		Adapter,
	POCTET_STRING	posPassphrase
);


BOOLEAN
MgntActSet_ExcludedMacAddressList(
	PADAPTER		Adapter,
	pu1Byte			pMacAddrList,
	u4Byte			NumMacAddrList
);


// For turbo mode related, added by Roger. 2006.12.07.
BOOLEAN
MgntActSet_RT_TurboModeType(
	IN	PADAPTER			Adapter,
	IN	pu1Byte			pTurboModeType
);

// For turbo mode related, added by Roger. 2006.12.07.
VOID
MgntActSet_EnterTurboMode(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bEnterTurboMode
);

BOOLEAN
MgntActSet_Locked_STA_Address(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			LockedListBuf,
	IN	u4Byte			LockedListCnt
);


BOOLEAN
MgntActSet_Accepted_STA_Address(
	IN	PADAPTER		Adapter,
	IN	pu1Byte		LockedListBuf,
	IN	u4Byte		LockedListCnt
);

BOOLEAN
MgntActSet_Rejected_STA_Address(
	IN	PADAPTER		Adapter,
	IN	pu1Byte		LockedListBuf,
	IN	u4Byte		LockedListCnt
);


BOOLEAN
MgntActSet_RF_State(
	IN	PADAPTER			Adapter,
	IN	RT_RF_POWER_STATE	StateToSet,
	IN	RT_RF_CHANGE_SOURCE	ChangeSource,
	IN   BOOLEAN				FromGPIO
);

VOID
MgntActSet_TX_POWER_LEVEL(
	PADAPTER		Adapter,
	int				TxPowerLevel
	);

BOOLEAN
MgntActSet_TX_POWER_DBM(
	PADAPTER		Adapter,
	s4Byte			powerInDbm
);

VOID
UpdateTxPowerDbmWorkItemCallback(
	IN PVOID		pContext
);

RT_STATUS
MgntActSet_AdditionalProbeReqIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
);

RT_STATUS
MgntActSet_AdditionalBeaconIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
);

RT_STATUS
MgntActSet_AdditionalProbeRspIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
);

RT_STATUS
MgntActSet_AdditionalAssocReqIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
);

VOID
MgntActSet_ApType(
	IN PADAPTER pAdapter,
	IN BOOLEAN mActingAsAp
);

RT_NDIS_VERSION
MgntTranslateNdisVersionToRtNdisVersion(
	IN UINT NdisVersion
);

VOID
MgntActSet_802_11_WMM_MODE(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bWMMEnable
	);

VOID
MgntActSet_802_11_WMM_UAPSD(
	IN	PADAPTER		Adapter,
	IN	AC_UAPSD		WMM_UAPSD
	);

BOOLEAN
MgntActSet_802_11_CustomizedAsocIE(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u1Byte			InformationBufferLength
	);

BOOLEAN
MgntActSet_802_11_Sigma_Capability(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u1Byte			InformationBufferLength
	);

RT_STATUS
MgntActSet_802_11_Beamforming_MODE(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	);

RT_STATUS
MgntActSet_802_11_LDPC_MODE(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	);

RT_STATUS
MgntActSet_802_11_STBC_MODE(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	);

VOID
MgntActSet_S5_WAKEUP_INFO(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
);

#if (P2P_SUPPORT == 1)	
VOID
MgntActSet_P2PMode(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN		bP2PMode,
	IN 	BOOLEAN		bGO,
	IN	u1Byte			ListenChannel,
	IN	u1Byte			IntendedOpChannel,
	IN	u1Byte			GOIntent
	);

VOID
MgntActSet_P2PProvisionIE(
	IN	PADAPTER		Adapter,
	IN	pu1Byte 			Information, // along with OUI, no id and length
	IN	u2Byte		 	InformationLen
	);

VOID
MgntActSet_P2PFlushScanList(
	IN	PADAPTER		Adapter
	);

VOID
MgntActSet_P2PProvisioningResult(
	IN	PADAPTER		Adapter,
	IN	P2P_PROVISIONING_RESULT ProvisioningResult
	);

VOID
MgntActSet_P2PChannelList(
	IN	PADAPTER Adapter,
	IN	u1Byte ChannelListLen,
	IN	pu1Byte pChannelList
	);

VOID
MgntActSet_P2PListenChannel(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ListenChannel
	);
#endif	// #if (P2P_SUPPORT == 1)	

#if ( WPS_SUPPORT == 1 )
RT_STATUS
MgntActSet_WPS_Information(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u2Byte			InformationBufferLength
	);
#endif 

#endif // #ifndef __INC_MGNTACTSETPARAM_H

