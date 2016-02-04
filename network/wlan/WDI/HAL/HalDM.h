/*****************************************************************************
 *	Copyright(c) 2011,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:		HALDM.h
 *
 *
 * Note:		The header file records 
 *			
 *
 * Export:		
 *
 * Abbrev:			
 *
 * History:		
 *	Data		Who		Remark 
 *	09/22/2011  MHC    	Create initial version.
 * 
******************************************************************************/
#ifndef	__HALDM_H__
#define __HALDM_H__




//============================================================
// Description:
//
// This file is for 92CE/92CU dynamic mechanism only
//
//
//============================================================

//============================================================
// structure and define
//============================================================
#define		DM_Type_ByFW				0
#define		DM_Type_ByDriver			1

//============================================================
// function prototype
//============================================================
void	InitHalDm(								
    IN	PADAPTER	Adapter	
    );
void    DeInitHalDm(
    IN PADAPTER Adapter
    );


void	HalDmWatchDog(						
   IN	PADAPTER	Adapter	
    );


u1Byte
dm_initial_gain_MinPWDB(
	IN	PADAPTER	pAdapter
);

void 
dm_CCK_PacketDetectionThresh(
	IN	PADAPTER	pAdapter
);

VOID 
dm_CCK_PacketDetectionThresh_DMSP(
	IN	PADAPTER	pAdapter
);


VOID
DM_Write_DIG_DMSP(
	IN	PADAPTER	pAdapter
);


extern	VOID
DM_AggModeSwitch(
	IN	PADAPTER	Adapter
	);


BOOLEAN
dm_DualMacGetParameterFromBuddyAdapter(
		PADAPTER	Adapter
);


//
// 2011/02/10 MH Add 92D relative definition in the section.
//
VOID
DM_Write_DIG_DMSP(
	IN	PADAPTER	pAdapter
);
//
// 2011/02/10 MH Add 92D relative definition in the section.
//

VOID
odm_FindMinimumRSSI_Dmsp(
	IN	PADAPTER	pAdapter
);

VOID
DM_OverwriteDig(
	IN	PADAPTER	Adapter,
	IN	u1Byte		digValue
	);

VOID
DM_OverwriteCckCcaThres(
	IN	PADAPTER	Adapter,
	IN	u1Byte		ccaThres
	);

VOID	
DM_HwPbcGPIOChk(	
	IN	PADAPTER	pAdapter
	);

#endif	//__HALDM_H__

