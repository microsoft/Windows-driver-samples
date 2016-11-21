/*****************************************************************************
 *	Copyright 2011,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:		WPS.h	(RTL8190  Header H File)
 *
 *
 * Note:		Declare some variable which will be used by any debug command.
 *			
 *
 * Export:		
 *
 * Abbrev:			
 *
 * History:		
 *	Data		Who		Remark 
 *	06/15/2011  MHC   	Create initial version in different filed.
 *						Move WPS relative define to the header file.
 * 
 *****************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef	__WPS_H__
#define __WPS_H__


/*--------------------------Define Parameters-------------------------------*/

#define MAX_SIMPLE_CONFIG_IE_LEN		255
#define MAX_SIMPLE_CONFIG_IE_LEN_V2	512
#define MAX_SIMPLE_CONFIG_IE_ASSEMBLE_LEN	512
#define MAX_SIMPLE_Return_PacketLength 2500
#define MAX_SUB_WPS_IE_LENGTH		251

// Define support WPS IE version
#define	SUPPORT_WPS_INFO_VERSION	0x01

/*--------------------------Define structure--------------------------------*/

typedef enum _WPS_INFO_OPCODE{
	WPS_INFO_NO_OPERATION		 = 0,
	WPS_INFO_SUPPORT_VERSION	 = 1,
	WPS_INFO_BEACON_IE			 = 2,
	WPS_INFO_ASOCREQ_IE			 = 3,
	WPS_INFO_ASOCRSP_IE			 = 4,
	WPS_INFO_PROBEREQ_IE			 = 5,
	WPS_INFO_PROBERSP_IE			 = 6,
	WPS_INFO_QUERY_INFO			 = 7,
}WPS_INFO_OPCODE;

/*------------------------------Define structure----------------------------*/ 


//
// Simple Config. by CCW copy from 818x
//
typedef struct _SIMPLE_CONFIG_T{
	DECLARE_RT_OBJECT(_SIMPLE_CONFIG_T);

	BOOLEAN		bEnabled; // TRUE if simple config is enabled and we have to handle corresponding IE in scan, (re)association.
	u2Byte		IELen; // Length of our Simple Config IE.
	u1Byte		IEBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2]; // Buffer to keep our Simple Config IE.
	BOOLEAN		bAPEnabled;		// For AP mode WPS
	u1Byte		WpsIeVersion;

	u1Byte		InfoCtrl;
	u2Byte		ieBeaconLen;
	u1Byte		ieBeaconBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2];
	u2Byte		ieAsocReqLen;
	u1Byte		ieAsocReqBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2];
	u2Byte		ieAsocRspLen;
	u1Byte		ieAsocRspBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2];
	u2Byte		ieProbeReqLen;
	u1Byte		ieProbeReqBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2];
	u2Byte		ieProbeRspLen;
	u1Byte		ieProbeRspBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2];
	
	u1Byte		AssembleIEBuf[MAX_SIMPLE_CONFIG_IE_ASSEMBLE_LEN]; // Buffer to assemble fragment IE.
	u2Byte		AssembleIELen;
	u1Byte		RecieveBuf[MAX_SIMPLE_Return_PacketLength];//Save The packet we Recieve
	u2Byte		RecieveLength;
	BOOLEAN		bRecievePacket;//Flag to check we recieve Packet or not
	BOOLEAN       bFragmentIE;
}SIMPLE_CONFIG_T, *PSIMPLE_CONFIG_T;

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/

#if WPS_SUPPORT
#define GET_SIMPLE_CONFIG(__pMgntInfo) 					((PSIMPLE_CONFIG_T)((__pMgntInfo)->pSimpleConfig))
#define GET_SIMPLE_CONFIG_ENABLED(__pMgntInfo)		(GET_SIMPLE_CONFIG(__pMgntInfo)->bEnabled)
#define GET_SIMPLE_CONFIG_IE_LEN(__pMgntInfo)			(GET_SIMPLE_CONFIG(__pMgntInfo)->IELen)
#define SET_SIMPLE_CONFIG_IE_LEN(__pMgntInfo, __len)	( (GET_SIMPLE_CONFIG(__pMgntInfo)->IELen) = (__len) )
#else
#define GET_SIMPLE_CONFIG(__pMgntInfo)					NULL
#define GET_SIMPLE_CONFIG_ENABLED(__pMgntInfo)		FALSE
#define GET_SIMPLE_CONFIG_IE_LEN(__pMgntInfo)			0
#define SET_SIMPLE_CONFIG_IE_LEN(__pMgntInfo, __len)
#endif

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
VOID
WPS_Init(
	IN	PADAPTER	Adapter
	);

BOOLEAN
WPS_OnAsocReq(
	IN		PADAPTER	Adapter,
	OUT		BOOLEAN		*pbIndicate
	);

BOOLEAN
WPS_MgntActSet_802_11_SSID(
	IN		PADAPTER	Adapter
	);

VOID
WPS_ConstructBeaconFrame(
	IN		PADAPTER	Adapter
	);

VOID
WPS_AppendElement(
	IN	PADAPTER			Adapter,
	IN	POCTET_STRING		posFrame,
	IN	BOOLEAN				bCheckFrag,
	IN	WPS_INFO_OPCODE	frameType
	);


VOID
WPS_CopyRxEAPPacket(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

RT_STATUS
WPS_QueryRxEAPPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength,
	OUT	pu4Byte			BytesWritten,
	OUT	pu4Byte			BytesNeeded
	);

/*--------------------------Exported Function prototype---------------------*/


#endif	/* __WPS_H__ */

/* End of WPS.h */
