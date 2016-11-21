//-----------------------------------------------------------------------------
//	File:
//		Dot11d.c
//
//	Description:
//		Implement 802.11d. 
//
//-----------------------------------------------------------------------------

#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Dot11d.tmh"
#endif

/*------------------------ Private Funtion Declaration-------------------------------------*/
// The following function are called only in Dot11d.c
#define	RtResetScanChannel(__pChannelList)			\
{													\
	(__pChannelList)->ScanChannelListLength = 0;	\
	(__pChannelList)->ScanChannelIndex = 0;			\
}

#define	RtInsertScanChannel(__pChannelList, __pChnlListEntry)								\
{																						\
	if((__pChannelList)->ScanChannelListLength < MAX_SCAN_CHANNEL_NUM)					\
	{																					\
		(__pChannelList)->ScanChannelList[(__pChannelList)->ScanChannelListLength ++] =	\
			(PRT_CHNL_LIST_ENTRY)(__pChnlListEntry);											\
	}																					\
}

#define	RtGetScanChannel(__pChannelList)												\
	(((__pChannelList)->ScanChannelIndex >= (__pChannelList)->ScanChannelListLength) ?	\
		NULL : (__pChannelList)->ScanChannelList[(__pChannelList)->ScanChannelIndex])

#define	RtPopScanChannel(__pChannelList)												\
	(((__pChannelList)->ScanChannelIndex >= (__pChannelList)->ScanChannelListLength) ?	\
		NULL : (__pChannelList)->ScanChannelList[(__pChannelList)->ScanChannelIndex ++])

	
#define	RtResetChannelList(__pChannelList)															\
{																									\
	(__pChannelList)->ChannelPlan = RT_CHANNEL_DOMAIN_MAX;											\
	(__pChannelList)->bDot11d = FALSE;																\
	(__pChannelList)->ChannelLen = 0;																\
	(__pChannelList)->WirelessMode = WIRELESS_MODE_UNKNOWN;											\
	PlatformZeroMemory((__pChannelList)->ChnlListEntry, sizeof(RT_CHNL_LIST_ENTRY) * MAX_CHANNEL_NUM);	\
	RtResetScanChannel(__pChannelList);																\
}
/*------------------------ End of Private Funtion Declaration------------------------------*/


static u1Byte s_MaxChnlList[] = {
	1,2,3,4,5,6,7,8,9,10,11,12,13,14, 
	36,40,44,48,52,56,60,64,149,153,157,161,165
};


#define DOMAIN_CODE_2G_WORLD \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define DOMAIN_CODE_2G_ETSI1 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define DOMAIN_CODE_2G_ETSI2 \
		{1,2,3,4,5,6,7,8,9,10,11,12,13,14}, 14		
#define DOMAIN_CODE_2G_FCC1 \
        {1,2,3,4,5,6,7,8,9,10,11}, 11
#define DOMAIN_CODE_2G_MKK1 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13,14}, 14
#define DOMAIN_CODE_2G_MKK2 \
        {1,2,3,4,5,6,7,8,9,10,11,12,13}, 13
#define DOMAIN_CODE_2G_GLOBAL \
        {1,2,3,4,5,6,7,8,9,10,11,12,13,14}, 14
#define DOMAIN_CODE_2G_WHQL_WFD \
		{1,4,6,8,11}, 5

#define DOMAIN_CODE_5G_ETSI1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define DOMAIN_CODE_5G_ETSI2 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define DOMAIN_CODE_5G_ETSI3 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,149,153,157,161,165}, 22
#define DOMAIN_CODE_5G_ETSI4 \
        {36,40,44,48}, 4
#define DOMAIN_CODE_5G_ETSI5 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165}, 21
#define DOMAIN_CODE_5G_ETSI6 \
        {36,40,44,48,52,56,60,64}, 8
#define DOMAIN_CODE_5G_ETSI7 \
        {36,40,44,48,52,56,60,64,149,153,157,161,165}, 13
#define DOMAIN_CODE_5G_ETSI8 \
        {149,153,157,161}, 4
#define DOMAIN_CODE_5G_ETSI9 \
        {100,104,108,112,116,132,136,140}, 8
#define DOMAIN_CODE_5G_ETSI10 \
        {149,153,157,161,165}, 5
#define DOMAIN_CODE_5G_ETSI11 \
        {36,40,44,48,52,56,60,64,132,136,140,149,153,157,161,165}, 16
#define DOMAIN_CODE_5G_ETSI12 \
        {149,153,157,161}, 4
#define DOMAIN_CODE_5G_ETSI13 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140}, 16
#define DOMAIN_CODE_5G_ETSI14 \
        {36,40,44,48,52,56,60,64,132,136,140}, 11

#define DOMAIN_CODE_5G_FCC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}, 24
#define DOMAIN_CODE_5G_FCC2 \
        {36,40,44,48,149,153,157,161,165}, 9
#define DOMAIN_CODE_5G_FCC3 \
        {36,40,44,48,52,56,60,64,149,153,157,161,165}, 13
#define DOMAIN_CODE_5G_FCC4 \
        {36,40,44,48,52,56,60,64,149,153,157,161}, 12
#define DOMAIN_CODE_5G_FCC5 \
        {149,153,157,161,165}, 5
#define DOMAIN_CODE_5G_FCC6 \
        {36,40,44,48,52,56,60,64}, 8
#define DOMAIN_CODE_5G_FCC7 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165}, 21
#define DOMAIN_CODE_5G_FCC8 \
        {149,153,157,161}, 4
#define DOMAIN_CODE_5G_FCC9 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165}, 21
#define DOMAIN_CODE_5G_FCC10 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161}, 20
			
#define DOMAIN_CODE_5G_IC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165}, 20

#define DOMAIN_CODE_5G_KCC1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161}, 19

#define DOMAIN_CODE_5G_MKK1 \
        {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}, 19
#define DOMAIN_CODE_5G_MKK2 \
        {36,40,44,48,52,56,60,64}, 8
#define DOMAIN_CODE_5G_MKK3 \
        {100,104,108,112,116,120,124,128,132,136,140}, 11
#define DOMAIN_CODE_5G_MKK4 \
        {36,40,44,48}, 4

#define DOMAIN_CODE_5G_NCC1 \
        {56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165}, 16
#define DOMAIN_CODE_5G_NCC2 \
        {56,60,64,149,153,157,161,165}, 8  			
#define DOMAIN_CODE_5G_NCC3 \
        {149,153,157,161,165}, 5
#define DOMAIN_CODE_5G_NCC4 \
        {52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165}, 17
#define DOMAIN_CODE_5G_WHQL_WFD \
		{36,40,44,48,149,157,165}, 7
#define UNDEFINED \
        {0}, 0

//
// Note:
// 	(1) I merge Default Channel Plan to here because I think channel plans are global defintions
//	which shall not depend on ICs or customized requests (ex. scan period, passive scan,
//	join actions, etc.).  If the channel plans depend on the ICs, we should take care of the mp kits
//	for all ICs. And we take a large risk to maintain different channel plans for differnet ICs.
//	(2) We shall not add/modify the channel plans excpet when the channels in the channel plan
//	are different from the existing channel plan.
//	(3) All extra infomation about the channel plan for customized requests should not put into the
//	definitions of the DefaultChannelPlan, and instead, keep the information in the 
//	RT_CHNL_LIST_ENTRY when we construct the RT_CHANNEL_LIST. To compatible with the
//	past customized requests, I merge old customized channel plan in the RtConstructChannelList().
//	By Bruce, 2008-09-01.
//


static RT_CHANNEL_PLAN	DefaultChannelPlan[RT_CHANNEL_DOMAIN_MAX] = {
#if (DFS_TEST_CHNL_PLAN == 1)
	// The channel plan is for DFS test only.
	//4 #00 EFUSE_00h: RT_CHANNEL_DOMAIN_FCC
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,100,104,124,128,149,153,157,161,165} ,28, UNDEFINED, UNDEFINED},	
#else							
	//4 #00 EFUSE_00h: RT_CHANNEL_DOMAIN_FCC
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165} ,35, UNDEFINED, UNDEFINED},	
#endif
	//4 #01 EFUSE_01h: RT_CHANNEL_DOMAIN_IC
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165} , 31, UNDEFINED, UNDEFINED},			
		
	//4 #02 EFUSE_02h: RT_CHANNEL_DOMAIN_ETSI
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,100,104,108,112,116,120,124,128,132,136,140} , 31, UNDEFINED, UNDEFINED},				
		
	//4 #03 EFUSE_03h: RT_CHANNEL_DOMAIN_SPAIN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13} , 13, UNDEFINED, UNDEFINED},										
		
	//4 #04 EFUSE_04h: RT_CHANNEL_DOMAIN_FRANCE
	{{1,2,3,4,5,6,7,8,9,10,11,12,13} , 13, UNDEFINED, UNDEFINED},										
		
	//4 #05 EFUSE_05h: RT_CHANNEL_DOMAIN_MKK
	{{1,2,3,4,5,6,7,8,9,10,11,12,13} , 13, UNDEFINED, UNDEFINED},			
		
	//4 #06 EFUSE_06h: RT_CHANNEL_DOMAIN_MKK1
	{{1,2,3,4,5,6,7,8,9,10,11,12,13} , 13, UNDEFINED, UNDEFINED},			
		
	//4 #07 EFUSE_07h: RT_CHANNEL_DOMAIN_ISRAEL
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64} , 21, UNDEFINED, UNDEFINED},										
		
	//4 #08 EFUSE_08h: RT_CHANNEL_DOMAIN_TELEC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64} , 22, UNDEFINED, UNDEFINED},			
		
	//4 #09 EFUSE_09h: RT_CHANNEL_DOMAIN_MIC
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64} , 21, UNDEFINED, UNDEFINED},				
		
	//4 #10 EFUSE_0Ah: RT_CHANNEL_DOMAIN_GLOBAL_DOMAIN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14} , 14, UNDEFINED, UNDEFINED},									
		
	//4 #11 EFUSE_0Bh: RT_CHANNEL_DOMAIN_WORLD_WIDE_13
	{{1,2,3,4,5,6,7,8,9,10,11,12,13} , 13, UNDEFINED, UNDEFINED},										
		
	//4 #12 EFUSE_0Ch: RT_CHANNEL_DOMAIN_TAIWAN
	{{1,2,3,4,5,6,7,8,9,10,11,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165} , 26, UNDEFINED, UNDEFINED},				
		
	//4 #13 EFUSE_0Dh: RT_CHANNEL_DOMAIN_CHIAN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,149,153,157,161,165} , 18, UNDEFINED, UNDEFINED},	
		
	//4 #14 EFUSE_0Eh: RT_CHANNEL_DOMAIN_SINGAPORE_INDIA_MEXICO
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64,149,153,157,161,165} , 24, UNDEFINED, UNDEFINED},	
		
	//4 #15 EFUSE_0Fh: RT_CHANNEL_DOMAIN_KOREA
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,149,153,157,161,165} , 33, UNDEFINED, UNDEFINED},				
		
	//4 #16 EFUSE_10h: RT_CHANNEL_DOMAIN_TURKEY
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,52,56,60,64} , 19, UNDEFINED, UNDEFINED},	
		
	//4 #17 EFUSE_11h: RT_CHANNEL_DOMAIN_JAPAN
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140} , 32, UNDEFINED, UNDEFINED},			
		
	//4 #18 EFUSE_12h: FCC_No_DFS
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,149,153,157,161,165} , 20, UNDEFINED, UNDEFINED},				
		
	//4 #19 EFUSE_13h: Japan_No_DFS
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48} , 17, UNDEFINED, UNDEFINED},	
		
	//4 #20 EFUSE_14h: WORLD_WIDE_5G	// RT_CHANNEL_DOMAIN_WORLD_WIDE_5G
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165} , 37, UNDEFINED, UNDEFINED},

	//4 #21 Taiwan_No_DFS	// RT_CHANNEL_DOMAIN_TAIWAN_NO_DFS
	{{1,2,3,4,5,6,7,8,9,10,11,56,60,64,149,153,157,161,165},19, UNDEFINED, UNDEFINED},	
		
	//4 #22 EFUSE_16h Default 7F: RealtekReserved	// RT_CHANNEL_DOMAIN_DEFAULT
	// 2013/01/29 MH According to new definition from RF team, RTK default channel plan will support all CH.
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165},37, UNDEFINED, UNDEFINED},	
	
	//4 #23 EFUSE_17h OmniPeek-All-Channel (RT_CHANNEL_DOMAIN_OMNIPEEK_ALL_CHANNEL): 2.4G and 5G now
	{{1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165},38, UNDEFINED, UNDEFINED},	

	//4 #24 RT_CHANNEL_DOMAIN_WHQL
	{{1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,149,157,165} , 18, UNDEFINED, UNDEFINED},	

//1 ============= New definition supports 2.4G and 5G =============

	// #25 EFUSE_20h: RT_CHANNEL_DOMAIN_WW13_2G 2.4G World Wide 13
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  UNDEFINED},
		
	// #26 EFUSE_21h: RT_CHANNEL_DOMAIN_EUROPE_2G 2.4G Europe 
	{UNDEFINED, DOMAIN_CODE_2G_ETSI1,  UNDEFINED},
		
	// #27 EFUSE_22h: RT_CHANNEL_DOMAIN_US_2G 2.4G US
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   UNDEFINED},
		
	// #28 EFUSE_23h: RT_CHANNEL_DOMAIN_JAPAN_2G 2.4G Japan
	{UNDEFINED, DOMAIN_CODE_2G_MKK1,   UNDEFINED},
		
	// #29 EFUSE_24h: RT_CHANNEL_DOMAIN_FRANCE_2G 2.4G France
	{UNDEFINED, DOMAIN_CODE_2G_ETSI2,  UNDEFINED},
		
	// #30 EFUSE_25h: RT_CHANNEL_DOMAIN_US_2G_5G 2.4G/5G US
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_FCC1},
		
	// #31 EFUSE_26h: RT_CHANNEL_DOMAIN_WW13_2G_EUROPE_5G 2.4G World Wide 13 + 5G Europe
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_ETSI1},
		
	// #32 EFUSE_27h: RT_CHANNEL_DOMAIN_JAPAN_2G_5G 2.4G/5G Japan 
	{UNDEFINED, DOMAIN_CODE_2G_MKK1,   DOMAIN_CODE_5G_MKK1},
		
	// #33 EFUSE_28h: RT_CHANNEL_DOMAIN_WW13_2G_KOREA_5G 2.4G World Wide 13 + 5G Korea
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_KCC1},
		
	// #34 EFUSE_29h: RT_CHANNEL_DOMAIN_WW13_2G_US_5G 2.4G World Wide 13 + 5G US/DFS Channels
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_FCC2},
		
	// #35 EFUSE_30h: RT_CHANNEL_DOMAIN_WW13_2G_INDIA_5G 2.4G World Wide 13 + 5G India/Mexico            
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_FCC3},
		
	// #36 EFUSE_31h: RT_CHANNEL_DOMAIN_WW13_2G_VENEZUELA_5G 2.4G World Wide 13 + 5G Venezuela               
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_FCC4},
		
	// #37 EFUSE_32h: RT_CHANNEL_DOMAIN_WW13_2G_CHINA_5G 2.4G World Wide 13 + 5G China                   
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_FCC5},
		
	// #38 EFUSE_33h: RT_CHANNEL_DOMAIN_WW13_2G_ISRAEL_5G 2.4G World Wide 13 + 5G Israel                  
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_FCC6},
		
	// #39 EFUSE_34h: RT_CHANNEL_DOMAIN_US_2G_CANADA_5G 2.4G US + 5G US/Canada
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_FCC7},
		
	// #40 EFUSE_35h: RT_CHANNEL_DOMAIN_WW13_2G_AUSTRALIA_5G 2.4G World Wide 13 + 5G Australia/New Zealand
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_ETSI2},
		
	// #41 EFUSE_36h: RT_CHANNEL_DOMAIN_WW13_2G_RUSSIA_5G 2.4G World Wide 13 + 5G Russia
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,  DOMAIN_CODE_5G_ETSI3},
		
	// #42 EFUSE_37h: RT_CHANNEL_DOMAIN_JAPAN_2G_W52_W53_5G 2.4G Japan + 5G Japan(W52, W53)
	{UNDEFINED, DOMAIN_CODE_2G_MKK1,   DOMAIN_CODE_5G_MKK2},
		
	// #43 EFUSE_38h: RT_CHANNEL_DOMAIN_JAPAN_2G_W56_5G
	{UNDEFINED, DOMAIN_CODE_2G_MKK1,   DOMAIN_CODE_5G_MKK3},
		
	// #44 EFUSE_39h: RT_CHANNEL_DOMAIN_US_2G_TAIWAN_5G
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_NCC1},
		
	// #45 EFUSE_40h: RT_CHANNEL_DOMAIN_US_2G_TAIWAN_DFS_5G
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_NCC2},     		

	// #46 EFUSE_41h: RT_CHANNEL_DOMAIN_GLOBAL_DOMAIN_2G
	{UNDEFINED, DOMAIN_CODE_2G_GLOBAL,   UNDEFINED},     		

	// #47 EFUSE_42h: RT_CHANNEL_DOMAIN_2G_ETSI1_5G_ETSI4
	{UNDEFINED, DOMAIN_CODE_2G_ETSI1,   DOMAIN_CODE_5G_ETSI4},     		

	// #48 EFUSE_43h: RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC2
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_FCC2},     		

	// #49 EFUSE_44h: RT_CHANNEL_DOMAIN_2G_FCC1_5G_NCC3
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_NCC3},     		

	// #50 EFUSE_45h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI5
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI5},     		

	// #51 EFUSE_46h: RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC8
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_FCC8},     		

	// #52 EFUSE_47h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI6
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI6},     		

	// #53 EFUSE_48h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI7
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI7},

	// #54 EFUSE_49h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI8
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI8},

	// #55 EFUSE_50h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI9
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI9},

	// #56 EFUSE_51h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI10
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI10},

	// #57 EFUSE_52h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI11
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI11},

	// #58 EFUSE_53h: RT_CHANNEL_DOMAIN_WW13_2G_5G_NCC4
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_NCC4},

	// #59 EFUSE_54h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI12
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI12},

	// #60 EFUSE_55h: RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC9
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_FCC9},
	
	// #61 EFUSE_56h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI13
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI13},

	// #62 EFUSE_57h: RT_CHANNEL_DOMAIN_2G_FCC1_5G_FCC10
	{UNDEFINED, DOMAIN_CODE_2G_FCC1,   DOMAIN_CODE_5G_FCC10},
	
	// #63 EFUSE_58h: RT_CHANNEL_DOMAIN_2G_MKK2_5G_MKK4
	{UNDEFINED, DOMAIN_CODE_2G_MKK2,   DOMAIN_CODE_5G_MKK4},
	
	// #64 EFUSE_59h: RT_CHANNEL_DOMAIN_WW13_2G_5G_ETSI14
	{UNDEFINED, DOMAIN_CODE_2G_WORLD,   DOMAIN_CODE_5G_ETSI14},
	
	//1 ============= End of customer definition  =============

	//4  RT_CHANNEL_DOMAIN_WHQL_WFD
	{UNDEFINED, DOMAIN_CODE_2G_WHQL_WFD,   DOMAIN_CODE_5G_WHQL_WFD},

//1 ============= End of New definition  =============
};          

/*------------------------Private of Protected Funtion Declaration-------------------------------------*/
// The following function are called only in Dot11d.c
BOOLEAN
RtConstructChannelList(
	IN	PADAPTER	pAdapter
	);
PRT_CHNL_LIST_ENTRY
RtGetChnlListEntry(
	IN		PADAPTER			pAdapter,
	IN		u1Byte				Channel
	);

BOOLEAN
RtConstructScanList(
	IN	PADAPTER	pAdapter
	);

VOID
RtCalcScanStatistic(
	IN	PADAPTER	pAdapter
	);

VOID
RtCalcScanPeriod(
	IN	PADAPTER	pAdapter
	);

#if DBG
VOID
DumpChannelList(
	IN	PADAPTER	pAdapter
	);

VOID
DumpScanChannelList(
	IN	PADAPTER	pAdapter
	);
#else
	#define	DumpChannelList(_pAdapter)
	#define	DumpScanChannelList(_pAdapter)
#endif
/*------------------------ End of Private of Protected Funtion Declaration------------------------------*/

//------------------------------------------------------------------------------
// Debug routines. 
//------------------------------------------------------------------------------
#if DBG
VOID
DumpChannelList(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
	u1Byte				i;

	FunctionIn(COMP_SCAN);

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("Channel Plan: ChannelLen %d\n", pChannelList->ChannelLen));
	for(i = 0; i < pChannelList->ChannelLen; i ++)
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("%d ", pChannelList->ChnlListEntry[i].ChannelNum));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));

	for(i = 0; i < pChannelList->ChannelLen; i ++)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("##Channel: %d\n", pChannelList->ChnlListEntry[i].ChannelNum));
		RT_TRACE(COMP_SCAN, DBG_TRACE, ("##ScanType: %d\n", pChannelList->ChnlListEntry[i].ScanType));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("##ScanPeriod: %d\n", pChannelList->ChnlListEntry[i].ScanPeriod));
		RT_TRACE(COMP_SCAN, DBG_TRACE, ("##MaxTxPwrDbm: %d\n", pChannelList->ChnlListEntry[i].MaxTxPwrDbm));
		RT_TRACE(COMP_SCAN, DBG_TRACE, ("##ExInfo: %X\n", pChannelList->ChnlListEntry[i].ExInfo));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));
	}
	RT_TRACE(COMP_SCAN, DBG_TRACE, ("\n"));

	FunctionOut(COMP_SCAN);
	
}

VOID
DumpScanChannelList(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
	u1Byte				i;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan List: (len = %d) ", pChannelList->ScanChannelListLength));
	for(i = 0; i < pChannelList->ScanChannelListLength; i ++)
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("%d ", pChannelList->ScanChannelList[i]->ChannelNum));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));
}

#endif

// <20140311, Kordan> Knuth Shuffle Algorithm
// Please refer to http://rosettacode.org/wiki/Knuth_shuffle#C

void _shuffle(PRT_CHNL_LIST_ENTRY *list, u1Byte len) {		
	u4Byte j;						
	PRT_CHNL_LIST_ENTRY tmp;				
	
	while(len) {					
		j = (u4Byte)(PlatformGetCurrentTime()%len);				
		if (j != len - 1) {			
			tmp = list[j];			
			list[j] = list[len - 1];
			list[len - 1] = tmp;	
		}					
		len--;
	}						
}	

VOID
shuffleScanChannelList(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);

	if(pChannelList == NULL)
		return;
	else if(pChannelList->ScanChannelListLength == 0)
		return;

	_shuffle(pChannelList->ScanChannelList, pChannelList->ScanChannelListLength-1);
}


//
//	Description:
//		Reset to the state as we are just entering a regulatory domain.
//
VOID
Dot11d_Reset(
	IN PADAPTER		pAdapter
	)
{
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);

	if(!pDot11dInfo->bEnabled)
		return;

	pDot11dInfo->State = DOT11D_STATE_NONE;
	pDot11dInfo->CountryIeLen = 0;
	pDot11dInfo->ChnlListLen = 0;
	RESET_CIE_WATCHDOG(pMgntInfo);

	RtResetChannelList(GET_RT_CHANNEL_LIST(pMgntInfo));

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("Dot11d_Reset()\n"));
	AddDrvLog(pAdapter, LID_DOT11D_RESET, NULL, 0);
}

//
//	Description:
//		Update country IE from Beacon or Probe Resopnse 
//		and configure PHY for operation in the regulatory domain.
//
//	TODO: 
//		Configure Tx power.
//
//	Assumption:
//		1. IS_DOT11D_ENABLE() is TRUE.
//		2. Input IE is an valid one.
//
VOID
Dot11d_UpdateCountryIe(
	IN PADAPTER		pAdapter,
	IN pu1Byte		pTaddr,
	IN POCTET_STRING	posCoutryIe	 
	)
{
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	u1Byte i, j, NumTriples, MaxChnlNum, ChnlListLen;
	PCHNL_TXPOWER_TRIPLE pTriple;

	MaxChnlNum = 0;
	ChnlListLen = 0;
	NumTriples = (posCoutryIe->Length - 3) / 3; // skip 3-byte country string.
	pTriple = (PCHNL_TXPOWER_TRIPLE)(posCoutryIe->Octet + 3);

	if(posCoutryIe->Length > MAX_IE_LEN)
		return;	
		
	for(i = 0; i < NumTriples; i++)
	{
		if(MaxChnlNum >= pTriple->FirstChnl)
		{ // It is not in a monotonically increasing order, so stop processing.
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("Dot11d_UpdateCountryIe(): Invalid country IE, skip it........\n"));
			return; 
		}

		if((pTriple->NumChnls+ChnlListLen) > DOT11D_MAX_CHNL_NUM)
		{// To prevent malicious attack. Added by Roger, 2010.02.24.
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("Dot11d_UpdateCountryIe(): Invalid channel list length, skip it........\n"));
			return; 
		}

		for(j = 0 ; j < pTriple->NumChnls; j++)
		{
			pDot11dInfo->ChnlList[ChnlListLen] = (pTriple->FirstChnl + j);
			pDot11dInfo->MaxTxPwrDbmList[ChnlListLen] = pTriple->MaxTxPowerInDbm;
			MaxChnlNum = pDot11dInfo->ChnlList[ChnlListLen];// Keep tracking maximal channel number.
			ChnlListLen++;
		}	
		pTriple = (PCHNL_TXPOWER_TRIPLE)((pu1Byte)pTriple + 3);
	}
	pDot11dInfo->ChnlListLen = ChnlListLen;
	RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, "Dot11d_UpdateCountryIe(): Channel List:", 
		pDot11dInfo->ChnlList, pDot11dInfo->ChnlListLen);

	UPDATE_CIE_SRC(pMgntInfo, pTaddr);

	pDot11dInfo->CountryIeLen = posCoutryIe->Length;
	PlatformMoveMemory(
		pDot11dInfo->CountryIeBuf, 
		posCoutryIe->Octet,
		pDot11dInfo->CountryIeLen);
	pDot11dInfo->State = DOT11D_STATE_LEARNED;

	RT_PRINT_STR(COMP_SCAN, DBG_LOUD, "Dot11d_UpdateCountryIe(): Country Code", &pDot11dInfo->CountryIeBuf[0], 3);
	RT_PRINT_DATA(COMP_SCAN, DBG_TRACE, "Dot11d_UpdateCountryIe(): Country IE:", 
		pDot11dInfo->CountryIeBuf, pDot11dInfo->CountryIeLen);
	AddDrvLog(pAdapter, LID_DOT11D_GET_COUNTRY_IE, pDot11dInfo->CountryIeBuf, pDot11dInfo->CountryIeLen);
}


//
// Assumption:
//	1. IS_DOT11D_ENABLE() is TRUE.
//	2. Channel numbers in s_MaxChnlList[] or pDot11dInfo->ChnlList 
//	are monotonically increasing.
//
u1Byte
Dot11d_GetChannelList(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte*		ppChnlList
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	pu1Byte				pChnlList;
	u1Byte				ChnlListLen, First5gChnlIdx;

	if(IS_COUNTRY_IE_VALID(pMgntInfo))
	{
		ChnlListLen = pDot11dInfo->ChnlListLen;
		pChnlList = pDot11dInfo->ChnlList;
	}
	else
	{
		ChnlListLen = sizeof(s_MaxChnlList);
		pChnlList = s_MaxChnlList;
	}

	for(First5gChnlIdx = 0; First5gChnlIdx < ChnlListLen; First5gChnlIdx++)
	{
		if(pChnlList[First5gChnlIdx] > 14)
			break; // 1st A band channel bound
	}

	if(IS_5G_WIRELESS_MODE(pMgntInfo->dot11CurrentWirelessMode))
	{
		pChnlList += First5gChnlIdx;
		ChnlListLen -= First5gChnlIdx;
	}
	else
		ChnlListLen = First5gChnlIdx;

	if(ChnlListLen == 0)
		*ppChnlList = NULL;
	else
		*ppChnlList = pChnlList;

	RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, "Dot11d_GetChannelList: ", *ppChnlList, ChnlListLen);

	return ChnlListLen;
}


s4Byte
DOT11D_GetMaxTxPwrInDbm(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Channel
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	s4Byte MaxTxPwrInDbm = UNSPECIFIED_PWR_DBM;
	u4Byte ChnlIdx = 0;

	for(ChnlIdx = 0; ChnlIdx < pDot11dInfo->ChnlListLen; ChnlIdx++)
	{
		if(Channel == pDot11dInfo->ChnlList[ChnlIdx])
		{
			MaxTxPwrInDbm = (s4Byte)pDot11dInfo->MaxTxPwrDbmList[ChnlIdx];	
			break;
		}
	}

	return MaxTxPwrInDbm;
}


VOID
DOT11D_ScanComplete(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);

	switch(pDot11dInfo->State)
	{
	case DOT11D_STATE_LEARNED:
		pDot11dInfo->State = DOT11D_STATE_DONE;
		
		// Reset the channel list because dot11d has changed.
		RtResetChannelList(GET_RT_CHANNEL_LIST(pMgntInfo));
		break;

	case DOT11D_STATE_DONE:
		if(	GET_CIE_WATCHDOG(pMgntInfo) == 0 )
		{ // Reset country IE if previous one is gone. 
			Dot11d_Reset(Adapter); 
		}
		break;
		
	default: //MacOS warning:enum value "DOT11D_STATE_NONE" not handled in switch
		break;
	}

}

//
// Description:
//	The interface for all channel list actions.
//	Update the current channel list according to the input status code.
// Arguments:
//	Adapter -
//		NIC adapter context pointer.
// 	ActionCode -
//		An u4Byte number enumered in RT_CHNL_LIST_ACTION.
// 	pInputBuf -
//		The input buffer as a pointer.
// 	pOutputBuf -
//		The output buffer as a pointer.
// Return Value:
//	TRUE if this action is complete, or retrun FALSE.
// By Bruce, 2008-09-09.
//
BOOLEAN
RtActChannelList(
	IN		PADAPTER	pAdapter,
	IN		u4Byte		ActionCode,
	IN		PVOID		pInputBuf,
	OUT		PVOID		pOutputBuf
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	BOOLEAN				bResult = TRUE;
	
	switch(ActionCode)
	{
	case RT_CHNL_LIST_ACTION_INIT:
		RtResetChannelList(GET_RT_CHANNEL_LIST(pMgntInfo));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_RESET\n"));
		break;

	case RT_CHNL_LIST_ACTION_CONSTRUCT:
		bResult = RtConstructChannelList(pAdapter);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_CONSTRUCT\n"));
		break;

	case RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST:
		{
			PlatformAcquireSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);
			
			if(!IS_CHANNEL_LIST_VALID(pMgntInfo))
			{
				PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);
			
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST IS_CHANNEL_LIST_VALID FALSE\n"));
			
				RtConstructChannelList(pAdapter);
				// Because RtConstructChannelList() will reset the scan list, so we construct again
				bResult = RtConstructScanList(pAdapter);
				RT_ASSERT(bResult, ("RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST fail!!\n"));				
			}
			else
			{
				PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);
			}
			
			// Save the channel list address to the output context.
			if(pOutputBuf)
				*((PRT_CHANNEL_LIST*)pOutputBuf) = GET_RT_CHANNEL_LIST(pMgntInfo);
			}
		// RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST\n"));
		break;
	case RT_CHNL_LIST_ACTION_UPDATE_CHANNEL:
	case RT_CHNL_LIST_ACTION_ADD_CHANNEL:
		{
			PRT_CHNL_LIST_ENTRY	pChannelUpdateInfo = (PRT_CHNL_LIST_ENTRY)pInputBuf;
			PRT_CHNL_LIST_ENTRY	pChnlListEntry;

			if(!pChannelUpdateInfo)
			{
				bResult = FALSE;
				break;
			}

			PlatformAcquireSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);

			if(!IS_CHANNEL_LIST_VALID(pMgntInfo))
			{
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_UPDATE(ADD)_CHANNEL IS_CHANNEL_LIST_VALID FALSE\n"));
			
				PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);
			
				RtConstructChannelList(pAdapter);
			}
			else
			{
				PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);			
			}

			pChnlListEntry = RtGetChnlListEntry(pAdapter, pChannelUpdateInfo->ChannelNum);

			if(!pChnlListEntry)
			{ // No this channel, add into the channel list
				PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
				if(pChannelList->ChannelLen < MAX_CHANNEL_NUM)
				{
					PlatformMoveMemory(&(pChannelList->ChnlListEntry[pChannelList->ChannelLen]), pChannelUpdateInfo, sizeof(RT_CHNL_LIST_ENTRY));
					pChannelList->ChannelLen ++;
				}
				else
				{
					bResult = FALSE;
					break;
				}
			}
			else
			{
				PlatformMoveMemory(pChnlListEntry, pChannelUpdateInfo, sizeof(RT_CHNL_LIST_ENTRY));
			}
		}
		// RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_UPDATE(ADD)_CHANNEL\n"));
		break;

	case RT_CHNL_LIST_ACTION_DEL_CHANNEL:
		{
			u1Byte				Channel = *((pu1Byte)pInputBuf);
			PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
			u1Byte				i;
			
			bResult = FALSE;
			for(i = 0; i < pChannelList->ChannelLen; i ++)
			{
				if(pChannelList->ChnlListEntry[i].ChannelNum == Channel)
				{
					for(i=i; i < pChannelList->ChannelLen; i ++)
					{
						PlatformMoveMemory(&(pChannelList->ChnlListEntry[i]), &(pChannelList->ChnlListEntry[i + 1]), sizeof(RT_CHNL_LIST_ENTRY));
					}
					bResult = TRUE;
					break;
				}
			}
		}
		// RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_DELETE_CHANNEL\n"));
		break;

	case RT_CHNL_LIST_ACTION_GET_CHANNEL:
		{
			u1Byte				Channel = *((pu1Byte)pInputBuf);
			PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;

			PlatformAcquireSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);

			if(!IS_CHANNEL_LIST_VALID(pMgntInfo))
			{
				RT_TRACE(COMP_SCAN, DBG_TRACE, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_GET_CHANNEL IS_CHANNEL_LIST_VALID FALSE\n"));
			
				PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);			
				RtConstructChannelList(pAdapter);
			}
			else
			{
				PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);			
			}

			if((pChnlListEntry = RtGetChnlListEntry(pAdapter, Channel)) != NULL)
			{
				if(pChnlListEntry->ChannelNum == Channel)
					*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = pChnlListEntry;
				else
				{
					*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = NULL;
					bResult = FALSE;
				}
			}
			else
			{
				*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = NULL;
				bResult = FALSE;
			}
		}
		// RT_TRACE(COMP_SCAN, DBG_TRACE, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_GET_CHANNEL\n"));
		break;

	case RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST:
		{
			if(!IS_CHANNEL_LIST_VALID(pMgntInfo))
				RtConstructChannelList(pAdapter);

			bResult = RtConstructScanList(pAdapter);
		}
		//RT_ASSERT(bResult, ("RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST fail!!\n"));
	
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST\n"));
		break;

	case RT_CHNL_LIST_ACTION_POP_SCAN_CHANNEL:
		{
			PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
			PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;

			pChnlListEntry = RtPopScanChannel(pChannelList);
			if(pChnlListEntry)
			{
				bResult = TRUE;
				*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = pChnlListEntry;
			}
			else
			{
				bResult = FALSE;
				*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = NULL;
			}
		}
		// RT_TRACE(COMP_SCAN, DBG_TRACE, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_POP_SCAN_CHANNEL\n"));
		break;

	case RT_CHNL_LIST_ACTION_GET_SCAN_CHANNEL:
		{
			PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
			PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;

			pChnlListEntry = RtGetScanChannel(pChannelList);
			if(pChnlListEntry)
			{
				bResult = TRUE;
				*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = pChnlListEntry;
			}
			else
			{
				bResult = FALSE;
				*((PRT_CHNL_LIST_ENTRY*)pOutputBuf) = NULL;
			}

			if(pChannelList != NULL)
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("RtActChannelList(): <=====RT_CHNL_LIST_ACTION_GET_SCAN_CHANNEL index %d length %d channellength %d\n", pChannelList->ScanChannelIndex, pChannelList->ScanChannelListLength, pChannelList->ChannelLen));		
		}
		break;

	case RT_CHNL_LIST_ACTION_GET_CHANNEL_PLAN:
		{
			if(pInputBuf)
				*(RT_CHANNEL_PLAN **)pOutputBuf = &DefaultChannelPlan[*(RT_CHANNEL_DOMAIN *)pInputBuf];
		}
		break;
				
	default:
		bResult = FALSE;
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("RtActChannelList(): NO RT_CHNL_LIST_ACTION!!!\n"));
		break;
	}

	return bResult;
}

//
// Description:
//	Find the channel information in the channel list and retrun the address.
// By Bruce, 2008-09-09.
//
PRT_CHNL_LIST_ENTRY
RtGetChnlListEntry(
	IN		PADAPTER			pAdapter,
	IN		u1Byte				Channel
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
	PRT_CHNL_LIST_ENTRY	pChnlListEntry;
	int					i = 0;

	if(pChannelList->ChannelLen == 0)
		return NULL;

	for(i = 0; i < pChannelList->ChannelLen; i ++)
	{
		if(pChannelList->ChnlListEntry[i].ChannelNum == Channel)
		{
			pChnlListEntry = &(pChannelList->ChnlListEntry[i]);
			return pChnlListEntry;
		}
	}

	return NULL;
}

u1Byte
RtCustomizeChannelList(
	PMGNT_INFO			pMgntInfo,
	u1Byte				ChannelPlan,
	u1Byte				ChannelLen,
	PRT_CHNL_LIST_ENTRY	pChnlList
	)
{
	u1Byte				i;
	PRT_CHNL_LIST_ENTRY	pChnlListEntry;

	//3 // Put Customized Action here...
	switch(ChannelPlan)
	{
	case RT_CHANNEL_DOMAIN_GLOBAL_DOMAIN:
	case RT_CHANNEL_DOMAIN_GLOBAL_DOMAIN_2G:		// 2G global domain.
		{
			for(i = 0; i < ChannelLen; i ++)
			{
				pChnlListEntry = &(pChnlList[i]);

				// (1) Start IBSS at channel 1~11.		
				// (2) Join IBSS at channel 1~11.
				if(pChnlListEntry->ChannelNum >= 12 && pChnlListEntry->ChannelNum <= 14)
				{
					pChnlListEntry->ExInfo |= CHANNEL_EXINFO_NO_IBSS_JOIN;
					pChnlListEntry->ExInfo |= CHANNEL_EXINFO_NO_IBSS_START;
				}
				else
				{
					pChnlListEntry->ExInfo &= ~CHANNEL_EXINFO_NO_IBSS_JOIN;
					pChnlListEntry->ExInfo &= ~CHANNEL_EXINFO_NO_IBSS_START;
				}	

				// (3) Channel 1~11 is active, and 12~14 is passive when dot11 is not learned yet.
				if(!IS_COUNTRY_IE_VALID(pMgntInfo))
				{
					if(pChnlListEntry->ChannelNum >= 1 && pChnlListEntry->ChannelNum <= 11)
					{
						pChnlListEntry->ScanType = SCAN_ACTIVE;
						pChnlListEntry->ScanPeriod = DEFAULT_ACTIVE_SCAN_PERIOD;
					}

					if((pChnlListEntry->ChannelNum >= 12 && pChnlListEntry->ChannelNum <= 14))
					{						
						pChnlListEntry->ScanType = SCAN_PASSIVE;
						pChnlListEntry->ScanPeriod = DEFAULT_PASSIVE_SCAN_PERIOD;
					}
				}
			}
		}
		break;
		
	case RT_CHANNEL_DOMAIN_WORLD_WIDE_13:
		{
			for(i = 0; i < ChannelLen; i ++)
			{
				pChnlListEntry = &(pChnlList[i]);
				
				// (1) Channel 14 is disable
				if(pChnlListEntry->ChannelNum == 14)
				{
					for(++ i;  i < ChannelLen; i ++)
					{
						PlatformMoveMemory(&(pChnlList[i - 1]), &(pChnlList[i]), sizeof(RT_CHNL_LIST_ENTRY));
					}
					ChannelLen --;
					break;
				}
				
				// (2) Join IBSS at channel 1~11.
				// (3) Start IBSS at channel 1~11.
				// (4) channel 12~13, passive scan
				if(pChnlListEntry->ChannelNum >= 12 && pChnlListEntry->ChannelNum <= 13)
				{
					pChnlListEntry->ExInfo |= (CHANNEL_EXINFO_NO_IBSS_JOIN);
					pChnlListEntry->ExInfo |= (CHANNEL_EXINFO_NO_IBSS_START);
					pChnlListEntry->ScanType = SCAN_MIX;
					pChnlListEntry->ScanPeriod = DEFAULT_PASSIVE_SCAN_PERIOD;
				}
			}
		}
		break;
	case RT_CHANNEL_DOMAIN_DEFAULT://For Realtek default setting
		{
			for(i = 0; i < ChannelLen; i ++)
			{
				pChnlListEntry = &(pChnlList[i]);
				
				// (2) Join IBSS at channel 1~11.
				// (3) Start IBSS at channel 1~11.
				// (4) channel 12~13, passive scan
				if(pChnlListEntry->ChannelNum <=13 && pChnlListEntry->ChannelNum >= 12)
				{
					pChnlListEntry->ExInfo |= (CHANNEL_EXINFO_NO_IBSS_JOIN);
					pChnlListEntry->ExInfo |= (CHANNEL_EXINFO_NO_IBSS_START);
					pChnlListEntry->ScanType = SCAN_MIX;
					pChnlListEntry->ScanPeriod = DEFAULT_PASSIVE_SCAN_PERIOD;
				}
			}
		}
		break;

	default:
		break;
	}
	
	switch(pMgntInfo->CustomerID)
	{
		

		case RT_CID_WHQL:
			{
				for(i = 0; i < ChannelLen; i ++)
				{
					pChnlListEntry = &(pChnlList[i]);
					pChnlListEntry->ScanPeriod = SCAN_PERIOD_Long_Win_DTM;				
				}
			}
			break;

		default:
			break;
	}

	return ChannelLen;
}

VOID
_ForcedChannelPlan(
	PMGNT_INFO			pMgntInfo,
	BOOLEAN				bIs5G,
	pu1Byte				pChnlList,
	pu1Byte				pChannelLen
	)
{
	ps1Byte s =  (ps1Byte)((bIs5G) ? pMgntInfo->RegChannelPlan5G.Octet : 
		                             pMgntInfo->RegChannelPlan2G.Octet);
	u2Byte  len = ((bIs5G) ? pMgntInfo->RegChannelPlan5G.Length : 
		                     pMgntInfo->RegChannelPlan2G.Length); 
	u1Byte  c = 0, n = 0, i = 0;

	RT_TRACE( COMP_INIT, DBG_LOUD, ("===> _ForcedChannelPlan\n"));
	
	if (bIs5G) 
		if (pMgntInfo->RegChannelPlan5G.bDefaultStr)
			return;
	else 
		if (pMgntInfo->RegChannelPlan2G.bDefaultStr)
			return;	

	PlatformZeroMemory(pChnlList, *pChannelLen);
	*pChannelLen = 0;
	
	for (i = 0; i < len; ++i, ++s) {
		while (*s != ',' && *s >= '0' && *s <= '9') {
			c = 10 * c + (*s - '0');
			s++, i++;
		}
		pChnlList[n++] = c;
		c = 0;
	}

	*pChannelLen = n;
	
	RT_TRACE( COMP_INIT, DBG_LOUD, ("<=== _ForcedChannelPlan\n"));	
}

//
// Description:
//	(Re-)Construct the channel list by 11d or default channel plan.
// Note:
//	We collect all customized requests of joining/scanning about each channel in here.
// By Bruce, 2008-09-09.
//
BOOLEAN
RtConstructChannelList(
	IN	PADAPTER	pAdapter
	)
{
	pu1Byte				pChnlList = NULL;
	u1Byte 				ChnlsLen = 0, i = 0;
	PRT_CHNL_LIST_ENTRY	pChnlListEntry;
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	RT_CHANNEL_DOMAIN	ChannelPlan = GetDefaultMgntInfo(pAdapter)->ChannelPlan;
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);

	PlatformAcquireSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);

	if(pAdapter->pNdis62Common != NULL)
		RT_TRACE_F(COMP_SCAN, DBG_LOUD, ("port number %d \n", pAdapter->pNdis62Common->PortNumber));

	if(ChannelPlan >= RT_CHANNEL_DOMAIN_MAX) // Channel Plan is invalid, fix it.
		pMgntInfo->ChannelPlan = RT_CHANNEL_DOMAIN_FCC;
	
	RtResetChannelList(pChannelList);

	pChannelList->ChannelPlan = ChannelPlan;

	//3 // Dot11d or Default Channel Plan
	if(IS_DOT11D_ENABLE(pMgntInfo))
	{ // dot11d is enabled.
		RT_SCAN_TYPE	scanType = SCAN_ACTIVE;

		if(!IS_COUNTRY_IE_VALID(pMgntInfo))
		{ // domain learning is not complete
			scanType = SCAN_PASSIVE;
		}

		ChnlsLen = Dot11d_GetChannelList(pAdapter, &pChnlList);

		if(ChnlsLen > MAX_CHANNEL_NUM)
		{	
			PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);
			return FALSE;
		}	

		for(i = 0; i < ChnlsLen; i ++)
		{
			// Check if the channel is valid for the current wireless mode.
			if(!CHNL_ValidForWirelessMode(pChnlList[i], (u2Byte)pMgntInfo->dot11CurrentWirelessMode))
				continue;

			pChnlListEntry = &(pChannelList->ChnlListEntry[i]);

			pChnlListEntry->ChannelNum = pChnlList[i];

			// For 5G FCC Radar channel, default passive scan
			if(DFS_5G_RADAR_CHANNEL(pChnlListEntry->ChannelNum))
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("DFS_5G_RADAR_CHANNEL, 802.11d, set chnl %d to passive scan.\n", pChnlListEntry->ChannelNum));
				pChnlListEntry->ScanType = SCAN_PASSIVE;
				DFS_StaInsertToRadarChnlList(pAdapter, pChnlListEntry->ChannelNum);
			}
			else
				pChnlListEntry->ScanType = scanType;
			
			pChnlListEntry->ScanPeriod = (scanType == SCAN_ACTIVE) ? DEFAULT_ACTIVE_SCAN_PERIOD : DEFAULT_PASSIVE_SCAN_PERIOD;
			pChnlListEntry->MaxTxPwrDbm = DOT11D_GetMaxTxPwrInDbm(pAdapter, pChnlListEntry->ChannelNum);
			pChnlListEntry->ExInfo = 0;

			if(!IS_COUNTRY_IE_VALID(pMgntInfo))
			{ // domain learning is not complete, we cannot start IBSS.
				pChnlListEntry->ExInfo |= CHANNEL_EXINFO_NO_IBSS_START;
			}
			
			pChannelList->ChannelLen ++;
		}

		pChannelList->bDot11d = TRUE;
	}
	else
	{ // Normal Case
		u1Byte	usbswchnl;
		RT_TRACE( COMP_SCAN, DBG_LOUD, ("_RT_CHANNEL_DOMAIN(0x%X) mapped from hal_MapChannelPlan: %s channel plan is applied.\n", 
										ChannelPlan, (ChannelPlan >= RT_CHANNEL_DOMAIN_WHQL)?"New":"Legacy"));

		if (ChannelPlan < RT_CHANNEL_DOMAIN_WW13_2G)
		{
			pChnlList = DefaultChannelPlan[pChannelList->ChannelPlan].Channel;
			ChnlsLen = DefaultChannelPlan[pChannelList->ChannelPlan].Len;			
		}
		else
		{
			if(IS_WIRELESS_MODE_5G(pAdapter))
			{
				pChnlList = DefaultChannelPlan[pChannelList->ChannelPlan].Channel5G;
				ChnlsLen = DefaultChannelPlan[pChannelList->ChannelPlan].Len5G;	
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("===>RtConstructChannelList, DefaultChannelPlan[%d].Channel5G: ", pChannelList->ChannelPlan));
			}
			else
			{
				pChnlList    = DefaultChannelPlan[pChannelList->ChannelPlan].Channel2_4G;
				ChnlsLen = DefaultChannelPlan[pChannelList->ChannelPlan].Len2_4G;							
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("===>RtConstructChannelList, DefaultChannelPlan[%d].Channel2.4G: ", pChannelList->ChannelPlan));
			}		
			
		}

		if (pMgntInfo->RegChannelPlan2G.bDefaultStr != TRUE ||
			pMgntInfo->RegChannelPlan5G.bDefaultStr != TRUE)
			_ForcedChannelPlan(pMgntInfo, IS_WIRELESS_MODE_5G(pAdapter), pChnlList, &ChnlsLen);
		
		for(i = 0; i < ChnlsLen; i ++)
		{

			// Check if the channel is valid for the current wireless mode.
			if (!CHNL_ValidForWirelessMode(pChnlList[i], pMgntInfo->dot11CurrentWirelessMode))
			{
				// RT_TRACE_F(COMP_SCAN, DBG_LOUD, ("Skip channel (%d) for wirelessMode (%d)\n", pChnlList[i], pMgntInfo->dot11CurrentWirelessMode));
				continue;
			}
			
			pChnlListEntry = &(pChannelList->ChnlListEntry[pChannelList->ChannelLen]);
					
			pChnlListEntry->ChannelNum = pChnlList[i];

			// For 5G FCC Radar channel, default passive scan
			if (DFS_5G_RADAR_CHANNEL(pChnlListEntry->ChannelNum))
			{
				RT_TRACE( COMP_DFS, DBG_LOUD, ("DFS_5G_RADAR_CHANNEL, Non 802.11d, set chnl %d to passive scan.\n", pChnlListEntry->ChannelNum));
				pChnlListEntry->ScanType = SCAN_PASSIVE;
				DFS_StaInsertToRadarChnlList(pAdapter, pChnlListEntry->ChannelNum);
			}
			else
			{
				if (pMgntInfo->RegPassiveScan && ChannelPlan == RT_CHANNEL_DOMAIN_DEFAULT)
				{
					// 0= by channel plan, 1=5g all passive scan / 2= 24g passive scan /3= 2/5g all passive scan
					if (pMgntInfo->RegPassiveScan == 1 && pChnlListEntry->ChannelNum >= 36)
					{						
						RT_TRACE( COMP_DFS, DBG_LOUD, ("CHNL-%d passive scan", pChnlListEntry->ChannelNum));
						pChnlListEntry->ScanType = SCAN_PASSIVE;
					}
					else if (pMgntInfo->RegPassiveScan == 2 && pChnlListEntry->ChannelNum <= 14)
					{
						pChnlListEntry->ScanType = SCAN_PASSIVE;
					}
					else if (pMgntInfo->RegPassiveScan == 3)
					{
						pChnlListEntry->ScanType = SCAN_PASSIVE;
					}
					else
					{
						pChnlListEntry->ScanType = SCAN_ACTIVE;
					}
				}
				else
					pChnlListEntry->ScanType = SCAN_ACTIVE;

			}

			if(pAdapter->bInHctTest)
				pChnlListEntry->ScanPeriod = SCAN_PERIOD_Long_Win_DTM;
			else
			{
				{
					pChnlListEntry->ScanPeriod = DEFAULT_ACTIVE_SCAN_PERIOD;
				}
			}
			pChnlListEntry->MaxTxPwrDbm = UNSPECIFIED_PWR_DBM;
			pChnlListEntry->ExInfo = 0;

			pChannelList->ChannelLen ++;	
		}
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("===>RtConstructChannelList, ChannelList: "));
		for (i = 0; i < pChannelList->ChannelLen; ++i)
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("%d ", (pChannelList->ChnlListEntry[i]).ChannelNum));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n "));
	}

	pChannelList->WirelessMode = pMgntInfo->dot11CurrentWirelessMode;

	pChannelList->ChannelLen = RtCustomizeChannelList(pMgntInfo, pChannelList->ChannelPlan, pChannelList->ChannelLen, pChannelList->ChnlListEntry);

	// Increase the weighted channel
	#ifdef USE_SMART_SCAN
	{
		u1Byte			WeightedChannel[3] = {1, 6, 11};

		for(i = 0; i < 3; i ++)
		{
			if((pChnlListEntry = RtGetChnlListEntry(pAdapter, WeightedChannel[i])) != NULL)
				pChnlListEntry->ExInfo |= CHANNEL_EXINFO_WEIGHTED_SCAN_PERIOD;
		}
	}
	#endif // #ifdef USE_SMART_SCAN

	if(!pAdapter->bInHctTest)
	{
		for(i = 0; i < pChannelList->ChannelLen; i ++)
		{
			pChnlListEntry = &(pChannelList->ChnlListEntry[i]);
			pChnlListEntry->ScanPeriod = SCAN_PERIOD_Long;
		}	
	}

	DumpChannelList(pAdapter);

	PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);
	
	FunctionOut(COMP_SCAN);

	return TRUE;
}

u1Byte
RtGetConnectedChannels(
	IN	PADAPTER	pAdapter,
	OUT pu1Byte		ConnectedChannels,
	IN	u1Byte		maxChnlArrayNum
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	u1Byte uNumberOfConnectedChannel = 0;
	u1Byte i = 0;
	BOOLEAN bDuplicate = FALSE;

	if(0 == (uNumberOfConnectedChannel = MultiChannelGetConnectedChannels(
				pAdapter,
				ConnectedChannels,
				maxChnlArrayNum)))
	{
		ConnectedChannels[uNumberOfConnectedChannel++] = pDefaultAdapter->MgntInfo.dot11CurrentChannelNumber;
	}

	// Let AP can serve the Client During scanning -------------------------------------------------------------------
	if(IsAPModeExist(pDefaultAdapter))
	{
		bDuplicate = FALSE;
		
		for(i = 0; i < uNumberOfConnectedChannel; i++)
		{
			if(ConnectedChannels[i] == pDefaultAdapter->MgntInfo.dot11CurrentChannelNumber)
			{
				bDuplicate = TRUE;
				break;
			}
		}

		if(bDuplicate == FALSE)
		{
			ConnectedChannels[uNumberOfConnectedChannel++] = pDefaultAdapter->MgntInfo.dot11CurrentChannelNumber;
		}
	}
	// --------------------------------------------------------------------------------------------------------

	
	return uNumberOfConnectedChannel;
}

		

//
// Description:
//	(Re-)Construct the scan channel list.
//	Any customized request about scanning can be put here.
// By Bruce, 2008-09-09.
//
BOOLEAN
RtConstructScanList(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PADAPTER 			pDefaultAdapter = GetDefaultAdapter(pAdapter);

	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
	PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL, pFinalChnlListEntry = NULL;
	u2Byte				i, j, k;

	u1Byte	ConnectedChannels[MAX_SCAN_CHANNEL_NUM];
	u1Byte	uNumberOfConnectedChannel = 0;
	
	const u1Byte 			CountForAddConnectedChannel = 2;
	u1Byte 				uAccumulatedCountForAdd = 0;	

	u1Byte				BackupScanChannelListLength;
	PRT_CHNL_LIST_ENTRY	BackupScanChannelList[MAX_SCAN_CHANNEL_NUM];
	BOOLEAN				bRet = FALSE;
	
	if(pAdapter->pNdis62Common != NULL)
		RT_TRACE_F(COMP_SCAN, DBG_LOUD, ("port number %d \n", pAdapter->pNdis62Common->PortNumber));

	PlatformAcquireSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);

	RtResetScanChannel(pChannelList);

	RtCalcScanStatistic(pAdapter);
	RtCalcScanPeriod(pAdapter);

	RT_TRACE_F(COMP_SCAN, DBG_LOUD, ("ChannelLen %d  \n", pChannelList->ChannelLen));

	if(pDefaultAdapter->bInWFDTest && pDefaultAdapter->MgntInfo.WFDOpChannel != 0)
	{
		// Rearrange scan channel(one based)
		for(i = 0; i < pChannelList->ChannelLen; i ++)
		{
			pChnlListEntry = &(pChannelList->ChnlListEntry[i]);

			if(pChnlListEntry->ChannelNum == pDefaultAdapter->MgntInfo.WFDOpChannel)
			{
				RtInsertScanChannel(pChannelList, pChnlListEntry);
			}
		}
	}

	// Rearrange scan channel(one based)
	for(i = 0; i < CHANNEL_SCAN_CYCLE_NUM; i ++)
	{
		for(j = i; j < pChannelList->ChannelLen; j += CHANNEL_SCAN_CYCLE_NUM)
		{
			pChnlListEntry = &(pChannelList->ChnlListEntry[j]);

			if(pChnlListEntry->ChannelNum == pMgntInfo->dot11CurrentChannelNumber)
			{
				//4 //Put ChannelNumBeforeScan at the end of the channel list
				//4  //So we don't need to do channel switch after scan.
				pFinalChnlListEntry = pChnlListEntry;
			}
			else
			{
				if(!pDefaultAdapter->bInWFDTest ||
					(pChnlListEntry->ChannelNum != pDefaultAdapter->MgntInfo.WFDOpChannel))
					RtInsertScanChannel(pChannelList, pChnlListEntry);
			}
		}
	}
	
	if(pFinalChnlListEntry)
	{
		RtInsertScanChannel(pChannelList, pFinalChnlListEntry);
	}

	//8192DE SMSP Dual Scan for Hidden SSID
	if(pMgntInfo->bNeedSkipScan )//Add hidded ssid's channel to first place.
	{
		PRT_CHNL_LIST_ENTRY	ptmpChnlListEntry = NULL;
		ptmpChnlListEntry = (pChannelList->ScanChannelList[0]);		
		for(j = 1; j < pChannelList->ChannelLen; j ++)
			{
				pChnlListEntry = (pChannelList->ScanChannelList[j]);

				if(pChnlListEntry->ChannelNum == pMgntInfo->hiddenChannel)
				{
					pChannelList->ScanChannelList[0] = pChnlListEntry;
					pChannelList->ScanChannelList[j] = ptmpChnlListEntry;
					break;
				}
			}
	}


	// Construct Connected Channels: ---------------------------------------------------------------
	uNumberOfConnectedChannel = RtGetConnectedChannels(pAdapter, ConnectedChannels, MAX_SCAN_CHANNEL_NUM);

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("%s: uNumberOfConnectedChannel: %d\n", 
				__FUNCTION__, uNumberOfConnectedChannel)
			);
	
	for(i = 0; i < uNumberOfConnectedChannel; i++)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("%s: Connected Channel: %d\n", 
				__FUNCTION__, ConnectedChannels[i])
			);
	}
	// ------------------------------------------------------------------------------------------

	if(pMgntInfo->dot11CurrentChannelBandWidth == CHANNEL_WIDTH_20) //yangjun121019
	{
		// Insert Connected Channel for Every CountForAddConnectedChannel (Default 2) Channels -----------------------------------
		for(k = 0; k < uNumberOfConnectedChannel; k++)
		{
			// Search for the Connected Channel Information
			for(i = 0; i < pChannelList->ChannelLen; i++)
			{
				pChnlListEntry = &(pChannelList->ChnlListEntry[i]);

				if(pChnlListEntry->ChannelNum == ConnectedChannels[k])
				{
					// Backup the ScanChannelList
					BackupScanChannelListLength =  pChannelList->ScanChannelListLength;
					PlatformMoveMemory(BackupScanChannelList, pChannelList->ScanChannelList, sizeof(BackupScanChannelList));

					// Clean the ScanChannel
					RtResetScanChannel(pChannelList);

					// Set the Flag for Connected Channel
					pChnlListEntry->ExInfo |= CHANNEL_EXINFO_CONNECTED_CHANNEL;
					
					for(j = 0, uAccumulatedCountForAdd = 0; j < BackupScanChannelListLength; j++)
					{
						RtInsertScanChannel(pChannelList, BackupScanChannelList[j]);	

						if(BackupScanChannelList[j]->ChannelNum != ConnectedChannels[k])
						{// Different Channel
							uAccumulatedCountForAdd++;

							if(uAccumulatedCountForAdd % CountForAddConnectedChannel == 0)
							{// Insert Connected Channel
								RtInsertScanChannel(pChannelList, pChnlListEntry);	
							}
						}
						else
						{// Same Channel as Connected Channel
							uAccumulatedCountForAdd = 0;
						}
					}
				
					break;
				}
			}
		}
		// ------------------------------------------------------------------------------------------------------------
	}

	if(CustomScan_ConstructScanListCb(GET_CUSTOM_SCAN_INFO(pAdapter), pChannelList))
	{// system scan
		if(!pDefaultAdapter->bInHctTest)
			shuffleScanChannelList(pAdapter);
		DumpScanChannelList(pAdapter);
		bRet = (0 != pChannelList->ScanChannelListLength);
	}
	else
	{// other
		DumpScanChannelList(pAdapter);
		bRet = (0 != pChannelList->ScanChannelListLength);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_CHNLLIST_SPINLOCK);

	FunctionOut(COMP_SCAN);

	return bRet;
}

//
// Caculate the statistic of the scan list.
//
VOID
RtCalcScanStatistic(
	IN	PADAPTER	pAdapter
	)
{
	u1Byte 				j;
	u2Byte 				ChannelNum, i;
	PMGNT_INFO 			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);

	PlatformZeroMemory(pChannelList->EachChannelSTAs, MAX_SCAN_CHANNEL_NUM);
	pChannelList->bValidSTAStatis = FALSE;

	if(pMgntInfo->NumBssDesc4Query == 0)
		return;

	pChannelList->bValidSTAStatis = TRUE;
	for(i = 0; i < pMgntInfo->NumBssDesc4Query; i++)
	{
		ChannelNum = pMgntInfo->bssDesc4Query[i].ChannelNumber;
		for(j = 0; j < pChannelList->ChannelLen; j++)
		{
			if(pChannelList->ChnlListEntry[j].ChannelNum == ChannelNum)
			{
				pChannelList->EachChannelSTAs[j]++;
				break;
			}
		}	
	}
}

//
// Description:
// 	Caculate the scan period.
// By Bruce, 2009-04-09.
//
VOID
RtCalcScanPeriod(
	IN	PADAPTER			pAdapter
	)
{
	u1Byte 				i;
	PMGNT_INFO 			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
	static u1Byte			Scan5gCnt = 0;
	BOOLEAN				bShortScanPeriod = FALSE;
	
	if(IsActiveAPModeExist(pAdapter) 
		/*||pMgntInfo->mAssoc || pMgntInfo->bMediaConnect*/)// comment out for temporary solve less AP scaned when connected, Sean, 201501 
	{			
		bShortScanPeriod = TRUE;
	}
	
	if(IS_5G_WIRELESS_MODE(pChannelList->WirelessMode))
		Scan5gCnt++;

	for(i = 0; i < pChannelList->ChannelLen; i++)
	{
		if(pAdapter->bInHctTest)
		{
			//For Win7 DTMTest (Performance_ext). To avoid scan too long to roaming to another AP more than 4.1 seconds.
			//Maddest 05132009.
				if(pMgntInfo->NdisVersion>=RT_NDIS_VERSION_6_20)
					pChannelList->ChnlListEntry[i].ScanPeriod =SCAN_PERIOD_Long_Win_DTM;
				else
					pChannelList->ChnlListEntry[i].ScanPeriod = DEFAULT_ACTIVE_SCAN_PERIOD;

		}
		else if(bShortScanPeriod)
		{
			pChannelList->ChnlListEntry[i].ScanPeriod = SCAN_PERIOD_Short;
		}
		else if(pMgntInfo->RegForcedScanPeriod != 0 )
		{
			pChannelList->ChnlListEntry[i].ScanPeriod = pMgntInfo->RegForcedScanPeriod;
		}
		else if(pChannelList->ChnlListEntry[i].ScanType == SCAN_PASSIVE)
		{
			if(pChannelList->ChannelPlan == RT_CHANNEL_DOMAIN_WORLD_WIDE_5G || pChannelList->ChannelLen >= 15)
			{
				pChannelList->ChnlListEntry[i].ScanPeriod = ((i+Scan5gCnt)%3)?50:100;
			}
			else
			{
			pChannelList->ChnlListEntry[i].ScanPeriod = DEFAULT_PASSIVE_SCAN_PERIOD;
			}
		}
		else if(pMgntInfo->bScanOnly && pChannelList->bValidSTAStatis)
		{
			//
			// If "pMgntInfo->bScanOnly" is set false as scanning for join, we shall not set the scan period too long.
			// A long scan period may cause join procedure timeout.
			// Such as the example under XP, the ndis sets the SSID and wait just 2 Seconds for join complete.
			// If we cannot complete the join procedure as soon as possible (in 2 s), it will set the dummy SSID, which
			// a reset command, and stop the driver stop the join procedure.
			// By Bruce, 2009-04-09.
			//
			if(pChannelList->EachChannelSTAs[i] >= Scan_STA_Large)
			{
				RT_TRACE( COMP_SCAN, DBG_TRACE, ("RtCalcScanPeriod, Scan_STA_Large change ScanPeriod from %d to %d/%d\n", pChannelList->ChnlListEntry[i].ScanPeriod, pMgntInfo->RegScanLarge, SCAN_PERIOD_Long));

				if( pMgntInfo->RegScanLarge == 0 )
					pChannelList->ChnlListEntry[i].ScanPeriod = SMART_SCAN_PERIOD_LONG;
				else
					pChannelList->ChnlListEntry[i].ScanPeriod = pMgntInfo->RegScanLarge;
			}
			else if(pChannelList->EachChannelSTAs[i] >= Scan_STA_Middle)
			{
				RT_TRACE( COMP_SCAN, DBG_TRACE, ("RtCalcScanPeriod, Scan_STA_Middle change ScanPeriod from %d to %d/%d\n", pChannelList->ChnlListEntry[i].ScanPeriod, pMgntInfo->RegScanMiddle, DEFAULT_ACTIVE_SCAN_PERIOD));

				if( pMgntInfo->RegScanMiddle== 0 )
					pChannelList->ChnlListEntry[i].ScanPeriod = SMART_SCAN_PERIOD_MIDDLE;
				else
					pChannelList->ChnlListEntry[i].ScanPeriod = pMgntInfo->RegScanMiddle;
			}
			else
			{
				RT_TRACE( COMP_SCAN, DBG_TRACE, ("RtCalcScanPeriod, !(Scan_STA_Large && Scan_STA_Middle)  change ScanPeriod from %d to %d/%d\n", pChannelList->ChnlListEntry[i].ScanPeriod, pMgntInfo->RegScanNormal, SCAN_PERIOD_Short));

				if( pMgntInfo->RegScanNormal== 0 )
					pChannelList->ChnlListEntry[i].ScanPeriod = SMART_SCAN_PERIOD_SHORT;
				else
					pChannelList->ChnlListEntry[i].ScanPeriod = pMgntInfo->RegScanNormal;
			}
		}
		else
		{
			RT_TRACE( COMP_SCAN, DBG_TRACE, ("RtCalcScanPeriod, change ScanPeriod from %d to %d/%d\n", pChannelList->ChnlListEntry[i].ScanPeriod, pMgntInfo->RegScanActive, DEFAULT_ACTIVE_SCAN_PERIOD));

			//20121121 Sinda enlarge scan time per channel from 50 ms to 100 ms.
			if( pMgntInfo->RegScanActive== 0 )
				pChannelList->ChnlListEntry[i].ScanPeriod = DEFAULT_PASSIVE_SCAN_PERIOD;
			else
				pChannelList->ChnlListEntry[i].ScanPeriod = pMgntInfo->RegScanActive;
		}
		
		RT_TRACE(COMP_SCAN, DBG_TRACE, ("channel %d STAs %d Period %d\n", pChannelList->ChnlListEntry[i].ChannelNum, pChannelList->EachChannelSTAs[i], pChannelList->ChnlListEntry[i].ScanPeriod));
	}

}


//sherry sync with 92C_92D, 20110701 GetDualBandChannelList
u1Byte 
RtGetDualBandChannel(
	IN	PADAPTER			Adapter,
	OUT	PRT_CHNL_LIST_ENTRY	ChnlListEntryArray
)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	pu1Byte				pChnlList;
	u1Byte				listLen;
	PRT_DOT11D_INFO 	pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	u1Byte 				i, ChannelLen = 0;

	RT_TRACE(COMP_SCAN, DBG_LOUD,("============> RtGetDualBandChannel \n"));

	if(IS_DOT11D_ENABLE(pMgntInfo))
	{ // dot11d is enabled.
		RT_SCAN_TYPE	scanType = SCAN_ACTIVE;

		if(!IS_COUNTRY_IE_VALID(pMgntInfo))
		{ // domain learning is not complete
			scanType = SCAN_PASSIVE;
		}

		if(IS_COUNTRY_IE_VALID(pMgntInfo))
		{
			listLen = pDot11dInfo->ChnlListLen;
			pChnlList = pDot11dInfo->ChnlList;
		}
		else
		{
			listLen = sizeof(s_MaxChnlList);
			pChnlList = s_MaxChnlList;
		}

		if(listLen > MAX_CHANNEL_NUM)
			listLen = MAX_CHANNEL_NUM;
		
		RT_TRACE(COMP_SCAN, DBG_LOUD,("RtGetDualBandChannel: Dot11d case listLen %d \n", listLen));
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelNum"));
	
		for(i = 0; i < listLen; i ++)
		{
			ChnlListEntryArray[i].ChannelNum = pChnlList[i];

			// For 5G FCC Radar channel, default passive scan
			if(DFS_5G_RADAR_CHANNEL(ChnlListEntryArray[i].ChannelNum))
				ChnlListEntryArray[i].ScanType = SCAN_PASSIVE;
			else
				ChnlListEntryArray[i].ScanType = scanType;

			RT_TRACE(COMP_SCAN, DBG_LOUD,(" %d ",ChnlListEntryArray[i].ChannelNum));
		}

		ChannelLen = listLen;
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelLen %d\n", ChannelLen));
	}
	else
	{ // Normal Case
	
		RT_TRACE(COMP_SCAN, DBG_LOUD,("RtGetDualBandChannel: Normal case"));

		//RT_CHANNEL_DOMAIN_WW13_2G and after are in new format to support both 2.4G & 5G  
		if(pMgntInfo->ChannelPlan < RT_CHANNEL_DOMAIN_WW13_2G)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("DefaultChannelPlan[%d].Len %d \n", pMgntInfo->ChannelPlan, DefaultChannelPlan[pMgntInfo->ChannelPlan].Len));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelNum"));

			for(i = 0; i < DefaultChannelPlan[pMgntInfo->ChannelPlan].Len; i ++)
			{
				ChnlListEntryArray[i].ChannelNum  = DefaultChannelPlan[pMgntInfo->ChannelPlan].Channel[i];
				RT_TRACE(COMP_SCAN, DBG_LOUD,(" %d ",ChnlListEntryArray[i].ChannelNum));
			}

			ChannelLen = DefaultChannelPlan[pMgntInfo->ChannelPlan].Len;
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelLen %d\n", ChannelLen));
		}
		else
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("DefaultChannelPlan[%d].Len2_4G %d \n", pMgntInfo->ChannelPlan, DefaultChannelPlan[pMgntInfo->ChannelPlan].Len2_4G));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelNum"));
			
			for(i = 0; i < DefaultChannelPlan[pMgntInfo->ChannelPlan].Len2_4G; i++)
			{
				ChnlListEntryArray[i].ChannelNum  = DefaultChannelPlan[pMgntInfo->ChannelPlan].Channel2_4G[i];
				RT_TRACE(COMP_SCAN, DBG_LOUD,(" %d ",ChnlListEntryArray[i].ChannelNum));
			}

			RT_TRACE(COMP_SCAN, DBG_LOUD,("DefaultChannelPlan[%d].Len5G %d \n", pMgntInfo->ChannelPlan, DefaultChannelPlan[pMgntInfo->ChannelPlan].Len5G));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelNum"));

			ChannelLen = DefaultChannelPlan[pMgntInfo->ChannelPlan].Len2_4G;
			for(i = 0; i < DefaultChannelPlan[pMgntInfo->ChannelPlan].Len5G; i++)
			{
				ChnlListEntryArray[ChannelLen].ChannelNum  = DefaultChannelPlan[pMgntInfo->ChannelPlan].Channel5G[i];				
				RT_TRACE(COMP_SCAN, DBG_LOUD,(" %d ",ChnlListEntryArray[ChannelLen].ChannelNum));
				ChannelLen++;
			}

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ChannelLen %d\n", ChannelLen));
		}


		for(i = 0; i < ChannelLen; i ++)
		{
			// For 5G FCC Radar channel, default passive scan	
			if(DFS_5G_RADAR_CHANNEL(ChnlListEntryArray[i].ChannelNum))
				ChnlListEntryArray[i].ScanType = SCAN_PASSIVE;
			else
				ChnlListEntryArray[i].ScanType = SCAN_ACTIVE;
		}
	}

	RT_TRACE(COMP_SCAN, DBG_LOUD,("pMgntInfo->ChannelPlan %d \n",pMgntInfo->ChannelPlan));
	
	ChannelLen = RtCustomizeChannelList(pMgntInfo, pMgntInfo->ChannelPlan, ChannelLen, ChnlListEntryArray);

	return ChannelLen;
}


