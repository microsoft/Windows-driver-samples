/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrCmd.h

Abstract:

	Prototypes of basic command functions for SCM PSCR smartcard reader

Author:

	Andreas Straub

Notes:

    The file pscrcmd.h was reviewed by LCA in June 2011 and per license is
    acceptable for Microsoft use under Dealpoint ID 178449.

Revision History:

	Andreas Straub			7/16/1997	Initial Version

--*/

#if !defined( __PSCR_CMD_H__ )
#define __PSCR_CMD_H__

NTSTATUS
CmdResetInterface( 
	PREADER_EXTENSION	ReaderExtension			//	context of call
	);

NTSTATUS
CmdReset( 
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	UCHAR				Device,					//	device
	BOOLEAN				WarmReset,				//	TRUE: Warm, FALSE: cold Reset
	PUCHAR				pATR,					//	ptr to ATR buffer
	PULONG				ATRLength				//	len of ATR
	);

NTSTATUS
CmdDeactivate( 
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	UCHAR				Device					//	device
	);

NTSTATUS
CmdReadBinary(
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	USHORT				Offset,					//	offset in file
	PUCHAR				pData,					//	data buffer
	PULONG				pNBytes					//	length of bytes read
	);

NTSTATUS
CmdSelectFile(
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	USHORT				FileId					//	File Id
	);

NTSTATUS
CmdSetInterfaceParameter(
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	UCHAR				Device,					//	device
	PUCHAR				pTLVList,				//	ptr to TLV list
	UCHAR				TLVListLen				//	len of TLV list
	);

NTSTATUS
CmdReadStatusFile (
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	UCHAR				Device,					//	device
	PUCHAR				pTLVList,				//	ptr to TLV list
	PULONG				TLVListLen				//	len of TLV list
	);

NTSTATUS
CmdPscrCommand (
	PREADER_EXTENSION	ReaderExtension,		//	context of call
	PUCHAR				pInData,				//	ptr to input buffer
	ULONG				InDataLen,				//	len of input buffer
	PUCHAR				pOutData,				//	ptr to ouput buffer
	ULONG				OutDataLen,				//	len of output buffer
	PULONG				pNBytes					//	number of bytes transferred
	);

NTSTATUS
CmdGetFirmwareRevision (
	PREADER_EXTENSION	ReaderExtension			//	context of call
	);

NTSTATUS
CmdGetTagValue (
	UCHAR				Tag,					//	tag to be searched
	PUCHAR				pTLVList,				//	ptr to TLV list
	ULONG				TLVListLen,				//	len of TLV list
	PUCHAR				pTagLen,				//	tag length
	PVOID				pTagVal					//	tag value
	);

#endif // __PSCR_CMD_H__

//------------------------------- END OF FILE -------------------------------*/
