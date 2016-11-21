/*++

Copyright (c) Realtek Corporation. All rights reserved.

Module Name:
    N62C_AP_Def.h

Abstract:
    Contains AP Define and Structure
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    04-22-2008    Created

Notes:

--*/

#pragma once

#ifndef _AP_DEFS_H
#define _AP_DEFS_H

#define AP11_MAX_IE_BLOB_SIZE   (DOT11_MAX_PDU_SIZE - sizeof(DOT11_MGMT_HEADER) - \
                                   FIELD_OFFSET(DOT11_BEACON_FRAME, InfoElements) - 4)
#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

/**
 * Capabilities
 */
#define AP_SCAN_SSID_LIST_MAX_SIZE                          4
#define AP_DESIRED_SSID_LIST_MAX_SIZE                       1
#define AP_STRICTLY_ORDERED_SERVICE_CLASS_IMPLEMENTED       FALSE

/** ExtAP default configurations (hardware independent) */
#define AP_DEFAULT_ENABLED_AUTO_CONFIG          (DOT11_PHY_AUTO_CONFIG_ENABLED_FLAG | DOT11_MAC_AUTO_CONFIG_ENABLED_FLAG)

#define AP_DEFAULT_EXCLUDE_UNENCRYPTED          FALSE

#define AP_DEFAULT_AUTHENTICATION_ALGORITHM     DOT11_AUTH_ALGO_RSNA_PSK

#define AP_DEFAULT_UNICAST_CIPHER_ALGORITHM     DOT11_CIPHER_ALGO_CCMP

#define AP_DEFAULT_MULTICAST_CIPHER_ALGORITHM   AP_DEFAULT_UNICAST_CIPHER_ALGORITHM

#define AP_DEFAULT_DESIRED_PHY_ID_COUNT         1

#define AP_DEFAULT_DESIRED_PHY_ID               DOT11_PHY_ID_ANY

#define AP_DEFAULT_PHY_ID                       0

#define AP_DEFAULT_ADDITIONAL_IE_SIZE           0

#define AP_DEFAULT_ADDITIONAL_IE_DATA           NULL

#define AP_DEFAULT_ENABLE_WPS                   FALSE

/** 
 * ExtAP default registy info settings, with their ranges
 * a default setting is used if the registry is not present or has invalid value
 */

#define AP_DEFAULT_ALLOWED_ASSOCIATION_COUNT    128
#define AP_MIN_ALLOWED_ASSOCIATION_COUNT        0
#define AP_MAX_ALLOWED_ASSOCIATION_COUNT        255
    
#define AP_DEFAULT_ENABLE_5GHZ                  FALSE

#define AP_DEFAULT_CHANNEL                      11
#define AP_MIN_CHANNEL                          0
#define AP_MAX_CHANNEL                          11
#define AP_MAX_CHANNEL_FOR_11A                  136

#define AP_DEFAULT_FREQUENCY                    136

#define AP_DEFAULT_GROUP_KEY_RENEWAL_INTERVAL   3600        // in seconds
#define AP_MIN_GROUP_KEY_RENEWAL_INTERVAL       60
#define AP_MAX_GROUP_KEY_RENEWAL_INTERVAL       86400

#define AP_DEFAULT_ENABLE_CTS_PROTECTION        FALSE

#define AP_DEFAULT_ENABLE_FRAME_BURST           FALSE

#define AP_DEFAULT_BEACON_PERIOD                100         // in TUs
#define AP_MIN_BEACON_PERIOD                    10
#define AP_MAX_BEACON_PERIOD                    10000

#define AP_DEFAULT_DTIM_PERIOD                  2           // beacon intervals
#define AP_MIN_DTIM_PERIOD                      1
#define AP_MAX_DTIM_PERIOD                      100

#define AP_DEFAULT_RTS_THRESHOLD                2347
#define AP_MIN_RTS_THRESHOLD                    0
#define AP_MAX_RTS_THRESHOLD                    2347

#define AP_DEFAULT_FRAGMENTATION_THRSHOLD       2346
#define AP_MIN_FRAGMENTATION_THRESHOLD          256
#define AP_MAX_FRAGMENTATION_THRESHOLD          2346

#define AP_DEFAULT_SHORT_RETRY_LIMIT            7
#define AP_MIN_SHORT_RETRY_LIMIT                1
#define AP_MAX_SHORT_RETRY_LIMIT                255

#define AP_DEFAULT_LONG_RETRY_LIMIT             4
#define AP_MIN_LONG_RETRY_LIMIT                 1
#define AP_MAX_LONG_RETRY_LIMIT                 255

#define AP_DEFAULT_ENABLE_WMM                   FALSE

/**
*	Define in AP Assoc mgr WIN7 Sample Code
*/
/** Maximum number of stations we will cache state about */
#define AP_STA_MAX_ENTRIES_DEFAULT              32
#define AP_STA_MAX_ENTRIES_MIN                  16
#define AP_STA_MAX_ENTRIES_MAX                  64

/** Time to wait for association request from station after auth success (in number of milliseconds) */
#define AP_ASSOCIATION_REQUEST_TIMEOUT          100

/** Time in which the station has no activity we assume we have lost connectivity to that station (in number of seconds) */
#define AP_NO_ACTIVITY_TIME                     1800
/** The interval of the station inactive timer, in milliseconds */
#define AP_STA_INACTIVE_TIMER_INTERVAL          1000        

#define AP_MAX_AID                              2007
#define AP_INVALID_AID                          0xFFFF
#define AP_AID_TABLE_UNIT_SIZE                  8          
#define AP_AID_TABLE_UNIT_MASK                  0xFF
#define AP_AID_TABLE_SIZE                       (AP_MAX_AID/AP_AID_TABLE_UNIT_SIZE + 1)
#define AP_AID_HEADER                           0xC000

#define AP_DESIRED_PHY_MAX_COUNT    1

#define MAC_HASH_BUCKET_NUMBER      256

/** MAC hash function */
#define HASH_MAC(Mac)               (((Mac)[5] ^ (Mac)[4]) % MAC_HASH_BUCKET_NUMBER)

#define DOT11_MAX_MSDU_SIZE     (2346U)



/** 
 * Get access to the MP_EXTSTA_PORT from the MP_PORT
 */
#define MP_GET_AP_PORT(_Port)           ((PMP_EXTAP_PORT)(_Port->ChildPort))
#define AP_GET_MP_PORT(_ExtPort)        ((PMP_PORT)(_ExtPort->ParentPort))
#define AP_GET_VNIC(_ExtPort)           (AP_GET_MP_PORT(_ExtPort)->VNic)
#define AP_GET_ADAPTER(_ExtPort)        (AP_GET_MP_PORT(_ExtPort)->Adapter)
#define AP_GET_MP_HANDLE(_ExtPort)      (AP_GET_MP_PORT(_ExtPort)->MiniportAdapterHandle)
#define AP_GET_PORT_NUMBER(_ExtPort)    (AP_GET_MP_PORT(_ExtPort)->PortNumber)

/**
 * Get access to EXTAP components
 */
#define AP_GET_ASSOC_MGR(_ApPort)       (&(_ApPort)->AssocMgr)
#define AP_GET_CONFIG(_ApPort)          (&(_ApPort)->Config)
#define AP_GET_CAPABILITY(_ApPort)      (&(_ApPort)->Capability)
#define AP_GET_REG_INFO(_ApPort)        (&(_ApPort)->RegInfo)

/**
 * Get access to AP settings
 */

/** Settings maintained by association manager */ 
#define AP_GET_SSID(_ApPort)                    (AP_GET_ASSOC_MGR(_ApPort)->Ssid)
#define AP_GET_BSSID(_ApPort)                   (AP_GET_ASSOC_MGR(_ApPort)->Bssid)
#define AP_GET_CAPABILITY_INFO(_ApPort)         (AP_GET_ASSOC_MGR(_ApPort)->Capability)
#define AP_GET_AUTH_ALGO(_ApPort)               (AP_GET_ASSOC_MGR(_ApPort)->AuthAlgorithm)
#define AP_GET_UNICAST_CIPHER_ALGO(_ApPort)     (AP_GET_ASSOC_MGR(_ApPort)->UnicastCipherAlgorithm)
#define AP_GET_MULTICAST_CIPHER_ALGO(_ApPort)   (AP_GET_ASSOC_MGR(_ApPort)->MulticastCipherAlgorithm)
#define AP_GET_OPERATIONAL_RATE_SET(_ApPort)    (AP_GET_ASSOC_MGR(_ApPort)->OperationalRateSet)
#define AP_GET_WPS_ENABLED(_ApPort)             (AP_GET_ASSOC_MGR(_ApPort)->EnableWps)

/** Settings maintained by config */
#define AP_GET_BEACON_PERIOD(_ApPort)           (AP_GET_CONFIG(_ApPort)->BeaconPeriod)
#define AP_GET_DTIM_PERIOD(_ApPort)             (AP_GET_CONFIG(_ApPort)->DTimPeriod)
#define AP_GET_BEACON_IE(_ApPort)               (AP_GET_CONFIG(_ApPort)->AdditionalBeaconIEData)
#define AP_GET_BEACON_IE_SIZE(_ApPort)          (AP_GET_CONFIG(_ApPort)->AdditionalBeaconIESize)
#define AP_GET_PROBE_RESPONSE_IE(_ApPort)       (AP_GET_CONFIG(_ApPort)->AdditionalResponseIEData)
#define AP_GET_PROBE_RESPONSE_IE_SIZE(_ApPort)  (AP_GET_CONFIG(_ApPort)->AdditionalResponseIESize)

	/** commonly used MACROs */
#define VALIDATE_AP_INIT_STATE(pAdapter) \
	{ \
		if (GetAPState(pAdapter) != AP_STATE_STOPPED) \
		{ \
			Status = NDIS_STATUS_INVALID_STATE; \
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY | COMP_INIT | COMP_INDIC), DBG_LOUD, ("AP is not in INIT STATE\n"));\
			break; \
		} \
	}

#define VALIDATE_AP_OP_STATE(pAdapter) \
	{ \
		if (GetAPState(pAdapter) != AP_STATE_STARTED) \
		{ \
			Status = NDIS_STATUS_INVALID_STATE; \
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY | COMP_INIT | COMP_INDIC), DBG_LOUD, ("AP is not in OP STATE\n"));\
			break; \
		} \
	}

extern DOT11_MAC_ADDRESS Dot11BroadcastAddress;

/** Association Manager state */
typedef enum _AP_ASSOC_MGR_STATE
{
    AP_ASSOC_MGR_STATE_NOT_INITIALIZED = 0,
    AP_ASSOC_MGR_STATE_STARTING,
    AP_ASSOC_MGR_STATE_STARTED,
    AP_ASSOC_MGR_STATE_STOPPING,
    AP_ASSOC_MGR_STATE_STOPPED
} AP_ASSOC_MGR_STATE, *PAP_ASSOC_MGR_STATE;

//==M==> It might not need define in here
/**
 * Station port state
 */
typedef enum _STA_PORT_STATE
{
    STA_PORT_STATE_INVALID = 0,
    STA_PORT_STATE_OPEN,
    STA_PORT_STATE_CLOSED
} STA_PORT_STATE, *PSTA_PORT_STATE;

/** 
 * DOT11 Frame Class
 * See definitions in 802.11 standard.
 */
typedef enum _DOT11_FRAME_CLASS
{
    DOT11_FRAME_CLASS_1,
    DOT11_FRAME_CLASS_2,
    DOT11_FRAME_CLASS_3,
    DOT11_FRAME_CLASS_INVALID
} DOT11_FRAME_CLASS, *PDOT11_FRAME_CLASS;

typedef struct _MP_EXTAP_PORT      MP_EXTAP_PORT, *PMP_EXTAP_PORT;

//For store Station mac address Link to AP Port
/**
 * A hash table based on the MAC address of a station.
 * Stations with the same hash value are organized as a linked list.
 */
typedef struct _MAC_HASH_TABLE {
    /**
     * the table
     */
    LIST_ENTRY Buckets[MAC_HASH_BUCKET_NUMBER];

    /**
     * number of entries
     */
    ULONG EntryCount;
} MAC_HASH_TABLE, *PMAC_HASH_TABLE;

typedef const MAC_HASH_TABLE* PCMAC_HASH_TABLE;

/**
 * A hash table entry.
 */
typedef struct _MAC_HASH_ENTRY {
    LIST_ENTRY Linkage;

    /**
     * MAC address as the key of an entry
     */
    DOT11_MAC_ADDRESS MacKey;
} MAC_HASH_ENTRY, *PMAC_HASH_ENTRY;

/**
 * Callback function for APEnumMacEntry.
 *
 * The callback can return FALSE to terminate the
 * enumeration prematurely.
 */
typedef BOOLEAN
(*PENUM_MAC_ENTRY_CALLBACK)(
    __in PMAC_HASH_TABLE Table,
    __in PMAC_HASH_ENTRY MacEntry,
    __in PVOID CallbackCtxt
    );


typedef const MAC_HASH_ENTRY* PCMAC_HASH_ENTRY;


// TODO: do we really need this?
/** ExtAP capability */
typedef struct _AP_CAPABIITY
{
    /** Maximum number of SSIDs the NIC can support in OID_DOT11_SCAN_REQUEST */
    ULONG ScanSsidListSize;

    /** Maximum number of desired SSIDs the NIC can support */
    ULONG DesiredSsidListSize;

    /** Maximum number of Ethertype privacy exemptions the NIC can support */
    ULONG PrivacyExemptionListSize;

    /** Maximum number of associations the NIC can support */
    ULONG AssociationTableSize;
    
    // TODO: add other capabilities
} AP_CAPABIITY, *PAP_CAPABIITY;

/**
 * Settings read from the registry
 */
typedef struct _AP_REG_INFO 
{ 
    /** number of allowed associations */
    ULONG   AllowedAssociationCount;

    /** default channel/frequency */
    ULONG   DefaultChannel;

    // TODO: AP Mode (mixed)

    /** group key renewal interval, in seconds */
    ULONG   GroupKeyRenewalInterval;

    // TODO: transmission rate

    /** Beacon period, in TUs */
    ULONG   BeaconPeriod;

    /** DTIM period, in beacon intervals */
    ULONG   DTimPeriod;

    /** RTS threshold */
    ULONG   RtsThreshold;

    /** Fragmentation threshold */
    ULONG   FragmentationThreshold;

    /** Short retry limit */
    ULONG   ShortRetryLimit;

    /** Long retry limit */
    ULONG   LongRetryLimit;

    /** enable 5GHz or not */
    BOOLEAN Enable5GHz;

    /** enable CTS protection */
    BOOLEAN EnableCtsProtection;

    /** enable Frame burst */
    BOOLEAN EnableFrameBurst;

    /** enable WMM */
    BOOLEAN EnableWMM;
} AP_REG_INFO, *PAP_REG_INFO;

/**
 * ExtAP association manager
 */
typedef struct _AP_ASSOC_MGR
{
    /** ExtAP port */
    PMP_EXTAP_PORT          ApPort;
    
    /** state of Association Manager */
    AP_ASSOC_MGR_STATE      State;
    
    /**
     * Hash table for the stations
     */
    MAC_HASH_TABLE          MacHashTable;

    /** 
     * Lock we need before we adding/removing entries from the 
     * hash table. This will be acquired for read by
     * routines that are not modifying the table and acquired 
     * for write by routines that will be removing entries or
     * adding entries to the table.
     */
    //MP_READ_WRITE_LOCK      MacHashTableLock;

    /** AID table, a bit for each AID */
    UCHAR                   AidTable[AP_AID_TABLE_SIZE];
    
    /** 
     * Association related configurations
     * A lock is NOT required when updating/querying these configurations
     */

    /** SSID that we advertise (we only support one SSID) */
    DOT11_SSID              Ssid;

    /** AP BSSID */
    DOT11_MAC_ADDRESS       Bssid;

    /** Capability information */
    DOT11_CAPABILITY        Capability;
    
    /** Currently enabled authentication algorithm */
    DOT11_AUTH_ALGORITHM    AuthAlgorithm;  

    /** Currently enabled unicast cipher algorithm */
    DOT11_CIPHER_ALGORITHM  UnicastCipherAlgorithm;  

    /** Currently enabled multicast cipher algorithm */
    DOT11_CIPHER_ALGORITHM  MulticastCipherAlgorithm;  

    /** Use default auth cipher algorithms **/
    BOOLEAN bUseDefaultAlgorithms;

    /** Operational rate set */
    DOT11_RATE_SET          OperationalRateSet;

    /** Current setting related to acceptance of unencrypted data */
    BOOLEAN                 ExcludeUnencrypted;
    PDOT11_PRIVACY_EXEMPTION_LIST   PrivacyExemptionList;

    /** Enable WPS */
    BOOLEAN                 EnableWps;

    /** Scan related data */
    /** Scan in process */
    LONG                    ScanInProcess;

    /** Local copy of scan request */
    PDOT11_SCAN_REQUEST_V2  ScanRequest;

    /** Scan request ID */
    PVOID                   ScanRequestId;
    
    /** 
     * Station inactive timer 
     * When the timer fires, the inactive time of each
     * station is incremented by 1.
     */
    NDIS_HANDLE              StaInactiveTimer;

    /**
     * This is actually the count of the associated
     * stations because each station is going to 
     * increase the counter by 1 when it is associated
     * and decrement by 1 when it is disassociated.
     */
    LONG                    StaInactiveTimerCounter;

    /** For signalling the completion of a synchronous start request */
    NDIS_STATUS             StartBSSCompletionStatus;
    NDIS_EVENT              StartBSSCompletionEvent;
    NDIS_EVENT              StopBSSCompletionEvent;

    NDIS_STATUS             SetChannelCompletionStatus;
    NDIS_EVENT              SetChannelCompletionEvent;
         
} AP_ASSOC_MGR, *PAP_ASSOC_MGR;

/**
 * Tracks the state of a station
 */
typedef struct _AP_STA_ENTRY
{
    /** 
     * MAC hash entry.
     * This is used for hash table operations.
     */
    MAC_HASH_ENTRY          MacHashEntry;

    /** 
     * Pointer to the association manager
     * where the station is managed.
     */
    PAP_ASSOC_MGR           AssocMgr;
    
    /** Capability information */
    DOT11_CAPABILITY        CapabilityInformation;

    /** Listen interval */
    USHORT                  ListenInterval;

    /** Supported rates */
    DOT11_RATE_SET          SupportedRateSet;
    
    /** Current association state of the station */
    DOT11_ASSOCIATION_STATE AssocState;

    /** Current association ID */
    USHORT                  Aid;

    /** Power mode */
    DOT11_POWER_MODE        PowerMode;
    
    /** Auth algorithm */
    DOT11_AUTH_ALGORITHM    AuthAlgo;

    /** Unicast cipher algorithm */
    DOT11_CIPHER_ALGORITHM  UnicastCipher;

    /** Multicast cipher algorithm */
    DOT11_CIPHER_ALGORITHM  MulticastCipher;

    /** WPS enabled */
    BOOLEAN                 WpsEnabled;
    
    /** Association timer */
    NDIS_HANDLE             AssocTimer;

    /** Waiting for association request */
    LONG                    WaitingForAssocReq;

    /** 
     * Association up time, i.e. timestamp at which association is completed with success.
     * Timestamp value is returned by NdisGetCurrentSystemTime
     */
    LARGE_INTEGER           AssocUpTime;

    /** Statistics */
    DOT11_PEER_STATISTICS   Statistics;

    /** 
     * Station reference count. 
     * Indicate the number of external functions 
     * that are accessing the station entry.
     * The reference count is 1 when an entry is created.
     * It is deleted when the reference count reaches zero.
     */
    LONG                    RefCount;

#if 0
    /** Buffer for association complete indication */
    PDOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS AssocCompletePara;
    ULONG                   AssocCompleteParaSize;

    /** Received association request */
    PDOT11_MGMT_HEADER      AssocReqFrame;
    USHORT                  AssocReqFrameSize;
#endif
    /** Association decision */
    BOOLEAN                 AcceptAssoc;
    USHORT                  Reason;

    /** 
     * Inactive time. 
     * Indicates how long the station has been inactive, in seconds.
     */
    LONG                    InactiveTime;
    
    /** 
     * Port state
     * This is used to decide whether a non-forced scan
     * shall be allowed or not.
     */
    STA_PORT_STATE          PortState; 
} AP_STA_ENTRY, *PAP_STA_ENTRY;

/**
 * Holds current ExtAP configuration of the miniport that
 * are not managed by association manager. 
 * These configurations can be updated and/or queried via OIDs request from the OS. 
 * A lock is NOT needed when updating/querying these configurations.
 * This data is stateless so we don't need a flag to indicate whether it is initialized or not.
 */


typedef struct _AP_CONFIG 
{
    /** ExtAP port */
    PMP_EXTAP_PORT          ApPort;

    /** The types of auto configuration for 802.11 parameters that are enabled */
    ULONG                   AutoConfigEnabled;
    
    /** Beacon period, in TUs */
    ULONG                   BeaconPeriod;

    /** DTIM period, in beacon intervals */
    ULONG                   DTimPeriod;
#if 0       // remove it after confirmed
    /** RTS threshold */
    ULONG                   RtsThreshold;

    /** Short retry limit */
    ULONG                   ShortRetryLimit;

    /** Long retry limit */
    ULONG                   LongRetryLimit;

    /** Fragmentation threshold */
    ULONG                   FragmentationThreshold;

    /** Current operating frequency channel list for the DSSS/HRDSSS/ERP PHY */
    ULONG                   CurrentChannel;
    
    /** Current operating frequency channel list for the OFDM PHY */
    ULONG                   CurrentFrequency;
    
    /** Current PHY ID */
    ULONG                   CurrentPhyId;
    
#endif
    
    /** Default key ID */
    ULONG                   CipherDefaultKeyId;
    
    /** Desired PHY ID list */
    ULONG                   DesiredPhyList[AP_DESIRED_PHY_MAX_COUNT];
    ULONG                   DesiredPhyCount;

}AP_CONFIG, *PAP_CONFIG;

/** ExtAP port */
typedef struct _MP_EXTAP_PORT
{
    /** parent port */
    PMP_PORT        ParentPort;

    /** ExtAP capability */
    AP_CAPABIITY    Capability;

    /** Registry settings */
    AP_REG_INFO     RegInfo;
    
    /** 
     * AP reference count. 
     * Indicate the number of external functions 
     * that are accessing the AP port.
     * The AP port cannot be terminated
     * until it reaches zero.
     */
    LONG           RefCount;

    /** current AP configuration */
    AP_CONFIG       Config;

    /** association manager */
    AP_ASSOC_MGR    AssocMgr;

    // TODO: statistics
} MP_EXTAP_PORT, *PMP_EXTAP_PORT;

#endif  // _AP_DEFS_H

