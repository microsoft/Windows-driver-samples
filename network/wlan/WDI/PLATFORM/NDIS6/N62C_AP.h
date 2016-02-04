/*++

Copyright (c) Realtek . All rights reserved.

Module Name:
    N62C_AP.h

Abstract:
    Win 7 AP mode releated 
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    04-22-2008    Created
    
Notes:

--*/
#pragma once
    
#ifndef _N62_AP__H
#define _N62_AP__H


#if 0
/** Reference AP port */
LONG
N62CAPRefPort(
	IN	PADAPTER		pAdapter
	);

/** Dereference AP port */
LONG
N62CAPDerefPort(
	IN	PADAPTER		pAdapter
	);
/*
STA_PORT_STATE
ApGetStaPortState(
	IN	PRT_WLAN_STA		pEntry
    );

STA_PORT_STATE
ApSetStaPortState(
	IN	PRT_WLAN_STA		pEntry,
	IN	STA_PORT_STATE 		NewPortState
    );
*/
#endif

NDIS_STATUS
N62CStartApMode(
	IN PADAPTER	Adapter
	);

NDIS_STATUS
N62CStopApMode(
	IN PADAPTER	Adapter
	);

VOID
N62CApIndicateStatus(
	IN	PADAPTER		pAdapter,
	IN	NDIS_STATUS		GeneralStatus,
	IN	PVOID			RequestID,
	IN	PVOID			StatusBuffer,
	IN	UINT			StatusBufferSize
	);
VOID 
N62CApIndicateStopAp(
	IN	PADAPTER		Adapter
	);
VOID 
N62CApIndicateCanSustainAp(
	IN	PADAPTER		Adapter
	);
VOID 
N62CAPIndicateFrequencyAdopted(
	IN	PADAPTER		Adapter
    );
VOID
N62CAPIndicateIncomAssocStart(
	IN	PADAPTER		Adapter
	);

VOID
N62CAPIndicateIncomAssocComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status
	);

VOID
N62CAPIndicateIncomAssocReqRecv(
	IN	PADAPTER		Adapter
	);

VOID
N62CAPIndicateDisassociation(
	IN	PADAPTER		Adapter,
	IN	u2Byte			reason
	);

BOOLEAN
N62CAPInComingAssocDecsion(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu	
	);

NDIS_STATUS
N62CAPSetStaPortState(
	IN PADAPTER				Adapter,
	IN PDOT11_MAC_ADDRESS	PeerMacAddr,
	IN BOOLEAN 				PortOpen
  	 );

VOID
N62CResetAPVariables(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			IsApMode
	);

VOID
N62CApInitializeIndicateStateMachine(
	IN PN6_INDICATE_STATE_MACHINE IndicationEngine
	);


VOID
N62CAPClearStateBeforeSleep(
		PADAPTER	Adapter
	);

#endif // _N62_AP_H

