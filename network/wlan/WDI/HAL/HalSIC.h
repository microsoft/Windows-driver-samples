#ifndef __INC_HALSIC_H
#define __INC_HALSIC_H

//==================================================================
// Note: If SIC_ENABLE under PCIE, because of the slow operation
//	you should 
//	1) "#define USE_WORKITEM	1" 							in PrecompInc.h.
//	2) "#define RTL8723_FPGA_VERIFICATION	1"				in Precomp.h.WlanE.Windows
//	3) "#define LOAD_FIRMWARE_FROM_HEADER	0"	in Precomp.h.WlanE.Windows if needed.
//
#if (RTL8188E_FPGA_TRUE_PHY_VERIFICATION == 1)
#define	SIC_ENABLE				1
#define	SIC_HW_SUPPORT		1
#else
#define	SIC_ENABLE				0
#define	SIC_HW_SUPPORT		0
#endif
//==================================================================


#define	SIC_MAX_POLL_CNT		5

#if(SIC_HW_SUPPORT == 1)
#define	SIC_CMD_READY			0
#define	SIC_CMD_PREWRITE		0x1

#define	SIC_CMD_WRITE_88E			0x40
#define	SIC_CMD_PREREAD_88E		0x2
#define	SIC_CMD_READ_88E			0x80
#define	SIC_CMD_INIT_88E			0xf0
#define	SIC_INIT_VAL_88E			0xff

#define	SIC_INIT_REG_88E			0x1b7
#define	SIC_CMD_REG_88E			0x1EB		// 1byte
#define	SIC_ADDR_REG_88E			0x1E8		// 1b4~1b5, 2 bytes
#define	SIC_DATA_REG_88E			0x1EC		// 1b0~1b3

#define	SIC_CMD_WRITE_92C			0x11
#define	SIC_CMD_PREREAD_92C		0x2
#define	SIC_CMD_READ_92C			0x12
#define	SIC_CMD_INIT_92C			0x1f
#define	SIC_INIT_VAL_92C			0xff

#define	SIC_INIT_REG_92C			0x1b7
#define	SIC_CMD_REG_92C			0x1b6		// 1byte
#define	SIC_ADDR_REG_92C			0x1b4		// 1b4~1b5, 2 bytes
#define	SIC_DATA_REG_92C			0x1b0		// 1b0~1b3


#define GET_SIC_CMD_READ(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_CMD_READ_88E : SIC_CMD_READ_92C

#define GET_SIC_CMD_WRITE(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_CMD_WRITE_88E : SIC_CMD_WRITE_92C

#define GET_SIC_INIT_REG(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_INIT_REG_88E : SIC_INIT_REG_92C

#define GET_SIC_INIT_VAL(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_INIT_VAL_88E : SIC_INIT_VAL_92C

#define GET_SIC_CMD_INIT(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_CMD_INIT_88E : SIC_CMD_INIT_92C


#else
#define	SIC_CMD_READY			0
#define	SIC_CMD_WRITE			1
#define	SIC_CMD_READ			2

#define	SIC_CMD_REG_88E			0x1EB		// 1byte
#define	SIC_ADDR_REG_88E			0x1E8		// 1b9~1ba, 2 bytes
#define	SIC_DATA_REG_88E			0x1EC		// 1bc~1bf

#define	SIC_CMD_REG_92C			0x1b8		// 1byte
#define	SIC_ADDR_REG_92C			0x1b9		// 1b9~1ba, 2 bytes
#define	SIC_DATA_REG_92C			0x1bc		// 1bc~1bf

#define GET_SIC_CMD_READ(_Adapter)		SIC_CMD_READ
#define GET_SIC_CMD_WRITE(_Adapter)	SIC_CMD_WRITE

#endif


#define GET_SIC_CMD_REG(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_CMD_REG_88E : SIC_CMD_REG_92C

#define GET_SIC_CMD_PREREAD(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_CMD_PREREAD_88E : SIC_CMD_PREREAD_92C

#define GET_SIC_ADDR_REG(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_ADDR_REG_88E : SIC_ADDR_REG_92C

#define GET_SIC_DATA_REG(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter)? SIC_DATA_REG_88E : SIC_DATA_REG_92C


VOID
SIC_SetBBReg(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	);

u4Byte
SIC_QueryBBReg(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	);

VOID
SIC_Init(
	IN	PADAPTER	Adapter
	);

BOOLEAN
SIC_LedOff(
	IN	PADAPTER	Adapter
	);
#endif
