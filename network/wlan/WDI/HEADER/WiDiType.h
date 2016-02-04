//-----------------------------------------------------------------------------
//	File:
//		WiDiType.h
//
//	Description:
//		WiFi-Display related information and definitions for structure.
//
//-----------------------------------------------------------------------------
#ifndef __INC_WIDI_TYPE_H
#define __INC_WIDI_TYPE_H

#define WFD_OUI_TYPE				0x0A
#define WFD_MAX_SCAN_LIST_NUM		128	

#define	WFD_SUB_ID_LEN		1	// 1 Byte
#define WFD_SUB_LEN_LEN	2	// 2 Byte

#define	WFD_MAX_IE_CONTENT_LEN	(MAX_IE_LEN - SIZE_EID_AND_LEN - SIZE_OUI - SIZE_OUI_TYPE)

#define	WFD_LOCAL_IP_VER	1


//1 // Note:
//1 //	According to the WiDi spec, all content including WFD attribute header are
//1 //	aligned as "Big Endian".


#define	WFD_SET_SUB_ID(_pId, _val)		WriteH2N1BYTE(_pId, _val)
#define	WFD_GET_SUB_ID(_pId)			ReadH2N1BYTE(_pId)
#define WFD_SET_SUB_LEN(_pLen, _val) 	WriteH2N2BYTE(_pLen, _val)
#define	WFD_GET_SUB_LEN(_pLen)			ReadH2N2BYTE(_pLen)


//2 // <ToDo> Please check the tunneled fields are aligned as big or little endian!!
//4 // Category
#define SET_TUNNELED_FRAME_CATEGORY_CODE(_pStart, _val) 		WriteEF1Byte( ((UNALIGNED pu1Byte)(_pStart)), _val)
#define GET_TUNNELED_FRAME_CATEGORY_CODE(_pStart)			ReadEF1Byte( (UNALIGNED pu1Byte)(_pStart) )
//4 // OUI
#define SET_TUNNELED_FRAME_OUI(_pStart, _val)					cpOUI( ((UNALIGNED pu1Byte)(_pStart)+1), _val)
#define GET_TUNNELED_FRAME_OUI(_pStart, _val)					cpOUI( (UNALIGNED pu1Byte)(_val), ((UNALIGNED pu1Byte)_pStart)+1 )
//4 //Type
#define SET_TUNNELED_FRAME_BODY_TYPE(_pStart, _val)			WriteEF1Byte( ((UNALIGNED pu1Byte)(_pStart))+4, _val)
#define GET_TUNNELED_FRAME_BODY_TYPE(_pStart)				ReadEF1Byte( (UNALIGNED pu1Byte)(_pStart)+4 )

// ===== Device Information Subelement (ID = 0) =====
#define	WFD_DEVICE_INFO_SUBIE_LEN	6
// WFD Device Information
#define	WFD_SET_DEVICE_INFO_SUBIE_DEVICE_INFO(_pStart, _val)	WriteH2N2BYTE(_pStart, _val)
#define	WFD_GET_DEVICE_INFO_SUBIE_DEVICE_INFO(_pStart)			ReadH2N2BYTE(_pStart)
// Session Management Control Port
#define	WFD_SET_DEVICE_INFO_SUBIE_CONTROL_PORT(_pStart, _val)	WriteH2N2BYTE((UNALIGNED pu1Byte)(_pStart) + 2, _val)
#define	WFD_GET_DEVICE_INFO_SUBIE_CONTROL_PORT(_pStart)			ReadH2N2BYTE((UNALIGNED pu1Byte)(_pStart) + 2)
// WFD Device Maximum Throughput
#define	WFD_SET_DEVICE_INFO_SUBIE_MAX_TP(_pStart, _val)			WriteH2N2BYTE((UNALIGNED pu1Byte)(_pStart) + 4, _val)
#define	WFD_GET_DEVICE_INFO_SUBIE_MAX_TP(_pStart)				ReadH2N2BYTE((UNALIGNED pu1Byte)(_pStart) + 4)

// ===== Associated BSSID (ID = 1) =====
#define	WFD_ASSOC_BSSID_SUBIE_LEN	6
#define	WFD_SET_ASSOC_BSSID_SUBIE_BSSID(_pStart, _Bssid)	\
		PlatformMoveMemory((UNALIGNED pu1Byte)(_pStart), _Bssid, 6)
#define	WFD_GET_ASSOC_BSSID_SUBIE_BSSID(_pStart)	((UNALIGNED pu1Byte)(_pStart))


// ===== Coupled Sink Information Subelement (ID = 6) =====
#define	WFD_COUPLED_SINK_SUBIE_LEN	7
// Coupled Sink Status
#define	WFD_SET_COUPLED_SINK_SUBIE_STATUS(_pStart, _val)		WriteH2N1BYTE(_pStart, _val)
#define	WFD_GET_COUPLED_SINK_SUBIE_STATUS(_pStart)				ReadH2N1BYTE(_pStart)
// Coupled Sink Mac Address
#define	WFD_SET_COUPLED_SINK_SUBIE_SINK_ADDR(_pStart, _Addr)	\
		PlatformMoveMemory((UNALIGNED pu1Byte)(_pStart) + 1, _Addr, 6)
#define	WFD_GET_COUPLED_SINK_SUBIE_SINK_ADDR(_pStart)	((UNALIGNED pu1Byte)(_pStart) + 1)

// ===== Extended Capabilities Subelement (ID =7) =====
#define	WFD_EXT_CAP_SUBIE_LEN	2
// Extended Capabilities
#define	WFD_SET_EXT_CAP_SUBIE_CAP(_pStart, _val)				WriteH2N2BYTE(_pStart, _val)
#define	WFD_GET_EXT_CAP_SUBIE_CAP(_pStart)						ReadH2N2BYTE(_pStart)

// ===== Local IP Address Subelement (ID = 8) =====
#define	WFD_LOCAL_IP_SUBIE_LEN	5
#define	WFD_LOCAL_IP_IPV4_VER	1
// Local IP Address Version
#define	WFD_SET_LOCAL_IP_SUBIE_VER(_pStart, _val)				WriteH2N1BYTE(_pStart, _val)
#define	WFD_GET_LOCAL_IP_SUBIE_VER(_pStart)						ReadH2N1BYTE(_pStart)
// Local IP
#define	WFD_SET_LOCAL_IP_SUBIE_IP(_pStart, _pIp)	\
		PlatformMoveMemory((UNALIGNED pu1Byte)(_pStart) + 1, _pIp, 4)
#define	WFD_GET_LOCAL_IP_SUBIE_IP(_pStart)						((UNALIGNED pu1Byte)(_pStart) + 1)


// ===== Session Information Subelement (ID = 9) =====
#define	WFD_DEVICE_INFO_DESC_SIZE		24
// Length of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_LEN(_pStart, _idx, _val)			WriteH2N1BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx), _val)
#define	WFD_GET_SESSION_INFO_SUBIE_LEN(_pStart, _idx)				ReadH2N1BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx))
// Device Address of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_DEV_ADDR(_pStart, _idx, _Addr)	\
	PlatformMoveMemory((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 1,_Addr, 6)
#define	WFD_GET_SESSION_INFO_SUBIE_DEV_ADDR(_pStart, _idx)	((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 1)
// Associated BSSID of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_ASSOC_BSSID(_pStart, _idx, _Addr)	\
	PlatformMoveMemory((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 7,_Addr, 6)
#define	WFD_GET_SESSION_INFO_SUBIE_ASSOC_BSSID(_pStart, _idx)	((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 7)
// Device Information of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_DEV_INFO(_pStart, _idx, _val)	WriteH2N2BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 13, _val)
#define	WFD_GET_SESSION_INFO_SUBIE_DEV_INFO(_pStart, _idx)			ReadH2N2BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 13)
// Max Throughput of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_MAX_TP(_pStart, _idx, _val)		WriteH2N2BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 15, _val)
#define	WFD_GET_SESSION_INFO_SUBIE_MAX_TP(_pStart, _idx)			ReadH2N2BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 15)
// Coupled Sink Status of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_COUPLED_STATUS(_pStart, _idx, _val)	WriteH2N1BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 17, _val)
#define	WFD_GET_SESSION_INFO_SUBIE_COUPLED_STATUS(_pStart, _idx)		ReadH2N1BYTE((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 17)
// Coupled Sink Mac Address of the WFD Device Info Descriptor
#define	WFD_SET_SESSION_INFO_SUBIE_COUPLED_ADDR(_pStart, _idx, _Addr)	\
	PlatformMoveMemory((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 18,_Addr, 6)
#define	WFD_GET_SESSION_INFO_SUBIE_COUPLED_ADDR(_pStart, _idx)	((UNALIGNED pu1Byte)_pStart + (WFD_DEVICE_INFO_DESC_SIZE * _idx) + 18)

// ===== Alternative MAC Information Subelement (ID = 10) =====
#define	WFD_ALTERNATIVE_MAC_SUBIE_LEN	6
// Coupled Sink Mac Address
#define	WFD_SET_ALT_MAC_SUBIE_MAC(_pStart, _Addr)	\
		PlatformMoveMemory((UNALIGNED pu1Byte)(_pStart), _Addr, 6)
#define	WFD_GET_ALT_MAC_SUBIE_MAC(_pStart)	((UNALIGNED pu1Byte)(_pStart))

typedef enum _WFD_SUB_ID{
	WFD_SUB_ID_DEVICE_INFORMATION			=0,
	WFD_SUB_ID_ASSOC_BSSID					=1,
	WFD_SUB_ID_AUDIO_FORMATS				=2,
	WFD_SUB_ID_VIDEO_FORMATS				=3,	
	WFD_SUB_ID_3D_VIDEO_FORMATS				=4,
	WFD_SUB_ID_CONTENT_PROTECTION			=5,
	WFD_SUB_ID_COUPLED_SINK_INFORMATION		=6,
	WFD_SUB_ID_EXTENDED_CAPABILITY			=7,
	WFD_SUB_ID_LOCAL_IP						=8,
	WFD_SUB_ID_SESSION_INFORMATION			=9,
	WFD_SUB_ID_ALTERNATIVE_MAC				=10,
	// ===== Insert new Sub ID above this line =====
	WFD_SUB_ID_UNKNOW						= 0xFFFFFFFF
} WFD_SUB_ID, *PWFD_SUB_ID;

// WFD Devuce Information Bit Map
// Bit 0 ~ 1
typedef enum _WFD_DEV_TYPE
{
	WFD_DEV_TYPE_SOURCE = 0,
	WFD_DEV_TYPE_PRIMARY_SINK = 1,
	WFD_DEV_TYPE_SECONDARY_SINK = 2,
	WFD_DEV_TYPE_SOURCE_PRIMARY_SINK = 3,
}WFD_DEV_TYPE, *PWFD_DEV_TYPE;
#define	WFD_MASK_DEV_TYPE_MAP(_val)	((u2Byte)(_val) & 0x03)
// Bit 2
#define	WFD_DEV_SOURCE_COUPLED_SINK_SUPPORT			BIT2
// Bit 3
#define	WFD_DEV_SINK_COUPLED_SINK_SUPPORT			BIT3
// Bit 4 ~ 5
#define	WFD_DEV_SESSION_AVAILABILITY				BIT4
// Bit 6
#define	WFD_DEV_WSD_SUPPORT							BIT6
// Bit 7
#define	WFD_DEV_PC_TDLS								BIT7
// Bit 8
#define	WFD_DEV_CONTENT_PROTECTION					BIT8
// Bit 9
#define	WFD_DEV_TIME_SYNC							BIT9
// Bit 10
#define	WFD_DEV_PRIMARY_SINK_AUDIO_UN_SUPPORT		BIT10
// Bit 11
#define	WFD_DEV_SOURCE_AUDIO_ONLY_SUPPORT			BIT11
// Bit 12
#define	WFD_DEV_TDLS_PERSISTENT_GROUP				BIT12
// Bit 13
#define	WFD_DEV_TDLS_PERSISTENT_RE_INVOKE			BIT13

// Get device type in type WFD_DEV_TYPE
#define	WFD_GET_DEV_TYPE(_pWfdInfo)	(WFD_DEV_TYPE)(WFD_MASK_DEV_TYPE_MAP((_pWfdInfo)->deviceInfo))
#define	WFD_SET_DEV_TYPE(_pWfdInfo, _devType)		\
{	\
	CLEAR_FLAG((_pWfdInfo)->deviceInfo, 0x3);	\
	SET_FLAG((_pWfdInfo)->deviceInfo, WFD_MASK_DEV_TYPE_MAP(_devType));	\
}

// WFD Coupled Sink Information Status Bitmap
// Bit 0 ~ 1
#define	WFD_COUPLED_STATUS_AVAILABLE				0
#define	WFD_COUPLED_STATUS_COUPLED					BIT1
#define	WFD_COUPLED_STATUS_TEARDOWN					BIT2

// WFD Extended Capabilitied Bitmap
#define	WFD_EXT_CAP_UIBC_SUPPORT					BIT0
#define	WFD_EXT_CAP_I2C_SUPPORT						BIT1
#define	WFD_EXT_CAP_PRE_FERED_DISPLAY_SUPPORT		BIT2
#define	WFD_EXT_CAP_STANDBY_RESUME_SUPPORT			BIT3
#define	WFD_EXT_CAP_TDLS_PERSISTENT_SUPPORT			BIT4
#define	WFD_EXT_CAP_TDLS_BSSID_PERSISTENT_SUPPORT	BIT5

#define	WFD_DESC_SRC					u1Byte
#define	WFD_DESC_SRC_PROBE_RSP			(WFD_DESC_SRC)(BIT0)	// From probe response (Device or GO)
#define	WFD_DESC_SRC_BEACON				(WFD_DESC_SRC)(BIT1)	// From beacon (GO)
#define	WFD_DESC_SRC_TUNNELED			(WFD_DESC_SRC)(BIT2)	// From TDLS tunneled discovery (Client with TDLS)
#define	WFD_DESC_SRC_SESSION_IE			(WFD_DESC_SRC)(BIT3)	// From Client
#define	WFD_DESC_SRC_P2P_NEG			(WFD_DESC_SRC)(BIT4)	// From P2P handshake packet
#define	WFD_DESC_SRC_ASSOC				(WFD_DESC_SRC)(BIT5)	// From association packet

#define	WFD_INFO_DEVICE_TYPE			(u4Byte)0x00000001

#pragma pack(1)
typedef struct _WFD_DEVICE_INFO_DESC
{
	u1Byte			deviceAddr[6];	// The mac address of this device
	u1Byte			owerDevAddr[6];	// The AP address for the client (srcType must be WFD_DESC_SRC_SESSION_IE)
	WFD_DESC_SRC	srcType;	// The type which this desc got from. This field shall be set as WFD_DESC_SRC_SESSION_IE for client descriptors in GO.
	u1Byte			assocBSSID[6];	// The associated BSSID
	u2Byte			deviceInfo;	// Device Information
	u2Byte			controlPort;	// Session Control Port
	u2Byte			maxTP;			// Maximum average throughput in multiples of 1 Mbps
	u1Byte			coupledSinkStatus;
	u1Byte			coupledSinkAddr[6];	// Mac Address of coupled sink
	u2Byte			extCap;				// Extended capabilities
	u1Byte			ipAddress[4];	// IPv4 address
	u1Byte			altMacAddr[6];	// Alternative MAC Addr, this field are all 0 if invalid.
}WFD_DEVICE_INFO_DESC, *PWFD_DEVICE_INFO_DESC;

typedef struct _WFD_DESC_LIST
{
	u4Byte					descNum;		// Number of descriptor
	WFD_DEVICE_INFO_DESC	devDesc[1];		// Descriptor array in scan list.
}WFD_DESC_LIST, *PWFD_DESC_LIST;

#pragma pack()

typedef struct _WFD_CLIENT_ENTRY_INFO
{
	BOOLEAN					bWfdClient;		// If TRUE, it is a WFD client.
	PVOID					pEntry;			// The entry of RT_WLAN_STA.
	WFD_DEVICE_INFO_DESC	deviceDesc;		// WFD_DEVICE_INFO_DESC	
}WFD_CLIENT_ENTRY_INFO, *PWFD_CLIENT_ENTRY_INFO;

#define	WFD_DEFAULT_CONTROL_PORT	8554
#define	WFD_DEFAULT_MAX_TH			300	// Mbps

// WiFi Display information context.
typedef struct _WFD_INFO
{
	u4Byte					curVer;	// The current WFD version.
	BOOLEAN					bWfdEnable;
	RT_SPIN_LOCK			wfdSpinLock;
	u2Byte					deviceInfo;	// WFD Device Information field
	u2Byte					controlPort;	// Session Management Control Port
	u2Byte					maxTP;	// WFD DEvice Maximum Throughput	
	u1Byte					coupledSinkStatus;	// Coupled Sink Status
	u1Byte					coupledSinkAddr[6];	// Mac Address of Coupled Sink
	u2Byte					extCapability;	// Extended Capabilities
	u4Byte					scanListCnt;	// Number of descriptor of scanList.
	PWFD_DEVICE_INFO_DESC	pScanList;	// The descriptor list of WFD scan.
	PWFD_CLIENT_ENTRY_INFO	pClientList;	// The descriptor list of WFD client when we are GO.
	u1Byte					ipv4Addr[4];	// IP address.
} WFD_INFO, *PWFD_INFO;

// ================ WFD Version ======================================

// The current support version of WFD
#define	WFD_MIN_SUPPORT_VER					2
#define	WFD_MAX_SUPPORT_VER					2

#define	WFD_OBJECT_VERSION_DONT_CARE		0
#define	WFD_OBJECT_VERSION_INVLAID			0xFFFFFFFF

// ================ WFD_REQ_ID ======================================
//
// WFD_REQ_ID_XXX,

#define	WFD_REQ_ID							u4Byte
#define	WFD_REQ_ID_MIN_SUPPORT_VERSION		(WFD_REQ_ID)0x00000001
#define	WFD_REQ_ID_MAX_SUPPORT_VERSION		(WFD_REQ_ID)0x00000002
#define	WFD_REQ_ID_CURRENT_VERSION			(WFD_REQ_ID)0x00000003					

#define	WFD_REQ_ID_WFD_MODE					(WFD_REQ_ID)0x00000010
#define	WFD_REQ_ID_DEVICE_TYPE				(WFD_REQ_ID)0x00000011
#define	WFD_REQ_ID_SESSION_AVAILABILITY		(WFD_REQ_ID)0x00000012
#define	WFD_REQ_ID_CONTROL_PORT				(WFD_REQ_ID)0x00000013
#define	WFD_REQ_ID_IPV4_ADDR				(WFD_REQ_ID)0x00000014
#define	WFD_REQ_ID_SCAN_LIST				(WFD_REQ_ID)0x00000015
#define	WFD_REQ_ID_PREFERED_CONNECTION		(WFD_REQ_ID)0x00000016
#define	WFD_REQ_ID_HDCP_ENABLE				(WFD_REQ_ID)0x00000017


#define	WFD_REQ_ID_UNKOWN					(WFD_REQ_ID)0xFFFFFFFF
#endif // #ifndef __INC_WIDI_TYPE_H
