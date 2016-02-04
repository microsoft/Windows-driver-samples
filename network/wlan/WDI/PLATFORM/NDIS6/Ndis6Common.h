////////////////////////////////////////////////////////////////////////////////
//
//	File name:		Ndis6Common.c
//	Description:	Define data structure, RT_NDIS6_COMMON, to keep NDIS6.x 
//					common information and export some common helper functions.
//
//	Author:			rcnjko
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_NDIS6_COMMON_H
#define __INC_NDIS6_COMMON_H

//
// Header files for the data structure referenced in RT_NDIS6_COMMON.
//
#include "Protocol802_11.h" // For OCTET_STRING, 2005.10.4, by rcnjko.

//
//
// Interlocked bit manipulation interfaces
//
#define PLATFORM_INTERLOCKED_SET_BITS(Flags, Flag) \
    InterlockedOr((PLONG)(Flags), Flag)

#define PLATFORM_INTERLOCKED_CLEAR_BITS(Flags, Flag) \
    InterlockedAnd((PLONG)Flags, ~(Flag))

#define IsCurrDCMode(pPSC)			(pPSC->PowerProfile == NdisPowerProfileBattery)


typedef	struct _RT_NDIS_DRIVER_CONTEXT
{
	PDRIVER_OBJECT		pDriverObject;				// From DriverEntry input
	NDIS_HANDLE			Ndis6MiniportDriverHandle;	// From NIDS_HANDLE registered by NdisMRegisterMiniportDriver.
	RT_LIST_ENTRY		IoDeviceList;				// The registered IO device.
	u1Byte				IoDeviceCount;				// The count of IoDeviceList.
	WCHAR				RegistryPath[256];			// Keep registry path for adaptors to access common registries
}RT_NDIS_DRIVER_CONTEXT, *PRT_NDIS_DRIVER_CONTEXT;

typedef struct _WDI_TXRX_STATISTICS {	
	u4Byte			numWdiTxDescInit;
	u4Byte			numWdiTxDescDeInit;
	u4Byte			numWdiTxDataSend;
	u4Byte			numWdiTxNBLs;
	u4Byte			numWdiTxTcbs;
	u4Byte			numWdiTxNicSend;
	u4Byte			numWdiTxCoalesce;
	u4Byte			numWdiTxDataSendDrop;
	u4Byte			numWdiTxPauseIndicate;
	u4Byte			numWdiTxRestartIndicate;
	u4Byte			numWdiCompletePkt;
	u4Byte			maxDeqNBL;
	u4Byte			maxTxNBInNBL;
	u4Byte			numCheckTxPause;
	u4Byte			numTxResourceAvailable;
	u4Byte			monitorTxPauseReason;
	
	u4Byte			numRxIndicateNBLToUE;
	u4Byte			numRxReturnNBLFromUE;
	u4Byte			numWdiRxStop;
	u4Byte			numWdiRxRestart;
	u4Byte			numWdiRxPause;
	u4Byte			numWdiRxResume;
} WDI_TXRX_STATISTICS , *PWDI_TXRX_STATISTICS ;
extern WDI_TXRX_STATISTICS		GLWdiTxRxStatistics;
extern u4Byte						GLWdiTestType;	// only for testing

typedef struct _WDI_CMD_STATISTICS
{
	u4Byte			numTaskTimeout;
} WDI_CMD_STATISTICS, *PWDI_CMD_STATISTICS;
extern WDI_CMD_STATISTICS		GLWdiCmdStatistics;
//
// There are some incompatibility between win9x and win2000+. On win9x, Call to
// NdisInitializeString will cause driver not loaded (doesn't enter DriverEntry).
// We use this macro the resolve the incompatibility problem. 2006.08.11, by shien chang.
//
#ifdef NDIS50_MINIPORT
#define NdisInitializeString(Destination,Source)\
{\
    PNDIS_STRING _D = (Destination);\
    UCHAR *_S = (Source);\
    WCHAR *_P;\
    _D->Length = (strlen(_S)) * sizeof(WCHAR);\
    _D->MaximumLength = _D->Length + sizeof(WCHAR);\
    NdisAllocateMemoryWithTag((PVOID*)(&(_D->Buffer)), _D->MaximumLength, 0x8180);\
    _P = _D->Buffer;\
    while(*_S != '\0'){\
        *_P = (WCHAR)(*_S);\
        _S++;\
        _P++;\
    }\
    *_P = UNICODE_NULL;\
}
#endif

//
// Attributes for Native 802.11.
// 2006.10.08, by shien chang.
//
#define NATIVE_802_11_MEDIA_TYPE					NdisMediumNative802_11
#define NATIVE_802_11_PHYSICAL_MEDIA_TYPE		NdisPhysicalMediumNative802_11
#define NATIVE_802_11_MTU_SIZE					(NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE)
#define NATIVE_802_11_MAX_XMIT_LINK_SPEED		54000000
#define NATIVE_802_11_MAX_RCV_LINK_SPEED			54000000
#define NATIVE_802_11_MAX_MULTICAST_LIST			MAX_MCAST_LIST_NUM
#define NATIVE_802_11_MEDIA_DUPLEX_STATE		MediaDuplexStateFull
#define NATIVE_802_11_DATA_BACK_FILL_SIZE		8
#define NATIVE_802_11_OPMODE_CAPABILITY_MAJOR_VERSION	2
#define NATIVE_802_11_OPMODE_CAPABILITY_MINOR_VERSION	0

// Upto WinBlue: Realtek supports the following physical layers in DOT11_PHY_TYPE
//  + dot11_phy_type_ofdm 	a (5G)
//  + dot11_phy_type_hrdsss	b (2.4G)
//  + dot11_phy_type_erp		g (2.4G)
//  + dot11_phy_type_ht		n (2.4G + 5G)
//  + dot11_phy_type_vht		ac (5G)
#define NATIVE_802_11_MAX_NUM_PHY_TYPES			6

#define NATIVE_802_11_CURR_NUM_PHY_TYPES		1

#define NATIVE_802_11_MAX_SCAN_SSID				MAX_SSID_TO_SCAN
#define NATIVE_802_11_MAX_DESIRED_BSSID			8
#define NATIVE_802_11_MAX_DESIRED_SSID			1
#define NATIVE_802_11_MAX_EXCLUDED_MACADDR		MAX_EXCLUDED_MAC_ADDRESS_LIST
#define NATIVE_802_11_MAX_PRIVACY_EXEMPTION		32
#define NATIVE_802_11_MAX_KEY_MAPPING_ENTRY		32
#define NATIVE_802_11_MAX_KEY_MAPPING_ENTRY_HCT	8
#define NATIVE_802_11_MAX_DEFAULT_KEY_ENTRY		4
#define NATIVE_802_11_MAX_WEP_KEY_LENGTH		13
#define NATIVE_802_11_WEP40_KEY_LENGTH			5
#define NATIVE_802_11_WEP104_KEY_LENGTH			13
#define NATIVE_802_11_MAX_PMKID_CACHE			3
#define NATIVE_802_11_MAX_PER_STA_DEFAULT_KEY	32

#define	WDI_802_11_MAX_NUM_BAND_TYPES			2
#define	WDI_802_11_MAX_NUM_PHY_TYPES			NATIVE_802_11_MAX_NUM_PHY_TYPES

#define MAX_PNP_RECONNENT_COUNTER				25

#define GUID_ACTIVE_POWERSCHEME \
	{0x31F9F286, 0x5084, 0x42FE, {0xB7, 0x20, 0x2B, 0x02, 0x64, 0x99, 0x37, 0x63 }}

#define GUID_TYPICAL_POWER_SAVINGS \
	{0x381b4222, 0xf694, 0x41f0, {0x96, 0x85, 0xff, 0x5b, 0xb2, 0x60, 0xdf, 0x2e}}

#define GUID_MIN_POWER_SAVINGS \
	{0x8C5E7FDA, 0xE8BF, 0x4A96, {0x9A, 0x85, 0xA6, 0xE2, 0x3A, 0x8C, 0x63, 0x5C}}

#define GUID_MAX_POWER_SAVINGS \
	{0xA1841308, 0x3541, 0x4FAB, {0xBC, 0x81, 0xF7, 0x15, 0x56, 0xF2, 0x0B, 0x4A}}

#define GUID_WLAN_POWER_MODE \
	{0x12BBEBE6, 0x58D6, 0x4636, {0x95, 0xBB, 0x32, 0x17, 0xef, 0x86, 0x7c, 0x1a}} 


//
// For MAC address processing. 2006.11.01, by shien chang.
//
#define N6_MAKE_WILDCARD_MAC_ADDRESS(addr) \
{ \
	pu1Byte pu1Addr = (pu1Byte)(addr); \
	pu1Addr[0] = 0xFF; pu1Addr[1] = 0xFF; pu1Addr[2] = 0xFF; \
	pu1Addr[3] = 0xFF; pu1Addr[4] = 0xFF; pu1Addr[5] = 0xFF; \
}

// David: DOT11 information element. 2006.12.01, by shien chang.
typedef struct {
    UCHAR   ElementID;      // Element Id
    UCHAR   Length;         // Length
} DOT11_INFO_ELEMENT, * PDOT11_INFO_ELEMENT;

//
// Status indication state machine, 2006.11.29, by shien chang.
//
#define IS_STATUS_CODE_SUCCESS(StatusCode) \
			(StatusCode == 0 ? TRUE : FALSE)
	
//
//	070524, rcnjko:
//
//	MiniportReserved[0]: Reference count. 
//		Initialize as (1 + # of NB) when we start processing the NBL.
//		Minus 1 at each time NB returned and end of NBL processing.
//		The reason to initialized it as (1 + # of NB) is to prevent 
//		completed NBL before all processing done.
//
//	MiniportReserved[1]: Pointer to the NB to processing.
//		Initialize to pointer of first NB in the NBL, 
//		and will be advaced to next one when we had performed Scatter/Gather operation.
//
#define RT_NBL_GET_REF_CNT(_NBL) \
			((ULONG_PTR)((_NBL)->MiniportReserved[0]))
#define RT_NBL_SET_REF_CNT(_NBL, _Count) \
			(ULONG_PTR)((_NBL)->MiniportReserved[0]) = _Count
#define RT_NBL_INCREASE_REF_CNT(_NBL) \
			{ \
				ULONG_PTR	_RefCount; \
				_RefCount = (ULONG_PTR)((_NBL)->MiniportReserved[0]); \
				_RefCount ++; \
				(ULONG_PTR)((_NBL)->MiniportReserved[0]) = _RefCount; \
			}
#define RT_NBL_DECREASE_REF_CNT(_NBL) \
			{ \
				ULONG_PTR	_RefCount; \
				_RefCount = (ULONG_PTR)((_NBL)->MiniportReserved[0]); \
				_RefCount --; \
				(ULONG_PTR)((_NBL)->MiniportReserved[0]) = _RefCount; \
			}

#define RT_NBL_NEXT_SEND(_NBL) ((_NBL)->MiniportReserved[1])


#define RT_WDI_NBL_GET_REF_CNT(_NBL) \
			((ULONG_PTR)((_NBL)->MiniportReserved[1]))

#define RT_WDI_NBL_SET_REF_CNT(_NBL, _Count) \
			(ULONG_PTR)((_NBL)->MiniportReserved[1]) = _Count

#define RT_WDI_NBL_INCREASE_REF_CNT(_NBL) \
			{ \
				ULONG_PTR	_RefCount; \
				_RefCount = (ULONG_PTR)((_NBL)->MiniportReserved[1]); \
				_RefCount ++; \
				(ULONG_PTR)((_NBL)->MiniportReserved[1]) = _RefCount; \
			}

#define RT_WDI_NBL_DECREASE_REF_CNT(_NBL) \
			{ \
				ULONG_PTR	_RefCount; \
				_RefCount = (ULONG_PTR)((_NBL)->MiniportReserved[1]); \
				_RefCount --; \
				(ULONG_PTR)((_NBL)->MiniportReserved[1]) = _RefCount; \
			}

//
// Macros to manipulate NBL's link list, that is, NetBufferListHeader.NetBufferListData.Next.
// For example, we use it to link NBL waiting to process.
//
#define RT_GET_NBL_LINK(_NBL)       (&(NET_BUFFER_LIST_NEXT_NBL(_NBL)))
#define RT_GET_NBL_FROM_QUEUE_LINK(_pEntry)  \
    (PNET_BUFFER_LIST)(CONTAINING_RECORD((_pEntry), NET_BUFFER_LIST, Next))


#define N6C_CANNOT_TX   N6SDIO_CANNOT_TX

static DOT11_DATA_RATE_MAPPING_ENTRY N6_Std_abg_DataRateMappingTable[] = 
{
	{2, 0, 2},
	{4, 0, 4},
	{11, 0, 11},
	{12, 0, 12},
	{18, 0, 18},
	{22, 0, 22},
	{24, 0, 24},
	{36, 0, 36},
	{48, 0, 48},
	{72, 0, 72},
	{96, 0, 96},
	{108, 0, 108}
};

//------------------------------------------------------------------------------
// Definition and declaration about MIC.
//------------------------------------------------------------------------------

typedef struct _AUTHENTICATION_EVENT
{
	NDIS_802_11_STATUS_TYPE				StatusType;
	NDIS_802_11_AUTHENTICATION_REQUEST	Request;
}AUTHENTICATION_EVENT, *PAUTHENTICATION_EVENT;

// mask for authentication/integrity fields
#define NDIS_802_11_AUTH_REQUEST_AUTH_FIELDS        	0x0f
#define NDIS_802_11_AUTH_REQUEST_REAUTH				0x01
#define NDIS_802_11_AUTH_REQUEST_KEYUPDATE			0x02
#define NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR		0x06
#define NDIS_802_11_AUTH_REQUEST_GROUP_ERROR		0x0E

// MIC check time, 60 seconds.
#define MIC_CHECK_TIME	60000000

//Flags for PMKID Candidate list structure
#define NDIS_802_11_PMKID_CANDIDATE_PREAUTH_ENABLED	0x01

typedef	enum	_PRE_AUTH_INDICATION_REASON{
	PRE_AUTH_INDICATION_REASON_ASSOCIATION = 0x0,
	PRE_AUTH_INDICATION_REASON_PERIOTIC = 0x01
}PRE_AUTH_INDICATION_REASON;

//
//Add this micro to protect Init are ready, by Maddest 080731
//
#define N6_INIT_READY(__pAdapter)\
	(		(__pAdapter)->bHWInitReady &&\
			(__pAdapter)->bSWInitReady &&\
			(!(__pAdapter)->bInitializeInProgress))

//
// Fill N6 DOT11_EXTSTA_RECV_CONTEXT
// By Bruce, 2009-08-21.
//
#define	N6_FILL_DOT11_EXSTA_RECV_CONTEXT(_pRecCtx, _pAdapter, _pRfd)	\
{																		\
	PlatformZeroMemory(_pRecCtx, sizeof(DOT11_EXTSTA_RECV_CONTEXT));	\
	N6_ASSIGN_OBJECT_HEADER(											\
			_pRecCtx->Header,											\
			NDIS_OBJECT_TYPE_DEFAULT,									\
			DOT11_EXTSTA_RECV_CONTEXT_REVISION_1,						\
			sizeof(DOT11_EXTSTA_RECV_CONTEXT));							\
	_pRecCtx->uReceiveFlags = DOT11_RECV_FLAG_RAW_PACKET_TIMESTAMP;										\
	_pRecCtx->uPhyId = N6CQuery_DOT11_OPERATING_PHYID(_pAdapter);		\
	_pRecCtx->uChCenterFrequency = 										\
			MgntGetChannelFrequency(RT_GetChannelNumber(_pAdapter));	\
	_pRecCtx->usNumberOfMPDUsReceived = _pRfd->nTotalFrag;				\
	_pRecCtx->lRSSI = _pAdapter->RxStats.SignalStrength;				\
	_pRecCtx->ucDataRate = _pRfd->Status.DataRate;						\
	_pRecCtx->uSizeMediaSpecificInfo = 0;								\
	_pRecCtx->pvMediaSpecificInfo = NULL;								\
	_pRecCtx->ullTimestamp = ((u8Byte)_pRfd->Status.TimeStampHigh << 32) + _pRfd->Status.TimeStampLow;		\
}
typedef enum _N6_INDICATE_STATE
{
	N6_STATE_INITIAL = 			0x0001,
	N6_STATE_CONNECT_START =	0x0002,
	N6_STATE_ASOC_START = 		0x0004,
	N6_STATE_ASOC_COMPLETE =	0x0008,
	N6_STATE_CONNECT_COMPLETE = 0x0010,
	N6_STATE_DISASOC =			0x0020,
	N6_STATE_ROAM_START = 		0x0040,
	N6_STATE_ROAM_COMPLETE =	0x0080
} N6_INDICATE_STATE, *PN6_INDICATE_STATE;

typedef enum _N6_INDICATE_STATE_FLAG
{
	SM_fNone,
	SM_fConnected,
	SM_fRoaming
} N6_INDICATE_STATE_FLAG, *PN6_INDICATE_STATE_FLAG;

typedef enum _N6_INDICATE_OPMODE
{
	SM_mNone,
	SM_mInfra,
	SM_mAdhoc
} N6_INDICATE_OPMODE, *PN6_INDICATE_OPMODE;

typedef struct _N6_INDICATE_STATE_MACHINE
{
	N6_INDICATE_OPMODE				CurrOpMode;
	N6_INDICATE_STATE_FLAG			Flag;
	N6_INDICATE_STATE	CurrentState;
	u4Byte				NextState;
} N6_INDICATE_STATE_MACHINE, *PN6_INDICATE_STATE_MACHINE;

//
//
// 070604, rcnjko:
// 1. The NDIS6 miniport driver state management is based on the document:
// ws-07-0043-v00-all-wlan-driver-ndis6-miniport-driver-state-csscg-070604.vsd
//
// 2. Driver is initialized at halted state. 
// The state <= MINIPORT_PAUSING cannot Send/Recv packets will be drop, 
// see N6XXX_CANNOT_TX() and N6XXX_CANNOT_RX().
//
// 3. On NDIS6, shutdown and sleep requests happen 
// only if we are in paused state. 
//
// 4. We can add some state between running and pausing, 
// for example, Tx/Rx reseting, and keep packets in these state.
//

//
// Definition of NDIS6 miniport driver state.
//
typedef enum _N6C_MP_DRIVER_STATE
{
    MINIPORT_HALTED = 0,
    MINIPORT_PAUSED,
    MINIPORT_PAUSING,
    MINIPORT_RUNNING
} N6C_MP_DRIVER_STATE, *PN6C_MP_DRIVER_STATE;

//
// Routine to NDIS6 miniport driver state.
//
#define N6C_GET_MP_DRIVER_STATE(__pAdapter)	((__pAdapter)->pNdisCommon->MpDriverState)
#define N6C_SET_MP_DRIVER_STATE(__pAdapter, __eState)  ((__pAdapter)->pNdisCommon->MpDriverState = (__eState))

#define N6C_GET_MP_DRIVER_STATE_BACKUP(__pAdapter)	((__pAdapter)->pNdisCommon->MpDriverStateBackup)
#define N6C_SET_MP_DRIVER_STATE_BACKUP(__pAdapter, __eState)  ((__pAdapter)->pNdisCommon->MpDriverStateBackup = (__eState))


#define N6_GET_RX_NBL_POOL(_pAdapter)	\
	(GET_RT_SDIO_DEVICE(GetDefaultAdapter(_pAdapter))->RxNetBufferListPool) 

//
// Supported Auth and Cipher pairs. 2006.10.13, by shien chang.
//
typedef struct _NIC_SUPPORTED_AUTH_CIPHER_PAIRS
{
	PDOT11_AUTH_CIPHER_PAIR_LIST	pInfraUcastAuthCipherList;
	PDOT11_AUTH_CIPHER_PAIR_LIST	pInfraMcastAuthCipherList;
	PDOT11_AUTH_CIPHER_PAIR_LIST	pAdhocUcastAuthCipherList;
	PDOT11_AUTH_CIPHER_PAIR_LIST	pAdhocMcastAuthCipherList;
	PDOT11_AUTH_CIPHER_PAIR_LIST	pInfraSupportedMcastMgmtAlgoList;
} NIC_SUPPORTED_AUTH_CIPHER_PAIRS, *PNIC_SUPPORTED_AUTH_CIPHER_PAIRS;


//
// PHY MIB definition. 2006.10.09, by shien chang.
//
typedef struct _NIC_PHY_MIB
{
	DOT11_PHY_TYPE			PhyType;
	ULONG					PhyID;
	DOT11_RATE_SET			OperationalRateSet;
	DOT11_RATE_SET			BasicRateSet;
	DOT11_RATE_SET			ActiveRateSet;
	UCHAR					Channel;
	DOT11_SUPPORTED_DATA_RATES_VALUE_V2	SupportedDataRatesValue;
	DOT11_PHY_FRAME_STATISTICS		Statistics;
} NIC_PHY_MIB, *PNIC_PHY_MIB;


//
// Current IBSS parameters. By Bruce, 2007-08-13. 
//
typedef struct _RTL_DOT11_IBSS_PARAMS
{
	BOOLEAN					bDot11IbssJoinOnly;
	PVOID					AdditionalIEData;
	ULONG					AdditionalIESize;
} RTL_DOT11_IBSS_PARAMS, *PRTL_DOT11_IBSS_PARAMS;

typedef enum _PLATFORM_NDIS_VERSION{
	PLATFORM_NDIS_VERSION_50 = 0x0,
	PLATFORM_NDIS_VERSION_51 = 0x1,	
	PLATFORM_NDIS_VERSION_60 = 0x2,	
	PLATFORM_NDIS_VERSION_62 = 0x3,	
}PLATFORM_NDIS_VERSION;

//
// Determine the current pended OID type.
//
typedef enum _RT_PENDED_OID_TYPE
{
	RT_PENDED_OID_DONT_CARE,
	RT_PENDED_OID_RF_STATE,
	RT_PENDED_OID_PNP,
	RT_PENDED_OID_CREATE_DELETE_MAC,
} RT_PENDED_OID_TYPE, *PRT_PENDED_OID_TYPE;

typedef enum _RT_D0_FILTER_STATE
{
	RT_D0_FILTER_NONE 		= 0,   // NO fiter entry 
	RT_D0_FILTER_INIT 		 	= 1,   // NO buffer packet !! 
	RT_D0_FILTER_BUFFING		= 2,   // buffer packet and start timer 
	RT_D0_FILTER_FLUSHING 	= 3,  // Try to indic or drop Packet 
	RT_D0_FILTER_MAX
} RT_D0_FILTER_STATE, *PRT_D0_FILTER_STATE;

typedef enum _RT_OID_HISTORY_STATE
{
	RT_OID_HISTORY_NONE			= 0,	//Init state
	RT_OID_HISTORY_STARTED		= 1,	
	RT_OID_HISTORY_PENDING		= 2,
	RT_OID_HISTORY_COMPLETE		= 3,
	RT_OID_HISTORY_UNKNOWN		= 4,
	RT_OID_HISTORY_M3				= 5,
	RT_OID_HISTORY_M4				= 6,
	RT_OID_HISTORY_MAX
}RT_OID_HISTORY_STATE, *PRT_OID_HISTORY_STATE;

typedef struct _RT_OID_HISTORY_ENTRY
{
	RT_LIST_ENTRY			List;
	NDIS_OID				Oid;
	u4Byte					TransactionId;				//For WDI property to check if the same
	RT_OID_HISTORY_STATE	CurrentState;					//Started, pending, Complete
	u8Byte					CurrentTime;					//Record time when receive, pend or complete OID
	NDIS_REQUEST_TYPE		RequestType;
	PVOID					InformationBuffer;
	ULONG					InformationBufferLength;
}RT_OID_HISTORY_ENTRY, *PRT_OID_HISTORY_ENTRY;

#define OID_HISTORY_MAX_NUM	30

//20150702 Sinda
//We reserve MP_DEFAULT_NUMBER_OF_PORT number of peer for frame indication before connection establishment
//So there are (255 - MP_DEFAULT_NUMBER_OF_PORT) number of peer for connection.
#define MAX_PEER_NUM			255

typedef enum _RT_RX_STATE
{
	RT_RX_NORMAL			= 0,	//Init state
	RT_RX_NOTIFYING		= 1,
	RT_RX_STOP				= 2,
	RT_RX_PAUSE			= 3,
	RT_RX_PREPARE_STOP	= 4,
	RT_RX_MAX
}RT_RX_STATE, *PRT_RX_STATE;

//20151013 Sean moidy "ViaU3" to "UsbSafetySwitch"as a bit-oriented registry for USB workaround switch
typedef enum _RT_UsbSafetySwitch_Bits
{
	//20130620 MH add for VIA USB30 inpipe reset for rx faildue to host stop to send bulk in irp  workaround~!..
	RT_UsbSafetySwitch_ViaU3				= BIT0,	//set to 1=support VIA USB3.0 workaround. 
	RT_UsbSafetySwitch_IrpPendingCount	= BIT1,	//set to 1=not using Rx IrpPendingCount workaround
	RT_UsbSafetySwitch_8812_schmitt		= BIT2,	//set to 1=not using 8812AU schmitt trigger
	RT_UsbSafetySwitch_8814_schmitt		= BIT3	//set to 1=not using 8814AU schmitt trigger
}RT_UsbSafetySwitch_Bits;

typedef enum _RT_TIMER_RESOURCE_ACTION{
	RT_TIMER_RESOURCE_ACTION_CANCEL= 0x00,
	RT_TIMER_RESOURCE_ACTION_SUSPEND= 0x01,
	RT_TIMER_RESOURCE_ACTION_RESUME= 0x02,
}RT_TIMER_RESOURCE_ACTION, *PRT_TIMER_RESOURCE_ACTION;

typedef struct _RT_NDIS6_COMMON{

	// NDIS miniport adapter handle.
	NDIS_HANDLE		hNdisAdapter;		

	UINT				NdisVersion;

	// Tx NBL wait queue.
	RT_SINGLE_LIST_HEAD	TxNBLWaitQueue;

	// The Current NDIS PnP State of the miniport.
	N6C_MP_DRIVER_STATE		MpDriverState;
	N6C_MP_DRIVER_STATE		MpDriverStateBackup;

	NDIS_SPIN_LOCK		ThreadLock; // For timer synchronization.
	NDIS_EVENT			AllThreadCompletedEvent;
	s4Byte				OutStandingThreadCnt;// This counter indicates how many timers are scheduled or fired.

	//----------------------------------------------------------------------------
	// Variables for registry values.
	//
	//----------------------------------------------------------------------------
	
	// "NetworkAddress"
	u1Byte				NetworkAddress[6];

	// "SSID"
	OCTET_STRING		RegSSID;
	u1Byte				RegSSIDBuf[33];

	// "Bssid"
	u1Byte				RegBssidBuf[14];
	u1Byte				RegBssid[6];

	OCTET_STRING		RegLocale;
	u1Byte				RegLocaleBuf[4];

	// "CornerChange"
	u1Byte				bRegConcurrent;

	// "ChannelPlan"
	u2Byte				RegChannelPlan;

	u1Byte				RegRFType;
	
	// "WirelessMode"
	int					RegWirelessMode; // Default Wireless Mode for initialization.	

	// "HTMode"
	u1Byte				RegHTMode;

	// "Channel"
	int					RegChannel; //  2.4G Default channel.	
	int					RegChannel5G; //  5G Default channel.	

 	// "AntennaDiversityType" , 20120313 added by Kordan.
	u1Byte				AntDivType;

	 // "Crystal Frequency" , 20120606 added by Kordan.
	u1Byte				XtalFrequency;
	u1Byte				XtalSource;
	// "Amplifier Type" , 20120827 added by Kordan.
	u1Byte				AmplifierType_2G;
	u1Byte				AmplifierType_5G;
	
	s1Byte 				TxBBSwing_2G;
	s1Byte 				TxBBSwing_5G;
	BOOLEAN				bTurnOffEfuseMask;
	u1Byte				RFE_Type;
	BOOLEAN				bTFBGA;
	BOOLEAN 			bEfusePriorityAuto;	
	
	// For Netgear UI, 2005.01.13, by rcnjko.
	// "ForcedDataRate"
	u2Byte				RegForcedDataRate; // Force Data Rate. 0: Auto, 0x02: 1M ~ 0x6C: 54M.
	u2Byte				RegForcedBstDataRate; // Force Bst Data Rate. 0: Auto, 0x02: 1M ~ 0x6C: 54M. , 0x80: MCS0 ~ 0x8f: MCS15

	// "WirelessMode4ScanList" 
	int					RegWirelessMode4ScanList; // Wireless Mode used to filter scan list, see enum _WIRELESS_MODE for its definition. 
	//

	// For 818x UI and WPA Verify, 2004.11.30, by rcnjko.
	int					RegWepEncStatus;
	int					RegEncAlgorithm;
	int					RegAuthentAlg;
	int					WepEncStatusBeforeWpaVerify;
	int					EncAlgorithmBeforeWpaVerify;
	int					AuthentAlgBeforeWpaVerify;
	UCHAR				RegDefaultKeyBuf[4][61];
	OCTET_STRING		RegDefaultKey[4];
	UCHAR				RegDefaultKeyWBuf[4][61];
	OCTET_STRING		RegDefaultKeyW[4];
	int					RegDefaultKeyId;
	int					RegNetworkType;
	//

	// "EnablePreAuth" by Annie.
	BOOLEAN				RegEnablePreAuthentication;

	// For power save mode, 2005.02.15, by rcnjko.
	// "PowerSaveMode"
	u1Byte				RegPowerSaveMode; // Default power-saving mode to confige.

	// For WMM Power Save Mode : 
	// ACs are trigger/delivery enabled or legacy power save enabled. 2006-06-13 Isaiah
	u1Byte					StaUapsd;
	u1Byte					MaxSPLength;
	
	// For debugging IBSS beacon behavior. 2005.04.18, by rcnjko.
	int					RegEnableSwBeacon;

	// For AP mode. 2005.05.30, by rcnjko.
	int					RegActingAsAp;
	int					RegBeaconPeriod;
	int					RegDtimPeriod;
	int					RegPreambleMode;

	// For Intel class mode, 60 students want to send data in the same time.
	BOOLEAN				bRegClassMode;


	// For RF power state, 2005.08.23, by rcnjko.
	int					RegRfOff;

	// For debug monitor, 20140509 by Cosa
	int					RegDbgMonitor;

	// mix mode protection, 20141113 by Gibson
	int                 RegProtectionmode;
	
	// For NdisReadNetworkAddress().
	// <RJ_8185> We shall resotre the network address at mphalt of 8185 for HCT12.0 test.
	u1Byte				CurrentAddress[6];//dot11MacAddress
	BOOLEAN				bOverrideAddress;


	// "QoS", added by Annie, 2005-11-09.
	BOOLEAN				bRegSupportQoS;

	// Support CCX or not
	u1Byte				RegCcx;
	// CCX Radio Measurement, 2006.05.15, by rcnjko.
	int					RegCcxRm;	
	u2Byte				RegCcxOffLineDurUpLimit;

	u1Byte				RegCcxCatasRoamLimit;
	
	// Update CCX Cell Tx Power
	BOOLEAN				RegCcxUpdateTxCellPwr;
	// CCX CAC Enable
	BOOLEAN				RegCcxCACEnable;
	
	//For HW init 
	BOOLEAN				bRegHwParaFile;

	// For TPC, added by shien chang, 2006.06.29
	int					RegTxPowerLevel;
		
	// Antenna Diversity Mechanism, by Isaiah, 2007-01-03
	BOOLEAN				bRegSwAntennaDiversityMechanism;	
	
	// 070207, rcnjko: 802.11d.
	BOOLEAN				bRegDot11dEnable;

	// "TurboMode". Added by Roger, 2006.12.07.
	int					RegTurboModeSelect;		
	
	// Auto select channel, 2005.12.27, by rcnjko.
	int					RegAutoSelChnl;
	int					RegChnlWeight;			// Added by Annie, 2006-07-26.

	// "LiveTime", AP mode for station live time (Added fot ASUS). Annie, 2006-02-16.
	int					RegLiveTime;

	// For OID_GEN_VENDOR_DESCRIPTION, 2006.03.10, by rcnjko.
	OCTET_STRING		RegDriverDesc;
	u1Byte				RegDriverDescBuf[256];

	// "WdsMode", 0: WDS disabled, 1: AP+WDS.
	u1Byte				RegWdsMode;

	// "CustomerID", 0: allow EEPROM to determine pMgntInfo->CustomerID, >0: overwrite pMgntInfo->CustomerID. 
	int					RegCustomerID;

	//"RegFragThreshold", 256 ~2432. 2007-03-05 by David
	u2Byte				RegFragThreshold;

	// DM InitialGain control. hpfan 2009.03.31
	BOOLEAN		bRegDMInitialGain;
 	
	BOOLEAN		bRegGpioRfSw;

	BOOLEAN		bRegHwWpsPbc;
	//----------------------------------------------------------------------------
	// Workaround for HCT 12.0. 
	//
	//----------------------------------------------------------------------------
	BOOLEAN								bRegHctTest;
	BOOLEAN								bRegWFDTest;

	BOOLEAN								bRegScanTimeCheck;
	
	BOOLEAN								bRegChaos;	
	NDIS_802_11_NETWORK_TYPE			BeSetNetworkTypeByNDTest; 
	BOOLEAN								KeepDisconnectFlag;
	
	BOOLEAN								bRegPnpCapabilities;
	
	BOOLEAN								bRegFixedMacAddr;

	//----------------------------------------------------------------------------
	// For OID.
	//
	//----------------------------------------------------------------------------
	u4Byte								MaxPktSize;
	u4Byte								ulLookAhead;
	ULONG								NdisRssiTrigger;
	NDIS_802_11_SSID					TmpNdisSsid;
	NDIS_802_11_NETWORK_TYPE_LIST		TmpNdisNetworkTypeList;
	NDIS_802_11_CONFIGURATION			TmpConfig80211;

	u4Byte								NdisPacketFilter;

	//
	// 061214, rcnjko: workitem for OID to change RF pwoer state.
	//
	RT_WORK_ITEM						SetRFPowerStateWorkItem;
	RT_WORK_ITEM						SetAdhocLinkStateWorkItem;
	enum _RT_RF_POWER_STATE				eRfPowerStateToSet; // RF state to change in SetRFPowerStateWorkItem.
	// Privacy exempt list, 2008.10.14, haich.
	u4Byte						PrivacyExemptionEntrieNum;
	DOT11_PRIVACY_EXEMPTION	PrivacyExemptionEntries[NATIVE_802_11_MAX_PRIVACY_EXEMPTION];
	//----------------------------------------------------------------------------
	// For case of Out of RFD, 2006.07.25, by shien chang
	u2Byte								LowRfdThreshold;

	// "PSPXlinkMode". 0: PSP XLink Mode disable, 1: enable.
	u1Byte								PSPXlinkMode;

	//----------------------------------------------------------------------------
	// For HW/SW encryption/decryption. 0: Auto.  1: HW enc/dec.  2: SW enc/dec.
	// 2006.09.29, by shien chang.
	u1Byte								RegSWTxEncryptFlag;
	u1Byte								RegSWRxDecryptFlag;

	//----------------------------------------------------------------------------
	// Wait event for returned packet. If some packet are indicated to upper layer and are
	// not returned, we should wait before MiniportHalt. 2007.01.19, by shien chang.
	NDIS_EVENT							AllPacketReturnedEvent;

	//----------------------------------------------------------------------------
	// This is used to set EDCA parameters of QAP
	u4Byte								RegApEDCAParamBE;
	u4Byte								RegApEDCAParamBK;
	u4Byte								RegApEDCAParamVI;
	u4Byte								RegApEDCAParamVO;

	//----------------------------------------------------------------------------
	// This is used to set EDCA parameters of QSTA
	u4Byte								RegStaEDCAParamBE;
	u4Byte								RegStaEDCAParamBK;
	u4Byte								RegStaEDCAParamVI;
	u4Byte								RegStaEDCAParamVO;

	//----------------------------------------------------------------------------
	// This is No Ack Setting
	u1Byte								RegAcNoAck;

	RT_THREAD			TxHandleThread;
#if (RTL8723_SDIO_IO_THREAD_ENABLE || RTL8723_USB_IO_THREAD_ENABLE) 
	RT_THREAD			IOHandleThread;
#endif

	// For disable Cck rate
	BOOLEAN							bRegDisableCck;

	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//1//1Attention Please!!!<11n or 8190 specific code should be put below this line>
	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!	

	//----------------------------------------------------------------------------
	// For 802.11n only function.
	//
	//----------------------------------------------------------------------------		
	// For HT Cck rate support, Joseph
	BOOLEAN							bRegHT_EnableCck;

	//For 40MHz channel support, Joseph
	u1Byte							RegBWSetting;
	BOOLEAN							bRegBW40MHzFor2G;
	BOOLEAN							bRegBW40MHzFor5G;
	
	BOOLEAN							bReg11nAdhoc;
	BOOLEAN							bRegIOTBcm256QAM;
	u1Byte							bRegVht24g;
	u1Byte							bRegAdhocUseHWSec;

	// For AMSDU aggregation, Joseph
	int 								RegAMSDU;
	int								RegAMSDU_MaxSize;	
	
	int 								RegHWAMSDU;

	// For AMPDU aggregation <2006.08.10 Emily>
	int								bRegAMPDUEnable;
	int								RegAMPDU_Factor;
	int								RegMPDU_Density;
	
	// MIMO Power Save
	int								RegMimoPs;
	
	// For Rx Reorder Control
	u1Byte				RegRxReorder;
	u1Byte				RegRxReorder_WinSize;
	u1Byte				RegRxReorder_PendTime;

	//----------------------------------------------------------------------------
	// For native 802.11, 2006.10.09, by shien chang.
	//
	//----------------------------------------------------------------------------
	DOT11_OPERATION_MODE_CAPABILITY	dot11OperationModeCapability;
	DOT11_CURRENT_OPERATION_MODE		dot11CurrentOperationMode;
	PDOT11_SUPPORTED_PHY_TYPES			pDot11SupportedPhyTypes;
	ULONG								dot11OperatingPhyId;
	ULONG								dot11SelectedPhyId;

	PNIC_PHY_MIB						pDot11PhyMIBs;
	PNIC_PHY_MIB						pDot11OperatingPhyMIB;
	PNIC_PHY_MIB						pDot11SelectedPhyMIB;

	DOT11_SUPPORTED_POWER_LEVELS		dot11SupportedPowerLevels;
	ULONG								dot11CurrentTxPowerLevel;
	
	 // PHY list
    	ULONG                   					staDesiredPhyList[NATIVE_802_11_MAX_NUM_PHY_TYPES];
    	ULONG                   					staDesiredPhyCount;
    	ULONG                  					ActivePhyId; 
	
	
	// 802.11d MIB.
	BOOLEAN								dot11MultiDomainCapabilityImplemented;

	DOT11_OPTIONAL_CAPABILITY			RegOptionalCapability;
	DOT11_OPTIONAL_CAPABILITY			dot11CurrOptionalCapability;

	// Current power saving level.
	ULONG								PowerSaveLevel;	// Added by Annie, 2006-10-16.

	BOOLEAN								bDot11MediaStreamingEnabled;
	DOT11_SSID_LIST						dot11DesiredSSIDList;
	DOT11_SSID_LIST						dot11DesiredSSIDListCopy;
	u1Byte								dot11DesiredSSIDListIndex;

	//
	// From OID_DOT11_IBSS_PARAMS. This member is replaced by IbssParams.
	// BOOLEAN								bDot11IbssJoinOnly;
	// Current IBSS Parameters. By Bruce, 2007-08-13.
	//
	RTL_DOT11_IBSS_PARAMS				dot11IbssParams;

	// Auto Configuration, 2006.11.01, by shien chang.
	ULONG								dot11AutoConfigEnabled;

	DOT11_MAC_ADDRESS					dot11DesiredBSSIDList[NATIVE_802_11_MAX_DESIRED_BSSID];
	ULONG								NumDot11DesiredBSSIDList;

	ULONG								dot11UnreachableDetectionThreshold;
	
	// Use group key, 2006.11.06, by shien chang.
	BOOLEAN								dot11UnicastUseGroupEnabled;

	BOOLEAN								dot11HiddenNetworkEnabled;

	// Status indication, 2006.11.15, by shien chang.
	BOOLEAN								bToIndicateScanComplete;
	
	// New status indication state machine, 2006.11.29, by shien chang.
	N6_INDICATE_STATE_MACHINE			IndicationEngine;

	//for pend request 20061206 by David
    	/** If the miniport pends a request, the copy is kept here */
   	PNDIS_OID_REQUEST                   		PendedRequest;	
	

	// For Restore N6  unicase , mutilcase encrypt Alg ,CCW
	DOT11_CIPHER_ALGORITHM          					RegGroupALg;
	DOT11_CIPHER_ALGORITHM							RegPairwiseAlg;
       DOT11_AUTH_ALGORITHM             					RegAuthalg;         
	u1Byte											RegRSNADefaultkeybuf[61];
	OCTET_STRING									RegRSNADefaultkey;
	u4Byte 											RegRSNAKeyID;

	// To avoid explicit scan operation during wakeup auto link stage.
	BOOLEAN											bWakeupAutoLinkInProgressing;
	
	// Use this flag to check whether indicate roaming start while system resume, by Roger. 2007.11.07.
	BOOLEAN											bDissociateBeforeSleep;

	// To present that we do not disconnect to AP before PNP sleep. 2013.10.30, by tynli.
	BOOLEAN								bPnpKeepConnectToAP;

	//----------------------------------------------------------------------------
	// Power Save Related
	//----------------------------------------------------------------------------
	// For inactive power save mode. 2007.07.16, by shien chang.
	u1Byte								RegInactivePsMode;
	// For leisure power save mode in linked state.
	u1Byte								RegLeisurePsMode;
	BOOLEAN								RegAdvancedLPs;
	// LPS Max listen interval
	u1Byte								RegLPSMaxIntvl;
	//For Keep Alive mechanism.
	u1Byte								RegKeepAliveLevel;


	//----------------------------------------------------------------------------
	//	RTL8192C USB Tx/Rx Aggregation
	//----------------------------------------------------------------------------
#if TX_AGGREGATION
	BOOLEAN 			UsbTxAggMode;
	u1Byte				UsbTxAggTotalNum;
	u1Byte				UsbTxAggDescNum;
	//u1Byte				UsbTxAggPerBulkNum;
#endif
#if RX_AGGREGATION
	u4Byte				RxAggMode;
	u1Byte				RxAggBlockCount;			// USB Block count. Block size is 512-byte in hight speed and 64-byte in full speed
	u1Byte				RxAggBlockTimeout;
	u1Byte				RxAggPageCount;			// 8192C DMA page count
	u1Byte				RxAggPageTimeout;
#endif

	// WiFi Config, by Bruce, 2007-12-07.
	BOOLEAN				bRegWiFi;

	//Add by Jacken to save PCI BAR0 , BAR1 , Interrupt line register , command register	2008/02/14
	ULONG 				PCI_BAR0;
	ULONG 				PCI_BAR1;
	UCHAR 				PCI_InterruptLine;
	USHORT				PCI_CommandRegister;		

	u1Byte				RegCckPwEnl;

	// Accept ADDBA Request
	int 					bRegAcceptAddbaReq;
	
	// CCX Vista Context
	Cisco_CCX_CONTEXT	Cisco_CCX_CONTEXT;

	u1Byte				RegROAMSensitiveLevel;
	u1Byte				RegRoamHysteresis;

	u1Byte				RegRoamingLimit;

	//For Fw control LPS mode
	u1Byte				bRegFwCtrlLPS;
	BOOLEAN				bRegLowPowerEnable;
	BOOLEAN				bRegLPSDeepSleepEnable;

	// Indicate event immediately when reciecing deauth/disassoc. By Bruce, 2009-05-19.
	BOOLEAN				RegIndicateByDeauth;

	u1Byte				RegShowRate;

	RT_WORK_ITEM 		InitializeAdapterWorkItem;
	RT_TIMER			InitializeAdapterTimer;
	RT_TIMER			N6CSendSingleNetBufferListTimer;
	RT_THREAD			InitializeAdapterThread;

	RT_THREAD			InterruptHandlingThread;

	// For S3/S4/FS(S5) connect timer.
	RT_TIMER			PNPReConnentTimer;
	u1Byte				PNPconnentCout;

	//For AzWave TKIP in N mode
	BOOLEAN				bRegTKIPinNmode;
	BOOLEAN				bRegWEPinNmode;

	BOOLEAN				bRegClevoDriver;

	BOOLEAN				bRegTDLSEnable;


	// Turn on/off Win7 Virtaul Wifi support.
	BOOLEAN				bRegVWifiSupport;
	BOOLEAN				bRegDMDPMAC0VWifiSupport;	

	u1Byte				RegbtHsMode;
	u1Byte				bRegDefaultAntenna;
	u1Byte				RegAntDiv;
	u1Byte				RegAntDetection;

        u1Byte				bRegUseRxInterruptWorkItem;	
		
	// For WoWLAN mode. by tynli.
	u1Byte				RegAPOffloadEnable;
	u1Byte				RegWoWLANLPSLevel;
	u1Byte				bRegARPOffloadEnable;
	u1Byte				bRegGTKOffloadEnable;
	u1Byte				RegProtocolOffloadDecision;
	u1Byte				RegWoWLANS5Support;
	u1Byte				RegD2ListenIntvl;
	BOOLEAN				bRegFakeWoWLAN;
	u1Byte				bRegNSOffloadEnable;
	u1Byte				RegNLOEnable;
	u1Byte				RegPnpKeepLink;
	
	BOOLEAN				bRegWakeOnMagicPacket;
	BOOLEAN				bRegWakeOnPattern;
	BOOLEAN				bRegWakeOnDisconnect;

	// Connected standby feature
	BOOLEAN				bRegPacketCoalescing;
	
	u1Byte				RegFSSDetection;
	
	// BIT0 - 20MHz
	// BIT1 - 40MHz
	// BIT2 - 80MHz
	// BIT3 - 160MHz
	u1Byte				RegShortGISupport;
	
	u1Byte				RegAPTimExtend;
	u1Byte				RegUSBResetTxHang;

	BOOLEAN				bRegVelocity;

	// 2010/05/18 MH For GPIO detect timer delay setting.	
	u1Byte				bRegTimerGPIO;
	u2Byte				bRegGPIODelay;
	u2Byte				bRegGPIOBack;

	// 2015/08/04 Gary Add for customized Led Strategy
	u1Byte				bRegLedStrategy;

	// 2010/08/25 MH Add to support pdn mode switch.
	u1Byte				bRegPDNMode;

	// 2010/09/01 MH According to PM's request, we support dongle selective suspend mode switch.	
	u1Byte				bRegDongleSS;

	// 2010/09/13 MH According to PM's request, we support different SS power seave level.	
	u1Byte				bRegSSPwrLvl;

	// 2011/02/16 MH Add for SS HW radio detect workaround temporarily.
	u1Byte				bRegSSWakeCnt;

	// 2010/12/17 MH Add for RX aggregation mode switch according to TX/RX traffic.	
	u1Byte				bRegAggDMEnable;

	// 2010/12/31 MH Add for UPHY dynamic chaneg.	
	u1Byte				bRegUPDMEnable;

	// 2011/07/08 MH Add for different link speed display.
	u1Byte				bRegLinkSpeedLevel;	

	u1Byte				RegRSSI2GridMode;

	// 2011/07/14 MH Add for rx short cut.	
	u1Byte				bRegRxSC;

	// 2011/07/15 Sinda Add for tx short cut.	
	u1Byte				bRegTxSC;

	// 2011/09/15 MH Add registry for switching packet compete method.
	u1Byte				RegTxMode;

	// 2012/09/14 MH Add for EDCA turbo mode switch threshold.
	// 2012/09/14 MH Add for 88E rate adaptive mode siwtch.
	u2Byte				RegEdcaThresh;
	u2Byte				RegRALvl;

	// 2012/10/26 MH Add for 8812/8821 AC series dynamic switch.
	u1Byte				RegAcUsbDmaTime;
	u1Byte				RegAcUsbDmaSize;
	u1Byte				RegAcUsbDmaTime2;
	u1Byte				RegAcUsbDmaSize2;

	// 2012/10/31 MH Add for power limit table constraint.
	u4Byte				RegTxPwrLimit;

	// 2012/11/07 Awk add PowerBase for customers to define their power base.
	u1Byte				RegPowerBase;

	// 2012/11/23 Awk add to enable power limit
	u1Byte				RegEnableTxPowerLimit;
	u1Byte				RegEnableTxPowerByRate;
	BOOLEAN				RegTxPowerTraining;

	// 2015/02/11 VincentL Add for primary/secondary power limit table selection (used as initial one)
	u1Byte				RegTxPowerLimitTableSel;
	u1Byte				RegTxPwrLmtDynamicLoading;

	// 2015/04/27 VincentL Add for Toggle Support for TxPwrTable Dump Feature
	u1Byte				RegSupportTxPwrTableDump;

	// 2015/02/25 VincentL Add for UEFI method
	u1Byte				RegLoadSystemSKUfromUEFI;
	u1Byte				RegUEFIProfile;

	// 2015/07/07 VincentL Add for SMBIOS method
	u1Byte				RegLoadSystemSKUfromSMBIOS;
	u1Byte				RegLoadProcessorIDfromSMBIOS;

	OCTET_STRING		RegPwrByRateFile;
	u1Byte				RegPwrByRateFileBuf[33];
	OCTET_STRING		RegPwrLimitFile;
	u1Byte				RegPwrLimitFileBuf[33];
	OCTET_STRING		RegSecondaryPwrLimitFile;
	u1Byte				RegSecondaryPwrLimitFileBuf[33];

	OCTET_STRING		RegChannelPlan2G;
	u1Byte				RegChannelPlan2GBuf[40];
	OCTET_STRING		RegChannelPlan5G;
	u1Byte				RegChannelPlan5GBuf[100];
	
	u1Byte				RegDecryptCustomFile;

	// 2013/04/16 VincentLan Add to switch Spur Calibration Method
	u1Byte				RegSpurCalMethod;
	// 2013/01/23 VincentLan Add to enable IQK Firmware Offload
	u1Byte				RegIQKFWOffload;
	// 2013/11/23 VincentLan add for for KFree Feature Requested by RF David
	u1Byte				RegRfKFreeEnable;


	u1Byte				RegTxDutyEnable;	
	// 2011/11/15 MH Add for user can select different region and map to dedicated power gain offset table.
	u1Byte				RegPwrTblSel;

	u1Byte				RegTxPwrLevel;

	// 2011/11/15 MH Add for user can select different tx power by rate switch by default value and registry value.
	u1Byte				RegPwrByRate;	
	u4Byte				RegPwrRaTbl1;	
	u4Byte				RegPwrRaTbl2;	
	u4Byte				RegPwrRaTbl3;	
	u4Byte				RegPwrRaTbl4;	
	u4Byte				RegPwrRaTbl5;	
	u4Byte				RegPwrRaTbl6;	
	u4Byte				RegPwrRaTbl7;	
	u4Byte				RegPwrRaTbl8;	
	u4Byte				RegPwrRaTbl9;	
	u4Byte				RegPwrRaTbl10;	
	u4Byte				RegPwrRaTbl11;	
	u4Byte				RegPwrRaTbl12;	
	u4Byte				RegPwrRaTbl13;	
	u4Byte				RegPwrRaTbl14;	
	u4Byte				RegPwrRaTbl15;	
	u4Byte				RegPwrRaTbl16;	

	BOOLEAN				RegSendPacketByTimer;

	// For handling NBL wait queue
	RT_WORK_ITEM 		ReleaseDataFrameQueuedWorkItem;
	BOOLEAN				bReleaseNblWaitQueueInProgress;

	u1Byte				RegEarlymodeEnable;
	BOOLEAN				RegAutoAMPDUBurstModeEnable;
	u2Byte				RegAutoAMPDUBurstModeThreshold;
	
	u1Byte				RegReduceImr;
#if (NEW_EARLY_MODE_ENABLE == 1)	
	BOOLEAN				RegNewAMPDUBurstModeEnable;//New Earry Mode ctrl for 8814A,  add by ylb 20130418
#endif	
	u2Byte				RegTxHignTPThreshold;
	u2Byte				RegRxHignTPThreshold;


	u1Byte				RegTwoStaConcurrentMode;

	BOOLEAN				bRegDbgMode;
	u8Byte				RegDbgZone;
	
	PVOID			N6PowerSettingHandle;
	PVOID			N6WLANPowerModeHandle;


	// 2011/03/23 Offload Network
	ULONG				ScanPeriod;
	BOOLEAN				bNLOActiveScan;
	BOOLEAN				bFilterHiddenAP;
	u4Byte				Oidcounter;
	BOOLEAN				bToIndicateNLOScanComplete;
	
	GUID				NetCfgInstanceId;	// Read from the registry in initialization.

	u1Byte				RegPAMode;	

	//2011.09.23 LukeLee add for path diversity
	BOOLEAN				bPathDivEnable;

	BOOLEAN				bRegBtFwSupport;

	// Add TCP offload 
	ULONG				RegTCPOffloadMode;

	// D0 Rx colse 
	RT_TIMER			D0RxIndicatTimer;
	RT_LIST_ENTRY		D0FilterPktQueue;
	NDIS_SPIN_LOCK		D0FilterPktLock; // For timer synchronization.
	RT_D0_FILTER_STATE	D0FilterState;

	// 2011/12/08 hpfan Add for Tcp Reorder
	BOOLEAN				bTcpReorderEnable;
	
	u1Byte				bRegUseThreadHandleInterrupt;
	u1Byte				bRegDPCISRTest;

	//Sherry added for support txbuffer colease for 8192DU-VC
	BOOLEAN				bSupportTxPacketBufferColease;

	//Sherry added to allow user disable 1 band in advanced setting of device management
	u1Byte				RegWirelessBand;

	// 11n sigma support
	BOOLEAN				bReg40Intolerant;
	BOOLEAN				bRegAMPDUManual;
	u1Byte				RegTxSPStream;
	u1Byte				RegRxSPStream;
	u1Byte				RegBeamformCap;	// Beamforming Capability
	// Beamforming RF path number
	u1Byte				RegBeamformerCapRfNum;
	u1Byte				RegBeamformeeCapRfNum;

	u1Byte				RegLdpc;			// LDPC capability
	u1Byte				RegStbc;			// STBC capablitiy

	BOOLEAN				bRegBeamformAutoTest;

	u1Byte				RegUsbMode;
	u1Byte				RegForcedUsbMode;
	u1Byte				RegUsbCurMode;
	u2Byte				RegUsbMode3To2Cnt;
	u2Byte				RegUsbMode2To3Cnt;
	u2Byte				RegUsbMode2To3CntPrev;
	u2Byte				RegUsbMode2To3CntFail;
	u1Byte				RegUsbSwFast;
	u1Byte				RegUsbWps;
	u1Byte				RegUsbSp;
	u1Byte				RegUsbChnl;
	u1Byte				RegUsbSwitchSpeed;
	u1Byte				RegUsbSwitchThL;
	u1Byte				RegUsbSwitchThH;
	u1Byte				RegUsbSwitchThRssi;
	u1Byte				RegUsbSwBy;
	u1Byte				RegUsbRfSet;

	BOOLEAN				bRegEDCCASupport;

	// 2012/07/24 MH Add for win8 usb whql tst & WFD multi channel.
	u1Byte				RegUseDefaultCID;

	u1Byte				RegWfdTime;
	u1Byte				RegWfdChnl;

	u1Byte				RegNByteAccess; //page add for N Byte access BB reg when doing power by rate
	u1Byte				RegFWOffload;  //page add for fw offload for 8812au
	u1Byte				RegDownloadFW;
	BOOLEAN				RegFWQC;

	// 2012/11/28 MH Add for BB team AMPDU test.
	u1Byte				RegAMfactor;	
	u1Byte				RegVHTRSec;
	
	u2Byte				RegScanLarge;
	u2Byte				RegScanMiddle;
	u2Byte				RegScanNormal;
	u2Byte				RegScanActive;
	u2Byte				RegForcedScanPeriod;

	u1Byte				RegPreferBand;

	// Pre-transition for OID_PNP_SET_POWER OID wake up handling, added by Roger, 2012.11.28.
	u1Byte				RegPreTransPnP;

	//20140702 hana add for USB WFD test, wait seconds before skip scan when Go is connected
	u1Byte				RegWaitBeforeGoSkipScan;

	//20130109 Used for Invitation No/After to Pass, with skip scan when Go is connected
	u1Byte				RegGoSkipScanForWFDTest;	
	u1Byte				RegClientSkipScanForWFDTest;
	BOOLEAN				RegForceGoTxData20MBw;

	BOOLEAN				bRegHWRTSEnable;

	// "TxPwrPercentage"
	u1Byte				RegTxPwrPercentage; // Default Tx output power percentage.

	u1Byte				RegScanOffloadType;
	
	// 2013/02/05 MH Add for streamMode switch.
	u1Byte				RegStreamMode;
	// 2013/02/06 MH Add Transmit power control level for all customer in the future.
	u1Byte				RegTPCLvl;
	u1Byte				RegTPCLvlD;
	u1Byte				RegTPCLvl5g;
	u1Byte				RegTPCLvl5gD;

	u1Byte				RegEnableAdaptivity;
	u1Byte				RegL2HForAdaptivity;
	u1Byte				RegHLDiffForAdaptivity;
	u1Byte				RegEnableCarrierSense;
	u1Byte				RegNHMEnable;
	u1Byte				RegDmLinkAdaptivity;
	u1Byte				RegDCbackoff;
	u1Byte				RegAPNumTH;

	BOOLEAN				RegPacketDrop;

	u1Byte				RegEnableResetTxStuck;

	u1Byte				RegSifsThresh;

	u1Byte				RegFwload;	

	u1Byte				RegUsbSafetySwitch;

	u1Byte				RegRspPwr;
	u4Byte				RegPktIndicate;	

	BOOLEAN				RegClearAMDWakeUpStatus; // zhiyuan add for 88ee Lenovo AMD platform	

	BOOLEAN 			RegbCustomizedScanPeriod;	
	u2Byte				RegIntelCustomizedScanPeriod;
	u2Byte				RegAMDCustomizedScanPeriod;
	

	u2Byte		RegDisableBTCoexist;	

	BOOLEAN				RegEnableMA;	

	u1Byte				RegVhtWeakSecurity;
	//20130606 MH For passive scan control dynamic switch after meeting with RF/FAE team.
	// 0= by channel plan, 1=5g all passive scan / 2= 24g passive scan /3= 2/5g all passive scan
	u1Byte				RegPassiveScan;

	BOOLEAN 	RegIsAMDIOIC;
	u4Byte		RegWmmPage;
	u1Byte		RegDisableAC;	
	u4Byte		RegLedInterval;
	u1Byte		Reg8814auEfuse;
	u1Byte		RegValidRFPath;


	u1Byte		RegMultiChannelFcsMode;
	u1Byte		RegMultiChannelFcsNoA;
	u1Byte		RegMCCNoAStartTime;
	u1Byte		RegMCCStaBeaconTime;
	u1Byte		RegMCCQPktLevel;
	u1Byte		RegWFDOpChannel;
	u1Byte		RegWFDPeerOpChannel;
	u2Byte		RegConnectionConfigTimeIntv;
	u1Byte		RegRetryTimes;

	u4Byte		Regbcndelay;
	BOOLEAN		RegIntelPatchPNP;
       BOOLEAN       RegForcePCIeRate;
	u1Byte		RegRomDLFwEnable;
	u1Byte		RegFwCtrlPwrOff;
	u1Byte		RegCardDisableInLowClk;
	BOOLEAN		RegAutoChnlSel;
	u1Byte		RegDisableRtkSupportedP2P;
	BOOLEAN		RegFixBTTdma;
	u1Byte		RegFwIPSLevel;
	BOOLEAN		RegRxPsWorkAround;
	BOOLEAN		RegSdioDpcIsr;
	BOOLEAN 	RegNblRacingWA;

	RT_LIST_ENTRY		OidHistoryList;
	NDIS_SPIN_LOCK		OidHistorySpinLock;
	u1Byte				OidHistoryCount;
	
	// SDIO polling interrupt mode
	BOOLEAN 	RegSdioPollIntrHandler;
	u2Byte		RegSdioIntrPollingLimit;

	//-----------------------------------------------
	// WDI support capability
	//-----------------------------------------------
	u1Byte		WDISupport:1;
	u1Byte		Reserved:7;

	WDI_BAND_INFO_CONTAINER	BandInfo[WDI_802_11_MAX_NUM_BAND_TYPES];
	ULONG						BandInfoCount;
	WDI_PHY_INFO_CONTAINER		PhyInfo[WDI_802_11_MAX_NUM_PHY_TYPES];
	
	//Rx control state is used for port
	RT_RX_STATE					bRxControlState;
	//Rx data path state is used for adapter
	RT_RX_STATE					bRxDataPathState;

	u1Byte						DialogToken;
	PlatformSemaphore				RxNotifySemaphore;
	BOOLEAN		RegHangDetection;
	u1Byte		RegPreInitMem;
	 u1Byte				Reg88EIOTAction;
	u2Byte		RegTxSendAsap;

	// MAC Address Randomization 
	u1Byte		RegSupportMACRandom;

	// ECSA
	u1Byte		RegSupportECSA;

	// FT
	u1Byte		RegSupportFT;

	u1Byte		RegSuspendTimerInLowPwr;
} RT_NDIS6_COMMON, *PRT_NDIS6_COMMON;

typedef RT_NDIS6_COMMON RT_NDIS_COMMON, *PRT_NDIS_COMMON;


#define	GET_RT_DIRECT_IRP_INFO(__pNdisCommon)		NULL
#define	IS_RT_DIRECT_IRP_SUPPORT(__pNdisCommon)		FALSE


//
// Forward declaration for functions using RT_TX_LOCAL_BUFFER.
//
typedef struct _RT_TX_LOCAL_BUFFER RT_TX_LOCAL_BUFFER, *PRT_TX_LOCAL_BUFFER;

//
// NDIS6 Common Helper functions.
//
VOID
N6CopyPacket(
	IN	PNET_BUFFER				pNetBuffer,
	OUT	PRT_TX_LOCAL_BUFFER		pLocalBuffer,
	OUT pu2Byte					pPktLength);


VOID
N6IndicateCurrentPhyPowerState(
	IN	PADAPTER		pAdapter,
	IN	ULONG			phyId);

NDIS_STATUS
N6WriteRegRfPowerState(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bRfOn);

NDIS_STATUS
N6WriteRegDbgMonitor(
	IN	PADAPTER		Adapter,
	IN	u4Byte			DbgMonitor
	);

#if DRV_LOG_REGISTRY
NDIS_STATUS
N6WriteRegDriverState(
	IN	PADAPTER		Adapter,
	IN	ULONG			State);
#endif

NDIS_STATUS
N6WriteAdhocLinkState(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			AdhocLinkState);

NDIS_STATUS
N6WriteUsbCurrentMode(
	IN	PADAPTER		Adapter,
	IN	u1Byte			UsbMode);

NDIS_STATUS
N6WriteUsbMode3To2Counter(
	IN	PADAPTER		Adapter);

NDIS_STATUS
N6WriteUsbMode2To3Counter(
	IN	PADAPTER		Adapter);

NDIS_STATUS
N6WriteUsbModeSwitchFlag(
	IN	PADAPTER		Adapter,
	IN	u1Byte			UsbSwitch);

NDIS_STATUS
N6WriteUsbModeSwitchChnl(
	IN	PADAPTER		Adapter,
	IN	u2Byte			Channel);

NDIS_STATUS
N6WriteUsbModeSwitchBy(
	IN	PADAPTER		Adapter,
	IN	u1Byte			SwitchBy);

NDIS_STATUS
N6WriteUsbMode2To3CounterFail(
	IN	PADAPTER		Adapter);

VOID
N6IndicateScanComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status);

VOID
N6IndicateConnectionStart(
	IN	PADAPTER		Adapter);

VOID
N6IndicateConnectionComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status);

VOID
N6IndicateAssociationStart(
	IN	PADAPTER		Adapter);

VOID
N6IndicateAssociationComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status);

VOID
N6IndicateDisassociation(
	IN	PADAPTER		Adapter,
	IN	u2Byte			reason);

VOID
N6IndicateRoamingStart(
	IN	PADAPTER		Adapter);

VOID
N6IndicateRoamingComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status);

BOOLEAN
N6Dot11AddrIsBcast(
	IN	DOT11_MAC_ADDRESS	dot11Addr);

BOOLEAN
N6Dot11AddrIsfe(
	IN	DOT11_MAC_ADDRESS	dot11Addr);


VOID
InitializeAdapterThread(
	IN	PVOID	pContext
	);

BOOLEAN
InitializePendingOIDBeforeReady(
	IN	PADAPTER				Adapter,
	IN	PNDIS_OID_REQUEST	NdisRequest,
	OUT	PNDIS_STATUS			pRetVal
	);

VOID
LinkQualityReportCallback(
	IN	PADAPTER		Adapter);


VOID
N6CSendSingleNetBufferListTimerCallback(
		IN	PRT_TIMER	pTimer);

VOID
N6InitializeIndicateStateMachine(
	IN	PADAPTER		Adapter);

VOID
N6PushIndicateStateMachine(
	IN	PADAPTER		Adapter,
	IN	N6_INDICATE_STATE	StateToEnter,
	IN	u2Byte			StatusOrReasonCode);

DOT11_ASSOC_STATUS
N6TranslateToDot11AssocStatus(
	IN	N6_INDICATE_STATE	CurrentState,
	IN	u2Byte			StatusOrReasonCode);

int
GetSignedInteger(
	u4Byte	UsDW,
	u1Byte	BitNumber
);

VOID
SetRFPowerStateWorkItemCallback(
	IN PVOID			pContext
);

VOID
SetAdhocLinkStateWorkItemCallback(
	IN PVOID			pContext
);

VOID
SetTCPOffloadCapWorkItemCallback(
	IN PVOID			pContext
);


//
// Wait event for returned packet. 2007.01.19, by shien chang.
//
VOID
N6CStartWaitReturnPacketMechanism(
	IN	PADAPTER		Adapter
);

VOID
N6CWaitForReturnPacket(
	IN	PADAPTER		Adapter
);

NDIS_STATUS
N6AllocateNative80211MIBs(
	IN	PADAPTER		Adapter
	);

VOID
N6InitializeNative80211MIBs(
	IN 	PADAPTER 	Adapter
	);

VOID
N6IndicateLinkSpeed(
	PVOID			Adapter
	);

//
// NBL wait queue operation.
//
#define N6CIsNblWaitQueueEmpty(_Queue) RTIsSListEmpty( &(_Queue) )

#define N6CGetHeadNblWaitQueue(_Queue) RT_GET_NBL_FROM_QUEUE_LINK( RTGetHeadSList( &(_Queue)) )

VOID
N6CAddNblWaitQueue(
	IN PRT_SINGLE_LIST_HEAD		pNBLWaitQueue,
	IN PNET_BUFFER_LIST		pNetBufferLists,
	IN BOOLEAN				bToHead
	);

VOID
N6CConcatenateTwoList(
	IN PRT_SINGLE_LIST_ENTRY		pDest,
	IN PRT_SINGLE_LIST_ENTRY		pSource
	);

PNET_BUFFER_LIST
N6CRemoveNblWaitQueue(
	IN PRT_SINGLE_LIST_HEAD		pNBLWaitQueue,
	IN BOOLEAN					bFromHead
	);

VOID
N6CompleteNetBufferLists(
	IN	PADAPTER		Adapter,
	IN	PNET_BUFFER_LIST	pNetBufferLists,
	IN	NDIS_STATUS			ndisStatus,
	IN	BOOLEAN				bDispatchLevel
	);

VOID
N6WriteEventLog(
	IN PADAPTER Adapter,
	IN RT_EVENTLOG_TYPE type,
	IN	ULONG	Num
	);

VOID
N6CWriteEventLogEntry(
	IN  ADAPTER					*pAdapter,
	IN  NDIS_STATUS				ndisStatus,
	IN  ULONG					dataLen,
	IN  u1Byte					*data,
	IN  WCHAR					*wcFormat,
	IN  ...
	);

BOOLEAN
PlatformIsInDesiredBSSIDList(
	IN	PADAPTER		Adapter,
	IN	pu1Byte 		MacAddr
	);

BOOLEAN
PlatformGetDesiredSSIDList(
	IN	PADAPTER		Adapter
	);

BOOLEAN
N6ReceiveIndicateFilter(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
N6FulshD0CoalescingQueue(
	IN	PADAPTER				Adapter,
	IN	BOOLEAN					bAcqRxLock,
	IN	BOOLEAN					bIndic
	);

BOOLEAN
N6ReceiveCoalescingFilter(
	IN	PADAPTER				Adapter,
	IN	PRT_RFD					pRfd
	);

VOID
N6Fill80211PhyAttributes(
	IN	PADAPTER		Adapter,
	IN	PNDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES	pDot11Attributes
	);

VOID
N6Fill80211ExtStaAttributes(
	IN	PADAPTER		Adapter,
	IN	PNDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES	pDot11Attributes,
	IN	PNIC_SUPPORTED_AUTH_CIPHER_PAIRS	pSupportedAuthCipherAlgs
	);

NDIS_STATUS
N6CSetGeneralAttributes(
	IN	PADAPTER		Adapter
	);

NDIS_STATUS
N6Set80211Attributes(
	IN	PADAPTER		Adapter
	);

u4Byte
N6CGetTCPCheckFormNetBuffLisInfo(
	IN	PADAPTER		Adapter,
	IN  PNET_BUFFER_LIST	pNetBufferList
);

VOID
N6CSetTCPCheckToNetBuffLisInfo(
	IN	PADAPTER			pAdapter,
	IN	PRT_RFD				pRfd,
	IN  	PNET_BUFFER_LIST	pNetBufferList
);

VOID
N6FreeAdapter(
	IN	PADAPTER	Adapter
	);

BOOLEAN
N6MatchPrivacyExemptList(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
InitializeAdapterWorkItemCallback(
	IN PVOID			pContext
);

VOID
InitializeAdapterTimerCallback
(
	IN	PRT_TIMER		pTimer
	);

VOID
PNPReConnentTimerCallback(
	IN	PRT_TIMER		pTimer
	);

VOID
D0RxIndicatTimerCallback(
	IN	PRT_TIMER		pTimer
	);

BOOLEAN
N6CQueryPhyIdReady(
	IN	PADAPTER		Adapter
	);

PVOID 
GetTwoPortSharedResource(
	IN		PADAPTER	Adapter,
	IN		u1Byte		Type_ToGet,
	IN		PVOID		pVariable_ToGet, // For more readable on caller side.
	OUT		pu1Byte		pTwoPortStatus // For convinient (pu1Byte)NULL is OK.
	);

VOID
NdisOIDHistoryInit(
	IN PADAPTER pAdapter);

VOID
NdisOIDHistoryDeInit(
	IN PADAPTER pAdapter);

VOID
NdisOIDHistoryUpdate(
	IN	PADAPTER			pAdapter,
	IN	PNDIS_OID_REQUEST	pNdisRequest,
	IN	RT_OID_HISTORY_STATE		OidHistoryState);

// Dialog Token, should be a non zero integer
//      NOTE: DO NOT use "(_DialogToken)++" since in Win8 we predict the token value in OID_DOT11_WFD_GET_DIALOG_TOKEN
#define IncreaseDialogToken(_DialogToken) (((_DialogToken) + 1 == 0) ? ((_DialogToken) += 2) : ((_DialogToken) += 1))

VOID
N6CReleaseDataFrameQueuedWorkItemCallback(
	IN PVOID		pContext
	);

#if WPP_SOFTWARE_TRACE
#define RT_TRACE(_Comp, _Level, Fmt)
#define RT_TRACE_EX(_Comp, _Level, Fmt)
#define RT_TRACE_MAC(_Comp, _Level, Fmt)	
#define RT_TRACE_F(_Comp, _Level, Fmt)	//DoTraceMessage(_Comp, _Level, Fmt)

#define RT_ASSERT(_Exp,Fmt)
#define PRINT_DATA(_TitleString, _HexData, _HexDataLen)
#define RT_PRINT_DATA(_Comp, _Level, _TitleString, _HexData, _HexDataLen)
#define RT_PRINT_ADDR(_Comp, _Level, _TitleString, _Ptr)
#define RT_PRINT_ADDRS(_Comp, _Level, _TitleString, _Ptr, _AddNum)
#define RT_PRINT_STR(_Comp, _Level, _TitleString, _Ptr, _Len)
#define	RT_PRINT_UUID(_Comp, _Level, _TitleString, _UUID)
#else
//
// Macros for debug.
//
#if DBG
#define RT_TRACE(_Comp, _Level, Fmt)												\
			if(((_Comp) & GlobalDebugComponents) && ((_Level <= GlobalDebugLevel) || (_Level == DBG_SERIOUS)))	\
			{																	\
				if(DBG_SERIOUS == _Level) DbgPrint("[ERROR]");	\
				else if(DBG_WARNING == _Level) DbgPrint("[WARNING]");	\
				DbgPrint Fmt;														\
			}

#define RT_TRACE_EX(_Comp, _Level, Fmt)											\
			if(((_Comp) & GLDebugComponents) && ((_Level <= GlobalDebugLevel) || (_Level == DBG_SERIOUS)))	\
			{																	\
				DbgPrint Fmt;														\
			}
#define RT_TRACE_MAC(_Comp, _Level, Fmt)											\
			if(((_Comp) & HalMacDebugComponents) && ((_Level <= GlobalDebugLevel) || (_Level == DBG_SERIOUS)))	\
			{																	\
				DbgPrint Fmt;														\
			}

// Print Debug Message with prefix function name.
#define RT_TRACE_F(_Comp, _Level, Fmt)												\
			if(((_Comp) & GlobalDebugComponents) && (_Level <= GlobalDebugLevel))	\
			{																	\
				DbgPrint("%s%s(): ", (DBG_SERIOUS == _Level) ? "[ERROR]" : (DBG_WARNING == _Level) ? "[WARNING]" : "",__FUNCTION__);									\
				DbgPrint Fmt;														\
			}

#define RT_ASSERT(_Exp,Fmt)														\
			if(!(_Exp))															\
			{																	\
				DbgPrint("Assertion for exp(%s) at file(%s), line(%d), function[%s]\n", #_Exp, __FILE__, __LINE__, __FUNCTION__);												\
				DbgPrint Fmt;														\
				ASSERT(FALSE);													\
			}
			
#define PRINT_DATA(_TitleString, _HexData, _HexDataLen)						\
{												\
	char			*szTitle = _TitleString;					\
	pu1Byte		pbtHexData = _HexData;							\
	u4Byte		u4bHexDataLen = _HexDataLen;						\
	u4Byte		__i;									\
	DbgPrint("%s", szTitle);								\
	for (__i=0;__i<u4bHexDataLen;__i++)								\
	{											\
		if ((__i & 15) == 0) 								\
		{										\
			DbgPrint("\n");								\
		}										\
		DbgPrint("%02X%s", pbtHexData[__i], ( ((__i&3)==3) ? "  " : " ") );			\
	}											\
	DbgPrint("\n");										\
}

// RT_PRINT_XXX macros: implemented for debugging purpose.
// Added by Annie, 2005-11-21.
#define RT_PRINT_DATA(_Comp, _Level, _TitleString, _HexData, _HexDataLen)			\
			if(((_Comp) & GlobalDebugComponents) && (_Level <= GlobalDebugLevel))	\
			{									\
				int __i = 0, __j = 0, __k = 54;								\
				pu1Byte	ptr = (pu1Byte)_HexData;				\
				DbgPrint(__FUNCTION__"():");	\
				DbgPrint(_TitleString);						\
				for( __i=0; __i<(int)_HexDataLen; __i++ )				\
				{								\
					DbgPrint("%02X%s", ptr[__i], (((__i + 1) % 4) == 0)? "  " : " ");	\
					__k -= (3 + ((((__i + 1) % 4) == 0) ? 1 : 0));								\
					if ((((__i + 1) % 16) == 0) || ((__i + 1) == _HexDataLen))	\
					{								\
						for( ; __k > 0; __k --)		\
							DbgPrint(" ");			\
						__k = 54;					\
						for(__j = ((__i / 16) * 16); __j <= __i; __j ++)	\
							DbgPrint("%c", (ptr[__j] < 31 || ptr[__j] > 127) ? '.' : ptr[__j]);		\
						DbgPrint("\n");			\
					}							\
				}								\
			}

#define RT_PRINT_ADDR(_Comp, _Level, _TitleString, _Ptr)					\
			if(((_Comp) & GlobalDebugComponents) && (_Level <= GlobalDebugLevel))	\
			{									\
				int __i;								\
				pu1Byte	ptr = (pu1Byte)_Ptr;					\
				DbgPrint(__FUNCTION__ "(): ");	\
				DbgPrint(_TitleString);						\
				DbgPrint(" ");							\
				for( __i=0; __i<6; __i++ )						\
					DbgPrint("%02X%s", ptr[__i], (__i==5)?"":"-");		\
				DbgPrint("\n");							\
			}

#define RT_PRINT_ADDRS(_Comp, _Level, _TitleString, _Ptr, _AddNum)				\
			if(((_Comp) & GlobalDebugComponents) && (_Level <= GlobalDebugLevel))	\
			{									\
				int __i, __j;							\
				pu1Byte	ptr = (pu1Byte)_Ptr;					\
				DbgPrint(_TitleString);						\
				DbgPrint("\n");							\
				for( __i=0; __i<(int)_AddNum; __i++ )					\
				{								\
					for( __j=0; __j<6; __j++ )					\
						DbgPrint("%02X%s", ptr[__i*6+__j], (__j==5)?"":"-");	\
					DbgPrint("\n");						\
				}								\
			}

// Added by Annie, 2005-11-22.
#define	MAX_STR_LEN	64
#define	PRINTABLE(_ch)	(_ch>=' ' &&_ch<='~' )	// I want to see ASCII 33 to 126 only. Otherwise, I print '?'. Annie, 2005-11-22.

#define RT_PRINT_STR(_Comp, _Level, _TitleString, _Ptr, _Len)					\
			if(((_Comp) & GlobalDebugComponents) && (_Level <= GlobalDebugLevel))	\
			{									\
				int		__i;						\
				u1Byte	buffer[MAX_STR_LEN];					\
				int	length = (_Len<MAX_STR_LEN)? _Len : (MAX_STR_LEN-1) ;	\
				PlatformZeroMemory( buffer, MAX_STR_LEN );			\
				PlatformMoveMemory( buffer, (pu1Byte)_Ptr, length );		\
				for( __i=0; __i<MAX_STR_LEN; __i++ )					\
				{								\
					if( !PRINTABLE(buffer[__i]) )	buffer[__i] = '?';	\
				}								\
				buffer[length] = '\0';						\
				DbgPrint(_TitleString);						\
				DbgPrint(": %d, <%s>\n", _Len, buffer);				\
			}

#define	RT_PRINT_UUID(_Comp, _Level, _TitleString, _UUID)		\
{		\
	RT_TRACE_F(_Comp, _Level, (" %s ", _TitleString));		\
	RT_TRACE(_Comp, _Level, (" %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",	\
		(_UUID).Data1, (_UUID).Data2, (_UUID).Data3, (_UUID).Data4[0], (_UUID).Data4[1],	\
		(_UUID).Data4[2], (_UUID).Data4[3], (_UUID).Data4[4], (_UUID).Data4[5], (_UUID).Data4[6], (_UUID).Data4[7]));	\
}
			
#else	// of #if DBG
// 2009/06/22 MH Allow fre build to print info test.
#define RT_TRACE(_Comp, _Level, Fmt)
#define RT_TRACE_EX(_Comp, _Level, Fmt)
#define RT_TRACE_MAC(_Comp, _Level, Fmt)	
//#define	COMBINE_COMP(_COMP)	_COMP
#define RT_TRACE_F(_Comp, _Level, Fmt)
#define RT_ASSERT(_Exp,Fmt)
#define PRINT_DATA(_TitleString, _HexData, _HexDataLen)
#define RT_PRINT_DATA(_Comp, _Level, _TitleString, _HexData, _HexDataLen)
#define RT_PRINT_ADDR(_Comp, _Level, _TitleString, _Ptr)
#define RT_PRINT_ADDRS(_Comp, _Level, _TitleString, _Ptr, _AddNum)
#define RT_PRINT_STR(_Comp, _Level, _TitleString, _Ptr, _Len)
#define	RT_PRINT_UUID(_Comp, _Level, _TitleString, _UUID)
#endif	// of #if DBG
#endif

#define	N6C_RegisterIoDevice(__pAdapter)		(RT_STATUS_NOT_SUPPORT)
#define	N6C_DeregisterIoDevice(__pAdapter)		(RT_STATUS_NOT_SUPPORT)


#if (WDI_SUPPORT == 1)
#define	OS_SUPPORT_WDI(_pAdapter)		((_pAdapter)->pNdisCommon->WDISupport ? TRUE : FALSE)
#else
#define	OS_SUPPORT_WDI(_pAdapter)		FALSE
#endif


VOID
N6CTimerResourceInit(
	IN	PADAPTER	pAdapter
	);

VOID
N6CTimerResourceInsert(
	IN	PADAPTER 	pAdapter,
	IN	PRT_TIMER 	pTimer
	);

BOOLEAN
N6CTimerResourceRemove(
	IN	PADAPTER	pAdapter,
	IN	PRT_TIMER	pTimer
	);

VOID
N6CTimerResourceDump(
	IN	PADAPTER	pAdapter
	);

VOID
N6CTimerResourceAction(
	IN	PADAPTER	pAdapter,
	IN	RT_TIMER_RESOURCE_ACTION		Action
	);

#endif
