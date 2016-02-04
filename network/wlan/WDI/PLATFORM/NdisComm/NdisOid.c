#include "Mp_Precomp.h"

BOOLEAN
oids_RTKDbgInvalidLength(
	IN	ULONG				InformationBufferLength,
	IN	PRTK_DBG_CTRL_OIDS	pRTKDbg,
	OUT	PULONG				BytesNeeded
	)
{
	BOOLEAN		bRet=FALSE;
	ULONG		totalLen=0;

	totalLen = sizeof(RTK_DBG_CTRL_OIDS)-sizeof(ULONG)+pRTKDbg->ctrlDataLen;
	if(InformationBufferLength != totalLen)
	{
		bRet = TRUE;
		*BytesNeeded = totalLen;
		return bRet;
	}
	return bRet;
}





NDIS_STATUS
OIDQ_RTKReadReg(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesNeeded,
	IN	PULONG		pulInfo
	)
{
	NDIS_STATUS		Status=NDIS_STATUS_SUCCESS;
	ULONG			ulRegOffset;
	ULONG			ulRegDataWidth;
	ULONG			ulInfo = 0;

	// Verify input paramter.
	if(InformationBufferLength < sizeof(ULONG)*2)
	{
		Status = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(ULONG)*2;
		return Status;
	}
	// Get offset and data width.
	ulRegOffset = *((ULONG*)InformationBuffer);
	ulRegDataWidth = *((ULONG*)InformationBuffer+1);
	switch(ulRegDataWidth)
	{
	case 1:
		if( IS_BB_REG_OFFSET_92S(ulRegOffset) )
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			ulInfo = PHY_QueryBBReg(Adapter, ulRegOffset, bMaskDWord);
			ulInfo &= (bMaskByte0<<(8*mod_val));
			ulInfo = (ulInfo>>(8*mod_val));
		}
		else
			ulInfo = PlatformEFIORead1Byte(Adapter, ulRegOffset);
		break;
	case 2:
		if( IS_BB_REG_OFFSET_92S(ulRegOffset) )
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			ulInfo = PHY_QueryBBReg(Adapter, ulRegOffset, bMaskDWord);
			ulInfo &= (bMaskLWord<<(8*mod_val));
			ulInfo = (ulInfo>>(8*mod_val));
		}
		else
			ulInfo = PlatformEFIORead2Byte(Adapter, ulRegOffset);
		break;
	case 4:
		if( IS_BB_REG_OFFSET_92S(ulRegOffset) )
			ulInfo = PHY_QueryBBReg(Adapter, ulRegOffset, bMaskDWord);
		else
			ulInfo = PlatformEFIORead4Byte(Adapter, ulRegOffset);
		break;
	default:
		Status = NDIS_STATUS_INVALID_LENGTH;
		return Status;
	}

	*pulInfo = ulInfo;

	return Status;
}

NDIS_STATUS
OIDQ_RTKReadRegSIC(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesNeeded,
	IN	PULONG		pulInfo
	)
{
	NDIS_STATUS		Status=NDIS_STATUS_SUCCESS;
	ULONG			ulRegOffset;
	ULONG			ulRegDataWidth;
	ULONG			ulInfo = 0;

	// Verify input paramter.
	if(InformationBufferLength < sizeof(ULONG)*2)
	{
		Status = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(ULONG)*2;
		return Status;
	}
	// Get offset and data width.
	ulRegOffset = *((ULONG*)InformationBuffer);
	ulRegDataWidth = *((ULONG*)InformationBuffer+1);
	switch(ulRegDataWidth)
	{
	case 1:
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;

			ulInfo = SIC_QueryBBReg(Adapter, ulRegOffset, bMaskDWord);
			ulInfo &= (bMaskByte0<<(8*mod_val));
			ulInfo = (ulInfo>>(8*mod_val));
		}
		break;
	case 2:
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			ulInfo = SIC_QueryBBReg(Adapter, ulRegOffset, bMaskDWord);
			ulInfo &= (bMaskLWord<<(8*mod_val));
			ulInfo = (ulInfo>>(8*mod_val));
		}
		break;
	case 4:
		ulInfo = SIC_QueryBBReg(Adapter, ulRegOffset, bMaskDWord);
		break;
	default:
		Status = NDIS_STATUS_INVALID_LENGTH;
		return Status;
	}

	*pulInfo = ulInfo;

	return Status;
}

NDIS_STATUS
OIDS_RTKWriteReg(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	)
{
	NDIS_STATUS		Status=NDIS_STATUS_SUCCESS;
	ULONG			ulRegOffset;
	ULONG			ulRegDataWidth;
	ULONG			ulRegValue;

	// Verify input paramter.
	if(InformationBufferLength < sizeof(ULONG)*3)
	{
		Status = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(ULONG)*3;				
		return Status;
	}
	// Get offset, data width, and value to write.
	ulRegOffset = *((ULONG*)InformationBuffer);
	ulRegDataWidth = *((ULONG*)InformationBuffer+1);
	ulRegValue = *((ULONG*)InformationBuffer+2);

	switch(ulRegDataWidth)
	{
	case 1:
		if( IS_BB_REG_OFFSET_92S(ulRegOffset) )
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			PHY_SetBBReg(Adapter, ulRegOffset, (u4Byte)(bMaskByte0<<(8*mod_val)), ulRegValue);
		}
		else
			PlatformEFIOWrite1Byte(Adapter, ulRegOffset, (u1Byte)(ulRegValue));
		break;
	case 2:
		if( IS_BB_REG_OFFSET_92S(ulRegOffset) )
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			PHY_SetBBReg(Adapter, ulRegOffset, (bMaskLWord<<(8*mod_val)), ulRegValue);
		}
		else
			PlatformEFIOWrite2Byte(Adapter, ulRegOffset, (u2Byte)(ulRegValue));
		break;
	case 4:
		if( IS_BB_REG_OFFSET_92S(ulRegOffset) )
			PHY_SetBBReg(Adapter, ulRegOffset, bMaskDWord, ulRegValue);
		else
			PlatformEFIOWrite4Byte(Adapter, ulRegOffset, (u4Byte)(ulRegValue));
		break;
	default:
		Status = NDIS_STATUS_INVALID_LENGTH;
		return Status;
	}

	return Status;
}

NDIS_STATUS
OIDS_RTKWriteRegSIC(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	)
{
	NDIS_STATUS		Status=NDIS_STATUS_SUCCESS;
	ULONG			ulRegOffset;
	ULONG			ulRegDataWidth;
	ULONG			ulRegValue;

	// Verify input paramter.
	if(InformationBufferLength < sizeof(ULONG)*3)
	{
		Status = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(ULONG)*3;				
		return Status;
	}
	// Get offset, data width, and value to write.
	ulRegOffset = *((ULONG*)InformationBuffer);
	ulRegDataWidth = *((ULONG*)InformationBuffer+1);
	ulRegValue = *((ULONG*)InformationBuffer+2);

	switch(ulRegDataWidth)
	{
	case 1:
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			SIC_SetBBReg(Adapter, ulRegOffset, (u4Byte)(bMaskByte0<<(8*mod_val)), ulRegValue);
		}
		break;
	case 2:
		{
			ULONG mod_val=0;
			mod_val = ulRegOffset%4;
			if(mod_val)
				ulRegOffset -= mod_val;
			SIC_SetBBReg(Adapter, ulRegOffset, (bMaskLWord<<(8*mod_val)), ulRegValue);
		}
		break;
	case 4:
		SIC_SetBBReg(Adapter, ulRegOffset, bMaskDWord, ulRegValue);
		break;
	default:
		Status = NDIS_STATUS_INVALID_LENGTH;
		return Status;
	}

	return Status;
}


NDIS_STATUS
OIDS_RTKDbgControl(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded	
	)
{
	NDIS_STATUS			Status=NDIS_STATUS_SUCCESS;
	PRTK_DBG_CTRL_OIDS	pRTKDbg;

	pRTKDbg = (PRTK_DBG_CTRL_OIDS)InformationBuffer;

	RT_DISP(FDBG_CTRL, DBG_CTRL_TRACE, ("[RTKDBG], Type=0x%x, Len=0x%x(%d)\n", 
		pRTKDbg->ctrlType, pRTKDbg->ctrlDataLen, pRTKDbg->ctrlDataLen));

	if(oids_RTKDbgInvalidLength(InformationBufferLength, pRTKDbg, BytesNeeded))
	{
		RT_DISP(FDBG_CTRL, DBG_CTRL_TRACE, ("[RTKDBG], Invalid Length, return!!\n"));
		Status = NDIS_STATUS_INVALID_LENGTH;				
		return Status;
	}
	else
	{
		RT_DISP_DATA(FDBG_CTRL, DBG_CTRL_TRACE, ("[RTKDBG], Hex Data :"), &pRTKDbg->CtrlData, pRTKDbg->ctrlDataLen)
	}

	switch(pRTKDbg->ctrlType)
	{
		case RTK_DBG_OIDS_BT_PROFILE:
			RT_DISP(FDBG_CTRL, DBG_CTRL_TRACE, ("[RTKDBG], RTK_DBG_OIDS_BT_PROFILE\n"));
			break;
		default:
			break;
	}

	return Status;
}