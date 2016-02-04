#ifndef __INC_HAL_FW_H
#define __INC_HAL_FW_H


#define	FW_QUEME_MECHANISM_NEW	0

#define	FW_QUEUE_IDLE			0
#define	FW_QUEUE_WAIT			1

#define	FW_H2C_MAX_NUM		20

#if(FW_QUEME_MECHANISM_NEW == 1)
typedef struct _FW_COM_STR{
	VIRTUAL_MEMORY		fwDataListBuf;
	u4Byte				NumTotalH2c;
	RT_LIST_ENTRY		halFwH2cWaitQueue;
	u4Byte				NumFwWaitH2c;
	RT_LIST_ENTRY		halFwH2cIdleQueue;
	u4Byte				NumFwIdleH2c;
}FW_COM_STR, *PFW_COM_STR;
#else
#define	FW_COM_STR	u1Byte
#endif

// H2C command structre to keep the info for USB/SDIO interface.
// The H2C cmd format in 92S is different from 92C/88E series
// so use_H2C_CMD _8192C to separare 92S and 92C/88E series.
typedef struct _H2C_CMD_8192C
{
	u1Byte 	ElementID;
	u4Byte 	CmdLen;
	u4Byte	CmdBuffer[2];
	u1Byte	InProgress;
}H2C_CMD_8192C,*PH2C_CMD_8192C;

typedef struct _H2C_CMD_ENTRY
{
	RT_LIST_ENTRY	listEntry;
	H2C_CMD_8192C	h2cCmd;
}H2C_CMD_ENTRY,*PH2C_CMD_ENTRY;

#if(FW_QUEME_MECHANISM_NEW == 1)
//==================================================
//	extern function
//==================================================
RT_STATUS
FW_AllocateMemory(
	IN	PADAPTER	Adapter
	);
VOID
FW_FreeMemory(
	IN	PADAPTER	Adapter
	);
BOOLEAN
FW_SendH2c(
	IN	PADAPTER	Adapter,
	IN	u1Byte		elementId,
	IN	u4Byte		dataLen,
	IN	pu1Byte		pData
	);
BOOLEAN
FW_RetrieveH2c(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pData
	);
BOOLEAN
FW_IsH2cWaitQueueEmpty(
	IN	PADAPTER	Adapter
	);
#else
#define	FW_AllocateMemory(Adapter)				RT_STATUS_SUCCESS
#define	FW_FreeMemory(Adapter)
#endif

VOID
FW_InitializeVariables(
	IN	PADAPTER	Adapter
	);

VOID
FW_FillH2CCmd(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
	);


VOID
FW_WaitForH2CQueueEmpty(
	IN	PADAPTER	pAdapter
	);

VOID
FW_InitializeHALVars(
	IN	PADAPTER		Adapter
	);

RT_STATUS
FW_FirmwareDownload(
	IN	PADAPTER 	Adapter,
	IN	BOOLEAN 	bUsedWoWLANFw
	);

#endif
