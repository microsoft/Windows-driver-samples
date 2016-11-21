#ifndef __INC_HTGEN_H
#define __INC_HTGEN_H

extern u1Byte MCS_FILTER_ALL[16];
extern u1Byte	MCS_FILTER_1SS[16];

/* 2007/07/11 MH Modify the macro. Becaus STA may link with a N-AP. If we set
   STA in A/B/G mode and AP is still in N mode. The macro will be wrong. We have
   to add a macro to judge wireless mode. */
#define PICK_RATE(_nLegacyRate, _nMcsRate)	\
		(_nMcsRate==0)?(_nLegacyRate&0x7f):(_nMcsRate)

/* 2007/07/12 MH We only define legacy and HT wireless mode now. */
#define	LEGACY_WIRELESS_MODE		(WIRELESS_MODE_A|\
									 WIRELESS_MODE_B|\
									 WIRELESS_MODE_G)

#define CURRENT_RATE(WirelessMode, LegacyRate, HTRate)	\
					((WirelessMode & LEGACY_WIRELESS_MODE)!=0)?\
					(LegacyRate):\
					(PICK_RATE(LegacyRate, HTRate))


#define	RATE_ADPT_MCS32_MASK		0x01

#define	GET_HT_INFO(_pMgntInfo)	((PRT_HIGH_THROUGHPUT)((_pMgntInfo)->pHTInfo))

#define 	IS_UNDER_11N_AES_MODE(_ADAPTER) 	((_ADAPTER->MgntInfo.pHTInfo->bCurrentHTSupport==TRUE) &&\
												(_ADAPTER->MgntInfo.SecurityInfo.PairwiseEncAlgorithm==RT_ENC_ALG_AESCCMP))

#define	HT_MGNT_SET_AMSDU(_ADAPTER, _MODE)\
		_ADAPTER->MgntInfo.pHTInfo->ForcedAMSDUMode = \
		(PLATFORM_LIMITED_TX_BUF_SIZE(_ADAPTER))?_ADAPTER->MgntInfo.pHTInfo->ForcedAMSDUMode:_MODE


typedef enum _AMSDU_SIZE{
	AMSDU_SIZE_UNSPECIFED = 0,
	HT_AMSDU_SIZE_4K = 3839,
	HT_AMSDU_SIZE_8K = 7935,	
	VHT_AMSDU_SIZE_4K = 3895,
	VHT_AMSDU_SIZE_8K = 7991,
	VHT_AMSDU_SIZE_11K = 11454,
}AMSDU_SIZE;


typedef enum AGGRE_SIZE{
	HT_AGG_SIZE_8K = 0,
	HT_AGG_SIZE_16K = 1,
	HT_AGG_SIZE_32K = 2,
	HT_AGG_SIZE_64K = 3,
	VHT_AGG_SIZE_128K = 4,
	VHT_AGG_SIZE_256K = 5,
	VHT_AGG_SIZE_512K = 6,
	VHT_AGG_SIZE_1024K = 7,
}AGGRE_SIZE_E, *PAGGRE_SIZE_E;


// Determines the minimum timing between the start of adjacent MPDUs within an AMPDU, measured at the PHY-SAP.
typedef enum _HT_MPDU_DENSITY{
	HT_DENSITY_NO_RESTRICTION = 0,
	HT_DENSITY_1_4US = 1, 		// 1/4us
	HT_DENSITY_1_2US = 2, 		// 1/2us
	HT_DENSITY_1US = 3, 		// 1us
	HT_DENSITY_2US = 4, 		// 2us
	HT_DENSITY_4US = 5, 		// 4us
	HT_DENSITY_8US = 6, 		// 8us
	HT_DENSITY_16US = 7, 		// 16s
}HT_MPDU_DENSITY, *PHT_MPDU_DENSITY;


// Indicate different AP vendor for IOT issue.
typedef enum _HT_IOT_PEER
{
	HT_IOT_PEER_UNKNOWN = 0,
	HT_IOT_PEER_REALTEK = 1,
	HT_IOT_PEER_REALTEK_92SE = 2,
	HT_IOT_PEER_BROADCOM = 3,
	HT_IOT_PEER_RALINK = 4,
	HT_IOT_PEER_ATHEROS = 5,
	HT_IOT_PEER_CISCO = 6,
	HT_IOT_PEER_MERU = 7,	
	HT_IOT_PEER_MARVELL = 8,
	HT_IOT_PEER_REALTEK_SOFTAP = 9,// peer is RealTek SOFT_AP, by Bohn, 2009.12.17
	HT_IOT_PEER_SELF_SOFTAP = 10, // Self is SoftAP
	HT_IOT_PEER_AIRGO = 11,
	HT_IOT_PEER_REALTEK_JAGUAR_BCUTAP = 12,
	HT_IOT_PEER_REALTEK_JAGUAR_CCUTAP = 13,	
	HT_IOT_PEER_CMW500 = 14,
	HT_IOT_PEER_MAX,
}HT_IOT_PEER_E, *PHTIOT_PEER_E;

typedef enum _HT_IOT_PEER_SUBTYPE
{
	HT_IOT_PEER_ATHEROS_DIR635 = 0,
	HT_IOT_PEER_ATHEROS_DIR655 = 1,
	HT_IOT_PEER_ATHEROS_NETGEAR_WNDAP3200 = 3,
	HT_IOT_PEER_BROADCOM_NETGEAR_WNDAP4500 = 4,
	HT_IOT_PEER_ATHEROS_NETGEAR_WNDAP3500 = 5,	
	HT_IOT_PEER_LINKSYS_E4200_V1 = 6,
	HT_IOT_PEER_TPLINK_AC1750 = 7,
	HT_IOT_PEER_RALINK_DIR300 = 8,
}HT_IOT_PEER_SUBTYPE_E, *PHTIOT_PEER_SUBTYPE_E;


// IOT Action for different AP
typedef enum _HT_IOT_ACTION{
	HT_IOT_ACT_AMSDU_ENABLE = 0x00000002,
	HT_IOT_ACT_AMSDU_AMPDU = 0x00000004,

	HT_IOT_ACT_DISABLE_EDCA_TURBO = 0x00000010,
	HT_IOT_ACT_MGNT_USE_CCK_6M = 0x00000020,
	HT_IOT_ACT_FORCED_DATA_RATE = 0x00000040,
	
	HT_IOT_ACT_FORCED_CTS2SELF = 0x00000200,
	HT_IOT_ACT_FORCED_RTS = 0x00000400,

	HT_IOT_ACT_REJECT_ADDBA_REQ = 0x00001000,
	HT_IOT_ACT_EDCA_BIAS_ON_RX = 0x00004000,

	// Joseph add temporarily.
	HT_IOT_ACT_DISABLE_HIGH_POWER = 0x00040000,
	HT_IOT_ACT_DISABLE_TX_40_MHZ = 0x00080000,

	HT_IOT_ACT_TX_NO_AGGREGATION = 0x00100000,
	HT_IOT_ACT_DISABLE_TX_2SS = 0x00200000,
	HT_IOT_ACT_DISABLE_ALL_2SS = 0x00400000,
	HT_IOT_ACT_NULL_DATA_POWER_SAVING = 0x00800000,

	// Roger sync 91su branch to trunk
	HT_IOT_ACT_DISABLE_CCK_RATE = 0x01000000,
	HT_IOT_ACT_FORCED_ENABLE_BE_TXOP = 0x02000000,
	HT_IOT_ACT_WA_IOT_Broadcom = 0x04000000,
	HT_IOT_ACT_DISABLE_RX_40MHZ_SHORT_GI = 0x08000000,

	HT_IOT_ACT_BCM_AP_RX_FAIL_RELINK = 0x10000000,
	HT_IOT_ACT_DISABLE_RX_20MHZ_SHORT_GI = 0x20000000,
	// 2011/11/03 MH Disable extension channel bandwidth indication( No combinaton in primary & ext channel).
	HT_IOT_ACT_DISABLE_EXT_CHNL_COMBINE = 0x40000000,
	// 2011/11/28 MH Disable ALL AC TXOP for WNDAP4500 3T3R broadcom ap.
	HT_IOT_ACT_DISABLE_AC_TXOP = 0x80000000,
}HT_IOT_ACTION_E, *PHT_IOT_ACTION_E;


typedef enum _RT_HT_INF0_CAP{
	RT_HT_CAP_USE_TURBO_AGGR = 0x01,
	RT_HT_CAP_USE_LONG_PREAMBLE = 0x02,
	RT_HT_CAP_USE_AMPDU = 0x04,
	RT_HT_CAP_USE_WOW = 0x8,	
	RT_HT_CAP_USE_SOFTAP = 0x10,	
	RT_HT_CAP_USE_92SE = 0x20,
	RT_HT_CAP_USE_88C_92C = 0x40,
	RT_HT_CAP_USE_AP_CLIENT_MODE = 0x80,	// AP team request to reserve this bit, by Emily
}RT_HT_INF0_CAPBILITY, *PRT_HT_INF0_CAPBILITY;


typedef enum _RT_HT_INF1_CAP{
	RT_HT_CAP_USE_VIDEO_CLIENT = 0x01,
	RT_HT_CAP_USE_JAGUAR_BCUT = 0x02,
	RT_HT_CAP_USE_JAGUAR_CCUT = 0x04,
}RT_HT_INF1_CAPBILITY, *PRT_HT_INF1_CAPBILITY;

u2Byte
HTMcsToDataRate(
	PADAPTER		Adapter,
	u2Byte			nMcsRate
	);

u2Byte
HTPeerMcsToDataRate(
	PADAPTER		Adapter,
	PRT_WLAN_STA	pEntry,
	u1Byte			nMcsRate
	);

VOID
HTSetCCKSupport(
	PADAPTER		Adapter,
	BOOLEAN			bStarter,	
	pu1Byte			pPeerHTCap
	);

VOID
HTConstructInfoElement(
	PADAPTER			Adapter,
	POCTET_STRING		posHTCap);

VOID
HTConstructCapabilityElement(
	PADAPTER			Adapter,
	POCTET_STRING		posHTInfo,
	BOOLEAN				bAssoc);


VOID
HTOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu);

VOID
HTInitializeHTInfo(
	PADAPTER	Adapter);

VOID 
HTInitializeBssDesc(
	IN	PBSS_HT		pBssHT
	);

VOID
HTParsingHTCapElement(
	PADAPTER			Adapter,
	OCTET_STRING		HTCapIE,
	PRT_WLAN_BSS		pBssDesc);

VOID
HTParsingHTInfoElement(
	PADAPTER			Adapter,
	OCTET_STRING		HTInfoIE,
	PRT_WLAN_BSS		pBssDesc);

VOID
HTGetValueFromBeaconOrProbeRsp(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu,
	PRT_WLAN_BSS		BssDesc);

VOID
ResetIOTSetting(
	PMGNT_INFO		pMgntInfo);
	
VOID
HTResetSelfAndSavePeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc);

VOID
HTUpdateSelfAndPeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
	);

VOID
HTUseDefaultSetting(
	PADAPTER			Adapter
);

BOOLEAN
HTFilterMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pSupportMCS,
	OUT	pu1Byte			pOperateMCS
);

VOID
HT_PickMCSRate(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte			pOperateMCS
	);

u1Byte
HTGetHighestMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pMCSRateSet,
	IN	pu1Byte			pMCSFilter
	);

BOOLEAN
HTCCheck(
	PADAPTER			Adapter,
	PRT_RFD				pRfd,
	POCTET_STRING		pFrame
	);

VOID
HTCheckHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	pu1Byte			pHTCap
);


BOOLEAN
HTIOTActIsCCDFsync(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
	);


BOOLEAN
HTIOTActIsForcedAMSDU8K(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
	);


VOID
IOTPeerDetermine(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
);

VOID
HTRecoverBWTo40MHz(
	IN		PADAPTER	Adapter
);

#endif
