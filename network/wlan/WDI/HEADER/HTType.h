#ifndef __INC_HTTYPE_H
#define __INC_HTTYPE_H


//------------------------------------------------------------
// The HT Capability element is present in beacons, association request, 
//	reassociation request and probe response frames
//------------------------------------------------------------

//
// Operation mode value
//
#define HT_OPMODE_NO_PROTECT		0
#define HT_OPMODE_OPTIONAL		1
#define HT_OPMODE_40MHZ_PROTECT	2
#define HT_OPMODE_MIXED			3

//
// MIMO Power Save Setings
//
#define MIMO_PS_STATIC				0
#define MIMO_PS_DYNAMIC			1
#define MIMO_PS_NOLIMIT			3


#define sHTCLng	4


/* 2007/06/07 MH Define sub-carrier mode for 40MHZ. */
typedef enum _HT_Bandwidth_40MHZ_Sub_Carrier{
	SC_MODE_DUPLICATE = 0,
	SC_MODE_LOWER = 1,
	SC_MODE_UPPER = 2,
	SC_MODE_FULL40MHZ = 3,
}HT_BW40_SC_E;

//HT capability info
#define SET_HT_CAPABILITY_ELE_LDPC_CAP(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 0, 1, _val)
#define SET_HT_CAPABILITY_ELE_CHL_WIDTH(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE(_pEleStart, 1, 1, _val)
#define SET_HT_CAPABILITY_ELE_MIMO_PWRSAVE(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE(_pEleStart, 2, 2, _val)
#define SET_HT_CAPABILITY_ELE_GREEN_FIELD(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE(_pEleStart, 4, 1, _val)
#define SET_HT_CAPABILITY_ELE_SHORT_GI20M(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE(_pEleStart, 5, 1, _val)
#define SET_HT_CAPABILITY_ELE_SHORT_GI40M(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE(_pEleStart, 6, 1, _val)
#define SET_HT_CAPABILITY_ELE_TX_STBC(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE(_pEleStart, 7, 1, _val)

#define SET_HT_CAPABILITY_ELE_RX_STBC(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 0, 2, _val)
#define SET_HT_CAPABILITY_ELE_DELAY_BA(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 2, 1, _val)
#define SET_HT_CAPABILITY_ELE_MAX_AMSDU_SIZE(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 3, 1, _val)
#define SET_HT_CAPABILITY_ELE_DSS_CCK(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 4, 1, _val)
#define SET_HT_CAPABILITY_ELE_PSMP(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 5, 1, _val)
#define SET_HT_CAPABILITY_ELE_FORTY_INTOLERANT(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 6, 1, _val)
#define SET_HT_CAPABILITY_ELE_LSIG_TXOP_PROTECT(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 7, 1, _val)

#define GET_HT_CAPABILITY_ELE_LDPC_CAP(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 0, 1)
#define GET_HT_CAPABILITY_ELE_CHL_WIDTH(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 1, 1)
#define GET_HT_CAPABILITY_ELE_MIMO_PWRSAVE(_pEleStart)			LE_BITS_TO_1BYTE(_pEleStart, 2, 2)
#define GET_HT_CAPABILITY_ELE_GREEN_FIELD(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 4, 1)
#define GET_HT_CAPABILITY_ELE_SHORT_GI20M(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 5, 1)
#define GET_HT_CAPABILITY_ELE_SHORT_GI40M(_pEleStart)				LE_BITS_TO_1BYTE(_pEleStart, 6, 1)
#define GET_HT_CAPABILITY_ELE_TX_STBC(_pEleStart)					LE_BITS_TO_1BYTE(_pEleStart, 7, 1)

#define GET_HT_CAPABILITY_ELE_RX_STBC(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+1, 0, 2)
#define GET_HT_CAPABILITY_ELE_DELAY_BA(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+1, 2, 1)
#define GET_HT_CAPABILITY_ELE_MAX_AMSDU_SIZE(_pEleStart)			LE_BITS_TO_1BYTE((_pEleStart)+1, 3, 1)
#define GET_HT_CAPABILITY_ELE_DSS_CCK(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+1, 4, 1)
#define GET_HT_CAPABILITY_ELE_PSMP(_pEleStart)						LE_BITS_TO_1BYTE((_pEleStart)+1, 5, 1)
#define GET_HT_CAPABILITY_ELE_FORTY_INTOLERANT(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart)+1, 6, 1)
#define GET_HT_CAPABILITY_ELE_LSIG_TXOP_PROTECT(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart)+1, 7, 1)

//MAC HT parameters info
#define SET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 0, 2, _val)
#define SET_HT_CAPABILITY_ELE_MPDU_DENSITY(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 2, 3, _val)
#define SET_HT_CAPABILITY_ELE_RSVD2(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 5, 3, _val)

#define GET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(_pEleStart)	LE_BITS_TO_1BYTE((_pEleStart)+2, 0, 2)
#define GET_HT_CAPABILITY_ELE_MPDU_DENSITY(_pEleStart)			LE_BITS_TO_1BYTE((_pEleStart)+2, 2, 3)
#define GET_HT_CAPABILITY_ELE_RSVD2(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+2, 5, 3)

//Supported MCS set
#define GET_HT_CAPABILITY_ELE_MCS(_pEleStart)						((_pEleStart)+3)
			
//Extended HT Capability Info	
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO(_pEleStart, _val)					WriteEF2Byte(((pu1Byte)(_pEleStart))+19, _val)
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO(_pEleStart)						ReadEF2Byte(((pu1Byte)(_pEleStart))+19)

#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_PCO(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 0, 1, ((u1Byte)_val) )
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_PCO_TIME(_pEleStart, _val)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 1, 2, ((u1Byte)_val) )
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_RSVD3(_pEleStart, _val)			SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 3, 5, ((u1Byte)_val) )
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_MCS_FEEDBACK(_pEleStart, _val)	SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 8, 2, ((u1Byte)_val) )
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_HTC(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 10, 1, ((u1Byte)_val) )
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_RDR(_pEleStart, _val)				SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 11, 1, ((u1Byte)_val) )
#define SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_RSVD4(_pEleStart, _val)			SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pEleStart))+19, 12, 4, ((u1Byte)_val) )

#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_PCO(_pEleStart)					((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 0, 1))
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_PCO_TIME(_pEleStart)			((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 1, 2))
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_RSVD3(_pEleStart)				((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 3, 5))
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_MCS_FEEDBACK(_pEleStart)		((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 8, 2))
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_HTC(_pEleStart)					((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 10, 1))
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_RDR(_pEleStart)					((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 11, 1))
#define GET_HT_CAPABILITY_ELE_EXT_HTCAPINFO_RSVD4(_pEleStart)				((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pEleStart))+19, 12, 4))

		
//TXBF Capabilities
#define SET_HT_CAPABILITY_ELE_TXBF_CAP(_pEleStart, _val)					WriteEF4Byte(((pu1Byte)(_pEleStart))+21, _val)
#define SET_HT_CAP_TXBF_RECEIVE_NDP_CAP(_pEleStart, _val)					SET_BITS_TO_LE_4BYTE( ((pu1Byte)(_pEleStart))+21, 3, 1, ((u1Byte)_val) )
#define SET_HT_CAP_TXBF_TRANSMIT_NDP_CAP(_pEleStart, _val)				SET_BITS_TO_LE_4BYTE( ((pu1Byte)(_pEleStart))+21, 4, 1, ((u1Byte)_val) )
#define SET_HT_CAP_TXBF_EXPLICIT_COMP_STEERING_CAP(_pEleStart, _val)		SET_BITS_TO_LE_4BYTE( ((pu1Byte)(_pEleStart))+21, 10, 1, ((u1Byte)_val) )
#define SET_HT_CAP_TXBF_EXPLICIT_COMP_FEEDBACK_CAP(_pEleStart, _val)		SET_BITS_TO_LE_4BYTE( ((pu1Byte)(_pEleStart))+21, 15, 2, ((u1Byte)_val) )
#define SET_HT_CAP_TXBF_COMP_STEERING_NUM_ANTENNAS(_pEleStart, _val)	SET_BITS_TO_LE_4BYTE( ((pu1Byte)(_pEleStart))+21, 23, 2, ((u1Byte)_val) )
#define SET_HT_CAP_TXBF_CHNL_ESTIMATION_NUM_ANTENNAS(_pEleStart, _val)	SET_BITS_TO_LE_4BYTE( ((pu1Byte)(_pEleStart))+21, 27, 2, ((u1Byte)_val) )


#define GET_HT_CAPABILITY_ELE_TXBF_CAP(_pEleStart)						ReadEF4Byte(((pu1Byte)(_pEleStart))+21)
#define GET_HT_CAP_TXBF_EXPLICIT_COMP_STEERING_CAP(_pEleStart)			LE_BITS_TO_4BYTE((_pEleStart)+21, 10, 1)
#define GET_HT_CAP_TXBF_EXPLICIT_COMP_FEEDBACK_CAP(_pEleStart)			LE_BITS_TO_4BYTE((_pEleStart)+21, 15, 2)
#define GET_HT_CAP_TXBF_COMP_STEERING_NUM_ANTENNAS(_pEleStart)	LE_BITS_TO_4BYTE((_pEleStart)+21, 23, 2)
#define GET_HT_CAP_TXBF_CHNL_ESTIMATION_NUM_ANTENNAS(_pEleStart)	LE_BITS_TO_4BYTE((_pEleStart)+21, 27, 2)


//Antenna Selection Capabilities
#define SET_HT_CAPABILITY_ELE_AS_CAP(_pEleStart, _val)				WriteEF1Byte(((pu1Byte)(_pEleStart))+25, _val)
#define GET_HT_CAPABILITY_ELE_AS_CAP(_pEleStart)					ReadEF1Byte(((pu1Byte)(_pEleStart))+25)


//------------------------------------------------------------
// The HT Information element
//------------------------------------------------------------
#define SET_HT_INFO_ELE_CONTROL_CHL(_pEleStart, _val)				WriteEF1Byte(_pEleStart, _val)
#define GET_HT_INFO_ELE_CONTROL_CHL(_pEleStart)					ReadEF1Byte(_pEleStart)

#define SET_HT_INFO_ELE_EXT_CHL_OFFSET(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 0, 2, _val)
#define SET_HT_INFO_ELE_STA_CHL_WIDTH(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 2, 1, _val)
#define SET_HT_INFO_ELE_RIFS(_pEleStart, _val)						SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 3, 1, _val)
#define SET_HT_INFO_ELE_PSMP_ACCESS_ONLY(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 4, 1, _val)
#define SET_HT_INFO_ELE_SRV_INT_GRAN(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+1, 5, 3, _val)

#define GET_HT_INFO_ELE_EXT_CHL_OFFSET(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+1, 0, 2)
#define GET_HT_INFO_ELE_STA_CHL_WIDTH(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+1, 2, 1)
#define GET_HT_INFO_ELE_RIFS(_pEleStart)							LE_BITS_TO_1BYTE((_pEleStart)+1, 3, 1)
#define GET_HT_INFO_ELE_PSMP_ACCESS_ONLY(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+1, 4, 1)
#define GET_HT_INFO_ELE_SRV_INT_GRAN(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+1, 5, 3)

#define SET_HT_INFO_ELE_OPT_MODE(_pEleStart, _val)					SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 0, 2, _val)
#define SET_HT_INFO_ELE_NON_GF_DEV(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 2, 1, _val)

#define GET_HT_INFO_ELE_OPT_MODE(_pEleStart)						LE_BITS_TO_1BYTE((_pEleStart)+2, 0, 2)
#define GET_HT_INFO_ELE_OBSS_NON_HT_MEMBER(_pEleStart)			LE_BITS_TO_1BYTE((_pEleStart)+2, 4, 1)
#define GET_HT_INFO_ELE_NON_GF_DEV(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+2, 2, 1)
#define GET_HT_INFO_ELE_CHANNEL(_pEleStart)						LE_BITS_TO_1BYTE((_pEleStart)+0, 0, 8)

#define SET_HT_INFO_ELE_DUAL_BEACON(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+4, 6, 1, _val)
#define SET_HT_INFO_ELE_DUAL_CTS_PROTECT(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE((_pEleStart)+4, 7, 1, _val)

#define GET_HT_INFO_ELE_DUAL_BEACON(_pEleStart, _val)				LE_BITS_TO_1BYTE((_pEleStart)+4, 6, 1)
#define GET_HT_INFO_ELE_DUAL_CTS_PROTECT(_pEleStart, _val)		LE_BITS_TO_1BYTE((_pEleStart)+4, 7, 1)

#define SET_HT_INFO_ELE_SECONDARY_BEACON(_pEleStart, _val)		SET_BITS_TO_LE_1BYTE((_pEleStart)+5, 0, 1, _val)
#define SET_HT_INFO_ELE_LSIG_TXOP_PROTECT_FULL(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart)+5, 1, 1, _val)
#define SET_HT_INFO_ELE_PCO_ACTIVE(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+5, 2, 1, _val)
#define SET_HT_INFO_ELE_PCO_PHASE(_pEleStart, _val)				SET_BITS_TO_LE_1BYTE((_pEleStart)+5, 3, 1, _val)

#define GET_HT_INFO_ELE_SECONDARY_BEACON(_pEleStart)				LE_BITS_TO_1BYTE((_pEleStart)+5, 0, 1)
#define GET_HT_INFO_ELE_LSIG_TXOP_PROTECT_FULL(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart)+5, 1, 1)
#define GET_HT_INFO_ELE_PCO_ACTIVE(_pEleStart)						LE_BITS_TO_1BYTE((_pEleStart)+5, 2, 1)
#define GET_HT_INFO_ELE_PCO_PHASE(_pEleStart)						LE_BITS_TO_1BYTE((_pEleStart)+5, 3, 1)

#define GET_HT_INFO_ELE_BASIC_MCS(_pEleStart)						((_pEleStart)+6)


// MIMO Power Save control field.
// This is appear in MIMO Power Save Action Frame
#define GET_MIMOPS_FRAME_PS_ENABLE(_pStart)			((u1Byte)LE_BITS_TO_1BYTE( ((pu1Byte)(_pStart))+sMacHdrLng+2, 0, 1))
#define GET_MIMOPS_FRAME_PS_MODE(_pStart)			((u1Byte)LE_BITS_TO_1BYTE( ((pu1Byte)(_pStart))+sMacHdrLng+2, 1, 1))


//------------------------------------------------------------
// The HT Control field
//------------------------------------------------------------
#define SET_HT_CTRL_CSI_STEERING(_pEleStart, _val)					SET_BITS_TO_LE_1BYTE((_pEleStart)+2, 6, 2, _val)
#define SET_HT_CTRL_NDP_ANNOUNCEMENT(_pEleStart, _val)			SET_BITS_TO_LE_1BYTE((_pEleStart)+3, 0, 1, _val)
#define GET_HT_CTRL_NDP_ANNOUNCEMENT(_pEleStart)					LE_BITS_TO_1BYTE((_pEleStart)+3, 0, 1)


typedef enum _HT_SPEC_VER{
	HT_SPEC_VER_IEEE = 0,
	HT_SPEC_VER_EWC = 1,
}HT_SPEC_VER, *PHT_SPEC_VER;


typedef enum _HT_AMPDM_MODE_E{
	HT_AMPDU_AUTO = 0,
	HT_AMPDU_ENABLE = 1,
	HT_AMPDU_DISABLE = 2,
}HT_AMPDU_MODE_E, *PHT_AMPDU_MODE_E;

typedef enum _HT_AMSDU_MODE_E{
	HT_AMSDU_AUTO = 0,
	HT_AMSDU_ENABLE = 1,
	HT_AMSDU_DISABLE = 2,
	HT_AMSDU_WITHIN_AMPDU = 3, 
}HT_AMSDU_MODE_E, *PHT_AMSDU_MODE_E;

typedef enum _SHORTGI_MODE_E{
	SHORTGI_AUTO = 0,
	SHORTGI_FORCE_ENABLE = 1,
	SHORTGI_FORCE_DISABLE = 2,
}SHORTGI_MODE_E, *PSHORTGI_MODE_E;

#define	LDPC_HT_ENABLE_RX			BIT0
#define	LDPC_HT_ENABLE_TX			BIT1
#define	LDPC_HT_TEST_TX_ENABLE		BIT2
#define	LDPC_HT_CAP_TX				BIT3

#define	STBC_HT_ENABLE_RX			BIT0
#define	STBC_HT_ENABLE_TX			BIT1
#define	STBC_HT_TEST_TX_ENABLE		BIT2
#define	STBC_HT_CAP_TX				BIT3

#define	BEAMFORMING_HT_BEAMFORMER_ENABLE	BIT0	// Declare our NIC supports beamformer
#define	BEAMFORMING_HT_BEAMFORMEE_ENABLE	BIT1	// Declare our NIC supports beamformee
#define	BEAMFORMING_HT_BEAMFORMER_TEST		BIT2	// Transmiting Beamforming no matter the target supports it or not
#define	BEAMFORMING_HT_BEAMFORMER_STEER_NUM		(BIT4|BIT5)
#define	BEAMFORMING_HT_BEAMFORMEE_CHNL_EST_CAP	(BIT6|BIT7)

//------------------------------------------------------------
//  The Data structure is used to keep HT related variables when card is 
//  configured as non-AP STA mode.  **Note**  Current_xxx should be set 
//	to default value in HTInitializeHTInfo()
//------------------------------------------------------------

typedef struct _RT_HIGH_THROUGHPUT{
	DECLARE_RT_OBJECT(_RT_HIGH_THROUGHPUT);

	BOOLEAN				bEnableHT;
	BOOLEAN				bCurrentHTSupport;

	SHORTGI_MODE_E		ForcedShortGI;
	BOOLEAN				bCurShortGI40MHz;			// Tx Short GI for 40MHz
	BOOLEAN				bCurShortGI20MHz;			// Tx Short GI for 20MHz

	u1Byte				HtLdpcCap;					// Capability combination of HT LDPC LDPC_HT_XXX 
	u1Byte				HtCurLdpc;					// Current target connection supports LDPC

	u1Byte				HtStbcCap;					// Capability combination of HT STBC_HT_XXX
	u1Byte				HtCurStbc;					// Current target connection support STBC

	u1Byte				HtBeamformCap;				// Capability combination of VHT LDPC LDPC_VHT_XXX 
	u1Byte				HtCurBeamform;				// Current combination of BEAMFORMING_VHT_XXX in BSS connection 

	BOOLEAN				bRegSuppCCK;				// Tx CCK rate capability
	BOOLEAN				bCurSuppCCK;				// Tx CCK rate capability

	// 802.11n spec version for "peer"
	HT_SPEC_VER			ePeerHTSpecVer;

	// HT related information for "Self"
	u1Byte				SelfHTCap[26];				//HT_CAPABILITY_ELE	SelfHTCap;					// This is HT cap element sent to peer STA, which also indicate HT Rx capabilities.
	u1Byte				SelfHTInfo[22];				// This is HT info element sent to peer STA, which also indicate HT Rx capabilities.

	// HT related information for "Peer"
	u1Byte				PeerHTCapBuf[32];
	u1Byte				PeerHTInfoBuf[32];

	// A-MSDU related
	BOOLEAN				bAMSDU_Support;			// This indicates Tx A-MSDU capability
	u2Byte				nAMSDU_MaxSize;			// This indicates Tx A-MSDU capability
	BOOLEAN				bCurrent_AMSDU_Support;	// This indicates Tx A-MSDU capability
	u2Byte				nCurrent_AMSDU_MaxSize;	// This indicates Tx A-MSDU capability
	
	BOOLEAN				bHWAMSDU_Support;			// This indicates Tx A-MSDU capability

	// AMPDU  related <2006.08.10 Emily>
	BOOLEAN				bAMPDUEnable;				// This indicate Tx A-MPDU capability
	BOOLEAN				bCurrentAMPDUEnable;		// This indicate Tx A-MPDU capability		
	u1Byte				AMPDU_Factor;				// This indicate Tx A-MPDU capability
	u1Byte				CurrentAMPDUFactor;			// This indicate Tx A-MPDU capability
	u1Byte				MPDU_Density;				// This indicate Tx A-MPDU capability
	u1Byte				CurrentMPDUDensity;			// This indicate Tx A-MPDU capability

	// Forced A-MPDU enable
	HT_AMPDU_MODE_E		ForcedAMPDUMode;
	u1Byte				ForcedAMPDUFactor;
	u1Byte				ForcedMPDUDensity;

	// Forced A-MSDU enable
	HT_AMSDU_MODE_E		ForcedAMSDUMode;
	u2Byte				ForcedAMSDUMaxSize;
	u1Byte				ForcedAMSDUMaxNum;
	u4Byte				AmsduCnt[5];	// For test only , should be deleted later.

	u1Byte				CurrentOpMode;

	// MIMO PS related
	u1Byte				SelfMimoPs;
	u1Byte				PeerMimoPs;
	
	// 40MHz Channel Offset settings.
	BOOLEAN				bPeer40MHzCap;					// Supported channel width set
	EXTCHNL_OFFSET		PeerExtChnlOffset;
	BOOLEAN				bPeer40MHzIntolerant;			// Forty MHz Intolerant

	// 20/40 Bss Coexistence
	BOOLEAN				bBssCoexist;					// 20/40 Bss Coexistence Management Support from self config
	BOOLEAN				bPeerBssCoexistence;			// 20/40 BSS Coexistence Management Support from AP
	BOOLEAN				bCurOBSSScanExemptionGrt;		// OBSS Scanning exemption grant
	u2Byte				CurOBSSScanInterval;			// BSS Channel Width Trigger Scan Interval
	u2Byte				IdleOBSSScanCnt;				// The scan idle count for BSS scan interval
	u8Byte				lastTimeSentObssRptUs;			// Last system time to send the Obss report

	// For Realtek proprietary A-MPDU factor for aggregation
	u1Byte				RT2RT_HT_Mode;	

	// Rx Reorder control
	BOOLEAN				bRegRxReorderEnable;
	BOOLEAN				bCurRxReorderEnable;
	u1Byte				RxReorderWinSize;
	u1Byte				RxReorderPendingTime;
	u2Byte				RxReorderDropCounter;

	// Accept ADDBA Request
	BOOLEAN				bAcceptAddbaReq;

	//RDG enable
	BOOLEAN				bRDGEnable;


	//Added for dual band 40MHz support
	BOOLEAN				bRegBW40MHzFor2G;
	BOOLEAN				bRegBW40MHzFor5G; 	

	BOOLEAN				b40Intolerant;
	BOOLEAN				bAMPDUManual;
	u1Byte				nTxSPStream;
	u1Byte				nRxSPStream;
	BOOLEAN				bRegShortGI40MHz;
	BOOLEAN				bRegShortGI20MHz;
}RT_HIGH_THROUGHPUT, *PRT_HIGH_THROUGHPUT;


//------------------------------------------------------------
// The Data structure is used to keep HT related variable for "each Sta"
// when card is configured as "AP mode"
//------------------------------------------------------------

typedef struct _RT_HTINFO_STA_ENTRY{
	BOOLEAN			bEnableHT;

	u1Byte			LDPC;
	u1Byte			STBC;

	BOOLEAN			bSupportCck;
	
	u2Byte			AMSDU_MaxSize;
	
	u1Byte			AMPDU_Factor;
	u1Byte			MPDU_Density;

	u1Byte			MimoPs;

	u1Byte			McsRateSet[16];
	u1Byte			HTHighestOperaRate;
	
	BOOLEAN			bCurRxReorderEnable;
	
	BOOLEAN			bShortGI20M;
	BOOLEAN			bShortGI40M;		

	u1Byte			HtCurBeamform;
}RT_HTINFO_STA_ENTRY, *PRT_HTINFO_STA_ENTRY;


//------------------------------------------------------------
// The Data structure is used to keep HT related variable for "each AP"
// when card is configured as "STA mode"
//------------------------------------------------------------

typedef struct _BSS_HT{

	BOOLEAN					bdSupportHT;
	
	// HT related elements
	u1Byte					bdHTCapBuf[32];
	u2Byte					bdHTCapLen;
	u1Byte					bdHTInfoBuf[32];
	u2Byte					bdHTInfoLen;

	HT_SPEC_VER				bdHTSpecVer;
	
	u1Byte					RT2RT_HT_Mode;
	
	BOOLEAN					bd40Intolerant;
	BOOLEAN					bdOBSSExemption;
	u2Byte					OBSSScanInterval;
}BSS_HT, *PBSS_HT;

typedef struct _MIMO_RSSI{
	u4Byte	EnableAntenna;
	u4Byte	AntennaA;
	u4Byte 	AntennaB;
	u4Byte 	AntennaC;
	u4Byte 	AntennaD;
	u4Byte	Average;
}MIMO_RSSI, *PMIMO_RSSI;

typedef struct _MIMO_EVM{
	u4Byte	EVM1;
	u4Byte    EVM2;
}MIMO_EVM, *PMIMO_EVM;

#endif //__INC_HTTYPE_H
