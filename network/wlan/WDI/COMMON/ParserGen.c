/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	ParserGen.c
	
Abstract:
	Parse MSDU to match the specified type, fetch fixed offset.
	    
Major Change History:
	When		Who					What
	---------- ---------------   -------------------------------
	2007-06-11	shienchang			Create.
	2008-03-11	Bruce				Modify from 818xB.
	
--*/

#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "ParserGen.tmh"
#endif

// Global protocols and the corresponding actions.
static GP_RULE GPGlobalRule[] =
{
	{ PROTO_ETHERNET,		PROTO_IP,			12,		2,		GPParserHandlerEthernet },
	{ PROTO_WLAN_80211,		PROTO_WLAN_LLC,		0,		2,		GPParserHandlerWlan },
	{ PROTO_WLAN_LLC,		PROTO_IP,			6,		2,		GPParserHandlerWlanLlc },
	{ PROTO_IP, 			PROTO_TCP,			9,		1,		GPParserHandlerIP },
	{ PROTO_IP,				PROTO_UDP,			9,		1,		GPParserHandlerIP },
	{0},
};

// The SIP protocol definitions and the corresponding actions.
static GP_RULE GPVoWlanSipRule[] =
{
	{ PROTO_UDP,		PROTO_SIP_INVITE,	UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleUDP2SIP },
	{ PROTO_UDP,		PROTO_SIP_STATUS,	UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleUDP2SIP },
	{ PROTO_UDP,		PROTO_SIP_BYE,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleUDP2SIP },
	{ PROTO_SIP_INVITE,	PROTO_SIP_SDP,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleSIP2SDP },
	{ PROTO_SIP_STATUS,	PROTO_SIP_SDP,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleSIP2SDP },
	{ PROTO_SIP_STATUS, PROTO_UNKNOWN,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleSIPSTATUS },
	{ PROTO_SIP_SDP,	PROTO_UNKNOWN,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleSDP },
	{ PROTO_SIP_BYE,	PROTO_UNKNOWN,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleSIPBYE },
	{ PROTO_UDP,		PROTO_RTP,			UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleUDP2RTP },
	{ PROTO_RTP,		PROTO_RTP_G711,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleRTP2G711 },
	{ PROTO_RTP_G711,	PROTO_UNKNOWN,		UNKNOWN_SIG, UNKNOWN_SIG,  GPParserCustomHandleG711 },
	{0},
};
//=============================================================================
//	Prototype of protected function.
//=============================================================================
TR_ACTION
ParseBuffer(
	IN		PADAPTER			Adapter,
	IN		PGPPARSE_TOKEN		pToken
	);

VOID
RuleMatchingProcess(
	IN		PADAPTER			Adapter,
	IN		PGPPARSE_TOKEN		pToken,
	IN		PGENERIC_PARSER		pParser
	);

BOOLEAN
MatchingByRule(
	IN		PADAPTER			Adapter,
	IN		PGP_RULE			pRule,
	IN		PGPPARSE_TOKEN		pToken,
	IN		PGENERIC_PARSER		pParser
	);

BOOLEAN
GetSignatureAndAdvanceToNextProtocol(
	IN	PGPPARSE_TOKEN		pToken,
	IN	u4Byte				DataOffset,
	IN	u4Byte				HeaderLength,
	IN	PVOID				pDataBuffer,
	IN	u4Byte				DataBufLength
	);

BOOLEAN
GetNextLine(
	IN		PGPPARSE_TOKEN	pToken,
	IN		BOOLEAN			bNoAdvance,
	OUT		pu1Byte			pOutBuffer,
	IN		u4Byte			OutBufferLen
	);

BOOLEAN
UngetLine(
	IN		PGPPARSE_TOKEN	pToken
	);

BOOLEAN
AdvanceAndGetDataByOffset(
	IN	PGPPARSE_TOKEN		pToken,
	IN	u4Byte				DataOffset,
	IN	BOOLEAN				bNoAdvance,
	OUT	PVOID				pDataBuffer,
	IN	u4Byte				DataBufLength
	);

//=============================================================================
//	End of Prototype of protected function.
//=============================================================================

//=============================================================================
// Private utility function.
//=============================================================================

#if DBG
//
//	Description:
//		Debug routine of protocol.
//	2007.07.10, by shien chang.
//
VOID
DbgProtocol(
	IN		u4Byte				protocol
	)
{
	switch (protocol)
	{
		case PROTO_UNKNOWN:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("\n"));
			break;
		case PROTO_ETHERNET:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("ethernet -> "));
			break;
		case PROTO_WLAN_80211:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("wlan -> "));
			break;
		case PROTO_WLAN_LLC:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("llc -> "));
			break;
		case PROTO_IP:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("ip -> "));
			break;
		case PROTO_TCP:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("tcp -> "));
			break;
		case PROTO_UDP:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("udp -> "));
			break;
		case PROTO_RTP:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("rtp -> "));
			break;
		case PROTO_SIP_INVITE:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("sip invite -> "));
			break;
		case PROTO_SIP_STATUS:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("sip status -> "));
			break;
		case PROTO_SIP_BYE:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("sip bye -> "));
			break;
		case PROTO_SIP_NOTIFY:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("sip notify -> "));
			break;
		case PROTO_SIP_SDP:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("sip sdp -> "));
			break;
		case PROTO_RTP_G711:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("rtp g711 -> "));
			break;
		default:
			RT_TRACE(COMP_CCX, DBG_TRACE, ("protocol: %d -> ", protocol));
			break;
	}
}
#endif


//
//	Description:
//		Request every parser to parse the tx or rx packet.
//	2007.07.10, by shien chang.
//
TR_ACTION
ParseBuffer(
	IN		PADAPTER			Adapter,
	IN		PGPPARSE_TOKEN		pToken
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	u4Byte				i;
	PGENERIC_PARSER		pParser;
	TR_ACTION			trAction = TR_ACTION_CONTINUE;

	for (i=0; i < MAX_PARSER_NUM; i++)
	{
		pParser = *(((PGENERIC_PARSER*)pMgntInfo->pGenericParser) + i);
		if ( pParser )
		{
			//
			// Drive the parser go through the registered rules.
			//
			if ( pParser->Gate && pParser->Gate(Adapter, pParser) )
			{
				RuleMatchingProcess(Adapter, pToken, pParser);
				if (pParser->Action != NULL)
					trAction = pParser->Action(Adapter, pToken, pParser);
				if (trAction != TR_ACTION_CONTINUE)
					break;
			}
		}
	}

	return trAction;
}

//
//	Description:
//		This routine drive parser according to parsing rules.
//	2007.07.10, by shien chang.
//
VOID
RuleMatchingProcess(
	IN		PADAPTER			Adapter,
	IN		PGPPARSE_TOKEN		pToken,
	IN		PGENERIC_PARSER		pParser
	)
{
	PGP_RULE	pRule = NULL;
	BOOLEAN		bMatched = FALSE;
	PGP_RULE	pRuleSet[2];
	u4Byte		i=0;

	pRuleSet[0] = pParser->pParseRules;
	pRuleSet[1] = GPGlobalRule;
	
	while ( i < sizeof(pRuleSet) / sizeof(PGP_RULE) )
	{		
		//
		// Go through parser's rules & global rules.
		//
		bMatched = FALSE;
		
		for (pRule = pRuleSet[i];
			(pRule != NULL) && (pRule->CurrProtocol != 0);
			pRule ++)
		{
			if (pRule->CurrProtocol == pToken->currProtocol)
			{
				bMatched = MatchingByRule(Adapter,
										pRule,
										pToken,
										pParser);
				if (bMatched)
					break;	
			}
		}

		if (bMatched)
		{
			if (pRule->NextProtocol != PROTO_UNKNOWN)
			{
				i = 0;
			}
			else
			{
				break;
			}
		}		
		else
			i ++;
	}

	if (!bMatched) 
	{
		DbgProtocol(pToken->currProtocol);
		if (pToken->ProtocolCount < MAX_PROTOCOL_NUM)
			pToken->ProtocolSuite[pToken->ProtocolCount++] = pToken->currProtocol;
	}
	DbgProtocol(PROTO_UNKNOWN);
}

//
//	Description:
//		Try to match a rule.
//	2007.7.13, by shien chang.
//
BOOLEAN
MatchingByRule(
	IN		PADAPTER			Adapter,
	IN		PGP_RULE			pRule,
	IN		PGPPARSE_TOKEN		pToken,
	IN		PGENERIC_PARSER		pParser
	)
{
	u1Byte		SigBuffer[MAX_SIG_LENGTH];
	BOOLEAN		bApply = FALSE;
	BOOLEAN		bMatched = FALSE;
	BOOLEAN 	bAdv=FALSE;	
	u4Byte		currIndex, currOffset;
	u4Byte		hdrLen=0;
	
	if ( pRule->SigLength <= MAX_SIG_LENGTH)
	{
		currIndex = pToken->currBufferIndex;
		currOffset = pToken->currBufferOffset;
		hdrLen = PROTO_UNKNOWN_HDRLEN;
							
		//
		// Check signature.
		// case 1: if signature is unknown, invoke the customer routine
		//            and advance to next protocol if success.
		// case 2: if signature is known, get the signature first and then
		//            invoke the costomer routine.
		//
		if ( pRule->SigLength == UNKNOWN_SIG )
		{
			bApply = pRule->ParseHandler(
						Adapter,
						pParser->ParserContext,
						pToken->currProtocol,
						pRule->NextProtocol,
						NULL,
						0,
						pToken,
						&hdrLen);

			if (hdrLen == PROTO_UNKNOWN_HDRLEN)
			{
				pToken->currBufferIndex = currIndex;
				pToken->currBufferOffset = currOffset;
				bAdv = TRUE;
			}
			else if (bApply)
			{
				bAdv = GetSignatureAndAdvanceToNextProtocol(
							pToken,
							pRule->SigStart,
							hdrLen,
							NULL,
							0);
			}
							
			if (bApply && bAdv)
			{
				DbgProtocol(pToken->currProtocol);
				if (pToken->ProtocolCount < MAX_PROTOCOL_NUM)
					pToken->ProtocolSuite[pToken->ProtocolCount++] = pToken->currProtocol;
							
				if (pRule->NextProtocol != PROTO_UNKNOWN)
					pToken->currProtocol = pRule->NextProtocol;
				bMatched = TRUE;
			}
							
		}
		else
		{
			if (AdvanceAndGetDataByOffset(
						pToken,
						pRule->SigStart,
						TRUE,
						SigBuffer,
						pRule->SigLength))
			{
				bApply = pRule->ParseHandler(
							Adapter,
							pParser->ParserContext,
							pToken->currProtocol,
							pRule->NextProtocol,
							SigBuffer,
							pRule->SigLength,
							pToken,
							&hdrLen);

				if (hdrLen == PROTO_UNKNOWN_HDRLEN)
				{
					pToken->currBufferIndex = currIndex;
					pToken->currBufferOffset = currOffset;
					bAdv = TRUE;
				}
				else if (bApply)
				{
					bAdv = GetSignatureAndAdvanceToNextProtocol(
								pToken,
								pRule->SigStart,
								hdrLen,
								NULL,
								0);
				}

				if (bApply && bAdv)
				{
					DbgProtocol(pToken->currProtocol);
					if (pToken->ProtocolCount < MAX_PROTOCOL_NUM)
						pToken->ProtocolSuite[pToken->ProtocolCount++] = pToken->currProtocol;
								
					if (pRule->NextProtocol != PROTO_UNKNOWN)
						pToken->currProtocol = pRule->NextProtocol;
					bMatched = TRUE;
				}
			}			
					
		}
	}
	else
	{
		RT_TRACE(COMP_CCX, DBG_WARNING, ("Signature length is too long (limit = %d)\n", MAX_SIG_LENGTH));
	}

	return bMatched;
}

//
//	Description:
//		Get signature at DataOffset and advance to the begining of
//		next protocol.
//
//	Arguments:
//		pToken - the parsing token which maintain by parser.
//		DataOffset - start offset of signature.
//		HeaderLength -length of current protocol.
//		pDataBuffer - buffer to store the read signature.
//		DataBufLength - lenght of signature to be read.
//
//	Return:
//		TRUE if read success.
//		FALSE if read failed.
//
//	Note:
//		If pDataBuffer is NULL or DataBufLength is zero, this routine will
//		just advance to DataOffset and doesn't read any data.
//
//	2007.07.10, by shien chang.
//
BOOLEAN
GetSignatureAndAdvanceToNextProtocol(
	IN	PGPPARSE_TOKEN		pToken,
	IN	u4Byte				DataOffset,
	IN	u4Byte				HeaderLength,
	IN	PVOID				pDataBuffer,
	IN	u4Byte				DataBufLength
	)
{
	
	if (pDataBuffer!=NULL && DataBufLength!=0)
	{
		if (AdvanceAndGetDataByOffset(pToken, 
									DataOffset, 
									FALSE,
									pDataBuffer, 
									DataBufLength))
		{
			if (AdvanceAndGetDataByOffset(pToken,
										HeaderLength-DataOffset,
										FALSE,
										NULL,
										0))
			{
				return TRUE;
			}
		}
	}
	else
	{
		if (AdvanceAndGetDataByOffset(pToken,
									HeaderLength,
									FALSE,
									NULL,
									0))
		{
			return TRUE;
		}		
	}

	return FALSE;
}

//
//	Description:
//		Get next line of protocol such as SIP, SDP ... which are
//		end with every line a "0x0d 0x0a" seprator. This routine will
//		advance to begining of next line after current line has read
//		according to bNoAdvance flag.
//
//	Arguments:
//		pToken - the parsing token which maintain by parser.
//		bNoAdvance - TRUE: advance to next line.
//					FALSE: don't advance to next line.
//		pOutBuffer - buffer to store the read data.
//		OutBufferLen - length of pOutBuffer.
//
//	Return:
//		TRUE if read success.
//		FALSE if read failed.
//
//	Assumption:
//		pOutBuffer can't be NULL.
//		OutBufferLen can't be zero.
//
//	2007.07.10, by shien chang.
//
BOOLEAN
GetNextLine(
	IN		PGPPARSE_TOKEN	pToken,
	IN		BOOLEAN			bNoAdvance,
	OUT		pu1Byte			pOutBuffer,
	IN		u4Byte			OutBufferLen
	)
{
	u4Byte	BytesRead = 0;
	u4Byte	i, j;
	u4Byte	start;
	
	PlatformZeroMemory(pOutBuffer, OutBufferLen);
	
	for(i = pToken->currBufferIndex; i < pToken->BufferCount; i ++)
	{
		if (i == pToken->currBufferIndex) 
			start = pToken->currBufferOffset;
		else
			start = 0;
		
		for (j = start; j < pToken->BufferList[i].Length; j ++)
		{
			pOutBuffer[BytesRead ++]  = pToken->BufferList[i].VirtualAddress[j];
			
			if (pOutBuffer[BytesRead-2]==0x0d &&
				pOutBuffer[BytesRead-1]==0x0a) 
			{
				if (!bNoAdvance)
				{
					if (j == pToken->BufferList[i].Length-1)
					{
						if (i + 1 < pToken->BufferCount)
						{
							pToken->currBufferIndex = i + 1;
							pToken->currBufferOffset = 0;
						}
						else
						{
							pToken->currBufferIndex = i;
							pToken->currBufferOffset = j;
						}
					}
					else
					{
						pToken->currBufferIndex = i;
						pToken->currBufferOffset = j + 1;
					}
				}
				return TRUE;
			}
			if (BytesRead==OutBufferLen)
				return FALSE;
		}
	}

	return FALSE;
}

//
//	Description:
//		Retreat the point back to previous line. This is
//		a reverse operation of GetNextLine.
//
//	Argument:
//		pToken - the parsing token which maintain by parser.
//
//	Return:
//		TRUE if retreat success.
//		FALSE if retreat failed.
//
//	2007.07.10, by shien chang.
//
BOOLEAN
UngetLine(
	IN		PGPPARSE_TOKEN	pToken
	)
{
	u4Byte		i, j, len;
	u1Byte		found = 0, sta = 0;
	u4Byte		m=0,n=0;

	for (i = pToken->currBufferIndex + 1; i > 0; i --)
	{
		if (i==pToken->currBufferIndex+1) 
			len = pToken->currBufferOffset;
		else
			len = pToken->BufferList[i-1].Length;
		
		for (j = len; j > 0; j --)
		{
			if (pToken->BufferList[i-1].VirtualAddress[j-1] == 0x0a)
			{
				sta++;
				continue;
			}
			if (pToken->BufferList[i-1].VirtualAddress[j-1] == 0x0d)
			{
				if (sta == 1)
				{
					found++;
					if (found == 2)
					{
						pToken->currBufferIndex = m;
						pToken->currBufferOffset = n;
						return TRUE;
					}
				}
				else
					sta = 0;
			}

			sta = 0;
			m = i-1;
			n = j-1;
		}

	}

	return FALSE;
}

//
//	Description:
//		Advance to the offset specific by DataOffset and get following
//		bytes into buffer.
//
//	Arguments:
//		pToken - the parsing token which maintain by parser.
//		DataOffset - the offset parser will advance before get data.
//		bNoAdvance - TRUE: only get data at DataOffset, not advance to.
//					FALSE: advance to DataOffset, and get data.
//		pDataBuffer -buffer to store the read data.
//		DataBufferLength - lenght of pDataBuffer and bytes to read.
//
//	Return:
//		TRUE if read success.
//		FALSE if read failed.
//
//	Note:
//		If pDataBuffer is NULL or DataBufLength is zero, this routine will
//		just advance to DataOffset and doesn't read any data.
//
//	2007.07.10, by shien chang.
//
BOOLEAN
AdvanceAndGetDataByOffset(
	IN	PGPPARSE_TOKEN		pToken,
	IN	u4Byte				DataOffset,
	IN	BOOLEAN				bNoAdvance,
	OUT	PVOID				pDataBuffer,
	IN	u4Byte				DataBufLength
	)
{
	BOOLEAN		bResult = TRUE;
	u4Byte		BytesRead = 0;
	u4Byte		i;
	u4Byte		offset = 0;
	pu1Byte		pBuffer = (pu1Byte)pDataBuffer;
	u4Byte		len;
	u4Byte		tmpIndex=0, tmpOffset=0;
	BOOLEAN		bOnlyAdvance = (pDataBuffer==NULL || DataBufLength==0) ? TRUE : FALSE;
	BOOLEAN		bOnlyAdvanceComplete = FALSE;
	
	for(i = pToken->currBufferIndex;i < pToken->BufferCount; i ++)
	{
		if(pToken->BufferList[i].Length==0)
			continue;

		if (i == pToken->currBufferIndex)
		{
			len = pToken->BufferList[i].Length - pToken->currBufferOffset;
		}
		else
		{
			len = pToken->BufferList[i].Length;
		}
		
		for (; (BytesRead!=DataBufLength) || bOnlyAdvance; BytesRead ++)
		{
			if ( (len + offset) > (DataOffset + BytesRead) )
			{
				u4Byte ByteIndexToCopy = 
					(i == pToken->currBufferIndex) ?
							(DataOffset+BytesRead + pToken->currBufferOffset ) - offset :
							(DataOffset+BytesRead) - offset;


				if (!bOnlyAdvance)
					pBuffer[BytesRead] = pToken->BufferList[i].VirtualAddress[ByteIndexToCopy];

				if (BytesRead == 0)
				{
					tmpIndex = i;
					tmpOffset = ByteIndexToCopy;

					if (bOnlyAdvance) 
					{
						bOnlyAdvanceComplete = TRUE;
						break;
					}
				}
			}
			else
				break;
		}

		if (bOnlyAdvance)
		{
			if (bOnlyAdvanceComplete)
				break;
		}
		else
		{
			if (BytesRead == DataBufLength)
				break;
		}
		offset += len;
	}

	if (!bOnlyAdvance && BytesRead != DataBufLength)
	{
		RT_TRACE(COMP_CCX, DBG_LOUD, ("AdvanceAndGetDataByOffset(): some data can not be retrived\n"));
		bResult = FALSE;
	}

	if (bResult && !bNoAdvance)
	{
		pToken->currBufferIndex = tmpIndex;
		pToken->currBufferOffset = tmpOffset;
	}

	return bResult;
}


//=============================================================================
// End of Private utility function.
//=============================================================================




//=============================================================================
// Public parser interface.
//=============================================================================

//
//	Description:
//		Use high performance parser.
//	2007.07.12, by shien chang.
//
VOID
GPUseHPParser(
	IN		PGENERIC_PARSER	pParser
	)
{
	PGP_HP_ENTRY	pEntry, pTmpEntry;
	PGP_HP_NODE		pNode;
	PGP_RULE		pRuleSet[2];
	PGP_RULE		pRule;
	u4Byte			i, j;
	u4Byte			entryIndex = 0;
	u4Byte			nodeIndex = 0;
	u4Byte			currNode=0, nextNode=0;
	PRT_LIST_ENTRY	pListEntry;
	
	if (pParser->pParsingTree == NULL) 
	{
		pParser->bUseHP = FALSE;
		return;
	}

	pRuleSet[0] = GPGlobalRule;
	pRuleSet[1] = pParser->pParseRules;

	for (i = 0; i < 2; i ++)
	{
		for (pRule = pRuleSet[i];
			(pRule != NULL) && (pRule->CurrProtocol != 0);
			pRule ++)
		{			
			for (j = 0; j < nodeIndex; j ++) 
			{
				pNode = GP_HP_GET_NODE_BY_INDEX(pParser->pParsingTree, j);
				if (pNode->ProtocolId == pRule->CurrProtocol)
				{
					currNode = j;
					break;
				}
			}
			if (j == nodeIndex) 
			{
				currNode = nodeIndex;
				pNode = GP_HP_GET_NODE_BY_INDEX(pParser->pParsingTree, currNode);
				pNode->ProtocolId = pRule->CurrProtocol;
				pNode->bUsed = TRUE;
				RTInitializeListHead(&(pNode->Link));
				nodeIndex ++;
				if (nodeIndex == MAX_HP_PROTOCOL_PER_PARSER) 
				{
					pParser->bUseHP = FALSE;
					return;
				}
			}

			for (j = 0; j < nodeIndex; j ++)
			{
				pNode = GP_HP_GET_NODE_BY_INDEX(pParser->pParsingTree, j);
				if (pNode->ProtocolId == pRule->NextProtocol)
				{
					nextNode = j;
					break;
				}
			}
			if (j == nodeIndex)
			{
				nextNode = nodeIndex;
				pNode = GP_HP_GET_NODE_BY_INDEX(pParser->pParsingTree, nextNode);
				pNode->ProtocolId = pRule->NextProtocol;
				pNode->bUsed = TRUE;
				RTInitializeListHead(&(pNode->Link));
				nodeIndex++;
				if (nodeIndex == MAX_HP_PROTOCOL_PER_PARSER) 
				{
					pParser->bUseHP = FALSE;
					return;
				}
			}

			pEntry = GP_HP_GET_ENTRY_BY_INDEX(pParser->pParsingTree, entryIndex);
			pNode = GP_HP_GET_NODE_BY_INDEX(pParser->pParsingTree, currNode);

			// Check overwrite.
			for (pListEntry = (&pNode->Link)->Flink;
				pListEntry != &pNode->Link;
				pListEntry = pListEntry->Flink)
			{
				pTmpEntry = (PGP_HP_ENTRY)pListEntry;
				if ( (pTmpEntry->pRule->CurrProtocol == pRule->CurrProtocol) &&
					(pTmpEntry->pRule->NextProtocol == pRule->NextProtocol) )
				{
					RTRemoveEntryList(pListEntry);
					break;
				}
			}
				
			pEntry->pRule = pRule;
			pEntry->index = (u1Byte)nextNode;
			RTInsertTailList(&(pNode->Link), &(pEntry->Link));
			entryIndex++;
			if (entryIndex == MAX_HP_RULES_PER_PARSER) 
			{
				pParser->bUseHP = FALSE;
				return;
			}
			
		}
	}

	pParser->bUseHP = TRUE;
}

//
//	Description:
//		This routine parse ethernet header.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserHandlerEthernet(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u2Byte		typeLength;
	BOOLEAN		bResult = FALSE;

	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	if (pSigBuffer == NULL) return FALSE;

	typeLength = N2H2BYTE( *((UNALIGNED pu2Byte)pSigBuffer) );
	
	switch (NextProtocol)
	{
		case PROTO_IP:
			if (typeLength == 0x0800) 
			{
				*AdvBytes = PROTO_ETHERNET_HDRLEN;
				bResult = TRUE;
			}
			break;
			
		default:
			break;
	}

	return bResult;
}

//
//	Description:
//		This routine parse 802.11 wlan header.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserHandlerWlan(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte		QosCtrlLen = 0;
	
	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	if (pSigBuffer == NULL)
		return FALSE;

	if ( IsDataFrame(pSigBuffer) )
	{
		if( IsQoSDataFrame( pToken->BufferList[0].VirtualAddress ) )
		{
			QosCtrlLen = sQoSCtlLng;
		}

		*AdvBytes = sMacHdrLng+QosCtrlLen+pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
		return TRUE;
	}

	return FALSE;
}

//
//	Description:
//		This routine parse 802.11 wlan LLC header.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserHandlerWlanLlc(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u2Byte		typeLength;
	BOOLEAN		bResult = FALSE;

	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	if (pSigBuffer == NULL)
		return FALSE;
	typeLength = N2H2BYTE( *((UNALIGNED pu2Byte)pSigBuffer) );
	
	switch (NextProtocol)
	{
		case PROTO_IP:
			if (typeLength == 0x0800) 
			{
				*AdvBytes = PROTO_WLAN_LLC_HDRLEN;
				bResult = TRUE;
			}
			break;
			
		default:
			break;
	}

	return bResult;
}

//
//	Description:
//		This routine parse ip header.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserHandlerIP(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u1Byte		type = *pSigBuffer;
	BOOLEAN		bResult = FALSE;

	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	if (pSigBuffer == NULL)
		return FALSE;
	
	switch (NextProtocol)
	{
		case PROTO_TCP:
			if (type == 0x06) 
			{
				*AdvBytes = PROTO_IP_HDRLEN;
				bResult = TRUE;
			}
			break;

		case PROTO_UDP:
			if (type == 0x11) 
			{
				*AdvBytes = PROTO_IP_HDRLEN;
				bResult = TRUE;
			}
			break;

		default:
			break;
	}

	return bResult;
}

//
//	Description:
//		Allocate a parser and associate the parser with a gate
//		routine which checks parsing criteria and a action routine
//		which decides how to handle the parsed packet.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPAllocateParser(
	IN		PADAPTER			Adapter,
	IN		u1Byte				Pid,
	IN		const char*				strParserName,
	IN		ifAllocateParser	AllocateRoutine,
	IN		GPParserGate		ParserGateRoutine,
	IN		GPParserAction		ParserActionRoutine,
	IN		BOOLEAN				bUseHP
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	RT_STATUS			Status;
	u4Byte				i;
	BOOLEAN				bFound = FALSE;
	PGENERIC_PARSER		pParser;

	//
	// Allocate global data structure first.
	//
	if (pMgntInfo->pGenericParser == NULL)
	{
		Status = PlatformAllocateMemory(
						Adapter,
						&(pMgntInfo->pGenericParser),
						sizeof(PGENERIC_PARSER) * MAX_PARSER_NUM);
		if (Status != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_CCX, DBG_SERIOUS, ("GPAllocateParser(): Failed to allocate parsers\n"));
			return FALSE;
		}

		PlatformZeroMemory(pMgntInfo->pGenericParser,
							sizeof(PGENERIC_PARSER) * MAX_PARSER_NUM);
	}

	//
	// Try to allocate parser.
	//
	for (i = 0; i < MAX_PARSER_NUM; i ++)
	{
		if ( *(((PGENERIC_PARSER*)pMgntInfo->pGenericParser) + i) == NULL)
		{
			//
			// Allocate parser
			//
			Status = PlatformAllocateMemory(
							Adapter,
							(PVOID*)&pParser,
							sizeof(GENERIC_PARSER));
			if (Status != RT_STATUS_SUCCESS)
			{
				RT_TRACE(COMP_CCX, DBG_SERIOUS, ("GPAllocateParser(): Failed to allocate parser\n"));
				return FALSE;			
			}

			PlatformZeroMemory(pParser, sizeof(GENERIC_PARSER));

			pParser->PID = Pid;
			PlatformZeroMemory(pParser->ParserName, MAX_PARSER_NAME);
			if (strParserName)
			{
				ASCII_STR_COPY(pParser->ParserName, strParserName, MAX_PARSER_NAME);
			}
			pParser->Gate = ParserGateRoutine;
			pParser->Action = ParserActionRoutine;
			pParser->bUseHP = bUseHP;

			if (bUseHP)
			{
				//
				// Allocate HP rule structure.
				//
				Status = PlatformAllocateMemory(
								Adapter,
								(PVOID*)&pParser->pParsingTree,
								MAX_HP_PROTOCOL_PER_PARSER * sizeof(GP_HP_NODE) +
								MAX_HP_RULES_PER_PARSER * sizeof(GP_HP_ENTRY));
				if (Status != RT_STATUS_SUCCESS)
				{
					pParser->pParsingTree = NULL;
				}

				PlatformZeroMemory(pParser->pParsingTree,
									MAX_HP_PROTOCOL_PER_PARSER * sizeof(GP_HP_NODE) +
									MAX_HP_RULES_PER_PARSER * sizeof(GP_HP_ENTRY));
			}
			else
			{
				pParser->pParsingTree = NULL;
			}

			//
			// Allocate specific data.
			//
			if (AllocateRoutine)
			{
				if (! AllocateRoutine(Adapter, pParser))
				{
					GPFreeParser(Adapter, Pid, NULL);
					return FALSE;
				}
			}

			*(((PGENERIC_PARSER*)pMgntInfo->pGenericParser) + i) = pParser;
			
			bFound = TRUE;
			break;
		}
	}

	return bFound;
}

//
//	Description:
//		Deallocate parser allocated at GPAllocateParser.
//	2007.07.10, by shien chang.
//
VOID
GPFreeParser(
	IN		PADAPTER		Adapter,
	IN		u1Byte			Pid,
	IN		ifFreeParser	FreeRoutine
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	BOOLEAN				bClear = TRUE;
	u4Byte				i;
	PGENERIC_PARSER		pParser;
	
	if (pMgntInfo->pGenericParser == NULL)
		return;
	
	//
	// Try to free parser.
	//
	for (i = 0; i < MAX_PARSER_NUM; i++)
	{
		pParser = *(((PGENERIC_PARSER*)pMgntInfo->pGenericParser) + i);
		if (pParser)
		{
			if (pParser->PID == Pid)
			{
				if (FreeRoutine)
					FreeRoutine(Adapter, pParser);

				if (pParser->pParsingTree)
				{
					PlatformFreeMemory(
						pParser->pParsingTree,
						MAX_HP_PROTOCOL_PER_PARSER * sizeof(GP_HP_NODE) +
						MAX_HP_RULES_PER_PARSER * sizeof(GP_HP_ENTRY));
				}
				
				PlatformFreeMemory(pParser, sizeof(GENERIC_PARSER));

				*(((PGENERIC_PARSER*)pMgntInfo->pGenericParser)+i) = NULL;
				break;
			}
		}
	}

	//
	// Free all pointer if no parser.
	//
	for (i=0; i < MAX_PARSER_NUM; i++)
	{
		pParser = *(((PGENERIC_PARSER*)pMgntInfo->pGenericParser) + i);
		if (pParser)
		{
			bClear = FALSE;
			break;
		}
	}

	if (bClear)
	{
		PlatformFreeMemory(pMgntInfo->pGenericParser,
						sizeof(PGENERIC_PARSER) * MAX_PARSER_NUM);
	}
}



//
//	Description:
//		Parse Tx packet which started as InitProtocol.
//	2007.07.10, by shien chang.
//
TR_ACTION
GPParseTCB(
	IN		PADAPTER	Adapter,
	IN		PRT_TCB		pTcb,
	IN		PROTO_ID	InitProtocol
	)
{
	GPPARSE_TOKEN		token;
	PGPPARSE_TOKEN		pToken = &token;

	PlatformZeroMemory(pToken, sizeof(GPPARSE_TOKEN));

	PlatformMoveMemory(pToken->BufferList, 
						pTcb->BufferList,
						sizeof(SHARED_MEMORY) * MAX_PER_PACKET_BUFFER_LIST_LENGTH);
	pToken->BufferCount = pTcb->BufferCount;
	pToken->currBufferIndex = 0;
	pToken->currBufferOffset = 0;
	pToken->currProtocol = InitProtocol;
	pToken->pDataObj = pTcb;
	pToken->gpFlag = GPFLAG_TX;

	return ParseBuffer(Adapter, pToken);
	
}

//
//	Description:
//		Parse Rx packet which started as InitProtocol.
//	2007.07.10, by shien chang.
//
TR_ACTION
GPParseRFD(
	IN		PADAPTER	Adapter,
	IN		PRT_RFD		pRfd,
	IN		PROTO_ID	InitProtocol
	)
{
	GPPARSE_TOKEN		token;
	PGPPARSE_TOKEN		pToken = &token;

	PlatformZeroMemory(pToken, sizeof(GPPARSE_TOKEN));
	
	pToken->BufferList[0].VirtualAddress = pRfd->Buffer.VirtualAddress + pRfd->FragOffset;
	pToken->BufferList[0].Length = pRfd->FragLength;
	pToken->BufferCount = 1;
	pToken->currBufferIndex = 0;
	pToken->currBufferOffset = 0;
	pToken->currProtocol = InitProtocol;
	pToken->pDataObj = pRfd;
	pToken->gpFlag = GPFLAG_RX;

	return ParseBuffer(Adapter, pToken);
	
}


//=============================================================================
// End of Public parser interface.
//=============================================================================

//=============================================================================
// Custom parser routines.
//=============================================================================

//
//	Description:
//		Gate routine of VOWLAN parser.
//		If return TRUE, parser rules will be driven.
//		If return FALSE, parser will skip it's work.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserGateVoWlanSIP(
	IN		PADAPTER	Adapter,
	IN		PVOID		pParser
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS	pStaQos = pMgntInfo->pStaQos;

	//
	// Check if WMM is enabled, CCX is version 4
	//
	if (pMgntInfo->mAssoc)
	{
		if ( (pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM) ||
			(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMMSA) )
		{
			u1Byte	CurrCcxVerNumber = 0;
			
			CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);
			
			if ( CurrCcxVerNumber >= 4 )
			{
				pu1Byte param = GET_WMM_PARAM_ELE_AC_PARAM(pStaQos->WMMParamEle);
				u1Byte acmVO = GET_WMM_AC_PARAM_ACM(param + AC3_VO * 4);

				if (acmVO)
					return TRUE;
			}
		}
	}

	return FALSE;
}

//
//	Description:
//		Allocation routine of VOWLAN parser.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPAllocateParserVoWlanSIP(
	IN		PADAPTER			Adapter,
	IN		PGENERIC_PARSER		pParser
	)
{
	PGP_VOWLAN_CONTEXT		pContext;
	RT_STATUS				Status;

	Status = PlatformAllocateMemory(Adapter,
									(PVOID*)&pContext,
									sizeof(GP_VOWLAN_CONTEXT));
	if (Status != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_CCX, DBG_SERIOUS, ("GPAllocateParserVoWlanSIP(): Failed to allocate memory\n"));
		return FALSE;
	}

	PlatformZeroMemory(pContext, sizeof(GP_VOWLAN_CONTEXT));

	GP_REGISTER_RULES(pParser, GPVoWlanSipRule);
	GP_BIND_CONTEXT(pParser, pContext);

	if (pParser->pParsingTree && pParser->bUseHP)
		GPUseHPParser(pParser);
	
	return TRUE;
}

//
//	Description:
//		Deallocation routine of VOWLAN parser.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPFreeParserVoWlanSIP(
	IN		PADAPTER			Adapter,
	IN		PGENERIC_PARSER		pParser
	)
{
	PGP_VOWLAN_CONTEXT	pContext;

	pContext = (PGP_VOWLAN_CONTEXT)GP_GET_CONTEXT(pParser);
	if (pContext)
	{
		PlatformFreeMemory(pContext, sizeof(GP_VOWLAN_CONTEXT));
	}

	GP_UNBIND_CONTEXT(pParser);
	GP_UNREGISTER_RULES(pParser);

	return TRUE;
}

//
//	Description:
//		Action routine of VOWLAN parser.
//	2007.07.10, by shien chang.
//
TR_ACTION
GPParserActionVoWlanSIP(
	IN		PADAPTER	Adapter,
	IN		PGPPARSE_TOKEN		pToken,
	IN		PVOID		pParser
	)
{
	TR_ACTION			trAction = TR_ACTION_CONTINUE;
	//PGENERIC_PARSER		pGPParser = (PGENERIC_PARSER)pParser;
	//PGP_VOWLAN_CONTEXT	pContext = (PGP_VOWLAN_CONTEXT)pGPParser->ParserContext;

	return trAction;
}


//
//	Description:
//		Custom routine to parse UDP to SIP.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleUDP2SIP(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS			pStaQos = pMgntInfo->pStaQos;
	u1Byte				line[256];
	const char*				InviteSig = "INVITE sip:";
	const char*				StatusSig = "SIP/2.0";
	const char*				ByeSig = "BYE sip:";
	const char*				NotifySig = "NOTIFY sip:";
	const char*				AckSig = "ACK sip:";
	BOOLEAN				bReturn = FALSE;
	RT_SPINLOCK_TYPE	SpinLockType;
	BOOLEAN				bQosData = FALSE;
	BOOLEAN				bRejectData = FALSE;
	
	RT_ASSERT( ((pToken->gpFlag == GPFLAG_TX)||(pToken->gpFlag == GPFLAG_RX)),
		("GPParserCustomHandleUDP2SIP(): Parser flag must either be GPFLAG_TX or GPFLAG_RX\n"));

	if (pToken->gpFlag == GPFLAG_TX)
		SpinLockType = RT_TX_SPINLOCK;
	else if (pToken->gpFlag == GPFLAG_RX)
		SpinLockType = RT_RX_SPINLOCK;
	else
		return bReturn;
	
	*AdvBytes = PROTO_UNKNOWN_HDRLEN;

	if (!GetNextLine(pToken, TRUE, line, sizeof(line)) )
		return FALSE;
	
	switch (NextProtocol)
	{
		case PROTO_SIP_INVITE:
			if (PlatformCompareMemory(
					&(line[PROTO_UDP_HDRLEN]), (PVOID)InviteSig, 11) == 0)
			{
				RT_TRACE(COMP_CCX , DBG_LOUD, ("GPParserCustomHandleUDP2SIP(): PROTO_SIP_INVITE, pToken->gpFlag = %02X\n", pToken->gpFlag));
				CCX_GPParserCustomHandleUDP2SIP(Adapter, SpinLockType, &bQosData, &bReturn, &bRejectData);
			}					
			break;

		case PROTO_SIP_STATUS:
			if (PlatformCompareMemory(
						&line[PROTO_UDP_HDRLEN],
						(PVOID)StatusSig,
						7) == 0)
			{
				if (CCX_CAC_IsVoiceTsExist(Adapter))
				{
					bQosData = TRUE;
					bReturn = TRUE;
				}
				else if(pToken->gpFlag == GPFLAG_TX)
				{ // Reject the status packet responding to the in-call.
					bRejectData = TRUE;
				}
			}	
			break;

		case PROTO_SIP_BYE:
			if (PlatformCompareMemory(
						&line[PROTO_UDP_HDRLEN],
						(PVOID)ByeSig,
						8) == 0)
			{
				PlatformReleaseSpinLock(Adapter, SpinLockType);
				RT_TRACE(COMP_CCX, DBG_LOUD, ("GPParserCustomHandleUDP2SIP(): PROTO_SIP_BYE, pToken->gpFlag = %02X\n", pToken->gpFlag));
				CCX_CAC_DelTs(Adapter);
				PlatformAcquireSpinLock(Adapter, SpinLockType);
				bReturn = TRUE;
			}	
			break;

		case PROTO_SIP_NOTIFY:
			if (PlatformCompareMemory(
						&line[PROTO_UDP_HDRLEN],
						(PVOID)NotifySig,
						11) == 0)
			{
				//if (CCX_CAC_IsSignalTsExist(Adapter))
				if (CCX_CAC_IsVoiceTsExist(Adapter))
					bQosData = TRUE;
				bReturn = TRUE;
			}
			break;
			
		case PROTO_SIP_ACK:
			if (PlatformCompareMemory(
						&line[PROTO_UDP_HDRLEN],
						(PVOID)AckSig,
						8) == 0)
			{
				//if (CCX_CAC_IsSignalTsExist(Adapter))
				if (CCX_CAC_IsVoiceTsExist(Adapter))
					bQosData = TRUE;
				bReturn = TRUE;
			}
			break;
			
		default:
			break;
	}
	
	if (bQosData)
	{
		*AdvBytes = PROTO_UDP_HDRLEN;
		if (pToken->gpFlag == GPFLAG_TX)
		{
			PQOS_TSTREAM	pTs = NULL;
			PRT_TCB			pTcb = (PRT_TCB)pToken->pDataObj;
			pu1Byte			pHeader = pTcb->BufferList[0].VirtualAddress;
			if (pTcb)
			{
				pTcb->DataRate = RT_CCX_CAC_MIN_PHY_RATE(Adapter);
				pTcb->TSID = 0;
				pTcb->priority = 6;
				SET_80211_HDR_QOS_EN(pHeader, 1);

				if (!ACTING_AS_AP(Adapter))
				{
					pTs = &(pStaQos->StaTsArray[pTcb->TSID]);
					if (pTs && pTs->bUsed)
					{
						pTcb->DataRate = QosGetNPR(Adapter, pTs);
					}
				}
			}
		}
	}
	else if (bRejectData)
	{
		PRT_TCB	pTcb = (PRT_TCB)pToken->pDataObj;
		if (pTcb) pTcb->TSID = SESSION_REJECT_TSID;
	}
	
	return bReturn;
}

//
//	Description:
//		Custom routine to parse SIP to SDP.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleSIP2SDP(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u1Byte	line[256];
	const char*	AudioSig = "m=audio";

	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	
	while (GetNextLine(pToken, FALSE, line, sizeof(line)) )
	{
		if (PlatformCompareMemory(line, (PVOID)AudioSig, 7) == 0)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

//
//	Description:
//		Custom routine to handle SIP STATUS message.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleSIPSTATUS(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u1Byte				line[256];
	u1Byte				callId[MAX_SESSION_ID];
	u4Byte				callIdLen;
	RT_SPINLOCK_TYPE	SpinLockType;
	
	RT_ASSERT( ((pToken->gpFlag == GPFLAG_TX)||(pToken->gpFlag == GPFLAG_RX)),
		("GPParserCustomHandleUDP2SIP(): Parser flag must either be GPFLAG_TX or GPFLAG_RX\n"));

	if (pToken->gpFlag == GPFLAG_TX)
		SpinLockType = RT_TX_SPINLOCK;
	else if (pToken->gpFlag == GPFLAG_RX)
		SpinLockType = RT_RX_SPINLOCK;
	else
		return FALSE;
	
	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	
	if (GetNextLine(pToken, TRUE, line, sizeof(line)) )
	{
		// 
		// SIP reponse status code:
		//	1XX 	Provisional 	100 Trying
		//	2XX	Successful		200 OK
		//	3XX	Redirection 	302 Moved Temporarily	
		//	4XX	Client Error	404 Not Found
		//	5XX	Server Error	504 Server Time-out
		//	6XX	Global Failure	603 Decline
		// By Bruce, 2008-03-17.
		//
		if ( (line[8] == '4') || (line[8] == '5' || (line[8] == '6') ))
		{
			if (GetCallId(pToken, FALSE, callId, &callIdLen))
			{
				RemoveSession((PGP_VOWLAN_CONTEXT)pParserContext, callId, callIdLen);
			}

			PlatformReleaseSpinLock(Adapter, SpinLockType);
			RT_TRACE(COMP_CCX, DBG_LOUD, ("GPParserCustomHandleSIPSTATUS(): Remove Session(reason = %02X), pToken->gpFlag = %02X\n", line[8], pToken->gpFlag));
			CCX_CAC_DelTs(Adapter);
			PlatformAcquireSpinLock(Adapter, SpinLockType);

			if (pToken->gpFlag == GPFLAG_TX)
			{
				PRT_TCB	pTcb = (PRT_TCB)pToken->pDataObj;
				pu1Byte	pHeader = pTcb->BufferList[0].VirtualAddress;
				if (pTcb)
				{
					pTcb->TSID = DEFAULT_TSID;
					pTcb->priority = 0;
					SET_80211_HDR_QOS_EN(pHeader, 0);
				}
			}
		}
	}

	return TRUE;
}

//
//	Description:
//		Get call id which is contained in SIP message.
//	2007.07.10, by shien chang.
//
BOOLEAN
GetCallId(
	IN		PGPPARSE_TOKEN	pToken,
	IN		BOOLEAN			bNoAdvance,
	OUT		pu1Byte			callId,
	OUT		pu4Byte			callIdLen
	)
{
	u1Byte	buf[256];
	u4Byte	i;
	const char*	CallSig = "Call-ID:";
	
	while (GetNextLine(pToken, bNoAdvance, buf, sizeof(buf)))
	{
		if (PlatformCompareMemory(buf, (PVOID)CallSig, 8) == 0)
		{
			for (i=9; i<sizeof(buf); i++)
			{
				if (buf[i] == 0x2e)
				{
					PlatformMoveMemory(callId, &(buf[9]), i-9);
					*callIdLen = i-9;
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

//
//	Description:
//		Session Management - remove a session.
//	2007.07.10, by shien chang.
//
VOID
RemoveSession(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		pu1Byte				sessionId,
	IN		u4Byte				sessionIdLen
	)
{
	u4Byte	i;

	if (sessionIdLen > MAX_SESSION_ID)
		return;
	
	for (i = 0; i < MAX_SESSION; i ++)
	{
		if ( (pContext->Session[i].bUsed == TRUE) &&
			(pContext->Session[i].sessionIdLen == sessionIdLen) &&
			(PlatformCompareMemory(
					pContext->Session[i].sessionId,
					sessionId,
					sessionIdLen) == 0) )
		{
			RT_PRINT_STR(COMP_CCX, DBG_LOUD, "RemoveSession():\n", pContext->Session[i].sessionId, pContext->Session[i].sessionIdLen);
			pContext->Session[i].bUsed = FALSE;
			break;
		}
	}

}

//
//	Description:
//		Custom routine to handle SDP.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleSDP(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u1Byte		line[256];
	u2Byte		port = 0;
	u2Byte		type = 65535;
	u1Byte		charTable[] = { '0','1','2','3','4','5','6','7','8','9' };
	u4Byte		i,j;
	u1Byte		callId[MAX_SESSION_ID];
	u4Byte		callIdLen;
	BOOLEAN		bReturn = FALSE;
	u4Byte		hdrLen=0;
	const char*		RtpSig = "RTP/AVP";

	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	
	if (GetCallId(pToken, FALSE, callId, &callIdLen))
	{
		if (!AddSession((PGP_VOWLAN_CONTEXT)pParserContext, callId, callIdLen))
		{
			RT_TRACE(COMP_CCX, DBG_LOUD, ("GPParserCustomHandleUDP2SIP(): Failed to add a call\n"));
			return FALSE;
		}
	}
	else
		return FALSE;

	if (GPParserCustomHandleSIP2SDP(Adapter,
									pParserContext,
									CurrProtocol,
									NextProtocol,
									pSigBuffer,
									SigBufferLen,
									pToken,
									&hdrLen))
	{
		if (!UngetLine(pToken))
			return FALSE;
	}
	else 
		return FALSE;
	
	if (!GetNextLine(pToken, TRUE, line, sizeof(line)) )
		return FALSE;

	// get port num
	for (i = 8; i < sizeof(line); i ++)
	{
		for (j = 0; j < sizeof(charTable); j ++)
		{
			if (line[i] == charTable[j])
				break;
		}

		if (j==sizeof(charTable))
			break;

		port = (port*10)+(u2Byte)j;
	}

	if (port)
	{
		i ++;
		if ((i + 7 < 256) && PlatformCompareMemory(
					&(line[i]),
					(PVOID)RtpSig,
					7) == 0)
		{
			i += 7;
			while (line[i] != 0x0d)
			{
				i ++;
				type = 65535;
				
				for (; i < sizeof(line); i ++)
				{
					for (j = 0; j < sizeof(charTable); j ++)
					{
						if (line[i] == charTable[j])
							break;
					}

					if (j==sizeof(charTable))
						break;

					if (type==65535)
						type=0;
					type = (type*10) + (u2Byte)j;
				}

				if (type == 0 || type == 8)
				{
					if (pToken->gpFlag == GPFLAG_TX)
					{	// Update Port Num
						UpdateSessionInfo((PGP_VOWLAN_CONTEXT)pParserContext, callId, callIdLen, port, 0);
						bReturn = TRUE;
					}
					else if (pToken->gpFlag == GPFLAG_RX)
					{
						// Update Port Num
						UpdateSessionInfo((PGP_VOWLAN_CONTEXT)pParserContext, callId, callIdLen, 0, port);
						bReturn = TRUE;
					}
					else
					{
						RT_TRACE(COMP_CCX, DBG_LOUD, ("GPParserCustomHandleSDP(): Invalid flag\n"));
					}
					
					break;
				}
			}
		}
	}

	return bReturn;
}

//
//	Description:
//		Session Management - add a new session.
//	2007.07.10, by shien chang.
//
BOOLEAN
AddSession(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		pu1Byte				sessionId,
	IN		u4Byte				sessionIdLen
	)
{
	u4Byte	i;

	if (sessionIdLen > MAX_SESSION_ID)
	{
		RT_TRACE(COMP_CCX, DBG_WARNING, ("AddSession(): Failed, sessionIDLen(%d) > Max(%d)\n", sessionIdLen, MAX_SESSION_ID));
		return FALSE;
	}

	RT_PRINT_STR(COMP_CCX, DBG_LOUD, "AddSession():\n", sessionId, sessionIdLen);
	
	//
	// Check if an call can be added.
	//
	for (i = 0; i < MAX_SESSION; i ++)
	{
		if (pContext->Session[i].bUsed)
		{
			if ( (sessionIdLen == pContext->Session[i].sessionIdLen) &&
				(PlatformCompareMemory(sessionId,
								pContext->Session[i].sessionId,
								sessionIdLen) == 0) )
			{
				RT_TRACE(COMP_CCX, DBG_LOUD, ("AddSession(): This session has been existed!\n"));
				return TRUE;
			}	
		}
		
		if (!pContext->Session[i].bUsed) break;
	}

	if (i != MAX_SESSION)
	{
		PlatformZeroMemory(pContext->Session[i].sessionId, MAX_SESSION_ID);
		PlatformMoveMemory(pContext->Session[i].sessionId, sessionId, sessionIdLen);
		pContext->Session[i].sessionIdLen = sessionIdLen;
		pContext->Session[i].bUsed = TRUE;		
		return TRUE;
	}

	RT_TRACE(COMP_CCX, DBG_WARNING, ("AddSession(): Add Session Failed SessionNum = %d > Max(%d) \n", i, MAX_SESSION));

	return FALSE;
}

//
//	Description:
//		Session Management - update a session information.
//	2007.07.10, by shien chang.
//
BOOLEAN
UpdateSessionInfo(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		pu1Byte				sessionId,
	IN		u4Byte				sessionIdLen,
	IN		u2Byte				srcPort,
	IN		u2Byte				dstPort
	)
{
	u4Byte	i;

	if (sessionIdLen > MAX_SESSION_ID)
		return FALSE;
	
	for (i = 0; i < MAX_SESSION; i ++)
	{
		if ( (pContext->Session[i].bUsed == TRUE) &&
			(pContext->Session[i].sessionIdLen == sessionIdLen) &&
			(PlatformCompareMemory(
					pContext->Session[i].sessionId,
					sessionId,
					sessionIdLen) == 0) )
		{
			if (srcPort)
				pContext->Session[i].srcPort = srcPort;
			if (dstPort)
				pContext->Session[i].dstPort = dstPort;
			return TRUE;
		}
	}

	return FALSE;
}

//
//	Description:
//		Session Management - match a session by port.
//	2007.07.10, by shien chang.
//
BOOLEAN
MatchSession(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		u2Byte				Port,
	IN		u1Byte				Flag
	)
{
	u4Byte	i;

	if ( (Flag != GPFLAG_TX) && (Flag != GPFLAG_RX) )
		return FALSE;
	if (Port == 0)
		return FALSE;
	
	for (i = 0; i < MAX_SESSION; i ++)
	{
		if (pContext->Session[i].bUsed)
		{
			if ( (Flag == GPFLAG_TX) && (pContext->Session[i].srcPort == Port) )
				return TRUE;
			if ( (Flag == GPFLAG_RX) && (pContext->Session[i].dstPort == Port) )
				return TRUE;
		}
	}

	return FALSE;
}


//
//	Description:
//		Custom routine to handle SIP BYE message.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleSIPBYE(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u1Byte	callId[MAX_SESSION_ID];
	u4Byte	callIdLen;
	
	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	
	if (GetCallId(pToken, FALSE, callId, &callIdLen))
	{
		RemoveSession((PGP_VOWLAN_CONTEXT)pParserContext, callId, callIdLen);
	}

	return TRUE;
}

//
//	Description:
//		Custom routine to parse UDP to RTP.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleUDP2RTP(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u2Byte		PortBuf=0;
	u2Byte		Port;
	u4Byte		offset;
	BOOLEAN		bReturn = FALSE;

	*AdvBytes = PROTO_UNKNOWN_HDRLEN;

	if ( (pToken->gpFlag == GPFLAG_TX) || (pToken->gpFlag == GPFLAG_RX) )
	{
		offset = 0;	
	}
	else
	{
		return FALSE;
	}

	if (AdvanceAndGetDataByOffset(pToken,
								offset,
								TRUE,
								(pu1Byte)&PortBuf,
								sizeof(PortBuf)) )
	{
		Port = N2H2BYTE(PortBuf);
		if ( MatchSession((PGP_VOWLAN_CONTEXT)pParserContext, Port, pToken->gpFlag) )
		{
			*AdvBytes = PROTO_UDP_HDRLEN;
			bReturn = TRUE;
		}
	}
		
	return bReturn;
}

//
//	Description:
//		Custom routine to parse RTP to G.711.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleRTP2G711(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	u1Byte	type;
	
	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	
	if (AdvanceAndGetDataByOffset(pToken, 
								1, 
								TRUE, 
								&type,
								sizeof(type)) )
	{
		//
		// RTP payload types:
		//	0x0:		PCMU(mu-law G.711)	Audio	Clock rate = 8000
		//	0x08:		PCMA(A-law G.711)		Audio	Clock rate = 8000
		// By Bruce, 2008-03-18.
		//
		if (type==0x00 || type==0x08)
			return TRUE;
	}

	return FALSE;
}

//
//	Description:
//		Custom routine to handle G.711 packet.
//	2007.07.10, by shien chang.
//
BOOLEAN
GPParserCustomHandleG711(
	IN		PADAPTER		Adapter,
	IN		PVOID			pParserContext,
	IN		PROTO_ID		CurrProtocol,
	IN		PROTO_ID		NextProtocol,
	OUT		pu1Byte			pSigBuffer,
	IN		u4Byte			SigBufferLen,
	IN		PGPPARSE_TOKEN	pToken,
	OUT		pu4Byte			AdvBytes
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs = NULL;
	
	*AdvBytes = PROTO_UNKNOWN_HDRLEN;
	if (pToken->gpFlag == GPFLAG_TX)
	{
		PRT_TCB	pTcb = (PRT_TCB)pToken->pDataObj;
		pu1Byte	pHeader = pTcb->BufferList[0].VirtualAddress;
		if (pTcb)
		{
			pTcb->DataRate = RT_CCX_CAC_MIN_PHY_RATE(Adapter);
			pTcb->TSID = 0;
			pTcb->priority = 6;
			SET_80211_HDR_QOS_EN(pHeader, 1);

			if (!ACTING_AS_AP(Adapter))
			{
				pTs = &(pStaQos->StaTsArray[pTcb->TSID]);
				if (pTs && pTs->bUsed)
				{
					pTcb->DataRate = QosGetNPR(Adapter, pTs);
				}
			}
		}
	}
	return TRUE;
}

//
//   Must call After TranslateHandleRxDot11FrameHeader
//	Note : No Qos , IV , HTC  802.11 frame !!
//		   And Data frame !!
//

BOOLEAN
GPGetParseRFDInfo(
	IN		PADAPTER		Adapter,
	IN		PRT_RFD			pRfd,
	IN		PRX_FILTER_INFO	pRxfiterInfo
	)
{
	/*
	typedef struct _RX_FILTER_INFO{
		// MAC
		pu1Byte		pDA;
		pu1Byte		pSA;
		u1Byte		PacketType;  // RX_PACKET_TYPE
		pu1Byte		pEtherType;  // 2 bytes : IPv4 , IPv6 , ARP ...  
		// IP
		u1Byte		ARPOption;  // Request or Response 
		pu1Byte		ARPSPA;      // ARP Sender IP 
		pu1Byte		ARPTPA;      // ARP Target IP 
		u1Byte		Protocol;      // UDP : 0x11  , TCP : 0x06
		//UDP
		pu1Byte		pDestinationPort; // 2 bytes
	}RX_FILTER_INFO,*PRX_FILTER_INFO;
	*/
	OCTET_STRING		frame;
	//PRX_FILTER_INFO 		pRxfiterInfo = &(pRfd->D0FilterCoalPktInfo);
	pu1Byte				pCurrentbuf;
	u1Byte				ArpType[2] = {0x08 ,0x06};
	u1Byte				IPv4Type[2] = {0x08 ,0x00};
	u1Byte				IPv6Type[2] = {0x86 ,0xdd};
	
	FillOctetString(frame, pRfd->Buffer.VirtualAddress+PLATFORM_GET_FRAGOFFSET(pRfd), pRfd->PacketLength);

	PlatformZeroMemory(pRxfiterInfo, sizeof(RX_FILTER_INFO));

	// Just for Data packet !!
	if(!IsDataFrame(frame.Octet))
	{
		pRxfiterInfo->PacketType = RXPacketTypeUndefined;
		return FALSE;
	}
	
	// MAC !!
	pRxfiterInfo->pDA = Frame_pDaddr(frame);
	pRxfiterInfo->pSA = Frame_pSaddr(frame);

	if( MacAddr_isBcst(pRxfiterInfo->pDA))
	{
		pRxfiterInfo->PacketType = RXPacketTypeBroadcast;
	}
	else if( MacAddr_isMulticast(pRxfiterInfo->pDA) )
	{
		pRxfiterInfo->PacketType = RXPacketTypeMulticast;
	}
	else
	{
		pRxfiterInfo->PacketType = RXPacketTypeUnicast;
		return TRUE;
	}
	/*
	if( Frame_ValidAddr4(frame) )
	{
		offset += 6;  May no Address4 frame !!
	}
	*/

	//RT_PRINT_DATA(COMP_TEST, DBG_LOUD, "===> GPGetParseRFDInfo:\n", frame.Octet, frame.Length);
	pRxfiterInfo->pEtherType  = frame.Octet + sMacHdrLng  + LLC_HEADER_SIZE;

	pCurrentbuf = frame.Octet + sMacHdrLng  + LLC_HEADER_SIZE + TYPE_LENGTH_FIELD_SIZE;
	
	// IP !!
	if( PlatformCompareMemory(pRxfiterInfo->pEtherType , ArpType, 2) == 0)
	{
		// ARP
		pRxfiterInfo->ARPOption = pCurrentbuf + 6;
		pRxfiterInfo->ARPSPA = pCurrentbuf + 6 + 2 + 6;  // 6 Option offset , 2 Option Len , 6 SPA Mac Len 
		pRxfiterInfo->ARPTPA = pCurrentbuf + 6 + 2 + 6 + 4 + 6 ; //  // 6 Option offset , 2 Option Len , 6 SPA Mac Len , 4 SPA len , 6 TPA Mac Len
	}
	else if(PlatformCompareMemory(pRxfiterInfo->pEtherType , IPv4Type, 2) == 0)
	{
		pRxfiterInfo->Protocol = pCurrentbuf[9];
		// UDP !!
		if(pRxfiterInfo->Protocol == 0x11)
		{
			u1Byte		IPv4len = (pCurrentbuf[0]&0x0f) * 4;
			pRxfiterInfo->pDestinationPort =  pCurrentbuf + IPv4len + 2;  // (pCurrentbuf[0]>>4) IPv4 Heard len 2 Source Port 
			//RT_PRINT_DATA(COMP_TEST, DBG_LOUD, "IPV4 pRxfiterInfo->pDestinationPort :\n", pRxfiterInfo->pDestinationPort, 2);
		}
		//RT_TRACE(COMP_TEST, DBG_LOUD , ("===> pRfd nTotalSubframe (%d) \n" , pRfd->nTotalSubframe) );
	}
	else if(PlatformCompareMemory(pRxfiterInfo->pEtherType , IPv6Type, 2) == 0)
	{
		
		pRxfiterInfo->Protocol = pCurrentbuf[6];
		// UDP !!
		if(pRxfiterInfo->Protocol == 0x11)
		{
			pRxfiterInfo->pDestinationPort =  pCurrentbuf + 40 + 2; // 40 IPv6 Len , 2 Source Port
			//RT_PRINT_DATA(COMP_TEST, DBG_LOUD, "IPV6 pRxfiterInfo->pDestinationPort :\n", pRxfiterInfo->pDestinationPort, 2);
		}
		//RT_TRACE(COMP_TEST, DBG_LOUD , ("===> pRfd nTotalSubframe (%d) \n" , pRfd->nTotalSubframe) );
	}
	
		
	return TRUE;
	
}


//=============================================================================
// End of Custom parser routines.
//=============================================================================

