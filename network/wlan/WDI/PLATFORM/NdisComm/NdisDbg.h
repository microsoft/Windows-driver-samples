#ifndef __INC_NDIS_DBG_H
#define __INC_NDIS_DBG_H


#define	BT_H2C_MAX_RETRY								1
#define	BT_MAX_C2H_LEN									20

typedef enum _RTL_EXT_C2H_EVT
{
	EXT_C2H_WIFI_FW_ACTIVE_RSP = 0,
	EXT_C2H_TRIG_BY_BT_FW = 1,
	MAX_EXT_C2HEVENT
}RTL_EXT_C2H_EVT;


// OP codes definition between the user layer and driver
typedef enum _BT_CTRL_OPCODE_UPPER{
	BT_UP_OP_BT_READY										= 0x00, 
	BT_UP_OP_BT_SET_MODE									= 0x01,
	BT_UP_OP_BT_SET_TX_RX_PARAMETER							= 0x02,
	BT_UP_OP_BT_SET_GENERAL									= 0x03,
	BT_UP_OP_BT_GET_GENERAL									= 0x04,
	BT_UP_OP_BT_TEST_CTRL									= 0x05,
	BT_UP_OP_MAX
}BT_CTRL_OPCODE_UPPER,*PBT_CTRL_OPCODE_UPPER;

typedef enum _BT_SET_GENERAL{
	BT_GSET_REG												= 0x00, 
	BT_GSET_RESET											= 0x01, 
	BT_GSET_TARGET_BD_ADDR									= 0x02, 
	BT_GSET_TX_PWR_FINETUNE									= 0x03,
	BT_GSET_TRACKING_INTERVAL								= 0x04,
	BT_GSET_THERMAL_METER									= 0x05,
	BT_GSET_CFO_TRACKING									= 0x06,
	BT_GSET_ANT_DETECTION									= 0x07,
	BT_GSET_MAX
}BT_SET_GENERAL,*PBT_SET_GENERAL;

typedef enum _BT_GET_GENERAL{
	BT_GGET_REG												= 0x00, 
	BT_GGET_STATUS											= 0x01,
	BT_GGET_REPORT											= 0x02,
	BT_GGET_AFH_MAP											= 0x03,
	BT_GGET_AFH_STATUS										= 0x04,
	BT_GGET_IQK_FLOW										= 0x05,
	BT_GGET_IQK_RESULT										= 0x06,
	BT_GGET_MAX
}BT_GET_GENERAL,*PBT_GET_GENERAL;

// definition for BT_UP_OP_BT_SET_GENERAL
typedef enum _BT_REG_TYPE{
	BT_REG_RF								= 0,
	BT_REG_MODEM							= 1,
	BT_REG_BLUEWIZE							= 2,
	BT_REG_VENDOR							= 3,
	BT_REG_LE								= 4,
	BT_REG_MAX
}BT_REG_TYPE,*PBT_REG_TYPE;

// definition for BT_LO_OP_GET_AFH_MAP
typedef enum _BT_AFH_MAP_TYPE{
	BT_AFH_MAP_RESULT						= 0,
	BT_AFH_MAP_WIFI_PSD_ONLY				= 1,
	BT_AFH_MAP_WIFI_CH_BW_ONLY				= 2,
	BT_AFH_MAP_BT_PSD_ONLY					= 3,
	BT_AFH_MAP_HOST_CLASSIFICATION_ONLY		= 4,
	BT_AFH_MAP_MAX
}BT_AFH_MAP_TYPE,*PBT_AFH_MAP_TYPE;

// definition for BT_UP_OP_BT_GET_GENERAL
typedef enum _BT_REPORT_TYPE{
	BT_REPORT_RX_PACKET_CNT					= 0,
	BT_REPORT_RX_ERROR_BITS					= 1,
	BT_REPORT_RSSI							= 2,
	BT_REPORT_CFO_HDR_QUALITY				= 3,
	BT_REPORT_CONNECT_TARGET_BD_ADDR		= 4,
	BT_REPORT_MAX
}BT_REPORT_TYPE,*PBT_REPORT_TYPE;


//OP codes definition between driver and bt fw
typedef enum _BT_CTRL_OPCODE_LOWER{
	BT_LO_OP_GET_BT_VERSION										= 0x00, 
	BT_LO_OP_RESET												= 0x01,
	BT_LO_OP_TEST_CTRL											= 0x02,
	BT_LO_OP_SET_BT_MODE										= 0x03,
	BT_LO_OP_SET_CHNL_TX_GAIN									= 0x04,
	BT_LO_OP_SET_PKT_TYPE_LEN									= 0x05,
	BT_LO_OP_SET_PKT_CNT_L_PL_TYPE								= 0x06,
	BT_LO_OP_SET_PKT_CNT_H_PKT_INTV								= 0x07,
	BT_LO_OP_SET_PKT_HEADER										= 0x08,
	BT_LO_OP_SET_WHITENCOEFF									= 0x09,
	BT_LO_OP_SET_BD_ADDR_L										= 0x0a,
	BT_LO_OP_SET_BD_ADDR_H										= 0x0b,
	BT_LO_OP_WRITE_REG_ADDR										= 0x0c,
	BT_LO_OP_WRITE_REG_VALUE									= 0x0d,
	BT_LO_OP_GET_BT_STATUS										= 0x0e,
	BT_LO_OP_GET_BD_ADDR_L										= 0x0f,
	BT_LO_OP_GET_BD_ADDR_H										= 0x10,
	BT_LO_OP_READ_REG											= 0x11,
	BT_LO_OP_SET_TARGET_BD_ADDR_L								= 0x12,
	BT_LO_OP_SET_TARGET_BD_ADDR_H								= 0x13,
	BT_LO_OP_SET_TX_POWER_CALIBRATION							= 0x14,
	BT_LO_OP_GET_RX_PKT_CNT_L									= 0x15,
	BT_LO_OP_GET_RX_PKT_CNT_H									= 0x16,
	BT_LO_OP_GET_RX_ERROR_BITS_L								= 0x17,
	BT_LO_OP_GET_RX_ERROR_BITS_H								= 0x18,
	BT_LO_OP_GET_RSSI											= 0x19,
	BT_LO_OP_GET_CFO_HDR_QUALITY_L								= 0x1a,
	BT_LO_OP_GET_CFO_HDR_QUALITY_H								= 0x1b,
	BT_LO_OP_GET_TARGET_BD_ADDR_L								= 0x1c,
	BT_LO_OP_GET_TARGET_BD_ADDR_H								= 0x1d,
	BT_LO_OP_GET_AFH_MAP_L										= 0x1e,
	BT_LO_OP_GET_AFH_MAP_M										= 0x1f,
	BT_LO_OP_GET_AFH_MAP_H										= 0x20,
	BT_LO_OP_GET_AFH_STATUS										= 0x21,
	BT_LO_OP_SET_TRACKING_INTERVAL								= 0x22,
	BT_LO_OP_SET_THERMAL_METER									= 0x23,
	BT_LO_OP_SET_CFO_TRACKING									= 0x24,
	BT_LO_OP_SET_FW_POLICY_REQ									= 0x25,
	BT_LO_OP_SET_ANT_DETECTION									= 0x26,
	BT_LO_OP_GET_IQK_FLOW										= 0x27,
	BT_LO_OP_GET_IQK_RESULT										= 0x28,
	BT_LO_OP_MAX
}BT_CTRL_OPCODE_LOWER,*PBT_CTRL_OPCODE_LOWER;

//OP codes definition between driver and bt stack
typedef enum _BT_CTRL_OPCODE_STACK{
	BT_STACK_OP_GET_BT_VERSION									= 0x00,
	BT_STACK_OP_IGNORE_WLAN_ACTIVE_CTRL							= 0x01,
	BT_STACK_OP_LNA_CONSTRAIN_CTRL								= 0x02,
	BT_STACK_OP_BT_POWER_DEC_CTRL								= 0x03,
	BT_STACK_OP_BT_PSD_MODE_CTRL								= 0x04,
	BT_STACK_OP_BT_WIFI_BW_CHNL_NOTIFY							= 0x05,
	BT_STACK_OP_QUERY_BT_AFH_MAP								= 0x06,
	BT_STACK_OP_BT_REGISTER_ACCESS								= 0x07,
	BT_STACK_OP_MAX
}BT_CTRL_OPCODE_STACK,*PBT_CTRL_OPCODE_STACK;


// return status definition to the user layer
typedef enum _BT_CTRL_STATUS{
	BT_STATUS_SUCCESS									= 0x00, // Success
	BT_STATUS_BT_OP_SUCCESS								= 0x01,	// bt fw op execution success
	BT_STATUS_H2C_SUCCESS								= 0x02,	// H2c success
	BT_STATUS_H2C_TIMTOUT								= 0x03, // H2c timeout
	BT_STATUS_H2C_BT_NO_RSP								= 0x04, // H2c sent, bt no rsp
	BT_STATUS_C2H_SUCCESS								= 0x05, // C2h success
	BT_STATUS_C2H_REQNUM_MISMATCH						= 0x06,	// bt fw wrong rsp
	BT_STATUS_OPCODE_U_VERSION_MISMATCH					= 0x07,	// Upper layer OP code version mismatch.
	BT_STATUS_OPCODE_L_VERSION_MISMATCH					= 0x08,	// Lower layer OP code version mismatch.
	BT_STATUS_UNKNOWN_OPCODE_U							= 0x09, // Unknown Upper layer OP code
	BT_STATUS_UNKNOWN_OPCODE_L							= 0x0a, // Unknown Lower layer OP code
	BT_STATUS_PARAMETER_FORMAT_ERROR_U					= 0x0b,	// Wrong parameters sent by upper layer.
	BT_STATUS_PARAMETER_FORMAT_ERROR_L					= 0x0c,	// bt fw parameter format is not consistency
	BT_STATUS_PARAMETER_OUT_OF_RANGE_U					= 0x0d,	// uppery layer parameter value is out of range
	BT_STATUS_PARAMETER_OUT_OF_RANGE_L					= 0x0e,	// bt fw parameter value is out of range
	BT_STATUS_UNKNOWN_STATUS_L							= 0x0f,	// bt returned an defined status code
	BT_STATUS_UNKNOWN_STATUS_H							= 0x10,	// driver need to do error handle or not handle-well.
	BT_STATUS_WRONG_LEVEL								= 0x11, // should be under passive level
	BT_STATUS_NOT_IMPLEMENT 							= 0x12,	// op code not implemented yet
	BT_STATUS_BT_STACK_OP_SUCCESS						= 0x13,	// bt stack op execution success
	BT_STATUS_BT_STACK_NOT_SUPPORT						= 0x14, 	// stack version not support this.
	BT_STATUS_BT_STACK_SEND_HCI_EVENT_FAIL				= 0x15, 	// send hci event fail
	BT_STATUS_BT_STACK_NOT_BIND							= 0x16,	// stack not bind wifi driver
	BT_STATUS_BT_STACK_NO_RSP							= 0x17,	// stack doesn't have any rsp.
	BT_STATUS_MAX
}BT_CTRL_STATUS,*PBT_CTRL_STATUS;

typedef enum _BT_OPCODE_STATUS{
	BT_OP_STATUS_SUCCESS									= 0x00, // Success
	BT_OP_STATUS_VERSION_MISMATCH							= 0x01,	
	BT_OP_STATUS_UNKNOWN_OPCODE								= 0x02,
	BT_OP_STATUS_ERROR_PARAMETER							= 0x03,
	BT_OP_STATUS_MAX
}BT_OPCODE_STATUS,*PBT_OPCODE_STATUS;


typedef struct _BT_REQ_CMD{
    UCHAR       opCodeVer;
    UCHAR       OpCode;
    USHORT      paraLength;
    UCHAR       pParamStart[1];
} BT_REQ_CMD, *PBT_REQ_CMD;

typedef struct _BT_RSP_CMD{
    USHORT      status;
    USHORT      paraLength;
    UCHAR       pParamStart[1];
} BT_RSP_CMD, *PBT_RSP_CMD;

typedef struct _BT_H2C{
	u1Byte	opCodeVer:4;
	u1Byte	reqNum:4;
	u1Byte	opCode;
	u1Byte	buf[1];
}BT_H2C, *PBT_H2C;

typedef struct _BT_EXT_C2H{
	u1Byte	extendId;
	u1Byte	statusCode:4;
	u1Byte	retLen:4;
	u1Byte	opCodeVer:4;
	u1Byte	reqNum:4;
	u1Byte	buf[1];
}BT_EXT_C2H, *PBT_EXT_C2H;

typedef struct _BT_STACK_COEX_INFO{
	u1Byte	opCode;
	u1Byte	opStatus;
	u1Byte	bufLen;
	u1Byte	buf[1];
}BT_STACK_COEX_INFO, *PBT_STACK_COEX_INFO;

typedef struct _HAL_REQ_CMD{
    UCHAR       OpCodeVer;
    UCHAR       OpCode;
    USHORT      ParaLength;
    UCHAR       pParamStart[1];
} HAL_REQ_CMD, *PHAL_REQ_CMD;

typedef struct _HAL_RSP_CMD{
    USHORT		Status;
    USHORT      ParaLength;
    UCHAR       pParamStart[1];
} HAL_RSP_CMD, *PHAL_RSP_CMD;

//
// Type of action going on in DbgWorkItem.
//
#define DBG_NOOP					0
#define DBG_READ_MAC_1BYTE			1
#define DBG_READ_MAC_2BYTE			2
#define DBG_READ_MAC_4BYTE			3
#define DBG_WRITE_MAC_1BYTE			4
#define DBG_WRITE_MAC_2BYTE			5
#define DBG_WRITE_MAC_4BYTE			6
#define DBG_READ_BB_CCK				7
#define DBG_WRITE_BB_CCK			8
#define DBG_READ_BB_OFDM			9
#define DBG_WRITE_BB_OFDM			10
#define DBG_READ_RF					11
#define DBG_WRITE_RF				12
#define DBG_READ_EEPROM_1BYTE		13
#define DBG_WRITE_EEPROM_1BYTE		14
#define DBG_READ_EEPROM_2BYTE		15
#define DBG_WRITE_EEPROM_2BYTE		16
#define DBG_OUT_CMD				17

#define DBG_SWITCH_ANTENNA			26
#define DBG_SET_TXPWR_FOR_ALL_RATE	27
//
// <Roger_Notes> The following DBG IO components are ONLY for 92S.
// Please do NOT revise the macro definitions(i.e., compatible with DLL export functions)
// 2008.11.17.
//
#define DBG_WRITE_EFUSE_1BYTE			53
#define DBG_READ_EFUSE_1BYTE			54
#define DBG_READ_EFUSE_2BYTE			55
#define DBG_READ_EFUSE_4BYTE			56
#define DBG_UPDATE_EFUSE				57

#define DBG_WRITE_BT_EFUSE_1BYTE  		64
#define DBG_READ_BT_EFUSE_1BYTE		65
#define DBG_READ_BT_EFUSE_2BYTE		66
#define DBG_READ_BT_EFUSE_4BYTE		67
#define DBG_UPDATE_BT_EFUSE			68
#define DBG_UPDATE_BT_EFUSE_UTILIZE	69
#define DBG_UPDATE_BT_EFUSE_MAP		70
#define	DBG_BT_CONTROL				75

// Forward declaration.
typedef struct _ADAPTER ADAPTER, *PADAPTER;

VOID
NDBG_Init(
	IN	PADAPTER pAdapter
);

VOID
NDBG_Halt(
	IN	PADAPTER pAdapter
);

NDIS_STATUS
NDBG_BtControl(
	IN	PADAPTER		pAdapter,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	);
BOOLEAN
NDBG_GetBtFwVersion(
	IN	PADAPTER		pAdapter,
	IN	pu2Byte			pBtRealFwVer,
	IN	pu1Byte			pBtFwVer
	);
VOID
NDBG_FwC2hBtControl(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		tmpBuf,
	IN	u1Byte		length
	);
VOID
NDBG_StackBtCoexNotify(
	IN	PADAPTER	Adapter,
	IN	u1Byte		stackOpCode,
	IN	u1Byte		stackOpStatus,
	IN	pu1Byte		tmpBuf,
	IN	u1Byte		length
	);

BOOLEAN
DbgReadMacReg(
	IN	PADAPTER			pAdapter,
	IN	ULONG				ulRegOffset,
	IN	ULONG				ulRegDataWidth
);

BOOLEAN
DbgWriteMacReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
);

BOOLEAN
DbgReadBbReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulBeOFDM
);

BOOLEAN
DbgWriteBbReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulBeOFDM,
	IN	ULONG		ulRegValue
);

BOOLEAN
DbgReadRfReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
);

BOOLEAN
DbgWriteRfReg(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
);

BOOLEAN
DbgReadEeprom(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
);

BOOLEAN
DbgWriteEeprom(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
);

BOOLEAN
DbgOutCmd(
	IN	PADAPTER		pAdapter,
	IN	PUCHAR			ulOutCmd,
	IN	ULONG			ulOutCmdWidth
);

BOOLEAN
DbgReadEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
);

BOOLEAN
DbgWriteEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
);

BOOLEAN
DbgUpdateEFuse(
	IN	PADAPTER	pAdapter	
);

BOOLEAN
DbgReadBTEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth
);

BOOLEAN
DbgWriteBTEFuse(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulRegOffset,
	IN	ULONG		ulRegDataWidth,
	IN	ULONG		ulRegValue
);

BOOLEAN
DbgUpdateBTEFuse(
	IN	PADAPTER	pAdapter	
);

BOOLEAN
DbgSetTxPowerForAllRate(
	IN	PADAPTER	pAdapter,
	IN	ULONG		ulTxPowerData
	);


BOOLEAN
DbgSetTxAntenna(
	IN	PADAPTER	pAdapter,
	IN	u1Byte		selectedAntenna
);

#endif
