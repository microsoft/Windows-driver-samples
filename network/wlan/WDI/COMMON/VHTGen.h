#ifndef __INC_VHTGEN_H
#define __INC_VHTGEN_H

#define	GET_VHT_INFO(_pMgntInfo)	((PRT_VERY_HIGH_THROUGHPUT)((_pMgntInfo)->pVHTInfo))

//VHT capability info
#define SET_VHT_CAPABILITY_ELE_MAX_MPDU_LENGTH(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 0, 2, _val)
#define SET_VHT_CAPABILITY_ELE_CHL_WIDTH(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 2, 2, _val)
#define SET_VHT_CAPABILITY_ELE_RX_LDPC(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 4, 1, _val)
#define SET_VHT_CAPABILITY_ELE_SHORT_GI80M(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE(_pEleStart, 5, 1, _val)
#define SET_VHT_CAPABILITY_ELE_SHORT_GI160M(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE(_pEleStart, 6, 1, _val)
#define SET_VHT_CAPABILITY_ELE_TX_STBC(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE(_pEleStart, 7, 1, _val)
#define SET_VHT_CAPABILITY_ELE_RX_STBC(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 0, 3, _val)
#define SET_VHT_CAPABILITY_ELE_SU_BFER(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 3, 1, _val) // B11
#define SET_VHT_CAPABILITY_ELE_SU_BFEE(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 4, 1, _val) // B12
#define SET_VHT_CAPABILITY_ELE_BFER_ANT_SUPP(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 5, 3, _val) // B13~B15
#define SET_VHT_CAPABILITY_ELE_SOUNDING_DIMENSIONS(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 0, 3, _val) // B16~B18
#define SET_VHT_CAPABILITY_ELE_MU_BFER(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 3, 1, _val) // B19
#define SET_VHT_CAPABILITY_ELE_MU_BFEE(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 4, 1, _val) // B20
#define SET_VHT_CAPABILITY_ELE_TXOP_PS(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 5, 1, _val)
#define SET_VHT_CAPABILITY_ELE_HTC_VHT(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 6, 1, _val)
#define SET_VHT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(_pEleStart, _val)		SET_BITS_TO_LE_2BYTE((_pEleStart)+2, 7, 3, _val) //B23~B25
#define SET_VHT_CAPABILITY_ELE_LINK_ADAPTION(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+3, 2, 2, _val) // B26~B27
#define SET_VHT_CAPABILITY_ELE_MCS_RX_MAP(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE((_pEleStart)+4, 0, 16, _val)   //B0~B15 indicate Rx MCS MAP, we write 0 to indicate MCS0~7. by page
#define SET_VHT_CAPABILITY_ELE_MCS_RX_HIGHEST_RATE(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE((_pEleStart)+6, 0, 13, _val)  
#define SET_VHT_CAPABILITY_ELE_MCS_TX_MAP(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE((_pEleStart)+8, 0, 16, _val)   //B0~B15 indicate Tx MCS MAP, we write 0 to indicate MCS0~7. by page
#define SET_VHT_CAPABILITY_ELE_MCS_TX_HIGHEST_RATE(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE((_pEleStart)+10, 0, 13, _val)  


#define GET_VHT_CAPABILITY_ELE_MAX_MPDU_LENGTH(_pEleStart)			LE_BITS_TO_1BYTE(_pEleStart, 0, 2)
#define GET_VHT_CAPABILITY_ELE_CHL_WIDTH(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 2, 2)
#define GET_VHT_CAPABILITY_ELE_RX_LDPC(_pEleStart)			LE_BITS_TO_1BYTE(_pEleStart, 4, 1)
#define GET_VHT_CAPABILITY_ELE_SHORT_GI80M(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 5, 1)
#define GET_VHT_CAPABILITY_ELE_SHORT_GI160M(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 6, 1)
#define GET_VHT_CAPABILITY_ELE_TX_STBC(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 7, 1)
#define GET_VHT_CAPABILITY_ELE_RX_STBC(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+1, 0, 3)
#define GET_VHT_CAPABILITY_ELE_SU_BFER(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+1, 3, 1)
#define GET_VHT_CAPABILITY_ELE_SU_BFEE(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+1, 4, 1)
#define GET_VHT_CAPABILITY_ELE_SU_BFEE_STS_CAP(_pEleStart)	LE_BITS_TO_2BYTE((_pEleStart)+1, 5, 3)
#define GET_VHT_CAPABILITY_ELE_SU_BFER_SOUND_DIM_NUM(_pEleStart)	LE_BITS_TO_2BYTE((_pEleStart)+2, 0, 3)
#define GET_VHT_CAPABILITY_ELE_MU_BFER(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+2, 3, 1)
#define GET_VHT_CAPABILITY_ELE_MU_BFEE(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+2, 4, 1)
#define GET_VHT_CAPABILITY_ELE_TXOP_PS(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+2, 5, 1)
#define GET_VHT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(_pEleStart)	LE_BITS_TO_2BYTE((_pEleStart)+2, 7, 3)
#define GET_VHT_CAPABILITY_ELE_RX_MCS(_pEleStart)					       ((_pEleStart)+4)
#define GET_VHT_CAPABILITY_ELE_TX_MCS(_pEleStart)					       ((_pEleStart)+8)


//VHT Operation Information Element
#define SET_VHT_OPERATION_ELE_CHL_WIDTH(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 0, 8, _val)
#define SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ1(_pEleStart, _val)			SET_BITS_TO_LE_2BYTE(_pEleStart+1, 0, 8, _val)
#define SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ2(_pEleStart, _val)			SET_BITS_TO_LE_2BYTE(_pEleStart+2, 0, 8, _val)
#define SET_VHT_OPERATION_ELE_BASIC_MCS_SET(_pEleStart, _val)			SET_BITS_TO_LE_2BYTE(_pEleStart+3, 0, 16, _val)

#define GET_VHT_OPERATION_ELE_CHL_WIDTH(_pEleStart)		LE_BITS_TO_1BYTE(_pEleStart,0,8)
#define GET_VHT_OPERATION_ELE_CENTER_FREQ1(_pEleStart)	LE_BITS_TO_1BYTE((_pEleStart)+1,0,8)
#define GET_VHT_OPERATION_ELE_CENTER_FREQ2(_pEleStart)     LE_BITS_TO_1BYTE((_pEleStart)+2,0,8)

//VHT Operating Mode 
#define SET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE(_pEleStart, 0, 2, _val)
#define SET_VHT_OPERATING_MODE_FIELD_RX_NSS(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 4, 3, _val)
#define SET_VHT_OPERATING_MODE_FIELD_RX_NSS_TYPE(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE(_pEleStart, 7, 1, _val)
#define GET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(_pEleStart)			LE_BITS_TO_1BYTE(_pEleStart, 0, 2)
#define GET_VHT_OPERATING_MODE_FIELD_RX_NSS(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 4, 3)
#define GET_VHT_OPERATING_MODE_FIELD_RX_NSS_TYPE(_pEleStart)		LE_BITS_TO_1BYTE(_pEleStart, 7, 1)

#define SET_EXT_CAPABILITY_ELE_OP_MODE_NOTIF(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+7, 6, 1, _val)
#define GET_EXT_CAPABILITY_ELE_OP_MODE_NOTIF(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+7, 6, 1)

// VHT GID Management Frame Info.
#define GET_VHT_GID_MGNT_INFO_MEMBERSHIP_STATUS(_pStart)				LE_BITS_TO_1BYTE((_pStart), 0, 8)
#define GET_VHT_GID_MGNT_INFO_USER_POSITION(_pStart)					LE_BITS_TO_1BYTE((_pStart), 0, 8)

// VHT Beamforming Report Poll
#define SET_VHT_BF_REPORT_POLL_RA(_hdr, _val)							cpMacAddr(((pu1Byte)(_hdr))+4, (pu1Byte)(_val))
#define SET_VHT_BF_REPORT_POLL_TA(_hdr, _val)							cpMacAddr(((pu1Byte)(_hdr))+10, (pu1Byte)(_val))
#define SET_VHT_BF_REPORT_POLL_FEEDBACK_SEG_RETRAN_BITMAP(_hdr, _val)	SET_BITS_TO_LE_1BYTE((_hdr)+16, 0, 8, (_val))

typedef enum _VHT_DATA_SC{
	VHT_DATA_SC_DONOT_CARE = 0,
	VHT_DATA_SC_20_UPPER_OF_80MHZ = 1,
	VHT_DATA_SC_20_LOWER_OF_80MHZ = 2,
	VHT_DATA_SC_20_TOP_OF_80MHZ = 3,
	VHT_DATA_SC_20_BOTTOM_OF_80MHZ = 4,
	VHT_DATA_SC_20_RECV1 = 5,
	VHT_DATA_SC_20_RECV2 = 6,
	VHT_DATA_SC_20_RECV3 = 7,
	VHT_DATA_SC_20_RECV4 = 8,
	VHT_DATA_SC_40_UPPER_OF_80MHZ = 9,
	VHT_DATA_SC_40_LOWER_OF_80MHZ = 10,
}VHT_DATA_SC, *PVHT_DATA_SC_E;

typedef enum _PROTECTION_MODE{
	PROTECTION_MODE_AUTO = 0,
	PROTECTION_MODE_FORCE_ENABLE = 1,
	PROTECTION_MODE_FORCE_DISABLE = 2,
}PROTECTION_MODE, *PPROTECTION_MODE;

typedef enum _RT_BEAMFORMING_PKT_TYPE{
	RT_BF_PKT_TYPE_NONE = 0,
	RT_BF_PKT_TYPE_UNICAST_NDPA = 1,
	RT_BF_PKT_TYPE_BROADCAST_NDPA = 2,
	RT_BF_PKT_TYPE_BF_REPORT_POLL = 3,
	RT_BF_PKT_TYPE_FINAL_BF_REPORT_POLL = 4,
}RT_BEAMFORMING_PKT_TYPE;

#define	LDPC_VHT_ENABLE_RX			BIT0
#define	LDPC_VHT_ENABLE_TX			BIT1
#define	LDPC_VHT_TEST_TX_ENABLE		BIT2
#define	LDPC_VHT_CAP_TX				BIT3

#define	STBC_VHT_ENABLE_RX			BIT0
#define	STBC_VHT_ENABLE_TX			BIT1
#define	STBC_VHT_TEST_TX_ENABLE		BIT2
#define	STBC_VHT_CAP_TX				BIT3


#define	BEAMFORMING_VHT_BEAMFORMER_ENABLE	BIT0	// Declare our NIC supports beamformer
#define	BEAMFORMING_VHT_BEAMFORMEE_ENABLE	BIT1	// Declare our NIC supports beamformee
#define	BEAMFORMING_VHT_MU_MIMO_AP_ENABLE	BIT2	// Declare our NIC support MU-MIMO AP mode
#define	BEAMFORMING_VHT_MU_MIMO_STA_ENABLE	BIT3 // Declare our NIC support MU-MIMO STA mode
#define	BEAMFORMING_VHT_BEAMFORMER_TEST		BIT4	// Transmiting Beamforming no matter the target supports it or not
#define	BEAMFORMING_VHT_BEAMFORMER_STS_CAP		(BIT8|BIT9|BIT10)	// Asoc rsp cap
#define	BEAMFORMING_VHT_BEAMFORMEE_SOUND_DIM		(BIT12|BIT13|BIT14)	// Asoc rsp cap

//
// VHT Group ID (GID) Management Frame
//
#define FRAME_OFFSET_VHT_GID_MGNT_CATEGORY  (sMacHdrLng + 0)
#define FRAME_OFFSET_VHT_GID_MGNT_ACTION  (sMacHdrLng + 1)
#define FRAME_OFFSET_VHT_GID_MGNT_MEMBERSHIP_STATUS_ARRAY  (sMacHdrLng + 2)
#define FRAME_OFFSET_VHT_GID_MGNT_USER_POSITION_ARRAY  (sMacHdrLng + 10)

typedef struct _RT_VERY_HIGH_THROUGHPUT{
	DECLARE_RT_OBJECT(_RT_VERY_HIGH_THROUGHPUT);

	BOOLEAN				bEnableVHT;
	BOOLEAN				bCurrentVHTSupport;

	BOOLEAN				bRegBW80MHz;				// Tx 80MHz channel capablity

	BOOLEAN				bRegShortGI80MHz;			// Capability setting for Tx Short GI for 80MHz
	BOOLEAN				bCurShortGI80MHz;			// Tx Short GI for 80MHz

	u1Byte				VhtLdpcCap;					// Capability combination of VHT LDPC LDPC_VHT_XXX 
	u1Byte				VhtCurLdpc;					// Current combination of VHT LDPC LDPC_VHT_XXX of target connection supports LDPC

	u1Byte				VhtStbcCap;					// Capability combination of STBC_VHT_XXX.
	u1Byte				VhtCurStbc;					// Current setting of STBC

	u2Byte				VhtBeamformCap;				// Capability combination of VHT LDPC LDPC_VHT_XXX 
	u2Byte				VhtCurBeamform;				// Current combination of BEAMFORMING_VHT_XXX in BSS connection 
	
	u1Byte				AMPDU_Len;					// Maximum A-MPUD Length Exponent

	// VHT related information for "Self"
	u1Byte				SelfVHTCap[26];				//HT_CAPABILITY_ELE	SelfHTCap;					// This is HT cap element sent to peer STA, which also indicate HT Rx capabilities.
	u1Byte				SelfVHTInfo[22];				// This is HT info element sent to peer STA, which also indicate HT Rx capabilities.

	// VHT related information for "Peer"
	u1Byte				PeerVHTCapBuf[32];
	u1Byte				PeerVHTInfoBuf[32];
	
	// 40MHz Channel Offset settings.
	CHANNEL_WIDTH		PeerChnlBW;
	
	// Operating Mode Notification
	BOOLEAN				bOpModeNotif;
	CHANNEL_WIDTH		BWToSwitch;
	u1Byte				RxSSToSwitch;
	BOOLEAN				RxSSTypeBfmeeToSwitch;

	// Sigma related
	BOOLEAN				bFixedRTSBW;
	CHANNEL_WIDTH		DynamicRTSBW;
	u1Byte				BWSignalingCtrl;
	u1Byte				nTxSPStream;
	u1Byte				nRxSPStream;
	BOOLEAN				bUpdateMcsCap;
	u1Byte				McsCapability;
	BOOLEAN				bAssignTxLGIRate;
	u2Byte				highestTxLGIRate;
	CHANNEL_WIDTH		DynamicCTSBW;
}RT_VERY_HIGH_THROUGHPUT, *PRT_VERY_HIGH_THROUGHPUT;



typedef struct _RT_VHTINFO_STA_ENTRY{
	BOOLEAN			bEnableVHT;

	u1Byte			LDPC;
	u1Byte			STBC;

	BOOLEAN			bShortGI80M;

	u2Byte			VhtCurBeamform;				// Beamforming Settings.

	u2Byte			AMSDU_MaxSize;
	u1Byte			VHTRateSet[2];
}RT_VHTINFO_STA_ENTRY, *PRT_VHTINFO_STA_ENTRY;


//------------------------------------------------------------
// The Data structure is used to keep VHT related variable for "each AP"
// when card is configured as "STA mode"
//------------------------------------------------------------

typedef struct _BSS_VHT{

	BOOLEAN					bdSupportVHT;
	
	// VHT related elements
	u1Byte					bdVHTCapBuf[13];
	u2Byte					bdVHTCapLen;
	u1Byte					bdVHTOperBuf[6];
	u2Byte					bdVHTOperLen;
	u1Byte					bdVHTOpModeNotifBuf[1];
	u2Byte					bdVHTOpModeNotifLen;
}BSS_VHT, *PBSS_VHT;


u2Byte
VHTMcsToDataRate(
	PADAPTER		Adapter,
	u2Byte			nMcsRate
);

u1Byte
RateToSpatialStream(
	pu1Byte			pVHTRate
);


u1Byte
VHTIOTActIsAMsduEnable(
	PADAPTER		Adapter
);


VOID
VHTConstructOpModeNotification(
	PADAPTER			Adapter,
	pu1Byte 				Addr,
	pu1Byte 				Buffer,
	pu4Byte 				pLength
	);

VOID
VHTConstructCapabilityElement(
	PADAPTER			Adapter,
	POCTET_STRING		posVHTInfo,
	BOOLEAN				bAssoc
);


VOID
VHTConstructOperationElement(
	PADAPTER		Adapter,
	POCTET_STRING	posVHTOperation
);

VOID
VHTSendOperatingModeNotification(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr
	);

u1Byte
VHTGetHighestMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pVHTMCSRateSet
	);

RT_STATUS
VHT_OnOpModeNotify(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

VOID
VHTOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu);

VOID
VHTInitializeVHTInfo(
	PADAPTER	Adapter
);

VOID
VHTCheckVHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	pu1Byte			pVHTCap
);

BOOLEAN
VHTModeSupport(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu
);

VOID
VHTGetValueFromBeaconOrProbeRsp(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu,
	PRT_WLAN_BSS		bssDesc
);

VOID
VHTResetSelfAndSavePeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
);


VOID
VHTUseDefaultSetting(
	PADAPTER			Adapter
);


VOID
VHTUpdateSelfAndPeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
	);

VOID
VHTMaskSuppDataRate(
	pu1Byte			pVHTRate,
	u1Byte			maxSS,
	u1Byte			mcsCap
	);

#endif
