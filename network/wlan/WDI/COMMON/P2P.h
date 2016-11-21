#ifndef __INC_P2P_11_H
#define __INC_P2P_11_H


#define P2P_VERSION				0x00000005

// For Intel Graphics driver running as Miracast source, it checks the transmit speed and determines the support resolutions.
// The min speed to support 1920x1080 is 300Mbps.
#define	INTEL_HIGH_RESOLUTION_MIN_XMIT_SPEED	300000000	// bps


//======================================================================
// Wi-Fi Direct Start
//======================================================================

// For simulating managed AP in soft AP mode.
#define	P2P_SIMULATE_MANAGED_AP		0


#ifndef MIN
    #define MIN(a, b)                  ((a) < (b)? a : b)
#endif  // MIN
#ifndef MAX
    #define MAX(a, b)                  ((a) > (b)? a : b)
#endif  // MAX


#define IS_5G_CHANNEL(__channel)	\
	((__channel) >= 36 && (__channel) <= 165) // TODO: this is a incorrect definition

#define	P2P_UPDATE_MANAGED_INFO_PERIOD	3 // * watchdog 

#define P2P_MAX_NUM_NOA_DESC 2

#define	P2P_PS_TIMEOUT_TOLERANCE	1024 // 1 TU
#define	P2P_TBTT_SEND_BEACON_TIME	(5 * 1024) // 5 TUs

// Scan Related
#define P2P_MAX_DISCOVERABLE_INTERVAL 3
#define P2P_MIN_DISCOVERABLE_INTERVAL 1
#define P2P_DEFAULT_MGNT_PERIOD 100 //MS
#define P2P_DEFAULT_MGNT_PERIOD_SHORT 20 //MS

#define P2P_SCAN_PERIOD_SCAN P2P_DEFAULT_MGNT_PERIOD //ms
#define P2P_SCAN_PERIOD_SEARCH 100 - 20 //ms
#define P2P_SWITCH_CHNL_PERIOD 5 //ms

#define P2P_SCAN_FIND_PHASE_LOOP_TIMES 32 // times of looping between listen and find phase before sending GONReq 

// Block the scan request from the default port to prevent channel switching which causes P2P handshake fail.
#define	P2P_BLOCK_NORMAL_SCAN_PERIOD	3000000 // us

//
// Device Discovery code
//
#define P2P_DEV_DISC_SCAN_ONLY 0x00
#define P2P_DEV_DISC_STOP 0xFD
#define P2P_DEV_DISC_INFINITE 0xFF

// Configuration Timeout, in units of 10ms
#define P2P_GO_CONFIG_TIMEOUT 0xFF // 2550ms
#define P2P_CLIENT_CONFIG_TIMEOUT 0x00

// Frame exchange timeout
#define P2P_GO_NEGO_FRAME_TIMEOUT 2000 //ms
#define P2P_INVITATION_FRAME_TIMEOUT 500 //ms
#define P2P_DEVICE_DISCOVERABILITY_FRAME_TIMEOUT 1000 //ms, need to consider peer client power saving mode
#define P2P_DEVICE_DISCOVERABILITY_BEACON_TIMEOUT 1000 // ms
#define P2P_PROVISION_DISCOVERY_TIMEOUT 2000 //ms

#define	P2P_PROVISION_DISCOVERY_SHORT_TIMEOUT	300	//ms
#define	P2P_PROVISION_DISCOVERY_RETRY_CNT		3
#define P2P_SERVICE_DISCOVERY_TIMEOUT 3000 // ms, this is the time for the entire SD protocol exchange including Comeback Req/Rsp
#define P2P_SERVICE_DISCOVERY_COMEBACK_TIMEOUT 300 // ms
#define	P2P_SCAN_WPS_GROUP_PERIOD		50 //ms

#define P2P_MS_TO_SLOTCOUNT(_Time) (((_Time)/P2P_DEFAULT_MGNT_PERIOD))

#define P2P_EXT_LISTEN_TIMING_DURATION_MS 400 //ms
#define P2P_EXT_LISTEN_TIMING_PERIOD_MS 3000 // ms

#define P2P_EXT_LISTEN_TIMING_PERIOD_SC P2P_MS_TO_SLOTCOUNT(pP2PInfo->ExtListenTimingPeriod)

#define P2P_NO_CLIENT_MAX_DURATION_MS 60000 //ms
#define P2P_NO_CLIENT_PERIOD_SC P2P_MS_TO_SLOTCOUNT(P2P_NO_CLIENT_MAX_DURATION_MS)

#define P2P_CLIENT_DISCONNECTED_MAX_DURATION_MS 10000 //ms
#define P2P_CLIENT_DISCONNECTED_PERIOD_SC P2P_MS_TO_SLOTCOUNT(P2P_CLIENT_DISCONNECTED_MAX_DURATION_MS)

#define P2P_FORCE_SCAN_LIST_INDICATE_DURATION_MS 5000 //ms
#define P2P_FORCE_SCAN_LIST_INDICATE_PERIOD_SC P2P_MS_TO_SLOTCOUNT(P2P_FORCE_SCAN_LIST_INDICATE_DURATION_MS)

#define P2P_SD_COMEBACK_REQ_TIMEOUT_MS 1000 //ms
#define P2P_SD_COMEBACK_REQ_TIMEOUT_SC P2P_MS_TO_SLOTCOUNT(P2P_SD_COMEBACK_REQ_TIMEOUT_MS)

#define P2P_ELEMENT_ID 0xDD

#define P2P_MAX_P2P_CLIENT  16  // shall be no greater than ASSOCIATE_ENTRY_NUM

#define P2P_MAX_SCAN_LIST 64

typedef enum _P2P_SUPPORT_STATE
{
	P2P_SUPPORT_STATE_NOT_SUPPORT = 0,
	P2P_SUPPORT_STATE_UNINITIALIZED = 1,
	P2P_SUPPORT_STATE_RTK_SUPPORT = 2,
	P2P_SUPPORT_STATE_OS_SUPPORT = 3,
}P2P_SUPPORT_STATE, *PP2P_SUPPORT_STATE;

#define	P2P_ADAPTER_OS_SUPPORT_P2P(_pAdapter)	\
	((P2P_SUPPORT_STATE_OS_SUPPORT == (_pAdapter)->P2PSupport) ? TRUE : FALSE)


#define	P2P_ADAPTER_RTK_SUPPORT_P2P(_pAdapter)	\
	((P2P_SUPPORT_STATE_RTK_SUPPORT == (_pAdapter)->P2PSupport) ? TRUE : FALSE)


#define P2P_SERVICE_DESC_LIST_SIZE 8 // we may have a list to record the info of the services we have
#define P2P_SERVICE_MAX_RSP_FRAG_THRESHOLD 1500 // in bytes
#define	P2P_SERVICE_MIN_RSP_FRG_THRESHOLD	32 // in bytes

// P2P IE Length field related
#define P2P_MAX_ATTRIBUTE_LEN 0xFFFF
#define P2P_IE_ATTR_LEN_WRITE_EF(_ptr, _val) (WriteEF2Byte(_ptr, _val))
#define P2P_IE_ATTR_LEN_READ_EF(_ptr) (ReadEF2Byte(_ptr))

#define	P2P_SKIP_SCAN_LOOP_CNT		1	// Skip scan countr when any adapter is in connected state.
#define	P2P_SKIP_SCAN_DURATION_MS	150	// millisecond to skip P2P scan (at least one Beacon)

//
// We can't send frame of CCK rate to a P2P Device.
// 
//
#define P2P_LOWEST_RATE MGN_6M

#if ( P2P_SUPPORT == 1 ) 
#define P2P_ENABLED(_pP2P_INFO) ( ((_pP2P_INFO)->Role) != P2P_NONE)
#else
#define P2P_ENABLED(_pP2P_INFO) 0
#endif
#define P2P_FRAME_GET_TYPE(_pBuf) (EF1Byte((_pBuf)[0]) & 0xFC)

#define P2P_WILDCARD_MAC_ADDR(_addr) \
( \
	( ((_addr)[0] == 0xff) && ((_addr)[1] == 0xff) && \
	((_addr)[2] == 0xff) && ((_addr)[3] == 0xff) && \
	((_addr)[4] == 0xff) && ((_addr)[5] == 0xff) )  ? TRUE : FALSE \
)

typedef enum _P2P_MINOR_RESON_CODE
{
	P2P_MINOR_RESON_RESERVED = 0,
	P2P_MINOR_RESON_CROSS_CONNECTION = 1,
	P2P_MINOR_RESON_MANAGED_BIT = 2,
	P2P_MINOR_RESON_CONCURRENT_COEXISTENCE = 3,
	P2P_MINOR_RESON_INFRASTRUCTURE_MANAGED_BIT = 4,
}P2P_MINOR_RESON_CODE, *PP2P_MINOR_RESON_CODE;


#define FRAME_OFFSET_P2P_PUB_ACT_CATEGORY (sMacHdrLng + 0)
#define FRAME_OFFSET_P2P_PUB_ACT_ACTION (sMacHdrLng + 1)
#define FRAME_OFFSET_P2P_PUB_ACT_OUI (sMacHdrLng + 2)
#define FRAME_OFFSET_P2P_PUB_ACT_OUI_TYPE (sMacHdrLng + 5)
#define FRAME_OFFSET_P2P_PUB_ACT_OUI_SUBTYPE (sMacHdrLng + 6)
#define FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN (sMacHdrLng + 7)
#define FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS (sMacHdrLng + 8)


//
// GAS Init Req Frame
//
#define FRAME_OFFSET_GAS_INIT_REQ_CATEGORY (sMacHdrLng + 0)
#define FRAME_OFFSET_GAS_INIT_REQ_ACTION (sMacHdrLng + 1)
#define FRAME_OFFSET_GAS_INIT_REQ_DIALOG_TOKEN (sMacHdrLng + 2)
#define FRAME_OFFSET_GAS_INIT_REQ_AD_PROT_IE (sMacHdrLng + 3)
#define FRAME_OFFSET_GAS_INIT_REQ_LEN (sMacHdrLng + 7)
#define FRAME_OFFSET_GAS_INIT_REQ_QUERY_REQ_INFO_ID (sMacHdrLng + 9)
#define FRAME_OFFSET_GAS_INIT_REQ_QUERY_REQ_LEN (sMacHdrLng + 11)
#define FRAME_OFFSET_GAS_INIT_REQ_QUERY_REQ_OI (sMacHdrLng + 13)
#define FRAME_OFFSET_GAS_INIT_REQ_OUI_SUBTYPE (sMacHdrLng + 16)
#define FRAME_OFFSET_GAS_INIT_REQ_SERVICE_UPDATE_INDICATOR (sMacHdrLng + 17)
#define FRAME_OFFSET_GAS_INIT_REQ_SERVICE_REQ_TLV (sMacHdrLng + 19)

//
// Service Request TLV
//
#define OFFSET_SERVICE_REQ_TLV_LEN 0
#define OFFSET_SERVICE_REQ_TLV_SERVICE_PROT_TYPE 2
#define OFFSET_SERVICE_REQ_TLV_SERVICE_TRANSACTION_ID 3
#define OFFSET_SERVICE_REQ_TLV_QUERY_DATA 4

//
// GAS Init Rsp Frame
//
#define FRAME_OFFSET_GAS_INIT_RSP_CATEGORY (sMacHdrLng + 0)
#define FRAME_OFFSET_GAS_INIT_RSP_ACTION (sMacHdrLng + 1)
#define FRAME_OFFSET_GAS_INIT_RSP_DIALOG_TOKEN (sMacHdrLng + 2)
#define FRAME_OFFSET_GAS_INIT_RSP_STATUS_CODE_11U (sMacHdrLng + 3)
#define FRAME_OFFSET_GAS_INIT_RSP_COMEBACK_DELAY (sMacHdrLng + 5)
#define FRAME_OFFSET_GAS_INIT_RSP_AD_PROT_IE (sMacHdrLng + 7)
#define FRAME_OFFSET_GAS_INIT_RSP_LEN (sMacHdrLng + 11)
#define FRAME_OFFSET_GAS_INIT_RSP_QUERY_DATA (sMacHdrLng + 13)
#define FRAME_OFFSET_GAS_INIT_RSP_QUERY_RSP_INFO_ID (sMacHdrLng + 13)
#define FRAME_OFFSET_GAS_INIT_RSP_QUERY_RSP_LEN (sMacHdrLng + 15)
#define FRAME_OFFSET_GAS_INIT_RSP_QUERY_RSP_OI (sMacHdrLng + 17)
#define FRAME_OFFSET_GAS_INIT_RSP_OUI_SUBTYPE (sMacHdrLng + 20)
#define FRAME_OFFSET_GAS_INIT_RSP_SERVICE_UPDATE_INDICATOR (sMacHdrLng + 21)
#define FRAME_OFFSET_GAS_INIT_RSP_TLV (sMacHdrLng + 23)

//
// Service Response TLV
//
#define OFFSET_SERVICE_RSP_TLV_LEN 0
#define OFFSET_SERVICE_RSP_TLV_SERVICE_PROT_TYPE 2
#define OFFSET_SERVICE_RSP_TLV_SERVICE_TRANSACTION_ID 3
#define OFFSET_SERVICE_RSP_TLV_STATUS 4
#define OFFSET_SERVICE_RSP_TLV_RSP_DATA 5

//
// GAS Comeback Req
//
#define FRAME_OFFSET_GAS_COMEBACK_REQ_CATEGORY  (sMacHdrLng + 0)
#define FRAME_OFFSET_GAS_COMEBACK_REQ_ACTION  (sMacHdrLng + 1)
#define FRAME_OFFSET_GAS_COMEBACK_REQ_DIALOG_TOKEN  (sMacHdrLng + 2)

//
// GAS Comeback Rsp
//
#define FRAME_OFFSET_GAS_COMEBACK_RSP_CATEGORY  (sMacHdrLng + 0)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_ACTION  (sMacHdrLng + 1)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_DIALOG_TOKEN  (sMacHdrLng + 2)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_STATUS_11U  (sMacHdrLng + 3)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_FRAG_ID  (sMacHdrLng + 5)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_COMEBACK_DELAY  (sMacHdrLng + 6)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_AD_PROT_IE  (sMacHdrLng + 8)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_LEN  (sMacHdrLng + 12)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_QUERY_RSP  (sMacHdrLng + 14) // Start of Query Response
#define FRAME_OFFSET_GAS_COMEBACK_RSP_QUERY_RSP_INFO_ID  (sMacHdrLng + 14)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_QUERY_RSP_LEN  (sMacHdrLng + 16)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_QUERY_RSP_OI  (sMacHdrLng + 18)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_OUI_SUBTYPE  (sMacHdrLng + 21)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_SERVICE_UPDATE_INDICATOR  (sMacHdrLng + 22)
#define FRAME_OFFSET_GAS_COMEBACK_RSP_TLV  (sMacHdrLng + 24)

#define SET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(__pHeader, __Value) 		WriteEF1Byte(((pu1Byte)(__pHeader)) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN, __Value)
#define GET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(__pHeader) 				ReadEF1Byte(((pu1Byte)(__pHeader)) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN)

typedef	enum _P2P_PUBLIC_ACTION_TYPE {
	P2P_PUB_ACT_GO_NEGO_REQ = 0,
	P2P_PUB_ACT_GO_NEGO_RSP,
	P2P_PUB_ACT_GO_NEGO_CONFIRM,
	P2P_PUB_ACT_INVITATION_REQ,
	P2P_PUB_ACT_INVITATION_RSP,
	P2P_PUB_ACT_DEV_DISCOVERABILITY_REQ,
	P2P_PUB_ACT_DEV_DISCOVERABILITY_RSP,
	P2P_PUB_ACT_PROVISION_DISCOVERY_REQ,
	P2P_PUB_ACT_PROVISION_DISCOVERY_RSP,
	P2P_PUB_ACT_GAS_INITIAL_REQ = 10,
	P2P_PUB_ACT_GAS_INITIAL_RSP = 11,
	P2P_PUB_ACT_GAS_COMEBACK_REQ = 12,
	P2P_PUB_ACT_GAS_COMEBACK_RSP = 13,
}P2P_PUBLIC_ACTION_TYPE, *PP2P_PUBLIC_ACTION_TYPE;

typedef	enum _P2P_ACTION_TYPE {
	P2P_ACT_NOTICE_OF_ABSENCE = 0,
	P2P_PUB_PRESENCE_REQ,
	P2P_PUB_PRESENCE_RSP,
	P2P_PUB_GO_DISCOVERABILITY_REQ,
}P2P_ACTION_TYPE, *PP2P_ACTION_TYPE;

#pragma pack(1)

typedef enum _DEVICE_CAPABILITY {
	dcServiceDiscovery = 0x01,
	dcP2PClientDiscoverability = 0x02,
	dcConcurrentOperation = 0x04,
	dcP2PInfrastructureManaged = 0x08,
	dcP2PDeviceLimit = 0x10,
	dcP2PInvitationProcedure = 0x20,
} DEVICE_CAPABILITY, *PDEVICE_CAPABILITY;

typedef enum _GROUP_CAPABILITY {
	gcP2PGroupOwner = 0x01,
	gcPersistentP2PGroup = 0x02,
	gcP2PGroupLimit = 0x04,
	gcIntraBSSDistribution = 0x08,
	gcCrossConnection = 0x10,
	gcPersistentReconnect = 0x20,
	gcGroupFormation = 0x40, // set to 1 when the P2P Dev is operating as a GO in the Provisioning Phase of Group Formation and set to 0 otherwise
} GROUP_CAPABILITY, *PGROUP_CAPABILITY;

typedef enum _MANAGEABILITY_BITMAP
{
	maP2PDeviceManagement = BIT0,			// The P2P Device Management field set to 1 indicates that the WLAN AP supports Managed P2P Device.
	maP2PCrossConnectionPermitted = BIT1,	// The Cross Connection Permitted field set to 1 indicates that the WLAN AP permits P2P Concurrent Devices to offer Cross Connection.
	maP2PCoexistenceOptional = BIT2,
}MANAGEABILITY_BITMAP, *PMANAGEABILITY_BITMAP;

typedef enum _INVITATION_FLAGS {
	ifP2PInvitationType = 0x01,
} INVITATION_FLAGS, *PINVITATION_FLAGS;

typedef struct _P2P_WPS_ATTRIBUTES_DEVICE_TYPE {
	u2Byte CategoryId;
	u1Byte Oui[4];
	u2Byte SubCategoryId;
} P2P_WPS_ATTRIBUTES_DEVICE_TYPE, *PP2P_WPS_ATTRIBUTES_DEVICE_TYPE;

typedef struct _P2P_WPS_ATTRIBUTES {
	u2Byte ConfigMethod;
	u2Byte DevicePasswdId;
	P2P_WPS_ATTRIBUTES_DEVICE_TYPE PrimaryDeviceType;
	u1Byte SecondaryDeviceTypeLength;
	P2P_WPS_ATTRIBUTES_DEVICE_TYPE SecondaryDeviceTypeList[8];
	u1Byte DeviceName[32];
	u1Byte DeviceNameLength;
} P2P_WPS_ATTRIBUTES, *PP2P_WPS_ATTRIBUTES;

//
// WPS IE is big endian (network order)
// We store WPS attributes as host order, but when
// 1. a WPS attribute is recvd, we translate from network order to host order
// 2. transmitting a WPS attribute, we translate from host order to network order.
//
#define P2P_WPS_ATTR_TRANSMITTED_USING_BE 1

#if P2P_WPS_ATTR_TRANSMITTED_USING_BE
	#define P2P_WPS_ATTR_WRITE_EF_2_BYTE(_ptr, _val) ((*((UNALIGNED pu2Byte)(_ptr))) = H2N2BYTE(_val))
	#define P2P_WPS_ATTR_READ_EF_2_BYTE(_ptr) (N2H2BYTE((*((UNALIGNED pu2Byte)(_ptr)))))
#else
	#define P2P_WPS_ATTR_WRITE_EF_2_BYTE(_ptr, _val) (WriteEF2Byte(_ptr, _val))
	#define P2P_WPS_ATTR_READ_EF_2_BYTE(_ptr) (ReadEF2Byte(_ptr))
#endif

typedef struct _P2P_CLIENT_INFO_DISCRIPTOR {
	u1Byte DeviceAddress[6];
	u1Byte InterfaceAddress[6];
	u1Byte DeviceCapability;
	P2P_WPS_ATTRIBUTES WpsAttributes;
} P2P_CLIENT_INFO_DISCRIPTOR, *PP2P_CLIENT_INFO_DISCRIPTOR;

typedef enum _P2P_ROLE {
	P2P_NONE = 0, // default
	P2P_DEVICE = 1,
	P2P_CLIENT = 2,
	P2P_GO = 3,
} P2P_ROLE, *PP2P_ROLE;

//
// For these macros, actually gcP2PGroupOwner implies P2P_GO role.
// We do redundent check only for accuracy.
// Note that a GO may not be in operating state since it may be in invitation complete, scan complete,...
//
#define P2P_ACTING_AS_GO(_pP2PInfo) ((_pP2PInfo)->Role == P2P_GO)
#define P2P_ACTING_AS_CLIENT(_pP2PInfo) ((_pP2PInfo)->State == P2P_STATE_OPERATING && !P2P_ACTING_AS_GO((_pP2PInfo))) // operating but not GO
#define P2P_ACTING_AS_DEVICE(_pP2PInfo) ((_pP2PInfo)->Role == P2P_DEVICE)
#define P2P_DOING_PROVISION_DISCOVERY(_pP2PInfo) ((_pP2PInfo)->State >= P2P_STATE_PROVISION_DISCOVERY_REQ_SEND && (_pP2PInfo)->State <= P2P_STATE_GO_NEGO_COMPLETE)

#define P2P_DOING_SERVICE_DISCOVERY_REQ(_pP2PInfo) ((_pP2PInfo)->State <= P2P_STATE_SERVICE_DISCOVERY_COMEBACK_RSP_WAIT && (_pP2PInfo)->State >= P2P_STATE_SERVICE_DISCOVERY_REQ_SEND)

#define	GET_P2P_PUBLIC_ACT_FRAME_DIALOG_TOKEN(__Start)		ReadEF1Byte(((pu1Byte)__Start) + 31)
#define	SET_P2P_PUBLIC_ACT_FRAME_DIALOG_TOKEN(__Start, __Val)	WriteEF1Byte((pu1Byte)(__Start) + 31, __Val)

#define	GET_P2P_VENDOR_ACT_FRAME_DIALOG_TOKEN(__Start)		ReadEF1Byte(((pu1Byte)__Start) + 30)
#define	SET_P2P_VENDOR_ACT_FRAME_DIALOG_TOKEN(__Start, __Val)	WriteEF1Byte((pu1Byte)(__Start) + 30, __Val)

typedef enum _P2P_PROVISIONING_RESULT {
	P2P_PROVISIONING_RESULT_UNKNOWN = 0,
	P2P_PROVISIONING_RESULT_SUCCEED,
	P2P_PROVISIONING_RESULT_FAILED,
} P2P_PROVISIONING_RESULT, *PP2P_PROVISIONING_RESULT;

typedef enum _P2P_CURRENT_ICS_STATUS {
	P2P_CURRENT_ICS_STATUS_OFF = 0,
	P2P_CURRENT_ICS_STATUS_ON = 1,
	P2P_CURRENT_ICS_STATUS_UNKNOWN = 2,
} P2P_CURRENT_ICS_STATUS, *PP2P_CURRENT_ICS_STATUS;

typedef enum _P2P_STATE
{
	P2P_STATE_DISABLED = 0,
	P2P_STATE_INITIALIZED = 1,
	//P2P_STATE_DISCONNECT = 2, // obsolete

	//======================================================================
	// Device Discovery
	//======================================================================
	P2P_STATE_DEV_DISC_START = 11,
	P2P_STATE_SCAN = 12,
	P2P_STATE_LISTEN = 13,
	P2P_STATE_SEARCH = 14,
	P2P_STATE_SPECIAL_PEER_SEARCH = 15,	// Newly-added for finding specific peers
	P2P_STATE_DEV_DISC_COMPLETE = 16,

	//======================================================================
	// Provision Discovery
	//======================================================================
	P2P_STATE_PROVISION_DISCOVERY_REQ_SEND = 21,
	P2P_STATE_PROVISION_DISCOVERY_RSP_WAIT = 22,
	P2P_STATE_PROVISION_DISCOVERY_COMPLETE = 23,

	//======================================================================
	// Group Formation
	//======================================================================
	// formator: pre formation -> req send -> rsp wait -> comp
	// formatee: req recvd -> rsp send -> confirm wait -> comp
	//P2P_STATE_PRE_GROUP_FORMATION = 31,
	P2P_STATE_GO_NEGO_REQ_SEND = 32,
	P2P_STATE_GO_NEGO_REQ_RECVD = 33,
	P2P_STATE_GO_NEGO_RSP_SEND = 34,
	P2P_STATE_GO_NEGO_RSP_WAIT = 35,
	P2P_STATE_GO_NEGO_CONFIRM_WAIT = 36,
	P2P_STATE_GO_NEGO_COMPLETE = 37,
	P2P_STATE_GO_NEGO_CONFIRM_SENT_WAIT = 38,

	//======================================================================
	// WPS
	//======================================================================
	P2P_STATE_PRE_PROVISIONING = 41,
	P2P_STATE_PROVISIONING = 42,

	//======================================================================
	// Group Operation
	//======================================================================
	P2P_STATE_OPERATING = 51,

	//======================================================================
	// Invitation
	//======================================================================
	// invitor: req send -> rsp wait -> rsp recvd -> comp
	// invitee: req recvd -> rsp send -> comp
	P2P_STATE_INVITATION_REQ_SEND = 61,
	P2P_STATE_INVITATION_RSP_WAIT = 62,
	P2P_STATE_INVITATION_REQ_RECVD = 63,
	P2P_STATE_INVITATION_COMPLETE = 64,

	//======================================================================
	// Device Discoverability
	//======================================================================
	P2P_STATE_DEVICE_DISCOVERABILITY_WAIT_BEACON = 71,
	P2P_STATE_DEVICE_DISCOVERABILITY_REQ_SEND = 72,
	P2P_STATE_DEVICE_DISCOVERABILITY_RSP_WAIT = 73,
	//P2P_STATE_GO_DISCOVERABILITY_REQ_SEND = 74,
	//P2P_STATE_GO_DISCOVERABILITY_REQ_RECVD = 75,
	P2P_STATE_DEVICE_DISCOVERABILITY_COMPLETE = 76,
	//======================================================================
	// Service Discovery
	//======================================================================
	// invitor: req send -> rsp wait -> rsp recvd -> comp
	// invitee: req recvd -> rsp send -> comp
	P2P_STATE_SERVICE_DISCOVERY_REQ_SEND = 81,
	P2P_STATE_SERVICE_DISCOVERY_RSP_WAIT = 82, // not to be dealt in the state machine
	//P2P_STATE_SERVICE_DISCOVERY_REQ_RECVD = 83,
	P2P_STATE_SERVICE_DISCOVERY_COMPLETE = 84,
	//P2P_STATE_SERVICE_DISCOVERY_COMEBACK_REQ_SEND = 85, // For the requestor to send ComebackReq
	P2P_STATE_SERVICE_DISCOVERY_COMEBACK_RSP_WAIT = 86, // For the requestor to wait ComebackRsp
	//P2P_STATE_SERVICE_DISCOVERY_COMEBACK_REQ_WAIT = 87, // For the requestee to wait ComebackReq

	P2P_STATE_MAX = 0xFF,
} P2P_STATE, *PP2P_STATE;

typedef struct _P2P_DEVICE_DISCRIPTOR {
	u1Byte WPS_UUID[16];
	P2P_ROLE Role;
	
	// From P2P Capability subelement
	u1Byte DeviceCapability;
	u1Byte GroupCapability;

	// From P2P Device Info
	u1Byte DeviceAddress[6]; // the key
	P2P_WPS_ATTRIBUTES WpsAttributes;

	// Channel
	u1Byte RegulatoryClass;
	u1Byte ListenChannel;
	u1Byte OperatingChannel;
	u1Byte CountryString[3];
	u1Byte ChannelPlanChannel[32];
	u1Byte ChannelPlanLength;

	// P2P Group Info
	P2P_CLIENT_INFO_DISCRIPTOR P2PClientDescriptorList[P2P_MAX_P2P_CLIENT];
	u1Byte P2PClientDescriptorListLength;

	// SSID
	u1Byte SsidBuf[32];
	u1Byte SsidLen;

	u1Byte GOIntent;

	// Configuration Timeout
	u1Byte GOConfigurationTimeout;
	u1Byte ClientConfigurationTimeout;

	u1Byte DialogToken;
	
	u1Byte GroupBssid[6]; //1  used for invitation

	//
	// This is valid after AsocReq, AsocRsp, GONReq, GONRsp is recvd
	//
	u1Byte IntendedP2PInterfaceAddress[6];
	
	u1Byte Status;
	u1Byte MinorReasonCode;

	u2Byte ExtendedListenTimingPeriod; //ms
	u2Byte ExtendedListenTimingDuration; // ms

	// v1Spec
	s2Byte SignalStrength;

	// v2Spec
	u4Byte version;
	u1Byte manufacturerName[65];

} P2P_DEVICE_DISCRIPTOR, *PP2P_DEVICE_DISCRIPTOR;

typedef struct _P2P_DEVICE_DESCRIPTOR_V0 {
	u1Byte WPS_UUID[16];
	P2P_ROLE Role;
	
	// From P2P Capability subelement
	u1Byte DeviceCapability;
	u1Byte GroupCapability;

	// From P2P Device Info
	u1Byte DeviceAddress[6]; // the key
	P2P_WPS_ATTRIBUTES WpsAttributes;

	// Channel
	u1Byte RegulatoryClass;
	u1Byte ListenChannel;
	u1Byte OperatingChannel;
	u1Byte CountryString[3];
	u1Byte ChannelPlanChannel[32];
	u1Byte ChannelPlanLength;

	// P2P Group Info
	P2P_CLIENT_INFO_DISCRIPTOR P2PClientDescriptorList[P2P_MAX_P2P_CLIENT];
	u1Byte P2PClientDescriptorListLength;

	// SSID
	u1Byte SsidBuf[32];
	u1Byte SsidLen;

	u1Byte GOIntent;

	// Configuration Timeout
	u1Byte GOConfigurationTimeout;
	u1Byte ClientConfigurationTimeout;

	u1Byte DialogToken;
	
	u1Byte GroupBssid[6]; //1  used for invitation

	//
	// This is valid after AsocReq, AsocRsp, GONReq, GONRsp is recvd
	//
	u1Byte IntendedP2PInterfaceAddress[6];
	
	u1Byte Status;
	u1Byte MinorReasonCode;

	u2Byte ExtendedListenTimingPeriod; //ms
	u2Byte ExtendedListenTimingDuration; // ms
} P2P_DEVICE_DESCRIPTOR_V0, *PP2P_DEVICE_DESCRIPTOR_V0;

typedef struct _P2P_DEVICE_DESCRIPTOR_V1 {
	P2P_DEVICE_DESCRIPTOR_V0	descV0;
	s2Byte 						SignalStrength;
} P2P_DEVICE_DESCRIPTOR_V1, *PP2P_DEVICE_DESCRIPTOR_V1;

typedef struct _P2P_DEVICE_DESCRIPTOR_V2 {
	P2P_DEVICE_DESCRIPTOR_V1	descV1;
	u4Byte						version;
	u1Byte						manufacturerName[65];
} P2P_DEVICE_DESCRIPTOR_V2, *PP2P_DEVICE_DESCRIPTOR_V2;


typedef struct _P2P_PROFILE {
	u1Byte OpChannel;
	u1Byte SsidBufLen;
	u1Byte SsidBuf[32];
}P2P_PROFILE, *PP2P_PROFILE;

// Description:
// 	Set by upper layer to let driver know the current profile list.
// 	Used when receiving reinvoke request to determine whether we have corresponding profile.
// 	Reply unknown group if we don't.
//
typedef struct _P2P_PROFILE_LIST_ENTRY
{
	P2P_ROLE							selfRole;
	UCHAR 							ssidBufLen;
	UCHAR 							ssidBuf[32];
	UCHAR								grpBssid[6];
}P2P_PROFILE_LIST_ENTRY, *PP2P_PROFILE_LIST_ENTRY;

typedef struct _P2P_PROFILE_LIST
{
	u4Byte							nProfiles;
	P2P_PROFILE_LIST_ENTRY		profileList[1];
}P2P_PROFILE_LIST, *PP2P_PROFILE_LIST;

typedef	struct _P2P_GO_FORMATED_INFO {
	P2P_ROLE				p2pRole;	// The role this device will be. Only GO and Client are vliad.
	P2P_DEVICE_DISCRIPTOR	targetDesc;	// The target descriptor that will be joined.
}P2P_GO_FORMATED_INFO, *PP2P_GO_FORMATED_INFO;

typedef struct _P2P_LIB_INVITATION_REQ_CONTEXT
{
	u1Byte DialogToken;
	P2P_ROLE InvitorRole;
	u1Byte TargetDeviceAddress[6];
	BOOLEAN bPersistent;
	u1Byte OpChannel;
	u1Byte GroupBssid[6];
	u1Byte GroupDeviceAddress[6];
	u1Byte GroupSsidLen;
	u1Byte GroupSsidBuf[32];
}P2P_LIB_INVITATION_REQ_CONTEXT, *PP2P_LIB_INVITATION_REQ_CONTEXT;

typedef struct _P2P_LIB_INVITATION_RSP_CONTEXT
{
	u4Byte Status;
	BOOLEAN bPersistent;
	u1Byte OpChannel;
	P2P_ROLE Role;
	u1Byte GroupSsidLen;
	u1Byte GroupSsidBuf[32];
}P2P_LIB_INVITATION_RSP_CONTEXT, *PP2P_LIB_INVITATION_RSP_CONTEXT;

typedef	struct	_P2P_POWERSAVE_NOA_SET
{
	BOOLEAN		bNoAEn;			// Indicate wheter the NoA is enabled.
	u4Byte		NoADur;			// The duration of NoA in micro-second.
	u4Byte		NoAInt;			// The interval of NoA in micro-second.
	BOOLEAN		bUseStartTime;	// If this is true, use the u4Start time as the start time, otherwisr caculate the start time automatically through the current TSF.
	u4Byte		u4StartTime;
	u1Byte		NoACnt;			// The number of NoA.
}P2P_POWERSAVE_NOA_SET, *PP2P_POWERSAVE_NOA_SET;

#define	P2P_PS_MIN_CTWIN	10	// 10 ms

typedef	struct	_P2P_POWERSAVE_SET
{
	BOOLEAN					bOppPs;			// if true, the GO will enter PS mode out of CTWindows.
	u1Byte					CTWindow;		// The period of time in TU after a TBTT during which the P2P GO is present.
	P2P_POWERSAVE_NOA_SET	NoASet[P2P_MAX_NUM_NOA_DESC];		// The array of NoA power settings.
}P2P_POWERSAVE_SET, *PP2P_POWERSAVE_SET;

typedef enum _P2P_SD_PROTOCOL
{
	P2P_SD_PROTOCOL_ALL_TYPE,
	P2P_SD_PROTOCOL_BONJOUR,
	P2P_SD_PROTOCOL_UPNP,
	P2P_SD_PROTOCOL_WS_DISCOVERY,
	P2P_SD_PROTOCOL_RSVD_4,
	P2P_SD_PROTOCOL_RSVD_5,
	P2P_SD_PROTOCOL_RSVD_6,
	P2P_SD_PROTOCOL_RSVD_7,
	P2P_SD_PROTOCOL_RSVD_8,
	P2P_SD_PROTOCOL_RSVD_9,
	P2P_SD_PROTOCOL_RSVD_10,
	P2P_SD_PROTOCOL_P2PSVC,
	P2P_SD_PROTOCOL_VENDOR_SPECIFIC = 255,
}P2P_SD_PROTOCOL, *PP2P_SD_PROTOCOL;

typedef enum _P2P_SD_STATUS_CODE
{
	P2P_SD_STATUS_SUCCESS,
	P2P_SD_STATUS_SERVICE_PROTOCOL_TYPE_UNAVAIABLE,
	P2P_SD_STATUS_REQUESTED_INFO_UNAVAIABLE,
	P2P_SD_STATUS_BAD_REQUEST,

	//  Defined by Realtek
	P2P_SD_STATUS_SERVICE_RSP_TIMEOUT,
	P2P_SD_STATUS_COMEBACK_REQ_TIMEOUT,
	P2P_SD_STATUS_COMEBACK_RSP_TIMEOUT,
	P2P_SD_STATUS_COMMON_CHANNEL_NOT_ARRIVED,
	P2P_SD_STATUS_BAD_RESPONSE_1,
	P2P_SD_STATUS_BAD_RESPONSE_2,
	P2P_SD_STATUS_BAD_RESPONSE_3,
	P2P_SD_STATUS_WAIT_FOR_COMEBACK_REQ,
	P2P_SD_STATUS_WAIT_FOR_COMEBACK_RSP,
}P2P_SD_STATUS_CODE, *PP2P_SD_STATUS_CODE;

#define P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN 1024
typedef struct _P2P_SERVICE_DESCRIPTOR
{
	P2P_SD_PROTOCOL ServiceType;
	u2Byte BufferLength; 
	u1Byte Buffer[P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN]; // The ServiceInfo is for the serivice info we provide
}P2P_SERVICE_DESCRIPTOR, *PP2P_SERVICE_DESCRIPTOR;

typedef struct _P2P_SERVICE_REQ_TLV
{
	u1Byte TransactionID;
	P2P_SERVICE_DESCRIPTOR ServiceDesc;
}P2P_SERVICE_REQ_TLV, *PP2P_SERVICE_REQ_TLV;

typedef struct _P2P_SERVICE_RSP_TLV
{
	u1Byte TransactionID;
	P2P_SERVICE_DESCRIPTOR ServiceDesc;
	P2P_SD_STATUS_CODE Status;
}P2P_SERVICE_RSP_TLV, *PP2P_SERVICE_RSP_TLV;

//
// This is the structure filled by UI to issue a SD Request
//
#define P2P_SD_MAX_SERVICES 10
typedef struct _P2P_SD_REQ_CONTEXT
{
	u1Byte TargetDeviceAddress[6]; // specifies the peer to query
	u1Byte ServiceReqTLVSize;
	P2P_SERVICE_REQ_TLV ServiceReqTLVList[1];
}P2P_SD_REQ_CONTEXT, *PP2P_SD_REQ_CONTEXT;

typedef struct _P2P_SD_RSP_CONTEXT
{
	P2P_SD_STATUS_CODE SDStatus;
	u1Byte SourceDeviceAddress[6];
	u2Byte ServiceUpdateIndicator;
	u1Byte ServiceRspTLVSize;
	P2P_SERVICE_RSP_TLV ServiceRspTLVList[1];
}P2P_SD_RSP_CONTEXT, *PP2P_SD_RSP_CONTEXT;

#define P2P_MAX_SERVICE_BUF_SIZE (sizeof(P2P_SD_RSP_CONTEXT) + (P2P_SD_MAX_SERVICES - 1) * sizeof(P2P_SERVICE_RSP_TLV))

#pragma pack()

typedef enum _P2P_STATUS_CODE {
	P2P_STATUS_SUCCESS = 0,
	P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE = 1,
	P2P_STATUS_FAIL_INCOMPATIBLE_PARAMETERS = 2,
	P2P_STATUS_FAIL_LIMIT_REACHED = 3,
	P2P_STATUS_FAIL_INVALID_PARAMETERS = 4,
	P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST = 5,
	P2P_STATUS_FAIL_PREVIOUS_PROTOCOL_ERROR_OR_DISRUPTIVE_BEHAVIOR = 6,
	P2P_STATUS_FAIL_NO_COMMON_CHANNELS = 7,
	P2P_STATUS_FAIL_UNKNOWN_P2P_GROUP = 8,
	P2P_STATUS_FAIL_BOTH_P2P_DEVICES_INDICATED_GO_INTENT_OF_15 = 9,
	P2P_STATUS_FAIL_INCOMPATIBLE_PROVISION_METHOD = 10,
	P2P_STATUS_FAIL_REJECTED_BY_USER = 11,
	P2P_STATUS_SUCCESS_ACCEPTED_BY_USER = 12,

	P2P_STATUS_FAIL_COMMON_CHANNEL_NOT_ARRIVED = 200,
	P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GON_RSP,
	P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GON_CONFIRM,
	P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GO_BEACON,
	P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_DEVICE_DISCOVERABILITY_RSP,
	P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_PROVISION_DISCOVERY_RSP,

	P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_INVITATION_RSP,

	P2P_STATUS_FAIL_PROVISION_DISCOVERY_FAILED,
	
	P2P_STATUS_FAIL_SERVICE_DISCOVERY_FAILED,

	P2P_STATUS_MAX = 255,
} P2P_STATUS_CODE, *PP2P_STATUS_CODE;

typedef struct _P2P_CONNECTION_CONTEXT {
	P2P_DEVICE_DISCRIPTOR ConnectingDevice;
	u1Byte Status;
	u1Byte DialogToken;
	u1Byte FindPhaseLoopTimes;
	BOOLEAN bGoingToBeGO;

	// WPS

	// Set after P2P_STATE_PRE_PROVISIONING until Provisioning succeeds
	BOOLEAN bDoingProvisioning;

	P2P_PROVISIONING_RESULT ProvisioningResult;

	BOOLEAN bProbePeerChannelList;
} P2P_CONNECTION_CONTEXT, *PP2P_CONNECTION_CONTEXT;

typedef struct _P2P_INVITATION_CONTEXT {
	P2P_ROLE InvitorRole;
	BOOLEAN bPersistentInvitation;
	u1Byte DialogToken;
	P2P_DEVICE_DISCRIPTOR InvitedDevice;
	BOOLEAN bToSendInvitationReqOnProbe; // when inviting a P2P Device, we have to wait for its ProbeReq so that we know its current chnl
	BOOLEAN bToUseDeviceDiscoverability;
	P2P_STATUS_CODE Status;
	BOOLEAN bInvitor;
	u1Byte SsidBuf[32];
	u1Byte SsidLen;
	u1Byte GroupBssid[6];
	u1Byte GODeviceAddress[6];
	u1Byte Channel; // channel that will be used to send the invitation req
	u1Byte OpChannel; // op channel in the invitation request

	BOOLEAN bWaitingBackwardInvite;
	
}P2P_INVITATION_CONTEXT, *PP2P_INVITATION_CONTEXT;

typedef struct _P2P_PROVISION_DISCOVERY_CONTEXT {
	BOOLEAN bDoingProvisionDiscovery;
	u1Byte devAddr[6];
	BOOLEAN go;
	u1Byte goBssid[6];
	u1Byte Channel;
	
	u1Byte SsidBuf[32];
	u1Byte SsidLen;
	
}P2P_PROVISION_DISCOVERY_CONTEXT, *PP2P_PROVISION_DISCOVERY_CONTEXT;

typedef struct _P2P_PERSISTENT_PROFILE {
	u1Byte SsidBuf[32];
	u1Byte SsidLen;
	u1Byte GroupBssid[6];
	u1Byte GODeviceAddress[6];
}P2P_PERSISTENT_PROFILE, *PP2P_PERSISTENT_PROFILE;

typedef struct _P2P_DEVICE_DISCOVERABILITY_CONTEXT {
	BOOLEAN			bWaitingBeaconFromGO;
	u1Byte			GoBssid[6];
	u1Byte			GODeviceAddr[6];
	u1Byte			GOSsidBuf[32];
	u1Byte			GOSsidLen;
	u1Byte			ClientDeviceAddress[6];
	u1Byte			DialogToken;
	u1Byte			GOChannel;
	P2P_STATUS_CODE	Status;
	u1Byte			OriginalChannel;
	P2P_STATE		OriginalState;
	BOOLEAN			bGoConnect; // Try to connect it if discoverability request is sent successfully.
}P2P_DEVICE_DISCOVERABILITY_CONTEXT, *PP2P_DEVICE_DISCOVERABILITY_CONTEXT;

typedef enum _WPS_ATTRIBUTE_TAG { // network order
	P2P_WPS_ATTR_TAG_CONFIG_METHODS = 0x1008,
	P2P_WPS_ATTR_TAG_DEVICE_NAME = 0x1011,
	P2P_WPS_ATTR_TAG_DEVICE_PASSWORD_ID = 0x1012,
	P2P_WPS_ATTR_TAG_UUID_E = 0x1047,
	P2P_WPS_ATTR_TAG_VERSION = 0x104A,
	P2P_WPS_ATTR_TAG_PRIMARY_DEVICE_TYPE = 0x1054,
	P2P_WPS_ATTR_TAG_SEC_DEVICE_TYPE_LIST = 0x1055,
	P2P_WP2_ATTR_TAG_REQUESTED_DEVICE_TYPE = 0x106A,
	P2P_WP2_ATTR_TAG_MANUFACTURER_NAME = 0x1021,
} WPS_ATTRIBUTE_TAG, *PWPS_ATTRIBUTE_TAG;

typedef enum _WPS_CONFIG_METHODS 
{
	P2P_WPS_CONFIG_METHODS_LABEL = 0x04,
	P2P_WPS_CONFIG_METHODS_DISPLAY = 0x08,
	P2P_WPS_CONFIG_METHODS_PUSHBUTTON = 0x80,
	P2P_WPS_CONFIG_METHODS_KEYPAD = 0x100,
	P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN = 0x1000,
}WPS_CONFIG_METHODS, *PWPS_CONFIG_METHODS;

typedef enum _WPS_DEVICE_PASSWD_ID 
{
	P2P_WPS_DEV_PASSWD_ID_DEFAULT =0x0000, 
	P2P_WPS_DEV_PASSWD_ID_USER_SPEC =0x0001, 
	P2P_WPS_DEV_PASSWD_ID_MACHINE_SPEC =0x0002, 
	P2P_WPS_DEV_PASSWD_ID_REKEY =0x0003,
	P2P_WPS_DEV_PASSWD_ID_PBC =0x0004, 
	P2P_WPS_DEV_PASSWD_ID_REG_SPEC =0x0005, 
	P2P_WPS_DEV_PASSWD_ID_RESERVED =0x0006,
	P2P_WPS_DEV_PASSWD_ID_WFDS_DEFAULT_PIN = 0x0008,
}WPS_DEVICE_PASSWD_ID , *PWPS_DEVICE_PASSWD_ID;

typedef struct _P2P_SD_CONTEXT
{
	BOOLEAN bDoingServiceDiscovery; // set when P2PServiceDiscoveryReq is successfully called and cleared when common channel arrived or failed
	BOOLEAN bRequester;

	P2P_SD_REQ_CONTEXT UserSDReq; // Set from upper layer and used to construct SDReq
	P2P_SERVICE_REQ_TLV UserSDReqTLVList[P2P_SD_MAX_SERVICES - 1];

	u1Byte DialogToken;
	u1Byte TransactionID;

	u1Byte TargetDeviceAddress[6];

	u2Byte ServiceUpdateIndicator;

	// The request we recieve
	u1Byte ServiceReqRecvdSize;
	P2P_SERVICE_REQ_TLV ServiceReqRecvd[P2P_SD_MAX_SERVICES];
	u1Byte ServiceRspRecvdSize;
	P2P_SERVICE_RSP_TLV ServiceRspRecvd[P2P_SD_MAX_SERVICES];

	// The buffer we used to construct Rsp TLV list
	u2Byte ANQPQueryRspFieldToSendSize;
	u1Byte ANQPQueryRspFieldToSendBuf[P2P_MAX_SERVICE_BUF_SIZE];
	u2Byte ANQPQueryRspFieldToSendOffset;

	P2P_SD_STATUS_CODE Status;
	u1Byte Channel; // the channel on which SDReq shall be sent or SDReq is recvd

	// For fragmentation
	BOOLEAN bFragment;
	u1Byte FragmentID;
}P2P_SD_CONTEXT, *PP2P_SD_CONTEXT;

typedef struct _P2P_NOA_DESCRIPTOR 
{
	BOOLEAN	bValid;
	u1Byte	CountOrType;
	u4Byte	Duration;
	u4Byte	Interval;
	u4Byte	StartTime;
	u8Byte	u8StartTime;	// For timeout reference purpose, and if need, update the beacon.
	u8Byte	u8EndTime;		// For NoA end reference or when need to update the beacon.
}P2P_NOA_DESCRIPTOR  , *PP2P_NOA_DESCRIPTOR ;

#define		P2P_PS_CANCEL_ALL_PS						0		// Cancel PS, keep awake
#define		P2P_PS_LEAVE_PS_BY_OUT_CTWIN_ClIENT_AWAKE	BIT0	// Leave sleep because one client is in active or our client is active.
#define		P2P_PS_LEAVE_PS_BY_NOA						BIT1	// Leave sleep because now is not in NoA.
#define		P2P_PS_ENTER_PS_BY_PERIODIC_NOA				BIT2	// Enter sleep because now is in periodic NoA.
#define		P2P_PS_ENTER_PS_BY_CTWIN					BIT3	// Enter sleep because now is out of CTWin (and no client is in active mode)
#define		P2P_PS_LEAVE_PS_BY_CTWIN_TBTT				BIT4	// Leave sleep because we are in CTWin.
#define		P2P_PS_LEAVE_PS_BY_TBTT						BIT5	// Leave sleep because we are in TBTT to send the Beacon
#define		P2P_PS_ENTER_PS_BY_1TIME_NOA				BIT6	// Enter sleep because we are in 1-time NoA

typedef enum 	_P2P_PS_STATE
{
	P2P_PS_AWAKE = 0,
	P2P_PS_DOZE_SEND_MGNT = 1, // For GO, it means scan process, just stop data queue.
	P2P_PS_DOZE = 2,
}P2P_PS_STATE, *PP2P_PS_STATE;

typedef enum	_P2P_PS_UPDATE_REASON
{
	P2P_PS_UPDATE_NONE = 0,					// Not Specified
	P2P_PS_UPDATE_BY_USER = 1,				// Send presence req for Client, or update NoA IE for GO
	P2P_PS_UPDATE_BY_PRESENCE_REQ = 2,		// Our GO receives the presence req from its client
	P2P_PS_UPDATE_BY_BEACON = 3,			// Our client receives the Beacon or Action frame including NoA IE.
	P2P_PS_UPDATE_BY_CLI_PS_MODE = 4,		// The Power Save mode changed
}P2P_PS_UPDATE_REASON, *PP2P_PS_UPDATE_REASON;

typedef struct _P2P_REG_CLASS_MAP {
	u1Byte 				RegClass;
	u1Byte 				MinChannel;
	u1Byte 				MaxChannel;
	u1Byte 				ChannelGap;
	CHANNEL_WIDTH		chWidth;
	EXTCHNL_OFFSET		chOffsest;
}P2P_REG_CLASS_MAP, *PP2P_REG_CLASS_MAP;

static P2P_REG_CLASS_MAP P2PRegClass[] = 
{
/*	{81, 1, 13, 1},
	{82, 14, 14, 1},
	{115, 36, 48, 4},
	{124, 149, 161, 4},
	{116, 36, 44, 8},
	{117, 40, 48, 8},
	{126, 149, 157, 8},
	{127, 153, 161, 8},
	{125, 165, 165, 1},
	{ 0, 0, 0, 0}
*/
	{81,	1,		13, 	1,		CHANNEL_WIDTH_20,	EXTCHNL_OFFSET_NO_EXT},
	{82,	14, 	14, 	1,		CHANNEL_WIDTH_20,	EXTCHNL_OFFSET_NO_EXT},
	
	//{83,	1,		6,		1,		CHANNEL_WIDTH_40, 	EXTCHNL_OFFSET_UPPER},
	//{84,	7,		13, 	1,		CHANNEL_WIDTH_40, 	EXTCHNL_OFFSET_NO_EXT},

	{115,	36, 	48, 	4,		CHANNEL_WIDTH_20,	EXTCHNL_OFFSET_NO_EXT},
	
	{116,	36, 	44, 	8,		CHANNEL_WIDTH_40, 	EXTCHNL_OFFSET_UPPER},
	{117,	40, 	48, 	8,		CHANNEL_WIDTH_40, 	EXTCHNL_OFFSET_NO_EXT},
	
	{124,	149,	161,	4,		CHANNEL_WIDTH_20,	EXTCHNL_OFFSET_NO_EXT},
	
	{126,	149,	157,	8,		CHANNEL_WIDTH_40, 	EXTCHNL_OFFSET_UPPER},
	{127,	153,	161,	8,		CHANNEL_WIDTH_40, 	EXTCHNL_OFFSET_NO_EXT},
	
	{125,	165,	165,	1,		CHANNEL_WIDTH_20,	EXTCHNL_OFFSET_NO_EXT},
	//---------------------------------------------------------------------------------
	{ 0, 0, 0, 0, CHANNEL_WIDTH_20, EXTCHNL_OFFSET_NO_EXT}
};


// For Win 8: OID_DOT11_WFD_DISCOVER_REQUEST -----------
#define P2P_DISCOVERY_SCAN_PHASE		BIT0
#define P2P_DISCOVERY_FIND_PHASE		BIT1
typedef ULONG P2P_DISCOVER_SEQUENCE;


#define P2P_MAX_DEVICE_ID_TO_SCAN		8
typedef struct _P2P_DEVICE_ID_TO_SCAN {
	ULONG uNumOfDeviceIDs;
	u1Byte DeviceIDs[P2P_MAX_DEVICE_ID_TO_SCAN][6];
} P2P_DEVICE_ID_TO_SCAN, * PP2P_DEVICE_ID_TO_SCAN; 
//----------------------------------------------------------------------------

// Save the raw packets of the devices found in the channel -------------------------------
typedef struct _P2P_DEVICE_LIST_ENTRY {

	// From MPDU
	u1Byte MacAddress[6];
	u1Byte DeviceAddress[6];

	MEMORY_BUFFER BeaconPacket;
	u8Byte BeaconHostTimestamp;
	
	MEMORY_BUFFER ProbeResponsePacket;
	u8Byte ProbeResponseHostTimestamp;

	u1Byte ChannelNumber;

	// From RT_RFD_STATUS
	s4Byte RecvSignalPower;
	u1Byte SignalStrength;

} P2P_DEVICE_LIST_ENTRY, *PP2P_DEVICE_LIST_ENTRY;

#define P2P_MAX_DEVICE_LIST	64
#define P2P_MAX_DEVICE_LIST_WHCK	15

typedef struct _P2P_DEVICE_LIST {
	u1Byte uNumberOfDevices;
	P2P_DEVICE_LIST_ENTRY DeviceEntry[P2P_MAX_DEVICE_LIST];
} P2P_DEVICE_LIST, *PP2P_DEVICE_LIST;

// + Action codes for the device list operation
#define P2P_DEVICE_LIST_ACTION_COPY_TO_QUERY_LIST	0x01
#define P2P_DEVICE_LIST_ACTION_CLEAR					0x02
#define P2P_DEVICE_LIST_ACTION_DUMP					0x03
//----------------------------------------------------------------------------

// Memory buffer action codes ----------------------------------------------------
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_BEACON								0x01
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_PROBE_RESPONSE					0x02
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_DEFAULT_REQUEST					0x03
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_PROBE_REQUEST						0x04
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_PROVISION_DISCOVERY_REQUEST		0x05
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_PROVISION_DISCOVERY_RESPONSE	0x06
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_GO_NEGOTIATION_REQUEST			0x07
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_GO_NEGOTIATION_RESPONSE			0x08
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_GO_NEGOTIATION_CONFIRM			0x09
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_INVITATION_REQUEST					0x0A
#define P2P_MEMORY_BUFFER_ACTION_ADDITIONAL_IE_INVITATION_RESPONSE				0x0B
//----------------------------------------------------------------------------

// Event ID for PlatformIndicateP2PEvent() -----------------------------------------
#define P2P_EVENT_NONE												0x00
#define P2P_EVENT_DEVICE_DISCOVERY_COMPLETE						0x01
#define P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE	0x02
#define P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST			0x03
#define P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE	0x04
#define P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE		0x05
#define P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE		0x06
#define P2P_EVENT_RECEIVED_GO_NEGOTIATION_REQUEST				0x07
#define P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE		0x08
#define P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE				0x09
#define P2P_EVENT_GO_NEGOTIATION_CONFIRM_SEND_COMPLETE		0x0A
#define P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM				0x0B
#define P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE				0x0C
#define P2P_EVENT_RECEIVED_INVITATION_REQUEST					0x0D
#define P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE				0x0E
#define P2P_EVENT_RECEIVED_INVITATION_RESPONSE					0x0F
#define P2P_EVENT_GO_OPERATING_CHANNEL							0x10
#define P2P_EVENT_DEV_FOUND										0x11


typedef union _P2P_EVENT_DATA {
	RT_STATUS rtStatus;
	MEMORY_BUFFER Packet;		
} P2P_EVENT_DATA, *PP2P_EVENT_DATA;
// ---------------------------------------------------------------------------

// Win8: For OID post process workitem --------------------------------------------------------------------
typedef enum _P2P_OID_POST_PROCESS_OPERATION {
	OID_OPERATION_NO_OPERATION,
	OID_OPERATION_SEND_PACKET,
	OID_OPERATION_INDICATE_DISCOVERY_COMPLETE
} P2P_OID_POST_PROCESS_OPERATION, * PP2P_OID_POST_PROCESS_OPERATION;
//----------------------------------------------------------------------------------------------------

typedef enum _P2P_CLIETN_JOIN_GROUP_WPS_STATE
{
	P2P_CLIETN_JOIN_GROUP_WPS_STATE_NONE = 0,	// No WPS state
	P2P_CLIETN_JOIN_GROUP_WPS_STATE_SCANNING,	// Scan WPS Group
	P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY,	// The WPS information of target GO is ready.
	P2P_CLIETN_JOIN_GROUP_WPS_STATE_ASSOCIATING,	// 802.11 Associating
	P2P_CLIETN_JOIN_GROUP_WPS_STATE_HANDSHAKING,	// WPS handshaking
}P2P_CLIETN_JOIN_GROUP_WPS_STATE, *PP2P_CLIETN_JOIN_GROUP_WPS_STATE;

// Win8: Information when Client starts to connect to GO ------------------------------------------------------
typedef struct _P2P_CLIENT_JOIN_GROUP_CONTEXT{

	BOOLEAN							bInGroupFormation;
	P2P_CLIETN_JOIN_GROUP_WPS_STATE	WpsState;

	u1Byte							uWaitForWpsSlotCount;			// 100 ms per slot
	RT_TIMER						P2PWaitForWpsReadyTimer;
	
} P2P_CLIENT_JOIN_GROUP_CONTEXT, *PP2P_CLIENT_JOIN_GROUP_CONTEXT;
// ----------------------------------------------------------------------------------------------------

// Win8: Set the Default Listen Channel and Operating Channel ----------
#define P2P_DEFAULT_LISTEN_CHANNEL					11
#define P2P_DEFAULT_OPERATING_CHANNEL				11
// -------------------------------------------------------------

// Win8: -----------------------------------------------------------
#define P2P_RESERVED_TIME_FOR_ACTION_FRAME_MS		30
//-----------------------------------------------------------------
#define	P2P_DEFAULT_GO_INTENT						14
#define	P2P_DEFAULT_GO_INTENT_TIE_BREAKER			1

typedef struct _P2P_INFO {
	DECLARE_RT_OBJECT(P2P_INFO);
	
	//======================================================================
	// Common
	//======================================================================
	u4Byte P2PVersion;
	P2P_ROLE Role;
	u8Byte	lastStateu8Time;
	P2P_STATE State;
	P2P_STATE PreviouslyIndicatedState;
	u8Byte PreviouslyIndicateStateTime;
		
	u1Byte DeviceAddress[6];
	u1Byte InterfaceAddress[6];
	
	u1Byte DeviceCapability;
	u1Byte GroupCapability;

	u1Byte GOIntent;

	BOOLEAN bProbeReqByLegacyClient;

	// Channel
	u1Byte RegulatoryClass;
	u1Byte OperatingChannel; // intended or preferred or actual op chnl
	u1Byte ListenChannel;
	u1Byte CountryString[3];
	//u1Byte ChannelPlanChannel[32]; // this is not to be modified but for reference only
	//u1Byte ChannelPlanLength;
	P2P_CHANNELS ChannelEntryList;
	
	//WPS
	P2P_WPS_ATTRIBUTES WpsAttributes;
	WPS_DEVICE_PASSWD_ID WpsDevPasswdId; // this is the source of our provisioning info
	
	// Configuration Timeout, in units of 10ms
	u1Byte GOConfigurationTimeout;
	u1Byte ClientConfigurationTimeout;

	// Scan List
	P2P_DEVICE_DISCRIPTOR ScanList[P2P_MAX_SCAN_LIST];
	u4Byte ScanListSize;
	P2P_DEVICE_DISCRIPTOR ScanList4Query[P2P_MAX_SCAN_LIST];
	u4Byte ScanList4QuerySize;
	P2P_STATE StateBeforeScan;
	u4Byte ForceScanListIndicateSlotCount;
	BOOLEAN bDeviceDiscoveryInProgress;
	
	// Connection
	P2P_CONNECTION_CONTEXT ConnectionContext;
	P2P_STATUS_CODE PreviousGONegoResult; // this is for Sigma query
	BOOLEAN bReinitiateConnection;
	u1Byte DevAddrToReConnect[6];
	u8Byte TimeStartWaitingForReinitiate;
	u4Byte msTimeWaitGoNegoConfirmSent;

	// Ext Listen Timing
	u4Byte ExtListenTimingPeriodSlotCount;
	BOOLEAN bExtendedListening;
	u2Byte ExtListenTimingPeriod;
	u2Byte ExtListenTimingDuration;
	
	// Status
	u1Byte Status;
	u1Byte MinorReasonCode;

	u1Byte DialogToken;
	u1Byte oidDialogToken;

	P2P_INVITATION_CONTEXT InvitationContext;
	BOOLEAN bAcceptInvitation;
	u1Byte AccpetInvitationDeviceAddress[6];
	P2P_DEVICE_DISCOVERABILITY_CONTEXT DeviceDiscoverabilityContext;

	// ICS
	P2P_CURRENT_ICS_STATUS CurrentIcsStatus;

	// Provision Discovery
	P2P_PROVISION_DISCOVERY_CONTEXT ProvisionDiscoveryContext;
	u1Byte							ProvisionReqRetryCnt;
	
	BOOLEAN bPreGroupFormation;

	BOOLEAN bSendProbeReqInExtendedListen;

	// Service Discovery
	P2P_SD_CONTEXT SDContext; // Used to do Service Discovery
	u2Byte SDRspFragmtntThreshold;
	u4Byte SDWaitForComebackReqSlotCount;

	// Profile list
	u4Byte profileListLen;
	PP2P_PROFILE_LIST pProfileList;

	//======================================================================
	// GO Specific
	//======================================================================
	BOOLEAN bGOStartedAutonomously;
	u4Byte P2PGONoClientSlotCount;
	u4Byte P2PClientDisconnectedSlotCount;
	u1Byte SSIDLen;
	u1Byte SSIDBuf[32];
	u1Byte SSIDPostfixLen;
	u1Byte SSIDPostfixBuf[32-9]; //32 -len("DIRECT-xx")
	u1Byte regSSIDBuf[32];		// Set by the upper layer
	u1Byte regSSIDLen;

	// PS related
	RT_P2P_PS_EXE_TYPE	psExeType;
	BOOLEAN				bUpdateFromBeacon; // When the inforamtion had ever updated, this flag set true.
	BOOLEAN				bUpdatePsParameter; // We need to update the PS parameter for USB interface in the next process.
	P2P_POWERSAVE_SET	P2pPsConfgSet;		// The temp p2p power save configuration for later use.
	u4Byte				PsFlag;
	u8Byte				NextTimeout;
	u4Byte				usSleepTime;
	P2P_PS_STATE		P2pPsState;
	u1Byte				NoAIEIndex;
	BOOLEAN 			bOppPS;
	u1Byte 				CTWindow;
	P2P_NOA_DESCRIPTOR	NoADescriptors[P2P_MAX_NUM_NOA_DESC];

	//======================================================================
	// P2P Managed
	//======================================================================
	BOOLEAN bWlanApRejection_CrossConnection; // set because of minor reason code 1 in deauth/disassoc from wlan ap
	BOOLEAN bWlanApRejection_Unmanaged; // minor reason code 2
	BOOLEAN bWlanApRejection_IncompatibleCoexistenceParameters; // minor reason code 3
	BOOLEAN bWlanApRejection_IncompatibleP2POperation; // minor reason code 4

	//======================================================================
	// Other 802.11 Info, to be carried in ProbeRsp constructed by IHV Service
	//======================================================================
	
	//======================================================================
	// Temporary Fields
	//======================================================================
	PADAPTER pAdapter; // this shall be the default adapter under win7


	// ActionTimer --------------------------------------------
	//	+ This may be used to replace P2PMgntTimer
	ACTION_TIMER_HANDLE		P2PActionTimer;
	// -------------------------------------------------------

		
	RT_TIMER P2PMgntTimer;
	u8Byte LastTimerFired;

	u1Byte		ManagedUpdateCnt;	// Update the managed info from Wlan AP.
	// 
	// If true, the SP is ended by the NoA, and the client must re-send the trigger frame to the P2P GO for a new SP.
	// In the P2P GO mode, it must automatically end all SPs for all clients no matter the final frame with the EOSP bit set, and shall
	// not send any WMM-PS packets to those clients without receiving the trigger frames. In P2P TestPlan 6.1.13 and 7.1.5.
	// By Bruce, 2010-06-29.
	BOOLEAN		bEospByNoA;

	// For P2P Power Save workitem.
	RT_WORK_ITEM	P2PPSWorkItem;


	PVOID		pP2PSvcInfo;

	P2P_ADD_IES AdditionalIEs;

	//======================================================================
	// Integrated device list
	//======================================================================
	P2P_DEV_LIST				devList;

	// For Win 8: OID_DOT11_WFD_GROUP_JOIN_PARAMETERS -----------------------
	P2P_CLIENT_JOIN_GROUP_CONTEXT ClientJoinGroupContext;
	// ----------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_DESIRED_GROUP_ID -----------------------------
	u1Byte DesiredTargetMacAddress[6];
	u4Byte uGroupTargetSSIDLength;
	u1Byte GroupTargetSSID[MAX_SSID_LEN];
	// ----------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_ADDITIONAL_IE  -----------------------------
	// -------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY  -----------------------------
	ULONG	uListenStateDiscoverability;			// How frequently the device will be found: Default(0)
	// -----------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_DISCOVER_REQUEST -------------------------------------------------------
	P2P_DISCOVER_SEQUENCE DiscoverSequence;		// Determine the discovery phases
	u1Byte	findPhaseSkipCnt;	// Counter to skip scanning when the other ports in connected state.
	RT_SCAN_TYPE ScanType;							// Determine the scan type in the scan phase
	P2P_DEVICE_ID_TO_SCAN ScanDeviceIDs;			// The Device IDs for the scan process
	BOOLEAN	bForceScanLegacyNetworks;				// Scan the legacy networks
	BOOLEAN	bDeviceDiscoveryIndicateToOS;			// Let the OID indicated only once 
	// + Search for Specific Channel
	BOOLEAN bDiscoverForSpecificChannels;			// Flag for enabling the search
	u1Byte DiscoverForSpecificChannels[10];			// Specific Channels
	u1Byte uNumberOfDiscoverForSpecificChannels;		// Number of Specific Channel
	u1Byte uNumberOfDiscoverRounds;					// Number of Discovery Rounds
	// + Save the Device Discovery Timing
	u8Byte LastDeviceDiscoveryOidIssueTime;			// Save the OS-Triggered Discovery Start Time
	u8Byte LastDeviceDiscoveryIndicatedTime;			// Save the OS-Triggered Discovery Complete Time
	// -------------------------------------------------------------------------------------------------

	// For saving the received raw packets ----------------------------------------------
	//	NOTE: These should be merged with the ScanList if the UI and IHV are all stable.
	P2P_DEVICE_LIST DeviceList; 
	P2P_DEVICE_LIST DeviceListForQuery;
	// -----------------------------------------------------------------------------
	
	// For Win 8: OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST --------------------------------------	
	u1Byte ProvisionRequestGroupCapability;
	BOOLEAN bProvisionRequestUseGroupID;
	u1Byte ProvisionRequestGroupIDDeviceAddress[6];
	u1Byte ProvisionRequestGroupIDSSID[MAX_SSID_LEN];
	u1Byte uProvisionRequestGroupIDSSIDLength;
	// -------------------------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE --------------------------------------	
	u1Byte ProvisionResponseReceiverDeviceAddress[6];
	u1Byte ProvisionResponseDialogToken;
	u1Byte ProvisionResponseConfigMethod;
	// -------------------------------------------------------------------------------------------------

	// For delaying operations from the OID command ----------------------------
	BOOLEAN	 bPostoneP2POidPostProcessWorkItem;
//	RT_WORK_ITEM P2POidPostProcessWorkItem;
	RT_TIMER	P2POidPostProcessTimer;
	P2P_OID_POST_PROCESS_OPERATION OidOperation;
	P2P_PUBLIC_ACTION_TYPE PacketSentInWorkItemCallback;
	//--------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST -------------------------
	u1Byte NegotiationRequestGroupCapability;
	u1Byte NegotiationRequestGOIntent;
	//u1Byte NegotiationRequestIntendedInterfaceAddress[6];
	//--------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE ------------------------
	u1Byte NegotiationResponsePeerDeviceAddress[6];
	u1Byte NegotiationResponseDialogToken;
	u1Byte NegotiationResponseStatus;
	u1Byte NegotiationResponseGroupOwnerIntent;
	//u1Byte NegotiationResponseIntendedInterfaceAddress[6];
	u1Byte NegotiationResponseGroupCapability;
	u1Byte NegotiationResponseGroupIDDeviceAddress[6];
	u1Byte NegotiationResponseGroupIDSSID[MAX_SSID_LEN];
	u1Byte uNegotiationResponseGroupIDSSIDLength;
	BOOLEAN bNegotiationResponseUseGroupID;
	//---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM ---------------------------
	u1Byte NegotiationConfirmPeerDeviceAddress[6];
	u1Byte NegotiationConfirmDialogToken;
	u1Byte NegotiationConfirmStatus;
	u1Byte NegotiationConfirmGroupCapability;
	u1Byte NegotiationConfirmGroupIDDeviceAddress[6];
	u1Byte NegotiationConfirmGroupIDSSID[MAX_SSID_LEN];
	u1Byte uNegotiationConfirmGroupIDSSIDLength;
	BOOLEAN bNegotiationConfirmUseGroupID;
	//---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_INVITATION_REQUEST ---------------------------
	u1Byte  InvitationRequestGroupBSSID[6];
	BOOLEAN bInvitationRequestUseGroupBSSID;
	u2Byte uInvitationRequestOperatingChannelNumber;
	BOOLEAN bInvitationRequestUseSpecifiedOperatingChannel;
	u1Byte InvitationRequestGroupIDDeviceAddress[6];
	u1Byte InvitationRequestPeerDeviceAddress[6];	
	u1Byte InvitationRequestGroupIDSSID[MAX_SSID_LEN];
	u1Byte uInvitationRequestGroupIDSSIDLength;
	BOOLEAN bInvitationRequestLocalGO;
	//---------------------------------------------------------------------------------

	
	// For Win 8: OID_DOT11_WFD_SEND_INVITATION_RESPONSE ------------------------------
	u1Byte InvitationResponseReceiverDeviceAddress[6];
	u1Byte InvitationResponseDialogToken;
	u1Byte InvitationResponseStatus;
	u1Byte  InvitationResponseGroupBSSID[6];
	BOOLEAN bInvitationResponseUseGroupBSSID;
	u2Byte uInvitationResponseOperatingChannelNumber;
	BOOLEAN bInvitationResponseUseSpecifiedOperatingChannel;

	// + Set the indication to OS when the invitation response completes
	BOOLEAN bInvitationResponseIndicateToOS;
	//---------------------------------------------------------------------------------


	// Debug Device Discovery  -------------------
	u2Byte	ProbeRequestSequenceNum;
	// ----------------------------------------
	

	// Trick for knowing next channel switch time -------
	s8Byte TimeStartToStopSendingProbeResponse;
	// -------------------------------------------

	u1Byte		RefCnt;	// If RefCnt > 0, the P2P configuration is on going.

	// ECSA
	BOOLEAN bChannelSwitch;
	WDI_P2P_CHANNEL_INDICATE_REASON P2PChannelIndicationReason;
} P2P_INFO, *PP2P_INFO;

#define	P2P_INC_REF_CNT(_pP2pInfo)	((_pP2pInfo)->RefCnt ++)
#define	P2P_DEC_REF_CNT(_pP2pInfo)	\
{	\
	if((_pP2pInfo)->RefCnt == 0)	\
	{	\
		;\
	}	\
	else	\
	{	\
		((_pP2pInfo)->RefCnt --);	\
	}	\
}
#define	P2P_GET_REF_CNT(_pP2pInfo)	((_pP2pInfo)->RefCnt)

//======================================================================
// P2P Utility
//======================================================================
BOOLEAN
P2PIsN24GSupported(
	IN PP2P_INFO pP2PInfo
	);

VOID
P2PSetRole(
	IN PP2P_INFO pP2PInfo,
	IN P2P_ROLE Role
	);
	
VOID
P2PGOSetBeaconInterval(
	IN PP2P_INFO pP2PInfo,
	u2Byte u2BeaconPeriod
	);

BOOLEAN
P2PIsChnlInChnlList(
	IN pu1Byte pChnlList,
	IN u1Byte ChnlListSize,
	IN u1Byte Chnl
	);

BOOLEAN
P2PIsDoingGroupFormation(
	IN PP2P_INFO pP2PInfo
	);

BOOLEAN
P2PCommonChannelArrived(
	IN PP2P_INFO pP2PInfo,
	IN P2P_DEV_LIST_ENTRY *pDev,
	IN OCTET_STRING osPacket
	);

OCTET_STRING 
P2PWpsIEGetAttribute(
	IN OCTET_STRING osWpsIEAttributes,
	IN BOOLEAN bWpsAttributesInBE,
	IN u2Byte Tag
	);

BOOLEAN
P2PDeviceTypeMatches(
	IN PP2P_INFO pP2PInfo,
	IN OCTET_STRING osWpsIE
	);

//======================================================================
// Dump Routine
//======================================================================

VOID 
P2PDumpDeviceCapability(
	IN u1Byte DeviceCapability
	);

VOID 
P2PDumpGroupCapability(
	IN u1Byte GroupCapability
	);

VOID
P2PDumpGroupFormationResult(
	PP2P_INFO pP2PInfo
	);

//======================================================================
// Scan List Hander
//======================================================================
u1Byte
P2PClientInfoGetCount(
	IN PP2P_INFO pP2PInfo
	);

PP2P_CLIENT_INFO_DISCRIPTOR 
P2PClientInfoFindByInterfaceAddress(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte InterfaceAddress
	);

PP2P_CLIENT_INFO_DISCRIPTOR 
P2PClientInfoFindByDeviceAddress(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte DeviceAddress
	);

PP2P_CLIENT_INFO_DISCRIPTOR 
P2PClientInfoEnumClients(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte StartIndex,
	OUT pu1Byte pIndex
	);

PP2P_DEVICE_DISCRIPTOR 
P2PScanListFind(
	IN PP2P_DEVICE_DISCRIPTOR pScanList,
	IN u4Byte ScanListSize,
	IN pu1Byte DeviceAddress, 
	IN pu1Byte InterfaceAddress, 
	OUT pu4Byte pScanListIndex
	);

BOOLEAN
P2PScanListAllFound(
	IN PP2P_INFO pP2PInfo
	);

PP2P_DEVICE_DISCRIPTOR 
P2PScanListAdd(
	IN OUT PP2P_DEVICE_DISCRIPTOR pScanList,
	IN OUT pu4Byte pScanListSize
	);

BOOLEAN
P2PScanListDumplicate(
	IN PP2P_DEVICE_DISCRIPTOR pScanList,
	IN u4Byte ScanListSize,
	IN PP2P_DEVICE_DISCRIPTOR pDevDesc
	);

VOID
P2PResetCommonChannelArrivingProcess(
	IN PP2P_INFO pP2PInfo
	);

BOOLEAN
P2PConstructScanList(
	IN 	P2P_INFO 				*pP2PInfo,
	IN  VOID					*req,
	IN  u1Byte					rate,
	IN  FRAME_BUF				*probeReqBuf
	);

//======================================================================
// Packet Constructor
//======================================================================

//======================================================================
// Customized Indications
//======================================================================
VOID
P2PIndicateOnProvisionDiscoveryRsp(
	IN PP2P_INFO pP2PInfo,
	IN u2Byte ConfigMethod,
	IN pu1Byte DeviceAddress
	);

VOID
P2PIndicateOnProvisionDiscoveryReq(
	IN PP2P_INFO pP2PInfo,
	IN u2Byte ConfigMethod,
	IN pu1Byte DeviceAddress,
	IN PP2P_WPS_ATTRIBUTES pWpsAttributes
	);

VOID
P2PIndicateOnInvitationReq(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_LIB_INVITATION_REQ_CONTEXT pLibInvitationReq
	);

VOID
P2PIndicateOnInvitationRsp(
	IN PP2P_INFO pP2PInfo,
	IN P2P_STATUS_CODE Status,
	IN BOOLEAN bPersistent,
	IN u1Byte OpChannel,
	IN P2P_ROLE Role,
	IN u1Byte GroupSsidLen,
	IN pu1Byte GroupSsidBuf
	);

VOID
P2PIndicateStartApRequest(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_PROFILE pP2PProfile
	);

VOID
P2PIndicateStopApRequest(
	IN PP2P_INFO pP2PInfo
	);

VOID
P2PIndicateGOFormatedInfo(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte Status,
	IN BOOLEAN bGoingToBeGO,
	IN PP2P_DEVICE_DISCRIPTOR pDevDesc
	);

VOID
P2PIndicateCurrentState(
	IN PP2P_INFO pP2PInfo,
	IN P2P_STATE CurrentState
	);

VOID
P2PIndicateCurrentRole(
	IN PP2P_INFO pP2PInfo,
	IN P2P_ROLE CurrentRole
	);

VOID
P2PIndicateDeviceReqClientGoFormation(
	IN	PP2P_INFO				pP2PInfo,
	IN	PP2P_DEVICE_DISCRIPTOR	pDevDesc
	);

VOID
P2PIndicateDeviceDiscoveryComplete(
	IN PP2P_INFO pP2PInfo
	);

VOID
P2PIndicateOnSDRsp(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_SD_CONTEXT pSDContext
	);

VOID
P2PIndicateOnSDReq(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_SD_CONTEXT pSDContext
	);

VOID
P2PIndicateScanList(
	IN PP2P_INFO pP2PInfo
	);

//======================================================================
// P2P Managmement
//======================================================================
#define P2P_MGNT_PERIOD 100 // in ms.
#define P2P_MGNT_MS_TO_SLOTCOUNT(_Time) (((_Time)/P2P_MGNT_PERIOD) + 1)

VOID
P2PInitializeChannelEntryList(
	PP2P_INFO pP2PInfo
);

VOID
P2PDeviceDiscoverForSpecificChannels(
	PP2P_INFO pP2PInfo,
	pu1Byte ChannelList,
	u1Byte uNumberOfChannel
);

BOOLEAN
P2PDeviceDiscovery(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte FindPhaseLoopTimes
	);

VOID
P2PExtendedListenStart(
	IN PP2P_INFO pP2PInfo
	);
	
VOID
P2PExtendedListenResetCounter(
	IN PP2P_INFO pP2PInfo
);

#if 0
VOID
P2PNormalScanComplete(
	IN PP2P_INFO pP2PInfo
	);
#endif

BOOLEAN
P2PProvisionDiscovery(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte MacAddress, // shall be the device address
	IN u2Byte ConfigMethod // should have exactly 1 bit set
	);

BOOLEAN
P2PConnect(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte DeviceAddress
	);

VOID
P2PDisconnect(
	IN PP2P_INFO pP2PInfo
	);

BOOLEAN 
P2PInvitePeerStart(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_LIB_INVITATION_REQ_CONTEXT pLibInvittionContext
	);

VOID
P2PInvitePeerComplete(
	IN PP2P_INFO pP2PInfo
	);

BOOLEAN
P2PDeviceDiscoverabilityReq(
	IN 	PP2P_INFO	pP2PInfo,
	IN 	pu1Byte		DeviceAddress,
	IN	BOOLEAN		bConnect
	);

RT_STATUS
P2P_OnAssocOK(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

u4Byte
P2PSetPowerSaveMode(
	IN	PP2P_INFO				pP2PInfo,
	IN	PP2P_POWERSAVE_SET		pP2pPs,
	IN	u1Byte					NoAIndex,
	IN	P2P_PS_UPDATE_REASON	psReason
	);

u4Byte
P2PUpdatePowerSavePara(
	IN	PP2P_INFO				pP2PInfo
	);

VOID
P2PInitializeTimer(
	PADAPTER pAdapter
);

VOID
P2PCancelTimer(
	PADAPTER pAdapter
);

VOID
P2PReleaseTimer(
	PADAPTER pAdapter
);

VOID
P2PSetPsState(
	IN	PP2P_INFO		pP2PInfo,
	IN	u4Byte			Source,
	IN	P2P_PS_STATE	P2pState,
	IN	u8Byte			Timeout	
	);

VOID
P2PPsTsf_Bit32_Toggle(
	IN	PP2P_INFO		pP2PInfo
	);

VOID
P2PNotifyClientPSChange(
	IN	PP2P_INFO		pP2PInfo
	);

VOID 
P2PServiceDiscoveryReq(
	PP2P_INFO pP2PInfo,
	PP2P_SD_REQ_CONTEXT pServiceQueryContent
	);

VOID 
P2PServiceDiscoveryRsp(
	PP2P_INFO pP2PInfo,
	PP2P_SD_RSP_CONTEXT pServiceRspContent
	);

u4Byte
P2PPresenceReq(
	IN	PP2P_INFO			pP2PInfo,
	IN	PP2P_POWERSAVE_SET	pP2pPs
	);

typedef struct _RT_WLAN_BSS	RT_WLAN_BSS, *PRT_WLAN_BSS;
VOID
P2PUpdateWlanApManagedInfo(
	IN	PP2P_INFO		pP2PInfo,
	IN	PRT_WLAN_BSS	pBssDesc
	);

//======================================================================
// Public Functions (Starting from Win8)
//======================================================================
#if 0
//-------------------------------------------------------------------------------
// For delaying sending the peer response packet from the OID command
//-------------------------------------------------------------------------------
VOID
P2POidPostProcessWorkItemCallback(
	IN	PVOID	pContext
);
#else
//-------------------------------------------------------------------------------
// For delaying sending the peer response packet from the OID command
//-------------------------------------------------------------------------------
VOID
P2POidPostProcessTimerCallback(
	IN PRT_TIMER		pTimer
);
#endif

//-------------------------------------------------------------------------------
// For delaying connection to wait for GO WPS ready
//-------------------------------------------------------------------------------
VOID
P2PWaitForWpsReadyTimerCallback(
	IN PRT_TIMER		pTimer
);

// -----------------------------------------
// Reset the Client Join Group Context
// -----------------------------------------
VOID
P2PResetClientJoinGroupContext(
	PP2P_INFO pP2PInfo
);

//-------------------------------------------------------------------------------
// Public P2P Utility Functions
//-------------------------------------------------------------------------------

BOOLEAN
P2PDeviceListActionInterface(
	PP2P_INFO pP2PInfo,
	IN u1Byte uAction,
	IN PP2P_DEVICE_LIST 	pDeviceList,
	IN PMEMORY_BUFFER	pInputBuffer,
	OUT PMEMORY_BUFFER pOutputBuffer
);

PP2P_DEVICE_LIST_ENTRY
P2PDeviceListFind(
	IN PP2P_DEVICE_LIST pDeviceList,
	IN pu1Byte pMacAddress
);

BOOLEAN
P2PAddScanDeviceID(
	PP2P_DEVICE_ID_TO_SCAN	pScanDeviceIDs,
	pu1Byte	pDeviceID
);

VOID
P2PClearScanDeviceID(
	PP2P_DEVICE_ID_TO_SCAN	pScanDeviceIDs
);

u1Byte
P2PGetChannel(
	IN PP2P_INFO pP2PInfo
);

BOOLEAN
P2PAdapterAcceptFrame(
	PADAPTER pAdapter,
	OCTET_STRING osPacket
);

#if (P2P_SUPPORT == 1)

#define P2P_DOING_DEVICE_DISCOVERY(_pP2PInfo) ((_pP2PInfo)->bDeviceDiscoveryInProgress || ((_pP2PInfo)->bDeviceDiscoveryIndicateToOS == TRUE) )
#define GET_P2P_INFO(__pAdapter) 	\
	(P2P_ADAPTER_OS_SUPPORT_P2P(__pAdapter) ? ((PP2P_INFO)(((__pAdapter)->MgntInfo).pP2PInfo)) : \
	((PP2P_INFO)((GetDefaultAdapter(__pAdapter))->MgntInfo.pP2PInfo)))
#define P2P_DOING_PURE_DEVICE_DISCOVERY(_pP2PInfo) (\
	!(((_pP2PInfo)->bPreGroupFormation || ((P2P_STATE_GO_NEGO_REQ_SEND <= (_pP2PInfo)->State) && (_pP2PInfo)->State <= P2P_STATE_GO_NEGO_COMPLETE)) || \
	(_pP2PInfo)->InvitationContext.bToSendInvitationReqOnProbe ||\
	(_pP2PInfo)->ProvisionDiscoveryContext.bDoingProvisionDiscovery ||\
	(_pP2PInfo)->SDContext.bDoingServiceDiscovery\
	)\
)
#define P2P_ACTING_IS_GO(_Adapter)		 (P2P_ACTING_AS_GO(GET_P2P_INFO(_Adapter)))
#define P2P_ACTING_IS_CLIENT(_Adapter)	 (P2P_ACTING_AS_CLIENT(GET_P2P_INFO(_Adapter)))
#define P2P_ACTING_IS_DEVICE(_Adapter)	 (P2P_ACTING_AS_DEVICE(GET_P2P_INFO(_Adapter)))

BOOLEAN
P2P_IsGoAcceptProbeReq(
	IN		PADAPTER		pAdapter,
	IN		POCTET_STRING	posMpdu,
	IN		POCTET_STRING	posSsidToScan
	);

RT_STATUS
P2P_OnBeacon(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu	
	);

RT_STATUS
P2P_OnProbeReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnProbeRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnAssocReqAccept(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_ClientOnDeauth(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_ClientOnDisassoc(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnGONReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnGONRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnGONConfirm(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnInvitationReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnInvitationRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnDeviceDiscoverabilityReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnDeviceDiscoverabilityRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnProvisionDiscoveryReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnProvisionDiscoveryRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnPresenceReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnGODiscoverabilityReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnSDReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnSDRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnSDComebackReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
P2P_OnSDComebackRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

VOID
P2PFreeAllocatedMemory(
	IN PADAPTER	pAdapter
);

VOID
P2P_FreeP2PInfo(
	IN	PADAPTER	pAdapter
	);

RT_STATUS
P2P_AllocP2PInfo(
	IN	PADAPTER	pAdapter
	);

BOOLEAN
P2PIsWildcardSsid(
	IN OCTET_STRING osSsid
	);

BOOLEAN
P2PIsChnlInChnlEntryList(
	IN P2P_CHANNELS *pChannelEntryList,
	IN u1Byte Chnl
	);

RT_STATUS
P2P_SetWpsIe(
	IN  const ADAPTER 			*pAdapter,
	IN  u2Byte 					iePayloadLen,
	IN  u1Byte 					*pIePayload
	);

VOID 
P2PDumpWpsAttributes(
	IN PP2P_WPS_ATTRIBUTES pP2PWpsAttributes
	);

VOID 
P2PDumpScanList(
	IN PP2P_DEVICE_DISCRIPTOR pDevDescriptors,
	IN u4Byte nDevDescriptors
	);

VOID 
P2PScanListClear(
	IN  P2P_INFO				*pP2PInfo
	);

VOID 
P2PScanListCopy(
	IN OUT PP2P_DEVICE_DISCRIPTOR pScanListDest,
	IN OUT pu4Byte pScanListDestSize,
	IN PP2P_DEVICE_DISCRIPTOR pScanListSrc,
	IN u4Byte ScanListSrcSize
	);

BOOLEAN
P2PScanListEqual(
	IN PP2P_DEVICE_DISCRIPTOR pScanList1,
	IN u4Byte ScanList1Size,
	IN PP2P_DEVICE_DISCRIPTOR pScanList2,
	IN u4Byte ScanList2Size
	);

BOOLEAN
P2PScanListIsGo(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte DeviceAddr
	);

u1Byte
P2PScanListCeaseScan(
	IN PP2P_INFO pP2PInfo
	);

VOID
P2PInitialize(
	IN PP2P_INFO pP2PInfo,
	IN PADAPTER pAdapter,
	IN u1Byte ListenChannel,
	IN u1Byte IntendedOpChannel,
	IN u1Byte GOIntent
	);

VOID
P2PDeviceDiscoveryComplete(
	IN PP2P_INFO pP2PInfo,
	IN BOOLEAN	bRecoverState
	);

VOID
P2PExtendedListenComplete(
	IN PP2P_INFO pP2PInfo
	);

VOID 
P2PGOStartAutomously(
	IN PP2P_INFO pP2PInfo
	);

VOID
P2PIndicateCurrentDevPasswdId(
	IN PP2P_INFO pP2PInfo,
	IN u2Byte CurrentDevPasswdId
	);

VOID
P2PIndicateClientConnected(
	IN	PP2P_INFO				pP2PInfo,
	IN	pu1Byte					ClientInterfaceAddr
	);

VOID
P2PIndicateClientDisconnected(
	IN	PP2P_INFO				pP2PInfo,
	IN	pu1Byte					ClientInterfaceAddr
	);

VOID
P2PIndicateChangingApChnl(
	IN  PP2P_INFO 		pP2PInfo,
	IN  u1Byte			oldChnl,
	IN  u1Byte			newChnl
	);

VOID
P2PMgntTimerCallback(
	IN PRT_TIMER		pTimer
	);

VOID
P2PMlmeAssociateRequest(
	PADAPTER		Adapter,
	pu1Byte			asocStaAddr,
	u4Byte			asocTmot,
	u2Byte			asCap,
	u2Byte			asListenInterval,
	BOOLEAN			Reassociate	
	);

VOID
P2PPsWorkItemCallback(
	IN	PVOID	pContext
	);

VOID
P2PPsTimeout(
	IN	PADAPTER			pAdapter
	);

VOID
P2POnLinkStatusWatchdog(
	IN		PADAPTER		Adapter
	);

VOID
P2PDisable(
	IN	PADAPTER		Adapter
	);

u4Byte
P2PTranslateP2PInfoToDevDesc(
	IN PP2P_INFO pP2PInfo,
	OUT PVOID pvDevDesc
	);

VOID
P2P_StartApRequest(
	IN	PADAPTER	Adapter
	);

VOID
P2P_APRemoveKey(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

VOID
P2P_APSetkey(
	IN	PADAPTER		Adapter,
	IN	u1Byte 			*pucMacAddr, 
	IN	u4Byte 			ulEncAlg
	);

VOID
P2P_FilerCck(
	IN		PADAPTER		Adapter,
	IN	OUT	BOOLEAN			*pbFilterCck
	);

VOID
P2P_ConstructAssociateReqFilterCck(
	IN		PADAPTER		Adapter,
	IN		OCTET_STRING	AsocReq,
	IN	OUT	BOOLEAN			*pbFilterCck
	);

VOID
P2P_UpdateScanList(
	IN		PADAPTER		Adapter
	);

BOOLEAN
P2P_ScanCallback(
	IN		PADAPTER		Adapter
	);

VOID
P2P_InactivePsCallback(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				InactievPsState
	);

RT_STATUS
P2PSetServiceFragThreshold(
	IN	PADAPTER	pAdapter,
	IN	u2Byte		serviceThreshold
	);

RT_STATUS
P2PGetServiceFragThreshold(
	IN	PADAPTER	pAdapter,
	IN	pu2Byte		pServiceThreshold
	);

RT_STATUS
P2PSetProfileList(
	IN	PADAPTER	pAdapter,
	IN	PP2P_PROFILE_LIST pProfileList
	);

RT_STATUS
P2P_SetP2PGoFullSSID(
	IN	PADAPTER	pAdapter,
	IN	pu1Byte		pSsidBuf,
	IN	u4Byte		ssidLen
	);

RT_STATUS
P2P_GetP2PGoFullSSID(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		maxSsidBufLen,
	IN	pu1Byte		pSsidBuf,
	IN	pu4Byte		pSsidLen
	);

BOOLEAN
IsP2PDeviceExisting(
	IN	PADAPTER	pAdapter
	);

BOOLEAN
IsRTKP2PDeviceExisting(
	IN	PADAPTER	pAdapter
	);

BOOLEAN
P2P_IPS_Accessible(
	IN	PADAPTER	pAdapter
	);

VOID
P2P_Construct_ProbeReqEx(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					ssidLen,
	IN  const u1Byte			*ssidBuf
	);

VOID
P2P_Construct_ProbeReq(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	);

RT_STATUS
P2P_CorrectDeviceCategory(
	IN	PADAPTER	pAdapter
	);

//======================================================================
// P2P Utility
//======================================================================
BOOLEAN
P2PIsSocialChannel(
	IN u1Byte Channel
	);

#else // #if (P2P_SUPPORT != 1)
#define	GET_P2P_INFO(__pAdapter)			NULL
#define	P2P_ACTING_IS_GO(_Adapter)			FALSE
#define	P2P_ACTING_IS_CLIENT(_Adapter)		FALSE
#define	P2P_ACTING_IS_DEVICE(_Adapter)		FALSE
#define	P2P_IsGoAcceptProbeReq(_pAdapter, _posMpdu, _posSsidToScan)	TRUE
#define	P2P_OnBeacon(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnProbeReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnProbeRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnAssocReqAccept(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnAssocOK(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_ClientOnDeauth(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_ClientOnDisassoc(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnGONReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnGONRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnGONConfirm(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnInvitationReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnInvitationRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnDeviceDiscoverabilityReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnDeviceDiscoverabilityRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnProvisionDiscoveryReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnProvisionDiscoveryRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnPresenceReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnGODiscoverabilityReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnSDReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnSDRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnSDComebackReq(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	P2P_OnSDComebackRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define P2PFreeAllocatedMemory(_pAdapter)
#define	P2P_FreeP2PInfo(_pAdapter)
#define	P2P_AllocP2PInfo(_pAdapter)	RT_STATUS_SUCCESS
#define	P2PNotifyClientPSChange(_pP2PInfo)
#define	P2PIsWildcardSsid(_osSsid)	FALSE
#define	P2PIsChnlInChnlEntryList(_pChannelEntryList, _Chnl)	FALSE
#define P2P_SetWpsIe(__pAdapter, __iePayloadLen, __pIePayload) RT_STATUS_SUCCESS
#define P2PDumpWpsAttributes(_pP2PWpsAttributes)
#define	P2PDumpScanList(_pDevDescriptors, _nDevDescriptors)
#define	P2PScanListClear(_pP2PInfo)
#define	P2PScanListCopy(_pScanListDest, _pScanListDestSize, _pScanListSrc, _ScanListSrcSize)
#define	P2PScanListEqual(_pScanList1, _ScanList1Size, _pScanList2, _ScanList2Size)	FALSE
#define	P2PScanListIsGo(_pP2PInfo, _InterfaceAddr)	FALSE
#define	P2PScanListCeaseScan(_pP2PInfo)	0
#define	P2PInitialize(_pP2PInfo, _pAdapter, _ListenChannel, _IntendedOpChannel, _GOIntent)	{(_pAdapter)->P2PSupport = P2P_SUPPORT_STATE_NOT_SUPPORT;}
#define	P2PDeviceDiscoveryComplete(_pP2PInfo, _RecoverState)
#define	P2PExtendedListenComplete(_pP2PInfo)
#define	P2PGOStartAutomously(_pP2PInfo)
#define	P2PIndicateClientConnected(_pP2PInfo, _ClientInterfaceAddr)
#define	P2PIndicateClientDisconnected(_pP2PInfo, _ClientInterfaceAddr)
#define	P2PIndicateChangingApChnl(pP2PInfo, oldChnl, newChnl)
#define	P2PMgntTimerCallback(_pTimer)
#define	P2PMlmeAssociateRequest(_Adapter, _asocStaAddr, _asocTmot, _asCap, _asListenInterval, _Reassociate)
#define	P2PPsWorkItemCallback(_pContext)
#define	P2PPsTimeout(_pAdapter)
#define	P2POnLinkStatusWatchdog(_Adapter)
#define	P2PDisable(_Adapter)
#define	P2PTranslateP2PInfoToDevDesc(_pP2PInfo, _pDevDesc)
#define	P2P_StartApRequest(_Adapter)
#define	P2P_APRemoveKey(_Adapter, _pSTA)
#define	P2P_APSetkey(_Adapter, _pucMacAddr, _ulEncAlg)
#define	P2P_FilerCck(_Adapter, _pbFilterCck)
#define	P2P_ConstructAssociateReqFilterCck(_Adapter, _AsocReq, _pbFilterCck)
#define	P2P_UpdateScanList(_Adapter)
#define	P2P_ScanCallback(_Adapter)	TRUE
#define	P2P_InactivePsCallback(_pAdapter, _InactievPsState)
#define	P2PSetServiceFragThreshold(_pAdapter, _serviceThreshold)	RT_STATUS_FAILURE
#define	P2PGetServiceFragThreshold(_pAdapter, _pServiceThreshold)	RT_STATUS_FAILURE
#define	P2PSetProfileList(_pAdapter, _pProfileList)					RT_STATUS_FAILURE
#define P2P_SetP2PGoFullSSID(_pAdapter, _pSsidBuf, _pSsidLen)		RT_STATUS_FAILURE
#define P2P_GetP2PGoFullSSID(_pAdapter, _maxSsidBufLen, _pSsidBuf, _pSsidLen)	RT_STATUS_FAILURE
#define	IsP2PDeviceExisting(_pAdapter)	FALSE
#define	IsRTKP2PDeviceExisting(_pAdapter)	FALSE
#define	P2P_IPS_Accessible(_pAdapter)	TRUE
#define	P2P_Construct_ProbeReq(_pBuf, _pP2PInfo)
#define	P2P_DOING_PURE_DEVICE_DISCOVERY(_pP2PInfo) 	FALSE
#define	P2P_DOING_DEVICE_DISCOVERY(_pP2PInfo)		FALSE
#define	GET_P2P_INFO(__pAdapter)	NULL
#define P2PIsSocialChannel(_Channel)    FALSE
#define	P2P_CorrectDeviceCategory(__pAdapter)	RT_STATUS_NOT_SUPPORT
#endif // #if (P2P_SUPPORT == 1)

#endif // #ifndef __INC_P2P_11_H

