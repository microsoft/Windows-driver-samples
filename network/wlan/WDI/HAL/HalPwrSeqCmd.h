#ifndef REALTEK_POWER_SEQUENCE_CMD
#define REALTEK_POWER_SEQUENCE_CMD

#include "Precomp.h"

/*---------------------------------------------*/
//3 The value of cmd: 4 bits
/*---------------------------------------------*/
#define  PWR_CMD_READ 			0x00
     // offset: the read register offset
     // msk: the mask of the read value
     // value: N/A, left by 0
     // note: dirver shall implement this function by read & msk
#define    PWR_CMD_WRITE			0x01
     // offset: the read register offset
     // msk: the mask of the write bits
     // value: write value
     // note: driver shall implement this cmd by read & msk after write
#define    PWR_CMD_POLLING			0x02
     // offset: the read register offset
     // msk: the mask of the polled value
     // value: the value to be polled, masked by the msd field.
     // note: driver shall implement this cmd by
     // do{
     // if( (Read(offset) & msk) == (value & msk) )
     // break;
     // } while(not timeout);
#define    PWR_CMD_DELAY			0x03
     // offset: the value to delay
     // msk: N/A
     // value: the unit of delay, 0: us, 1: ms

#define    PWR_CMD_END			0x04
     // offset: N/A
     // msk: N/A
     // value: N/A

/*---------------------------------------------*/
//3 The value of base: 4 bits
/*---------------------------------------------*/
   // define the base address of each block
#define   PWR_BASEADDR_MAC		0x00
#define   PWR_BASEADDR_USB		0x01
#define   PWR_BASEADDR_PCIE		0x02
#define   PWR_BASEADDR_SDIO 		0x03

/*---------------------------------------------*/
//3 The value of interface_msk: 4 bits
/*---------------------------------------------*/
#define	PWR_INTF_SDIO_MSK		BIT0
#define	PWR_INTF_USB_MSK		BIT1
#define	PWR_INTF_PCI_MSK		BIT2
#define	PWR_INTF_ALL_MSK		(BIT0|BIT1|BIT2|BIT3)

/*---------------------------------------------*/
//3 The value of fab_msk: 4 bits
/*---------------------------------------------*/
#define	PWR_FAB_TSMC_MSK		BIT0
#define	PWR_FAB_UMC_MSK		BIT1
#define	PWR_FAB_ALL_MSK		(BIT0|BIT1|BIT2|BIT3)

/*---------------------------------------------*/
//3The value of cut_msk: 8 bits
/*---------------------------------------------*/
#define	PWR_CUT_TESTCHIP_MSK	BIT0
#define	PWR_CUT_A_MSK			BIT1
#define	PWR_CUT_B_MSK			BIT2
#define	PWR_CUT_C_MSK			BIT3
#define	PWR_CUT_D_MSK			BIT4
#define	PWR_CUT_E_MSK			BIT5
#define	PWR_CUT_F_MSK			BIT6
#define	PWR_CUT_G_MSK			BIT7
#define	PWR_CUT_ALL_MSK		0xFF


typedef enum _PWRSEQ_CMD_DELAY_UNIT_
{
   PWRSEQ_DELAY_US,
   PWRSEQ_DELAY_MS,
} PWRSEQ_DELAY_UNIT;

typedef struct _WL_PWR_CFG_
{
	u2Byte 	offset;
	u1Byte 	cut_msk; 		
	u1Byte 	fab_msk:4; 		
	u1Byte 	interface_msk:4; 		
	u1Byte 	base:4; 	
	u1Byte 	cmd:4; 	
	u1Byte 	msk;
	u1Byte 	value;
	
} WLAN_PWR_CFG, *PWLAN_PWR_CFG;


#define	GET_PWR_CFG_OFFSET(__PWR_CMD)	__PWR_CMD.offset
#define	GET_PWR_CFG_CUT_MASK(__PWR_CMD)	__PWR_CMD.cut_msk
#define	GET_PWR_CFG_FAB_MASK(__PWR_CMD)	__PWR_CMD.fab_msk
#define	GET_PWR_CFG_INTF_MASK(__PWR_CMD)	__PWR_CMD.interface_msk
#define	GET_PWR_CFG_BASE(__PWR_CMD)	__PWR_CMD.base
#define	GET_PWR_CFG_CMD(__PWR_CMD)	__PWR_CMD.cmd
#define	GET_PWR_CFG_MASK(__PWR_CMD)	__PWR_CMD.msk
#define	GET_PWR_CFG_VALUE(__PWR_CMD)	__PWR_CMD.value


//================================================================================
//	Prototype of protected function.
//================================================================================
BOOLEAN
HalPwrSeqCmdParsing(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					CutVersion, 
	IN	u1Byte					FabVersion,
	IN	u1Byte					InterfaceType,
	IN	WLAN_PWR_CFG			PwrCfgCmd[]
	);

#endif
