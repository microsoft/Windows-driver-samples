#ifndef __INC_N6SDIOPLATFORMWINDOWS_H
#define __INC_N6SDIOPLATFORMWINDOWS_H

//
// SD Bus driver versions
//
#define 	SDBUS_DRIVER_VERSION_1          0x100
#define 	SDBUS_DRIVER_VERSION_2          0x200

//
// Properties of a Secure Digital (SD) card that an SD device driver can set with an SD request.
//
#define	SDP_MEDIA_CHANGECOUNT		0	// 4Bytes data, read-only
#define	SDP_MEDIA_STATE				1	//v1.0 SDPROP_MEDIA_STATE, read-only
#define	SDP_WRITE_PROTECTED			2	//v1.0 BOOLEAN, read-only
#define	SDP_FUNCTION_NUMBER			3	//v1.0 UCHAR, read-only
#define	SDP_FUNCTION_TYPE				4	// v2.0 SDBUS_FUNCTION_TYPE
#define	SDP_BUS_DRIVER_VERSION		5	// v2.0 USHORT
#define	SDP_BUS_WIDTH					6	// v2.0 UCHAR, 1-bit or 4-bits
#define	SDP_BUS_CLOCK					7	// v2.0 ULONG, in KHz
#define	SDP_BUS_INTERFACE_CONTROL	8	// v2.0 UCHAR, This property is corresponding to CCCR[7]
#define	SDP_HOST_BLOCK_LENGTH		9	// v2.0 USHORT, read-only
#define	SDP_FUNCTION_BLOCK_LENGTH	10	// v2.0 USHORT
#define	SDP_FN0_BLOCK_LENGTH			11	// v2.0 USHORT
#define	SDP_FUNCTION_INT_ENABLE		12	// v2.0 BOOLEAN

//SDIO Card Metaformat for multiple CIS areas.
#define	CISTPL_NULL					0x00
#define	CISTPL_CHECKSUM				0x10
#define 	CISTPL_VERS_1       				0x15
#define 	CISTPL_ALTSTR       				0x16
#define 	CISTPL_MANFID       				0x20
#define 	CISTPL_FUNCID       				0x21
#define 	CISTPL_FUNCE        				0x22
#define 	CISTPL_SDIO_STD     				0x91
#define 	CISTPL_SDIO_EXT       			0x92
#define 	CISTPL_END          				0xff 	// The end-of-chain Tuple
#define 	CISTPL_LINK_END     				0xff

#define 	SDIO_BUS_SIZE 512
#define	SDIO_MAX_LENGTH_BYTE_MODE	SDIO_BUS_SIZE

// SDIO CCCR registers
#define	SDIO_CCCRx_SDIOx				0x00	// CCCR/SDIO Revision
#define	SDIO_CCCR_SDx					0x01	// SD Specification Revision
#define	SDIO_CCCR_IOEx					0x02	// IO Enable
#define	SDIO_CCCR_IORx					0x03	// IO Ready
#define 	SDIO_CCCR_IENx					0x04	// Interrupt Enable
#define 	SDIO_CCCR_INTx					0x05	// Interrupt Pending
#define 	SDIO_CCCR_ASx					0x06	// IO Abort
#define 	SDIO_CCCR_BUS_INTF_CTL		0x07	// Bus Interface Control
#define 	SDIO_CCCR_CARD_CAP			0x08	// Card Capability
#define	SDIO_CCCR_COMMON_CIS_PTR	0x09	// Common CIS pointer
#define	SDIO_CCCR_FN0_BLOCK_SIZE		0x10	// Block Size for Function 0
#define	SDIO_CCCR_BEGIN				0x00
#define	SDIO_CCCR_END					0xff

#define	RT_GET_SDIO_COMMON_CISPTR(__pDevice)	(__pDevice->SdioCommonCISPtr)

// FBR registers
#define	SDIO_FBR_FUNC1_BEGIN			0x100	// Offset for Function 1
#define	SDIO_FBR_FUNC1_END			0x1FF

// SDIO CIS Area registers
#define	SDIO_CIS_BEGIN					0x00001000
#define	SDIO_CIS_END					0x00017fff

#define	SDIO_MAX_TX_QUEUE			3		// HIQ, MIQ and LOQ
#define	SDIO_MAX_RX_QUEUE			1

// Forward declaration.
typedef enum _RT_STATUS RT_STATUS;
typedef struct _RT_TCB	RT_TCB, *PRT_TCB; 
typedef struct _SDIO_OUT_CONTEXT *PSDIO_OUT_CONTEXT;
typedef struct _SDIO_IN_CONTEXT *PSDIO_IN_CONTEXT;
//


// Some USB 1.1 host driver cannot afford too large bulkin size on Win2k platform
#define PLATFORM_LIMITED_RX_BUF_SIZE(_ADAPTER) FALSE
// Some USB 1.1 host driver cannot afford too large bulkout size on Win2k platform
#define PLATFORM_LIMITED_TX_BUF_SIZE(_ADAPTER) FALSE
	

extern	PCHAR 
PlatformSystemPowerString(
	SYSTEM_POWER_STATE Type
);
extern 	PCHAR 
PlatformDevicePowerString(
	DEVICE_POWER_STATE Type
);

NTSTATUS
PlatformSdioGetProperty(
	IN 	PRT_SDIO_DEVICE	pDevice,
	IN 	SDBUS_PROPERTY Property,
	IN 	PVOID Buffer,
      	IN 	ULONG Length
);

NTSTATUS
PlatformSdioSetProperty(
       IN 	PRT_SDIO_DEVICE	pDevice,
       IN 	SDBUS_PROPERTY Property,
       IN 	PVOID Buffer,
       IN 	ULONG Length
);

RT_STATUS
PlatformSdioCmd53ReadWrite(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			DeviceID,
	UCHAR			funcNum,			
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeFlag,
	PVOID			buffer
);

RT_STATUS
PlatformSdioCmd53ReadWriteBlock(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,			
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeFlag,
	PVOID			buffer
);

RT_STATUS
PlatformSdioCmd53ReadWriteByte(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,			
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeFlag,
	PVOID			buffer
);

RT_STATUS
PlatformSdioCmd53ReadWriteMDL(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,	
	PMDL			pmdl,
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeToDevice,
	BOOLEAN			blockMode
);

RT_STATUS
PlatformSdioCmd52ReadWrite(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			DeviceID,
	UCHAR			funcNum,		
	ULONG			byteCount,
	ULONG			registerIndex,	
	BOOLEAN			writeToDevice,
	PVOID			buffer
);

RT_STATUS
PlatformSdioCmd52ReadWriteByte(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,
	ULONG			registerIndex,
	PUCHAR			data,
	BOOLEAN			writeToDevice
);


//
// Asyn I/O Write
//
NTSTATUS
IssueIrpForAsynSdioIOWrite(
	PRT_SDIO_DEVICE	device, 
	u2Byte			Count,
	u4Byte			Index,
	PVOID			pOutRegisterData
);

#if RTL8723_SDIO_IO_THREAD_ENABLE
VOID
SdioAsynIOWriteEnqueue(
	PRT_SDIO_DEVICE	device, 
	u1Byte		DeviceID,
	UCHAR		FuncNum,	
	u2Byte			Count,
	u4Byte		Index,
	PVOID			pOutRegisterData
);
#endif

NTSTATUS
SdioAsynIOWrite(
	PRT_SDIO_DEVICE	device, 
	u1Byte		DeviceID,
	UCHAR		FuncNum,	
	u2Byte			Count,
	u4Byte		Index,
	PVOID			pOutRegisterData
);

VOID
SdioIOComplete(
	PRT_SDIO_DEVICE	device
);

NTSTATUS
SdioAsynIOWriteComplete(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			pIrp,
	PVOID			Context
);


VOID
RTsdioCancelAsynIoPendingIrp(
	IN	PVOID		Adapter
);


BOOLEAN
PrepareSdioAWBs(
	PRT_SDIO_DEVICE	device
);


BOOLEAN
FreeSdioAWBs(
	PRT_SDIO_DEVICE	device,
	BOOLEAN			bReset
);


VOID
ReturnSdioAWB(
	PRT_SDIO_DEVICE	device,
	PRT_AWB			pAwb
);

#endif // #ifndef __INC_N6SDIOPLATFORMWINDOWS_H
