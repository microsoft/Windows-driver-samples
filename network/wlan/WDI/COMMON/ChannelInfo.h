#ifndef __INC_CHANNEL_INFO_H
#define __INC_CHANNEL_INFO_H

#define GET_CHNL_INFO(_pMgntInfo)	((PRT_CHANNEL_INFO)((_pMgntInfo)->pChannelInfo))

// Determine if the Channel Operation is in progress

#define CHNL_OP_IN_PROGRESS(_pChnlInfo)	\
		((_pChnlInfo)->ChnlOp > CHNLOP_NONE) ? TRUE : FALSE

#define CHNL_RUN_ABOVE_40MHZ(_pMgntInfo)	\
		(((_pMgntInfo)->dot11CurrentChannelBandWidth >= CHANNEL_WIDTH_40)?TRUE:FALSE)
		
#define CHNL_RUN_ABOVE_80MHZ(_pMgntInfo)	\
		(((_pMgntInfo)->dot11CurrentChannelBandWidth >= CHANNEL_WIDTH_80)?TRUE:FALSE)

u1Byte	
CHNL_GetCenterFrequency(
	IN	u1Byte				Channel,
	IN	CHANNEL_WIDTH		ChnlBW,
	IN	EXTCHNL_OFFSET 		ExtChnlOffset
	);


EXTCHNL_OFFSET
CHNL_GetExt20OffsetOf5G(
	IN u1Byte			channel
	);


u1Byte
CHNL_IsLegalChannel(
	PADAPTER		Adapter,
	u8Byte			freq_channel
	);


BOOLEAN 
CHNL_IsLegal5GChannel(
	IN PADAPTER			Adapter,
	IN u1Byte			channel
	);

BOOLEAN
CHNL_AcquireOpLock(
	PADAPTER	Adapter,
	CHNLOP		ChnlOp
	);

VOID
CHNL_ReleaseOpLock(
	PADAPTER	Adapter
	);


BOOLEAN
CHNL_ValidForWirelessMode(
	u1Byte	channel,
	u2Byte	wirelessmode
	);


CHANNEL_WIDTH
CHNL_GetRegBWSupport(
	IN	PADAPTER	Adapter
);


VOID
CHNL_ChangeBwChnlFromPeerWorkitemCallBack(
	IN	PVOID			Context
	);


VOID
CHNL_SetBwChnlFromPeer(
	IN	PADAPTER			Adapter
	);


BOOLEAN
CHNL_GetCurrnetChnlInfo(
	IN		PADAPTER	Adapter,
	OUT 	pu1Byte 		pBuf,
	IN OUT 	pu4Byte		pBufLen
	);

VOID
CHNL_SetBwChnlCallback(
	IN	PRT_TIMER		pTimer
	);


CHANNEL_WIDTH
CHNL_CheckChnlPlanWithBW(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				PrimaryChnl,	
	IN	CHANNEL_WIDTH		Bandwidth,	
	IN	EXTCHNL_OFFSET		BwOffset	
	);


VOID
CHNL_SetBwChnl(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				PrimaryChnl,
	IN	CHANNEL_WIDTH		Bandwidth,
	IN	EXTCHNL_OFFSET		BwOffset
	);


VOID
CHNL_SetChnlInfoFromDestPort(
	IN	PADAPTER			pDestAdapter,
	IN	PADAPTER			pSrcAdapter
	);

BOOLEAN
CHNL_SwChnlAndSetBwInProgress(
	PADAPTER	Adapter
	);

u1Byte
RT_GetChannelNumber(
	IN	PADAPTER	Adapter
	);;

#endif
