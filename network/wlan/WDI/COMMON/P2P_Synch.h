//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_LOCK_H
#define __INC_P2P_LOCK_H

	typedef NDIS_SPIN_LOCK P2P_LOCK;

VOID
p2p_InitLock(
	IN  P2P_LOCK 			*lock
	);

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
VOID
_IRQL_raises_(DISPATCH_LEVEL)
p2p_AcquireLock(
	IN  P2P_LOCK 			*lock
	);

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
VOID
_IRQL_requires_(DISPATCH_LEVEL)
p2p_ReleaseLock(
	IN  P2P_LOCK 			*lock
	);

VOID
p2p_FreeLock(
	IN  P2P_LOCK 			*lock
	);

#endif	// #ifndef __INC_P2P_LOCK_H