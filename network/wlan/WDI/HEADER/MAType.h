#ifndef __INC_MATYPE_H
#define __INC_MATYPE_H

typedef enum _ACT_CATEGORY{
	ACT_CAT_SPECTRUM_MGNT = 0,		// Spectrum management
	ACT_CAT_QOS	= 1,				// Qos
	ACT_CAT_DLS	= 2,				// Direct Link Protocol (DLS)
	ACT_CAT_BA = 3,					// Block Ack
	ACT_CAT_PUBLIC = 4,				// Public
	ACT_CAT_RM = 5,					// Radio Measurement (RM)
	ACT_CAT_FT = 6,					// Fast BSS Transition
	ACT_CAT_HT = 7,					// High Throughput
	ACT_CAT_SAQ = 8,				// Security Association Query
	ACT_CAT_SAQ_PD_PUBLIC = 9,		// Protected Dual of Public Action
	ACT_CAT_TDLS 	= 12,				// Tunneled Direct Link Setup
	ACT_CAT_WMM	= 17,				// WMM
	ACT_CAT_VHT	= 21, 				// VHT
	ACT_CAT_VENDOR_PROTECT = 126,	// Vendor-specific Protected
	ACT_CAT_VENDOR = 127,			// Vendor-specific
} ACT_CATEGORY, *PACT_CATEGORY;

typedef enum _TS_ACTION{
	ACT_ADDTSREQ 	= 0,
	ACT_ADDTSRSP 	= 1,
	ACT_DELTS		= 2,
	ACT_SCHEDULE	= 3,
} TS_ACTION, *PTS_ACTION;

typedef enum _DL_ACTION{
	ACT_DLSREQ 			= 0,
	ACT_DLSRSP 			= 1,
	ACT_DLSTEARDOWN		= 2,
} DL_ACTION, *PDL_ACTION;

typedef enum _BA_ACTION{
	ACT_ADDBAREQ 	= 0,
	ACT_ADDBARSP 	= 1,
	ACT_DELBA		= 2,
} BA_ACTION, *PBA_ACTION;

typedef enum _PUBLIC_ACTION{
	ACT_PUBLIC_BSSCOEXIST = 0, // 20/40 BSS Coexistence
	ACT_PUBLiC_MP = 7, // Measurement Pilot
	ACT_PUBLIC_TDLS_DISCOVERYRSP = 14,
}PUBLIC_ACTION, *PPUBLIC_ACTION;

typedef enum _HT_ACTION{
	ACT_RECOMMAND_WIDTH			= 0,
	ACT_MIMO_PWR_SAVE 			= 1,
	ACT_PSMP					= 2,
	ACT_SET_PCO_PHASE			= 3,
	ACT_CSI_MATRICS				= 4,
	ACT_NOCOMPRESSED_BEAMFORMING= 5,
	ACT_COMPRESSED_BEAMFORMING	= 6,
	ACT_ANTENNA_SELECT			= 7,
} HT_ACTION, *PHT_ACTION;

typedef enum _RM_ACTION{
	ACT_RM_REQUEST = 0, // Radio Measurement Request
	ACT_RM_REPORT = 1, // Radio Measurement Report
	ACT_RM_LINK_MEASURE_REQUEST = 2, // Link Measurement Request
	ACT_RM_LINK_MEASURE_REPORT = 3, // Link Measurement Report
	ACT_RM_NEIGHBOR_RRT_REQUEST = 4, // Neighbor Report Request
	ACT_RM_NEIGHBOT_RPT_RESPONSE = 5, // Neighbor Report Response
}RM_ACTION, *PRM_ACTION;

typedef enum _TDLS_ACTION{
	ACT_TDLS_SETUPREQ		= 0,
	ACT_TDLS_SETUPRSP			= 1,
	ACT_TDLS_SETUPCONFIRM	= 2,
	ACT_TDLS_TEARDOWN		= 3,
	ACT_TDLS_PEERTRAFFICIND	= 4,
	ACT_TDLS_CHNLSWITCHREQ	= 5,
	ACT_TDLS_CHNLSWITCHRSP	= 6,
	ACT_TDLS_PEERPSMREQ		= 7,
	ACT_TDLS_PEERPSMRSP		= 8,
	ACT_TDLS_PEERTRAFFICRSP	= 9,
	ACT_TDLS_DISCOVERYREQ	= 10,
	ACT_TDLS_MAX				= 11,
}TDLS_ACTION, *PTDLS_ACTION;

typedef enum _VHT_ACTION{
	ACT_VHT_COMPRESSED_BEAMFORMING	= 0,
	ACT_VHT_GROUPID_MANAGEMENT			= 1,
	ACT_VHT_OPMODE_NOTIFICATION		= 2,
}VHT_ACTION, *PVHT_ACTION;
#endif
