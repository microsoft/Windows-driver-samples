/******************************************************************************
 * 
 *     (c) Copyright  2008, RealTEK Technologies Inc. All Rights Reserved.
 * 
 * Module:	HalRf.h	( Header File)
 * 
 * Note:	Collect every HAL RF type exter API or constant.	 
 *
 * Function:	
 * 		 
 * Export:	
 * 
 * Abbrev:	
 * 
 * History:
 * Data			Who		Remark
 * 
 * 09/25/2008	MHC		Create initial version.
 * 
 * 
******************************************************************************/
/* Check to see if the file has been included already.  */


/*--------------------------Define Parameters-------------------------------*/

//
// For RF 6052 Series
//
/*--------------------------Define Parameters-------------------------------*/

/*------------------------------Define structure----------------------------*/ 

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/


//
// RF RL6052 Series API
//
void		PHY_RF6052SetBandwidth8723B(	
										IN	PADAPTER				Adapter,
										IN	CHANNEL_WIDTH		Bandwidth);	

RT_STATUS	PHY_RF6052_Config_8723B(	IN	PADAPTER		Adapter	);

/*--------------------------Exported Function prototype---------------------*/


/* End of HalRf.h */
