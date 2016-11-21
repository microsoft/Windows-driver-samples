#include "Mp_Precomp.h"

//-----------------------------------------------------------------------------
//	Description:
//		Callback function of DbgWorkItem.
//
//-----------------------------------------------------------------------------

// global buf for BT c2h rsp.
u1Byte	GLCurBtC2hRsp[20];
u1Byte	GLCurBtStackRsp[100];

BT_CTRL_STATUS
ndbg_CheckBtRspStatus(
	IN	PADAPTER			Adapter
	)
{
	BT_CTRL_STATUS	retStatus=BT_OP_STATUS_SUCCESS;
	PBT_EXT_C2H 		pExtC2h=(PBT_EXT_C2H)&GLCurBtC2hRsp[0];

	switch(pExtC2h->statusCode)
	{
		case BT_OP_STATUS_SUCCESS:
			retStatus = BT_STATUS_BT_OP_SUCCESS;
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], BT status : MP_BT_STATUS_SUCCESS\n"));
			break;
		case BT_OP_STATUS_VERSION_MISMATCH:
			retStatus = BT_STATUS_OPCODE_L_VERSION_MISMATCH;
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], BT status : MP_BT_STATUS_OPCODE_L_VERSION_MISMATCH\n"));
			break;
		case BT_OP_STATUS_UNKNOWN_OPCODE:
			retStatus = BT_STATUS_UNKNOWN_OPCODE_L;
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], BT status : MP_BT_STATUS_UNKNOWN_OPCODE_L\n"));
			break;
		case BT_OP_STATUS_ERROR_PARAMETER:
			retStatus = BT_STATUS_PARAMETER_FORMAT_ERROR_L;
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], BT status : MP_BT_STATUS_PARAMETER_FORMAT_ERROR_L\n"));
			break;
		default:
			retStatus = BT_STATUS_UNKNOWN_STATUS_L;
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], BT status : MP_BT_STATUS_UNKNOWN_STATUS_L\n"));
			break;
	}
	
	return retStatus;
}	

BT_CTRL_STATUS
ndbg_CheckC2hFrame(
	IN	PADAPTER		Adapter,
	IN	PBT_H2C			pH2c
	)
{
	BT_CTRL_STATUS	c2hStatus=BT_STATUS_C2H_SUCCESS;
	PBT_EXT_C2H 		pExtC2h=(PBT_EXT_C2H)&GLCurBtC2hRsp[0];
		
	RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], C2H rsp hex: \n"), pExtC2h, 6);

	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], statusCode = 0x%x\n", pExtC2h->statusCode));
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], retLen = %d\n", pExtC2h->retLen));
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], opCodeVer : req/rsp=%d/%d\n", pH2c->opCodeVer, pExtC2h->opCodeVer));
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], reqNum : req/rsp=%d/%d\n", pH2c->reqNum, pExtC2h->reqNum));
	if(pExtC2h->reqNum != pH2c->reqNum)
	{
		c2hStatus = BT_STATUS_C2H_REQNUM_MISMATCH;
		RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! C2H reqNum Mismatch!!\n"));
	}
	else if(pExtC2h->opCodeVer != pH2c->opCodeVer)
	{
		c2hStatus = BT_STATUS_OPCODE_L_VERSION_MISMATCH;
		RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! OPCode version L mismatch!!\n"));
	}

	return c2hStatus;
}

BT_CTRL_STATUS
ndbg_SendH2c(
	IN	PADAPTER	Adapter,
	IN	PBT_H2C		pH2c,
	IN	u2Byte		h2cCmdLen
	)
{
	KIRQL				OldIrql = KeGetCurrentIrql();
	BT_CTRL_STATUS		h2cStatus=BT_STATUS_H2C_SUCCESS;
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(Adapter->ndisDbgCtx);
	u1Byte				i;

	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], ndbg_SendH2c()=========>\n"));

	PlatformResetEvent(&pDbgCtx->dbgH2cRspEvent);
	PlatformResetEvent(&pDbgCtx->dbgBtC2hEvent);

	if(OldIrql == PASSIVE_LEVEL)
	{
		RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], H2C hex: \n"), pH2c, h2cCmdLen);

		for(i=0; i<BT_H2C_MAX_RETRY; i++)
		{
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Send H2C command to wifi!!!\n"));
			
			pDbgCtx->h2cReqNum++;
			pDbgCtx->h2cReqNum %= 16;
			if(PlatformWaitEvent(&pDbgCtx->dbgH2cRspEvent, 100))
			{
				RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Received H2C Rsp Event!!!\n"));
				if(PlatformWaitEvent(&pDbgCtx->dbgBtC2hEvent, 500))
				{
					RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Received BT C2H Event!!!\n"));
					break;
				}
				else
				{
					RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! BT C2H Event timeout!!\n"));
					h2cStatus = BT_STATUS_H2C_BT_NO_RSP;
				}
			}
			else
			{
				RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! H2C Rsp Event timeout!!\n"));
				h2cStatus = BT_STATUS_H2C_TIMTOUT;
			}
		}
	}
	else
	{
		RT_ASSERT(FALSE, ("[BTDBG],  ndbg_SendH2c() can only run under PASSIVE_LEVEL!!\n"));
		h2cStatus = BT_STATUS_WRONG_LEVEL;
	}

	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], ndbg_SendH2c()<=========\n"));
	return h2cStatus;
}


BT_CTRL_STATUS
ndbg_BtFwOpCodeProcess(
	IN	PADAPTER		Adapter,
	IN	u1Byte			btFwOpCode,
	IN	u1Byte			opCodeVer,
	IN	pu1Byte			pH2cPar,
	IN	u1Byte			h2cParaLen
	)
{
	u1Byte				H2C_Parameter[20] ={0};
	PBT_H2C				pH2c=(PBT_H2C)&H2C_Parameter[0];
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(Adapter->ndisDbgCtx);
	u2Byte				paraLen=0;
	BT_CTRL_STATUS		h2cStatus=BT_STATUS_H2C_SUCCESS, c2hStatus=BT_STATUS_C2H_SUCCESS;
	BT_CTRL_STATUS		retStatus=BT_STATUS_H2C_BT_NO_RSP;

	pH2c->opCode = btFwOpCode;
	pH2c->opCodeVer = opCodeVer;
	pH2c->reqNum = pDbgCtx->h2cReqNum;
	PlatformMoveMemory(&pH2c->buf[0], pH2cPar, h2cParaLen);

	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pH2c->opCode=%d\n", pH2c->opCode));
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pH2c->opCodeVer=%d\n", pH2c->opCodeVer));
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pH2c->reqNum=%d\n", pH2c->reqNum));
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], h2c parameter length=%d\n", h2cParaLen));
	if(h2cParaLen)
	{
		RT_DISP_DATA(FBT, BT_DBG_STATE, ("[BTDBG], parameters(hex): \n"), &pH2c->buf[0], h2cParaLen);
	}

	h2cStatus = ndbg_SendH2c(Adapter, pH2c, h2cParaLen+2);
	if(BT_STATUS_H2C_SUCCESS == h2cStatus)
	{
		// if reach here, it means H2C get the correct c2h response, 
		c2hStatus = ndbg_CheckC2hFrame(Adapter, pH2c);
		if(BT_STATUS_C2H_SUCCESS == c2hStatus)
		{
			retStatus = ndbg_CheckBtRspStatus(Adapter);
		}
		else
		{
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! C2H failed for pH2c->opCode=%d\n", pH2c->opCode));
			// check c2h status error, return error status code to upper layer.
			retStatus = c2hStatus;
		}
	}
	else
	{
		RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! H2C failed for pH2c->opCode=%d\n", pH2c->opCode));
		// check h2c status error, return error status code to upper layer.
		retStatus = h2cStatus;
	}

	return retStatus;
}

BT_CTRL_STATUS
ndbg_BtStackOpCodeProcess(
	IN	PADAPTER		Adapter,
	IN	u1Byte			dataLen,
	IN	PVOID			pData
	)
{
	KIRQL				OldIrql = KeGetCurrentIrql();
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(Adapter->ndisDbgCtx);
	BT_CTRL_STATUS		retStatus=BT_STATUS_BT_STACK_NO_RSP;
	PBT_STACK_COEX_INFO	pStackRsp=(PBT_STACK_COEX_INFO)&GLCurBtStackRsp[0];

	PlatformResetEvent(&pDbgCtx->dbgBtCoexEvent);


	if(OldIrql == PASSIVE_LEVEL)
	{
		if(PlatformWaitEvent(&pDbgCtx->dbgBtCoexEvent, 100))
		{
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Received BT Coex Event from stack!!!\n"));
			retStatus = (BT_CTRL_STATUS)pStackRsp->opStatus;
		}
		else
		{
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! BT Coex Rsp Event timeout!!\n"));
			retStatus = BT_STATUS_BT_STACK_NO_RSP;
		}
	}
	else
		retStatus = BT_STATUS_WRONG_LEVEL;
		
	return retStatus;
}

VOID
DbgWorkItemCallback(
	IN PVOID			pContext
	)
{
	PADAPTER				pAdapter = (PADAPTER)pContext;
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
		
	// Execute specified action.
	if(pDbgCtx->CurrDbgAct != NULL)
	{
#if DBG
		u8Byte StartTime, EndTime;
		StartTime = PlatformGetCurrentTime();
#endif

		pDbgCtx->CurrDbgAct(pAdapter);

#if DBG
		EndTime = PlatformGetCurrentTime();
		RT_TRACE(COMP_DBG, DBG_LOUD, 
			("DbgActType: %d, time spent: %I64d us\n",
			pDbgCtx->DbgActType, (EndTime-StartTime) )); 
#endif
	}

	PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	pDbgCtx->bDbgWorkItemInProgress = FALSE;
	PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	
	if(pDbgCtx->bDbgDrvUnload)
	{
		PlatformSetEvent( &(pDbgCtx->DbgWorkItemEvent) );
	}
}

//-----------------------------------------------------------------------------
//	Description:
//		Callback function of a workitem for IO.	
//
//-----------------------------------------------------------------------------
VOID
DbgIoCallback(
	IN	PVOID	Context
	)
{
	PADAPTER		pAdapter = (PADAPTER)Context;
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(pAdapter);

	RT_ASSERT((KeGetCurrentIrql() == PASSIVE_LEVEL), 
		("DbgIoCallback(): not in PASSIVE_LEVEL!\n"));

	switch(pDbgCtx->DbgActType)
	{
	//
	// <Roger_Notes> The following IO items are for RTL8192SU purpose.
	// These are just instead of CMD I/O case to compatible Command Line Utility.
	// 2008.09.02.
	//
	case DBG_WRITE_MAC_1BYTE:
		if( IS_BB_REG_OFFSET_92S(pDbgCtx->DbgIoOffset) )
		{
			ULONG mod_val=0;
			mod_val = pDbgCtx->DbgIoOffset%4;
			if(mod_val)
				pDbgCtx->DbgIoOffset -= mod_val;
			PHY_SetBBReg(pAdapter, pDbgCtx->DbgIoOffset, (u4Byte)(bMaskByte0<<(8*mod_val)), pDbgCtx->DbgIoValue);
		}
		else
		{
			if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_LOCAL)
			{// Local REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					PlatformEFSdioLocalCmd52Write1Byte(pAdapter, pDbgCtx->DbgIoOffset, (u1Byte)(pDbgCtx->DbgIoValue));
				else
					PlatformEFSdioLocalCmd53Write1Byte(pAdapter, pDbgCtx->DbgIoOffset, (u1Byte)(pDbgCtx->DbgIoValue));
			}
			else
			{ // IO REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					PlatformEFSdioCmd52Write1Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 1, pDbgCtx->DbgIoOffset, (u1Byte)(pDbgCtx->DbgIoValue));
				else
					PlatformEFIOWrite1Byte(pAdapter, pDbgCtx->DbgIoOffset, (u1Byte)(pDbgCtx->DbgIoValue));
			}
		}
		break;

	case DBG_WRITE_MAC_2BYTE:
		if( IS_BB_REG_OFFSET_92S(pDbgCtx->DbgIoOffset) )
		{
			ULONG mod_val=0;
			mod_val = pDbgCtx->DbgIoOffset%4;
			if(mod_val)
				pDbgCtx->DbgIoOffset -= mod_val;
			PHY_SetBBReg(pAdapter, pDbgCtx->DbgIoOffset, (bMaskLWord<<(8*mod_val)), pDbgCtx->DbgIoValue);
		}
		else
		{
			if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_LOCAL)
			{// Local REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					PlatformEFSdioLocalCmd52Write2Byte(pAdapter, pDbgCtx->DbgIoOffset, (u2Byte)(pDbgCtx->DbgIoValue));
				else
					PlatformEFSdioLocalCmd53Write2Byte(pAdapter, pDbgCtx->DbgIoOffset, (u2Byte)(pDbgCtx->DbgIoValue));
			}
			else
			{ // IO REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					PlatformEFSdioCmd52Write2Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 1, pDbgCtx->DbgIoOffset, (u2Byte)(pDbgCtx->DbgIoValue));
				else
					PlatformEFIOWrite2Byte(pAdapter, pDbgCtx->DbgIoOffset, (u2Byte)(pDbgCtx->DbgIoValue));
			}
		}
		break;

	case DBG_WRITE_MAC_4BYTE:
		if( IS_BB_REG_OFFSET_92S(pDbgCtx->DbgIoOffset) )
			PHY_SetBBReg(pAdapter, pDbgCtx->DbgIoOffset, bMaskDWord, pDbgCtx->DbgIoValue);
		else
		{
			if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_LOCAL)
			{// Local REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					PlatformEFSdioLocalCmd52Write4Byte(pAdapter, pDbgCtx->DbgIoOffset, (u4Byte)(pDbgCtx->DbgIoValue));
				else
					PlatformEFSdioLocalCmd53Write4Byte(pAdapter, pDbgCtx->DbgIoOffset, (u4Byte)(pDbgCtx->DbgIoValue));
			}
			else
			{ // IO REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					PlatformEFSdioCmd52Write4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 1, pDbgCtx->DbgIoOffset, (u4Byte)(pDbgCtx->DbgIoValue));		
				else
					PlatformEFIOWrite4Byte(pAdapter, pDbgCtx->DbgIoOffset, (u4Byte)(pDbgCtx->DbgIoValue));
			}
		}
		break;
	
	case DBG_READ_MAC_1BYTE:
		if( IS_BB_REG_OFFSET_92S(pDbgCtx->DbgIoOffset) )
		{
			ULONG mod_val=0;
			mod_val = pDbgCtx->DbgIoOffset%4;
			if(mod_val)
				pDbgCtx->DbgIoOffset -= mod_val;
			pDbgCtx->DbgIoValue = PHY_QueryBBReg(pAdapter, pDbgCtx->DbgIoOffset, bMaskDWord);
			pDbgCtx->DbgIoValue &= (bMaskByte0<<(8*mod_val));
			pDbgCtx->DbgIoValue = (pDbgCtx->DbgIoValue>>(8*mod_val));
		}
		else
		{
			if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_LOCAL)
			{// Local REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					pDbgCtx->DbgIoValue = PlatformEFSdioLocalCmd52Read1Byte(pAdapter, pDbgCtx->DbgIoOffset);
				else
					pDbgCtx->DbgIoValue = PlatformEFSdioLocalCmd53Read1Byte(pAdapter, pDbgCtx->DbgIoOffset);
			}
			else
			{ // IO REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					pDbgCtx->DbgIoValue = PlatformEFSdioCmd52Read1Byte(pAdapter,  WLAN_IOREG_DEVICE_ID, 1,pDbgCtx->DbgIoOffset);		
				else
					pDbgCtx->DbgIoValue = PlatformEFIORead1Byte(pAdapter, pDbgCtx->DbgIoOffset);
			}
		}
		break;

	case DBG_READ_MAC_2BYTE:
		if( IS_BB_REG_OFFSET_92S(pDbgCtx->DbgIoOffset) )
		{
			ULONG mod_val=0;
			mod_val = pDbgCtx->DbgIoOffset%4;
			if(mod_val)
				pDbgCtx->DbgIoOffset -= mod_val;
			pDbgCtx->DbgIoValue = PHY_QueryBBReg(pAdapter, pDbgCtx->DbgIoOffset, bMaskDWord);
			pDbgCtx->DbgIoValue &= (bMaskLWord<<(8*mod_val));
			pDbgCtx->DbgIoValue = (pDbgCtx->DbgIoValue>>(8*mod_val));
		}
		else
		{
			if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_LOCAL)
			{// Local REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					pDbgCtx->DbgIoValue = PlatformEFSdioLocalCmd52Read2Byte(pAdapter, pDbgCtx->DbgIoOffset);
				else
					pDbgCtx->DbgIoValue = PlatformEFSdioLocalCmd53Read2Byte(pAdapter, pDbgCtx->DbgIoOffset);
			}
			else
			{ // IO REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					pDbgCtx->DbgIoValue = PlatformEFSdioCmd52Read2Byte(pAdapter,  WLAN_IOREG_DEVICE_ID, 1,pDbgCtx->DbgIoOffset);		
				else
					pDbgCtx->DbgIoValue = PlatformEFIORead2Byte(pAdapter, pDbgCtx->DbgIoOffset);
			}
		}
		break;

	case DBG_READ_MAC_4BYTE:
		if( IS_BB_REG_OFFSET_92S(pDbgCtx->DbgIoOffset) )
			pDbgCtx->DbgIoValue = PHY_QueryBBReg(pAdapter, pDbgCtx->DbgIoOffset, bMaskDWord);
		else
		{
			if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_LOCAL)
			{// Local REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					pDbgCtx->DbgIoValue = PlatformEFSdioLocalCmd52Read4Byte(pAdapter, pDbgCtx->DbgIoOffset);
				else
					pDbgCtx->DbgIoValue = PlatformEFSdioLocalCmd53Read4Byte(pAdapter, pDbgCtx->DbgIoOffset);
			}
			else
			{ // IO REG
				if(pDevice->SdioRegDbgCtrl & SDIO_REG_CTRL_CMD52)
					pDbgCtx->DbgIoValue = PlatformEFSdioCmd52Read4Byte(pAdapter,  WLAN_IOREG_DEVICE_ID, 1,pDbgCtx->DbgIoOffset);		
				else
					pDbgCtx->DbgIoValue = PlatformEFIORead4Byte(pAdapter, pDbgCtx->DbgIoOffset);
			}
		}
		break;

	case DBG_READ_BB_OFDM:
	case DBG_READ_BB_CCK:
		pDbgCtx->DbgIoValue = PHY_QueryBBReg(pAdapter, pDbgCtx->DbgIoOffset, bMaskDWord);
		break;

	case DBG_WRITE_BB_OFDM:
	case DBG_WRITE_BB_CCK:
		PHY_SetBBReg(pAdapter, pDbgCtx->DbgIoOffset, bMaskDWord, pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_RF:	
		pDbgCtx->DbgIoValue = PHY_QueryRFReg(pAdapter, (u1Byte)pDbgCtx->DbgRfPath, pDbgCtx->DbgIoOffset, bRFRegOffsetMask);
		break;

	case DBG_WRITE_RF:
		PHY_SetRFReg(pAdapter,  (u1Byte)pDbgCtx->DbgRfPath, pDbgCtx->DbgIoOffset, bRFRegOffsetMask, pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_EEPROM_1BYTE:
		pDbgCtx->DbgIoValue = ReadEEprom( pAdapter, (u2Byte)(pDbgCtx->DbgIoOffset>>1) );
		if(pDbgCtx->DbgIoOffset % 2 == 1)
		{
			pDbgCtx->DbgIoValue >>= 8;
		}
		pDbgCtx->DbgIoValue &= 0xff;
		break;

	case DBG_READ_EEPROM_2BYTE:
		pDbgCtx->DbgIoValue = ReadEEprom( pAdapter, (u2Byte)(pDbgCtx->DbgIoOffset>>1) );
		pDbgCtx->DbgIoValue &= 0xffff;
		break;

	case DBG_WRITE_EEPROM_1BYTE:
		{
			USHORT usTmp = 0;

			usTmp = ReadEEprom( pAdapter, (u2Byte)(pDbgCtx->DbgIoOffset>>1) );
			if(pDbgCtx->DbgIoOffset % 2 == 1)
			{
				usTmp &= 0x00ff; // Clear high byte of the word.
				usTmp |= (pDbgCtx->DbgIoValue << 8);
			}
			else
			{
				usTmp &= 0xff00; // Clear low byte of the word.
				usTmp |= (pDbgCtx->DbgIoValue);
			}
			WriteEEprom(pAdapter, (u1Byte)(pDbgCtx->DbgIoOffset>>1), usTmp);
		}
		break;

	case DBG_WRITE_EEPROM_2BYTE:
		WriteEEprom(pAdapter, (u1Byte)(pDbgCtx->DbgIoOffset>>1), (u2Byte)(pDbgCtx->DbgIoValue&0xffff));
		break;

#if 0
	case DBG_OUT_CMD:
		{
			u4Byte	BufferLengthRead;
			u1Byte	i=0;
			
			PlatformUsbSyncVendorRequest(
					pAdapter, 
					TRUE, 	// bWrite
					0x05, 	// bReq
					0, 		// wValue
					0x8051, // wIndex
					(pu1Byte)&(pDbgCtx->DbgIoBuf),  // pBuffer
					pDbgCtx->DbgIoValue,   // BufferLength
					&BufferLengthRead          // pBufferLengthRead
					);
		}
		break;
#endif

	case DBG_WRITE_EFUSE_1BYTE:
		EFUSE_ShadowWrite(pAdapter, 1, (u2Byte)pDbgCtx->DbgIoOffset, (UINT32)pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_EFUSE_1BYTE:
		EFUSE_MaskedShadowRead(pAdapter, 1, (u2Byte)(pDbgCtx->DbgIoOffset), (UINT32 *)&pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_EFUSE_2BYTE:
		EFUSE_MaskedShadowRead(pAdapter, 2, (u2Byte)(pDbgCtx->DbgIoOffset), (UINT32 *)&pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_EFUSE_4BYTE:
		EFUSE_MaskedShadowRead(pAdapter, 4, (u2Byte)(pDbgCtx->DbgIoOffset), (UINT32 *)&pDbgCtx->DbgIoValue);
		break;
		
	case DBG_UPDATE_EFUSE:
		pDbgCtx->DbgIoValue = EFUSE_ShadowUpdate(pAdapter, FALSE);	
		break;	

	case DBG_WRITE_BT_EFUSE_1BYTE:
		EFUSE_ShadowWriteBT(pAdapter, 1, (u2Byte)pDbgCtx->DbgIoOffset, (UINT32)pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_BT_EFUSE_1BYTE:
		EFUSE_ShadowReadBT(pAdapter, 1, (u2Byte)(pDbgCtx->DbgIoOffset), (UINT32 *)&pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_BT_EFUSE_2BYTE:
		EFUSE_ShadowReadBT(pAdapter, 2, (u2Byte)(pDbgCtx->DbgIoOffset), (UINT32 *)&pDbgCtx->DbgIoValue);
		break;

	case DBG_READ_BT_EFUSE_4BYTE:
		EFUSE_ShadowReadBT(pAdapter, 4, (u2Byte)(pDbgCtx->DbgIoOffset), (UINT32 *)&pDbgCtx->DbgIoValue);
		break;
		
	case DBG_UPDATE_BT_EFUSE:
		pDbgCtx->DbgIoValue = EFUSE_ShadowUpdateBT(pAdapter, FALSE);
		break;	

	case DBG_SWITCH_ANTENNA:
		pAdapter->HalFunc.SetTxAntennaHandler(pAdapter, (u1Byte)pDbgCtx->DbgIoValue);
		break;

	case DBG_SET_TXPWR_FOR_ALL_RATE:
		HAL_SetTxPowerForAllRate(pAdapter, pDbgCtx->DbgIoValue);
		break;
		
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
//	Description:
//		Initialize debugging realted information or resource.
//		It should be called in context of MiniportInitialize().
//
//-----------------------------------------------------------------------------
VOID
NDBG_Init(
	IN	PADAPTER pAdapter
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(pAdapter->ndisDbgCtx);

	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], NDBG_Init()\n"));

	pDbgCtx->bDbgDrvUnload = FALSE;
	
	PlatformInitializeEvent(&(pDbgCtx->DbgWorkItemEvent));
	PlatformInitializeEvent(&pDbgCtx->dbgH2cRspEvent);
	PlatformInitializeEvent(&pDbgCtx->dbgBtC2hEvent);
	PlatformInitializeEvent(&pDbgCtx->dbgBtCoexEvent);
	PlatformInitializeSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	PlatformInitializeWorkItem(
		pAdapter,
		&(pDbgCtx->DbgWorkItem), 
		(RT_WORKITEM_CALL_BACK)DbgWorkItemCallback,
		(PVOID)pAdapter,
		"DbgWorkItem");

	pDbgCtx->bDbgWorkItemInProgress = FALSE;
	pDbgCtx->CurrDbgAct = NULL;
}

//-----------------------------------------------------------------------------
//	Description:
//		DeInitialize for debugging realted resource.
//		It should be called in context of MiniportHalt().
//
//-----------------------------------------------------------------------------
VOID
NDBG_Halt(
	IN	PADAPTER pAdapter
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], NDBG_Halt()\n"));

	pDbgCtx->bDbgDrvUnload = TRUE;

	PlatformFreeWorkItem( &(pDbgCtx->DbgWorkItem) );

	if(pDbgCtx->bDbgWorkItemInProgress)
	{
		PlatformWaitEvent(&(pDbgCtx->DbgWorkItemEvent), 2000);
	}
	
	PlatformFreeSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
}

//-----------------------------------------------------------------------------
//	Description:
//		Read MAC register.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgReadMacReg(
	IN	PADAPTER			Adapter,
	IN	ULONG				ulRegOffset,
	IN	ULONG				ulRegDataWidth
	)
{
	PADAPTER				pAdapter;
	PRT_NDIS_DBG_CONTEXT	pDbgCtx;
	BOOLEAN bResult = TRUE;
	ULONG	ulIoType;

	if(Adapter->bSWInitReady)
		pAdapter=GetDefaultAdapter(Adapter);
	else
		pAdapter=Adapter;
	
	pDbgCtx = &(pAdapter->ndisDbgCtx);

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_READ_MAC_1BYTE;
		break;

	case 2:
		ulIoType = DBG_READ_MAC_2BYTE;
		break;

	case 4:
		ulIoType = DBG_READ_MAC_4BYTE;
		break;

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgReadMacReg(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = 0xffffffff;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Write MAC register.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgWriteMacReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN bResult = TRUE;
	ULONG	ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_WRITE_MAC_1BYTE;
		break;

	case 2:
		ulIoType = DBG_WRITE_MAC_2BYTE;
		break;

	case 4:
		ulIoType = DBG_WRITE_MAC_4BYTE;
		break;

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgWriteMacReg(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = ulRegValue;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Read BB register.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgReadBbReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulBeOFDM
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN bResult = TRUE;
	ULONG	ulIoType;

	if(ulBeOFDM == 1)
	{
		ulIoType = DBG_READ_BB_OFDM;
	}
	else
	{
		ulIoType = DBG_READ_BB_CCK;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgReadBbReg(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = 0xffffffff;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Write BB register.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgWriteBbReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulBeOFDM,
	IN	ULONG		ulRegValue
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN bResult = TRUE;
	ULONG	ulIoType;

	if(ulBeOFDM == 1)
	{
		ulIoType = DBG_WRITE_BB_OFDM;
	}
	else
	{
		ulIoType = DBG_WRITE_BB_CCK;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgWriteBbReg(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = ulRegValue;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Read RF register.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgReadRfReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN bResult = TRUE;
	ULONG	ulIoType;
	ULONG	RF_PATH=0, INulRegOffset=0;

	ulIoType = DBG_READ_RF; 
	RF_PATH = (ulRegDataWidth >> 4); // Get RF path
	INulRegOffset = ulRegOffset & 0x0fff;

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgReadRfReg(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = 0xffffffff;
			pDbgCtx->DbgRfPath = RF_PATH;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;
			
			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Write RF register.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgWriteRfReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN bResult = TRUE;
	ULONG	ulIoType, RF_PATH, INulRegOffset, INulRegValue;

	ulIoType = DBG_WRITE_RF;
	RF_PATH  = (ulRegOffset >> 12);
	INulRegOffset = ulRegOffset & 0xfff;	//[11:0]
	INulRegValue = ulRegValue & bRFRegOffsetMask;


	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgWriteRfReg(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = INulRegOffset;
			pDbgCtx->DbgIoValue = INulRegValue;
			pDbgCtx->DbgRfPath = RF_PATH;


			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Read EEPROM for OID_RT_PRO_READ_EEPROM.	
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgReadEeprom(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_READ_EEPROM_1BYTE;
		break;

	case 2:
		ulIoType = DBG_READ_EEPROM_2BYTE;
		break;

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE,
				("DbgWorkItem is in progress!, skip DbgReadEeprom(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = 0xffffffff;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}


//-----------------------------------------------------------------------------
//	Description:
//		Write EEPROM for OID_RT_PRO_WRITE_EEPROM.
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgWriteEeprom(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_WRITE_EEPROM_1BYTE;
		break;

	case 2:
		ulIoType = DBG_WRITE_EEPROM_2BYTE;
		break;

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip Pro8187IoCallback(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = ulRegValue;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}


//-----------------------------------------------------------------------------
//	Description:
//		87S-USB Out Command.
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgOutCmd(
	IN	PADAPTER		pAdapter,
	IN	PUCHAR			ulOutCmd,
	IN	ULONG			ulOutCmdWidth
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;
	ULONG i=0;

	ulIoType = DBG_OUT_CMD; 

	// Perform IO via workitem.
	PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	if(pDbgCtx->bDbgWorkItemInProgress)
	{
		RT_ASSERT(FALSE, 
			("DbgWorkItem is in progress!, skip Pro8187IoCallback(), IoType: %d.\n", ulIoType));
		bResult = FALSE;
	}
	else
	{
		PlatformZeroMemory(&pDbgCtx->DbgIoBuf, 64);
		pDbgCtx->DbgIoOffset = 0;
		
		pDbgCtx->DbgIoValue = ulOutCmdWidth; //use DbgIoValue to indicate OutCmd length
		PlatformMoveMemory(&pDbgCtx->DbgIoBuf, ulOutCmd, ulOutCmdWidth);

		pDbgCtx->bDbgWorkItemInProgress = TRUE;
		pDbgCtx->DbgActType = ulIoType;
		pDbgCtx->CurrDbgAct = DbgIoCallback;
		PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
	}
	PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);

	return bResult;
}

//-----------------------------------------------------------------------------
//	Description:
//		Read EEPROM for OID_RT_PRO_READ_EFUSE.	
//	Added by Roger, 2008.11.10.
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgReadEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_READ_EFUSE_1BYTE;
		break;

	case 2:
		ulIoType = DBG_READ_EFUSE_2BYTE;
		break;

	case 4:
		ulIoType = DBG_READ_EFUSE_4BYTE;
		break;

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE,
				("DbgWorkItem is in progress!, skip DbgReadEeprom(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = 0xffffffff;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}


//-----------------------------------------------------------------------------
//	Description:
//		Write EEPROM for OID_RT_PRO_WRITE_EFUSE.
//	Added by Roger, 2008.11.10.
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgWriteEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_WRITE_EFUSE_1BYTE;
		break;	

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip Pro8187IoCallback(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = ulRegValue;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}
//-----------------------------------------------------------------------------
//	Description:
//		Write EEPROM for OID_RT_PRO_WRITE_EFUSE.
//	Added by Roger, 2008.11.10.
//		
//-----------------------------------------------------------------------------
BOOLEAN
DbgUpdateEFuse(
	IN	PADAPTER	pAdapter	
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;
	
	ulIoType = DBG_UPDATE_EFUSE;	

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip Pro8187IoCallback(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;
			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

BOOLEAN
DbgReadBTEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_READ_BT_EFUSE_1BYTE;
		break;

	case 2:
		ulIoType = DBG_READ_BT_EFUSE_2BYTE;
		break;

	case 4:
		ulIoType = DBG_READ_BT_EFUSE_4BYTE;
		break;

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE,
				("DbgWorkItem is in progress!, skip DbgReadBTEFuse(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = 0xffffffff;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

BOOLEAN
DbgWriteBTEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	switch(ulRegDataWidth)
	{
	case 1:
		ulIoType = DBG_WRITE_BT_EFUSE_1BYTE;
		break;	

	default:
		bResult = FALSE;
		break;
	}

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgWriteBTEFuse(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoOffset = ulRegOffset;
			pDbgCtx->DbgIoValue = ulRegValue;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

BOOLEAN
DbgUpdateBTEFuse(
	IN	PADAPTER	pAdapter	
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;
	
	ulIoType = DBG_UPDATE_BT_EFUSE;	

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip DbgUpdateBTEFuse(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;
			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

BOOLEAN
DbgSetTxAntenna(
	IN	PADAPTER	pAdapter,
	IN	u1Byte		selectedAntenna
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;

	ulIoType = DBG_SWITCH_ANTENNA;	

	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE, 
				("DbgWorkItem is in progress!, skip Pro8187IoCallback(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->DbgIoValue = selectedAntenna;
			pDbgCtx->CurrDbgAct = DbgIoCallback;
			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;	
}

BOOLEAN
DbgSetTxPowerForAllRate(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulTxPowerData
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	BOOLEAN 		bResult = TRUE;
	ULONG			ulIoType;


	ulIoType = DBG_SET_TXPWR_FOR_ALL_RATE;


	if(bResult == TRUE)
	{
		// Perform IO via workitem.
		PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
		if(pDbgCtx->bDbgWorkItemInProgress)
		{
			RT_ASSERT(FALSE,
				("DbgWorkItem is in progress!, skip DbgSetTxPowerForAllRate(), IoType: %d.\n", ulIoType));
			bResult = FALSE;
		}
		else
		{
			pDbgCtx->DbgIoValue = ulTxPowerData;

			pDbgCtx->bDbgWorkItemInProgress = TRUE;
			pDbgCtx->DbgActType = ulIoType;
			pDbgCtx->CurrDbgAct = DbgIoCallback;

			PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem) );
		}
		PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);
	}

	return bResult;
}

static BOOLEAN
ndbg_BtControlStart(
	IN	PADAPTER	pAdapter
	)
{
	BOOLEAN 		bResult = TRUE;
	PRT_NDIS_DBG_CONTEXT	pDbgCtx = &(pAdapter->ndisDbgCtx);
	ULONG	ulIoType = DBG_BT_CONTROL;
	
	PlatformAcquireSpinLock(pAdapter, RT_DBG_SPIN_LOCK);

	if(pDbgCtx->bDbgWorkItemInProgress)
	{
		RT_ASSERT(FALSE, 
		("DbgWorkItem is in progress!, skip Pro8187IoCallback(), IoType: %d.\n", ulIoType));
		bResult = FALSE;			
	}
	else
	{
		pDbgCtx->bDbgWorkItemInProgress = TRUE;
		pDbgCtx->DbgActType = ulIoType;
		pDbgCtx->CurrDbgAct = DbgIoCallback;
		PlatformScheduleWorkItem( &(pDbgCtx->DbgWorkItem));
    }

	PlatformReleaseSpinLock(pAdapter, RT_DBG_SPIN_LOCK);

	return bResult;
}

NDIS_STATUS
NDBG_BtControl(
	IN	PADAPTER		pAdapter,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER			pDefAdapter=GetDefaultAdapter(pAdapter);
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(pDefAdapter->ndisDbgCtx);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	if(InformationBufferLength < sizeof(ULONG))
	{
		RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! insufficient InformationBufferLength!!\n"));
		Status = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(ULONG);
	}
	else
	{
		RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], InformationBuffer: \n"), InformationBuffer, InformationBufferLength);
		if(InformationBufferLength <= 100)
		{
			PlatformMoveMemory(&pDbgCtx->btInBuf[0], InformationBuffer, InformationBufferLength);
			RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], pDbgCtx->btInBuf: \n"), &pDbgCtx->btInBuf[0], InformationBufferLength);
			if(!ndbg_BtControlStart(pDefAdapter))
			{
				RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! DBG workitem is under progress!!\n"));
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		else
		{
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Error!! InformationBufferLength should < 100!!\n"));
			Status = NDIS_STATUS_INVALID_LENGTH;
		}
	}
	
	return Status;
}

BOOLEAN
NDBG_GetBtFwVersion(
	IN	PADAPTER		pAdapter,
	IN	pu2Byte			pBtRealFwVer,
	IN	pu1Byte			pBtFwVer
	)
{
	PBT_EXT_C2H			pExtC2h=(PBT_EXT_C2H)&GLCurBtC2hRsp[0];
	pu1Byte				pBtRspContent=(pu1Byte)&pExtC2h->buf[0];
	BT_CTRL_STATUS		retStatus;
	u1Byte				h2cParaBuf[6] ={0};
	u1Byte				h2cParaLen=0;
	u1Byte				btOpcode, btFwVer;
	u1Byte				btOpcodeVer=0;
	u2Byte				btRealFwVer=0, realStatus=0;
	pu2Byte				pu2Tmp;

	// Get BT FW version
	// fill h2c parameters
	btOpcode = BT_LO_OP_GET_BT_VERSION;
	// execute h2c and check respond c2h from bt fw is correct or not
	retStatus = ndbg_BtFwOpCodeProcess(pAdapter, btOpcode, btOpcodeVer, &h2cParaBuf[0], h2cParaLen);
	// ckeck bt return status.
	if(BT_STATUS_BT_OP_SUCCESS != retStatus)
	{
		realStatus = ((btOpcode<<8)|retStatus);
		*pBtRealFwVer = 0xffff;
		*pBtFwVer = 0xff;
		RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Query BT fw version fail, status code=0x%x!!!\n", realStatus));
		return FALSE;
	}
	else
	{	
		pu2Tmp = (pu2Byte)&pBtRspContent[0];
		btRealFwVer = *pu2Tmp;
		btFwVer = pExtC2h->buf[2];
		*pBtRealFwVer = btRealFwVer;
		*pBtFwVer = btFwVer;
		RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], Query BT fw version successful!! btRealFwVer=0x%x, btFwVer=0x%x\n", btRealFwVer, btFwVer));
		return TRUE;
	}
}

VOID
NDBG_FwC2hBtControl(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		tmpBuf,
	IN	u1Byte		length
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(Adapter->ndisDbgCtx);
	PBT_EXT_C2H		pExtC2h=(PBT_EXT_C2H)tmpBuf;

	RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], NDBG_FwC2hBtControl(), hex: \n"), 
						tmpBuf, length);

	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pExtC2h->extendId=0x%x\n", pExtC2h->extendId));
	switch(pExtC2h->extendId)
	{
		case EXT_C2H_WIFI_FW_ACTIVE_RSP:
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], EXT_C2H_WIFI_FW_ACTIVE_RSP\n"));
			RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], pExtC2h->buf hex: \n"), 
				&pExtC2h->buf[0], (length-3));
			PlatformSetEvent(&pDbgCtx->dbgH2cRspEvent);
			break;
		case EXT_C2H_TRIG_BY_BT_FW:
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], EXT_C2H_TRIG_BY_BT_FW\n"));
			PlatformMoveMemory(&GLCurBtC2hRsp[0], tmpBuf, length);
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pExtC2h->statusCode=0x%x\n", pExtC2h->statusCode));
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pExtC2h->retLen=0x%x\n", pExtC2h->retLen));
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pExtC2h->opCodeVer=0x%x\n", pExtC2h->opCodeVer));
			RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pExtC2h->reqNum=0x%x\n", pExtC2h->reqNum));
			RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], pExtC2h->buf hex: \n"), 
				&pExtC2h->buf[0], (length-3));
			PlatformSetEvent(&pDbgCtx->dbgBtC2hEvent);
			break;
		default:
			break;
	}
}

VOID
NDBG_StackBtCoexNotify(
	IN	PADAPTER	Adapter,
	IN	u1Byte		stackOpCode,
	IN	u1Byte		stackOpStatus,
	IN	pu1Byte		tmpBuf,
	IN	u1Byte		length
	)
{
	PRT_NDIS_DBG_CONTEXT	pDbgCtx=&(Adapter->ndisDbgCtx);
	PBT_STACK_COEX_INFO	pStackRsp=(PBT_STACK_COEX_INFO)&GLCurBtStackRsp[0];

	pStackRsp->opCode = stackOpCode;
	pStackRsp->opStatus = stackOpStatus;
	pStackRsp->bufLen = length;
	if(length)
	{
		PlatformMoveMemory(&pStackRsp->buf[0], tmpBuf, length);
		RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], NDBG_StackBtCoexNotify(), hex: \n"), 
			tmpBuf, length);
	}
	RT_DISP(FBT, BT_DBG_STATE, ("[BTDBG], pStackRsp->opCode=0x%x, pStackRsp->opStatus=0x%x\n", 
		pStackRsp->opCode, pStackRsp->opStatus));
	PlatformSetEvent(&pDbgCtx->dbgBtCoexEvent);
}


