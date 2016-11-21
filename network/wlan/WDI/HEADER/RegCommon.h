/******************************************************************************

     (c) Copyright 2007, RealTEK Technologies Inc. All Rights Reserved.

 Module:	RegCommon.h		

 Note:      The file define the initiali registry value cross the following platform
		  (1) NDIS5
		  (2) NDIS6
		  (3) MAC OS
 
 Function:
 		 
 Export:

 Abbrev:

 History:
	Data			Who		Remark
	
	05/0r/2007  	Emily    	Create initial version.

	
******************************************************************************/

#ifndef __INC_REGCOMMON_H
#define __INC_REGCOMMON_H

//-----------------------------------------------------------------------------
// Common definition for all of the platform
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Seperate definition for different platform
//-----------------------------------------------------------------------------
	//Windows Platform
	NDIS_STRING		DefaultSSID = NDIS_STRING_CONST("");
	NDIS_STRING		DefaultKey = NDIS_STRING_CONST("");
	NDIS_STRING		DefaultDeviceDesc = NDIS_STRING_CONST("Realtek Wireless LAN");
 	NDIS_STRING		DefaultPairingKey = NDIS_STRING_CONST("12345678");
	NDIS_STRING		DefaultFileName = NDIS_STRING_CONST("");
	NDIS_STRING 	gChannelPlan2G  = NDIS_STRING_CONST("1,2,3,4,5,6,7,8,9,10,11,12,13,14");
	NDIS_STRING 	gChannelPlan5G = NDIS_STRING_CONST("36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165");	
	#define REGISTRY_STR					NDIS_STRING_CONST

	#define REGISTRY_OFFSET(field)		((u4Byte)FIELD_OFFSET(RT_NDIS_COMMON,field))
	#define REGISTRY_SIZE(field)			sizeof(((PRT_NDIS_COMMON)0)->field)
	


static MP_REG_ENTRY CommonRegTable[] = {
	// Registry value name						Type					Offset in MP_ADAPTER						Field size							Default Value		Min		Max
	{REGISTRY_STR("NetworkAddress"),	0,NdisParameterInteger,	REGISTRY_OFFSET(NetworkAddress),			REGISTRY_SIZE(NetworkAddress),		0xFF,				0,		0xFF},
	{REGISTRY_STR("SSID"),				0,NdisParameterString,		REGISTRY_OFFSET(RegSSID),				REGISTRY_SIZE(RegSSID),				(ADDR2DATA)&DefaultSSID,0,	32},
	{REGISTRY_STR("Bssid"),				0,NdisParameterString,		REGISTRY_OFFSET(RegBssidBuf),			REGISTRY_SIZE(RegBssidBuf),			0, 					0,		13},	
	{REGISTRY_STR("Locale"), 			0,NdisParameterString,		REGISTRY_OFFSET(RegLocale),				REGISTRY_SIZE(RegLocale), 				(ADDR2DATA)&DefaultFileName,	0,		4},
	{REGISTRY_STR("bConcurrentMode"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegConcurrent),		REGISTRY_SIZE(bRegConcurrent),		0,					0,		1},
	//----------------------------------------------------------------------------
	// WiFi config, by Bruce, 2007-12-07.
	{REGISTRY_STR("WiFiConfg"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegWiFi),	REGISTRY_SIZE(bRegWiFi),	0,      0,       1},  // 0: Disable WiFi Config, 1: Enable WiFi Config
	//----------------------------------------------------------------------------
	{REGISTRY_STR("ChannelPlan"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegChannelPlan),		REGISTRY_SIZE(RegChannelPlan),		0x7f,					0,		0x7f},
	{REGISTRY_STR("ChannelPlan2G"),			0,NdisParameterString,  REGISTRY_OFFSET(RegChannelPlan2G), 		REGISTRY_SIZE(RegChannelPlan2G),	(ADDR2DATA)&gChannelPlan2G,					0,		255},
	{REGISTRY_STR("ChannelPlan5G"),			0,NdisParameterString,  REGISTRY_OFFSET(RegChannelPlan5G), 		REGISTRY_SIZE(RegChannelPlan5G),	(ADDR2DATA)&gChannelPlan5G,					0,		255},		
	{REGISTRY_STR("WirelessMode"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegWirelessMode),		REGISTRY_SIZE(RegWirelessMode),		WIRELESS_MODE_AUTO,      WIRELESS_MODE_A,       WIRELESS_MODE_MAX},
	{REGISTRY_STR("HTMode"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegHTMode),				REGISTRY_SIZE(RegHTMode),		0,      0,       4},
 	{REGISTRY_STR("Channel"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegChannel),			REGISTRY_SIZE(RegChannel),			1,      1,       200},
	{REGISTRY_STR("Channel5G"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegChannel5G),			REGISTRY_SIZE(RegChannel5G),			36,      36,       200},
	{REGISTRY_STR("RFType"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegRFType),				REGISTRY_SIZE(RegRFType),		15,					0,		15}, // RF_1T2R = 0, RF_MAX_TYPE = 15
	//----------------------------------------------------------------------------
	// Efuse value get from registry
	{REGISTRY_STR("bEfusePriorityAuto"),	0,NdisParameterInteger, REGISTRY_OFFSET(bEfusePriorityAuto),	REGISTRY_SIZE(bEfusePriorityAuto),  1, 					0,		1},		//	0: Registry-Priority Auto 1: EFUSE-Priority Auto
   	{REGISTRY_STR("AntennaDivType"),	    0,NdisParameterInteger,	REGISTRY_OFFSET(AntDivType),			REGISTRY_SIZE(AntDivType),		    2,					1,		3}, 	// 1: for 88EE, 1Tx and 1RxCG are diversity.(2 Ant with SPDT), 2:  for 88EE, 1Tx and 2Rx are diversity.( 2 Ant, Tx and RxCG are both on aux port, RxCS is on main port ), 3: for 88EE, 1Tx and 1RxCG are fixed.(1Ant, Tx and RxCG are both on aux port)
	{REGISTRY_STR("AmplifierType2G"),	    0,NdisParameterInteger,	REGISTRY_OFFSET(AmplifierType_2G),		REGISTRY_SIZE(AmplifierType_2G),	0,					0,		24}, 
	{REGISTRY_STR("AmplifierType5G"),	    0,NdisParameterInteger,	REGISTRY_OFFSET(AmplifierType_5G),		REGISTRY_SIZE(AmplifierType_5G),	0,					0,		192},
	{REGISTRY_STR("TxBBSwing2G"),	    	0,NdisParameterInteger,	REGISTRY_OFFSET(TxBBSwing_2G),			REGISTRY_SIZE(TxBBSwing_2G),		0,					0,		9}, 		
	{REGISTRY_STR("TxBBSwing5G"),	    	0,NdisParameterInteger,	REGISTRY_OFFSET(TxBBSwing_5G),			REGISTRY_SIZE(TxBBSwing_5G),	    0,					0,		9},
	{REGISTRY_STR("bTurnOffEfuseMask"),	    0,NdisParameterInteger,	REGISTRY_OFFSET(bTurnOffEfuseMask),		REGISTRY_SIZE(bTurnOffEfuseMask),	0,					0,		1},		//	0: EFUSE Mask ON  1: EFUSE Mask OFF		
	{REGISTRY_STR("RFEType"),	    		0,NdisParameterInteger,	REGISTRY_OFFSET(RFE_Type),				REGISTRY_SIZE(RFE_Type),			64,					0,		64},		//	0-4: User define RFE Type  64: Efuse define RFE Type
	{REGISTRY_STR("bTFBGA"),				0,NdisParameterInteger, REGISTRY_OFFSET(bTFBGA),				REGISTRY_SIZE(bTFBGA),				0, 					0,		1},		//	0: Others(e.g., QFN) 1: TFBGA	

	//----------------------------------------------------------------------------
	// For Sercomm UI, 2005.01.13, by rcnjko.
 	{REGISTRY_STR("ForcedDataRate"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegForcedDataRate),		REGISTRY_SIZE(RegForcedDataRate),	0,      0,       0xc7}, // 0: Auto, (0x02: 1M ~ 0x6C: 54M) , (0x80: MCS0 ~ 0x8f: MCS15)
 	{REGISTRY_STR("ForcedBstDataRate"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegForcedBstDataRate),	REGISTRY_SIZE(RegForcedBstDataRate),	0,      0,       0xc7}, // 0: Auto, (0x02: 1M ~ 0x6C: 54M)

 	{REGISTRY_STR("WirelessMode4ScanList"),0,NdisParameterInteger,		REGISTRY_OFFSET(RegWirelessMode4ScanList),REGISTRY_SIZE(RegWirelessMode4ScanList),	WIRELESS_MODE_AUTO,      WIRELESS_MODE_A,       WIRELESS_MODE_N_5G},
	//----------------------------------------------------------------------------
	// For 818x UI and WPA Verify, 2004.11.30, by rcnjko.
	{REGISTRY_STR("DataEncAlg"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegWepEncStatus),		REGISTRY_SIZE(RegWepEncStatus),		REG_WEP_STATUS_NO_WEP,          REG_WEP_STATUS_NO_WEP,  REG_WEP_STATUS_WEP128},
	{REGISTRY_STR("EncAlgorithm"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegEncAlgorithm),		REGISTRY_SIZE(RegEncAlgorithm),		REG_WEP_Encryption,       REG_WEP_Encryption,  	REG_AESCCMP_Encryption},
    	{REGISTRY_STR("AuthentAlg"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegAuthentAlg),			REGISTRY_SIZE(RegAuthentAlg),		Ndis802_11AuthModeOpen, Ndis802_11AuthModeOpen,  Ndis802_11AuthModeAutoSwitch},	 // <NOTE> For Meeting House, authentication mode is required to be Open.
    	{REGISTRY_STR("DefaultKey0"),			0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKey[0]),		REGISTRY_SIZE(RegDefaultKey[0]),		(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKey1"),			0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKey[1]),		REGISTRY_SIZE(RegDefaultKey[1]),		(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKey2"),			0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKey[2]),		REGISTRY_SIZE(RegDefaultKey[2]),		(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKey3"),			0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKey[3]),		REGISTRY_SIZE(RegDefaultKey[3]),		(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKeyW0"),		0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKeyW[0]),		REGISTRY_SIZE(RegDefaultKeyW[0]),	(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKeyW1"),		0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKeyW[1]),		REGISTRY_SIZE(RegDefaultKeyW[1]),	(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKeyW2"),		0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKeyW[2]),		REGISTRY_SIZE(RegDefaultKeyW[2]),	(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKeyW3"),		0,NdisParameterString,		REGISTRY_OFFSET(RegDefaultKeyW[3]),		REGISTRY_SIZE(RegDefaultKeyW[3]),	(ADDR2DATA)&DefaultKey,    	0,              61},
    	{REGISTRY_STR("DefaultKeyId"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegDefaultKeyId),		REGISTRY_SIZE(RegDefaultKeyId),		0,                      0,              3},
    	{REGISTRY_STR("NetworkType"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegNetworkType),		REGISTRY_SIZE(RegNetworkType),		NI_Infrastructure,      NI_ADHOC,       NI_AUTO},
	//----------------------------------------------------------------------------
	// For pre-authentication default setting. By Annie. 
	{REGISTRY_STR("EnablePreAuth"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegEnablePreAuthentication),REGISTRY_SIZE(RegEnablePreAuthentication),	1,      0,       1},
	//----------------------------------------------------------------------------
	// For power save mode. 2005.02.15, by rcnjko.
    	{REGISTRY_STR("PowerSaveMode"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegPowerSaveMode),		REGISTRY_SIZE(RegPowerSaveMode),	0,      0,       2}, // 0: Ndis802_11PowerModeCAM, 1: Ndis802_11PowerModeMAX_PSP MAX_PS 2: Ndis802_11PowerModeFast_PSP.  
	//----------------------------------------------------------------------------
	// IBSS beacon behavior. 2005.12.16, by rcnjko.
	{REGISTRY_STR("IbssBeacon"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegEnableSwBeacon),		REGISTRY_SIZE(RegEnableSwBeacon),	0,	0,		2}, // 0: BEACON_SEND_AUTO_HW, 1: BEACON_SEND_AUTO_SW, 2: BEACON_SEND_MANUAL
	//----------------------------------------------------------------------------
	// For SW AP mode. 2005.05.30, by rcnjko.
	{REGISTRY_STR("ActingAsAp"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegActingAsAp),			REGISTRY_SIZE(RegActingAsAp),		FALSE,		FALSE,			TRUE},
	{REGISTRY_STR("AH_BcnIntv"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegBeaconPeriod),		REGISTRY_SIZE(RegBeaconPeriod),				100,					20,		1000},
	{REGISTRY_STR("DtimPeriod"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegDtimPeriod),			REGISTRY_SIZE(RegDtimPeriod),			1,		1,			255},
	{REGISTRY_STR("PreambleMode"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegPreambleMode),		REGISTRY_SIZE(RegPreambleMode),		2,		1,			3}, // 1:Long, 2:Auto, 3:Short
	// For intel class mode, 2011.02.09, by lanhsin
	//----------------------------------------------------------------------------
	{REGISTRY_STR("ActingAsClassMode"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegClassMode),		REGISTRY_SIZE(bRegClassMode),		FALSE,		FALSE,			TRUE},
	//----------------------------------------------------------------------------
	// For RF Power State. 2005.05.30, by rcnjko.
	{REGISTRY_STR("RFOff"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegRfOff),				REGISTRY_SIZE(RegRfOff),				0,		0,			1}, // 1: RF off, 0: RF on.
	//----------------------------------------------------------------------------
	// for WPP debug monitor, 20140509, by Cosa
	{REGISTRY_STR("DbgMonitor"), 			0,NdisParameterInteger, REGISTRY_OFFSET(RegDbgMonitor),				REGISTRY_SIZE(RegDbgMonitor),				0,		0,			0xffffffff}, // debug monitor
	// For QoS. Added by Annie, 2005-11-09.
	{REGISTRY_STR("QoS"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegSupportQoS),		REGISTRY_SIZE(bRegSupportQoS),		1,		0,			2}, // 1: Support QoS, 0: not support. // 2: Fake aut mode for customer temporarily~~!!
	{REGISTRY_STR("StaUapsd"),			0,NdisParameterInteger,	REGISTRY_OFFSET(StaUapsd),				REGISTRY_SIZE(StaUapsd),			0,					0,		15},  //default 0 : Disable UAPSD
	{REGISTRY_STR("MaxSPLength"),			0,NdisParameterInteger,	REGISTRY_OFFSET(MaxSPLength),			REGISTRY_SIZE(MaxSPLength),			0,					0,		3},  //default 0
	//----------------------------------------------------------------------------
	// For CCX RM, 2006.05.15, by rcnjko.
	{REGISTRY_STR("CcxEnable"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegCcx),				REGISTRY_SIZE(RegCcx),			0,					0,		1}, // 1: Support CCX, 0: not support it. 
	{REGISTRY_STR("CcxRm"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegCcxRm),				REGISTRY_SIZE(RegCcxRm),			0,					0,		1}, // 1: Support CCX Radio Measurement, 0: not support it.
	{REGISTRY_STR("CcxOffLineDurUpLimit"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegCcxOffLineDurUpLimit),	REGISTRY_SIZE(RegCcxOffLineDurUpLimit),0,					0,		0xffff}, // 0: Unlimited >0: Max non-serving channel measurement duration (in TU) 
	{REGISTRY_STR("CcxUpdateTxCellPwr"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegCcxUpdateTxCellPwr),	REGISTRY_SIZE(RegCcxUpdateTxCellPwr),0,					0,		1}, // 0: Disable, 1: Enable
	//----------------------------------------------------------------------------
	// For CCX CAC
	{REGISTRY_STR("CcxCAC"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegCcxCACEnable),			REGISTRY_SIZE(RegCcxCACEnable),					0,							0,				1}, // 1: Support CCX CAC, 0: not support it.
	// For CCX CatastrophicRoamLimit
	{REGISTRY_STR("CcxCatastrophicRoamLimit"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegCcxCatasRoamLimit),			REGISTRY_SIZE(RegCcxCatasRoamLimit),					0,							0,				0xFF}, // 0: Disable
	//----------------------------------------------------------------------------
	{REGISTRY_STR("bHwParaFile"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegHwParaFile),			REGISTRY_SIZE(bRegHwParaFile),		1,					0,		2}, // 0: (Default) use parameters hardcoded, 1: read from file, 2: load default first, and then partial modify from file.
	//----------------------------------------------------------------------------
	
	// For TX Power Level asked by Lenovo, 2009.03.06 by Mars
	{REGISTRY_STR("TxPowerLevel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegTxPowerLevel),		REGISTRY_SIZE(RegTxPowerLevel),	0,					0,		4},		// 0:Default 1:25% 2:50% 3:75% 4:100%
	//----------------------------------------------------------------------------
	// For case of Out of RFD, 2006.07.25 by shien chang
	{REGISTRY_STR("LowRfdThreshold"),		0,NdisParameterInteger,	REGISTRY_OFFSET(LowRfdThreshold),		REGISTRY_SIZE(LowRfdThreshold),		4,					1,		64},
	//----------------------------------------------------------------------------
	// PSPXlinkMode, enable promicuous mode for XLink protocol. 2006.09.04, by shien chang
	{REGISTRY_STR("PSPXlinkMode"),		0,NdisParameterInteger,	REGISTRY_OFFSET(PSPXlinkMode),			REGISTRY_SIZE(PSPXlinkMode),			0,                   0,              1},
	//----------------------------------------------------------------------------
	// For "SW Tx Encrypt" and "SW Rx Decrypt". 0: Auto.  1: HW enc/dec.  2:SW enc/dec.
	// 2006.09.29, by shien chang.
	{REGISTRY_STR("RegSWTxEncryptFlag"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegSWTxEncryptFlag),	REGISTRY_SIZE(RegSWTxEncryptFlag),	0,                   0,              2},
	{REGISTRY_STR("RegSWRxDecryptFlag"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegSWRxDecryptFlag),	REGISTRY_SIZE(RegSWRxDecryptFlag),	0,                   0,              2},
	//----------------------------------------------------------------------------
	// For HCT test. 061010, by rcnjko.
	{REGISTRY_STR("HctTest"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegHctTest),			REGISTRY_SIZE(bRegHctTest),			0,                   0,              1},
	{REGISTRY_STR("Chaos"),					0,NdisParameterInteger,	REGISTRY_OFFSET(bRegChaos),			REGISTRY_SIZE(bRegChaos),			0,                   0,              1},	

	// For OID_GEN_VENDOR_DESCRIPTION, 2006.03.10, by rcnjko.
	{REGISTRY_STR("DriverDesc"),			0,NdisParameterString,		REGISTRY_OFFSET(RegDriverDesc),			REGISTRY_SIZE(RegDriverDesc),			(ADDR2DATA)&DefaultDeviceDesc,      0,       255},
	//----------------------------------------------------------------------------
	// For EQC, Fixed MAC Address requirement
	{REGISTRY_STR("FixedMacAddrForEQC"),	0,NdisParameterInteger, REGISTRY_OFFSET(bRegFixedMacAddr), 		REGISTRY_SIZE(bRegFixedMacAddr),		0,		0,		 1},  // 0: disable, 1: enable
	//----------------------------------------------------------------------------
	// For turbo mode. Roger, 2006.12.07.
	{REGISTRY_STR("TurboMode"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegTurboModeSelect),	REGISTRY_SIZE(RegTurboModeSelect),	2,      0,       2},	// 0: Disable, 1: Enable, 2: Auto Turbo by 8186 (following 8186 beacon).
	//----------------------------------------------------------------------------
	// 070208, rcnjko: 802.11d
	{REGISTRY_STR("Dot11dEnable"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegDot11dEnable),		REGISTRY_SIZE(bRegDot11dEnable),		0,					0,		1}, // 0: disable, 1: enable. 
	//----------------------------------------------------------------------------
	// For AP mode auto select channel. 2005.12.27, by rcnjko.
	{REGISTRY_STR("AutoSelChnl"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegAutoSelChnl),			REGISTRY_SIZE(RegAutoSelChnl),		0,      0,       1},	// Default: Disable.
	{REGISTRY_STR("ChnlWeight"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegChnlWeight),			REGISTRY_SIZE(RegChnlWeight),		0,      0,       10},	// Current only 5 modes, Ref: AutoSelectChannel(). Added by Annie, 2006-07-26.
	//----------------------------------------------------------------------------
	// For ASUS request. Annie, 2006-02-16.
	{REGISTRY_STR("LiveTime"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegLiveTime),			REGISTRY_SIZE(RegLiveTime),			1200,			0,		2147483647},		// in second. (Set "600" = 10 minutes, "86400" = 24hr.)
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	// WDS, 2006.06.11, by rcnjko. 
	{REGISTRY_STR("WdsMode"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegWdsMode),			REGISTRY_SIZE(RegWdsMode),			0,                   0,              1}, // 0: WDS disabled, 1: AP+WDS 
	//----------------------------------------------------------------------------
	// CustomerID, see RT_CUSTOMER_ID for its definition. 2006.07.03, by rcnjko. 
	{REGISTRY_STR("CustomerID"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegCustomerID),			REGISTRY_SIZE(RegCustomerID),		0,                   0,              0x7FFFFFFF}, // 0: allow EEPROM to determine pMgntInfo->CustomerID , >0: overwrite pMgntInfo->CustomerID. 
	//fragmentation threshold 2007.03.05 by David
	{REGISTRY_STR("FragThresh"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegFragThreshold),		REGISTRY_SIZE(RegFragThreshold),		2347,		256,			2347}, // 	
	//----------------------------------------------------------------------------
	// Default EDCA parameter for AP mode
	{REGISTRY_STR("ApEDCAParamBE"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegApEDCAParamBE),		REGISTRY_SIZE(RegApEDCAParamBE),	0x00006403,			0,		0xffffffff}, 
	{REGISTRY_STR("ApEDCAParamBK"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegApEDCAParamBK),		REGISTRY_SIZE(RegApEDCAParamBK),	0x0000A427,			0,		0xffffffff},  
	{REGISTRY_STR("ApEDCAParamVI"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegApEDCAParamVI),		REGISTRY_SIZE(RegApEDCAParamVI),	0x005E4341,			0,		0xffffffff},  
	{REGISTRY_STR("ApEDCAParamVO"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegApEDCAParamVO),		REGISTRY_SIZE(RegApEDCAParamVO),	0x002F3261,			0,		0xffffffff},  
	//----------------------------------------------------------------------------
	// Default EDCA parameter for STA mode

	{REGISTRY_STR("StaEDCAParamBE"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegStaEDCAParamBE),	REGISTRY_SIZE(RegStaEDCAParamBE),	0x0000A403,			0,		0xffffffff}, 

	{REGISTRY_STR("StaEDCAParamBK"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegStaEDCAParamBK),	REGISTRY_SIZE(RegStaEDCAParamBK),	0x0000A427,			0,		0xffffffff},  
	{REGISTRY_STR("StaEDCAParamVI"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegStaEDCAParamVI),		REGISTRY_SIZE(RegStaEDCAParamVI),	0x005E4342,			0,		0xffffffff},  

	{REGISTRY_STR("StaEDCAParamVO"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegStaEDCAParamVO),	REGISTRY_SIZE(RegStaEDCAParamVO),	0x002F3262,			0,		0xffffffff},  

	//----------------------------------------------------------------------------
	// No Ack Setting
	{REGISTRY_STR("AcNoAck"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegAcNoAck),			REGISTRY_SIZE(RegAcNoAck),			0,					0,		0xFF}, 	
	//----------------------------------------------------------------------------
	// For inactive power save feature.
	// If this mode is enabled, NIC will go into rf off state when disconnected. 2006.07.16, by shien chang.
	{REGISTRY_STR("InactivePs"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegInactivePsMode),		REGISTRY_SIZE(RegInactivePsMode),			2,			0,		2}, // 0: disable, 1: enable. 
	//----------------------------------------------------------------------------
	// For inactive power save feature.
	// If this mode is enabled, NIC will go into 802.11 power save mode at leisure .
	{REGISTRY_STR("bLeisurePs"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegLeisurePsMode),		REGISTRY_SIZE(RegLeisurePsMode),			1,			0,		2}, // 0: disable, 1: enable. 
	{REGISTRY_STR("bAdvancedLPs"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegAdvancedLPs),		REGISTRY_SIZE(RegAdvancedLPs),				0,			0,		1}, // 0: disable, 1: enable. 
	//----------------------------------------------------------------------------
	// LPS Listen Interval.
	// 0x0 - eFastPs, 0xFF -DTIM, 0xNN - 0xNN * BeaconIntvl
	{NDIS_STRING_CONST("LPSIntvl"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegLPSMaxIntvl),		REGISTRY_SIZE(RegLPSMaxIntvl),			5,			0,		0xFF}, 
	//----------------------------------------------------------------------------
	// FW control LPS mode. Added by tynli.
	// 0=False, 1=MinPS, 2=MaxPS, 3=Manually setting wakeup bcn interval (refer to RegLPSMaxIntvl).
	{REGISTRY_STR("bFwCtrlLPS"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegFwCtrlLPS),			REGISTRY_SIZE(bRegFwCtrlLPS),			3,		0,		3},	//0=False, 1=MinPS, 2=MaxPS, 3=Manually setting wakeup bcn interval
	// Low power state, ex: 32k. Added by tynli.
	{REGISTRY_STR("bLowPowerEnable"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegLowPowerEnable),			REGISTRY_SIZE(bRegLowPowerEnable),			0,		0,		1},	//0=Disable, 1=Enable
	//CLose IO During 32K. Added by Sherry.
	{REGISTRY_STR("bLPSDeepSleepEnable"),				0,NdisParameterInteger, REGISTRY_OFFSET(bRegLPSDeepSleepEnable),			REGISTRY_SIZE(bRegLPSDeepSleepEnable),			0,		0,		1}, //0=Disable, 1=Enable
	//----------------------------------------------------------------------------
	// Keep-Alive mechanism. 2008-08-16 Isaiah.
 	// Send Null-Function Data in period in order to  tell AP that we are alive.
	{REGISTRY_STR("KeepAliveLevel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegKeepAliveLevel),		REGISTRY_SIZE(RegKeepAliveLevel),			1,			0,		2}, // 0: disable, 1: default(depend on Tx/Rx). 2. Force 
	//----------------------------------------------------------------------------
	// Roam Sensitive Level : 0x7F: Disable raoming
	{NDIS_STRING_CONST("RegROAMSensitiveLevel"),		0,NdisParameterInteger,		REGISTRY_OFFSET(RegROAMSensitiveLevel),	REGISTRY_SIZE(RegROAMSensitiveLevel),		127,                   30,              127},
	// Roam Hysteresis : 0 means using AP setting or default setting
	{NDIS_STRING_CONST("RoamHysteresis"),		0,NdisParameterInteger,		REGISTRY_OFFSET(RegRoamHysteresis),	REGISTRY_SIZE(RegRoamHysteresis),		6,                   0,              10},
	//----------------------------------------------------------------------------
	// Roaming Accept Time, in sec.
	{NDIS_STRING_CONST("RegRoamingLimit"),		0,NdisParameterInteger,		REGISTRY_OFFSET(RegRoamingLimit),	REGISTRY_SIZE(RegRoamingLimit),		4,                   0,              30},
	//----------------------------------------------------------------------------
	// For  Disable CCK rate 
	{REGISTRY_STR("DisableCck"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegDisableCck),		REGISTRY_SIZE(bRegDisableCck),		0,					0,		1}, // 1: CCK disable, 0: CCK enable .	
	//----------------------------------------------------------------------------
	// For  CCK rate support
	{REGISTRY_STR("HT_EnableCck"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegHT_EnableCck),		REGISTRY_SIZE(bRegHT_EnableCck),		1,					0,		1}, // 1: HT CCK enable, 0: HT CCK disable .	
	//----------------------------------------------------------------------------

	{REGISTRY_STR("AdhocUseHWSec"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegAdhocUseHWSec),			REGISTRY_SIZE(bRegAdhocUseHWSec),		0,					0,		1}, // 1: Adhoc use HW encrypt/decrypt for FPGA verification, 0: Adhoc use HW encrypt/decrypt as default.
	{REGISTRY_STR("11nAdhoc"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bReg11nAdhoc),			REGISTRY_SIZE(bReg11nAdhoc),		0,					0,		1}, // 1: 11n adhoc 20Mhz support, 0: 11g adhoc 20MHz support.

	//----------------------------------------------------------------------------
	{REGISTRY_STR("IOTBcm256QAM"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegIOTBcm256QAM),	REGISTRY_SIZE(bRegIOTBcm256QAM),		0,					0,		1}, // 1: Bcm IOT 256QAM support
	// 2013/10/04 MH Add for 2.4G VHT mode support.
	{REGISTRY_STR("Vht24g"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegVht24g),			REGISTRY_SIZE(bRegVht24g),		0,					0,		1}, // 1: support 2.4G 256Q VHT mode 0: disable	
	//----------------------------------------------------------------------------
	// For A-MSDU
	{REGISTRY_STR("AMSDU"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegAMSDU),				REGISTRY_SIZE(RegAMSDU),			0,					0,		1},
	{REGISTRY_STR("AMSDU_MaxSize"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegAMSDU_MaxSize),		REGISTRY_SIZE(RegAMSDU_MaxSize),	HT_AMSDU_SIZE_8K,	0,		VHT_AMSDU_SIZE_11K},
	{REGISTRY_STR("HWAMSDU"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegHWAMSDU),			REGISTRY_SIZE(RegHWAMSDU),			0,					0,		1},
	//----------------------------------------------------------------------------
	// For 802.11n A-MPDU <2006.08.08 Emily>
	{REGISTRY_STR("AMPDUEnable"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegAMPDUEnable),		REGISTRY_SIZE(bRegAMPDUEnable),		1,					0,		1},		// 0:Disable A-MPDU, 2: Enable A-MPDU
	{REGISTRY_STR("AMPDU_Factor"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegAMPDU_Factor),		REGISTRY_SIZE(RegAMPDU_Factor),		3,					0,		VHT_AGG_SIZE_1024K},		// 0: 2n13(8K), 1:2n14(16K), 2:2n15(32K), 3:2n16(64k)	
	{REGISTRY_STR("MPDU_Density"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegMPDU_Density),		REGISTRY_SIZE(RegMPDU_Density),		0,					0,		HT_DENSITY_16US},		// 0: No restriction, 1: 1/8usec, 2: 1/4usec, 3: 1/2usec, 4: 1usec, 5: 2usec, 6: 4usec, 7:8usec
	//----------------------------------------------------------------------------
	// 11N No Protection. Turn on this registry to disable all protection in 11N condition
	{REGISTRY_STR("MimoPs"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegMimoPs),				REGISTRY_SIZE(RegMimoPs),			3,      0,       3},  // 0: Static Mimo Ps, 1: Dynamic Mimo Ps, 3: No Limitation, 2: Reserved(Set to 3 automatically.)
	//----------------------------------------------------------------------------
	// For Rx Reorder Control
	{REGISTRY_STR("RxReorder"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegRxReorder),			REGISTRY_SIZE(RegRxReorder),			1,		0,		0xf}, // 0: disable, 1: enable, 2: switch by throughput
	{REGISTRY_STR("RxReorder_WinSize"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegRxReorder_WinSize),		REGISTRY_SIZE(RegRxReorder_WinSize),		64,		1,		256},
	{REGISTRY_STR("RxReorder_PendTime"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegRxReorder_PendTime),	REGISTRY_SIZE(RegRxReorder_PendTime),		30,		1,		10000},	

	//----------------------------------------------------------------------------
	//	RTL8192C USB Tx/Rx Aggregation
	//----------------------------------------------------------------------------
#if TX_AGGREGATION
	{REGISTRY_STR("UsbTxAggMode"), 			0,	NdisParameterInteger, REGISTRY_OFFSET(UsbTxAggMode),			REGISTRY_SIZE(UsbTxAggMode), 1, 0, 1},
	{REGISTRY_STR("UsbTxAggTotalNum"), 		0,NdisParameterInteger, REGISTRY_OFFSET(UsbTxAggTotalNum),		REGISTRY_SIZE(UsbTxAggTotalNum),		6,	1,	31},	
	{REGISTRY_STR("UsbTxAggDescNum"), 		0,	NdisParameterInteger, REGISTRY_OFFSET(UsbTxAggDescNum),			REGISTRY_SIZE(UsbTxAggDescNum), 6, 1, 15},
	//{REGISTRY_STR("UsbTxAggPerBulkNum"), 		0,NdisParameterInteger, REGISTRY_OFFSET(UsbTxAggPerBulkNum),		REGISTRY_SIZE(UsbTxAggPerBulkNum),		6,	1,	15},
#endif
#if	RX_AGGREGATION
	{REGISTRY_STR("UsbRxAggMode"),			0,	NdisParameterInteger, REGISTRY_OFFSET(RxAggMode),			REGISTRY_SIZE(RxAggMode), 3, 0, 3},
	{REGISTRY_STR("UsbRxAggBlockCount"), 		0,	NdisParameterInteger, REGISTRY_OFFSET(RxAggBlockCount),		REGISTRY_SIZE(RxAggBlockCount), 8, 1, 255},
	{REGISTRY_STR("UsbRxAggBlockTimeout"),	0,	NdisParameterInteger, REGISTRY_OFFSET(RxAggBlockTimeout),	REGISTRY_SIZE(RxAggBlockTimeout), 6, 1, 15},
	{REGISTRY_STR("UsbRxAggPageCount"),		0,	NdisParameterInteger, REGISTRY_OFFSET(RxAggPageCount),		REGISTRY_SIZE(RxAggPageCount), 16, 1, 255},
	{REGISTRY_STR("UsbRxAggPageTimeout"),	0,	NdisParameterInteger, REGISTRY_OFFSET(RxAggPageTimeout),		REGISTRY_SIZE(RxAggPageTimeout), 6, 1, 32},
#endif
	//----------------------------------------------------------------------------
	// Accept ADDBA Request
	{REGISTRY_STR("AcceptAddbaReq"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegAcceptAddbaReq),	REGISTRY_SIZE(bRegAcceptAddbaReq),	1,		0,		1},	
	{REGISTRY_STR("SupportPNPCapabilities"),			0,NdisParameterInteger, REGISTRY_OFFSET(bRegPnpCapabilities),	REGISTRY_SIZE(bRegPnpCapabilities), 	1,		0,		1}, // 1 support pnp set oid, 0 unload driver when s3,s4
	//----------------------------------------------------------------------------
	{REGISTRY_STR("CloEnable"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegClevoDriver),		REGISTRY_SIZE(bRegClevoDriver),			0,		0,		1}, // 0:Other customers' driver 1: Clevo driver version

	{REGISTRY_STR("ARPOffloadEnable"),				0,NdisParameterInteger, REGISTRY_OFFSET(bRegARPOffloadEnable), 		REGISTRY_SIZE(bRegARPOffloadEnable),			0,		0,		1},	// 0: disable, 1: enable
	{REGISTRY_STR("NSOffloadEnable"),				0,NdisParameterInteger, REGISTRY_OFFSET(bRegNSOffloadEnable), 		REGISTRY_SIZE(bRegNSOffloadEnable),			0,		0,		1},	// 0: disable, 1: enable
	{REGISTRY_STR("GTKOffloadEnable"),			0,NdisParameterInteger, REGISTRY_OFFSET(bRegGTKOffloadEnable), 		REGISTRY_SIZE(bRegGTKOffloadEnable),			0,		0,		1},	// 0: disable, 1: enable
	{REGISTRY_STR("ProtocolOffloadDecision"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegProtocolOffloadDecision), 		REGISTRY_SIZE(RegProtocolOffloadDecision),			0,		0,		1},	// 0: disable, 1: enable

        {REGISTRY_STR("DMInitialGain"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegDMInitialGain),		REGISTRY_SIZE(bRegDMInitialGain),		1,		0,		1}, // 1 support pnp set oid, 0 unload driver when s3,s4	
	//----------------------------------------------------------------------------

	{REGISTRY_STR("GPIORFSW"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegGpioRfSw),		REGISTRY_SIZE(bRegGpioRfSw),			1,		0,		1},

	{REGISTRY_STR("GPIOHWPBC"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegHwWpsPbc),		REGISTRY_SIZE(bRegHwWpsPbc),			0,		0,		1},

	//----------------------------------------------------------------------------
	// Indicate event when receiving deauth/disassoc packet. If this registry is set false, the disconnected state shall be recovered by roaming. Impact on XP/MacOS
	// 0: Not indicate, 1: Indicate
	{REGISTRY_STR("IndicateByDeauth"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegIndicateByDeauth),		REGISTRY_SIZE(RegIndicateByDeauth),			0,			0,		1}, 
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	// Show Tx/Rx rate setting
	{REGISTRY_STR("ShowRate"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegShowRate),		REGISTRY_SIZE(RegShowRate),			0,		0,		3}, // 0:determine by E-fuse, 1:forced Tx, 2:forced Rx, 3:forced Rx Max
	//----------------------------------------------------------------------------
	// TKIP in N mode
	{REGISTRY_STR("TKIPinNmode"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegTKIPinNmode),		REGISTRY_SIZE(bRegTKIPinNmode),			0,		0,		1}, // 0:TKip ONLY B/G 1: TKIP will use N mode
	// WEP in N mode
	{REGISTRY_STR("WEPinNmode"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegWEPinNmode),		REGISTRY_SIZE(bRegWEPinNmode),			0,		0,		1}, // 0:WEP ONLY B/G 1: WEP will use N mode
	//----------------------------------------------------------------------------
	// WoWLAN mode. Added by tynli. 2009.09.01.
	{REGISTRY_STR("APOffloadEnable"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegAPOffloadEnable), 		REGISTRY_SIZE(RegAPOffloadEnable),			0,		0,		1},	// 0: disable, 1: enable
	// WoWLAN LPS. 0: disable, 1: enable LPS but not enter 32K, 2: enable LPS and enter 32K
	{REGISTRY_STR("WoWLANLPSLevel"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegWoWLANLPSLevel), 		REGISTRY_SIZE(RegWoWLANLPSLevel),			0,		0,		2},
	{REGISTRY_STR("*WakeOnMagicPacket"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegWakeOnMagicPacket),			REGISTRY_SIZE(bRegWakeOnMagicPacket),			0,                   0,              1},
	{REGISTRY_STR("*WakeOnPattern"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegWakeOnPattern),			REGISTRY_SIZE(bRegWakeOnPattern),			0,                   0,              1},
	{REGISTRY_STR("WakeOnDisconnect"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegWakeOnDisconnect),			REGISTRY_SIZE(bRegWakeOnDisconnect),			1,                   0,              1},
	{REGISTRY_STR("WoWLANS5Support"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegWoWLANS5Support), 		REGISTRY_SIZE(RegWoWLANS5Support),			0,		0,		1},	// 0: disable, 1: enable
	// Connected standby LPS Listen Interval. 0xNN = 0xNN * BeaconIntvl
	{NDIS_STRING_CONST("D2ListenIntvl"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegD2ListenIntvl),		REGISTRY_SIZE(RegD2ListenIntvl),			5,			1,		0xFF}, 
	{NDIS_STRING_CONST("bFakeWoWLAN"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegFakeWoWLAN),		REGISTRY_SIZE(bRegFakeWoWLAN),			0,			0,		1}, // 0: disable, 1: enable

	{REGISTRY_STR("NLOEnable"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegNLOEnable), 		REGISTRY_SIZE(RegNLOEnable),			0,		0,		1},	// 0: disable, 1: enable
	// For AOAC reconnection policy.
	{REGISTRY_STR("PnpKeepLink"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegPnpKeepLink),			REGISTRY_SIZE(RegPnpKeepLink),			0,                   0,              1}, // 0: disable, 1: enable
	{REGISTRY_STR("btHsMode"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegbtHsMode),		REGISTRY_SIZE(RegbtHsMode), 		2,					0,		2}, 		// 0: Force to disable HS mode, 1: Force to enable HS mode, 2: auto mode
	//----------------------------------------------------------------------------
	{REGISTRY_STR("Velocity"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegVelocity),		REGISTRY_SIZE(bRegVelocity),			1,		0,		1}, // 0:disable, 1: enable
	// 2010/09/01 MH According to PM's request, we support dongle selective suspend mode switch.	
	{REGISTRY_STR("DongleSS"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegDongleSS),	REGISTRY_SIZE(bRegDongleSS),		0,		0,		1}, 	// 0:power down disable, 1:Power down enable,.
	// 2011/02/16 MH Add for CU selective suspend workaroud temporarily. This is used to  prevent CU GPIO HW floating.	
	{REGISTRY_STR("SSWakeCnt"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegSSWakeCnt),	REGISTRY_SIZE(bRegSSWakeCnt),		60,		30,		2000}, 	// 30~2000=GPIO detect rounds.

	{REGISTRY_STR("*PacketCoalescing"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegPacketCoalescing),			REGISTRY_SIZE(bRegPacketCoalescing),			1,                   0,              1}, // 0: disable, 1: enable
	//----------------------------------------------------------------------------
	{REGISTRY_STR("bTDLSEnable"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegTDLSEnable),		REGISTRY_SIZE(bRegTDLSEnable),			1,		0,		1},
	//----------------------------------------------------------------------------
       // Turn on/off Virtual wifi cpability.
	{REGISTRY_STR("VWifiSupport"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegVWifiSupport),		REGISTRY_SIZE(bRegVWifiSupport),			1,		0,		1}, // 0:Disabl Virtual Wifi support, 1:Enable
	//vivi add this reg, 1230
	{REGISTRY_STR("DMDPMAC0VWifiSupport"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegDMDPMAC0VWifiSupport),		REGISTRY_SIZE(bRegDMDPMAC0VWifiSupport),			0,		0,		1}, // 0:Disabl Virtual Wifi support, 1:Enable

	{REGISTRY_STR("UseRxInterruptWorkItem"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegUseRxInterruptWorkItem),	REGISTRY_SIZE(bRegUseRxInterruptWorkItem),		0,		0,		1}, // 0:Disable DisableRxInterruptWorkitem, 1:Enable DisableRxInterruptWorkitem.
	{REGISTRY_STR("bRegDefaultAntenna"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegDefaultAntenna),	REGISTRY_SIZE(bRegDefaultAntenna),	1,                   0,              1}, // 0 : Using Aux Antenna , 1 : Using Main Antenna 
	{REGISTRY_STR("AntDiv"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegAntDiv),			REGISTRY_SIZE(RegAntDiv),			2,		0,		2}, // 0:OFF , 1:ON, 2:From Efuse 
	{REGISTRY_STR("AntDetection"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegAntDetection),			REGISTRY_SIZE(RegAntDetection),			1,		0,		4}, // BIT0: Antenna Detection by Single Tone, BIT1: Antenna Detection by RSSI// TCP offload 
	{REGISTRY_STR("RegTCPOffloadMode"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegTCPOffloadMode),	REGISTRY_SIZE(RegTCPOffloadMode),	0,                   0,              256}, // bit0 : IPV4 Tx, bit1 : IPV4 RX, bit5 : IPV6 Tx, bit6 : IPV6 RX 


	//----------------------------------------------------------------------------
	// For reset Tx hang on 88/92CUSB. 0=Disable, 1=Enable. Added by tynli. 2010.03.23.
	{REGISTRY_STR("USBResetTxHang"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegUSBResetTxHang), 		REGISTRY_SIZE(RegUSBResetTxHang),			0,		0,		1},
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	// Extended TIM set.
	// 0=Disable, 1=Enable
	{REGISTRY_STR("APTimExtend"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegAPTimExtend), 		REGISTRY_SIZE(RegAPTimExtend),			0,		0,		1},
	//----------------------------------------------------------------------------
	// 2010/05/18 MH For ECS quick GPIO detect timer function add three inf function.
	{REGISTRY_STR("TimerGPIO"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegTimerGPIO),	REGISTRY_SIZE(bRegTimerGPIO),		0,		0,		1}, // 0:Disable GPIO timer detect, 1:Enable GPIO timer detect,.
	{REGISTRY_STR("GPIODelay"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegGPIODelay),	REGISTRY_SIZE(bRegGPIODelay),		500,		100,		5000}, //100~5000 ms delay for GPIO detect scheme delay timer
	{REGISTRY_STR("GPIOBack"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegGPIOBack),	REGISTRY_SIZE(bRegGPIOBack),		250,		10,		5000}, //10~5000 ms delay for GPIO detect return callback timer delay
	
	// 2015/07/29 Gary For customized LEDStrategy.
	{REGISTRY_STR("LedStrategy"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegLedStrategy),	REGISTRY_SIZE(bRegLedStrategy),		64,		0,		64}, //0~63 led MODE, 64: Default LED Mode
	
	// 2010/08/25 MH According to PM's request, we support inf to switch power down mode.	
	{REGISTRY_STR("PDNMode"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegPDNMode),	REGISTRY_SIZE(bRegPDNMode),		1,		0,		1}, 	// 0:power down disable, 1:Power down enable,.


	// 2010/09/13 MH According to PM's request, we support different SS power seave level.	
	{REGISTRY_STR("SSPwrLvl"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegSSPwrLvl),	REGISTRY_SIZE(bRegSSPwrLvl),		2,		0,		2}, 	// 0:~2=different SS power save level. Level 2 is the best one now.


	// 2010/12/17 MH Add for RX aggregation mode switch according to TX/RX traffic.	
	{REGISTRY_STR("AggDMEnable"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegAggDMEnable),	REGISTRY_SIZE(bRegAggDMEnable),		1,		0,		2}, 	// 0/1/2=disable/enable/auto.	
	// use another thread to send packets. prevent BSOD by stack overflow
	{REGISTRY_STR("SendPacketByTimer"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegSendPacketByTimer), 		REGISTRY_SIZE(RegSendPacketByTimer),			0,		0,		1}, // 1: use timer to send packet, 0 : off

	// 2010/12/31 MH Add for UPHY dynamic chaneg.	
	{REGISTRY_STR("UPDMEnable"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegUPDMEnable),	REGISTRY_SIZE(bRegUPDMEnable),		1,		0,		2}, 	// 0/1/2=disable/enable/auto.	
	//----------------------------------------------------------------------------
	// Earlymode ctrl. 0=Disable, 1=Enable. by wl 20101012.
	{REGISTRY_STR("bEarlymodeEnable"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegEarlymodeEnable), 		REGISTRY_SIZE(RegEarlymodeEnable),			0,		0,		1},

	{REGISTRY_STR("bAutoAMPDUBurstMode"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegAutoAMPDUBurstModeEnable), 		REGISTRY_SIZE(RegAutoAMPDUBurstModeEnable),			1,		0,		1},
	{REGISTRY_STR("AutoAMPDUBurstModeThreshold"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegAutoAMPDUBurstModeThreshold), 		REGISTRY_SIZE(RegAutoAMPDUBurstModeThreshold),			92,		1,		256},
	{REGISTRY_STR("TxHignTPThreshold"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegTxHignTPThreshold), 		REGISTRY_SIZE(RegTxHignTPThreshold),			50,		1,		1000}, // Mbps
	{REGISTRY_STR("RxHignTPThreshold"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegRxHignTPThreshold), 		REGISTRY_SIZE(RegRxHignTPThreshold),			50,		1,		1000}, // Mbps
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	//Two STA Concurrent Mode: 0: Disable(MAC 0 5G/2.4G 2*2), 1: Enable(MAC 0 5G MAC 1 2.4G 1*1), 2: Auto(MAC 0 can upto 2*2 5G  if MAC 1 disconnect)
	{REGISTRY_STR("TwoStaConcurrentMode"),				0,NdisParameterInteger, REGISTRY_OFFSET(RegTwoStaConcurrentMode), 		REGISTRY_SIZE(RegTwoStaConcurrentMode),			0,		0,		2},
	//----------------------------------------------------------------------------
	// 2011/07/08 MH Add for Link Speed display for different customer.	
	{REGISTRY_STR("LinkSpeedLevel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegLinkSpeedLevel),	REGISTRY_SIZE(bRegLinkSpeedLevel),		0,		0,		0xff}, 	// 0/~N= For different speed display.

	// 0: default, 1: real signal strength (Acer request), 2: HP special mode
	{REGISTRY_STR("RSSI2GridMode"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegRSSI2GridMode),	REGISTRY_SIZE(RegRSSI2GridMode),		0,		0,		4},

	// 2011/07/14 MH Add for rx short.	
	{REGISTRY_STR("RxSC"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegRxSC),	REGISTRY_SIZE(bRegRxSC),		1,		0,		1}, 	// 0/1=disable/enable
	// 2011/07/15 Sinda Add for tx short cut.	
	{REGISTRY_STR("TxSC"),	0,NdisParameterInteger,	REGISTRY_OFFSET(bRegTxSC),	REGISTRY_SIZE(bRegTxSC),		1,		0,		1}, 	// 0/1=disable/enable

	{REGISTRY_STR("PAMode"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegPAMode), 	REGISTRY_SIZE(RegPAMode),	PA_MODE_INTERNAL_SP3T,	 PA_MODE_EXTERNAL,		PA_MODE_INTERNAL_SPDT}, 

	// 2011/09/15 MH Add registry for switching packet compete method.
	{REGISTRY_STR("TxMode"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegTxMode),		REGISTRY_SIZE(RegTxMode),	0,      0x0,       0xF}, 	// 0x0~0xF. Different TX mode.

	// 2011/11/15 MH Add for user can select different region and map to dedicated power gain offset table.
	{REGISTRY_STR("PwrTblSel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrTblSel),		REGISTRY_SIZE(RegPwrTblSel),	0,      0x0,       0xF}, 	// 0x0~0xF. 1/2/3=ETSI/MKK/FCC

	// 0: Highest, 1: Medium-high, 2: Medium, 3: Medium-low, 4: Lowest
	{REGISTRY_STR("TxPwrLevel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegTxPwrLevel),		REGISTRY_SIZE(RegTxPwrLevel),	0,      0,       4},

	// 2011/11/15 MH Add for user can select different tx power by rate switch by default value and registry value.
	{REGISTRY_STR("PwrByRate"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrByRate),		REGISTRY_SIZE(RegPwrByRate),	0,      0x0,       0x1}, 	// 0x0/1 = by default/ by registry.
	{REGISTRY_STR("PwrRaTbl1"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl1),		REGISTRY_SIZE(RegPwrRaTbl1),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 1 index diff.
	{REGISTRY_STR("PwrRaTbl2"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl2),		REGISTRY_SIZE(RegPwrRaTbl2),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 2 index diff.
	{REGISTRY_STR("PwrRaTbl3"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl3),		REGISTRY_SIZE(RegPwrRaTbl3),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 3 index diff.
	{REGISTRY_STR("PwrRaTbl4"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl4),		REGISTRY_SIZE(RegPwrRaTbl4),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 4 index diff.
	{REGISTRY_STR("PwrRaTbl5"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl5),		REGISTRY_SIZE(RegPwrRaTbl5),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 5 index diff.
	{REGISTRY_STR("PwrRaTbl6"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl6),		REGISTRY_SIZE(RegPwrRaTbl6),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 6 index diff.
	{REGISTRY_STR("PwrRaTbl7"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl7),		REGISTRY_SIZE(RegPwrRaTbl7),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 7 index diff.
	{REGISTRY_STR("PwrRaTbl8"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl8),		REGISTRY_SIZE(RegPwrRaTbl8),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 8 index diff.
	{REGISTRY_STR("PwrRaTbl9"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl9),		REGISTRY_SIZE(RegPwrRaTbl9),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 9 index diff.
	{REGISTRY_STR("PwrRaTbl10"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl10),		REGISTRY_SIZE(RegPwrRaTbl10),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 10 index diff.
	{REGISTRY_STR("PwrRaTbl11"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl11),		REGISTRY_SIZE(RegPwrRaTbl11),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 11 index diff.
	{REGISTRY_STR("PwrRaTbl12"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl12),		REGISTRY_SIZE(RegPwrRaTbl12),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 12 index diff.
	{REGISTRY_STR("PwrRaTbl13"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl13),		REGISTRY_SIZE(RegPwrRaTbl13),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 13 index diff.
	{REGISTRY_STR("PwrRaTbl14"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl14),		REGISTRY_SIZE(RegPwrRaTbl14),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 14 index diff.
	{REGISTRY_STR("PwrRaTbl15"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl15),		REGISTRY_SIZE(RegPwrRaTbl15),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 15 index diff.
	{REGISTRY_STR("PwrRaTbl16"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegPwrRaTbl16),		REGISTRY_SIZE(RegPwrRaTbl16),	0x00000000,      0x0,       0xFFFFFFFF}, 	// 0x???????? ==> For power table 16 index diff.

	//2011.09.23 LukeLee add for path diversity
	{REGISTRY_STR("PathDiv"), 		0,NdisParameterInteger, REGISTRY_OFFSET(bPathDivEnable), REGISTRY_SIZE(bPathDivEnable),		0,		0,		2}, 	// 0: disable PathDiv,  1: 1T PathDiv,  2: 2T PathDiv

	{REGISTRY_STR("DbgZone"), 		0,NdisParameterString, REGISTRY_OFFSET(RegDbgZone), REGISTRY_SIZE(RegDbgZone),		0,		0,		128}, 	// 1 = Fix tx power 
	{REGISTRY_STR("DbgMode"), 		0,NdisParameterInteger, REGISTRY_OFFSET(bRegDbgMode), REGISTRY_SIZE(bRegDbgMode),		0,		0,		1}, 	// 1 = Fix tx power 
	//----------------------------------------------------------------------------
	// 2011/12/08 hpfan Add for Tcp Reorder
	{REGISTRY_STR("TcpReorder"), 		0,NdisParameterInteger, REGISTRY_OFFSET(bTcpReorderEnable), REGISTRY_SIZE(bTcpReorderEnable),		0,		0,		1}, 	// 0: disable TcpReorder,  1: enable TcpReorder

	//2011.11.29 Sherry Add for Belkin 2.4G or 5G support different bandwidth
	{REGISTRY_STR("BW40MHzFor2G"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegBW40MHzFor2G),			REGISTRY_SIZE(bRegBW40MHzFor2G),		1,					0,		1}, // 1: 40MHz support, 0: 20MHz support.
	{REGISTRY_STR("BW40MHzFor5G"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bRegBW40MHzFor5G),			REGISTRY_SIZE(bRegBW40MHzFor5G),		1,					0,		1}, // 1: 40MHz support, 0: 20MHz support.
	//----------------------------------------------------------------------------

	//2012.01.04 Sherry Add for support txpacketbuffer coleose 
	{REGISTRY_STR("SupportTxPacketBufferColease"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bSupportTxPacketBufferColease),			REGISTRY_SIZE(bSupportTxPacketBufferColease),		0,					0,		1}, 

	//2012.02.06 Sherry Add for Belkin to switch band by end-user
	{REGISTRY_STR("WirelessBand"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegWirelessBand),			REGISTRY_SIZE(RegWirelessBand),		2,					0,		2}, // 0:2.4G 1:5G, 2:Both
	//----------------------------------------------------------------------------
	// 11n Sigma Support
	{REGISTRY_STR("b40Intolerant"),			0,NdisParameterInteger,	REGISTRY_OFFSET(bReg40Intolerant),			REGISTRY_SIZE(bReg40Intolerant),		0,					0,		1}, // 0/1 = Disable / Enable
	{REGISTRY_STR("bAMPDUManual"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegAMPDUManual),			REGISTRY_SIZE(bRegAMPDUManual),	0,					0,		1}, // 0/1 = Disable / Enable
	{REGISTRY_STR("TxSPStream"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegTxSPStream),			REGISTRY_SIZE(RegTxSPStream),		0,					0,		2}, // 0/1/2 = AutoMax / 1SS / 2SS
	{REGISTRY_STR("RxSPStream"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegRxSPStream),			REGISTRY_SIZE(RegRxSPStream),		0,					0,		2}, // 0/1/2 = AutoMax / 1SS / 2SS
	// BIT0: Enable VHT Beamformer, BIT1: Enable VHT Beamformee
	// BIT2:Enable VHT MU-MIMO AP, BIT3:Enable VHT MU-MIMO STA
	// BIT4: Enable HT Beamformer, BIT5: Enable HT Beamformee
	{REGISTRY_STR("BeamformCap"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegBeamformCap),			REGISTRY_SIZE(RegBeamformCap),		0,				0,		0x33},
	{REGISTRY_STR("BeamformAutoTest"),		0,NdisParameterInteger,	REGISTRY_OFFSET(bRegBeamformAutoTest),	REGISTRY_SIZE(bRegBeamformAutoTest),	0,				0,		1},
	// Set BF Rf path number, 0 for auto, others for manual
	{REGISTRY_STR("BeamformerCapRfNum"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegBeamformerCapRfNum),			REGISTRY_SIZE(RegBeamformerCapRfNum),		0,				0,		0x4},
	{REGISTRY_STR("BeamformeeCapRfNum"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegBeamformeeCapRfNum),			REGISTRY_SIZE(RegBeamformeeCapRfNum),		0,				0,		0x4},
	// BIT0: Enable VHT LDPC Rx, BIT1: Enable VHT LDPC Tx, BIT4: Enable HT LDPC Rx, BIT5: Enable HT LDPC Tx
	{REGISTRY_STR("LdpcCap"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegLdpc),			REGISTRY_SIZE(RegLdpc),		0x33,					0,		0x33},
	// BIT0: Enable VHT STBC Rx, BIT1: Enable VHT STBC Tx, BIT4: Enable HT STBC Rx, BIT5: Enable HT STBC Tx
	{REGISTRY_STR("StbcCap"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegStbc),			REGISTRY_SIZE(RegStbc),		0x33,					0,		0x33},
	//----------------------------------------------------------------------------
	// 0:undefined, 1:USB2, 2:USB3 3: USB switch can detect RSSI 4: USB switch + long range 5: VL/VS/VN always USB20 
	// 6: Reserve 8: Can U3 to U2, but forbid U2 to U3.
	{REGISTRY_STR("UsbMode"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbMode),				REGISTRY_SIZE(RegUsbMode),		0,					0,		2}, // 0:undefined, 1:USB2, 2:USB3
	{REGISTRY_STR("ForcedUsbMode"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegForcedUsbMode),			REGISTRY_SIZE(RegForcedUsbMode),0,					0,		0xf}, // 0:undefined, 1:USB2, 2:USB3
	{REGISTRY_STR("UsbCurMode"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbCurMode),				REGISTRY_SIZE(RegUsbCurMode),		0,					0,		2}, 

	{REGISTRY_STR("UsbMode3To2Cnt"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbMode3To2Cnt),			REGISTRY_SIZE(RegUsbMode3To2Cnt),	0,				0,		0xffff}, // 0:undefined, USB 3 transfer to 2 mode counter.
	{REGISTRY_STR("UsbMode2To3Cnt"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbMode2To3Cnt),			REGISTRY_SIZE(RegUsbMode2To3Cnt),	0,				0,		0xffff}, // 0:undefined, USB 2 transfer to 3 mode counter.
	{REGISTRY_STR("UsbMode2To3CntFail"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbMode2To3CntFail),		REGISTRY_SIZE(RegUsbMode2To3CntFail),	0,				0,		0xffff}, // 0:undefined, USB 2 transfer to 3 mode fail counter.	

	{REGISTRY_STR("UsbSwFast"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbSwFast),				REGISTRY_SIZE(RegUsbSwFast),0,					0,		1}, // USB switch speed level
	//20130523 MH add for NEC WPS check condition.
	{REGISTRY_STR("UsbWps"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegUsbWps),			REGISTRY_SIZE(RegUsbWps), 		0,		0,		 0xff},
	//20130523 MH add for USB switch speed.
	{REGISTRY_STR("UsbSp"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegUsbSp),			REGISTRY_SIZE(RegUsbSp), 		0,		0,		 0xff},
	{REGISTRY_STR("UsbChnl"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegUsbChnl),			REGISTRY_SIZE(RegUsbChnl), 		0,		0,		 0xff},
	{REGISTRY_STR("UsbSwitchSpeed"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbSwitchSpeed),			REGISTRY_SIZE(RegUsbSwitchSpeed),10,					0,		100}, // USB switch speed level
	{REGISTRY_STR("UsbSwitchThL"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbSwitchThL),			REGISTRY_SIZE(RegUsbSwitchThL),35,					0,		100}, // RSSI Low Threshold to Switch USB
	{REGISTRY_STR("UsbSwitchThH"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbSwitchThH),			REGISTRY_SIZE(RegUsbSwitchThH),45,					0,		100}, // RSSI High Threshold to Switch USB
	{REGISTRY_STR("UsbSwitchThRssi"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbSwitchThRssi),			REGISTRY_SIZE(RegUsbSwitchThRssi),35,					0,		100}, // RSSI for first link to U2 or U3.
	{REGISTRY_STR("UsbSwBy"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbSwBy),			REGISTRY_SIZE(RegUsbSwBy), 0,					0,		8}, // 0= by serverice, 1=by driver
	{REGISTRY_STR("UsbRfSet"),				0,NdisParameterInteger,	REGISTRY_OFFSET(RegUsbRfSet),			REGISTRY_SIZE(RegUsbRfSet), 0,					0,		100}, // 0= by device and host mode compare, 1=by Default

	// 2012/09/14 MH Add for EDCA turbo mode switch threshold.
	// 2012/09/14 MH Add for 88E rate adaptive mode siwtch.
	{REGISTRY_STR("EdcaThresh"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegEdcaThresh),		REGISTRY_SIZE(RegEdcaThresh),	0x100,      0x0,       0xFFFF}, 	// 0x0~0xFFFF. The thresh to disable EDA turbo.
	{REGISTRY_STR("RALvl"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegRALvl),			REGISTRY_SIZE(RegRALvl),		0x2,      0x0,       0xF}, 		// 0x0~0xF. The Driver layer rate adaptive level.

	// 2012/10/26 MH Add for 8812/8821 AC series dynamic switch. for USB3.0
	// 2013/04/18 MH According to YX's test the below parameters will be better in iperf/chariot test.
	{REGISTRY_STR("AcUsbDmaTime"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegAcUsbDmaTime),	REGISTRY_SIZE(RegAcUsbDmaTime),	0x1a,	0,	0xff}, 	
	{REGISTRY_STR("AcUsbDmaSize"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegAcUsbDmaSize),	REGISTRY_SIZE(RegAcUsbDmaSize),		0x07,	0,	0xff}, 			
	// 2013/04/18 MH Add USB2.0 Aggregation parameters
	{REGISTRY_STR("AcUsbDmaTime2"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegAcUsbDmaTime2),	REGISTRY_SIZE(RegAcUsbDmaTime2),	0x10,	0,	0xff}, 	
	{REGISTRY_STR("AcUsbDmaSize2"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegAcUsbDmaSize2),	REGISTRY_SIZE(RegAcUsbDmaSize2),		0x06,	0,	0xff}, 			

	// 2012/10/31 MH Add for power limit table constraint.
	{REGISTRY_STR("TxPwrLimit"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegTxPwrLimit),	REGISTRY_SIZE(RegTxPwrLimit),		0xFFFFFFFF,		0,		0xFFFFFFFF}, 	// Limit different Rate power index.
	// 0x10080604 ==> HT40/HT20/LegyctOFDM/CCK for example, HT40 max + 0x10 power index HT20 max +0x8 power index
	{REGISTRY_STR("DecryptCustomFile"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegDecryptCustomFile), 		REGISTRY_SIZE(RegDecryptCustomFile),	0,					0,		1}, 
	// Win8 FSS mode detection. 2012.09.13, by tynli.
	{REGISTRY_STR("FSSDetection"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegFSSDetection),		REGISTRY_SIZE(RegFSSDetection),			0,		0,		1}, // 0: disable, 1: enable

	//20130117 Used for Invitation No/After to Pass, with skip scan when Go is connected
	{REGISTRY_STR("GoSkipScanForWFDTest"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegGoSkipScanForWFDTest),		REGISTRY_SIZE(RegGoSkipScanForWFDTest), 	0x0,		0,		0x1},

	//20130117 reserve an optional to Pass WFD which is used for skip scan when Client is connected
	{REGISTRY_STR("ClientSkipScanForWFDTest"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegClientSkipScanForWFDTest),		REGISTRY_SIZE(RegClientSkipScanForWFDTest), 	0x0,		0,		0x1},

	//20130314 sherry add for reset tx stuck
	{REGISTRY_STR("EnableResetTxStuck"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegEnableResetTxStuck),			REGISTRY_SIZE(RegEnableResetTxStuck), 		0,		0,		 2},

	//----------------------------------------------------------------------------
	//2013/02/05 zhiyuan add for Lenovo AMD Platform, wake up on lan need twice push power button (sleep mode) to go to sleep. need clear wake up status after wake up.
	{REGISTRY_STR("ClearAMDWakeUpStatus"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegClearAMDWakeUpStatus),		REGISTRY_SIZE(RegClearAMDWakeUpStatus), 		0,		0,		1}, // 0/1 = disable/enable clear AMD wake up status.
	
	//20130415 For BT coexist with single antenna.
 	{REGISTRY_STR("DisableBTCoexist"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegDisableBTCoexist),			REGISTRY_SIZE(RegDisableBTCoexist),			0,      0,       1}, 	
	
	//20130606 MH For passive scan control dynamic switch after meeting with RF/FAE team.
 	{REGISTRY_STR("PassiveScan"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegPassiveScan),			REGISTRY_SIZE(RegPassiveScan),			0,      0,       0xFF}, 	// 0= by channel plan, 1=5g all passive scan / 2= 24g passive scan /3= 2/5g all passive scan

	// Set WFD Operating Channel
	{REGISTRY_STR("WFDOpChannel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegWFDOpChannel),	REGISTRY_SIZE(RegWFDOpChannel),		0,		0,		11}, 	// 0: driver default value, 1~11: channel number
	{REGISTRY_STR("WFDPeerOpChannel"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegWFDPeerOpChannel),	REGISTRY_SIZE(RegWFDPeerOpChannel),		0,		0,		11}, 	// 0: driver default value, 1~11: channel number
	{REGISTRY_STR("WFDTest"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegWFDTest),			REGISTRY_SIZE(bRegWFDTest),			0,                   0,              1},
	
	// Add for v3006.0.0320.2014 WHQL submission.
	{REGISTRY_STR("FixBTTdma"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegFixBTTdma),			REGISTRY_SIZE(RegFixBTTdma),		1,		0,		 1},  // 0: disable, 1: enable

	//----------------------------------------------------------------------------
	// Device WHQL test "DPC ISR" for SDIO. Added by tynli, 2014.09.11.
	{REGISTRY_STR("SdioDpcIsr"),	0,NdisParameterInteger, REGISTRY_OFFSET(RegSdioDpcIsr), 		REGISTRY_SIZE(RegSdioDpcIsr),		0,		0,		 1},  // 0: disable, 1: enable
	
	//----------------------------------------------------------------------------
	// For Intel low power platform. 2013.11.18, by tynli.
	{REGISTRY_STR("IntelPatchPNP"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegIntelPatchPNP),			REGISTRY_SIZE(RegIntelPatchPNP), 		0,		0,		 1},	// 0: disable, 1: enable
	
	//----------------------------------------------------------------------------
	// For PCIE Gen1 to Gen 2. 2013.12.24, by gw.
	{REGISTRY_STR("ForcePCIeRate"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegForcePCIeRate),			REGISTRY_SIZE(RegForcePCIeRate), 		1,		0,		 2},// 0: Auto 1:Gen1 24G 2:Gen2 5G

	//201401216 Yu Chen add for BB Adaptivity
	{REGISTRY_STR("DCbackoff"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegDCbackoff),			REGISTRY_SIZE(RegDCbackoff),			4,      0,       16},
	//201401225 Yu Chen add for BB Adaptivity
	{REGISTRY_STR("APNumTH"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegAPNumTH),			REGISTRY_SIZE(RegAPNumTH),			15,      0,       250},

	//[Work Around] Force GO port to transmit 20M BW data temporarily, added by Roger, 2013.10.18.
	{REGISTRY_STR("ForceGoTxData20MBw"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegForceGoTxData20MBw),		REGISTRY_SIZE(RegForceGoTxData20MBw), 	1,		0,		1}, // Default is 20M BW

	//20140702 hana add for USB WFD test, wait seconds before skip scan when Go is connected
	{REGISTRY_STR("WaitBeforeGoSkipScan"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegWaitBeforeGoSkipScan),		REGISTRY_SIZE(RegWaitBeforeGoSkipScan), 	0x0,		0,		0xFF},

	// 2012/11/07 Awk add PowerBase for customers to define their power base.
	{REGISTRY_STR("PowerBase"),	0,NdisParameterInteger, REGISTRY_OFFSET(RegPowerBase),	REGISTRY_SIZE(RegPowerBase),		14,		0,		0xFF},				
	
	// 2012/11/23 Awk add for enabling/disabling power limit
	{REGISTRY_STR("EnableTxPowerLimit"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegEnableTxPowerLimit),	REGISTRY_SIZE(RegEnableTxPowerLimit),		2,		0,		2},				
	{REGISTRY_STR("EnableTxPowerByRate"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegEnableTxPowerByRate),	REGISTRY_SIZE(RegEnableTxPowerByRate),		2,		0,		2},				
	{REGISTRY_STR("TxPowerTraining"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegTxPowerTraining),	REGISTRY_SIZE(RegTxPowerTraining),		1,		0,		1},

	// 2015/02/11 VincentL Add for primary/secondary power limit table selection (used as initial one)
	{REGISTRY_STR("TxPowerLimitTableSel"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegTxPowerLimitTableSel),	REGISTRY_SIZE(RegTxPowerLimitTableSel),		0,		0,		4},				
	{REGISTRY_STR("TxPwrLmtDynamicLoading"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegTxPwrLmtDynamicLoading),	REGISTRY_SIZE(RegTxPwrLmtDynamicLoading),		0,		0,		1},

	// 2015/04/27 VincentL Add for Toggle Support for TxPwrTable Dump Feature
	{REGISTRY_STR("SupportTxPwrTableDump"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegSupportTxPwrTableDump),	REGISTRY_SIZE(RegSupportTxPwrTableDump),		0,		0,		1},	

	// 2015/02/25 VincentL Add for UEFI method
	{REGISTRY_STR("LoadSystemSKUfromUEFI"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegLoadSystemSKUfromUEFI),	REGISTRY_SIZE(RegLoadSystemSKUfromUEFI),		0,		0,		1},	
	{REGISTRY_STR("UEFIProfile"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegUEFIProfile),	REGISTRY_SIZE(RegUEFIProfile),		0,		0,		100},				

	// 2015/07/07 VincentL Add for SMBIOS method
	{REGISTRY_STR("LoadSystemSKUfromSMBIOS"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegLoadSystemSKUfromSMBIOS),	REGISTRY_SIZE(RegLoadSystemSKUfromSMBIOS),		0,		0,		1},	
	{REGISTRY_STR("LoadProcessorIDfromSMBIOS"), 0,NdisParameterInteger, REGISTRY_OFFSET(RegLoadProcessorIDfromSMBIOS),	REGISTRY_SIZE(RegLoadProcessorIDfromSMBIOS),		0,		0,		1},

	{REGISTRY_STR("PwrByRateFile"),			0,NdisParameterString, REGISTRY_OFFSET(RegPwrByRateFile),			REGISTRY_SIZE(RegPwrByRateFile), 	(ADDR2DATA)&DefaultFileName,					0,		32}, 
	{REGISTRY_STR("PwrLimitFile"),			0,NdisParameterString, REGISTRY_OFFSET(RegPwrLimitFile), 		REGISTRY_SIZE(RegPwrLimitFile),	(ADDR2DATA)&DefaultFileName,					0,		32}, 
	{REGISTRY_STR("SecondaryPwrLimitFile"),			0,NdisParameterString, REGISTRY_OFFSET(RegSecondaryPwrLimitFile), 		REGISTRY_SIZE(RegSecondaryPwrLimitFile),	(ADDR2DATA)&DefaultFileName,					0,		32}, 

	// 2013/01/23 VincentLan add for enabling/disabling IQK Firmware Offload
	{REGISTRY_STR("IQKFWOffload"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegIQKFWOffload),		REGISTRY_SIZE(RegIQKFWOffload),			0,		0,		1},

	// 2013/04/16 VincentLan Add to switch Spur Calibration Method
	{REGISTRY_STR("SpurCalMethod"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegSpurCalMethod),		REGISTRY_SIZE(RegSpurCalMethod),			0,		0,		1},

	// 2013/11/23 VincentLan add for for KFree Feature Requested by RF David
	{REGISTRY_STR("RfKFreeEnable"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegRfKFreeEnable),		REGISTRY_SIZE(RegRfKFreeEnable),			0,		0,		2},


	{REGISTRY_STR("TxDutyEnable"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegTxDutyEnable),		REGISTRY_SIZE(RegTxDutyEnable),			0,		0,		1},

	//2012.10.08 Sherry Add for Abcom to open EDCCA
	{REGISTRY_STR("EDCCASupport"),			0,NdisParameterInteger, REGISTRY_OFFSET(bRegEDCCASupport),			REGISTRY_SIZE(bRegEDCCASupport),	0,					0,		1}, // 1: 40MHz support, 0: 20MHz support.

	// 2012/07/24 MH Add for win8 usb whql tst & WFD multi channel.
	// For WHCK Test support type.
	{REGISTRY_STR("UseDefaultCID"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegUseDefaultCID),	REGISTRY_SIZE(RegUseDefaultCID),		0,		0,		0xff}, 	// 0/1/2=Read from EFUSE /Forced default CID/ Forced HCT_CID)

	// BT FW patched ROM Code control, added by Roger, 2011.11.23.
	{REGISTRY_STR("BtFwSupport"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegBtFwSupport),			REGISTRY_SIZE(bRegBtFwSupport),			0,                   0,              1}, //0: Not support BT FW Download, 1: Support BT FW Download
	
	// For WHCK USB WFD test channel switch time.
	{REGISTRY_STR("WfdTime"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegWfdTime),	REGISTRY_SIZE(RegWfdTime),		150,		0,		0xff}, 			// Multi Channel switch time
	// For WHCK USB WFD Channel switch pattern.
	{REGISTRY_STR("WfdChnl"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegWfdChnl),	REGISTRY_SIZE(RegWfdChnl),		0,		0,		0xff}, 				// Multi Channel switch reported channel
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// Short GI support Bit Map
	// BIT0 - 20MHz, 0: support, 1: non-support
	// BIT1 - 40MHz, 0: support, 1: non-support
	// BIT2 - 80MHz, 0: support, 1: non-support
	// BIT3 - 160MHz, 0: support, 1: non-support
	{REGISTRY_STR("ShortGISupport"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegShortGISupport),		REGISTRY_SIZE(RegShortGISupport),		0xF,		0,		0xF},
	//----------------------------------------------------------------------------
	// 2012/11/06 Page Add for Power by rate DW/NByte access switch
	//  0: disable, means Byte by Byte access BB reg, 1: enable, means DW access BB reg while set power by rate  2: enable, means N Byte access BB reg
	{REGISTRY_STR("NByteAccess"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegNByteAccess),		REGISTRY_SIZE(RegNByteAccess),			0,		0,		2}, 
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// 2012/11/13 Page Add for Intial Offload by FW. 2012/11/27, page revise the definition: add FWOffload = 2 to indicate FW SW Channel offload
	// 0: disable, means driver do the whole intial work by itself  1: enable, means FW will do some initial offload work for driver to save time
	{REGISTRY_STR("FWOffload"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegFWOffload),		REGISTRY_SIZE(RegFWOffload),			0,		0,		2}, 
	//----------------------------------------------------------------------------
	// 0: download FW from file, 1: download FW from header
	{REGISTRY_STR("DownloadFW"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegDownloadFW),		REGISTRY_SIZE(RegDownloadFW),			1,		0,		1}, 

	// 0: Disable, 1: Enable
	{REGISTRY_STR("FWQC"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegFWQC),		REGISTRY_SIZE(RegFWQC),			0,		0,		1}, 

	// 2012/11/21 Sinda add for Scan time per channel
	{REGISTRY_STR("ScanLarge"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegScanLarge),		REGISTRY_SIZE(RegScanLarge),			0,		0,		1000}, 
	{REGISTRY_STR("ScanMiddle"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegScanMiddle),		REGISTRY_SIZE(RegScanMiddle),			0,		0,		1000}, 
	{REGISTRY_STR("ScanNormal"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegScanNormal),		REGISTRY_SIZE(RegScanNormal),			0,		0,		1000}, 
	{REGISTRY_STR("ScanActive"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegScanActive),		REGISTRY_SIZE(RegScanActive),			0,		0,		1000}, 
	{REGISTRY_STR("ForcedScanPeriod"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegForcedScanPeriod),		REGISTRY_SIZE(RegForcedScanPeriod),			0,		0,		1000},

	// 0: No Preference, 1: 2.4G first, 2: 5G first
	{REGISTRY_STR("PreferBand"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegPreferBand),		REGISTRY_SIZE(RegPreferBand),			0,		0,		2}, 

	// 2012/11/28 MH Add for BB team AMDPU test requiremtn.
	{REGISTRY_STR("AMfactor"), 		0,NdisParameterInteger, REGISTRY_OFFSET(RegAMfactor),		REGISTRY_SIZE(RegAMfactor),			7,		0,		0xff}, 		// AMPDU factor for HT/VHT.

	// 2012/11/28 MH Add for BB team AMDPU test requiremtn.
	{REGISTRY_STR("VHTRSec"), 		0,NdisParameterInteger, REGISTRY_OFFSET(RegVHTRSec),		REGISTRY_SIZE(RegVHTRSec),			0,		0,		0xFF}, 		// VHT support rate select.

	// Pre-transition for OID_PNP_SET_POWER OID wake up handling, added by Roger, 2012.11.28.
	{REGISTRY_STR("PreTransPnP"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegPreTransPnP),		REGISTRY_SIZE(RegPreTransPnP),			0,		0,		1}, // 0: disable, 1: enable

	//----------------------------------------------------------------------------
	// 2012/12/06 Page Add for BW setting on Advanced Settings
	// 0: 20 MHz only, 1: 20_40 MHz only, 2: 20_40_80 MHz
	{REGISTRY_STR("BWSetting"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegBWSetting),		REGISTRY_SIZE(RegBWSetting),			0,		0,		2}, 
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	{REGISTRY_STR("HWRTSEnable"),			0,NdisParameterInteger, REGISTRY_OFFSET(bRegHWRTSEnable),		REGISTRY_SIZE(bRegHWRTSEnable),		0,		0,		1},	// 0: disable, 1: enable
	//----------------------------------------------------------------------------
	{REGISTRY_STR("TxPwrPercentage"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegTxPwrPercentage),		REGISTRY_SIZE(RegTxPwrPercentage),		TX_PWR_PERCENTAGE_3,      TX_PWR_PERCENTAGE_0,       TX_PWR_PERCENTAGE_3},// 0: 12.5% TxPwr percentage, 1: 25% TxPwr percentage, 2: 50% TxPwr percentage, 3: 100% TxPwr percentage
	//----------------------------------------------------------------------------
	// Fw scan offload type. 2013.01.30, Added by tynli.
	// 0: disable, 1: D0 scan offload, 2: D3 scan offload, 3: D0 and D3 both support.
	{REGISTRY_STR("ScanOffloadType"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegScanOffloadType),		REGISTRY_SIZE(RegScanOffloadType), 	0,		0,		3},
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	// 2013/02/05 MH Add registry for Disable scan ability when linked with AP.
	{REGISTRY_STR("StreamMode"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegStreamMode),		REGISTRY_SIZE(RegStreamMode),			0,		0,		1}, 
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// 2013/02/06 MH Add Transmit power control level for all customer in the future.
	// 2013/03/07 MH ASUS request more function for 5G.
	{REGISTRY_STR("TPCLvl"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegTPCLvl),		REGISTRY_SIZE(RegTPCLvl),			0,		0,		0xFF}, 			// THis is a real dbm value one is 0.5db.
	{REGISTRY_STR("TPCLvlD"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegTPCLvlD),		REGISTRY_SIZE(RegTPCLvlD),			0,		0,		0xFF}, 			// THis is a real dbm value one is 0.5db.
	{REGISTRY_STR("TPCLvl5g"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegTPCLvl5g),		REGISTRY_SIZE(RegTPCLvl5g),			0,		0,		0xFF}, 			// THis is a real dbm value one is 0.5db.
	{REGISTRY_STR("TPCLvl5gD"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegTPCLvl5gD),		REGISTRY_SIZE(RegTPCLvl5gD),			0,		0,		0xFF}, 			// THis is a real dbm value one is 0.5db.
	//----------------------------------------------------------------------------

	//20141113 Gibson add mix protection mode
	{REGISTRY_STR("ProtectionMode"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegProtectionmode),			    REGISTRY_SIZE(RegProtectionmode),			1,      0,       1},// 0: rts/cts, 1: cts-to-self

	//20130305 Sinda add for BB adaptivity
 	{REGISTRY_STR("EnableAdaptivity"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegEnableAdaptivity),			REGISTRY_SIZE(RegEnableAdaptivity),			2,      0,       2},// 0:disable, 1: Enable, 2: Auto
 	{REGISTRY_STR("L2HForAdaptivity"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegL2HForAdaptivity),			REGISTRY_SIZE(RegL2HForAdaptivity),			0,      0,       0xFF},		// F5, F3, F1, EF pattern for customer
 	{REGISTRY_STR("HLDiffForAdaptivity"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegHLDiffForAdaptivity),			REGISTRY_SIZE(RegHLDiffForAdaptivity),		0,      0,       9},	// 7, 9 pattern for customer
	//20140124 Yu Chen add for BB CarrierSense
	{REGISTRY_STR("EnableCarrierSense"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegEnableCarrierSense),			REGISTRY_SIZE(RegEnableCarrierSense),			0,      0,       1},
	//20140710 Yu Chen add for BB NHMEnable
	{REGISTRY_STR("NHMEnable"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegNHMEnable),			REGISTRY_SIZE(RegNHMEnable),			0,      0,       1},
	//20140716 Yu Chen add for BB DynamicLink Adaptivity
	{REGISTRY_STR("DynamicLinkAdaptivity"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegDmLinkAdaptivity),			REGISTRY_SIZE(RegDmLinkAdaptivity),			0,      0,       1},

	// 0: disable, 1: enable
	{REGISTRY_STR("PacketDrop"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegPacketDrop),			REGISTRY_SIZE(RegPacketDrop),			1,      0,       1},


	//20130520 MH add for NEC check condition.
	{REGISTRY_STR("SifsThresh"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegSifsThresh),			REGISTRY_SIZE(RegSifsThresh), 		0,		0,		 0xff},


	//For HP System logo
	{REGISTRY_STR("bCustomizedScanPeriod"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegbCustomizedScanPeriod),		REGISTRY_SIZE(RegbCustomizedScanPeriod),			0,			0,		1}, // 0: disable, 1: enable
 	{REGISTRY_STR("IntelCustomizedScanPeriod"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegIntelCustomizedScanPeriod),			REGISTRY_SIZE(RegIntelCustomizedScanPeriod),			100,      1,       200},	//in ms
 	{REGISTRY_STR("AMDCustomizedScanPeriod"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegAMDCustomizedScanPeriod),			REGISTRY_SIZE(RegAMDCustomizedScanPeriod),			100,      1,       200},	//in ms
	//----------------------------------------------------------------------------

	// For Lenovo Mutual Authentication function, added by Roger, 2013.05.03.
	{REGISTRY_STR("EnableMA"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegEnableMA),		REGISTRY_SIZE(RegEnableMA),			1,			0,		1}, // 0: disable, 1: enable
	//----------------------------------------------------------------------------
	{REGISTRY_STR("VhtWeakSecurity"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegVhtWeakSecurity),		REGISTRY_SIZE(RegVhtWeakSecurity),		0,		0,		3},	// 0: disable, BIT1: tkip, BIT1: wep
	//----------------------------------------------------------------------------


	//20130614 MH add for Edimax firmware load fail workaround~!..
	{REGISTRY_STR("Fwload"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegFwload),			REGISTRY_SIZE(RegFwload), 		1,		0,		 0xf},	// 0=original location, 1=the last block use 4 bytes!

	//20151013 Sean moidy "ViaU3" to "UsbSafetySwitch"as a bit-oriented registry for USB workaround switch 
	{REGISTRY_STR("UsbSafetySwitch"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegUsbSafetySwitch),			REGISTRY_SIZE(RegUsbSafetySwitch), 		0,		0,		 0xf},	//See RT_UsbSafetySwitch_Bits

	{REGISTRY_STR("IsAMDIOIC"), 	0,NdisParameterInteger, REGISTRY_OFFSET(RegIsAMDIOIC),		REGISTRY_SIZE(RegIsAMDIOIC),			0,			0,		1}, // 0: disable, 1: enable	

	//20130822 MH add for response ack power tx agc offset for 8811au DNI case. It may be used for other IC
	{REGISTRY_STR("RspPwr"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegRspPwr),			REGISTRY_SIZE(RegRspPwr), 		0,		0,		 0xf},	// Default response ack power offset.

	// For FW support FCS
	{REGISTRY_STR("MultiChannelFcsMode"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegMultiChannelFcsMode),	REGISTRY_SIZE(RegMultiChannelFcsMode),		0,		0,		9}, 		// 0: disable, 1:enable FwFCS (ping), 2:enable FwFCS (throughput)
	{REGISTRY_STR("MultiChannelFcsNoA"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegMultiChannelFcsNoA),	REGISTRY_SIZE(RegMultiChannelFcsNoA),		40,		0,		80}, 	// Minimum: 30, Maximum: 80 (Unit: sTU)
	{REGISTRY_STR("MCCNoAStartTime"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegMCCNoAStartTime),	REGISTRY_SIZE(RegMCCNoAStartTime),		30,		10,		90}, 	// Minimum: 10, Maximum: 90 (Unit: sTU, reference GO's TBTT)
	{REGISTRY_STR("MCCStaBeaconTime"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegMCCStaBeaconTime),	REGISTRY_SIZE(RegMCCStaBeaconTime),		40,		15,		85},		// Minimum: 15, Maximum: 85 (Unit: sTU, reference GO's TBTT)
	{REGISTRY_STR("MCCQPktLevel"),		0,NdisParameterInteger,	REGISTRY_OFFSET(RegMCCQPktLevel),		REGISTRY_SIZE(RegMCCQPktLevel),			6,		0,		0x0f},		// 0: default, 1: by MacIdSleep, 2: by driver drop, 3: by macid drop

	{REGISTRY_STR("ConnectionConfigTimeIntv"),			0,NdisParameterInteger,	REGISTRY_OFFSET(RegConnectionConfigTimeIntv),		REGISTRY_SIZE(RegConnectionConfigTimeIntv),				200,					100,		500},
	{REGISTRY_STR("bScanTimeCheck"),				0,NdisParameterInteger,	REGISTRY_OFFSET(bRegScanTimeCheck),			REGISTRY_SIZE(bRegScanTimeCheck),			0,                   0,              1},

	// Set retry limit
	{REGISTRY_STR("RetryTimes"),	0,NdisParameterInteger,	REGISTRY_OFFSET(RegRetryTimes),	REGISTRY_SIZE(RegRetryTimes),		0,		0,		48}, 

	//20131004 MH add for driver indicate packet to OS layer. But in fact, this should be zero. Only OS after win7 support the feature.
	{REGISTRY_STR("PktIndicate"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegPktIndicate),			REGISTRY_SIZE(RegPktIndicate), 		0,		0,		 0xffffffff},	// Default Rx packet filter indicate type.

	//21031018 Sean add for 8812 performance mode page size adjustment, format: pubQ:HQ:LQ:NQ 
	{REGISTRY_STR("WmmPage"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegWmmPage),			REGISTRY_SIZE(RegWmmPage), 		0xDB101000,		0,		 0xffffffff},	//page size for each queue, each in one byte, pubQ:HQ:LQ:NQ .
	{REGISTRY_STR("DisableAC"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegDisableAC),			REGISTRY_SIZE(RegDisableAC), 		0,		0,		 1},   // 1: to disable AC wirlessmode
	{REGISTRY_STR("LedInterval"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegLedInterval),			REGISTRY_SIZE(RegLedInterval), 		1000,		0,		 0xffffffff},   // LED blink interval in ms for Alpha
	{REGISTRY_STR("8814auEfuse"),		0,NdisParameterInteger,  REGISTRY_OFFSET(Reg8814auEfuse),			REGISTRY_SIZE(Reg8814auEfuse), 		0,		0,		 1},	// 1 to used to update 8814au efuse 0x0E bit 4 to 1 
	// 8814A TX: 3/2/1/0/AUX=BIT7/6/5/4 RX: 3/2/1/0/AUX=BIT3/2/1/0 
	// For example RF= 3T4R = 0xEF=239, RF= 3T3R = 0xEE = 238 RF=2T2R = b01100110 = 0x66 = 102. RF = 1T1R = 0x22 = 34
	//8814aValidRFPath	BIT7	BIT6	BIT5	BIT4	BIT3	BIT2	BIT1	BIT0	HEX	Decimal	Remark
	//RF-TX				RF-RX						
	//		3	2	1	AUX	3	2	1	AUX			
	// 3T4R	1	1	1	0	1	1	1	1	0xEF	239	 3T4R  default setting
	// 3T3R	1	1	1	0	1	1	1	0	0xEE	238	
	// 2T2R	0	1	1	0	0	1	1	0	0x66	102	
	// 1T1R	0	0	1	0	0	0	1	0	0x22	34	
											
	// 4T4R	1	1	1	1	1	1	1	1	0xFF	255	Revise Reg to support
	// Use WlanCLI_20150326 to execute write reg dowrd 93c 003c0642 / write reg dword 940 093f93f0										
	// 2T4R	0	1	1	0	1	1	1	1	0x6F	111	sample
	// ?T?R											
	{REGISTRY_STR("ValidRFPath"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegValidRFPath),			REGISTRY_SIZE(RegValidRFPath), 		0,		0,		 0xff},
	{REGISTRY_STR("PreInitMem"),		0,NdisParameterInteger,  REGISTRY_OFFSET(RegPreInitMem),			REGISTRY_SIZE(RegPreInitMem), 		0,		0,		 1},
	{REGISTRY_STR("88EIOTAction"),		0,NdisParameterInteger,  REGISTRY_OFFSET(Reg88EIOTAction),			REGISTRY_SIZE(Reg88EIOTAction), 		0,		0,		 1},

	//----------------------------------------------------------------------------
	{REGISTRY_STR("bcndelay"),	0,NdisParameterInteger,  REGISTRY_OFFSET(Regbcndelay),		REGISTRY_SIZE(Regbcndelay), 		0,		0,		 0xffffffff}, 	// For beacon delay mechanism,
	//----------------------------------------------------------------------------
	// Download Fw by ROM.
	{REGISTRY_STR("RomDLFwEnable"),			0,NdisParameterInteger, REGISTRY_OFFSET(RegRomDLFwEnable),			REGISTRY_SIZE(RegRomDLFwEnable), 		0,		0,		 1}, // 0: disable, 1: enable 
	//----------------------------------------------------------------------------
	// Power off by Fw. RF on/off and IPS in 32K
	{REGISTRY_STR("bFwCtrlPwrOff"), 		0,NdisParameterInteger, REGISTRY_OFFSET(RegFwCtrlPwrOff),			REGISTRY_SIZE(RegFwCtrlPwrOff),		0,		0,		 1}, // 0: disable, 1: enable 
	// Card disable, shutdown in 32K
	{REGISTRY_STR("CardDisableInLowClk"), 		0,NdisParameterInteger, REGISTRY_OFFSET(RegCardDisableInLowClk),			REGISTRY_SIZE(RegCardDisableInLowClk),		0,		0,		 1},  // 0: disable, 1: enable
	// FW IPS level. 0: not enter 32K, 1: enter 32K
	{REGISTRY_STR("FwIPSLevel"), 		0,NdisParameterInteger, REGISTRY_OFFSET(RegFwIPSLevel),			REGISTRY_SIZE(RegFwIPSLevel),		1,		0,		 1},
	//----------------------------------------------------------------------------
	// For P2P auto channel selection mechanism, added by Roger, 2014.05.15.
#if(AUTO_CHNL_SEL_NHM == 1)
	{REGISTRY_STR("AutoChnlSel"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegAutoChnlSel),			REGISTRY_SIZE(RegAutoChnlSel),		0,		0,		 1},  // 0: disable, 1: enable
#endif	


	// WDI LE Hang Detection and Recovery, added by Roger.
	{REGISTRY_STR("HangDetection"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegHangDetection),			REGISTRY_SIZE(RegHangDetection),		0,		0,		 1},  // 0: disable, 1: enable

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	// Rx power state workaround for SDIO. Added by tynli, 2014.09.09.
	{REGISTRY_STR("RxPsWA"), 	0,NdisParameterInteger, REGISTRY_OFFSET(RegRxPsWorkAround),			REGISTRY_SIZE(RegRxPsWorkAround),		0,		0,		 1},  // 0: disable, 1: enable
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// NBL wait queue workaround. Added by tynli, 2015.01.15.
	{REGISTRY_STR("NblRacingWA"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegNblRacingWA), 		REGISTRY_SIZE(RegNblRacingWA),		0,		0,		 1},  // 0: disable, 1: enable

	//----------------------------------------------------------------------------
	// SDIO polling interrupt in interrupt handler. Added by tynli, 2015.01.16.
	{REGISTRY_STR("SdioPollIntrHandler"), 		0,NdisParameterInteger, REGISTRY_OFFSET(RegSdioPollIntrHandler),			REGISTRY_SIZE(RegSdioPollIntrHandler),		0,		0,		 1}, // 0: disable, 1: enable 	
	{REGISTRY_STR("SdioIntrPollingLimit"), 	0,NdisParameterInteger, REGISTRY_OFFSET(RegSdioIntrPollingLimit),		REGISTRY_SIZE(RegSdioIntrPollingLimit),		50,		0,		 0xffff}, // polling count 
	//----------------------------------------------------------------------------

	//----------------------------------------------------------------------------
	// Specifies whether to disable P2P_SUPPORT_STATE_RTK_SUPPORT, default is 2 (auto)
	// 		0: not to disable rtk supported p2p, 
	// 		1: to disable rtk supported p2p,
	//		2: auto
	{REGISTRY_STR("DisableRtkSupportedP2P"), 0, NdisParameterInteger, REGISTRY_OFFSET(RegDisableRtkSupportedP2P), REGISTRY_SIZE(RegDisableRtkSupportedP2P), 2, 0, 2},  
	//----------------------------------------------------------------------------

	// 20150715 MH N6 Send packet complete immediately for small each TX packet like multicast RTP script.
	// Bit15 = enable=1 or disable=0 biit 0~14 copy for local buffer number
	// According to 8192EU test in XP/win7 with multicast small each send packet, the performance can improve as USB-559 description.
	//The registry is the ratio to send packet by copy packet after receivein the NDIS packet or NBL buffer.	
	{REGISTRY_STR("TxSendAsap"),	0,NdisParameterInteger, REGISTRY_OFFSET(RegTxSendAsap), 		REGISTRY_SIZE(RegTxSendAsap),		0x0000,		0,		 0xffff},  // Bit15 = enable=1 or disable=0 biit 0~14 copy for local buffer number

	// MAC Address Randomization WDI 
	{REGISTRY_STR("SupportMACRandom"),	0,NdisParameterInteger, REGISTRY_OFFSET(RegSupportMACRandom), 		REGISTRY_SIZE(RegSupportMACRandom),		0,		0,		1}, 

	// Cancel and suspend programmed Timer before entering Dx low power state to resolve premature wake from Mobile SoC off, added by Roger, 2016.01.15
	{REGISTRY_STR("SuspendTimerInLowPwr"),	0,NdisParameterInteger, REGISTRY_OFFSET(RegSuspendTimerInLowPwr), 		REGISTRY_SIZE(RegSuspendTimerInLowPwr),		0,		0,		1}, 

	// eCSA support WDI 
	{REGISTRY_STR("SupportECSA"),	0,NdisParameterInteger, REGISTRY_OFFSET(RegSupportECSA), 		REGISTRY_SIZE(RegSupportECSA),		0,		0,		1},

	// Fast transition 11r WDI
	{REGISTRY_STR("SupportFT"),		0,NdisParameterInteger, REGISTRY_OFFSET(RegSupportFT), 		REGISTRY_SIZE(RegSupportFT),		0,		0,		1},
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------

};


#define COM_NUM_REG_PARAMS (((ULONG)sizeof(CommonRegTable)) / ((ULONG)sizeof(MP_REG_ENTRY)))

#endif
