/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    N62C_Port.h

Abstract:
    Contains Port specific defines
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    04-22-2007    Created

Notes:

--*/

#pragma once

#ifndef _N62_PORT__H
#define _N62_PORT__H

typedef struct _ADAPTER         ADAPTER, *PADAPTER;
typedef struct _HVL             HVL, *PHVL;
typedef struct _HW              HW, *PHW;
typedef struct _VNIC            VNIC, *PVNIC;
typedef struct _MP_PORT            MP_PORT, *PMP_PORT;

#define Platform_PORT_ACQUIRE_PNP_MUTEX(_Port)    \
    NDIS_WAIT_FOR_MUTEX(&(_Port->ResetPnpMutex))
#define Platform_PORT_RELEASE_PNP_MUTEX(_Port)    \
    NDIS_RELEASE_MUTEX(&(_Port->ResetPnpMutex))

// If any of the flags are set, we cannot send
#define MP_PORT_CANNOT_SEND_MASK        (MP_PORT_PAUSED | MP_PORT_PAUSING | MP_PORT_HALTING | \
                                            MP_PORT_IN_RESET)

// If any of these flags are set, we cannot scan
#define MP_PORT_CANNOT_SCAN_MASK        (MP_PORT_PAUSED | MP_PORT_PAUSING | MP_PORT_HALTING | \
                                            MP_PORT_IN_RESET)
    
#define MP_SET_PORT_STATUS(_Port, _Flag)    \
    ((_Port->Status) |= (_Flag))

#define MP_CLEAR_PORT_STATUS(_Port, _Flag)  \
    ((_Port->Status) &=! (_Flag))

#define MP_TEST_PORT_STATUS(_Port, _Flag)   \
   ((_Port->Status) =0)

// Port status flags and manipulation macros
#define MP_PORT_PAUSED                  0x00000001
#define MP_PORT_PAUSING                 0x00000002
#define MP_PORT_HALTING                 0x00000010
#define MP_PORT_IN_RESET                0x00000020

#define DEFAULT_NDIS_PORT_NUMBER    0
#define HELPER_PORT_PORT_NUMBER     0xFFFFFFFF
#define MP_MAX_NUMBER_OF_PORT   	  0x8

// 0x05 is enough: ExtSTA + ExtAP + ExtDevice + ExtRoleGO + ExtRoleClient  
#define MP_DEFAULT_NUMBER_OF_PORT 0x5


/** Maximum number of candidate BSS we will use for association */
#define STA_CANDIDATE_AP_MAX_COUNT              32

// Pool tags for this driver
#define MP_MEMORY_TAG           'MltA'  // MP Layer
#define PORT_MEMORY_TAG         'PltA'  // Port
#define EXTSTA_MEMORY_TAG       'SltA'  // ExtSTA
#define EXTAP_MEMORY_TAG        'AltA'  // ExtAP
#define HVL_MEMORY_TAG          'VltA'  // HVL/VNIC
#define HW_MEMORY_TAG           'HltA'  // HW

/** Maximum number of MAC addresses we support in the excluded list */
#define STA_EXCLUDED_MAC_ADDRESS_MAX_COUNT      4
/** Max number of desired BSSIDs we can handle */
#define STA_DESIRED_BSSID_MAX_COUNT             8
/** Max number of desired PHYs we can handle */
#define STA_DESIRED_PHY_MAX_COUNT               5
/** Max number of PMKID we can handle */
#define STA_PMKID_MAX_COUNT                     3
/** Max number of enabled multicast cipher algorithms */
#define STA_MULTICAST_CIPHER_MAX_COUNT          6

#define MP_VERIFY_QUEUE(_PacketQueue)           \
    ASSERT(!((_PacketQueue)->Head == NULL && (_PacketQueue)->Count != 0));   \
    ASSERT(!((_PacketQueue)->Head != NULL && (_PacketQueue)->Count == 0));   \
    ASSERT(!((_PacketQueue)->Head == NULL && (_PacketQueue)->Tail != NULL)); \
    ASSERT(!((_PacketQueue)->Head != NULL && (_PacketQueue)->Tail == NULL));

#define N62CInitPacketQueue(PacketQueue)			\
	NdisZeroMemory(PacketQueue, sizeof(MP_PACKET_QUEUE));

#ifndef MIN
    #define MIN(a, b)                  ((a) < (b)? a : b)
#endif  // MIN
#ifndef MAX
    #define MAX(a, b)                  ((a) > (b)? a : b)
#endif  // MAX

#define MAX_PHY_ID 								10
__inline BOOLEAN
N62CPacketQueueIsEmpty(
	PMP_PACKET_QUEUE PacketQueue
	)
{
	MP_VERIFY_QUEUE(PacketQueue);
	return (((PacketQueue)->Head==NULL)? TRUE : FALSE);
}

__inline BOOLEAN
N62CDeinitPacketQueue(
	PMP_PACKET_QUEUE PacketQueue
	)
{
	return N62CPacketQueueIsEmpty(PacketQueue);
}

typedef struct _PORT_HELPER
{

	RT_WORK_ITEM			CreateDeleteMacWorkitem;	
	RT_TIMER				CreateDeleteMacTimer;
	BOOLEAN					bCreateMac;
	BOOLEAN					bDeleteMac;	
	PDOT11_MAC_INFO			pCreateDot11MacInfo;       
	PNDIS_OID_REQUEST		pCreateDeleteOID;
} PORT_HELPER, *PPORT_HELPER;

typedef struct _RT_NDIS62_COMMON
{

	MP_PORT_OP_STATE	CurrentOpState;	   	// Indicate to Win7.
	MP_PORT_TYPE 		PortType;   			// Used by Port, ChildPort & MP	
	NDIS_PORT_NUMBER	PortNumber; 		// NDIS allocate port number	
	BOOLEAN				bWPSEnable;		// AP Port for OID_DOT11_WPS_ENABLED
	BOOLEAN				bIsDot11PhyIdSet[MAX_PHY_ID];
	BOOLEAN				bDot11SetPhyIdReady;

}RT_NDIS62_COMMON, *PRT_NDIS62_COMMON;


/**
 * Registry configuration information
 */
typedef struct _MP_HELPER_REG_INFO
{
    /** Link quality for low threshold */
    ULONG                       RSSILinkQualityThreshold;

    /** Time in 100ns, to hold a BSS entry even after beacons are missed */
    ULONG                       BSSEntryExpireTime;

    /** Max number of BSS entries to cache */
    ULONG                       BSSEntryMaxCount;

    ULONG                       ActiveScanChannelCount; // MP_SCAN_SET_CHANNEL_COUNT_ACTIVE

    ULONG                       PassiveScanChannelCount;// MP_SCAN_SET_CHANNEL_COUNT_PASSIVE

    ULONG                       ScanRescheduleTime; // MP_SCAN_RESCHEDULE_TIME_MS

    ULONG                       InterScanTime;      // MP_INTER_SCAN_TIME
    
} MP_HELPER_REG_INFO, *PMP_HELPER_REG_INFO;


typedef struct _MP_HELPER_PORT
{
    /** Pointer back to the parent port of this port */
    PMP_PORT                    ParentPort;

    NDIS_DEVICE_POWER_STATE     DevicePowerState;

    /** Pointer to helper port registry info */
    PMP_HELPER_REG_INFO         RegInfo;

    LONG                        TrackingExclusiveAccessAcquireCount;
    LONG                        TrackingExclusiveAccessReleaseCount;
    LONG                        TrackingExclusiveAccessGrantCount;
} MP_HELPER_PORT, *PMP_HELPER_PORT;


/** Contains configuration information read from the registry for each of the port */
typedef struct _MP_PORT_REG_INFO
{
    /** Helper port's info */
    PVOID                       HelperPortRegInfo;
    /** EXTSTA port's info */
    PVOID                       ExtStaRegInfo;
    /** EXTAP port's info */
    PVOID                       ExtAPRegInfo;
}MP_PORT_REG_INFO, *PMP_PORT_REG_INFO;

/**
 * State read from the registry
 */
typedef struct _STA_REG_INFO 
{ 
    /** Max number of adhoc station entries to maintain */
    ULONG                       AdhocStationMaxCount;

    /** Time in 100ns, to hold an entry even after beacons are missed */
    ULONG                       BSSEntryExpireTime;

    /** Number of missed beacons before we roam */
    ULONG                       LostAPRoamBeaconCount;

    /** Number of low quality beacons before we roam */
    ULONG                       RSSIRoamBeaconCount;

    ULONG                       Bogus;
} STA_REG_INFO, *PSTA_REG_INFO;

typedef struct _STA_ADHOC_STA_INFO
{
    /** 
     * Linked list of discovered access points
     * This list must not be modified/read without acquiring the 
     * ListLock
     */
    LIST_ENTRY                  StaList;
    ULONG                       StaCount;
    ULONG                       DeauthStaCount;

    /** 
     * Lock we need before we adding/removing entries from the 
     * discovered AP list. This will be acquired for read by
     * routines that are not modifying the chain and acquired 
     * for write by routines that will be removing entries or
     * adding entries to the chain
     */
     // TODO: Win 7
   // MP_READ_WRITE_LOCK          StaListLock; 

    /** 
     * Lock for changing state, protecting connection variables, etc. 
     *
     * When acquiring both this lock and StaListLock, StaListLock 
     * must be acquired FIRST to ensure consistency throughout the code
     * and avoid deadlock
     */
    NDIS_SPIN_LOCK              StaInfoLock;

    ULONG                       AdHocState;
    
    /** Watchdog timer used in AdHoc mode */
    NDIS_HANDLE                 WatchdogTimer;
    LONG                        TimerCounter;

    /** Connect work item */
    NDIS_HANDLE                 ConnectWorkItem;

    /** For signalling the completion of a synchroneous join request */
    NDIS_EVENT                  JoinCompletionEvent;
    NDIS_STATUS                 JoinCompletionStatus;

    /** For signalling the completion of a synchronous start/stop request */
    NDIS_STATUS                 StartBSSCompletionStatus;
    NDIS_EVENT                  StartBSSCompletionEvent;
    NDIS_EVENT                  StopBSSCompletionEvent;

} STA_ADHOC_STA_INFO, *PSTA_ADHOC_STA_INFO;

/**
 * Holds current configuration of the miniport. This information
 * is updated on OIDs request from the OS. One place this
 * is used is for selecting the access points for making a conneciton
 */
typedef struct _STA_CURRENT_CONFIG 
{
    /** BSS type configured by the OS */
    DOT11_BSS_TYPE              BSSType;

    /** SSID that we can associate with (we only support one SSID)
     */
    DOT11_SSID                  SSID;

    /** List of MAC addresses we should not try to associate with */
    DOT11_MAC_ADDRESS           ExcludedMACAddressList[STA_EXCLUDED_MAC_ADDRESS_MAX_COUNT];
    ULONG                       ExcludedMACAddressCount;
    BOOLEAN                     IgnoreAllMACAddresses;

    /** Desired BSSID we should attempt to associate with */
    DOT11_MAC_ADDRESS           DesiredBSSIDList[STA_DESIRED_BSSID_MAX_COUNT];
    ULONG                       DesiredBSSIDCount;
    BOOLEAN                     AcceptAnyBSSID;

    /** PHY list */
    ULONG                       DesiredPhyList[STA_DESIRED_PHY_MAX_COUNT];
    ULONG                       DesiredPhyCount;
    ULONG                       ActivePhyId;

    /** PMKID list */
    DOT11_PMKID_ENTRY           PMKIDList[STA_PMKID_MAX_COUNT];
    ULONG                       PMKIDCount;

    /** Currently enabled authentication algorithm */
    DOT11_AUTH_ALGORITHM        AuthAlgorithm;  

    /** Currently enabled unicast cipher algorithm */
    DOT11_CIPHER_ALGORITHM      UnicastCipherAlgorithm;  

    /** Currently enabled multicast cipher algorithm */
    DOT11_CIPHER_ALGORITHM      MulticastCipherAlgorithmList[STA_MULTICAST_CIPHER_MAX_COUNT];
    ULONG                       MulticastCipherAlgorithmCount;
    DOT11_CIPHER_ALGORITHM      MulticastCipherAlgorithm;  

    /** Current setting of unreachable detection threshold */
    ULONG                       UnreachableDetectionThreshold;

    /** Current setting related to acceptance of unencrypted data */
    BOOLEAN                     ExcludeUnencrypted;
    PDOT11_PRIVACY_EXEMPTION_LIST   PrivacyExemptionList;

    /** Current association ID */
    BOOLEAN                     ValidAID;
    USHORT                      AID;

    /** Current Listen interval */
    USHORT                      ListenInterval;

    /** Current power saving level */
    ULONG                       PowerSavingLevel;

    /** Current IBSS parameters */
    BOOLEAN                     IBSSJoinOnly;
    PVOID                       AdditionalIEData;
    ULONG                       AdditionalIESize;

    /** Media streaming enabled or not */
    BOOLEAN                     MediaStreamingEnabled;

    /** Determines if we associate with an BSS which does not support any pairwise cipher */
    BOOLEAN                     UnicastUseGroupEnabled;

    /** Current hidden OID setting */
    BOOLEAN                     HiddenNetworkEnabled;
    
    /** check for use protection bit in beacon ERP IE after a successful connection or scan */
    BOOLEAN                     CheckForProtectionInERP;
}STA_CURRENT_CONFIG, *PSTA_CURRENT_CONFIG;


/**
 * Holds information about each BSS we have found.
 * This is updated on a beacon/probe response in the context of our
 * receive handler. A linked list of these entries is maintained
 * by the station to keep track of the discovered BSS
 */
typedef struct _MP_BSS_ENTRY 
{
    /** List entry linkage */
    LIST_ENTRY                  Link;

    /** Reference count to keep track of number of users of the AP entry.
     * When the entry is added to the linked list, this starts at 1. Association
     * process would add an extra reference to keep the entry around while
     * it is still associating/associated.
     */
    LONG                        RefCount;

    /**
     * Spinlock to protect modification of beacon/information element pointers while
     * other threads may be using it
     */
    NDIS_SPIN_LOCK              Lock;

    DOT11_PHY_TYPE              Dot11PhyType;

    ULONG                       PhyId;

    /** RSSI for the beacon/probe */
    LONG                        RSSI;

    /** Link quality */
    ULONG                       LinkQuality;

    ULONG                       ChannelCenterFrequency;
    
    DOT11_BSS_TYPE              Dot11BSSType;

    DOT11_MAC_ADDRESS           Dot11BSSID;

    DOT11_MAC_ADDRESS           MacAddress;

    USHORT                      BeaconInterval;

    ULONGLONG                   BeaconTimestamp;

    ULONGLONG                   HostTimestamp;

    DOT11_CAPABILITY            Dot11Capability;
    
    ULONG                       MaxBeaconFrameSize;
    PUCHAR                      pDot11BeaconFrame;      // Beacon frame starting after management header
    ULONG                       BeaconFrameSize;
    
    PUCHAR                      pDot11InfoElemBlob;     // Pointer to the information elements in the beacon frame
    ULONG                       InfoElemBlobSize;       // Length of information element blob

    UCHAR                       Channel;    // Valid only if it is non-zero

    DOT11_SSID                  ProbeSSID;  // The SSID from probe responses. This is stored
                                            // separately since probe buffer may get overwritten
                                            // by beacons and we may lose the SSID from probe

    /** 
     * The OS needs to be given the association request 
     * packet information on a association completion. This 
     * information is cached in this structure
     */
    __field_ecount(AssocRequestLength) 
    PUCHAR                      pAssocRequest;
    USHORT                      AssocRequestLength;     // Includes header

    /** 
     * The OS needs to be given the association response
     * packet information on a association completion. This 
     * information is cached in this structure
     */
    USHORT                      AssocResponseLength;    // Includes header
    __field_ecount(AssocResponseLength) 
    PUCHAR                      pAssocResponse;

    /** Association ID */
    USHORT                      AssocID;

    /** Association state. Just keeps state and not lock protected */
    DOT11_ASSOCIATION_STATE     AssocState;

    /** Timestamp when we obtained the association */
    LARGE_INTEGER               AssociationUpTime;

    /** 
     * Cost for roaming purpose. A lower number is better. We dont
     * rank of APs based on this, but use higher number this for rejecting 
     * some AP
     */
    LONG                        AssocCost;

    /**
     * Time in 100 ns after which we assume connectivity is lost
     * with this AP
     */
    ULONGLONG                   NoBeaconRoamTime;

    /**
     * Number of contiguous beacons which had signal strength lower than
     * our roaming threshold
     */
    ULONG                       LowQualityCount;

} MP_BSS_ENTRY, * PMP_BSS_ENTRY;

/** Structure holds for passing BSS information among the various layer */
typedef struct _MP_BSS_DESCRIPTION
{
    DOT11_BSS_TYPE  BSSType;

    DOT11_MAC_ADDRESS BSSID;

    DOT11_MAC_ADDRESS MacAddress;

    USHORT          BeaconPeriod;

    ULONGLONG       Timestamp;

    DOT11_CAPABILITY Capability;

    /* When true the hardware wont beacon */
    BOOLEAN         NoBeaconMode;
    
    ULONG           PhyId;
    UCHAR           Channel;    // Valid only if it is non-zero

    ULONG           InfoElemBlobSize;
    UCHAR           InfoElemBlob[1];              // Must be the last field.

} MP_BSS_DESCRIPTION, *PMP_BSS_DESCRIPTION;

/**
 * Connection context structure used during infrastructure connection attempt
 */
typedef struct _STA_INFRA_CONNECT_CONTEXT
{
    /** 
     * Used to keep track of number of asynchronous functions pending to
     * complete the connection attempt. This is used in CONN_STATE_IN_RESET 
     * to wait for the connection process to complete before reseting the
     * adapter. Reset waits for this to go to zero. This structure is 
     * modified using interlocked operations.
     */
    LONG                        AsyncFuncCount;

    /**
     * Different events can cause the driver to reset its connection attempt.
     * This mutex is used to serialize multiple simultaneous reset/cleanup
     * routines
     */
    NDIS_MUTEX                  DisconnectGate;
    
    /** 
     * Lock for changing state, protecting connection variables, etc. 
     * This is used for connecting and during scanning
     *
     * When acquiring both this lock and lock on the AP entry, the AP entry 
     * lock must be acquired second to ensure consistency throughout the code
     * and avoid deadlock
     */
    NDIS_SPIN_LOCK              Lock;

    /** 
     * Current association state of the station. This variable is 
     * normally modified by the various association routines. The Reset/Disconnect
     * routines will modify this once they are sure no association routine is using
     * this. The Connect routine would initialize this. This is modified
     * with the Lock held
     */
    STA_ASSOC_STATE             AssociateState;

    /**
     * DeAuthReason value is valid only when AssociateState == ASSOC_STATE_REMOTELY_DEAUTHENTICATED
     */
    USHORT                      DeAuthReason;

    /** 
     * This field is only meaningful when AssociateState is 
     * ASSOC_STATE_WAITING_FOR_AUTHENTICATE. It specifies the sequence 
     * number of the authenticate response we are expecting.
     */
    USHORT                      ExpectedAuthSeqNumber;

    /** 
     * Tracks whether or not we are allowed to connect. This is only 
     * modified by the Connect/Reset/Disconnect routines. The various
     * functions that handle associate would read this to check if they 
     * should continue with the association. This is modified
     * with the Lock held
     */
    STA_CONNECT_STATE           ConnectState;

    /**
     * Stores the previous connect state. This is used when we are reset
     * etc in the middle of a connect.
     */
    STA_CONNECT_STATE           PreviousConnectState;
    
    /** 
     * BSSEntry that we are currently using for association, are associated with. 
     * This is modified with the Lock held
     */
    PMP_BSS_ENTRY              ActiveAP;
    
    /**
     * The candidate access points we will attempt to associate with.
     */
    PMP_BSS_ENTRY              CandidateAPList[STA_CANDIDATE_AP_MAX_COUNT];
    ULONG                       CandidateAPCount;

    /** Index of the candidate AP we are currently associating with (starts with 0) */
    ULONG                       CurrentCandidateAPIndex;

    /**
     * Buffer to use for association completion. We will preallocate this
     * buffer to ensure that we are able to indicate completion even in case
     * of low resources. We only use this for signalling association completion
     * in case we run low on resources
     */
    PUCHAR                      pAssocFailBuffer;

    /** 
     * Timer object used when waiting for authentication process to complete.
     * The timer will be set when the authentication process begins
     */
    NDIS_HANDLE                 Timer_AuthenticateTimeout;

    /** 
     * Timer object used when waiting for association process to complete.
     * The timer will be set when the association process begins
     */
    NDIS_HANDLE                 Timer_AssociateTimeout;

    BOOLEAN                     RoamForSendFailures;

    /** indicate link quality to the OS */
    BOOLEAN                     UpdateLinkQuality;

    PMP_BSS_DESCRIPTION        JoinBSSDescription;
    ULONG                      JoinFailureTimeout;
}STA_INFRA_CONNECT_CONTEXT, *PSTA_INFRA_CONNECT_CONTEXT;

/**
 * State we maintain for holding 
 */
typedef struct _STA_SCAN_CONTEXT
{
    /** Scan related data. (May be grouped together later) */
    PVOID                       ScanRequestID;

    /** Scan request buffer for OID request from OS */
    PDOT11_SCAN_REQUEST_V2      ExternalScanRequestBuffer;

    /** Timer we use for periodic scanning */
    NDIS_HANDLE                 Timer_PeriodicScan;

    /** Scan request buffer we use for internal scan request */
    PDOT11_SCAN_REQUEST_V2      InternalScanRequestBuffer;

    /** Length of above buffer */
    ULONG                       InternalScanBufferLength;

    /** Flags that keep track of scan state */
    ULONG                       Flags;

    /** Tracks how many times the periodic scans have been disabled. When non-zero
     * periodic scans shouldnt happen
     */
    ULONG                       PeriodicScanDisableCount;
    
    /**
     * Number of times that the periodic scan timer has run, but
     * did has not done any scanning
     */
    ULONG                       PeriodicScanCounter;

    /**
     * Number of ticks since are have performed a periodic scan
     * for roaming/connectiong purposes
     */
    ULONG                       RoamingScanGap;

    /** The time at which we started the last scan (periodic or OS scan) */
    ULONGLONG                   LastScanTime;

    /**
     * Set to true to indicate that we are scanning for roaming
     */
    BOOLEAN                     ScanForRoam;

    /**
     * Set to true when we need to roam now or we have a problem
     */
    BOOLEAN                     MustRoam;

    /** 
     * This is generally true and we would be adding the SSID of the network in
     * our probe requests. When we go to sleep, this becomes false, causing us to 
     * stop putting the SSID in our probe requests until we wake up
     */
    BOOLEAN                     SSIDInProbeRequest;

}STA_SCAN_CONTEXT, *PSTA_SCAN_CONTEXT;

typedef struct _STA_PMKID_CACHE {
    
    /** The time at which we checked for PMKID candidate */
    ULONGLONG                   CheckingTime;

    /** Number of PMKID candidates indicated last time */
    ULONG                       Count;

    /** PMKID candidates indicated last time */
    DOT11_BSSID_CANDIDATE       Candidate[STA_PMKID_MAX_COUNT];

} STA_PMKID_CACHE, PSTA_PMKID_CACHE;


/**
 * Maintains station side statistics
 */
typedef struct _STA_STATS
{
    ULONGLONG                   ullUcastWEPExcludedCount;
    ULONGLONG                   ullMcastWEPExcludedCount;
    
}STA_STATS, *PSTA_STATS;

/*
* Define structure for Helper Port
*
*/
/** Used to hold state about miniport pause */
typedef struct _MP_MINIPORT_PAUSE_PARAMETERS
{
    /** The pause parameters provided by NDIS */
    PNDIS_MINIPORT_PAUSE_PARAMETERS NdisParameters;
    
    /** The event that is fired when the pause is completed */
    NDIS_EVENT              CompleteEvent;

}MP_MINIPORT_PAUSE_PARAMETERS, *PMP_MINIPORT_PAUSE_PARAMETERS;


/** Used to hold state about ndis reset */
typedef struct _MP_NDIS_RESET_PARAMETERS
{
    PBOOLEAN                AddressingReset;

    NDIS_STATUS             ResetStatus;    

    /** The event that is fired when the reset is completed */
    NDIS_EVENT              CompleteEvent;
}MP_NDIS_RESET_PARAMETERS, *PMP_NDIS_RESET_PARAMETERS;


/** Used to hold state about power OID */
typedef struct _MP_POWER_PARAMETERS
{
    /** The device state to transition to */
    NDIS_DEVICE_POWER_STATE NewDeviceState;
    
    /** The event that is fired when the oid processing is completed */
    NDIS_EVENT              CompleteEvent;

}MP_POWER_PARAMETERS, *PMP_POWER_PARAMETERS;


/** Used to hold state about port pause */
typedef struct _MP_PORT_PAUSE_PARAMETERS
{
    /** The port that needs to be paused */
    PMP_PORT                PortToPause;

    /** The event that is fired when the pause is completed */
    NDIS_EVENT              CompleteEvent;

}MP_PORT_PAUSE_PARAMETERS, *PMP_PORT_PAUSE_PARAMETERS;


/** Used to hold state about port terminate */
typedef struct _MP_TERMINATE_PORT_PARAMETERS
{
    /** The port that needs to be terminated */
    PMP_PORT                PortToTerminate;

    /** The event that is fired when the terminate is completed */
    NDIS_EVENT              CompleteEvent;
}MP_TERMINATE_PORT_PARAMETERS, *PMP_TERMINATE_PORT_PARAMETERS;


typedef struct _MP_EXTSTA_PORT
{
    PMP_PORT                    ParentPort;

    /** State from the registry */
    PSTA_REG_INFO               RegInfo;

    /** Information about AdHoc stations */
	STA_ADHOC_STA_INFO          AdHocStaInfo;

    /** Current configuration of extensible station */
	STA_CURRENT_CONFIG          Config;

    /** Connection request context information */
	STA_INFRA_CONNECT_CONTEXT   ConnectContext;

    /** State we maintaining scan state */
    STA_SCAN_CONTEXT            ScanContext;

    /** Current PMKID cache the station maintains */
    STA_PMKID_CACHE             PMKIDCache;

    /** Statistics information */
    STA_STATS                   Statistics;

} MP_EXTSTA_PORT, *PMP_EXTSTA_PORT;

VOID
N62CAcquireMUTEX(
	PMP_PORT				Port,
	RT_MUTEX_TYPE			type
	);

VOID
N62CReleaseMUTEX(
	PMP_PORT				Port,
	RT_MUTEX_TYPE			type
	);

VOID
N62CHelperPortHandlePortTerminate(
    IN PMP_PORT                 Port,
    IN PMP_PORT                 PortToTerminate
    );

NDIS_STATUS
N62C_OID_DOT11_CREATE_MAC(
	IN  PADAPTER				pAdapter,
	IN  PNDIS_OID_REQUEST   	NdisRequest
	);

NDIS_STATUS
N62C_OID_DOT11_DELETE_MAC(
	IN  PADAPTER				pAdapter,
	IN  PNDIS_OID_REQUEST   	NdisRequest
	);

VOID
N62CCreateDeleteMacTimerCallback(
	IN PRT_TIMER			pTimer
	);

VOID
N62CCreateDeleteMacWorkItemCallback(
	IN PVOID			pContext
	);

VOID
N62CApSendDisassocWithOldChnlWorkitemCallback(
	IN PVOID			pContext
	);


NDIS_STATUS
N62CCreateMac(
	IN PADAPTER				pAdapter
	);

NDIS_STATUS
N62CDeleteMac(
	IN PADAPTER				pAdapter
	);

NDIS_STATUS
N62CHelperAllocateNdisPort(
	IN PADAPTER					pAdapter,
	OUT PNDIS_PORT_NUMBER		AllocatedPortNumber
	);

NDIS_STATUS
N62CStaInitializePort(
    IN  PMP_PORT		Port,
    IN  PVOID			RegistryInformation
    );

NDIS_STATUS 
N62CStaOidHandler(
	IN  PADAPTER			pAdapter,
	IN  PMP_PORT                Port,
	IN  PNDIS_OID_REQUEST       OidRequest
	);

NDIS_STATUS 
N62CStaSendEventHandler(
    IN  PMP_PORT                Port,
    //IN  PMP_TX_MSDU             PacketList,
    IN  ULONG                   SendFlags
    );

NDIS_STATUS
N62CStaReceiveEventHandler(
    IN  PMP_PORT                Port,
   // IN  PMP_RX_MSDU             PacketList,
    IN  ULONG                   ReceiveFlags
    );

NDIS_STATUS
N62CHelperHandleMiniportRestart(
	IN PADAPTER								Adapter,
	IN  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
	);

NDIS_STATUS
N62CHelperHandleMiniportPause(
	IN PADAPTER								pAdapter,
	IN  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
	);

NDIS_STATUS
N62CHelperActivateNdisPort(
	IN PADAPTER				  pAdapter,
	IN  NDIS_PORT_NUMBER        PortNumberToActivate
	);

VOID
N62CHelperDeactivateNdisPort(
	IN PADAPTER				pAdapter,
    IN  NDIS_PORT_NUMBER        PortNumberToDeactivate
    );

VOID
N62CHelperFreeNdisPort(
    IN PADAPTER				pAdapter,
    IN  NDIS_PORT_NUMBER        PortNumberToFree
    );

NDIS_STATUS
N62CHandleMiniportPause(
	IN PADAPTER								pAdapter,
	IN  PNDIS_MINIPORT_PAUSE_PARAMETERS	MiniportPauseParameters
	);

MP_PORT_TYPE
N62CGetPortTypeByOpMode(
    IN  ULONG                   OpMode
    );

//NDIS_STATUS
//N62CSetCurrentPortOperationMode(
//	IN  PADAPTER				pAdapter,
//	IN  PNDIS_OID_REQUEST   	NdisRequest	
//	);

NDIS_STATUS
N62CSetOperationMode(
	IN  PADAPTER			pAdapter,
	IN  ULONG			OpMode
    	);


NDIS_STATUS
N62CPortSetOperationMode(
	IN  PADAPTER		   pAdapter,
	IN  ULONG                   OpMode
	);

VOID
N62CDeleteVirtualPort(
	IN  PADAPTER		   pAdapter
	);

VOID
N62CChangePortType(
	IN  PADAPTER			pAdapter,
	IN  MP_PORT_TYPE		OldPortType,
	IN  MP_PORT_TYPE		NewPortType
	);

VOID
N62CExtAdapterHandleNBLInWaitQueue(
	IN PADAPTER pAdapter
	);

NDIS_STATUS
N62CChangePortTypeByOpMode(
	IN  PADAPTER			pAdapter,
	IN	PDOT11_CURRENT_OPERATION_MODE dot11OpMode
);	

#endif //_N62_PORT__H
