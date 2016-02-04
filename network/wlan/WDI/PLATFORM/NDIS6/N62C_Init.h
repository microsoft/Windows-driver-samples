
//Helper Port Reg Init Parameter

/** Maximum number of BSSs we will store in our BSS entries list */
#define MP_BSS_ENTRY_MAX_ENTRIES_DEFAULT       128
#define MP_BSS_ENTRY_MAX_ENTRIES_MIN           16
#define MP_BSS_ENTRY_MAX_ENTRIES_MAX           512

/**
 * Time duration after it was created at which a BSS entry is 
 * considered to have expired
 */
#define MP_BSS_ENTRY_EXPIRE_TIME_DEFAULT       750000000       // 75 seconds
#define MP_BSS_ENTRY_EXPIRE_TIME_MIN           150000000       // 15 seconds
#define MP_BSS_ENTRY_EXPIRE_TIME_MAX          2000000000       // 200 seconds

/**
 * Link quality value (0-100 which if we go below for a sequence of beacons,
 * would suggest that we have a poor signal
 */
#define MP_POOR_LINK_QUALITY_THRESHOLD_DEFAULT   15
#define MP_POOR_LINK_QUALITY_THRESHOLD_MIN       5
#define MP_POOR_LINK_QUALITY_THRESHOLD_MAX       80


/** Maximum number of channels that we scan for when doing a passive scan */
#define MP_SCAN_SET_CHANNEL_COUNT_PASSIVE_DEFAULT      2
#define MP_SCAN_SET_CHANNEL_COUNT_PASSIVE_MIN          1
#define MP_SCAN_SET_CHANNEL_COUNT_PASSIVE_MAX          5

/** Maximum number of channels that we scan for when doing an active scan */
#define MP_SCAN_SET_CHANNEL_COUNT_ACTIVE_DEFAULT       3   // We can scan more channels when in active
#define MP_SCAN_SET_CHANNEL_COUNT_ACTIVE_MIN           1
#define MP_SCAN_SET_CHANNEL_COUNT_ACTIVE_MAX           5


/** Interval between partial scan timer (in milliseconds) */
#define MP_SCAN_RESCHEDULE_TIME_MS_DEFAULT             300
#define MP_SCAN_RESCHEDULE_TIME_MS_MIN                 10
#define MP_SCAN_RESCHEDULE_TIME_MS_MAX                 1000


/** Interval between mutiple scan (in seconds) */
#define MP_INTER_SCAN_TIME_DEFAULT                     60
#define MP_INTER_SCAN_TIME_MIN                         30
#define MP_INTER_SCAN_TIME_MAX                         1000

//STA Port Reg Init Parameter

/** Maximum number of Adhoc stations we will cache state about */
#define STA_ADHOC_STA_MAX_ENTRIES_DEFAULT       64
#define STA_ADHOC_STA_MAX_ENTRIES_MIN           16
#define STA_ADHOC_STA_MAX_ENTRIES_MAX           512

/**
 * Time duration after it was created at which an BSS entry is 
 * considered to have expired
 */
#define STA_BSS_ENTRY_EXPIRE_TIME_DEFAULT       750000000       // 75 seconds
#define STA_BSS_ENTRY_EXPIRE_TIME_MIN           150000000       // 15 seconds
#define STA_BSS_ENTRY_EXPIRE_TIME_MAX          2000000000       // 200 seconds

/** 
 * Number of beacon intervals after which if we havent 
 * received a beacon from the AP we assume we have 
 * lost connectivity
 */
#define STA_INFRA_ROAM_NO_BEACON_COUNT_DEFAULT  20
#define STA_INFRA_ROAM_NO_BEACON_COUNT_MIN      5
#define STA_INFRA_ROAM_NO_BEACON_COUNT_MAX      50

/**
 * Number of beacon for which we have continuously received
 * beacons with RSSI below above threshold, we will roam
 */
#define STA_INFRA_RSSI_ROAM_BEACON_COUNT_DEFAULT        15
#define STA_INFRA_RSSI_ROAM_BEACON_COUNT_MIN            5
#define STA_INFRA_RSSI_ROAM_BEACON_COUNT_MAX            20


//
// Macros for assigning and verifying NDIS_OBJECT_HEADER
//
#define MP_ASSIGN_NDIS_OBJECT_HEADER(_header, _type, _revision, _size) \
    (_header).Type = _type; \
    (_header).Revision = _revision; \
    (_header).Size = _size; 

VOID 
N62CUpdateRegSetting(
	PADAPTER pAdapter
	);

NDIS_STATUS
N62CInitialize(
	IN PADAPTER								pHelperAdapter,	
       IN    NDIS_HANDLE         MiniportAdapterHandle,
    	IN  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
	);

NDIS_STATUS
N62CAllocateExtAdapter(
	IN	PADAPTER		pDefaultAdapter,
	IN	PADAPTER 		*pHelperAdapter,
	IN	NDIS_HANDLE	MiniportAdapterHandle
	);

VOID
N62CFreeExtAdapter(
	IN PADAPTER pExtAdapter
	);

NDIS_STATUS
N62CAllocatePortCommonComponent(
	IN	PADAPTER		pAdapter
);

VOID
N62CFreePortCommonComponent(
	IN	PADAPTER		pAdapter
);

NDIS_STATUS
N62CAllocatePortSpecificComponent(
	IN	PADAPTER		pAdapter
);

VOID
N62CFreePortSpecificComponent(
	IN	PADAPTER		pAdapter
);

NDIS_STATUS
N62CSet80211Attributes(
	IN	PADAPTER		Adapter
	);
/*
NDIS_STATUS
N62CStartMP(
	IN	PADAPTER		pAdapter	
	);
*/

void
N62CInitVariable(
	IN	PADAPTER		pAdapter
	);

NDIS_STATUS
N62CAllocateN62CommResource(
    IN  PADAPTER                pAdapter
    );