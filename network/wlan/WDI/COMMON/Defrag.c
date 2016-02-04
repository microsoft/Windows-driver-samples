#include "Mp_Precomp.h"


#if WPP_SOFTWARE_TRACE
#include "DeFrag.tmh"
#endif

VOID
DefragInitialize(
	PADAPTER		Adapter
	)
{
	DefragInit(Adapter->DefragArray, MAX_DEFRAG_PEER);
}

VOID
DefragReset(
	PADAPTER		Adapter
	)
{
	u2Byte	i;
	
	for(i=0;i<MAX_DEFRAG_PEER;i++)
	{
		if(Adapter->DefragArray[i].bUsed)
		{
			DefragEntryFree(&Adapter->DefragArray[i], Adapter);
		}
	}
}

PRT_RFD
DefragAddRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd,
	pu1Byte			pSenderAddr,
	u1Byte			TID,
	u2Byte			SeqNum,
	u1Byte			FragNum,
	BOOLEAN			bMoreFrag,
	BOOLEAN			bEncrypted
	)
{
	/*
	 *	Note:	Call this function when
	 *				1. More frag is set
	 *				2. Fragment number is not zero
	 *			The RFD passed in will be queued or freed after this function call.
	 *			ICV should be removed before this function.
	 *			
	 *	Return:	NULL, if no MSDU is complete
	 *			RFD list, if one MSDU is complete
	 *
	*/

	PRT_RFD			pRetRfd=NULL;
	PDEFRAG_ENTRY	pEntry;
	u2Byte			EncryptionOverhead=0;
	OCTET_STRING	frame;
	u1Byte			QosCtrlLen = 0;		// Added by Annie, 2006-01-09.
	u1Byte			HTCLen = 0;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;

	if( pRfd->Status.bIsQosData )
	{
		QosCtrlLen = sQoSCtlLng;
	}
	if( pRfd->Status.bContainHTC)
		HTCLen = sHTCLng;

	if(FragNum==0)
	{	// First frag, find a vacancy entry
		pEntry=DefragFindFreeEntry(Adapter->DefragArray, 	MAX_DEFRAG_PEER);

		if(pEntry==NULL)
		{
			// No free entry, do age function and try again
			DefragAge(
				Adapter->DefragArray, 
				MAX_DEFRAG_PEER,
				PlatformGetCurrentTime(),
				Adapter);

			pEntry=DefragFindFreeEntry(Adapter->DefragArray, 	MAX_DEFRAG_PEER);
		}
		
		if(pEntry==NULL)
		{
			// If we stall can not get a free entry, knock out the least rescent used (LRU)
			// entry to find one. 2006.07.25, by shien chang
			DefragFreeLRUEntry(Adapter->DefragArray,
								MAX_DEFRAG_PEER,
								Adapter);

			pEntry=DefragFindFreeEntry(Adapter->DefragArray, 	MAX_DEFRAG_PEER);

			RT_ASSERT(pEntry!=NULL, ("DefragAddRFD(): pEntry should not be NULL.\n"));
			
		}
		
			DefragEntrySetRFD(
				pEntry,
				pRfd,
				pSenderAddr,
				TID,
				SeqNum,
				FragNum);
			
	}
	else
	{	// 2~ frag
		pEntry=DefragSearch(
			Adapter->DefragArray, 
			MAX_DEFRAG_PEER,
			pSenderAddr,
			TID,
			SeqNum,
			FragNum);

		if(pEntry==NULL)
		{
			pRfd->bReturnDirectly = TRUE;
			ReturnRFDList(Adapter, pRfd);
			RT_ASSERT(Adapter->NumIdleRfd <= Adapter->NumRfd, ("DefragAddRFD(): Adapter->NumIdleRfd(%ld)\n", Adapter->NumIdleRfd));
		}
		else
		{
			if(bEncrypted && !pMgntInfo->SafeModeEnabled)
			{
				FillOctetString(frame, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->FragLength);
				SecGetEncryptionOverhead(
					Adapter,
					&EncryptionOverhead,
					NULL,
					NULL,
					NULL,
					TRUE,
					MacAddr_isMulticast(Frame_pDaddr(frame)));
			}
			//2 Remove header and MPDU head overhead (ex. IV)
			MAKE_RFD_OFFSET_AT_FRONT(pRfd, sMacHdrLng + QosCtrlLen + HTCLen + EncryptionOverhead)		// Add QosCtrlLen; Modified by Annie, 2006-01-09.
		
			DefragEntryAddRFD(pEntry, pRfd, FragNum);
			if(!bMoreFrag)
			{	//2 This MSDU is complete, return it
				pRetRfd=pEntry->pRfdHead;
				pEntry->bUsed=FALSE;
			}
		}	
	}
	return pRetRfd;
}


/*
 *	Note:	Following functions should not be called outside Defrag.c
 *
*/
VOID
DefragInit(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size
	)
{
	u2Byte	i;
	
	for(i=0;i<Size;i++)
	{
		pDefragArray[i].bUsed=FALSE;
	}
}

PDEFRAG_ENTRY
DefragSearch(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size,
	pu1Byte			pSenderAddr,
	u1Byte			TID,
	u2Byte			SeqNum,
	u1Byte			FragNum
	)
{
	u2Byte	i;
	
	for(i=0;i<Size;i++)
	{
		if(!pDefragArray[i].bUsed)
			continue;

		if(	SeqNum		==	pDefragArray[i].SeqNum				&&
			FragNum	==	(pDefragArray[i].LastFragNum+1)		&&
			TID			==	pDefragArray[i].TID					&&
			eqMacAddr(pSenderAddr, pDefragArray[i].SenderAddr)
			)
		{
			return &pDefragArray[i];
		}	
	}

	return NULL;
}

VOID
DefragRemoveOldest(
	PADAPTER	Adapter
	)
{
	u2Byte			i;
	u2Byte			Oldest_Frag = MAX_DEFRAG_PEER;
	PDEFRAG_ENTRY	pDefragArray = Adapter->DefragArray;
	
	for(i=0; i<MAX_DEFRAG_PEER; i++)
	{
		if(!pDefragArray[i].bUsed)
			continue;

		if(Oldest_Frag == MAX_DEFRAG_PEER)
		{
			Oldest_Frag = i;
			continue;
		}
				
		if( pDefragArray[Oldest_Frag].usMaxLifeTimeStamp > pDefragArray[i].usMaxLifeTimeStamp )
		{
			Oldest_Frag = i;
		}	
	}

	if( Oldest_Frag != MAX_DEFRAG_PEER )
		DefragEntryFree(&pDefragArray[Oldest_Frag], Adapter);
}

PDEFRAG_ENTRY
DefragFindFreeEntry(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size
	)
{
	u2Byte	i;
	
	for(i=0;i<Size;i++)
	{
		if(!pDefragArray[i].bUsed)
			return &pDefragArray[i];
	}

	return NULL;
}

VOID
DefragAge(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size,
	u8Byte			usCurrentTime,
	PADAPTER		Adapter
	)
{
	u2Byte	i;
	
	for(i=0;i<Size;i++)
	{
		if(!pDefragArray[i].bUsed)
			continue;

		if(usCurrentTime > pDefragArray[i].usMaxLifeTimeStamp)
			DefragEntryFree(&pDefragArray[i], Adapter);
	}
}

VOID
DefragEntrySetRFD(
	PDEFRAG_ENTRY	pEntry,
	PRT_RFD			pRfd,
	pu1Byte			pSenderAddr,
	u1Byte			TID,
	u2Byte			SeqNum,
	u1Byte			FragNum	
	)
{
	RT_ASSERT(FragNum==0, ("DefragEntrySetRFD() with nonzero FragNum !!\n"));

	pEntry->bUsed=TRUE;
	cpMacAddr(pEntry->SenderAddr, pSenderAddr);
	pEntry->TID=TID;
	pEntry->SeqNum=SeqNum;
	pEntry->LastFragNum=FragNum;
	pEntry->usLastArriveTimeStamp=PlatformGetCurrentTime();
	pEntry->usMaxLifeTimeStamp=pEntry->usLastArriveTimeStamp + MaxDefragLifeTime;
	pEntry->pRfdHead=pRfd;
	pEntry->pRfdTail=pRfd;
}

VOID
DefragEntryAddRFD(
	PDEFRAG_ENTRY	pEntry,
	PRT_RFD			pRfd,
	u1Byte			FragNum	
	)
{
	RT_ASSERT(FragNum==(pEntry->LastFragNum+1), ("DefragEntryAddRFD() with wrong FragNum !!\n"));
	
	pEntry->LastFragNum=FragNum;

	// Increment fragment counter
	pEntry->pRfdHead->nTotalFrag++;

	// Add packet length
	pEntry->pRfdHead->PacketLength+=pRfd->FragLength;
	
	// Update arrival time
	pEntry->usLastArriveTimeStamp=PlatformGetCurrentTime();
	
	// Insert to tail
	pEntry->pRfdTail->NextRfd=pRfd;
	pEntry->pRfdTail=pRfd;
}

VOID
DefragEntryFree(
	PDEFRAG_ENTRY	pEntry,
	PADAPTER		Adapter	
	)
{
	ReturnRFDList(Adapter, (PRT_RFD)pEntry->pRfdHead);
	pEntry->bUsed=FALSE;
}

VOID
DefragRecycleRFD(
	PADAPTER		Adapter
	)
{
	// There may be many ways to recycle RFDs.
	// Now we only free the LRU RFDs in DefragArray. 2006.07.25, by shien chang.
	PADAPTER pAdapter;
	pAdapter = GetDefaultAdapter(Adapter);	
	DefragFreeLRUEntry(pAdapter->DefragArray, MAX_DEFRAG_PEER, pAdapter);
}

VOID
DefragFreeLRUEntry(
	PDEFRAG_ENTRY	pDefragArray,
	u2Byte			Size,
	PADAPTER		Adapter
	)
{
	u2Byte 	i, target=Size;
	PDEFRAG_ENTRY	pEntry;

	for (i=0; i<Size; i++)
	{
		if (pDefragArray[i].bUsed)
		{
			target=i;
			break;
		}
	}

	if (target != Size)
	{
		for (i=target+1; i<Size; i++)
		{
			if (!pDefragArray[i].bUsed)
				continue;
		
			if (pDefragArray[i].usLastArriveTimeStamp < pDefragArray[target].usLastArriveTimeStamp)
				target = i;
		}

		pEntry = &pDefragArray[target];
		ReturnRFDList(Adapter, (PRT_RFD)pEntry->pRfdHead);

		pEntry->bUsed=FALSE;
	}
}
