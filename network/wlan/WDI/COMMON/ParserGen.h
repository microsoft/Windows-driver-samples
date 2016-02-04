/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	ParserGen.h
	
Abstract:
	Parse MSDU to match the specified type, fetch fixed offset.
	    
Major Change History:
	When           Who					What
	---------- ---------------   -------------------------------
	2007-06-11	shienchang			Create.
	2008-03-10	Bruce				Modify from 818xB.
	
--*/
#ifndef __INC_PARSERGEN_H
#define __INC_PARSERGEN_H
/////////////////////////////////////////////////////////
//  Global defines
/////////////////////////////////////////////////////////

//
// Protocol identity define.
// Add your own id here if new parser need.
//
#define PROTO_UNKNOWN			0
#define PROTO_ETHERNET			1
#define PROTO_WLAN_80211		2
#define PROTO_WLAN_LLC			3
#define PROTO_IP				4
#define PROTO_TCP				5
#define PROTO_UDP				6
#define PROTO_RTP				7
#define PROTO_SIP_INVITE		8
#define PROTO_SIP_STATUS		9
#define PROTO_SIP_BYE			10
#define PROTO_SIP_NOTIFY		11
#define PROTO_SIP_ACK			12
#define PROTO_SIP_SDP			13
#define PROTO_RTP_G711			14

//
// Protocol fixed header length
//
#define PROTO_UNKNOWN_HDRLEN			0
#define PROTO_ETHERNET_HDRLEN			(12+2)
#define PROTO_WLAN_LLC_HDRLEN			(6+2)
#define PROTO_IP_HDRLEN					20
#define PROTO_UDP_HDRLEN				8

// Flag defines.
#define GPFLAG_TX						1
#define GPFLAG_RX						2


// Basic parse block
#define MAX_PROTOCOL_NUM				32

//
// Main generic parser defines.
//
#define MAX_PARSER_NUM					16
#define MAX_PARSER_NAME					32
#define MAX_SIG_LENGTH					32

#define MAX_HP_PROTOCOL_PER_PARSER		32
#define MAX_HP_RULES_PER_PARSER			64

////////////////////////////////////////////////////////
// Macro Definition
////////////////////////////////////////////////////////

#define GP_REGISTER_RULES(_parser, _rules) \
	(_parser)->pParseRules = (_rules)

#define GP_UNREGISTER_RULES(_parser) \
	(_parser)->pParseRules = NULL

#define GP_GET_CONTEXT(_parser) \
	(_parser)->ParserContext

#define GP_HP_GET_NODE_START(_pTree) \
	(_pTree)

#define GP_HP_GET_ENTRY_START(_pTree) \
	((_pTree) + MAX_HP_PROTOCOL_PER_PARSER)

#define GP_BIND_CONTEXT(_parser, _context) \
	(_parser)->ParserContext = (_context)

#define GP_UNBIND_CONTEXT(_parser) \
	(_parser)->ParserContext = NULL

#define GP_HP_GET_NODE_BY_INDEX(_pTree, _idx) \
	((PGP_HP_NODE)GP_HP_GET_NODE_START(_pTree)) + (_idx)

#define GP_HP_GET_ENTRY_BY_INDEX(_pTree, _idx) \
	((PGP_HP_ENTRY)GP_HP_GET_ENTRY_START(_pTree)) + (_idx)


typedef u4Byte	PROTO_ID, *PPROTO_ID;

typedef struct _GPPARSE_TOKEN
{
	SHARED_MEMORY		BufferList[MAX_PER_PACKET_BUFFER_LIST_LENGTH];
	u4Byte				BufferCount;
	u4Byte				currBufferIndex;
	u4Byte				currBufferOffset;
	PROTO_ID			currProtocol;
	PROTO_ID			ProtocolSuite[MAX_PROTOCOL_NUM];
	u1Byte				ProtocolCount;
	PVOID				pDataObj;
	u1Byte				gpFlag;
} GPPARSE_TOKEN, *PGPPARSE_TOKEN;


// For TCB/RFD action, 2007.06.21 by shien chang.
typedef enum{
	TR_ACTION_CONTINUE = 0,
	TR_ACTION_HANDLED = 1,
	TR_ACTION_DROPPED = 2,
}TR_ACTION;

typedef BOOLEAN
	(*GPParseHandler)(
		IN		PADAPTER		Adapter,
		IN		PVOID			pParseContext,
		IN		PROTO_ID		CurrProtocol,
		IN		PROTO_ID		NextProtocol,
		OUT		pu1Byte			pSigBuffer,
		IN		u4Byte			SigBufferLen,
		IN		PGPPARSE_TOKEN	pToken,
		OUT		pu4Byte			AdvBytes
		);


typedef struct _GP_RULE
{
	u4Byte				CurrProtocol;
	u4Byte				NextProtocol;
	u4Byte				SigStart;
	u4Byte				SigLength;
	GPParseHandler		ParseHandler;
} GP_RULE, *PGP_RULE;

typedef BOOLEAN
	(*GPParserGate)(
		IN		PADAPTER		Adapter,
		IN		PVOID			pParser
		);

typedef TR_ACTION
	(*GPParserAction)(
		IN		PADAPTER		Adapter,
		IN		PGPPARSE_TOKEN	pToken,
		IN		PVOID			pParser
		);

typedef struct _GP_HP_ENTRY
{
	RT_LIST_ENTRY	Link;
	PGP_RULE		pRule;
	u1Byte			index;
} GP_HP_ENTRY, *PGP_HP_ENTRY;


typedef struct _GP_HP_NODE
{
	RT_LIST_ENTRY	Link;
	PROTO_ID		ProtocolId;
	BOOLEAN			bUsed;
} GP_HP_NODE, *PGP_HP_NODE;


//
// Description:
//	PID:
//		This generic parser identifier.
//	ParserName:
//		The name of this generic parser.
//	ParserRules:
//		The corrresponding handler and the next protocol to the current protocol. 
//	ParserContext:
//		The input context about for this parser.
//	Gate:
//		A routine to determine if this parser should go parsing it or not.
//	Action:
//		An action routine to handle parserd packet.
//	pParsingTree:
//
//	bUseHP:
//		Use high performance parser.
// By Bruce, 2008-03-11.
//
typedef struct _GENERIC_PARSER
{
	u1Byte			PID;
	u1Byte			ParserName[MAX_PARSER_NAME];
	PGP_RULE		pParseRules;
	PVOID			ParserContext;	
	GPParserGate	Gate;
	GPParserAction	Action;
	PGP_HP_NODE		pParsingTree;
	BOOLEAN			bUseHP;
} GENERIC_PARSER, *PGENERIC_PARSER;

//
// Interface.
//
typedef BOOLEAN
	(*ifAllocateParser)(
		IN		PADAPTER			Adapter, 
		IN		PGENERIC_PARSER		pParser
		);

typedef BOOLEAN
	(*ifFreeParser)(
		IN		PADAPTER	Adapter,
		IN		PGENERIC_PARSER		pParser
		);



//=============================================================================
//	Prototype of public function in ParserGen.c.
//=============================================================================

//
// Parser routines.
//
#if DBG
VOID
DbgProtocol(
	IN		u4Byte				protocol
	);
#else
	#define DbgProtocol(_p)
#endif


VOID
GPUseHPParser(
	IN		PGENERIC_PARSER	pParser
	);

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
	);

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
	);

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
	);

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
	);

BOOLEAN
GPAllocateParser(
	IN		PADAPTER			Adapter,
	IN		u1Byte				Pid,
	IN		const char*				strParserName,
	IN		ifAllocateParser	AllocateRoutine,
	IN		GPParserGate		ParserGateRoutine,
	IN		GPParserAction		ParserActionRoutine,
	IN		BOOLEAN				bUseHP
	);

VOID
GPFreeParser(
	IN		PADAPTER		Adapter,
	IN		u1Byte			Pid,
	IN		ifFreeParser	FreeRoutine
	);


TR_ACTION
GPParseTCB(
	IN		PADAPTER	Adapter,
	IN		PRT_TCB		pTcb,
	IN		PROTO_ID	InitProtocol
	);

TR_ACTION
GPParseRFD(
	IN		PADAPTER	Adapter,
	IN		PRT_RFD		pRfd,
	IN		PROTO_ID	InitProtocol
	);





//=============================================================================
//	End of Prototype of public function in ParserGen.c.
//=============================================================================

//=============================================================================
// Customized parser section of GP_VOWLAN_SIP.
//=============================================================================

//
// Custom parser defines.
//
#define GP_VOWLAN_SIP			1	// The SIP Custom ID

#define MAX_SESSION				16
#define MAX_SESSION_ID			80

// Signature start.
#define UNKNOWN_SIG				0

typedef struct _SESSION
{
	u2Byte		srcPort;
	u2Byte		dstPort;
	u1Byte		sessionId[MAX_SESSION_ID];
	u4Byte		sessionIdLen;
	BOOLEAN		bUsed;
} SESSION, *PSESSION;

typedef struct _GP_VOWLAN_CONTEXT
{
	SESSION		Session[MAX_SESSION];
} GP_VOWLAN_CONTEXT, *PGP_VOWLAN_CONTEXT;

BOOLEAN
GPParserGateVoWlanSIP(
	IN		PADAPTER	Adapter,
	IN		PVOID		pParser
	);

TR_ACTION
GPParserActionVoWlanSIP(
	IN		PADAPTER	Adapter,
	IN		PGPPARSE_TOKEN		pToken,
	IN		PVOID		pParser
	);

BOOLEAN
GPAllocateParserVoWlanSIP(
	IN		PADAPTER			Adapter,
	IN		PGENERIC_PARSER		pParser
	);


BOOLEAN
GPFreeParserVoWlanSIP(
	IN		PADAPTER			Adapter,
	IN		PGENERIC_PARSER		pParser
	);

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
	);

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
	);

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
	);

BOOLEAN
GetCallId(
	IN		PGPPARSE_TOKEN	pToken,
	IN		BOOLEAN			bNoAdvance,
	OUT		pu1Byte			callId,
	OUT		pu4Byte			callIdLen
	);

BOOLEAN
AddSession(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		pu1Byte				sessionId,
	IN		u4Byte				sessionIdLen
	);


VOID
RemoveSession(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		pu1Byte				sessionId,
	IN		u4Byte				sessionIdLen
	);

BOOLEAN
UpdateSessionInfo(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		pu1Byte				sessionId,
	IN		u4Byte				sessionIdLen,
	IN		u2Byte				srcPort,
	IN		u2Byte				dstPort
	);

BOOLEAN
MatchSession(
	IN		PGP_VOWLAN_CONTEXT	pContext,
	IN		u2Byte				Port,
	IN		u1Byte				Flag
	);

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
	);

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
	);

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
	);

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
	);

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
	);

BOOLEAN
GPGetParseRFDInfo(
	IN		PADAPTER		Adapter,
	IN		PRT_RFD			pRfd,
	IN		PRX_FILTER_INFO	pRxfiterInfo
	);


//=============================================================================
// End of Customized parser section of GP_VOWLAN_SIP.
//=============================================================================


#endif // end of __INC_PARSERGEN_H

