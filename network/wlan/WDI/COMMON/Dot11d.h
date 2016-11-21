#ifndef __INC_DOT11D_H
#define __INC_DOT11D_H

// #define	USE_SMART_SCAN

#define	WEIGHTED_SCAN_PERIOD	2

#define DOT11D_MAX_CHNL_NUM 83

typedef struct _CHNL_TXPOWER_TRIPLE {
	u1Byte FirstChnl;
	u1Byte NumChnls;
	s1Byte MaxTxPowerInDbm;
}CHNL_TXPOWER_TRIPLE, *PCHNL_TXPOWER_TRIPLE;

typedef enum _DOT11D_STATE {
	DOT11D_STATE_NONE = 0,
	DOT11D_STATE_LEARNED,
	DOT11D_STATE_DONE,
}DOT11D_STATE;

typedef struct _RT_DOT11D_INFO {
	DECLARE_RT_OBJECT(RT_DOT11D_INFO);

	BOOLEAN bEnabled; // dot11MultiDomainCapabilityEnabled

	u2Byte CountryIeLen; // > 0 if CountryIeBuf[] contains valid country information element.
	u1Byte CountryIeBuf[MAX_IE_LEN];
	u1Byte CountryIeSrcAddr[6]; // Source AP of the country IE.
	u1Byte CountryIeWatchdog; 

	u1Byte ChnlListLen; // #Bytes valid in ChnlList[].
	u1Byte ChnlList[DOT11D_MAX_CHNL_NUM];
	s1Byte MaxTxPwrDbmList[DOT11D_MAX_CHNL_NUM];

	DOT11D_STATE State;
}RT_DOT11D_INFO, *PRT_DOT11D_INFO;

typedef enum _REGULATION_TXPWR_LMT {
	TXPWR_LMT_FCC = 0,
	TXPWR_LMT_MKK = 1,
	TXPWR_LMT_ETSI = 2,
	TXPWR_LMT_WW = 3,	
	
	TXPWR_LMT_MAX_REGULATION_NUM = 4
} REGULATION_TXPWR_LMT;

//
// We now define the following channels as the max channels in each channel plan.
// 2G, total 14 chnls
// {1,2,3,4,5,6,7,8,9,10,11,12,13,14}
// 5G, total 24 chnls
// {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}
#define	MAX_CHANNEL_NUM					38//14+24
#define	MAX_SCAN_CHANNEL_NUM			54

#define	CHANNEL_SCAN_CYCLE_NUM			2

#define	Scan_STA_Large					10
#define	Scan_STA_Middle					1
#define SCAN_PERIOD_Long				200	//ms
#define	SCAN_PERIOD_Short				20 	//ms
#define	SCAN_PERIOD_SHORT_NTGR		35 	//ms
#define	SCAN_PERIOD_LONG_NTGR		110 	//ms
#define	SCAN_PERIOD_LONG_NTGR_XP	50 	//ms

#define	DEFAULT_ACTIVE_SCAN_PERIOD		50 	//ms
#define	DEFAULT_PASSIVE_SCAN_PERIOD		100 // ms
#define SCAN_PERIOD_Long_Win_DTM		50	//ms
#define SCAN_PERIOD_Short_Win_DTM		20	//ms

#define SMART_SCAN_PERIOD_LONG	130 // ms
#define SMART_SCAN_PERIOD_MIDDLE 100 // ms
#define SMART_SCAN_PERIOD_SHORT	50 // ms

#define 	History_Count_Limit					4

// Extended Info about channel
#define	CHANNEL_EXINFO_NO_IBSS_START			BIT1	// Do not start IBSS
#define   CHANNEL_EXINFO_NO_IBSS_JOIN				BIT2	// Do not join IBSS
#define	CHANNEL_EXINFO_WEIGHTED_SCAN_PERIOD	BIT3	// This channel should be scaned with weighted period
#define	CHANNEL_EXINFO_CONNECTED_CHANNEL		BIT4// This channel is connected channel

//
// Channel Plan Type.
// Note: 
//	We just add new channel plan when the new channel plan is different from any of the following 
//	channel plan. 
//	If you just wnat to customize the acitions(scan period or join actions) about one of the channel plan,
//	customize them in RT_CHNL_LIST_ENTRY in the RT_CHANNEL_LIST.
// 
//3 |-----------------------------------------------------------------------|
//3 |<Note> When a new domain code added, check if the order correctly mapped with    
//3 |               DefaultChannelPlan array index.                                                                                    
//3 |-----------------------------------------------------------------------|
typedef enum _RT_CHANNEL_DOMAIN
{
	RT_CHANNEL_DOMAIN_FCC = 0,
	RT_CHANNEL_DOMAIN_IC = 1,
	RT_CHANNEL_DOMAIN_ETSI = 2,
	RT_CHANNEL_DOMAIN_SPAIN = 3,
	RT_CHANNEL_DOMAIN_FRANCE = 4,
	RT_CHANNEL_DOMAIN_MKK = 5,
	RT_CHANNEL_DOMAIN_MKK1 = 6,
	RT_CHANNEL_DOMAIN_ISRAEL = 7,
	RT_CHANNEL_DOMAIN_TELEC = 8,
	RT_CHANNEL_DOMAIN_MIC = 9,				// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_GLOBAL_DOMAIN = 10,		// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_WORLD_WIDE_13 = 11,		// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_TAIWAN = 12,		// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_CHIAN = 13,
	RT_CHANNEL_DOMAIN_SINGAPORE_INDIA_MEXICO  = 14,
	RT_CHANNEL_DOMAIN_KOREA = 15,
	RT_CHANNEL_DOMAIN_TURKEY = 16,
	RT_CHANNEL_DOMAIN_JAPAN = 17,
	RT_CHANNEL_DOMAIN_FCC_NO_DFS = 18,
	RT_CHANNEL_DOMAIN_JAPAN_NO_DFS= 19,
	RT_CHANNEL_DOMAIN_WORLD_WIDE_5G= 20,
	RT_CHANNEL_DOMAIN_TAIWAN_NO_DFS = 21,
	RT_CHANNEL_DOMAIN_DEFAULT = 22,
	RT_CHANNEL_DOMAIN_OMNIPEEK_ALL_CHANNEL = 23,
	RT_CHANNEL_DOMAIN_WHQL = 24,	
	
	//4 <New Definitions (The order below should not be changed!) >
	// <20120516, Kordan> The naming is according to the specification of channel plan on Mantis #11235.
	// (The legacy definition above is still reserved to be compatible with legacy EFUSE maps.)
	RT_CHANNEL_DOMAIN_WW13_2G             ,			// 25
	RT_CHANNEL_DOMAIN_EUROPE_2G            ,		// 26
	RT_CHANNEL_DOMAIN_US_2G                ,		// 27
	RT_CHANNEL_DOMAIN_JAPAN_2G             ,		// 28
	RT_CHANNEL_DOMAIN_FRANCE_2G            ,		// 29
	RT_CHANNEL_DOMAIN_US_2G_5G             ,		// 30
	RT_CHANNEL_DOMAIN_WW13_2G_EUROPE_5G    ,		// 31
	RT_CHANNEL_DOMAIN_JAPAN_2G_5G          ,		// 32
	RT_CHANNEL_DOMAIN_WW13_2G_KOREA_5G     ,		// 33
	RT_CHANNEL_DOMAIN_WW13_2G_US_5G        ,		// 34
	RT_CHANNEL_DOMAIN_WW13_2G_INDIA_5G     ,		// 35
	RT_CHANNEL_DOMAIN_WW13_2G_VENEZUELA_5G ,		// 36
	RT_CHANNEL_DOMAIN_WW13_2G_HONDURA_5G     ,		// 37
	RT_CHANNEL_DOMAIN_WW13_2G_ISRAEL_5G    ,		// 38
	RT_CHANNEL_DOMAIN_US_2G_CANADA_5G      ,		// 39
	RT_CHANNEL_DOMAIN_WW13_2G_AUSTRALIA_5G ,		// 40
	RT_CHANNEL_DOMAIN_WW13_2G_RUSSIA_5G    ,		// 41
	RT_CHANNEL_DOMAIN_JAPAN_2G_W52_W53_5G  ,		// 42
	RT_CHANNEL_DOMAIN_JAPAN_2G_W56_5G      ,		// 43
	RT_CHANNEL_DOMAIN_US_2G_TAIWAN_5G      ,		// 44
	RT_CHANNEL_DOMAIN_US_2G_TAIWAN_DFS_5G  , 		// 45
	RT_CHANNEL_DOMAIN_GLOBAL_DOMAIN_2G  , 			// 46
	RT_CHANNEL_DOMAIN_2G_ETSI1_5G_ETSI4, 			// 47
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC2  , 			// 48
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_NCC3  , 			// 49
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI5  , 			// 50
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC8  , 			// 51
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI6  , 			// 52
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI7  , 			// 53
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI8  , 			// 54
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI9  , 			// 55
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI10  ,			// 56
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI11  ,			// 57
	RT_CHANNEL_DOMAIN_WW13_2G_5G_NCC4  ,			// 58
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI12,			// 59
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC9,                      // 60
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI13,                // 61
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC10,                // 62
	RT_CHANNEL_DOMAIN_2G_MKK2_5G_MKK4,                // 63
	RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI14,                // 64
	
	
	//4 <----------------End of New Definitions---------------->
	
	//===== Add new channel plan above this line ===============//
#if 0
	// For new architecture we define different 2G/5G CH area for all country.
	// 2.4 G only
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_NULL				= 0x20,
	RT_CHANNEL_DOMAIN_2G_ETSI1_5G_NULL				= 0x21,
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_NULL				= 0x22,
	RT_CHANNEL_DOMAIN_2G_MKK1_5G_NULL				= 0x23,
	RT_CHANNEL_DOMAIN_2G_ETSI2_5G_NULL				= 0x24,
	// 2.4 G + 5G type 1
	RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC1				= 0x25,
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_ETSI1				= 0x26,
	RT_CHANNEL_DOMAIN_2G_WORLD_5G_ETSI1				= 0x27,
	// .....
#endif	

	RT_CHANNEL_DOMAIN_WHQL_WFD,	

	RT_CHANNEL_DOMAIN_UNDEFINED,
	RT_CHANNEL_DOMAIN_MAX,
}RT_CHANNEL_DOMAIN, *PRT_CHANNEL_DOMAIN;

typedef struct _RT_CHANNEL_PLAN
{
	u1Byte	Channel[MAX_CHANNEL_NUM];
	u1Byte	Len;	
	u1Byte	Channel2_4G[MAX_CHANNEL_NUM];
	u1Byte	Len2_4G;
	u1Byte	Channel5G[MAX_CHANNEL_NUM];
	u1Byte	Len5G;

}RT_CHANNEL_PLAN, *PRT_CHANNEL_PLAN;


// Scan type including active and passive scan.
typedef enum _RT_SCAN_TYPE
{
	SCAN_ACTIVE,
	SCAN_PASSIVE,
	SCAN_MIX,
}RT_SCAN_TYPE, *PRT_SCAN_TYPE;

//
// The action codes are intercommunicated by the interface RtActChannelList().
//
typedef enum _RT_CHNL_LIST_ACTION
{
	RT_CHNL_LIST_ACTION_INIT = 0,				// Init the channel list.
	RT_CHNL_LIST_ACTION_CONSTRUCT,				// (Re-)Construct the channel list
	RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST,		// Get the channel list
	RT_CHNL_LIST_ACTION_UPDATE_CHANNEL,			// Update Current Channel Info
	RT_CHNL_LIST_ACTION_ADD_CHANNEL,			// Add a new channel info into the channel list.
	RT_CHNL_LIST_ACTION_DEL_CHANNEL,			// Delete a channel info in the channel list.
	RT_CHNL_LIST_ACTION_GET_CHANNEL,			// Get the channel info in the channel list.
	RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST,	// Construct channel list
	RT_CHNL_LIST_ACTION_POP_SCAN_CHANNEL,		// Remove and get the first scan entry from the scan list
	RT_CHNL_LIST_ACTION_GET_SCAN_CHANNEL,		// Get the first scan entry from the scan list
	RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_PERIOD,// Construct channel scan period
	RT_CHNL_LIST_ACTION_GET_CHANNEL_PLAN,		// Get channel plan
}RT_CHNL_LIST_ACTION, *PRT_CHNL_LIST_ACTION;


// The channel information about this channel including joining, scanning, and power constraints.
typedef struct _RT_CHNL_LIST_ENTRY
{
	u1Byte			ChannelNum;		// The channel number.
	RT_SCAN_TYPE	ScanType;		// Scan type such as passive or active scan.
	u2Byte			ScanPeriod;		// Listen time in millisecond in this channel.
	s4Byte			MaxTxPwrDbm;	// Max allowed tx power.
	u4Byte			ExInfo;			// Extended Information for this channel.
}RT_CHNL_LIST_ENTRY, *PRT_CHNL_LIST_ENTRY;

//
// The current channel plan and the scan list.
//
typedef struct _RT_CHANNEL_LIST
{
	DECLARE_RT_OBJECT(RT_CHANNEL_LIST);
	BOOLEAN				bDot11d;						// True if this channel list supports dot11d. 
	u1Byte				ChannelLen;						// Availabe channel number.
	RT_CHANNEL_DOMAIN	ChannelPlan;					// Current channel plan for this channel list.
	u2Byte				WirelessMode;					// Record the wireless mode if this channel list is conformed to the current wireless mode.
	RT_CHNL_LIST_ENTRY		ChnlListEntry[MAX_CHANNEL_NUM];	// Info about each channel
	u1Byte				ScanChannelIndex;				// zero-based
	u1Byte				ScanChannelListLength;			// Current scan channel list length
	PRT_CHNL_LIST_ENTRY	ScanChannelList[MAX_SCAN_CHANNEL_NUM]; // Current scan channels.
	BOOLEAN				bValidSTAStatis;				//  TRUE is the statistics of the # of STAs from scan is valid.
	u1Byte				EachChannelSTAs[MAX_SCAN_CHANNEL_NUM]; // Current scan period.
}RT_CHANNEL_LIST, *PRT_CHANNEL_LIST;

#define GET_DOT11D_INFO(__pMgntInfo) ((PRT_DOT11D_INFO)((__pMgntInfo)->pDot11dInfo))

#define IS_DOT11D_ENABLE(__pMgntInfo) GET_DOT11D_INFO(__pMgntInfo)->bEnabled
#define IS_COUNTRY_IE_VALID(__pMgntInfo) (	(GET_DOT11D_INFO(__pMgntInfo)->CountryIeLen > 0) && \
											(IS_DOT11D_STATE_DONE(__pMgntInfo)))

#define IS_EQUAL_CIE_SRC(__pMgntInfo, __pTa) eqMacAddr(GET_DOT11D_INFO(__pMgntInfo)->CountryIeSrcAddr, __pTa) 
#define UPDATE_CIE_SRC(__pMgntInfo, __pTa) cpMacAddr(GET_DOT11D_INFO(__pMgntInfo)->CountryIeSrcAddr, __pTa)

#define IS_COUNTRY_IE_CHANGED(__pMgntInfo, __Ie) \
	(((__Ie).Length == 0 || (__Ie).Length != GET_DOT11D_INFO(__pMgntInfo)->CountryIeLen) ? \
	TRUE : \
	(PlatformCompareMemory(GET_DOT11D_INFO(__pMgntInfo)->CountryIeBuf, (__Ie).Octet, (__Ie).Length)))

#define CIE_WATCHDOG_TH 1
#define GET_CIE_WATCHDOG(__pMgntInfo) GET_DOT11D_INFO(__pMgntInfo)->CountryIeWatchdog
#define RESET_CIE_WATCHDOG(__pMgntInfo) GET_CIE_WATCHDOG(__pMgntInfo) = 0 
#define UPDATE_CIE_WATCHDOG(__pMgntInfo) ++GET_CIE_WATCHDOG(__pMgntInfo)

#define IS_DOT11D_STATE_DONE(__pMgntInfo) (GET_DOT11D_INFO(__pMgntInfo)->State == DOT11D_STATE_DONE)

#define	GET_RT_CHANNEL_LIST(__pMgntInfo) ((PRT_CHANNEL_LIST)((__pMgntInfo)->pChannelList))
#define IS_CHANNEL_LIST_VALID(__pMgntInfo) \
		((GET_RT_CHANNEL_LIST(__pMgntInfo)->ChannelLen > 0) && \
		(GET_RT_CHANNEL_LIST(__pMgntInfo)->WirelessMode == (__pMgntInfo)->dot11CurrentWirelessMode) && \
		(GET_RT_CHANNEL_LIST(__pMgntInfo)->ChannelPlan == (__pMgntInfo)->ChannelPlan))



VOID
Dot11d_Reset(
	IN PADAPTER		pAdapter
	);

VOID
Dot11d_UpdateCountryIe(
	IN PADAPTER		pAdapter,
	IN pu1Byte		pTaddr,
	IN POCTET_STRING	posCoutryIe	 
	);

u1Byte
Dot11d_GetChannelList(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte*		ppChnlList
	);

s4Byte
DOT11D_GetMaxTxPwrInDbm(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Channel
	);

VOID
DOT11D_ScanComplete(
	IN	PADAPTER		Adapter
	);

BOOLEAN
RtActChannelList(
	IN		PADAPTER	pAdapter,
	IN		u4Byte		ActionCode,
	IN		PVOID		pInputBuf,
	OUT		PVOID		pOutputBuf
	);

u1Byte 
RtGetDualBandChannel(
		IN	PADAPTER	Adapter,
		OUT		PRT_CHNL_LIST_ENTRY	ChnlListEntryArray
);
#endif // #ifndef __INC_DOT11D_H
