#include "Mp_Precomp.h"


#define CAM_CONTENT_COUNT 			8
#define CAM_PAIRWISE_KEY_POSITION		4

//   0~3 											: Default Key 
//   4											: Default Port Station Key ( Pairwise key )
//   5~HALF_CAM_ENTRY - MAX_BT_ASOC_ENTRY_NUM 	: TDSL 
//	HALF_CAM_ENTRY ~ TOTAL_CAM_ENTRY-1 		: ExtAP( VWifi , XP-ExtAP and Win7/8 ExtAP )
//   CAM_EXTSTA_PAIRWISE_KEY_POSITION			: ExtSta key ( Pairwise key )
#define CAM_TDSL_START_INDEX					5

#define CFG_DEFAULT_KEY  	BIT5
#define CFG_VALID        		BIT15


//add for AP HW , CCW

u1Byte AP_CamAddOneEntry(
	PADAPTER     Adapter,
	u1Byte 			*pucMacAddr, 
	u4Byte 			ulKeyId, 
	u4Byte 			ulEncAlg, 
	u4Byte 			ulUseDK, 
	u1Byte 			*pucKey
);

u1Byte AP_Setkey(
	PADAPTER     Adapter,
	u1Byte 			*pucMacAddr, 
	u4Byte 			ulKeyId, 
	u4Byte 			ulEncAlg, 
	u4Byte 			ulUseDK, 
	u1Byte 			*pucKey);


u4Byte AP_FindFreeEntry(
	PADAPTER     Adapter,
	pu1Byte		Macaddr
	);

void AP_ClearAllKey(
	PADAPTER     Adapter);


void AP_RemoveKey(
	PADAPTER		Adapter,
	PRT_WLAN_STA	pSTA);

//Add for AP enc ,CCW
//======================
u1Byte CamAddOneEntry(
	PADAPTER     Adapter,
	u1Byte 			*pucMacAddr, 
	u4Byte 			ulKeyId, 
	u4Byte			ulEntryId,	
	u4Byte 			ulEncAlg, 
	u4Byte 			ulUseDK, 
	u1Byte 			*pucKey
);
int CamDeleteOneEntry(
		PADAPTER	Adapter,
		u1Byte 			*pucMacAddr, 
		u4Byte 			ulKeyId
);

void CAMtest(
	PADAPTER     	Adapter,
	u1Byte   			ucTestItem);

void test_cam(
	PADAPTER     Adapter);

//----------------------------------------
//Function declartion for internal usage of CAM.c
//----------------------------------------

u1Byte CAM_find_usable(
	PADAPTER     Adapter);

void CAM_program_entry(
	PADAPTER     	Adapter,
	u4Byte	 			iIndex, 
	u1Byte			*pucMacad,
	u1Byte			*pucKey128, 
	u2Byte 			usConfig);

void CAM_read_entry(
	PADAPTER     	Adapter,
	u4Byte	 			iIndex, 
	u1Byte			*pucMacad,
	u1Byte			*pucKey128, 
	u2Byte			*usConfig);

void CAM_read_mac_config(
		PADAPTER     	Adapter,
		u1Byte 			ucIndex, 
		u1Byte			*pucMacad, 
		u2Byte			*pusTempConfig);

void CAM_mark_invalid(
	PADAPTER     	Adapter,	
	u1Byte 			ucIndex);

void CAM_empty_entry(
	PADAPTER     	Adapter,	
	u1Byte 			ucIndex);

void CAM_read_entry(
	PADAPTER     	Adapter,
	u4Byte	 		index, 
	u1Byte			*macad,
	u1Byte			*key128, 
	u2Byte			*config);

void CAM_debug_print(
	u1Byte			*pucMac,
	u1Byte			*pucKey,
	u2Byte 			usConfig);
VOID
CamDumpAll(	PADAPTER     	Adapter);

VOID
CamDumpAllCallbackWorkItem(
	IN	PVOID		pContext);

VOID
CamResetAllEntry(PADAPTER Adapter);


VOID 
AP_CamRestoreAllEntry(
	PADAPTER Adapter);


VOID 
CamRestoreAllEntry(PADAPTER Adapter);


void build_cam(
	PADAPTER     	Adapter);

void loopcam(
	PADAPTER     	Adapter
);
void SingleCam(
	PADAPTER     	Adapter);

void build_16_cam(
	PADAPTER     	Adapter);

void  BurstTxCAM(void);
//----IF Emily
void CAM_Policy_Verify(void);


//vivi added for new cam search flow, 20091028.
u4Byte Sta_FindFreeEntry(
	PADAPTER	Adapter,
	pu1Byte		pMacAddr,
	u4Byte 		KeyIndex, 
	u4Byte 		ulEncAlg, 
	u1Byte 		*pucKey
);




