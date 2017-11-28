

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Mon Jan 18 19:14:07 2038
 */
/* Compiler settings for RpcInterface.Idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/

#include <string.h>

#include "RpcInterface_h.h"

#define TYPE_FORMAT_STRING_SIZE   23                                
#define PROC_FORMAT_STRING_SIZE   245                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _RpcInterface_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } RpcInterface_MIDL_TYPE_FORMAT_STRING;

typedef struct _RpcInterface_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } RpcInterface_MIDL_PROC_FORMAT_STRING;

typedef struct _RpcInterface_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } RpcInterface_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const RpcInterface_MIDL_TYPE_FORMAT_STRING RpcInterface__MIDL_TypeFormatString;
extern const RpcInterface_MIDL_PROC_FORMAT_STRING RpcInterface__MIDL_ProcFormatString;
extern const RpcInterface_MIDL_EXPR_FORMAT_STRING RpcInterface__MIDL_ExprFormatString;

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: RpcInterface, ver. 1.0,
   GUID={0x906B0CE0,0xC70B,0x1067,{0xB3,0x17,0x00,0xDD,0x01,0x06,0x62,0xDA}} */


extern const MIDL_SERVER_INFO RpcInterface_ServerInfo;


extern const RPC_DISPATCH_TABLE RpcInterface_v1_0_DispatchTable;

static const RPC_CLIENT_INTERFACE RpcInterface___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x906B0CE0,0xC70B,0x1067,{0xB3,0x17,0x00,0xDD,0x01,0x06,0x62,0xDA}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    (RPC_DISPATCH_TABLE*)&RpcInterface_v1_0_DispatchTable,
    0,
    0,
    0,
    &RpcInterface_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE RpcInterface_v1_0_c_ifspec = (RPC_IF_HANDLE)& RpcInterface___RpcClientInterface;

extern const MIDL_STUB_DESC RpcInterface_StubDesc;

static RPC_BINDING_HANDLE RpcInterface__MIDL_AutoBindHandle;


void RemoteOpen( 
    /* [in] */ handle_t hBinding,
    /* [out] */ PPCONTEXT_HANDLE_TYPE pphContext)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&RpcInterface_StubDesc,
                  (PFORMAT_STRING) &RpcInterface__MIDL_ProcFormatString.Format[0],
                  hBinding,
                  pphContext);
    
}


void RemoteClose( 
    /* [out][in] */ PPCONTEXT_HANDLE_TYPE pphContext)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&RpcInterface_StubDesc,
                  (PFORMAT_STRING) &RpcInterface__MIDL_ProcFormatString.Format[36],
                  pphContext);
    
}


void StartMetering( 
    /* [in] */ PCONTEXT_HANDLE_TYPE phContext,
    /* [in] */ __int64 samplePeriod,
    /* [optional][in] */ __int64 context)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&RpcInterface_StubDesc,
                  (PFORMAT_STRING) &RpcInterface__MIDL_ProcFormatString.Format[74],
                  phContext,
                  samplePeriod,
                  context);
    
}


void SetSamplePeriod( 
    /* [in] */ PCONTEXT_HANDLE_TYPE phContext,
    /* [in] */ __int64 samplePeriod)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&RpcInterface_StubDesc,
                  (PFORMAT_STRING) &RpcInterface__MIDL_ProcFormatString.Format[124],
                  phContext,
                  samplePeriod);
    
}


void StopMetering( 
    /* [in] */ PCONTEXT_HANDLE_TYPE phContext)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&RpcInterface_StubDesc,
                  (PFORMAT_STRING) &RpcInterface__MIDL_ProcFormatString.Format[168],
                  phContext);
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const RpcInterface_MIDL_PROC_FORMAT_STRING RpcInterface__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure RemoteOpen */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x38 ),	/* 56 */
/* 18 */	0x40,		/* Oi2 Flags:  has ext, */
			0x1,		/* 1 */
/* 20 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pphContext */

/* 30 */	NdrFcShort( 0x110 ),	/* Flags:  out, simple ref, */
/* 32 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Procedure RemoteClose */

/* 36 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 38 */	NdrFcLong( 0x0 ),	/* 0 */
/* 42 */	NdrFcShort( 0x1 ),	/* 1 */
/* 44 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 46 */	0x30,		/* FC_BIND_CONTEXT */
			0xe4,		/* Ctxt flags:  via ptr, in, out, no serialize, */
/* 48 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 50 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 52 */	NdrFcShort( 0x38 ),	/* 56 */
/* 54 */	NdrFcShort( 0x38 ),	/* 56 */
/* 56 */	0x40,		/* Oi2 Flags:  has ext, */
			0x1,		/* 1 */
/* 58 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pphContext */

/* 68 */	NdrFcShort( 0x118 ),	/* Flags:  in, out, simple ref, */
/* 70 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 72 */	NdrFcShort( 0xe ),	/* Type Offset=14 */

	/* Procedure StartMetering */

/* 74 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 76 */	NdrFcLong( 0x0 ),	/* 0 */
/* 80 */	NdrFcShort( 0x2 ),	/* 2 */
/* 82 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 84 */	0x30,		/* FC_BIND_CONTEXT */
			0x44,		/* Ctxt flags:  in, no serialize, */
/* 86 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 88 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 90 */	NdrFcShort( 0x44 ),	/* 68 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	0x40,		/* Oi2 Flags:  has ext, */
			0x3,		/* 3 */
/* 96 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter phContext */

/* 106 */	NdrFcShort( 0x8 ),	/* Flags:  in, */
/* 108 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 110 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Parameter samplePeriod */

/* 112 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 114 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 116 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter context */

/* 118 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 120 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 122 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Procedure SetSamplePeriod */

/* 124 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 126 */	NdrFcLong( 0x0 ),	/* 0 */
/* 130 */	NdrFcShort( 0x3 ),	/* 3 */
/* 132 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 134 */	0x30,		/* FC_BIND_CONTEXT */
			0x44,		/* Ctxt flags:  in, no serialize, */
/* 136 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 138 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 140 */	NdrFcShort( 0x34 ),	/* 52 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 144 */	0x40,		/* Oi2 Flags:  has ext, */
			0x2,		/* 2 */
/* 146 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter phContext */

/* 156 */	NdrFcShort( 0x8 ),	/* Flags:  in, */
/* 158 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 160 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Parameter samplePeriod */

/* 162 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 164 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 166 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Procedure StopMetering */

/* 168 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x4 ),	/* 4 */
/* 176 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 178 */	0x30,		/* FC_BIND_CONTEXT */
			0x44,		/* Ctxt flags:  in, no serialize, */
/* 180 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 182 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 184 */	NdrFcShort( 0x24 ),	/* 36 */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 188 */	0x40,		/* Oi2 Flags:  has ext, */
			0x1,		/* 1 */
/* 190 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter phContext */

/* 200 */	NdrFcShort( 0x8 ),	/* Flags:  in, */
/* 202 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 204 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Procedure MeteringDataEvent */

/* 206 */	0x34,		/* FC_CALLBACK_HANDLE */
			0x48,		/* Old Flags:  */
/* 208 */	NdrFcLong( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 216 */	NdrFcShort( 0x20 ),	/* 32 */
/* 218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 220 */	0x40,		/* Oi2 Flags:  has ext, */
			0x2,		/* 2 */
/* 222 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter data */

/* 232 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 234 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 236 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter context */

/* 238 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 240 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 242 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

			0x0
        }
    };

static const RpcInterface_MIDL_TYPE_FORMAT_STRING RpcInterface__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/*  4 */	NdrFcShort( 0x2 ),	/* Offset= 2 (6) */
/*  6 */	0x30,		/* FC_BIND_CONTEXT */
			0xa4,		/* Ctxt flags:  via ptr, out, no serialize, */
/*  8 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 10 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 12 */	NdrFcShort( 0x2 ),	/* Offset= 2 (14) */
/* 14 */	0x30,		/* FC_BIND_CONTEXT */
			0xe5,		/* Ctxt flags:  via ptr, in, out, no serialize, can't be null */
/* 16 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 18 */	0x30,		/* FC_BIND_CONTEXT */
			0x45,		/* Ctxt flags:  in, no serialize, can't be null */
/* 20 */	0x0,		/* 0 */
			0x0,		/* 0 */

			0x0
        }
    };

static const unsigned short RpcInterface_FormatStringOffsetTable[] =
    {
    0,
    36,
    74,
    124,
    168,
    };


static const unsigned short _callbackRpcInterface_FormatStringOffsetTable[] =
    {
    206
    };


static const MIDL_STUB_DESC RpcInterface_StubDesc = 
    {
    (void *)& RpcInterface___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &RpcInterface__MIDL_AutoBindHandle,
    0,
    0,
    0,
    0,
    RpcInterface__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x801026e, /* MIDL Version 8.1.622 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

static const RPC_DISPATCH_FUNCTION RpcInterface_table[] =
    {
    NdrServerCall2,
    0
    };
static const RPC_DISPATCH_TABLE RpcInterface_v1_0_DispatchTable = 
    {
    1,
    (RPC_DISPATCH_FUNCTION*)RpcInterface_table
    };

static const SERVER_ROUTINE RpcInterface_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)MeteringDataEvent
    };

static const MIDL_SERVER_INFO RpcInterface_ServerInfo = 
    {
    &RpcInterface_StubDesc,
    RpcInterface_ServerRoutineTable,
    RpcInterface__MIDL_ProcFormatString.Format,
    _callbackRpcInterface_FormatStringOffsetTable,
    0,
    0,
    0,
    0};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

