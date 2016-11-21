#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Protocol802_11.tmh"
#endif

static u2Byte sPacketIEOffsetTable[] = 
{
	sMacHdrLng + 4, 		//SubType_Asoc_Req		= 0,
	sMacHdrLng + 6, 		//SubType_Asoc_Rsp		= 1,
	sMacHdrLng + 10, 		//SubType_Reasoc_Req	= 2,
	sMacHdrLng + 6, 		//SubType_Reasoc_Rsp	= 3,
	sMacHdrLng + 0, 		//SubType_Probe_Req	= 4,
	sMacHdrLng + 12, 		//SubType_Probe_Rsp	= 5,
	0, 					// rsvd				= 6, 	x
	0, 					// rsvd				= 7,	 x
	sMacHdrLng + 12, 		//SubType_Beacon		= 8, 
	0, 					//SubType_Atim			= 9, 	x
	0, 					//SubType_Disasoc		= 10, 	x
	sMacHdrLng + 6, 		//SubType_Auth			= 11,
	0, 					//SubType_Deauth		= 12, 	x
};

// Pattern Array
// Qos (0x1)
u1Byte	PAT_ACT_QOS_ADDTSREQ[] = {0x01, 0x00};
u1Byte	PAT_ACT_QOS_ADDTSRSP[] = {0x01, 0x01};
u1Byte	PAT_ACT_QOS_DELTS[] = {0x01, 0x02};
u1Byte	PAT_ACT_QOS_SCHEDULE[] = {0x01, 0x03};
// DLS (0x2)
u1Byte	PAT_ACT_DLS_DLSREQ[] = {0x02, 0x00};
u1Byte	PAT_ACT_DLS_DLSRSP[] = {0x02, 0x01};
u1Byte	PAT_ACT_DLS_DLSTEARDOWN[] = {0x02, 0x02};
// BA (0x3)
u1Byte	PAT_ACT_BA_ADDBAREQ[] = {0x03, 0x00};
u1Byte	PAT_ACT_BA_ADDBARSP[] = {0x03, 0x01};
u1Byte	PAT_ACT_BA_DELBA[] = {0x03, 0x02};
// Public (0x4)
u1Byte	PAT_ACT_BSS_COEXIST[] = {0x04, 0x00};
u1Byte	PAT_ACT_TDLS_DISC_RSP[] = {0x04, 0x0E};
u1Byte	PAT_ACT_P2P_GO_NEG_REQ[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x00};	// P2P GO Negotiation Request
u1Byte	PAT_ACT_P2P_GO_NEG_RSP[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x01};	// P2P GO Negotiation Response
u1Byte	PAT_ACT_P2P_GO_NEG_CONF[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x02};	// P2P GO Negotiation Confirm
u1Byte	PAT_ACT_P2P_INVIT_REQ[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x03};	// P2P Invitation Request
u1Byte	PAT_ACT_P2P_INVIT_RSP[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x04};	// P2P Invitation Response
u1Byte	PAT_ACT_P2P_DEV_DISCOVERABILITY_REQ[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x05};	// P2P Device Discoverability Request
u1Byte	PAT_ACT_P2P_DEV_DISCOVERABILITY_RSP[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x06};	// P2P Device Discoverability Response
u1Byte	PAT_ACT_P2P_PROV_DISC_REQ[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x07};	// P2P Provision Discovery Request
u1Byte	PAT_ACT_P2P_PROV_DISC_RSP[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x09, 0x08};	// P2P Provision Discovery Response
u1Byte	PAT_ACT_NAN_SDF[] = {0x04, 0x09, 0x50, 0x6F, 0x9A, 0x13};	// NAN SDF
u1Byte	PAT_ACT_GAS_INT_REQ[] = {0x04, 0x0A};	// GAS Initial Request
u1Byte	PAT_ACT_GAS_INT_RSP[] = {0x04, 0x0B};	// GAS Initial Response
u1Byte	PAT_ACT_GAS_COMEBACK_REQ[] = {0x04, 0x0C};	// GAS Comeback Request
u1Byte	PAT_ACT_GAS_COMEBACK_RSP[] = {0x04, 0x0D};	// GAS Comeback Response
// Radio Measurement - 11k (0x5)
u1Byte	PAT_ACT_RM_RM_REQ[] = {0x05, 0x00};	// Radio Measurement Request
u1Byte	PAT_ACT_RM_RM_RPT[] = {0x05, 0x01};	// Radio Measurement Report
u1Byte	PAT_ACT_RM_LM_REQ[] = {0x05, 0x02};	// Link Measurement Request
u1Byte	PAT_ACT_RM_LM_RPT[] = {0x05, 0x03};	// Link Measurement Report
u1Byte	PAT_ACT_RM_NR_REQ[] = {0x05, 0x04};	// Neighbor Report Request
u1Byte	PAT_ACT_RM_NR_RSP[] = {0x05, 0x05};	// Neighbor Report Response
// High Throughput - 11n (0x7)
u1Byte	PAT_ACT_HT_NOTI_CHNL_WIDTH[] = {0x07, 0x00};	// Notify Channel Width
u1Byte	PAT_ACT_HT_SM_PS[] = {0x07, 0x01};	// SM Power Save
u1Byte	PAT_ACT_HT_PSMP[] = {0x07, 0x02};	// PSMP
u1Byte	PAT_ACT_HT_SET_PCO_PHASE[] = {0x07, 0x03};	// Set PCO Phase
u1Byte	PAT_ACT_HT_CSI[] = {0x07, 0x04};	// CSI
u1Byte	PAT_ACT_HT_NON_COMPRESS_BEAMFORMING[] = {0x07, 0x05};	// Noncompressed Beamforming
u1Byte	PAT_ACT_HT_COMPRESS_BEAMFORMING[] = {0x07, 0x06};	// Compressed Beamforming
u1Byte	PAT_ACT_HT_ASEL_FEEDBACK[] = {0x07, 0x07};	// ASEL Indices Feedback
// SA Query (0x8)
u1Byte	PAT_ACT_SA_QUERY_REQ[] = {0x08, 0x00};	// SA Query Request
// TDLS-11z (0x0C)
u1Byte	PAT_ACT_TDLS_REQ[] = {0x0C, 0x00};	// TDLS Setup Request
u1Byte	PAT_ACT_TDLS_RSP[] = {0x0C, 0x01};	// TDLS Setup Response
u1Byte	PAT_ACT_TDLS_CONFIRM[] = {0x0C, 0x02};	// TDLS Setup Confirm
u1Byte	PAT_ACT_TDLS_TEARDOWN[] = {0x0C, 0x03};	// TDLS Setup Teardown
u1Byte	PAT_ACT_TDLS_PEER_TRAFIC_IND[] = {0x0C, 0x04};	// TDLS Peer Traffic Indication
u1Byte	PAT_ACT_TDLS_CHNL_SW_REQ[] = {0x0C, 0x05};	// TDLS Channel Switch Request
u1Byte	PAT_ACT_TDLS_CHNL_SW_RSP[] = {0x0C, 0x06};	// TDLS Channel Switch Response
u1Byte	PAT_ACT_TDLS_PEER_PSM_REQ[] = {0x0C, 0x07};	// TDLS Peer PSM Request
u1Byte	PAT_ACT_TDLS_PEER_PSM_RSP[] = {0x0C, 0x08};	// TDLS Peer PSM Response
u1Byte	PAT_ACT_TDLS_PEER_TRAFIC_RSP[] = {0x0C, 0x09};	// TDLS Peer Traffic Response
u1Byte	PAT_ACT_TDLS_DISC_REQ[] = {0x0C, 0x0A};	// TDLS Discovery Request
// WMM (0x11)
u1Byte	PAT_ACT_WMM_ADDTSREQ[] = {0x11, 0x00};
u1Byte	PAT_ACT_WMM_ADDTSRSP[] = {0x11, 0x01};
u1Byte	PAT_ACT_WMM_DELTS[] = {0x11, 0x02};
// VHT (0x15)
u1Byte	PAT_ACT_VHT_COMPRESSED_BEAMFORMING[] = {0x15, 0x00};
u1Byte	PAT_ACT_VHT_GROUPID_MANAGEMENT[] = {0x15, 0x01};
u1Byte	PAT_ACT_VHT_OPMODE_NOTIFICATION[] = {0x15, 0x02};
//Vendor Specific (0x7F)
u1Byte	PAT_ACT_P2P_NOA[] = {0x7F, 0x50, 0x6F, 0x9A, 0x09, 0x00};	// P2P Notice of Absence
u1Byte	PAT_ACT_P2P_PRESENCE_REQ[] = {0x7F, 0x50, 0x6F, 0x9A, 0x09, 0x01};	// P2P Presence Request
u1Byte	PAT_ACT_P2P_PRESENCE_RSP[] = {0x7F, 0x50, 0x6F, 0x9A, 0x09, 0x02};	// P2P Presence Response
u1Byte	PAT_ACT_P2P_GO_DISCOVERABILITY_REQ[] = {0x7F, 0x50, 0x6F, 0x9A, 0x09, 0x03};	// P2P GO Discoverability Request

#define	FILL_PATTERN_MAP(_Type, _Pattern)	{_Type, sizeof(_Pattern), _Pattern}

// The pattern array of action frames.
// By Bruce, 2012-03-09.
PKT_PATTERN_MAP	PktActPatternsMap[] =
{
	// Qos (0x1)
	FILL_PATTERN_MAP(ACT_PKT_QOS_ADDTSREQ, PAT_ACT_QOS_ADDTSREQ),
	FILL_PATTERN_MAP(ACT_PKT_QOS_ADDTSRSP, PAT_ACT_QOS_ADDTSRSP),
	FILL_PATTERN_MAP(ACT_PKT_QOS_DELTS, PAT_ACT_QOS_DELTS),
	FILL_PATTERN_MAP(ACT_PKT_QOS_SCHEDULE, PAT_ACT_QOS_SCHEDULE),
	// DLS (0x2)
	FILL_PATTERN_MAP(ACT_PKT_DLS_DLSREQ, PAT_ACT_DLS_DLSREQ),
	FILL_PATTERN_MAP(ACT_PKT_DLS_DLSRSP, PAT_ACT_DLS_DLSRSP),
	FILL_PATTERN_MAP(ACT_PKT_DLS_DLSTEARDOWN, PAT_ACT_DLS_DLSTEARDOWN),
	// BA (0x3)
	FILL_PATTERN_MAP(ACT_PKT_BA_ADDBAREQ, PAT_ACT_BA_ADDBAREQ),
	FILL_PATTERN_MAP(ACT_PKT_BA_ADDBARSP, PAT_ACT_BA_ADDBARSP),
	FILL_PATTERN_MAP(ACT_PKT_BA_DELBA, PAT_ACT_BA_DELBA),
	// Public (0x4)
	FILL_PATTERN_MAP(ACT_PKT_BSS_COEXIST, PAT_ACT_BSS_COEXIST),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_DISC_RSP, PAT_ACT_TDLS_DISC_RSP),
	FILL_PATTERN_MAP(ACT_PKT_P2P_GO_NEG_REQ, PAT_ACT_P2P_GO_NEG_REQ),
	FILL_PATTERN_MAP(ACT_PKT_P2P_GO_NEG_RSP, PAT_ACT_P2P_GO_NEG_RSP),
	FILL_PATTERN_MAP(ACT_PKT_P2P_GO_NEG_CONF, PAT_ACT_P2P_GO_NEG_CONF),
	FILL_PATTERN_MAP(ACT_PKT_P2P_INVIT_REQ, PAT_ACT_P2P_INVIT_REQ),
	FILL_PATTERN_MAP(ACT_PKT_P2P_INVIT_RSP, PAT_ACT_P2P_INVIT_RSP),
	FILL_PATTERN_MAP(ACT_PKT_P2P_DEV_DISCOVERABILITY_REQ, PAT_ACT_P2P_DEV_DISCOVERABILITY_REQ),
	FILL_PATTERN_MAP(ACT_PKT_P2P_DEV_DISCOVERABILITY_RSP, PAT_ACT_P2P_DEV_DISCOVERABILITY_RSP),
	FILL_PATTERN_MAP(ACT_PKT_P2P_PROV_DISC_REQ, PAT_ACT_P2P_PROV_DISC_REQ),
	FILL_PATTERN_MAP(ACT_PKT_P2P_PROV_DISC_RSP, PAT_ACT_P2P_PROV_DISC_RSP),
	FILL_PATTERN_MAP(ACT_PKT_NAN_SDF, PAT_ACT_NAN_SDF),
	FILL_PATTERN_MAP(ACT_PKT_GAS_INT_REQ, PAT_ACT_GAS_INT_REQ),
	FILL_PATTERN_MAP(ACT_PKT_GAS_INT_RSP, PAT_ACT_GAS_INT_RSP),
	FILL_PATTERN_MAP(ACT_PKT_GAS_COMEBACK_REQ, PAT_ACT_GAS_COMEBACK_REQ),
	FILL_PATTERN_MAP(ACT_PKT_GAS_COMEBACK_RSP, PAT_ACT_GAS_COMEBACK_RSP),
	// Radio Measurement - 11k (0x5)
	FILL_PATTERN_MAP(ACT_PKT_RM_RM_REQ, PAT_ACT_RM_RM_REQ),
	FILL_PATTERN_MAP(ACT_PKT_RM_RM_RPT, PAT_ACT_RM_RM_RPT),
	FILL_PATTERN_MAP(ACT_PKT_RM_LM_REQ, PAT_ACT_RM_LM_REQ),
	FILL_PATTERN_MAP(ACT_PKT_RM_LM_RPT, PAT_ACT_RM_LM_RPT),
	FILL_PATTERN_MAP(ACT_PKT_RM_NR_REQ, PAT_ACT_RM_NR_REQ),
	FILL_PATTERN_MAP(ACT_PKT_RM_NR_RSP, PAT_ACT_RM_NR_RSP),
	// High Throughput - 11n (0x7)
	FILL_PATTERN_MAP(ACT_PKT_HT_NOTI_CHNL_WIDTH, PAT_ACT_HT_NOTI_CHNL_WIDTH),
	FILL_PATTERN_MAP(ACT_PKT_HT_SM_PS, PAT_ACT_HT_SM_PS),
	FILL_PATTERN_MAP(ACT_PKT_HT_PSMP, PAT_ACT_HT_PSMP),
	FILL_PATTERN_MAP(ACT_PKT_HT_SET_PCO_PHASE, PAT_ACT_HT_SET_PCO_PHASE),
	FILL_PATTERN_MAP(ACT_PKT_HT_CSI, PAT_ACT_HT_CSI),
	FILL_PATTERN_MAP(ACT_PKT_HT_NON_COMPRESSED_BEAMFORMING, PAT_ACT_HT_NON_COMPRESS_BEAMFORMING),
	FILL_PATTERN_MAP(ACT_PKT_HT_COMPRESSED_BEAMFORMING, PAT_ACT_HT_COMPRESS_BEAMFORMING),
	FILL_PATTERN_MAP(ACT_PKT_HT_ASEL_FEEDBACK, PAT_ACT_HT_ASEL_FEEDBACK),
	// SA Query - 11w(0x8)
	FILL_PATTERN_MAP(ACT_PKT_SA_QUERY_REQ, PAT_ACT_SA_QUERY_REQ),
	// TDLS-11z (0x0C)
	FILL_PATTERN_MAP(ACT_PKT_TDLS_REQ, PAT_ACT_TDLS_REQ),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_RSP, PAT_ACT_TDLS_RSP),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_CONFIRM, PAT_ACT_TDLS_CONFIRM),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_TEARDOWN, PAT_ACT_TDLS_TEARDOWN),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_PEER_TRAFIC_IND, PAT_ACT_TDLS_PEER_TRAFIC_IND),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_CHNL_SW_REQ, PAT_ACT_TDLS_CHNL_SW_REQ),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_CHNL_SW_RSP, PAT_ACT_TDLS_CHNL_SW_RSP),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_PEER_PSM_REQ, PAT_ACT_TDLS_PEER_PSM_REQ),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_PEER_PSM_RSP, PAT_ACT_TDLS_PEER_PSM_RSP),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_PEER_TRAFIC_RSP, PAT_ACT_TDLS_PEER_TRAFIC_RSP),
	FILL_PATTERN_MAP(ACT_PKT_TDLS_DISC_REQ, PAT_ACT_TDLS_DISC_REQ),
	// WMM (0x11)
	FILL_PATTERN_MAP(ACT_PKT_WMM_ADDTSREQ, PAT_ACT_WMM_ADDTSREQ),
	FILL_PATTERN_MAP(ACT_PKT_WMM_ADDTSRSP, PAT_ACT_WMM_ADDTSRSP),
	FILL_PATTERN_MAP(ACT_PKT_WMM_DELTS, PAT_ACT_WMM_DELTS),
	// VHT (0x15)
	FILL_PATTERN_MAP(ACT_PKT_VHT_COMPRESSED_BEAMFORMING, PAT_ACT_VHT_COMPRESSED_BEAMFORMING),
	FILL_PATTERN_MAP(ACT_PKT_VHT_GROUPID_MANAGEMENT, PAT_ACT_VHT_GROUPID_MANAGEMENT),
	FILL_PATTERN_MAP(ACT_PKT_VHT_OPMODE_NOTIFICATION, PAT_ACT_VHT_OPMODE_NOTIFICATION),
	//Vendor Specific (0x7F)
	FILL_PATTERN_MAP(ACT_PKT_P2P_NOA, PAT_ACT_P2P_NOA),
	FILL_PATTERN_MAP(ACT_PKT_P2P_PRESENCE_REQ, PAT_ACT_P2P_PRESENCE_REQ),
	FILL_PATTERN_MAP(ACT_PKT_P2P_PRESENCE_RSP, PAT_ACT_P2P_PRESENCE_RSP),
	FILL_PATTERN_MAP(ACT_PKT_P2P_GO_DISCOVERABILITY_REQ, PAT_ACT_P2P_GO_DISCOVERABILITY_REQ),

	// ===== Insert new item above this line =====
	{ACT_PKT_TYPE_UNKNOWN, 0, NULL}
};

// 802.11 Action Frame IE offset
PKT_IE_OFFSET_MAP	ActPktIeOffsetMap[] =
{	
	// Qos (0x1)
	{ACT_PKT_QOS_ADDTSREQ, (sMacHdrLng + 3)},
	{ACT_PKT_QOS_ADDTSRSP, (sMacHdrLng + 5)},
	{ACT_PKT_QOS_SCHEDULE, (sMacHdrLng + 2)},
	// DLS (0x2)
	{ACT_PKT_DLS_DLSREQ, (sMacHdrLng + 18)},
	{ACT_PKT_DLS_DLSRSP, (sMacHdrLng + 18)},
	// Public (0x4)
	{ACT_PKT_BSS_COEXIST, (sMacHdrLng + 2)},
	{ACT_PKT_TDLS_DISC_RSP, (sMacHdrLng + 5)},
	{ACT_PKT_P2P_GO_NEG_REQ, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_GO_NEG_RSP, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_GO_NEG_CONF, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_INVIT_REQ, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_INVIT_RSP, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_DEV_DISCOVERABILITY_REQ, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_DEV_DISCOVERABILITY_RSP, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_PROV_DISC_REQ, (sMacHdrLng + 8)},
	{ACT_PKT_P2P_PROV_DISC_RSP, (sMacHdrLng + 8)},
	// WMM (0x11)
	{ACT_PKT_WMM_ADDTSREQ, (sMacHdrLng + 4)},
	{ACT_PKT_WMM_ADDTSRSP, (sMacHdrLng + 4)},
	{ACT_PKT_WMM_DELTS, (sMacHdrLng + 4)},
	//Vendor Specific (0x7F)
	{ACT_PKT_P2P_NOA, (sMacHdrLng + 7)},
	{ACT_PKT_P2P_PRESENCE_REQ, (sMacHdrLng + 7)},
	{ACT_PKT_P2P_PRESENCE_RSP, (sMacHdrLng + 7)},
	{ACT_PKT_P2P_GO_DISCOVERABILITY_REQ, (sMacHdrLng + 7)},
	
	// ===== Insert new item above this line =====
	{ACT_PKT_TYPE_UNKNOWN, 0}
};

// Pattern Array
//TDLS
u1Byte	PAT_ENCAP_DATA_TDLS_SETUP_REQ[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x00};
u1Byte	PAT_ENCAP_DATA_TDLS_SETUP_RSP[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x01};
u1Byte	PAT_ENCAP_DATA_TDLS_SETUP_CONFIRM[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x02};
u1Byte	PAT_ENCAP_DATA_TDLS_TEARDOWN[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x03};
u1Byte	PAT_ENCAP_DATA_TDLS_TRAFFIC_INDI[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x04};
u1Byte	PAT_ENCAP_DATA_TDLS_CHNL_SW_REQ[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x05};
u1Byte	PAT_ENCAP_DATA_TDLS_CHNL_SW_RSP[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x06};
u1Byte	PAT_ENCAP_DATA_TDLS_PSM_REQ[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x07};
u1Byte	PAT_ENCAP_DATA_TDLS_PSM_RSP[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x08};
u1Byte	PAT_ENCAP_DATA_TDLS_TRAFFIC_RSP[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x09};
u1Byte	PAT_ENCAP_DATA_TDLS_DISC_REQ[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x0C, 0x0A};
u1Byte	PAT_ENCAP_DATA_TDLS_PROBE_REQ[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x7F, 0x50, 0x6F, 0x9A, 0x04};
u1Byte	PAT_ENCAP_DATA_TDLS_PROBE_RSP[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x0D, 0x02, 0x7F, 0x50, 0x6F, 0x9A, 0x05};

// CCX
u1Byte	PAT_ENCAP_DATA_CCX_IAPP[] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};

// The pattern array of encapsulated data frames.
// By Bruce, 2012-03-23.
PKT_PATTERN_MAP	PktEncapDataPatternsMap[] =
{
	// TDLS
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_SETUP_REQ, PAT_ENCAP_DATA_TDLS_SETUP_REQ),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_SETUP_RSP, PAT_ENCAP_DATA_TDLS_SETUP_RSP),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_SETUP_CONFIRM, PAT_ENCAP_DATA_TDLS_SETUP_CONFIRM),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_TEARDOWN, PAT_ENCAP_DATA_TDLS_TEARDOWN),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_TRAFFIC_INDI, PAT_ENCAP_DATA_TDLS_TRAFFIC_INDI),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_CHNL_SW_REQ, PAT_ENCAP_DATA_TDLS_CHNL_SW_REQ),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_CHNL_SW_RSP, PAT_ENCAP_DATA_TDLS_CHNL_SW_RSP),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_PSM_REQ, PAT_ENCAP_DATA_TDLS_PSM_REQ),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_PSM_RSP, PAT_ENCAP_DATA_TDLS_PSM_RSP),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_TRAFFIC_RSP, PAT_ENCAP_DATA_TDLS_TRAFFIC_RSP),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_DISC_REQ, PAT_ENCAP_DATA_TDLS_DISC_REQ),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_PROBE_REQ, PAT_ENCAP_DATA_TDLS_PROBE_REQ),
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_TDLS_PROBE_RSP, PAT_ENCAP_DATA_TDLS_PROBE_RSP),

	// CCX RM
	FILL_PATTERN_MAP(ENCAP_DATA_PKT_CCX_IAPP, PAT_ENCAP_DATA_CCX_IAPP),

	// ===== Insert new item above this line =====
	{ENCAP_DATA_PKT_UNKNOWN, 0, NULL}
};



BOOLEAN
EqualOS(
	IN	OCTET_STRING	os1,
	IN	OCTET_STRING	os2
	)
{
	if( os1.Length!=os2.Length )
		return FALSE;

	if( os1.Length==0 )
		return FALSE;		

	return (PlatformCompareMemory(os1.Octet,os2.Octet,os1.Length)==0) ? TRUE:FALSE;
}


BOOLEAN
IsSSIDAny(
	IN	OCTET_STRING	ssid
	)
{
	BOOLEAN retValue = FALSE;
	
	if(ssid.Length == 0)	// a kind of "ANY SSID"
		retValue =  TRUE;
	else if(ssid.Length == 3)
	{
		if(	(ssid.Octet[0]=='A' || ssid.Octet[0]=='a') &&
			(ssid.Octet[1]=='N' || ssid.Octet[1]=='n') &&
			(ssid.Octet[2]=='Y' || ssid.Octet[2]=='y')	 	)
		retValue =  TRUE;
	}

	return retValue;	
}


BOOLEAN
IsSSIDDummy(
	IN	OCTET_STRING	ssid
	)
{
	u2Byte	i;
	u1Byte	ch;

	if(ssid.Length == 0)	// a kind of "ANY SSID"
		return FALSE;
	for(i = 0; i < ssid.Length; i++)
	{
		ch = ssid.Octet[i];
		if( (ch >= 0x20) && (ch <= 0x7e) )	//wifi, printable ascii code must be supported
			;//ok
		else
		{
			// If the SSID contain Any Illeagl ASCII Code
			// It should be reganize as dummy SSID.
			// It modified for No Link any If UI does no Set the OID or registery
			// Modifid by Mars, 20090722
			// 2010/04/12 MH For SEC Korea SSID case, we will not treat it as dummy SSID.
			// if the SSID length is not 32.
			if (ssid.Length == MAX_SSID_LEN)
				return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN
IsSSIDNdisTest(
	IN	OCTET_STRING	ssid
	)
{
	if(ssid.Length ==0)	// a kind of "ANY SSID"
	{
		return TRUE;
	}
	
	if(	(ssid.Length >= 8) &&
		(	(ssid.Octet[0]=='N' || ssid.Octet[0]=='n') &&
			(ssid.Octet[1]=='D' || ssid.Octet[1]=='d') &&
			(ssid.Octet[2]=='I' || ssid.Octet[2]=='i') &&	 	
			(ssid.Octet[3]=='S' || ssid.Octet[2]=='s') && 
			(ssid.Octet[4]=='T' || ssid.Octet[2]=='t') &&
			(ssid.Octet[5]=='E' || ssid.Octet[2]=='e') &&
			(ssid.Octet[6]=='S' || ssid.Octet[2]=='s') &&
			(ssid.Octet[7]=='T' || ssid.Octet[2]=='t'))	 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}

//
// Description:
//		Checks whether a packet contains any invalid IE.
//		Implemented based on PacketGetElement().
// 
// 2008.12.32, haich
//
BOOLEAN
PacketCheckIEValidity(
	IN	OCTET_STRING	packet,
	IN	PRT_RFD			pRfd
	)
{
	u2Byte			PacketSubType;
	u2Byte			offset = 0; 		// current offset to the packet
	BOOLEAN			bRet = TRUE;		// used for return
	u1Byte			u1EID;			// EID of  current IE
	u1Byte			u1ELength;		// Length of current IE
	u2Byte			ValidPacketLength=0;

	do
	{
		if(sMacHdrLng > packet.Length)
		{// not able to get packet type
			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("PacketCheckIEValidity(): sMacHdrLng(%u) > packet.Length (%u)\n", sMacHdrLng, packet.Length));
			bRet = FALSE;
			break;
		}

		if(!IsMgntFrame(packet.Octet))
		{// not a mangaement frame
			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("PacketCheckIEValidity(): frame type: %u\n", Frame_Type(packet)));
			bRet = FALSE;
			break;
		}

		PacketSubType = Frame_Subtype(packet);
		
		if(PacketSubType > 12)
		{// beyond deauth
			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("PacketCheckIEValidity(): wrong subtype: %u\n", PacketSubType));
			bRet = FALSE;
			break;
		}

		if(!(offset = sPacketIEOffsetTable[PacketSubType]))
		{// not a sub type of frame that could have IEs
			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("PacketCheckIEValidity(): not a sub type of frame that could have IEs, subtype: %u\n", PacketSubType));
			bRet = FALSE;
			break;
		}

		do // for all IEs
		{
			if(offset + 2 >= packet.Length)
			{// [malicious attack] not ok to read EID and Element Length
				RT_TRACE(COMP_SCAN, DBG_LOUD, 
					("PacketCheckIEValidity(): [malicious attack] not ok to read EID and Element Length\n"));
				bRet = TRUE;
				break;
			}

			u1EID = packet.Octet[offset];		// Get current Element ID.
			u1ELength = packet.Octet[offset+1];	// Get current length of the IE.

			if(!IsIELengthValid(u1EID, u1ELength))
			{
				bRet = FALSE;
				break;
			}
			
			if(offset + 2 + u1ELength < packet.Length) 
			{// Jump to the position of length of next IE. (2 byte is for the ID and length field.)
				offset = offset + 2 + u1ELength; // incr at least 2 for every loop
				ValidPacketLength = offset;
			}
			else if(offset + 2 + u1ELength == packet.Length) 
			{// the IEs in the packet are all valid.
				bRet = TRUE;
				ValidPacketLength = offset + 2 + u1ELength;
				break;
			}
			else 
			{// [malicious attack] length of IE exceeds packet length
				RT_TRACE(COMP_SCAN, DBG_LOUD, 
					("PacketCheckIEValidity(): [malicious attack] length of IE exceeds packet length\n"));
				bRet = TRUE;
				break;
			}
		}while(TRUE);

	}while(FALSE);

	if(!bRet)
	{
		RT_PRINT_DATA(COMP_SCAN, DBG_TRACE, "Packet with wrong IE:", packet.Octet, packet.Length);
	}
	pRfd->ValidPacketLength = ValidPacketLength;
	return bRet;

}

//
// Description:
//	Determine the packet type of the action frame.
// Argument:
//	[in] posMpdu -
//		The full 802.11 packet.
// Return:
//	If this packet is determined successfully, return the ACT_PKT_xxx.
//	If this function cannot recognize this packet, return ACT_PKT_TYPE_UNKNOWN.
// Remark:
//	It checks the category, action field (if there is such one) and OUI information to determine the type.
//	Do not input non-action frame to this function and it doesn't check if this packet is action frame.
// By Bruce, 2012-03-09.	
//
ACT_PKT_TYPE
PacketGetActionFrameType(
	IN	POCTET_STRING	posMpdu
	)
{
	u4Byte	idx = 0;

	if(posMpdu->Length <= sMacHdrLng)
	{
		RT_TRACE_F(COMP_DBG, DBG_WARNING, ("[WARNING] Invalid length (%d) for this packet!\n", posMpdu->Length));
		return ACT_PKT_TYPE_UNKNOWN;
	}

	//Retrieve the table and check the patterns
	for(idx = 0; ACT_PKT_TYPE_UNKNOWN != (ACT_PKT_TYPE)(PktActPatternsMap[idx].PktType); idx ++)
	{
		// Packet length mismatch
		if((posMpdu->Length - sMacHdrLng) < PktActPatternsMap[idx].PatternLen)
			continue;

		// Compare pattern
		if(0 == PlatformCompareMemory(Frame_FrameBody(*posMpdu), PktActPatternsMap[idx].Pattern, PktActPatternsMap[idx].PatternLen))
		{
			return (ACT_PKT_TYPE)(PktActPatternsMap[idx].PktType);
		}		
	}
	return ACT_PKT_TYPE_UNKNOWN;
}

//
// Description:
//	Determine the packet type of the encapsulated data frame.
// Argument:
//	[in] posDataContent -
//		The full data content after 802.11 header(including Qos + Security header + HC control header).
//		It should be the header of LLC.
// Return:
//	If this packet is determined successfully, return the ENCAP_DATA_PKT_xxx.
//	If this function cannot recognize this packet, return ENCAP_DATA_PKT_UNKNOWN.
// Remark:
//	It checks the LLC/SNAP header, and the following patters.
//	This function does not check the security or any 802.11 condition.
// By Bruce, 2012-03-23.
//
ENCAP_DATA_PKT_TYPE
PacketGetEncapDataFrameType(
	IN	POCTET_STRING	posDataContent
	)
{
	u4Byte	idx = 0;

	//Retrieve the table and check the patterns
	for(idx = 0; ENCAP_DATA_PKT_UNKNOWN != (ACT_PKT_TYPE)(PktEncapDataPatternsMap[idx].PktType); idx ++)
	{
		// Packet length mismatch
		if(posDataContent->Length < PktEncapDataPatternsMap[idx].PatternLen)
			continue;

		// Compare pattern
		if(0 == PlatformCompareMemory(posDataContent->Octet, PktEncapDataPatternsMap[idx].Pattern, PktEncapDataPatternsMap[idx].PatternLen))
		{
			return (ENCAP_DATA_PKT_TYPE)(PktEncapDataPatternsMap[idx].PktType);
		}		
	}
	return ENCAP_DATA_PKT_UNKNOWN;
}


//
// Description:
//	Get the offset according to the packet type.
// Arguments:
//	[in] posMpdu -
//		The full 802.11 packet.
//	[out] pOffset -
//		The returned offset for the fist IE.
// Return:
//	If this executes without any error, return FALSE.
//	If this packet has no IE, return FALSE.
// Remark:
//	None.
// By Bruce, 2012-03-09.
//
BOOLEAN
PacketGetIeOffset(
	IN	POCTET_STRING	posMpdu,
	OUT	pu2Byte			pOffset
	)
{
	BOOLEAN		bValid = TRUE;
	u4Byte		idx = 0;
	
	switch(PacketGetType(*posMpdu))
	{
		default:
			bValid = FALSE;
			break;
			
		case Type_Auth:
			*pOffset = (sMacHdrLng + 6);
			break;
			
		case Type_Probe_Req:
			*pOffset = (sMacHdrLng + 0);
			break;
			
		case Type_Beacon:
		case Type_Probe_Rsp:
			*pOffset = (sMacHdrLng + 12);
			break;
			
		case Type_Reasoc_Req:
			*pOffset = (sMacHdrLng + 10);
			break;
			
		case Type_Asoc_Req:
			*pOffset = (sMacHdrLng + 4);
			break;
			
		case Type_Asoc_Rsp:
		case Type_Reasoc_Rsp:
			*pOffset = (sMacHdrLng + 6);
			break;
			
		case Type_Disasoc:
		case Type_Deauth:
			*pOffset = (sMacHdrLng + 2);
			break;
				
		case Type_Action:
			{
				ACT_PKT_TYPE	actType = PacketGetActionFrameType(posMpdu);

				bValid = FALSE;
				if(ACT_PKT_TYPE_UNKNOWN == actType)
				{
					RT_TRACE_F(COMP_DBG, DBG_WARNING, ("[WARNING] Unrecognized action frame type!\n"));
					break;
				}
				for(idx = 0; ACT_PKT_TYPE_UNKNOWN != (ACT_PKT_TYPE)(ActPktIeOffsetMap[idx].PktType); idx ++)
				{
					if(actType == (ACT_PKT_TYPE)(ActPktIeOffsetMap[idx].PktType))
					{
						*pOffset = ActPktIeOffsetMap[idx].IeOffset;
						bValid = TRUE;
						break;
					}
				}

				if(!bValid)
				{
					RT_TRACE_F(COMP_DBG, DBG_WARNING, ("[WARNING] cannot find offset for action frame type = %d!\n", actType));
				}
			}
			break;
	}

	return bValid;
}

//
// Parsing Information Elements.
//
// This function is used to search the WPS Fragment IE
// In WPS 2.0. It wil content more then 2 IE is the forment
// DD-Len-00-50-f2-04-xxxxx-DD-Len-00-50-f2-04
//
u1Byte
PacketGetElementNum(
	IN	OCTET_STRING	packet,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	)
{
	u2Byte			offset;
	OCTET_STRING	IEs;
	OCTET_STRING	ret={0,0};	// used for return
	u1Byte 			IENum = 0;

	if(!PacketGetIeOffset(&packet, &offset))
	{
		return 0;
	}
	
	if(offset < packet.Length)
	{
		pu1Byte	pIE;
		u2Byte IELen = (packet.Length - offset);
		
		pIE = (packet.Octet + offset);
		FillOctetString(IEs, pIE,IELen);

		RT_PRINT_DATA(COMP_WPS, DBG_TRACE, "The IE in the packet :\n", pIE, IELen);
		do
		{
			ret= IEGetElement(IEs, ID, OUIType, OUISubType);
			if(ret.Length > 0)
			{
				IENum++;
				RT_TRACE(COMP_WPS, DBG_TRACE,("We find a WPS IE in probe or beacon Fragment num is %d \n",IENum));
				IELen = IELen - ((u2Byte)(ret.Octet - pIE) + ret.Length);
				pIE = (ret.Octet + ret.Length);			
				FillOctetString(IEs, pIE,IELen);			
				RT_PRINT_DATA(COMP_WPS, DBG_TRACE, "The IE after WPS IE in the packet :\n", pIE, IELen);
			}
			else
			{
				RT_TRACE(COMP_WPS, DBG_TRACE,("There is no WPS IE in the probe or beacon\n"));				
			}
		}
		while(ret.Length != 0);
		
		return IENum;
	}
	else
	{
		return IENum;
	}
}


//
// Parsing Information Elements.
//
// Added a parameter "OUISubType" and rewrited by Annie, 2005-11-08,
// since the element ID of WPA-IE and WMM-IE are the same(0xDD=221).
//
OCTET_STRING 
PacketGetElement(
	IN	OCTET_STRING	packet,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	)
{
	u2Byte			offset;
	OCTET_STRING	IEs;
	OCTET_STRING	ret={0,0};	// used for return

	if(!PacketGetIeOffset(&packet, &offset))
		return ret;

	if(offset < packet.Length)
	{
		FillOctetString(IEs, (packet.Octet + offset), (packet.Length - offset));
		return IEGetElement(IEs, ID, OUIType, OUISubType);
	}
	else
	{
		return ret;
	}
}

VOID
PacketMakeElement(
	IN	POCTET_STRING	packet,
	IN	ELEMENT_ID		ID,
	IN	OCTET_STRING	info
	)
{
	pu1Byte buf = packet->Octet + packet->Length;

	buf[0] = (u1Byte)ID;
	buf[1] = (u1Byte)info.Length;

	if(info.Length > 0)
	{
		PlatformMoveMemory( buf + 2, info.Octet, info.Length);
	}

	packet->Length += info.Length + 2;
}

VOID
PacketAppendData(
	IN	POCTET_STRING	packet,
	IN	OCTET_STRING	data
	)
{
	pu1Byte buf = packet->Octet + packet->Length;

	PlatformMoveMemory( buf, data.Octet, data.Length);

	packet->Length = packet->Length + data.Length;
}

BOOLEAN
TimGetBcMcBit(
	IN	OCTET_STRING	Tim
	)
{
	return	((Tim.Octet[2] & 0x01) == 0x01);
}

BOOLEAN	
TimGetAIDBit( 
	IN	OCTET_STRING	Tim, 
	IN	u2Byte			AID
	)
{
	BOOLEAN		result;
	u2Byte		offset,offset_byte;
	u1Byte		offset_bit;
	u2Byte		FirstStationInTim;
	
	FirstStationInTim = (Tim.Octet[2] & 0xFE) * 8;
	if( AID<FirstStationInTim || AID >= (FirstStationInTim+(Tim.Length-3)*8) )
	{	// Out of the range(too large)
		return FALSE;
	}
	else
	{
		
		offset = AID - FirstStationInTim;
		
		offset_byte = offset >>3;
		offset_bit  = (unsigned char)(offset & 0x7);

		// Look up in the partial virtual bitmap
		result = Tim.Octet[3 + offset_byte]&(0x01<<offset_bit);

		return result;
	}
}

u2Byte
N_DBPSOfRate(
		u4Byte	DataRate
		)
{
	u2Byte	N_DBPS = 24;

	switch(DataRate)
	{
	case 12:
		N_DBPS = 24;
		break;

	case 18:
		N_DBPS = 36;
		break;

	case 24:
		N_DBPS = 48;
		break;

	case 36:
		N_DBPS = 72;
		break;

	case 48:
		N_DBPS = 96;
		break;

	case 72:
		N_DBPS = 144;
		break;

	case 96:
		N_DBPS = 192;
		break;

	case 108:
		N_DBPS = 216;
		break;

	default:
		break;
	}

	return N_DBPS;
}

u2Byte 
ComputeTxTime(
	IN	u2Byte			FrameLength,
	IN	u4Byte			DataRate,
	IN	BOOLEAN			bManagementFrame,
	IN	BOOLEAN			bShortPreamble
	)
{
	u2Byte			FrameTime=0;
	u2Byte			N_DBPS;
	u2Byte			Ceiling;

	switch(DataRate)
	{
	case 2:
	case 4:
	case 11:
	case 22:
			// 1M can only use Long Preamble. Ref. 802.11b 18.2.2.2. 2005.01.18, by rcnjko.
			if( bManagementFrame || !bShortPreamble || DataRate == 2 )
			{	// long preamble
				FrameTime = (u2Byte)(144+48+(FrameLength*8*2/DataRate));		
			}
			else
			{	// Short preamble
				FrameTime = (u2Byte)(72+24+(FrameLength*8*2/DataRate));
			}

			if( ( FrameLength*8*2 % DataRate ) != 0 )
				FrameTime ++;
			break;
	case 12:
	case 18:
	case 24:
	case 36:
	case 44:
	case 48:
	case 66:
	case 72:
	case 96:
	case 108:
		N_DBPS = N_DBPSOfRate(DataRate);
		Ceiling = (16 + 8*FrameLength + 6) / N_DBPS + (((16 + 8*FrameLength + 6) % N_DBPS) ? 1 : 0);
		FrameTime = (u2Byte)(16 + 4 + 4*Ceiling + 6);
		break;
	}
	
	return FrameTime;
}


BOOLEAN
IsHiddenSsid(
	OCTET_STRING		ssid
	)
{
        if( ((ssid.Length == 1) &&      (ssid.Octet[0] == 0x20) ) ||
                ((ssid.Length >  0) &&  (ssid.Octet[0] == 0x0 ) ) ||
                (ssid.Length == 0)        )
        {
                return TRUE;
        }
        return FALSE;
}


BOOLEAN
BeHiddenSsid(
	pu1Byte	ssidbuf,
	u1Byte	ssidlen
	)
{
        if( ((ssidlen == 1) &&      (ssidbuf[0] == 0x20) ) ||
                ((ssidlen >  0) &&  (ssidbuf[0] == 0x0 ) ) ||
                (ssidlen == 0)        )
        {
                return TRUE;
        }
        return FALSE;
}



BOOLEAN
NullSSID(
	OCTET_STRING	bcnPkt
	)
{
	OCTET_STRING	ssIdBeacon;
	u1Byte			i;
	BOOLEAN			athHdnAP;	//sean,20030410, fix for linksys BEFW11S4

//	ssIdBeacon = PacketGetElement( bcnPkt, EID_SsId );		// Rewrited for new added parameter. Annie, 2005-11-08.
//	ssIdBeacon = PacketGetElement( bcnPkt, EID_SsId, OUI_SUB_DONT_CARE );
	ssIdBeacon = PacketGetElement( bcnPkt, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	for(i=0;i<ssIdBeacon.Length;i++)
	{
		if( ssIdBeacon.Octet[i] != 0x0 )
		{
			break;
		}
	}

	athHdnAP = (ssIdBeacon.Length == 1) && (ssIdBeacon.Octet[0]==0x20);
	return ((i==ssIdBeacon.Length)||athHdnAP)?TRUE:FALSE;
}

BOOLEAN
CompareSSID(
	pu1Byte	ssidbuf1,
	u2Byte	ssidlen1,
	pu1Byte	ssidbuf2,
	u2Byte	ssidlen2
)
{
	if(ssidlen1 == ssidlen2)
	{
		if((ssidlen1 == 0) ||
		    ( PlatformCompareMemory(ssidbuf1, ssidbuf2, ssidlen1) == 0 ))
		return TRUE;
	}
	
	return FALSE;
}

u1Byte
ComputeAckRate(
	IN	OCTET_STRING	BSSBasicRateSet,
	IN	u1Byte			DataRate)
{
	u2Byte	i;
	u1Byte	AckRate = 0;
	u1Byte	BasicRate;

	DataRate &= 0x7F;

	// Find the highest rate in the BSSBasicRateSet 
	// that is less than or equal to DataRate.
	for( i = 0; i < BSSBasicRateSet.Length; i++)
	{
		BasicRate = BSSBasicRateSet.Octet[i] & 0x7F; 
		if(	BasicRate <= DataRate && BasicRate > AckRate)
			AckRate = BasicRate; 
	}

	// Make sure the AckRate is in the same modulation of DataRate, 
	// otherwise we it shall use highest mandatory rate of PHY 
	// that is less than or equal to DataRate.
	switch(DataRate)
	{
	// CCK. 
	case MGN_1M:
	case MGN_2M:
	case MGN_5_5M:
	case MGN_11M:
		if(AckRate == 0 || !IS_CCK_RATE(AckRate) )
			AckRate = MGN_1M;
		break;

	// OFDM.
	case 12:
	case 18:
	case 24:
	case 36:
	case 48:
	case 72:
	case 96:
	case 108:
		if(	AckRate == 0 || IS_CCK_RATE(AckRate) )
		{
			if(DataRate >= 48)
			{ // 24M
				AckRate = 48;
			}
			else if(DataRate >= 24)
			{ // 12M
				AckRate = 24;
			}
			else
			{ // 6M
				AckRate = 12;
			}
		}
		break;

	default:
		RT_TRACE(COMP_DBG, DBG_SERIOUS, ("ComputeAckRate(): unsupported rate %#02X !!!\n", DataRate));
		if(AckRate == 0)
			AckRate = MGN_1M;
		break;
	}

	RT_ASSERT(AckRate != 0, ("ComputeAckRate(): AckRate should not be 0 !!!\n"));

	return AckRate;
}

//
//	Description:
//		Check if current offset of the MPDU is a valid IE.
//
BOOLEAN
HasNextIE(
	IN	POCTET_STRING	posMpdu,
	IN	u4Byte			Offset
	)
{
	if(Offset + 2 > posMpdu->Length) // 2 = 1(ID) + 1(Length).
		return FALSE;

	if(Offset + 2 + *(posMpdu->Octet + Offset + 1) > posMpdu->Length) 
		return FALSE;

	return TRUE;
}

//
//	Description:
//		Wrap the IE in an RT_DOT11_IE object and advance offset 
//		to next IE.
//
//	Assumption: 
//		Currnet offset contains a valid IE, that is, HasNextIE() 
//		returns TRUE before calling this function.
//
RT_DOT11_IE
AdvanceToNextIE(
	IN		POCTET_STRING	posMpdu,
	IN OUT	pu4Byte			pOffset
	)
{
	RT_DOT11_IE Ie;

	Ie.Id = *(posMpdu->Octet + *pOffset);
	Ie.Content.Length = *(posMpdu->Octet + *pOffset + 1);
	Ie.Content.Octet = posMpdu->Octet + *pOffset + 2;

	*pOffset += (2 + *(posMpdu->Octet + *pOffset + 1));
	return Ie;
}

// 
// Description:
//	Get the number of IE elements and extract the interested element for return.
// Arguments:
//	[in] IEs -
//		The IE elements to be retrived.
// 	[in] ID -
//		The referenced element ID in the IEs to be extracted.
// 	[in] OUIType -
//		Vendor specified OUI to be determined in the element.
//	[in] OUISubType -
//		The oui subtype of the element
// Return:
//	The number of the IEs.
// Revised by Bruce, 2012-03-26.
//
u1Byte
IEGetElementNum(
	IN	OCTET_STRING	IEs,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	)
{
	u1Byte 			IENum = 0;
	OCTET_STRING	osTmp, osSingleIE;
	u2Byte			offset = 0;
	
	do
	{
		if(offset >= IEs.Length)
			break;

		FillOctetString(osTmp, IEs.Octet + offset, (IEs.Length - offset));
		
		osSingleIE = IEGetElement(osTmp, ID, OUIType, OUISubType);
		if(osSingleIE.Length > 0)
		{
			IENum ++;

			offset += (SIZE_EID_AND_LEN + osSingleIE.Length);
		}
		else
		{
			break;
		}
	}
	while(TRUE);
		
	return IENum;
}


// 
// Description:
//	Parse the IE elements and extract the interested element for return.
// Arguments:
//	IEs -
//		The IE elements to be retrived.
// 	ID -
//		The referenced element ID in the IEs to be extracted.
// 	OUISubType -
//		Vendor specified OUI to be determined in the element.
// Revised by Bruce, 2009-02-12.
//
OCTET_STRING 
IEGetElement(
	IN	OCTET_STRING	IEs,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	)
{
	u2Byte			offset = 0;
	u2Byte			length = IEs.Length;
	OCTET_STRING	ret={0,0};	// used for return
	u1Byte			temp;
	BOOLEAN 		bIEMatched = FALSE;
	OCTET_STRING	osOuiSub;
	u1Byte			MaxElementLen;

	static u1Byte	WPATag[] = {0x00, 0x50, 0xf2, 0x01};
	static u1Byte	WMMTag[] = {0x00, 0x50, 0xf2, 0x02};				// Added by Annie, 2005-11-08.
	static u1Byte	Simpleconf[]={0x00, 0x50, 0xF2, 0x04};				//added by David, 2006-10-02
	static u1Byte	CcxRmCapTag[] = {0x00, 0x40, 0x96, 0x01};			// For CCX 2 S36, Radio Management Capability element, 2006.05.15, by rcnjko.
	static u1Byte	CcxVerNumTag[] = {0x00, 0x40, 0x96, 0x03};			// For CCX 2 S38, WLAN Device Version Number element. Annie, 2006-08-20.
	static u1Byte	WPA2GTKTag[] = {0x00, 0x0f, 0xac, 0x01};			// For MAC GTK data IE by CCW
	static u1Byte	CcxTsmTag[] = {0x00, 0x40, 0x96, 0x07};				// For CCX4 S56, Traffic Stream Metrics, 070615, by rcnjko.
	static u1Byte	CcxSSIDLTag[] = {0x00, 0x50, 0xf2, 0x05};
	static u1Byte	RealtekTurboModeTag[] = {0x00, 0xE0, 0x4C, 0x01};	// Added by Annie, 2005-12-27
	static u1Byte	RealtekAggModeTag[] = {0x00, 0xe0, 0x4c, 0x02};
	static u1Byte	RealtekBTIOTModeTag[] = {0x00, 0xe0, 0x4c, 0x03};	// Add for BT IOT 
	static u1Byte	RealtekBtHsTag[] = {0x00, 0xe0, 0x4c, 0x04};		// Add for BT HS 	
	static u1Byte	Epigram[] = {0x00,0x90,0x4c};
	static u1Byte	EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x033};			// For 11n EWC definition, 2007.07.17, by Emily
	static u1Byte	EWC11NHTInfo[] = {0x00, 0x90, 0x4c, 0x034};			// For 11n EWC definition, 2007.07.17, by Emily	
	static u1Byte	Epigram11ACCap[] = {0x00, 0x90, 0x4c, 0x04, 0x08, 0xBF, 0x0C};	// For 11ac Epigram definition
	static u1Byte	BroadcomCap_1[] = {0x00, 0x10, 0x18};
	static u1Byte	BroadcomCap_2[] = {0x00, 0x0a, 0xf7};
	static u1Byte	BroadcomCap_3[] = {0x00, 0x05, 0xb5};
	static u1Byte	BroadcomLinksysE4200Cap_1[] = {0x00, 0x10, 0x18,0x02,0x00,0xf0,0x3c};  // for Linksys E4200
	static u1Byte	BroadcomLinksysE4200Cap_2[] = {0x00, 0x10, 0x18,0x02,0x01,0xf0,0x3c};
	static u1Byte	BroadcomLinksysE4200Cap_3[] = {0x00, 0x10, 0x18,0x02,0x00,0xf0,0x2c};
	static u1Byte	CiscoCap[] = {0x00, 0x40, 0x96};			// For Cisco AP IOT issue, by Emily
	static u1Byte	MeruCap[] = {0x00, 0x0c, 0xe6};
	static u1Byte	RalinkCap[] ={0x00, 0x0c, 0x43};
	static u1Byte	AtherosCap_1[] = {0x00,0x03,0x7F};
	static u1Byte	AtherosCap_2[] = {0x00,0x13,0x74};
	static u1Byte	MarvellCap[] = {0x00, 0x50, 0x43};
	static u1Byte	AirgoCap[] = {0x00, 0x0a, 0xf5};
	static u1Byte	CcxSFA[] = {0x00, 0x40, 0x96, 0x14};
	static u1Byte	CcxDiagReqReason[] = {0x00, 0x40, 0x96, 0x12};
	static u1Byte	CcxMHDR[] = {0x00, 0x40, 0x96, 0x10};
	static u1Byte	P2P_OUI_WITH_TYPE[] = {0x50, 0x6F, 0x9A, WLAN_PA_VENDOR_SPECIFIC};
	static u1Byte	WFD_OUI_WITH_TYPE[] = {0x50, 0x6F, 0x9A, WFD_OUI_TYPE};
	static u1Byte	NAN_OUI_WITH_TYPE[] = {0x50, 0x6F, 0x9A, 0x13}; 
	static u1Byte	RealtekTDLSTag[] = {0x00, 0xe0, 0x4c, 0x03};

	//Mix mode can't get DHCP in MacOS Driver. CCW revice offset 2008-04-15
	//offset = 12;

	do
	{
		if( (offset + 2) >= length )
		{
			return ret;
		}
		
		temp = IEs.Octet[offset];	// Get current Element ID.

		if( temp == ID )
		{
			if( ID == EID_Vendor )
			{ // EID_Vendor(=0xDD=221): Vendor Specific, currently we have to consider WPA and WMM Information Element.
				switch(OUIType)
				{
					case OUI_SUB_WPA:
						FillOctetString(osOuiSub, WPATag,  sizeof(WPATag));
						break;
					case OUI_SUB_WPA2GTK:
						FillOctetString(osOuiSub, WPA2GTKTag,  sizeof(WPA2GTKTag));
						break;
						
					case OUI_SUB_CCX_TSM:
						FillOctetString(osOuiSub, CcxTsmTag,  sizeof(CcxTsmTag));
						break;

					case OUI_SUB_SSIDL:
						FillOctetString(osOuiSub, CcxSSIDLTag,	sizeof(CcxSSIDLTag));
						break;

					case OUI_SUB_WMM:
						FillOctetString(osOuiSub, WMMTag,  sizeof(WMMTag));
							break;

					case OUI_SUB_REALTEK_TURBO:
						FillOctetString(osOuiSub, RealtekTurboModeTag,	sizeof(RealtekTurboModeTag));
						break;

					case OUI_SUB_REALTEK_AGG:
						FillOctetString(osOuiSub, RealtekAggModeTag, sizeof(RealtekAggModeTag));
						break;

					case OUI_SUB_SimpleConfig:
						FillOctetString(osOuiSub, Simpleconf,  sizeof(Simpleconf));
						break;

					case OUI_SUB_CCX_RM_CAP:
						FillOctetString(osOuiSub, CcxRmCapTag,	sizeof(CcxRmCapTag));
						break;

					case OUI_SUB_CCX_VER_NUM:
						FillOctetString(osOuiSub, CcxVerNumTag,  sizeof(CcxVerNumTag));
						break;
						
					case OUI_SUB_EPIG_IE:
						FillOctetString(osOuiSub, Epigram,  sizeof(Epigram));
						break;

					case OUI_SUB_11N_EWC_HT_CAP:
						FillOctetString(osOuiSub, EWC11NHTCap,	sizeof(EWC11NHTCap));
						break;

					case OUI_SUB_11N_EWC_HT_INFO:
						FillOctetString(osOuiSub, EWC11NHTInfo,  sizeof(EWC11NHTInfo));
						break;

					case OUI_SUB_11AC_EPIG_VHT_CAP:
						FillOctetString(osOuiSub, Epigram11ACCap,  sizeof(Epigram11ACCap));
						break;

					case OUI_SUB_BROADCOM_IE_1:
						FillOctetString(osOuiSub, BroadcomCap_1,  sizeof(BroadcomCap_1));						
						break;
						
					case OUI_SUB_BROADCOM_IE_2:
						FillOctetString(osOuiSub, BroadcomCap_2,  sizeof(BroadcomCap_2));						
						break;
						
					case OUI_SUB_BROADCOM_IE_3:
						FillOctetString(osOuiSub, BroadcomCap_3,  sizeof(BroadcomCap_3));						
						break;
						
					case OUI_SUB_BROADCOM_LINKSYSE4200_IE_1:
						FillOctetString(osOuiSub, BroadcomLinksysE4200Cap_1,  sizeof(BroadcomLinksysE4200Cap_1));						
						break;
						
					case OUI_SUB_BROADCOM_LINKSYSE4200_IE_2:
						FillOctetString(osOuiSub, BroadcomLinksysE4200Cap_2,  sizeof(BroadcomLinksysE4200Cap_2));						
						break;
						
					case OUI_SUB_BROADCOM_LINKSYSE4200_IE_3:
						FillOctetString(osOuiSub, BroadcomLinksysE4200Cap_3,  sizeof(BroadcomLinksysE4200Cap_3));						
						break;			

					case OUI_SUB_CISCO_IE:
						FillOctetString(osOuiSub, CiscoCap, sizeof(CiscoCap));
						break;

					case OUI_SUB_MERU_IE:
						FillOctetString(osOuiSub, MeruCap, sizeof(MeruCap));
						break;

					case OUI_SUB_RALINK_IE:
						FillOctetString(osOuiSub, RalinkCap, sizeof(RalinkCap));
						break;
						
					case OUI_SUB_ATHEROS_IE_1:
						FillOctetString(osOuiSub, AtherosCap_1, sizeof(AtherosCap_1));
						break;
						
					case OUI_SUB_ATHEROS_IE_2:
						FillOctetString(osOuiSub, AtherosCap_2, sizeof(AtherosCap_2));
						break;
						
					case OUI_SUB_MARVELL:
						FillOctetString(osOuiSub, MarvellCap, sizeof(MarvellCap));
						break;
						
					case OUI_SUB_AIRGO:
						FillOctetString(osOuiSub, AirgoCap, sizeof(AirgoCap));
						break;	
						
					case OUI_SUB_CCX_SFA:
						FillOctetString(osOuiSub, CcxSFA, sizeof(CcxSFA));
						break;
						
					case OUI_SUB_CCX_DIAG_REQ_REASON:
						FillOctetString(osOuiSub, CcxDiagReqReason, sizeof(CcxDiagReqReason));
						break;

					case OUI_SUB_CCX_MFP_MHDR:
						FillOctetString(osOuiSub, CcxMHDR, sizeof(CcxMHDR));
						break;
						
					case OUI_SUB_WIFI_DIRECT:
						FillOctetString(osOuiSub, P2P_OUI_WITH_TYPE, sizeof(P2P_OUI_WITH_TYPE));
						break;

					case OUI_SUB_WIFI_DISPLAY:
						FillOctetString(osOuiSub, WFD_OUI_WITH_TYPE, sizeof(WFD_OUI_WITH_TYPE));
						break;
						
					case OUI_SUB_NAN:
						FillOctetString(osOuiSub, NAN_OUI_WITH_TYPE, sizeof(NAN_OUI_WITH_TYPE));
						break;
							
					case OUI_SUB_REALTEK_TDLS:
						FillOctetString(osOuiSub, RealtekTDLSTag, sizeof(RealtekTDLSTag));
						break;
						
					case OUI_SUB_REALTEK_BT_IOT :
						FillOctetString(osOuiSub, RealtekBTIOTModeTag, sizeof(RealtekBTIOTModeTag));
						break;

					case OUI_SUB_REALTEK_BT_HS:
						FillOctetString(osOuiSub, RealtekBtHsTag, sizeof(RealtekBtHsTag));
						break;
						
					default:
						FillOctetString(osOuiSub, NULL, 0);
						break;
				}
				if( osOuiSub.Length > 0 && (length >= (offset + 2 + osOuiSub.Length)) ) // Prevent malicious attack.
				{
					if( PlatformCompareMemory(
						(IEs.Octet + offset + 2), 
						osOuiSub.Octet, 
						osOuiSub.Length) == 0 )
					{ // OUI field and subtype field are matched
						bIEMatched = TRUE;

						//
						// 060801, Isaiah:
						// [UAPSD Logo] Marvel AP has similar element, [DD 07 00 50 F2 02 05 01 24].
						//
						if( (OUI_SUB_WMM == OUIType) && 
							(length >= (offset + 2 + osOuiSub.Length + 1)) )
						{ // WMM-IE Matched!
						 	u1Byte WmmSubtype = *(IEs.Octet+offset+2+sizeof(WMMTag));

							if(WmmSubtype != OUISubType)
								bIEMatched = FALSE;
						}
					}
				}
			}
			else	
			{ // Other ID: Matched!
				bIEMatched = TRUE;
			}
		}

		if(bIEMatched &&
			(length >= offset + 2 + IEs.Octet[offset+1]) ) // Prevent malicious attack.
		{ // IE matched! break to return.
			//
			// Get the length of current IE.
			// We also perform length checking here to pervent malicious attack.	
			//
			switch(ID)
			{
			case EID_SsId:
				MaxElementLen = MAX_SSID_LEN;
				break;
			case EID_SupRates:
				MaxElementLen = 12; //Because Belkin 11AC  on g Mode only has 12 Octets in this IE
				break;
			case EID_FHParms:
				MaxElementLen = MAX_FH_PARM_LEN;
				break;
			case EID_DSParms:
				MaxElementLen = MAX_DS_PARM_LEN;
				break;
			case EID_CFParms:
				MaxElementLen = MAX_CF_PARM_LEN;
				break;
			case EID_Tim:
				MaxElementLen = MAX_TIM_PARM_LEN;
				break;
			case EID_IbssParms:
				MaxElementLen = MAX_IBSS_PARM_LEN;
				break;
			case EID_QBSSLoad:
				MaxElementLen = MAX_QBSS_LOAD_LEN;
				break;
			case EID_EDCAParms:
				MaxElementLen = MAX_EDCA_PARM_LEN;
				break;
			case EID_TSpec:
				MaxElementLen = MAX_TSPEC_LEN;
				break;
			case EID_Schedule:
				MaxElementLen = MAX_SCHEDULE_LEN;
				break;
			case EID_Ctext:
				MaxElementLen = MAX_CTEXT_LEN;
				break;
			case EID_ERPInfo:
				MaxElementLen = MAX_ERP_INFO_LEN;
				break;
			case EID_TSDelay:
				MaxElementLen = MAX_TS_DELAY_LEN;
				break;
			case EID_TCLASProc:
				MaxElementLen = MAX_TC_PROC_LEN;
				break;
			case EID_HTCapability:
				MaxElementLen = MAX_HT_CAP_LEN;
				break;
			case EID_HTInfo:
				MaxElementLen = MAX_HT_INFO_LEN;
				break;
			case EID_QoSCap:
				MaxElementLen = MAX_QOS_CAP;
				break;
			case EID_ExtSupRates:
				MaxElementLen = MAX_EXT_SUP_RATE_LEN;
				break;

			case EID_WAPI:
				MaxElementLen = MAX_WAPI_IE_LEN;
				break;
			case EID_LinkIdentifier:
				MaxElementLen = MAX_LINKID_LEN;
				break;
			case EID_SupportedChannels:
				MaxElementLen = MAX_SUPCHNL_LEN;
				break;
			case EID_SupRegulatory:
				MaxElementLen = MAX_SUPREGULATORY_LEN;
				break;
			case EID_SecondaryChnlOffset:
				MaxElementLen = MAX_SECONDARYOFFSET_LEN;
				break;
			case EID_ChnlSwitchTimeing:
				MaxElementLen = MAX_CHNLSWITCHTIMING_LEN;
				break;
			case EID_VHTCapability:
				MaxElementLen = MAX_VHT_CAP_LEN;
				break;
			default:
				MaxElementLen = MAX_IE_LEN;
				break;
			}
			ret.Length = (IEs.Octet[offset+1] <= MaxElementLen) ? IEs.Octet[offset+1] : MaxElementLen;

			//
			// Get pointer to the first byte (ElementID and length are not included).
			//
			ret.Octet = IEs.Octet + offset + 2;

			break;
		}
		else
		{ // Different.
			temp = IEs.Octet[offset+1]; 		// Get the length of current IE.
			offset += (temp+2); 				// Jump to the position of length of next IE. (2 byte is for the ID and length field.)
		}
	}while(1);

	return ret;
}


//
//	Description:
//		Verify if specific IE length is valid.
//
BOOLEAN
IsIELengthValid(
	IN	u1Byte		IDIE,
	IN	u1Byte		IELength
	)
{
	BOOLEAN			bRet = TRUE;	
	u1Byte			MaxIELength = MAX_IE_LEN;

	switch(IDIE)
	{
		case EID_SsId:
			MaxIELength = MAX_SSID_LEN;
			break;

		case EID_SupRates:
			MaxIELength = 12;//Because Belkin 11AC  on g Mode only has 12 Octets in this IE
			break;
			
		case EID_FHParms:
			MaxIELength = MAX_FH_PARM_LEN;
			break;
			
		case EID_DSParms:
			MaxIELength = MAX_DS_PARM_LEN;
			break;
			
		case EID_CFParms:
			MaxIELength = MAX_CF_PARM_LEN;
			break;
			
		case EID_Tim:
			MaxIELength = MAX_TIM_PARM_LEN;
			break;
			
		case EID_IbssParms:
			MaxIELength = MAX_IBSS_PARM_LEN;
			break;
			
		case EID_QBSSLoad:
			MaxIELength = MAX_QBSS_LOAD_LEN;
			break;
			
		case EID_EDCAParms:
			MaxIELength = MAX_EDCA_PARM_LEN;
			break;
			
		case EID_TSpec:
			MaxIELength = MAX_TSPEC_LEN;
			break;
			
		case EID_Schedule:
			MaxIELength = MAX_SCHEDULE_LEN;
			break;
			
		case EID_Ctext:
			MaxIELength = MAX_CTEXT_LEN;
			break;
			
		case EID_ERPInfo:
			MaxIELength = MAX_ERP_INFO_LEN;
			break;
			
		case EID_TSDelay:
			MaxIELength = MAX_TS_DELAY_LEN;
			break;
			
		case EID_TCLASProc:
			MaxIELength = MAX_TC_PROC_LEN;
			break;
			
		case EID_QoSCap:
			MaxIELength = MAX_QOS_CAP;
			break;
			
		case EID_ExtSupRates:
			MaxIELength = MAX_EXT_SUP_RATE_LEN;
			break;

		case EID_WAPI:
			MaxIELength = MAX_WAPI_IE_LEN;
			break;
			
		case EID_LinkIdentifier:
			MaxIELength = MAX_LINKID_LEN;
			break;
			
		case EID_SupportedChannels:
			MaxIELength = MAX_SUPCHNL_LEN;
			break;
			
		case EID_SupRegulatory:
			MaxIELength = MAX_SUPREGULATORY_LEN;
			break;
			
		case EID_SecondaryChnlOffset:
			MaxIELength = MAX_SECONDARYOFFSET_LEN;
			break;
			
		case EID_ChnlSwitchTimeing:
			MaxIELength = MAX_CHNLSWITCHTIMING_LEN;
			break;

		default:
			MaxIELength = MAX_IE_LEN;
	}

	if(IELength > MaxIELength)
		bRet = FALSE;

	return bRet;
}

