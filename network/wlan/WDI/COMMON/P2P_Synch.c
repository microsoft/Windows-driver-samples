#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Synch.tmh"
#endif

#include "P2P_Synch.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
VOID
p2p_InitLock(
	IN  P2P_LOCK 			*lock
	)
{
	NdisAllocateSpinLock(lock);
}

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
_IRQL_raises_(DISPATCH_LEVEL)
VOID
p2p_AcquireLock(
	IN  P2P_LOCK 			*lock
	)
{
	NdisAcquireSpinLock(lock);
}

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
_IRQL_requires_(DISPATCH_LEVEL)
VOID
p2p_ReleaseLock(
	IN  P2P_LOCK 			*lock
	)
{
	NdisReleaseSpinLock(lock);
}

VOID
p2p_FreeLock(
	IN  P2P_LOCK 			*lock
	)
{
	NdisFreeSpinLock(lock);
	lock = NULL;
}

#endif
