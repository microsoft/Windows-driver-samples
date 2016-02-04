#ifndef __INC_MIMOPS_H
#define __INC_MIMOPS_H

VOID
SetSelfMimoPsMode(
	IN	PADAPTER 	Adapter, 
	IN	u1Byte 		NewMimoPsMode
	);

RT_STATUS
OnMimoPs(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
	);

VOID
SendMimoPsFrame(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pAddr,
	IN	u1Byte		NewMimoPsMode
	);

#endif