#ifndef __INC_HALDEF_H
#define __INC_HALDEF_H

//
// General Definition for 
//
/* Common definition for all 819x series driver */

//
// 2012/02/02 MH Add HW IC support here. To repacle the definition from precomp.h
// to current position. If Linux need to use small HAl code size, please disable
// IC support. We will list all IC support ability here.
//
#define	RTL8192S_SUPPORT                            0
#define	RTL8192C_SUPPORT                            0
#define	RTL8192D_SUPPORT                            0
#define	RTL8723A_SUPPORT				            0
#define	RTL8195A_SUPPORT				            0
#define	RTL8723B_SUPPORT				            1
#if(MS_SUPPORT == 0)
#define	RTL8703B_SUPPORT				            0
#define	RTL8188E_SUPPORT				            0
#define	RTL8812A_SUPPORT				            0
#define	RTL8821A_SUPPORT							0
#define	RTL8192E_SUPPORT							0
#define	RTL8814A_SUPPORT							0
#define	RTL8821B_SUPPORT				            0
#define	RTL8822B_SUPPORT							0
#define	RTL8188F_SUPPORT				            0
#define	RTL8723D_SUPPORT				            0

#define TESTCHIP_SUPPORT							1
#else
#define	RTL8703B_SUPPORT				            0
#define	RTL8188E_SUPPORT				            0
#define	RTL8812A_SUPPORT				            0
#define	RTL8821A_SUPPORT							0
#define	RTL8192E_SUPPORT							0
#define	RTL8814A_SUPPORT							0
#define	RTL8821B_SUPPORT				            0
#define	RTL8822B_SUPPORT							0
#define	RTL8188F_SUPPORT				            0
#define	RTL8723D_SUPPORT				            0

#define TESTCHIP_SUPPORT							0
#endif
//
// For different HW register polling type definition.
//
#define	REG_POLL_1BYTE_EQU							0
#define	REG_POLL_2BYTE_EQU							1
#define	REG_POLL_4BYTE_EQU							2
#define	REG_POLL_1BYTE_BIT_MASK						3
#define	REG_POLL_2BYTE_BIT_MASK						4
#define	REG_POLL_4BYTE_BIT_MASK						5

//
// For different memory size setting, you can switch HW IC support.
//


#define RTL819X_NO_SCAN_AFTER_LINK					0

#define	U1DONTCARE 	0xFF	
#define	U2DONTCARE 	0xFFFF	

#define	INTEL_VENDOR_ID			0x8086
#define	SIS_VENDOR_ID			0x1039
#define	ATI_VENDOR_ID			0x1002
#define	ATI_DEVICE_ID			0x7914
#define	AMD_VENDOR_ID			0x1022

#define MIN_DESC_BUFFER_LENGTH				5

//
// 2011/09/07 MH Add for different channel plan power index offset 
//
#define	PWR_IDX_GROUP_NUM			4		// CCK/Legacyofdm/HT20/HT40

typedef enum _PCI_BRIDGE_VENDOR
{
    PCI_BRIDGE_VENDOR_INTEL = 0,
    PCI_BRIDGE_VENDOR_ATI,
    PCI_BRIDGE_VENDOR_AMD,
    PCI_BRIDGE_VENDOR_SIS,
    PCI_BRIDGE_VENDOR_UNKNOWN,
    PCI_BRIDGE_VENDOR_MAX
} PCI_BRIDGE_VENDOR;


typedef enum _GPIORF_CONTROL{
	GPIORF_START = 0,
	GPIORF_STOP = 1,
	GPIORF_POLLING = 2,
}GPIORF_CONTROL;


typedef	enum _POWER_SAVE_MODE	// <power save mode> parameter of MlmePowerMgt
{
	POWER_SAVE_MODE_ACTIVE,
	POWER_SAVE_MODE_SAVE,
}POWER_SAVE_MODE;

//
// <Roger_Notes> For RTL8723 WiFi/BT/GPS multi-function configuration. 2010.10.06.
//
typedef enum _RT_MULTI_FUNC{
	RT_MULTI_FUNC_NONE = 0x00,
	RT_MULTI_FUNC_WIFI = 0x01,
	RT_MULTI_FUNC_BT = 0x02,
	RT_MULTI_FUNC_GPS = 0x04,
}RT_MULTI_FUNC,*PRT_MULTI_FUNC;


#define INCLUDE_MULTI_FUNC_BT(_Adapter)	(GET_HAL_DATA(_Adapter)->MultiFunc & RT_MULTI_FUNC_BT)
#define INCLUDE_MULTI_FUNC_GPS(_Adapter)	(GET_HAL_DATA(_Adapter)->MultiFunc & RT_MULTI_FUNC_GPS)

//
// <Roger_Notes> For RTL8723 WiFi PDn/GPIO polarity control configuration. 2010.10.08.
//
typedef enum _RT_POLARITY_CTL{
	RT_POLARITY_LOW_ACT = 0,
	RT_POLARITY_HIGH_ACT = 1,	
}RT_POLARITY_CTL,*PRT_POLARITY_CTL;

// For RTL8723 regulator mode. by tynli. 2011.01.14.
typedef enum _RT_REGULATOR_MODE{
	RT_SWITCHING_REGULATOR = 0,
	RT_LDO_REGULATOR = 1,	
}RT_REGULATOR_MODE,*PRT_REGULATOR_MODE;


typedef enum _DBI_TYPE
{
	DBI_READ,
	DBI_WRITE
} DBI_TYPE, *PDBI_TYPE;


#define			HAL_TIMER_OWNER_P2P_POWERSAVE			BIT0
#define			HAL_TIMER_OWNER_TDLS_OFF_CHNL_TRIGGER	BIT1
#define			HAL_TIMER_OWNER_TDLS_OFF_CHNL_TIMEOUT	BIT2
#define			HAL_TIMER_OWNER_MULTICHANNEL_SWITCH	BIT3

#define			HAL_TIMEOUT_REASON_INTERRUPT			1
#define			HAL_TIMEOUT_REASON_HALT					2


typedef VOID
(*HAL_TIMER_CALLBACK)(
	IN	PVOID	pContext
	);

typedef	struct	_HAL_TIMER_STATE
{
	RT_LIST_ENTRY		List;
	u4Byte				TimerOwners;		// The owners who have the timeout values.
	BOOLEAN				bAbsolute;			// True if the timeout is TSF matched with Timeout.
	u8Byte				Timeout;			// Timeout TSF timer for client mode.
	PVOID				pContext;			// Timeout callback context.
	HAL_TIMER_CALLBACK	callbackFunc;		// The timeout callback function.
}HAL_TIMER_STATE, *PHAL_TIMER_STATE;

#define	HAL_TIMER_FILL_TIMER_STATE(__pHalTimerState, __Owner, __DelayUs, __pContext, __Callback)	\
{	\
	(__pHalTimerState)->TimerOwners = __Owner;	\
	(__pHalTimerState)->bAbsolute = FALSE;	\
	(__pHalTimerState)->Timeout = __DelayUs;	\
	(__pHalTimerState)->pContext = __pContext;	\
	(__pHalTimerState)->callbackFunc = __Callback;	\
}

#define	HAL_TIMER_FILL_TIMER_STATE_FULL_TSF_TIME(__pHalTimerState, __Owner, __TsfUs, __pContext, __Callback)	\
{	\
	(__pHalTimerState)->TimerOwners = __Owner;	\
	(__pHalTimerState)->bAbsolute = TRUE;	\
	(__pHalTimerState)->Timeout = __TsfUs;	\
	(__pHalTimerState)->pContext = __pContext;	\
	(__pHalTimerState)->callbackFunc = __Callback;	\
}


typedef enum _RT_AMPDU_BRUST_MODE{
	RT_AMPDU_BRUST_NONE = 0,
	RT_AMPDU_BRUST_92D = 1,
	RT_AMPDU_BRUST_88E = 2,
	RT_AMPDU_BRUST_8812_4 = 3,
	RT_AMPDU_BRUST_8812_8 = 4,
	RT_AMPDU_BRUST_8812_12 = 5,
	RT_AMPDU_BRUST_8812_15 = 6,
	RT_AMPDU_BRUST_8723B = 7,
	RT_AMPDU_BRUST_8814A= 8,
	RT_AMPDU_BRUST_8822B= 9

}RT_AMPDU_BRUST,*PRT_AMPDU_BRUST_MODE;


// The power in dbm we never support. So define it as the unspecified power.
#define	UNSPECIFIED_PWR_DBM		(~0x7F)

// RF Off Level for IPS or HW/SW radio off
#define	RT_RF_OFF_LEVL_ASPM			BIT0	// PCI ASPM
#define	RT_RF_OFF_LEVL_CLK_REQ		BIT1	// PCI clock request
#define	RT_RF_OFF_LEVL_PCI_D3		BIT2	// PCI D3 mode
#define	RT_RF_OFF_LEVL_HALT_NIC		BIT3	// NIC halt, re-initialize hw parameters
#define	RT_RF_OFF_LEVL_FREE_FW		BIT4	// FW free, re-download the FW
#define	RT_RF_OFF_LEVEL_FW_IPS_32K		BIT5	// FW in IPS 32k
#define	RT_RF_PS_LEVEL_ALWAYS_ASPM	BIT6	// Always enable ASPM and Clock Req in initialization.
#define	RT_RF_OFF_LEVEL_RTD3		BIT7 // Follow wowlan Hw flow and control RF on/off by Fw
#define	RT_RF_LPS_P2P_PS			BIT29	// It's a P2P Power Save
#define	RT_RF_LPS_DISALBE_2R		BIT30	// When LPS is on, disable 2R if no packet is received or transmittd.
#define	RT_RF_LPS_LEVEL_ASPM		BIT31	// LPS with ASPM


// ASPM OSC Control bit, added by Roger, 2013.03.29.
#define	RT_PCI_ASPM_OSC_IGNORE		0	 // PCI ASPM ignore OSC control in default
#define	RT_PCI_ASPM_OSC_ENABLE		BIT0 // PCI ASPM controlled by OS according to ACPI Spec 5.0
#define	RT_PCI_ASPM_OSC_DISABLE		BIT1 // PCI ASPM controlled by driver or BIOS, i.e., force enable ASPM

// ASPM Backdoor Control
#define RT_PCI_BC_CLK_REQ			BIT0
#define RT_PCI_BC_ASPM_L0s			BIT1
#define RT_PCI_BC_ASPM_L1			BIT2
#define RT_PCI_BC_ASPM_L1Off		BIT3
#define RT_PCI_BC_ASPM_LTR			BIT4
#define RT_PCI_BC_ASPM_OBFF			BIT5

#if (RTL8723B_SUPPORT == 1)
// RTL8723B Series
#define IS_HARDWARE_TYPE_8723BE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8723BE)
#define IS_HARDWARE_TYPE_8723BS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8723BS)
#define IS_HARDWARE_TYPE_8723BU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8723BU)
#define	IS_HARDWARE_TYPE_8723B(_Adapter)			\
(IS_HARDWARE_TYPE_8723BE(_Adapter) || IS_HARDWARE_TYPE_8723BU(_Adapter) || IS_HARDWARE_TYPE_8723BS(_Adapter) )
#else
#define IS_HARDWARE_TYPE_8723BE(_Adapter)		FALSE
#define IS_HARDWARE_TYPE_8723BS(_Adapter)		FALSE
#define IS_HARDWARE_TYPE_8723BU(_Adapter)		FALSE
#define IS_HARDWARE_TYPE_8723B(_Adapter)		FALSE
#endif

// RTL8188E Series
#define IS_HARDWARE_TYPE_8188EE(_Adapter)	(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8188EE)
#define IS_HARDWARE_TYPE_8188EU(_Adapter)	(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8188EU)
#define IS_HARDWARE_TYPE_8188ES(_Adapter)	(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8188ES)

#define IS_HARDWARE_TYPE_8188E(_Adapter)			\
(IS_HARDWARE_TYPE_8188EE(_Adapter) || IS_HARDWARE_TYPE_8188EU(_Adapter) || IS_HARDWARE_TYPE_8188ES(_Adapter))

#define IS_HARDWARE_TYPE_OLDER_THAN_8812A(_Adapter)	\
	(IS_HARDWARE_TYPE_8188EE(_Adapter) || IS_HARDWARE_TYPE_8188EU(_Adapter) || IS_HARDWARE_TYPE_8188ES(_Adapter))

// RTL8812 Series
#define IS_HARDWARE_TYPE_8812AE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8812E)
#define IS_HARDWARE_TYPE_8812AU(_Adapter)	(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8812AU)
#define IS_HARDWARE_TYPE_8812A(_Adapter)			\
	(IS_HARDWARE_TYPE_8812AE(_Adapter) || IS_HARDWARE_TYPE_8812AU(_Adapter))


// RTL8821 Series
#define IS_HARDWARE_TYPE_8821AE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821E)
#define IS_HARDWARE_TYPE_8811AU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8811AU)
#define IS_HARDWARE_TYPE_8821AU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821U)
#define IS_HARDWARE_TYPE_8821U(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821U ||\
	              								 ((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8811AU)
#define IS_HARDWARE_TYPE_8821S(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821S)
#define IS_HARDWARE_TYPE_8821(_Adapter)			\
	(IS_HARDWARE_TYPE_8821AE(_Adapter) || IS_HARDWARE_TYPE_8821U(_Adapter)|| IS_HARDWARE_TYPE_8821S(_Adapter))
#define IS_HARDWARE_TYPE_8821A(_Adapter)			\
	(IS_HARDWARE_TYPE_8821AE(_Adapter) || IS_HARDWARE_TYPE_8821U(_Adapter)|| IS_HARDWARE_TYPE_8821S(_Adapter))

#define IS_HARDWARE_TYPE_JAGUAR(_Adapter)		\
(IS_HARDWARE_TYPE_8812A(_Adapter) || IS_HARDWARE_TYPE_8821(_Adapter))

#define IS_HARDWARE_TYPE_8821BE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821BE)
#define IS_HARDWARE_TYPE_8821BU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821BU)
#define IS_HARDWARE_TYPE_8821BS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8821BS)
#define IS_HARDWARE_TYPE_8821B(_Adapter)		\
(IS_HARDWARE_TYPE_8821BE(_Adapter) || IS_HARDWARE_TYPE_8821BU(_Adapter) || IS_HARDWARE_TYPE_8821BS(_Adapter))

#define IS_HARDWARE_TYPE_8822BE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8822BE)
#define IS_HARDWARE_TYPE_8822BU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8822BU)
#define IS_HARDWARE_TYPE_8822BS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8822BS)
#define IS_HARDWARE_TYPE_8822B(_Adapter)		\
(IS_HARDWARE_TYPE_8822BE(_Adapter) || IS_HARDWARE_TYPE_8822BU(_Adapter) || IS_HARDWARE_TYPE_8822BS(_Adapter))


#define IS_HARDWARE_TYPE_JAGUAR2(_Adapter)		\
(IS_HARDWARE_TYPE_8814A(_Adapter) || IS_HARDWARE_TYPE_8821B(_Adapter) || IS_HARDWARE_TYPE_8822B(_Adapter))

#define IS_HARDWARE_TYPE_JAGUAR_AND_JAGUAR2(_Adapter)		\
(IS_HARDWARE_TYPE_JAGUAR(_Adapter) || IS_HARDWARE_TYPE_JAGUAR2(_Adapter))


//RTL8192E Series
#define IS_HARDWARE_TYPE_8192EE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192EE)
#define IS_HARDWARE_TYPE_8192EU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192EU)
#define IS_HARDWARE_TYPE_8192ES(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8192ES)

#define IS_HARDWARE_TYPE_8192E(_Adapter)		\
(IS_HARDWARE_TYPE_8192EE(_Adapter) || IS_HARDWARE_TYPE_8192EU(_Adapter) ||IS_HARDWARE_TYPE_8192ES(_Adapter))

//RTL8814A Series
#define IS_HARDWARE_TYPE_8814AE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8814AE)
#define IS_HARDWARE_TYPE_8814AU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8814AU)
#define IS_HARDWARE_TYPE_8814AS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8814AS)

#define IS_HARDWARE_TYPE_8814A(_Adapter)		\
(IS_HARDWARE_TYPE_8814AE(_Adapter) || IS_HARDWARE_TYPE_8814AU(_Adapter) ||IS_HARDWARE_TYPE_8814AS(_Adapter))

//RTL8723D Series
#define IS_HARDWARE_TYPE_8723DE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8723DE)
#define IS_HARDWARE_TYPE_8723DU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8723DU)
#define IS_HARDWARE_TYPE_8723DS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8723DS)

#define IS_HARDWARE_TYPE_8723D(_Adapter)		\
(IS_HARDWARE_TYPE_8723DE(_Adapter) || IS_HARDWARE_TYPE_8723DU(_Adapter) ||IS_HARDWARE_TYPE_8723DS(_Adapter))


#define IS_HARDWARE_TYPE_UNKNOWN(_Adapter)	(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_MAX)

#define IS_DUAL_BAND_SUPPORT(_Adapter)		(( (GET_HAL_DATA(_Adapter)->BandSet == BAND_ON_BOTH )? TRUE:FALSE))

// RTL8703B Series
#define IS_HARDWARE_TYPE_8703BE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8703BE)
#define IS_HARDWARE_TYPE_8703BS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8703BS)
#define IS_HARDWARE_TYPE_8703BU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8703BU)
#define	IS_HARDWARE_TYPE_8703B(_Adapter)			\
(IS_HARDWARE_TYPE_8703BE(_Adapter) || IS_HARDWARE_TYPE_8703BU(_Adapter) || IS_HARDWARE_TYPE_8703BS(_Adapter) )

#define	IS_HALF_CAM_ENTEY_SERIES(_Adapter)       (IS_HARDWARE_TYPE_8703B(_Adapter) || IS_HARDWARE_TYPE_8188F(_Adapter))

#define IS_ONE_BYTE_ALIGNED_SERIES(_Adapter)       (IS_HARDWARE_TYPE_8703B(_Adapter) || IS_HARDWARE_TYPE_8188F(_Adapter) || IS_HARDWARE_TYPE_8822B(_Adapter))

#define IS_EFUSE_RECOVERY_NEEDED_SERIES(_Adapter)       (IS_HARDWARE_TYPE_8188E(_Adapter) || IS_HARDWARE_TYPE_8723B(_Adapter) || IS_HARDWARE_TYPE_8192E(_Adapter))

#define IS_NEW_USB_RXAGG_SERIES(_Adapter)       (IS_HARDWARE_TYPE_8703BU(_Adapter) || IS_HARDWARE_TYPE_8723DU(_Adapter) || IS_HARDWARE_TYPE_8822BU(_Adapter))

// RTL8188F Series
#define IS_HARDWARE_TYPE_8188FE(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8188FE)
#define IS_HARDWARE_TYPE_8188FS(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8188FS)
#define IS_HARDWARE_TYPE_8188FU(_Adapter)		(((PADAPTER)_Adapter)->HardwareType==HARDWARE_TYPE_RTL8188FU)
#define	IS_HARDWARE_TYPE_8188F(_Adapter)			\
( IS_HARDWARE_TYPE_8188FE(_Adapter) || IS_HARDWARE_TYPE_8188FS(_Adapter) || IS_HARDWARE_TYPE_8188FU(_Adapter) )

#define	RT_DISABLE_ASPM(_PADAPTER)
#define	RT_ENABLE_ASPM(_PADAPTER)
#define	RT_ENTER_D3(_PADAPTER, _bTempSetting)
#define	RT_LEAVE_D3(_PADAPTER, _bTempSetting)
#define RT_DBG_YB(_PADAPTER, _Offset)


#define	RT_CHECK_TX_IO_QUEUE_EMPTY(_PADAPTER)	PlatformSdioTxAndAwbQueueEmpty(_PADAPTER)
#define	RT_CHECK_TX_QUEUE_IDX_EMPTY(_PADAPTER, _QIDX)		PlatformSdioTxQueueIdxEmpty(_PADAPTER, _QIDX)

#define	RT_IS_DC_MODE(_pPSC)						IsCurrDCMode(_pPSC)

#define	HW_ENABLE_TX_ALL			0
#define	HW_DISABLE_TX_ALL			1
#define	HW_DISABLE_TX_DATA		2
#define	HW_DISABLE_TX_BEACON				3
#define	HW_ENABLE_TX_BEACON				4

// Fw PS state for RPWM.
/*
 BIT[2:0] = HW state
 BIT[3] = Protocol PS state,   1: register active state , 0: register sleep state
 BIT[4] = sub-state
*/
#define	FW_PS_GO_ON			BIT(0)
#define	FW_PS_TX_NULL			BIT(1)
#define	FW_PS_RF_ON			BIT(2)
#define	FW_PS_REGISTER_ACTIVE	BIT(3)

#define	FW_PS_DPS    		BIT0
#define	FW_PS_LCLK   		(FW_PS_DPS)
#define	FW_PS_RF_OFF   		BIT1
#define	FW_PS_ALL_ON   		BIT2
#define	FW_PS_ST_ACTIVE  	BIT3
//#define	FW_PS_LP    			BIT4 // low performance
#define	FW_PS_ISR_ENABLE    	BIT4
#define	FW_PS_IMR_ENABLE	BIT5


#define	FW_PS_ACK    		BIT6
#define	FW_PS_TOGGLE   		BIT7

 // 88E RPWM value
 // BIT[0] = 1: 32k, 0: 40M
#define	FW_PS_CLOCK_OFF		BIT0		// 32k
#define	FW_PS_CLOCK_ON		0		// 40M
 
#define	FW_PS_STATE_MASK  		(0x0F)
#define	FW_PS_STATE_HW_MASK 	(0x07)
#define	FW_PS_STATE_INT_MASK 	(0x3F)	// ISR_ENABLE, IMR_ENABLE, and PS mode should be inherited.

#define	FW_PS_STATE(x)   		(FW_PS_STATE_MASK & (x))
#define	FW_PS_STATE_HW(x) 		(FW_PS_STATE_HW_MASK & (x))
#define	FW_PS_STATE_INT(x)   	(FW_PS_STATE_INT_MASK & (x))
#define	FW_PS_ISR_VAL(x)		((x) & 0x70)
#define	FW_PS_IMR_MASK(x)   	((x) & 0xDF)
#define	FW_PS_KEEP_IMR(x)		((x) & 0x20)

 
#define	FW_PS_STATE_S0		(FW_PS_DPS)
#define	FW_PS_STATE_S1		(FW_PS_LCLK)
#define	FW_PS_STATE_S2		(FW_PS_RF_OFF)
#define	FW_PS_STATE_S3		(FW_PS_ALL_ON)
#define	FW_PS_STATE_S4		((FW_PS_ST_ACTIVE) | (FW_PS_ALL_ON))

#define	FW_PS_STATE_ALL_ON_88E	(FW_PS_CLOCK_ON) // ((FW_PS_RF_ON) | (FW_PS_REGISTER_ACTIVE))
#define	FW_PS_STATE_RF_ON_88E	(FW_PS_CLOCK_ON) // (FW_PS_RF_ON)
#define	FW_PS_STATE_RF_OFF_88E	(FW_PS_CLOCK_ON) // 0x0
#define	FW_PS_STATE_RF_OFF_LOW_PWR_88E	(FW_PS_CLOCK_OFF) // (FW_PS_STATE_RF_OFF)

#define	FW_PS_STATE_ALL_ON_92C	(FW_PS_STATE_S4)
#define	FW_PS_STATE_RF_ON_92C		(FW_PS_STATE_S3)
#define	FW_PS_STATE_RF_OFF_92C	(FW_PS_STATE_S2)
#define	FW_PS_STATE_RF_OFF_LOW_PWR_92C	(FW_PS_STATE_S1) 


// For 88E H2C PwrMode Cmd ID 5.
#define	FW_PWR_STATE_ACTIVE	((FW_PS_RF_ON) | (FW_PS_REGISTER_ACTIVE))
#define	FW_PWR_STATE_RF_OFF	0

#define	FW_PS_IS_ACK(x)  		((x) & FW_PS_ACK ) 
#define	FW_PS_IS_CLK_ON(x) 		((x) & (FW_PS_RF_OFF |FW_PS_ALL_ON ))
#define	FW_PS_IS_RF_ON(x)  		((x) & (FW_PS_ALL_ON))
#define	FW_PS_IS_ACTIVE(x)  		((x) & (FW_PS_ST_ACTIVE))
#define	FW_PS_IS_CPWM_INT(x)	((x) & 0x40)

#define	FW_CLR_PS_STATE(x) 		((x) = ((x) & (0xF0)))

#define	IS_IN_LOW_POWER_STATE_88E(pAdapter, FwPSState)		\
			(FW_PS_STATE(FwPSState) == FW_PS_CLOCK_OFF)

#define	IS_IN_LOW_POWER_STATE_92C(pAdapter, FwPSState)		\
			((FW_PS_STATE(FwPSState) == FW_PS_STATE_RF_OFF_LOW_PWR_92C)	&& \
				GET_POWER_SAVE_CONTROL(&(pAdapter->MgntInfo))->bLowPowerEnable)

//
// <Roger_TODO> We should take RTL8723B into consideration, 2012.10.08
//
#define IS_IN_LOW_POWER_STATE(_Adapter, _FwPSState)		IS_IN_LOW_POWER_STATE_88E(_Adapter, _FwPSState)


#define 	HW_CLOCK_OFF_TIMEOUT		10 // unit: ms

enum _Fw_Ps_State{
	FW_PS_STATE_ALL_ON = 0,
	FW_PS_STATE_RF_ON = 1,
	FW_PS_STATE_RF_OFF = 2,
	FW_PS_STATE_RF_OFF_LOW_PWR = 3,	
};

typedef enum _LOWPWR32K_EXCEPTION
{
	LOWPWR32K_EXCEPTION_NONE = 0x00,
	LOWPWR32K_EXCEPTION_CPWM_LOST = 0x01,
	LOWPWR32K_EXCEPTION_FW_HANG = 0x02,
	LOWPWR32K_EXCEPTION_CANNOT_RECOVER_HW_CLK = 0x03,
	LOWPWR32K_EXCEPTION_HW_POWER_OFF = 0x04,
	LOWPWR32K_EXCEPTION_MAX
}LOWPWR32K_EXCEPTION;

typedef enum _RSVDPAGE_TYPE
{
	RSVDPAGE_TYPE_BASIC = 0x00,
	RSVDPAGE_TYPE_LPS = 0x01,
	RSVDPAGE_TYPE_WOWLAN = 0x02,
	RSVDPAGE_TYPE_FCS = 0x04,
	RSVDPAGE_TYPE_BT = 0x08,
	RSVDPAGE_TYPE_MAX = 0xffff
}RSVDPAGE_TYPE;

typedef enum _RQPN_TYPE
{
	RQPN_TYPE_NORMAL = 0x00,
	RQPN_TYPE_WOWLAN  = 0x01,
	RQPN_TYPE_MAX
}RQPN_TYPE;

// ------------------------------------------------------------------------------------------------
// Specific  definition depend on different chipset 
// ------------------------------------------------------------------------------------------------
#define SILENT_RESET									0



// ------------------------------------------------------------------------------------------------
// We can try to handle all Tx shift condition together!!
// This includes the TxDesc for 8187 and 8187B, TxFwInfo for 8190Pci, and other shift for 8190Usb
// Caution: This is not implement now!!
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// The definition for FindAdapter Procedure
// (1) PCI memory mapped IO range. 
// (2) Revision ID
// (3) Default Hardware Type
// (4) Require IO mapped IO Even if Memory Mapped IO is applied
// ------------------------------------------------------------------------------------------------

#define HAL_HW_SDIO_8723S_PID				0x8723 // 8723S
#define HAL_HW_SDIO_8188E_PID				0x8179 //8188E
#define HAL_HW_SDIO_8821S_PID				0x8821
#define HAL_HW_SDIO_8192ES_PID			0x818B
#define HAL_HW_SDIO_8723BS_PID_TEST_CHIP	0x8753
#define HAL_HW_SDIO_8723BS_PID			0xB723
#define HAL_HW_SDIO_8723BS_PID_ACER 		0x0623
#define HAL_HW_SDIO_8723BS_PID_HP 		0x0523
#define HAL_HW_SDIO_8814AS_PID				0x8813//20130415 KaiYuan add for 8814 Temp
#define HAL_HW_SDIO_8703BS_PID				0xB703
#define HAL_HW_SDIO_8814AS_PID			0x8813//20130415 KaiYuan add for 8814 Temp
#define HAL_HW_SDIO_8821BS_PID			0xB821
#define HAL_HW_SDIO_8822BS_PID				0xB822
#define HAL_HW_SDIO_8188FS_PID			0xF179
#define HAL_HW_SDIO_8723DS_PID			0xD723  //8723D

#define HAL_HW_TYPE_ID_8723A				0x01	
#define HAL_HW_TYPE_ID_8188E				0x02
#define HAL_HW_TYPE_ID_8812A				0x04
#define HAL_HW_TYPE_ID_8821A				0x05
#define HAL_HW_TYPE_ID_8723B				0x06
#define HAL_HW_TYPE_ID_92E					0x07
#define HAL_HW_TYPE_ID_8814A				0x08
#define HAL_HW_TYPE_ID_8821B				0x09
#define HAL_HW_TYPE_ID_8822B				0x0a
#define HAL_HW_TYPE_ID_8703B				0x0b
#define HAL_HW_TYPE_ID_8188F				0x0c
#define HAL_HW_TYPE_ID_8723D				0x0f


// Power on  I/O space range.
#define IS_PCIE_POWER_ON_IO_REG(offset)	(((offset < 0x100) || (offset >= 0x0300 && offset < 0x0400)) ? TRUE: FALSE)
#define IS_SDIO_POWER_ON_IO_REG(DeviceID, offset) ( \
				((DeviceID == WLAN_IOREG_DEVICE_ID && (offset < 0x100)) ||\
				(DeviceID == SDIO_LOCAL_DEVICE_ID)) ? TRUE: FALSE)
#define IS_USB_POWER_ON_IO_REG(offset)	((offset < 0x0100 || offset >= 0xFE00) ? TRUE: FALSE)

//
// (1) Define the data type of ISR/IMR on each IC. 2005.11.13, by rcnjko.
// (2) Define allowed AMSDU size according to Tx FIFO size indifferent hardware,
// (3) Define Retry Limit for different hardware
//       2007/11/12 by Emily
//
	
#define HAL_GET_MAXIMUN_AMSDU_SIZE(_Size)			_Size	// Limited by TX FIFO Size	
#define HAL_GET_DECLARED_AMSDU_SIZE(_Peer_Cap)		(_Peer_Cap==0)?3839:7935;

// Set diffrent retry limit in infrastructure mode and adhoc mode because if driver is set
// in adhoc mode, driver has to send probe response, and if the other STA is scaning and switch
// to another channel, driver will send to many useless probe response packet. Mangament
// frame has higher priority which may cause starvation of data packet. 2008/05/14 by Emily

// The reason to set HAL_RETRY_LIMIT_INFRA to 0x30
// (1) Lanhsin modified 02272008 for Linksys WRT350N. Under air, if we don't retry more 
//      times, these APs can't rx ok and the TP will be bad.
// (2) Cosa modified 02272008 for BelkinF5D(Ralink) AP, Netgear WNR854T. Under air, if 
//      we don't retry more times, these APs can't rx ok and the TP will be bad.
#define HAL_RETRY_LIMIT_INFRA							48	
#define HAL_RETRY_LIMIT_AP_ADHOC						14

typedef u4Byte RT_INT_REG, *PRT_INT_REG;

// ------------------------------------------------------------------------------------------------
// Define file name for Firmware image and PHY configuration
// ------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------
//		RTL8192CU From file
//---------------------------------------------------------------------

			//2TODO:  The following need to check!!
			#define	RTL8192C_FW_TSMC_IMG				"rtl8192CU\\rtl8192cfwT.bin"
			#define	RTL8192C_FW_UMC_IMG				"rtl8192CU\\rtl8192cfwU.bin"
			#define 	RTL8192C_FW_UMC_B_IMG				"rtl8192CU\\rtl8192cfwU_B.bin"
			#define	RTL8192D_FW_IMG					"rtl8192DU\\rtl8192dfw.bin"
			#define 	RTL8192C_FW_TSMC_WW_IMG			"rtl8192CU\\rtl8192cfwTww.bin"
			#define 	RTL8192C_FW_UMC_WW_IMG			"rtl8192CU\\rtl8192cfwUww.bin"
			#define 	RTL8192C_FW_UMC_B_WW_IMG		"rtl8192CU\\rtl8192cfwU_Bww.bin"

			#define RTL8188C_PHY_REG					"rtl8192CU\\PHY_REG_1T.txt"	
			#define RTL8188C_PHY_RADIO_A				"rtl8192CU\\radio_a_1T.txt"
			#define RTL8188C_PHY_RADIO_B				"rtl8192CU\\radio_b_1T.txt"	
			#define RTL8188C_AGC_TAB					"rtl8192CU\\AGC_TAB_1T.txt"
			#define RTL8188C_PHY_MACREG					"rtl8192CU\\MAC_REG.txt"
			#define RTL8188C_PHY_RADIO_A_mCard			"rtl8192CU\\radio_a_1T_mCard.txt"
			#define RTL8188C_PHY_RADIO_B_mCard			"rtl8192CU\\radio_b_1T_mCard.txt" 
			#define RTL8188C_PHY_RADIO_A_HP			"rtl8192CU\\radio_a_1T_HP.txt"
			#define RTL8188C_PHY_REG_HP				"rtl8192CU\\PHY_REG_1T_HP.txt"	
			#define RTL8188C_PHY_REG_mCard 			"rtl8192CU\\PHY_REG_1T_mCard.txt"				
			#define RTL8188C_AGC_TAB_HP				"rtl8192CU\\AGC_TAB_1T_HP.txt"
			
			#define RTL8192C_PHY_REG					"rtl8192CU\\PHY_REG_2T.txt"	
			#define RTL8192C_PHY_REG_mCard			"rtl8192CU\\PHY_REG_2T_mCard.txt"				
			#define RTL8192C_PHY_RADIO_A				"rtl8192CU\\radio_a_2T.txt"
			#define RTL8192C_PHY_RADIO_B				"rtl8192CU\\radio_b_2T.txt"	
			#define RTL8192C_AGC_TAB					"rtl8192CU\\AGC_TAB_2T.txt"
			#define RTL8192C_PHY_MACREG					"rtl8192CU\\MAC_REG.txt"

			#define RTL819X_PHY_REG_PG					"rtl8192CU\\PHY_REG_PG.txt"
			#define RTL819X_PHY_REG_MP 					"rtl8192CU\\PHY_REG_MP.txt" 
			#define RTL819X_PHY_REG_PG_HP				"rtl8192CU\\PHY_REG_PG_HP.txt"
			#define RTL819X_PHY_RADIO_A					"rtl8192CU\\radio_a.txt"
			#define RTL819X_PHY_RADIO_B					"rtl8192CU\\radio_b.txt"			
			
//---------------------------------------------------------------------
//		RTL8192DU From file
//---------------------------------------------------------------------

			#define RTL8192D_PHY_REG					"rtl8192DU\\PHY_REG.txt"
			#define RTL8192D_PHY_REG_PG				"rtl8192DU\\PHY_REG_PG.txt"
			#define RTL8192D_PHY_REG_MP				"rtl8192DU\\PHY_REG_MP.txt"			
			
			#define RTL8192D_AGC_TAB					"rtl8192DU\\AGC_TAB.txt"
			#define RTL8192D_AGC_TAB_2G				"rtl8192DU\\AGC_TAB_2G.txt"
			#define RTL8192D_AGC_TAB_5G				"rtl8192DU\\AGC_TAB_5G.txt"
			#define RTL8192D_PHY_RADIO_A				"rtl8192DU\\radio_a.txt"
			#define RTL8192D_PHY_RADIO_B				"rtl8192DU\\radio_b.txt"
			#define RTL8192D_PHY_RADIO_A_intPA			"rtl8192DU\\radio_a_intPA.txt"
			#define RTL8192D_PHY_RADIO_B_intPA			"rtl8192DU\\radio_b_intPA.txt"			
			#define RTL8192D_PHY_MACREG				"rtl8192DU\\MAC_REG.txt"

//---------------------------------------------------------------------
//		RTL8812AU From file
//---------------------------------------------------------------------
			#define RTL8812_FW_IMG						"rtl8812AU\\rtl8812Ufw.bin"
			#define RTL8812_FW_WW_IMG					"rtl8812AU\\rtl8812Ufwww.bin"
			#define RTL8812_FW_BT_IMG						"rtl8812AU\\rtl8812UfwBT.bin"
			#define RTL8812_PHY_REG						"rtl8812AU\\PHY_REG.txt" 
			#define RTL8812_PHY_RADIO_A					"rtl8812AU\\RadioA.txt"
			#define RTL8812_PHY_RADIO_B					"rtl8812AU\\RadioB.txt"
			#define RTL8812_TXPWR_TRACK					"rtl8812AU\\TxPowerTrack.txt"			
			#define RTL8812_AGC_TAB						"rtl8812AU\\AGC_TAB.txt"
			#define RTL8812_PHY_MACREG 					"rtl8812AU\\MAC_REG.txt"
			#define RTL8812_PHY_REG_PG					"rtl8812AU\\PHY_REG_PG.txt"
			#define RTL8812_PHY_REG_MP 					"rtl8812AU\\PHY_REG_MP.txt" 	
			#define RTL8812_TXPWR_LMT					"rtl8812AU\\TXPWR_LMT.txt" 


//---------------------------------------------------------------------
//		RTL8723S From file
//---------------------------------------------------------------------
		#define RTL8723_FW_UMC_IMG					"rtl8723S\\rtl8723fw.bin"
		#define RTL8723_FW_UMC_B_IMG					"rtl8723S\\rtl8723fw_B.bin"
		#define RTL8723_FW_UMC_WW_IMG					"rtl8723S\\rtl8723fwww.bin"
		#define RTL8723_FW_UMC_B_WW_IMG 			"rtl8723S\\rtl8723fw_Bww.bin"
		#define RTL8723_PHY_REG						"rtl8723S\\PHY_REG_1T.txt" 
		#define RTL8723_PHY_RADIO_A					"rtl8723S\\radio_a_1T.txt"
		#define RTL8723_PHY_RADIO_B					"rtl8723S\\radio_b_1T.txt" 
		#define RTL8723_AGC_TAB						"rtl8723S\\AGC_TAB_1T.txt"
		#define RTL8723_PHY_MACREG 					"rtl8723S\\MAC_REG.txt"
		#define RTL8723_PHY_MACREG 					"rtl8723S\\MAC_REG.txt"
		#define RTL8723_PHY_REG_PG						"rtl8723S\\PHY_REG_PG.txt"
		#define RTL8723_PHY_REG_MP						"rtl8723S\\PHY_REG_MP.txt"	

//---------------------------------------------------------------------
//		RTL8188ES From file
//---------------------------------------------------------------------
		#define RTL8188E_FW_T_IMG					"rtl8188E\\rtl8188Efw.bin"
		#define RTL8188E_FW_T_WW_IMG				"rtl8188E\\rtl8188Efwww.bin"
		#define RTL8188E_FW_S_IMG					"rtl8188E\\rtl8188EfwS.bin"
		#define RTL8188E_FW_S_WW_IMG					"rtl8188E\\rtl8188EfwSww.bin"
		#define RTL8188E_PHY_REG						"rtl8188E\\PHY_REG_1T.txt" 
		#define RTL8188E_PHY_RADIO_A					"rtl8188E\\RadioA_1T.txt"
		#define RTL8188E_PHY_RADIO_B					"rtl8188E\\RadioB_1T.txt" 
		#define RTL8188E_AGC_TAB						"rtl8188E\\AGC_TAB_1T.txt"
		#define RTL8188E_PHY_MACREG 					"rtl8188E\\MAC_REG.txt"
		#define RTL8188E_PHY_REG_PG						"rtl8188E\\PHY_REG_PG.txt"
		#define RTL8188E_PHY_REG_MP 					"rtl8188E\\PHY_REG_MP.txt" 	
		#define RTL8188E_TXPWR_LMT 						"rtl8188E\\TXPWR_LMT.txt"
		#define RTL8188E_TXPWR_TRACK					"rtl8188E\\TxPowerTrack.txt"		

//---------------------------------------------------------------------
//		RTL8821S From file
//---------------------------------------------------------------------
		#define RTL8821_FW_IMG						"rtl8821AS\\RTL8821sFW.bin"
		#define RTL8821_FW_WW_IMG					"rtl8821AS\\RTL8821sFWww.bin"
		#define RTL8821_FW_BT_IMG						"rtl8821AS\\rtl8821AfwBT.bin"
		#define RTL8821_PHY_REG						"rtl8821AS\\PHY_REG.txt" 
		#define RTL8821_PHY_RADIO_A					"rtl8821AS\\RadioA.txt"
		#define RTL8821_PHY_RADIO_B					"rtl8821AS\\RadioB.txt" 
		#define RTL8821_TXPWR_TRACK					"rtl8821AS\\TxPowerTrack.txt" 				
		#define RTL8821_AGC_TAB						"rtl8821AS\\AGC_TAB.txt"
		#define RTL8821_PHY_MACREG 					"rtl8821AS\\MAC_REG.txt"
		#define RTL8821_PHY_REG_PG					"rtl8821AS\\PHY_REG_PG.txt"
		#define RTL8821_PHY_REG_MP 					"rtl8821AS\\PHY_REG_MP.txt"  
		#define RTL8821_TXPWR_LMT					"rtl8821AS\\TXPWR_LMT.txt" 

//---------------------------------------------------------------------
//		RTL8192E From file
//---------------------------------------------------------------------
		#define RTL8192E_FW_IMG						"rtl8192ES\\rtl8192ESfw.bin"
		#define RTL8192E_FW_WW_IMG						"rtl8192ES\\rtl8192ESfwww.bin"
		#define RTL8192E_FW_BT_IMG 					"rtl8192EE\\rtl8192ESfwBT.bin"
		#define RTL8192E_PHY_REG						"rtl8192ES\\PHY_REG.txt" 
		#define RTL8192E_PHY_RADIO_A				"rtl8192ES\\RadioA.txt"
		#define RTL8192E_PHY_RADIO_B				"rtl8192ES\\RadioB.txt" 
		#define RTL8192E_AGC_TAB						"rtl8192ES\\AGC_TAB.txt"
		#define RTL8192E_PHY_MACREG 					"rtl8192ES\\MAC_REG.txt"
		#define RTL8192E_PHY_REG_PG					"rtl8192ES\\PHY_REG_PG.txt"
		#define RTL8192E_PHY_REG_MP 					"rtl8192ES\\PHY_REG_MP.txt"
		#define RTL8192_TXPWR_TRACK 				"rtl8192ES\\TxPowerTrack.txt"	
		#define RTL8192E_TXPWR_LMT					"rtl8192ES\\TXPWR_LMT.txt" 		

//---------------------------------------------------------------------
//		RTL8723D From file
//---------------------------------------------------------------------
		#define RTL8723D_FW_IMG						"rtl8723DS\\rtl8723DSfw.bin"
		#define RTL8723D_FW_WW_IMG					"rtl8723DS\\rtl8723DSfwww.bin"
		#define RTL8723D_FW_BT_IMG 					"rtl8723DS\\rtl8723DSfwBT.bin"
		#define RTL8723D_PHY_REG						"rtl8723DS\\PHY_REG.txt" 
		#define RTL8723D_PHY_RADIO_A					"rtl8723DS\\RadioA.txt"
		#define RTL8723D_AGC_TAB						"rtl8723DS\\AGC_TAB.txt"
		#define RTL8723D_PHY_MACREG 					"rtl8723DS\\MAC_REG.txt"
		#define RTL8723D_PHY_REG_PG					"rtl8723DS\\PHY_REG_PG.txt"
		#define RTL8723D_PHY_REG_MP 					"rtl8723DS\\PHY_REG_MP.txt"
		#define RTL8723D_TXPWR_TRACK 					"rtl8723DS\\TxPowerTrack.txt"
		#define RTL8723D_TXPWR_LMT						"rtl8723DS\\TXPWR_LMT.txt" 		

//---------------------------------------------------------------------
//		RTL8814A From file
//---------------------------------------------------------------------
		#define RTL8814A_FW_IMG						"rtl8814as\\rtl8814Afw.bin"
		#define RTL8814A_PHY_REG						"rtl8814as\\PHY_REG.txt" 
		#define RTL8814A_PHY_RADIO_A					"rtl8814as\\RadioA.txt"
		#define RTL8814A_PHY_RADIO_B					"rtl8814as\\RadioB.txt"
		#define RTL8814A_PHY_RADIO_C					"rtl8814as\\RadioC.txt"
		#define RTL8814A_PHY_RADIO_D					"rtl8814as\\RadioD.txt" 
		#define RTL8814A_AGC_TAB						"rtl8814as\\AGC_TAB.txt"
		#define RTL8814A_PHY_MACREG 					"rtl8814as\\MAC_REG.txt"
		#define RTL8814A_PHY_REG_PG					"rtl8814as\\PHY_REG_PG.txt"
		#define RTL8814A_PHY_REG_MP 					"rtl8814as\\PHY_REG_MP.txt"
		#define RTL8814A_TXPWR_TRACK 					"rtl8814as\\TxPowerTrack.txt"
		#define RTL8814A_TXPWR_LMT						"rtl8814as\\TXPWR_LMT.txt" 		

//---------------------------------------------------------------------
//		RTL8821BS From file
//---------------------------------------------------------------------
        #define RTL8821B_FW_IMG						"rtl8821BS\\rtl8821Bfw.bin"
        #define RTL8821B_PHY_REG					"rtl8821BS\\PHY_REG.txt" 
        #define RTL8821B_PHY_RADIO_A				"rtl8821BS\\RadioA.txt"
        #define RTL8821B_TXPWR_TRACK				"rtl8821BS\\TxPowerTrack.txt" 		
        #define RTL8821B_AGC_TAB					"rtl8821BS\\AGC_TAB.txt"
        #define RTL8821B_PHY_MACREG 				"rtl8821BS\\MAC_REG.txt"
        #define RTL8821B_PHY_REG_PG					"rtl8821BS\\PHY_REG_PG.txt"
        #define RTL8821B_PHY_REG_MP 				"rtl8821BS\\PHY_REG_MP.txt" 	
        #define RTL8821B_TXPWR_LMT					"rtl8821BS\\TXPWR_LMT.txt" 		

//---------------------------------------------------------------------
//		RTL8822BS From file
//---------------------------------------------------------------------
        #define RTL8822B_FW_IMG						"rtl8822BS\\rtl8822Bfw.bin"
        #define RTL8822B_PHY_REG					"rtl8822BS\\PHY_REG.txt" 
        #define RTL8822B_PHY_RADIO_A				"rtl8822BS\\RadioA.txt"
		#define RTL8822B_PHY_RADIO_B				"rtl8822BS\\RadioB.txt"
        #define RTL8822B_TXPWR_TRACK				"rtl8822BS\\TxPowerTrack.txt" 		
        #define RTL8822B_AGC_TAB					"rtl8822BS\\AGC_TAB.txt"
        #define RTL8822B_PHY_MACREG 				"rtl8822BS\\MAC_REG.txt"
        #define RTL8822B_PHY_REG_PG					"rtl8822BS\\PHY_REG_PG.txt"
        #define RTL8822B_PHY_REG_MP 				"rtl8822BS\\PHY_REG_MP.txt" 	
        #define RTL8822B_TXPWR_LMT					"rtl8822BS\\TXPWR_LMT.txt" 	

//---------------------------------------------------------------------
//		RTL8723BS From file
//---------------------------------------------------------------------
		#define RTL8723B_FW_IMG						"rtl8723BS\\rtl8723Bfw.bin"
		#define RTL8723B_FW_WW_IMG					"rtl8723BS\\rtl8723Bfwww.bin"
		#define RTL8723B_FW_BT_IMG 					"rtl8723BS\\rtl8723BfwBT.bin"
		#define RTL8723B_PHY_REG						"rtl8723BS\\PHY_REG.txt" 
		#define RTL8723B_PHY_RADIO_A					"rtl8723BS\\RadioA.txt"
		#define RTL8723B_PHY_RADIO_B					"rtl8723BS\\RadioB.txt" 
		#define RTL8723B_TXPWR_TRACK					"rtl8723BS\\TxPowerTrack.txt" 
		#define RTL8723B_AGC_TAB						"rtl8723BS\\AGC_TAB.txt"
		#define RTL8723B_PHY_MACREG 					"rtl8723BS\\MAC_REG.txt"
		#define RTL8723B_PHY_REG_PG					"rtl8723BS\\PHY_REG_PG.txt"
		#define RTL8723B_PHY_REG_MP 					"rtl8723BS\\PHY_REG_MP.txt"
		#define RTL8723B_TXPWR_LMT 					"rtl8723BS\\TXPWR_LMT.txt"

//---------------------------------------------------------------------
//		RTL8703BS From file
//---------------------------------------------------------------------
		#define RTL8703B_FW_IMG						"rtl8703BS\\rtl8703Bfw.bin"
		#define RTL8703B_FW_WW_IMG					"rtl8703BS\\rtl8703Bfwww.bin"
		#define RTL8703B_FW_BT_IMG 					"rtl8703BS\\rtl8703BfwBT.bin"
		#define RTL8703B_PHY_REG					"rtl8703BS\\PHY_REG.txt" 
		#define RTL8703B_PHY_RADIO_A				"rtl8703BS\\RadioA.txt"
		#define RTL8703B_PHY_RADIO_B				"rtl8703BS\\RadioB.txt" 
		#define RTL8703B_TXPWR_TRACK				"rtl8703BS\\TxPowerTrack.txt" 
		#define RTL8703B_AGC_TAB					"rtl8703BS\\AGC_TAB.txt"
		#define RTL8703B_PHY_MACREG 				"rtl8703BS\\MAC_REG.txt"
		#define RTL8703B_PHY_REG_PG					"rtl8703BS\\PHY_REG_PG.txt"
		#define RTL8703B_PHY_REG_MP 				"rtl8703BS\\PHY_REG_MP.txt"
		#define RTL8703B_TXPWR_LMT 					"rtl8703BS\\TXPWR_LMT.txt"		
//---------------------------------------------------------------------
//		RTL8188FS From file
//---------------------------------------------------------------------
		#define RTL8188F_FW_IMG						"rtl8188FS\\rtl8188Ffw.bin"
		#define RTL8188F_FW_WW_IMG					"rtl8188FS\\rtl8188Ffwww.bin"
		#define RTL8188F_FW_BT_IMG 					"rtl8188FS\\rtl8188FfwBT.bin"
		#define RTL8188F_PHY_REG					"rtl8188FS\\PHY_REG.txt" 
		#define RTL8188F_PHY_RADIO_A				"rtl8188FS\\RadioA.txt"
		#define RTL8188F_PHY_RADIO_B				"rtl8188FS\\RadioB.txt" 
		#define RTL8188F_TXPWR_TRACK				"rtl8188FS\\TxPowerTrack.txt" 
		#define RTL8188F_AGC_TAB					"rtl8188FS\\AGC_TAB.txt"
		#define RTL8188F_PHY_MACREG 				"rtl8188FS\\MAC_REG.txt"
		#define RTL8188F_PHY_REG_PG					"rtl8188FS\\PHY_REG_PG.txt"
		#define RTL8188F_PHY_REG_MP 				"rtl8188FS\\PHY_REG_MP.txt"
		#define RTL8188F_TXPWR_LMT 					"rtl8188FS\\TXPWR_LMT.txt"	
//---------------------------------------------------------------------
//		RTL8723S From header
//---------------------------------------------------------------------
		// Fw Array
		#define Rtl8723_FwImageArray				Rtl8723SFwImgArray
		#define Rtl8723_FwUMCBCutImageArray		Rtl8723SFwUMCBCutImgArray
		#define Rtl8723_FwWWImageArray			Rtl8723FwWWImgArray
		#define Rtl8723_FwUMCBWWCutImageArray	Rtl8723FwUMCBCutWWImgArray
		
		// MAC/BB/PHY Array
		#define Rtl8723_MAC_Array					Rtl8723SMAC_2T_Array
		#define Rtl8723_AGCTAB_2TArray				Rtl8723SAGCTAB_2TArray
		#define Rtl8723_AGCTAB_1TArray				Rtl8723SAGCTAB_1TArray
		#define Rtl8723_PHY_REG_2TArray			Rtl8723SPHY_REG_2TArray			
		#define Rtl8723_PHY_REG_1TArray			Rtl8723SPHY_REG_1TArray
		#define Rtl8723_RadioA_2TArray				Rtl8723SRadioA_2TArray
		#define Rtl8723_RadioA_1TArray				Rtl8723SRadioA_1TArray
		#define Rtl8723_RadioB_2TArray				Rtl8723SRadioB_2TArray
		#define Rtl8723_RadioB_1TArray				Rtl8723SRadioB_1TArray
		#define Rtl8723_PHY_REG_Array_PG 			Rtl8723SPHY_REG_Array_PG
		#define Rtl8723_PHY_REG_Array_MP 			Rtl8723SPHY_REG_Array_MP
		
		// Array length
		#define Rtl8723_ImgArrayLength				Rtl8723SImgArrayLength
		#define Rtl8723_UMCBCutImgArrayLength		Rtl8723SUMCBCutImgArrayLength
		#define Rtl8723_WWImgArrayLength				Rtl8723WWImgArrayLength
		#define Rtl8723_UMCBCutWWImgArrayLength 	Rtl8723UMCBCutWWImgArrayLength
		#define Rtl8723_MAC_ArrayLength				Rtl8723SMAC_2T_ArrayLength
		#define Rtl8723_AGCTAB_1TArrayLength		Rtl8723SAGCTAB_1TArrayLength
		#define Rtl8723_PHY_REG_1TArrayLength 		Rtl8723SPHY_REG_1TArrayLength
		#define Rtl8723_PHY_REG_Array_MPLength		Rtl8723SPHY_REG_Array_MPLength
		#define Rtl8723_PHY_REG_Array_PGLength		Rtl8723SPHY_REG_Array_PGLength
		#define Rtl8723_RadioA_1TArrayLength		Rtl8723SRadioA_1TArrayLength
		#define Rtl8723_RadioB_1TArrayLength		Rtl8723SRadioB_1TArrayLength

		//---------------------------------------------------------------------
		//		RTL8192ES From header
		//---------------------------------------------------------------------
		// Fw Array
		#define Rtl8192E_FwImageArray				Rtl8192ESFwImgArray
		#define Rtl8192E_FwWWImageArray 			Rtl8192EFwWWImgArray
				
		// MAC/BB/PHY Array
		#define Rtl8192E_MAC_Array					Rtl8192ESMAC_1T_Array
		#define Rtl8192E_PHY_REG_1TArray			Rtl8192ESPHY_REG_1TArray
		#define Rtl8192E_AGCTAB_1TArray				Rtl8192ESAGCTAB_1TArray
		#define Rtl8192E_RadioA_1TArray				Rtl8192ESRadioA_1TArray
		#define Rtl8192E_PHY_REG_Array_PG			Rtl8192ESPHY_REG_Array_PG
		#define Rtl8192E_PHY_REG_Array_MP 			Rtl8192ESPHY_REG_Array_MP
				
		// Array length
		#define Rtl8192E_ImgArrayLength				Rtl8192ESImgArrayLength
		#define Rtl8192E_MAC_ArrayLength			Rtl8192ESMAC_1T_ArrayLength
		#define Rtl8192E_WWImgArrayLength			Rtl8192EWWImgArrayLength
		#define Rtl8192E_PHY_REG_1TArrayLength 		Rtl8192ESPHY_REG_1TArrayLength
		#define Rtl8192E_AGCTAB_1TArrayLength		Rtl8192ESAGCTAB_1TArrayLength
		#define Rtl8192E_RadioA_1TArrayLength		Rtl8192ESRadioA_1TArrayLength
		#define Rtl8192E_PHY_REG_Array_PGLength	Rtl8192ESPHY_REG_Array_PGLength
		#define Rtl8192E_PHY_REG_Array_MPLength	Rtl8192ESPHY_REG_Array_MPLength





// ---------------------------------------------------------------------      
//		RTL8723 Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8723_NIC_PWR_ON_FLOW				rtl8723A_power_on_flow
#define Rtl8723_NIC_RF_OFF_FLOW				rtl8723A_radio_off_flow
#define Rtl8723_NIC_DISABLE_FLOW				rtl8723A_card_disable_flow
#define Rtl8723_NIC_ENABLE_FLOW				rtl8723A_card_enable_flow
#define Rtl8723_NIC_SUSPEND_FLOW				rtl8723A_suspend_flow
#define Rtl8723_NIC_RESUME_FLOW				rtl8723A_resume_flow
#define Rtl8723_NIC_PDN_FLOW					rtl8723A_hwpdn_flow
#define Rtl8723_NIC_LPS_ENTER_FLOW			rtl8723A_enter_lps_flow
#define Rtl8723_NIC_LPS_LEAVE_FLOW				rtl8723A_leave_lps_flow

//---------------------------------------------------------------------
//		RTL8188E Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8188E_NIC_PWR_ON_FLOW				rtl8188E_power_on_flow
#define Rtl8188E_NIC_RF_OFF_FLOW				rtl8188E_radio_off_flow
#define Rtl8188E_NIC_DISABLE_FLOW				rtl8188E_card_disable_flow
#define Rtl8188E_NIC_ENABLE_FLOW				rtl8188E_card_enable_flow
#define Rtl8188E_NIC_SUSPEND_FLOW				rtl8188E_suspend_flow
#define Rtl8188E_NIC_RESUME_FLOW				rtl8188E_resume_flow
#define Rtl8188E_NIC_PDN_FLOW					rtl8188E_hwpdn_flow
#define Rtl8188E_NIC_LPS_ENTER_FLOW			rtl8188E_enter_lps_flow
#define Rtl8188E_NIC_LPS_LEAVE_FLOW			rtl8188E_leave_lps_flow

//---------------------------------------------------------------------
//		RTL8812 Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8812_NIC_PWR_ON_FLOW				rtl8812_power_on_flow
#define Rtl8812_NIC_RF_OFF_FLOW				rtl8812_radio_off_flow
#define Rtl8812_NIC_DISABLE_FLOW				rtl8812_card_disable_flow
#define Rtl8812_NIC_ENABLE_FLOW				rtl8812_card_enable_flow
#define Rtl8812_NIC_SUSPEND_FLOW				rtl8812_suspend_flow
#define Rtl8812_NIC_RESUME_FLOW				rtl8812_resume_flow
#define Rtl8812_NIC_PDN_FLOW					rtl8812_hwpdn_flow
#define Rtl8812_NIC_LPS_ENTER_FLOW			      rtl8812_enter_lps_flow
#define Rtl8812_NIC_LPS_LEAVE_FLOW				rtl8812_leave_lps_flow		

//---------------------------------------------------------------------
//		RTL8821 Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8821A_NIC_PWR_ON_FLOW				rtl8821A_power_on_flow
#define Rtl8821A_NIC_RF_OFF_FLOW				rtl8821A_radio_off_flow
#define Rtl8821A_NIC_DISABLE_FLOW				rtl8821A_card_disable_flow
#define Rtl8821A_NIC_ENABLE_FLOW				rtl8821A_card_enable_flow
#define Rtl8821A_NIC_SUSPEND_FLOW				rtl8821A_suspend_flow
#define Rtl8821A_NIC_RESUME_FLOW				rtl8821A_resume_flow
#define Rtl8821A_NIC_PDN_FLOW					rtl8821A_hwpdn_flow
#define Rtl8821A_NIC_LPS_ENTER_FLOW			rtl8821A_enter_lps_flow
#define Rtl8821A_NIC_LPS_LEAVE_FLOW			rtl8821A_leave_lps_flow		

//---------------------------------------------------------------------
//		RTL8821B Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8821B_NIC_PWR_ON_FLOW				rtl8821B_power_on_flow
#define Rtl8821B_NIC_RF_OFF_FLOW				rtl8821B_radio_off_flow
#define Rtl8821B_NIC_DISABLE_FLOW				rtl8821B_card_disable_flow
#define Rtl8821B_NIC_ENABLE_FLOW				rtl8821B_card_enable_flow
#define Rtl8821B_NIC_SUSPEND_FLOW				rtl8821B_suspend_flow
#define Rtl8821B_NIC_RESUME_FLOW				rtl8821B_resume_flow
#define Rtl8821B_NIC_PDN_FLOW					rtl8821B_hwpdn_flow
#define Rtl8821B_NIC_LPS_ENTER_FLOW			rtl8821B_enter_lps_flow
#define Rtl8821B_NIC_LPS_LEAVE_FLOW			rtl8821B_leave_lps_flow		

//---------------------------------------------------------------------
//		RTL8822B Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8822B_NIC_PWR_ON_FLOW			rtl8822B_power_on_flow
#define Rtl8822B_NIC_RF_OFF_FLOW			rtl8822B_radio_off_flow
#define Rtl8822B_NIC_DISABLE_FLOW			rtl8822B_card_disable_flow
#define Rtl8822B_NIC_ENABLE_FLOW			rtl8822B_card_enable_flow
#define Rtl8822B_NIC_SUSPEND_FLOW			rtl8822B_suspend_flow
#define Rtl8822B_NIC_RESUME_FLOW			rtl8822B_resume_flow
#define Rtl8822B_NIC_PDN_FLOW				rtl8822B_hwpdn_flow
#define Rtl8822B_NIC_LPS_ENTER_FLOW			rtl8822B_enter_lps_flow
#define Rtl8822B_NIC_LPS_LEAVE_FLOW			rtl8822B_leave_lps_flow		


//---------------------------------------------------------------------
//		RTL8192E Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8192E_NIC_PWR_ON_FLOW				rtl8192E_power_on_flow
#define Rtl8192E_NIC_RF_OFF_FLOW				rtl8192E_radio_off_flow
#define Rtl8192E_NIC_DISABLE_FLOW				rtl8192E_card_disable_flow
#define Rtl8192E_NIC_ENABLE_FLOW				rtl8192E_card_enable_flow
#define Rtl8192E_NIC_SUSPEND_FLOW				rtl8192E_suspend_flow
#define Rtl8192E_NIC_RESUME_FLOW				rtl8192E_resume_flow
#define Rtl8192E_NIC_PDN_FLOW					rtl8192E_hwpdn_flow
#define Rtl8192E_NIC_LPS_ENTER_FLOW			rtl8192E_enter_lps_flow
#define Rtl8192E_NIC_LPS_LEAVE_FLOW			rtl8192E_leave_lps_flow	

//---------------------------------------------------------------------
//		RTL8814A Power Configuration CMDs for PCIe interface
//---------------------------------------------------------------------
#define Rtl8814A_NIC_PWR_ON_FLOW				rtl8814A_power_on_flow
#define Rtl8814A_NIC_RF_OFF_FLOW				rtl8814A_radio_off_flow
#define Rtl8814A_NIC_DISABLE_FLOW				rtl8814A_card_disable_flow
#define Rtl8814A_NIC_ENABLE_FLOW				rtl8814A_card_enable_flow
#define Rtl8814A_NIC_SUSPEND_FLOW				rtl8814A_suspend_flow
#define Rtl8814A_NIC_RESUME_FLOW				rtl8814A_resume_flow
#define Rtl8814A_NIC_PDN_FLOW					rtl8814A_hwpdn_flow
#define Rtl8814A_NIC_LPS_ENTER_FLOW			rtl8814A_enter_lps_flow
#define Rtl8814A_NIC_LPS_LEAVE_FLOW			rtl8814A_leave_lps_flow	

//---------------------------------------------------------------------
//		RTL8723B Power Configuration CMDs
//---------------------------------------------------------------------
#define Rtl8723B_NIC_PWR_ON_FLOW				rtl8723B_power_on_flow
#define Rtl8723B_NIC_RF_OFF_FLOW				rtl8723B_radio_off_flow
#define Rtl8723B_NIC_DISABLE_FLOW				rtl8723B_card_disable_flow
#define Rtl8723B_NIC_ENABLE_FLOW				rtl8723B_card_enable_flow
#define Rtl8723B_NIC_SUSPEND_FLOW				rtl8723B_suspend_flow
#define Rtl8723B_NIC_RESUME_FLOW				rtl8723B_resume_flow
#define Rtl8723B_NIC_PDN_FLOW					rtl8723B_hwpdn_flow
#define Rtl8723B_NIC_LPS_ENTER_FLOW			rtl8723B_enter_lps_flow
#define Rtl8723B_NIC_LPS_LEAVE_FLOW			rtl8723B_leave_lps_flow		

//---------------------------------------------------------------------
//		RTL8703B Power Configuration CMDs
//---------------------------------------------------------------------
#define Rtl8703B_NIC_PWR_ON_FLOW				rtl8703B_power_on_flow
#define Rtl8703B_NIC_RF_OFF_FLOW				rtl8703B_radio_off_flow
#define Rtl8703B_NIC_DISABLE_FLOW				rtl8703B_card_disable_flow
#define Rtl8703B_NIC_ENABLE_FLOW				rtl8703B_card_enable_flow
#define Rtl8703B_NIC_SUSPEND_FLOW				rtl8703B_suspend_flow
#define Rtl8703B_NIC_RESUME_FLOW				rtl8703B_resume_flow
#define Rtl8703B_NIC_PDN_FLOW					rtl8703B_hwpdn_flow
#define Rtl8703B_NIC_LPS_ENTER_FLOW				rtl8703B_enter_lps_flow
#define Rtl8703B_NIC_LPS_LEAVE_FLOW				rtl8703B_leave_lps_flow
//---------------------------------------------------------------------
//		RTL8188F Power Configuration CMDs
//---------------------------------------------------------------------
#define Rtl8188F_NIC_PWR_ON_FLOW				rtl8188F_power_on_flow
#define Rtl8188F_NIC_RF_OFF_FLOW				rtl8188F_radio_off_flow
#define Rtl8188F_NIC_DISABLE_FLOW				rtl8188F_card_disable_flow
#define Rtl8188F_NIC_ENABLE_FLOW				rtl8188F_card_enable_flow
#define Rtl8188F_NIC_SUSPEND_FLOW				rtl8188F_suspend_flow
#define Rtl8188F_NIC_RESUME_FLOW				rtl8188F_resume_flow
#define Rtl8188F_NIC_PDN_FLOW					rtl8188F_hwpdn_flow
#define Rtl8188F_NIC_LPS_ENTER_FLOW				rtl8188F_enter_lps_flow
#define Rtl8188F_NIC_LPS_LEAVE_FLOW				rtl8188F_leave_lps_flow

//---------------------------------------------------------------------
//		RTL8723D Power Configuration CMDs
//---------------------------------------------------------------------
#define Rtl8723D_NIC_PWR_ON_FLOW				rtl8723D_power_on_flow
#define Rtl8723D_NIC_RF_OFF_FLOW				rtl8723D_radio_off_flow
#define Rtl8723D_NIC_DISABLE_FLOW				rtl8723D_card_disable_flow
#define Rtl8723D_NIC_ENABLE_FLOW				rtl8723D_card_enable_flow
#define Rtl8723D_NIC_SUSPEND_FLOW				rtl8723D_suspend_flow
#define Rtl8723D_NIC_RESUME_FLOW				rtl8723D_resume_flow
#define Rtl8723D_NIC_PDN_FLOW					rtl8723D_hwpdn_flow
#define Rtl8723D_NIC_LPS_ENTER_FLOW			rtl8723D_enter_lps_flow
#define Rtl8723D_NIC_LPS_LEAVE_FLOW			rtl8723D_leave_lps_flow		


typedef enum _FA_CNT_TYPE{
	FA_CNT_OFDM_CCA,
	FA_CNT_CCK_CCA,
	FA_CNT_OFDM_FAIL,
	FA_CNT_CCK_FAIL,
	FA_CNT_TYPE_MAX
}FA_CNT_TYPE;

//
// Forward declaration. 
//
typedef struct _ADAPTER	ADAPTER, *PADAPTER;
typedef struct _RT_RFD	RT_RFD, *PRT_RFD;
typedef struct _RT_TCB 	RT_TCB, *PRT_TCB;
typedef struct _RT_TCB_STATUS	RT_TCB_STATUS, *PRT_TCB_STATUS;
typedef struct _RT_RFD_STATUS	RT_RFD_STATUS, *PRT_RFD_STATUS;
typedef struct _USB_IN_CONTEXT  USB_IN_CONTEXT, *PUSB_IN_CONTEXT;
typedef struct _USB_OUT_CONTEXT USB_OUT_CONTEXT, *PUSB_OUT_CONTEXT;
typedef struct _SDIO_IN_CONTEXT SDIO_IN_CONTEXT, *PSDIO_IN_CONTEXT;
typedef struct _SDIO_OUT_CONTEXT SDIO_OUT_CONTEXT, *PSDIO_OUT_CONTEXT;
typedef struct _RT_WLAN_STA RT_WLAN_STA, *PRT_WLAN_STA;
typedef struct DM_Out_Source_Dynamic_Mechanism_Structure	DM_ODM_T, *PDM_ODM_T;

typedef enum _HARDWARE_TYPE{
	HARDWARE_TYPE_RTL8192E,
	HARDWARE_TYPE_RTL819xU,
	HARDWARE_TYPE_RTL8192SE,
	HARDWARE_TYPE_RTL8192SU,
	HARDWARE_TYPE_RTL8192CE,
	HARDWARE_TYPE_RTL8192CU,
	HARDWARE_TYPE_RTL8192DE,
	HARDWARE_TYPE_RTL8192DU,
	HARDWARE_TYPE_RTL8723AE,
	HARDWARE_TYPE_RTL8723AU,
	HARDWARE_TYPE_RTL8723AS,
	HARDWARE_TYPE_RTL8723BE,
	HARDWARE_TYPE_RTL8723BU,
	HARDWARE_TYPE_RTL8723BS,
	HARDWARE_TYPE_RTL8188EE,
	HARDWARE_TYPE_RTL8188EU,
	HARDWARE_TYPE_RTL8188ES,
	HARDWARE_TYPE_RTL8192EE,
	HARDWARE_TYPE_RTL8192EU,
	HARDWARE_TYPE_RTL8192ES,
	HARDWARE_TYPE_RTL8814AE,
	HARDWARE_TYPE_RTL8814AU,
	HARDWARE_TYPE_RTL8814AS,	
	HARDWARE_TYPE_RTL8812E,
	HARDWARE_TYPE_RTL8812AU,
	HARDWARE_TYPE_RTL8811AU,	
	HARDWARE_TYPE_RTL8821E,
	HARDWARE_TYPE_RTL8821U,
	HARDWARE_TYPE_RTL8821S,
	HARDWARE_TYPE_RTL8821BE,
	HARDWARE_TYPE_RTL8821BU,
	HARDWARE_TYPE_RTL8821BS,
	HARDWARE_TYPE_RTL8822BE,
	HARDWARE_TYPE_RTL8822BU,
	HARDWARE_TYPE_RTL8822BS,
	HARDWARE_TYPE_RTL8703BE,
	HARDWARE_TYPE_RTL8703BU,
	HARDWARE_TYPE_RTL8703BS,
	HARDWARE_TYPE_RTL8188FE,
	HARDWARE_TYPE_RTL8188FU,
	HARDWARE_TYPE_RTL8188FS,
	HARDWARE_TYPE_RTL8723DE,
	HARDWARE_TYPE_RTL8723DU,
	HARDWARE_TYPE_RTL8723DS,

	HARDWARE_TYPE_MAX,
}HARDWARE_TYPE;

typedef	enum _RT_BUS_TYPE{
	BUS_TYPE_CARDBUS,
	BUS_TYPE_MINIPCI,
	BUS_TYPE_PCI,
}RT_BUS_TYPE,*PRT_BUS_TYPE;

typedef	enum _RT_EEPROM_TYPE{
	EEPROM_93C46,
	EEPROM_93C56,
	EEPROM_BOOT_EFUSE,
}RT_EEPROM_TYPE,*PRT_EEPROM_TYPE;

typedef	enum _LED_CTL_MODE{
	LED_CTL_POWER_ON = 1,
	LED_CTL_LINK = 2,
	LED_CTL_NO_LINK = 3,
	LED_CTL_TX = 4,
	LED_CTL_RX = 5,
	LED_CTL_SITE_SURVEY = 6,
	LED_CTL_POWER_OFF = 7,
	LED_CTL_START_TO_LINK = 8,
	LED_CTL_START_WPS = 9,
	LED_CTL_STOP_WPS = 10,
	LED_CTL_START_WPS_BOTTON = 11, 		//added for runtop
	LED_CTL_STOP_WPS_FAIL = 12, 		//added for ALPHA	
	LED_CTL_STOP_WPS_FAIL_OVERLAP = 13, //added for BELKIN	
	LED_CTL_CONNECTION_NO_TRANSFER = 14,
	LED_CTL_OFF_BY_BUTTON = 15,   //added for ALPHA
}LED_CTL_MODE;



typedef enum _HW_VARIABLES{
	HW_VAR_ETHER_ADDR,
	HW_VAR_MULTICAST_REG,		
	HW_VAR_BASIC_RATE,
	HW_VAR_BSSID,					// HW Port 0 BSSID
	HW_VAR_BSSID1,				// HW Port 1 BSSID
	HW_VAR_MEDIA_STATUS,
	HW_VAR_BEACON_INTERVAL,
	HW_VAR_ATIM_WINDOW,	
	HW_VAR_SIFS,
	HW_VAR_SLOT_TIME,
	HW_VAR_ACK_PREAMBLE,
	HW_VAR_COMMAND,				// For Command Register, Annie, 2006-04-07.
	HW_VAR_WPA_CONFIG,			//2004/08/23, kcwu, for 8187 Security config
	HW_VAR_AMPDU_MIN_SPACE,		// The spacing between sub-frame. Roger, 2008.07.04.
	HW_VAR_AMPDU_FACTOR,
	HW_VAR_AC_PARAM,			// For AC Parameters, 2005.12.01, by rcnjko.
	HW_VAR_ACM_CTRL,			// For ACM Control, Annie, 2005-12-13.
	HW_VAR_CCX_CHNL_LOAD,		// For CCX 2 channel load request, 2006.05.04.
	HW_VAR_CCX_NOISE_HISTOGRAM,	// For CCX 2 noise histogram request, 2006.05.04.
	HW_VAR_CCX_CLM_NHM,			// For CCX 2 parallel channel load request and noise histogram request, 2006.05.12.
	HW_VAR_TURBO_MODE,			// For turbo mode related settings, added by Roger, 2006.12.15.
	HW_VAR_RF_STATE, 			// For change or query RF power state, 061214, rcnjko.
	HW_VAR_RF_OFF_BY_HW,		// For UI to query if external HW signal disable RF, 061229, rcnjko. 
	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//1Attention Please!!!<11n or 8190 specific code should be put below this line>
	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	HW_VAR_RCR,					//for RCR, David 2006,05,11
	HW_VAR_RRSR,
	HW_VAR_CHECK_BSSID,
	HW_VAR_RETRY_LIMIT,
	HW_VAR_INIT_TX_RATE,  		//Get Current Tx rate register. 2008.12.10. Added by tynli
	HW_VAR_RF_2R_DISABLE,
	HW_VAR_SET_RPWM,
	HW_VAR_FILL_A2ENTRY,
	HW_VAR_PS_PARTIAL_OFF,
	HW_VAR_PS_WOWLAN_ENABLE_PATIAL_OFF,
	HW_VAR_H2C_FW_PWRMODE, 		//by tynli. For setting FW related H2C cmd structure. 2009.2.18
	HW_VAR_H2C_FW_JOINBSSRPT, 	//by tynli. For setting FW related H2C cmd structure. 2009.2.18
	HW_VAR_H2C_FW_MEDIASTATUSRPT,
	HW_VAR_STOP_SEND_BEACON,
	HW_VAR_TSF_TIMER,				// Read from TSF register to get the current TSF timer, by Bruce, 2009-07-22.
	HW_VAR_TSF_TIMER1,			// Port 1 TSF Timer Register
	HW_VAR_TSFTR_SNC_OFFSET, 	// Control the offset between TSF and TSF1 Register when doing synchronization (signed offset)
	HW_VAR_IO_CMD,
	HW_VAR_H2C_FW_UPDATE_GTK, // For FW update GTK during S3/S4 for WoWLAN. 2009.06.10. by tynli.
	HW_VAR_WF_MASK,			//For WoWLAN wake up frame bit mask. 2009.06.18. by tynli.
	HW_VAR_WF_CRC,				//For WoWLAN wake up frame CRC value. 2009.06.18. by tynli.
	HW_VAR_WF_IS_MAC_ADDR, 		//For WoWLAN wake up frame content which is the same as our MAC addr. 2009.06.18. by tynli.
	HW_VAR_H2C_FW_OFFLOAD, 		//For WoWLAN. Offload to Fw by H2C cmd. 2009.07.31. by tynli.
	HW_VAR_RESET_WFCRC, 		//For WoWLAN. 2009.08.05. by tynli.
	HW_VAR_HANDLE_FW_C2H,		//Added by tynli. For handling FW C2H command. 2009.10.07.
	HW_VAR_AID,					//Added by tynli.
	HW_VAR_CORRECT_TSF,			//Added by tynli. 2009.10.22. For Hw count TBTT time.
	HW_VAR_FWLPS_RF_ON,			//Added by tynli. 2009.11.09. For checking if Fw finishs RF on sequence.
	HW_VAR_DUAL_TSF_RST,		//Added by tynli. 2009.12.03. Suggested by TimChen.
	HW_VAR_TSFR_SYNC_EN,			// Synchronize HW-Port 0 TSF to HW-Port 1 TSF 
	HW_VAR_DUAL_SWITCH_BAND,
	HW_VAR_INT_MIGRATION, //Added by Roger, 2010.03.05.
 	HW_VAR_RX_AGGR_USBTH,
	HW_VAR_RX_AGGR_USBTO,
	HW_VAR_RX_AGGR_PGTH,
	HW_VAR_RX_AGGR_PGTO,
	HW_VAR_FW_PSMODE_STATUS,		//Added by tynli. 2010.04.19.
	HW_VAR_P2P_PS_MODE,				// Inform the P2P power save to the HW/FW.
	HW_VAR_P2P_CLIENT_PS_STATE,		// Update the P2P client ps state to FW.
	HW_VAR_SETUP_PS_TIMER,			// Set PsTimer
	HW_VAR_PS_TIMEOUT,				// PsTimer Interrupt.
	HW_VAR_BW40MHZ_EXTCHNL,
	HW_VAR_FPGA_SUSPEND_STATUS,	// For RTL8723 FPGA Host suspend/resume verification, added by Roger, 2010.09.27.
	HW_VAR_MSDU_LIFE_TIME,			// MSDU Life Time to drop the packets out of life.
	HW_VAR_INIT_RTS_RATE,				//Added by tynli. 2010.11.16.
	// 2011/01/11 MH Add for 819xp compatiable.
	HW_VAR_RF_TIMING,					// Temporary for RF timing. This shall be removed after RF team verified OK.
	HW_VAR_APFM_ON_MAC,	//Auto FSM to Turn On, include clock, isolation, power control for MAC only
	HW_VAR_H2C_WLAN_INFO,	//by tynli. For setting FW related H2C cmd structure. 2011.2.10
	HW_VAR_VID,
	HW_VAR_PID,
	HW_VAR_AGGR_TIMEOUT,	// For AMPDU per station on vwifi, 2009.11.18 by hpfan
	HW_VAR_AGGR_LIMIT,		// For AMPDU per station on vwifi, 2009.11.18 by hpfan
	HW_VAR_INIT_TX_RATE_MACID,	// Add by hpfan. 2010.01.05 Get current tx rate for specified macid.
	HW_VAR_DIS_SW_BCN,			// Added by tynli. disable Sw DMA BCN
	HW_VAR_MACPHY_MODE,
	// The valid upper nav range for the HW updating, if the true value is larger than the upper range, the HW won't update it.
	// Unit in microsecond. 0 means disable this function.
	HW_VAR_NAV_UPPER,
	HW_VAR_RF_STATUS_INT, // HW RF On/Off interrupt status
	HW_VAR_FW_PS_STATE,	// For Fw LPS 32k. by tynli. 2011.02.25.
	HW_VAR_RESUME_CLK_ON,		// For Fw LPS 32k. by tynli.
	HW_VAR_FW_LPS_ENTER,
	HW_VAR_FW_LPS_LEAVE,
	HW_VAR_PRE_RPWM,
	HW_VAR_FW_CLK_CHANGE_STATE, // For Fw LPS 32k. by tynli.
	HW_VAR_FW_IPS_LEAVE,	// For Fw inactive PS. by tynli.
	HW_VAR_CHECK_NIC_UNPLUG,	// Read the ISR register and check if the NIC is unplugged.
	HW_VAR_HW_REG_TIMER_INIT,
	HW_VAR_HW_REG_TIMER_RESTART,
	HW_VAR_HW_REG_TIMER_START,
	HW_VAR_HW_REG_TIMER_STOP,
	HW_VAR_TCP_CHECKSUM_OFFLOAD,   //  For TCP checksunm En/Disable , Wirte only ( Read used  HW_VAR_RCR);
	HW_VAR_TX_RPT_MAX_MACID,
	HW_VAR_TX_PAUSE,		// For pause Tx
	HW_VAR_BW80MHZ_EXTCHNL,
	HW_VAR_PNP_FROM_NDIS_USB_SS,	// Check whether this PnP event was from NDIS USB Selective Suspend, 2012.05.16. Added By Roger, 2012.05.11
	HW_VAR_SUPPORT_USB3,
	HW_VAR_USB_MODE,
	HW_VAR_AMPDU_MAX_TIME,
	HW_VAR_AMPDU_MAX_AGG_NUM_DYNAMIC,	
	HW_VAR_BATCH_INDICATE_ENABLE,	
	HW_VAR_JAGUAR_PATCH,
	HW_VAR_ANTENNA_DETECTED_INFO, // Support for antenna detection info from each IC, added by Roger, 2012.11.27.
	HW_VAR_NQOS_SEQ_NUM, // Hw sequence number for non-Qos data and mgnt frame
	HW_VAR_R2T_SIFS,		// R2T SIFS
	HW_VAR_BT_FUNC_DISABLE, // Disable BT function, added by Roger, 2013.03.19.
	HW_VAR_ANT_SWITCH,
	HW_VAR_BOOST_INIT_GAIN,
	HW_VAR_FW_H2C_FCS_LOC,			// For FW support fast channel switch download rsvd page
	HW_VAR_FW_H2C_FCS_INFO_SET,	// For FW support fast channel switch set channel information
	HW_VAR_FW_H2C_FCS_INFO_STOP,	// For FW support fast channel switch stop switch
	HW_VAR_TIMER0,
	HW_VAR_MACID_PKT_SLEEP,
	HW_VAR_BOOST_INIT_GAIN_OS,
	HW_VAR_CUR_CENTER_CHNL,	// Current HW center channel
	HW_VAR_MAX_Q_PAGE_NUM,
	HW_VAR_AVBL_Q_PAGE_NUM,
	HW_VAR_DBI,
	HW_VAR_DBI_4,
	HW_VAR_MDIO,
#if (AUTO_CHNL_SEL_NHM == 1)
	HW_VAR_AUTO_CHNL_SEL,	// Auto Channel Selection, added by Roger, 2014.05.15.
#endif
	//--------------- START ------------------
	/* AMSDU related, haiso_ho, 2014.07.09 */
	//----------------------------------------
	HW_VAR_AMSDU_SUPPORT_PARTIALCOLAESCE,
	HW_VAR_AMSDU_NO_PADDING,
	HW_VAR_AMSDU_NEED_PREAGG_CHK,
	HW_VAR_AMSDU_GET_SEG_NUM,
	HW_VAR_AMSDU_GET_TXDESC_EXT_BUFFER_NUM,
	HW_VAR_AMSDU_TEST_SETTING,
	//---------------- END -------------------
	/* AMSDU related, haiso_ho, 2014.07.09 */
	//----------------------------------------

	//--------------- START ------------------
	/* R/W ptr related, hsiao_ho, 2014.07.10 */
	//----------------------------------------
	HW_VAR_RWPTR_SET_TXBUFF_DESC_OWN,
	HW_VAR_RWPTR_SET_RXQ_TXBD_IDX,
	HW_VAR_RWPTR_GET_RXQ_TXBD_IDX,
	HW_VAR_RWPTR_GET_RX_DESC_NUM,
	HW_VAR_RWPTR_RX_INTERRUPT,
	//---------------- END -------------------
	/* R/W ptr related, hsiao_ho, 2014.07.10 */
	//----------------------------------------

	//--------------- START ------------------
	/* WMM related, hsiao_ho, 2014.07.10 */
	//----------------------------------------
	HW_VAR_WMM_NEED_FIX_PARAM,
	//---------------- END -------------------
	/* WMM ptr related, hsiao_ho, 2014.07.10 */
	//----------------------------------------

	//--------------- START ------------------
	/* Early mode related, hsiao_ho, 2014.07.10 */
	//----------------------------------------
	HW_VAR_EARLY_MODE_SUPPORT,
	HW_VAR_EARLY_MODE_THRESHOLD,
	//---------------- END -------------------
	/* Early mode related, hsiao_ho, 2014.07.10 */
	//----------------------------------------

	//--------------- START ------------------
	/* MISC, hsiao_ho, 2014.07.11 */
	//----------------------------------------
	HW_VAR_CHK_JOINACTION,
	//---------------- END -------------------
	/* MISC, hsiao_ho, 2014.07.11 */
	//----------------------------------------

	/* for some IC need to set 4 TxAGC value at one times, suggest by Stanley, 2015.05.25 */
	HW_VAR_TXAGC_NEED_SET_4_RATE_AT_ONCE,
	
	HW_VAR_FCS_NOA,
	HW_VAR_FCS_ADJUST_TSF,
	
	//--------------- START ------------------	
	/*Avoid Rx DPC watchdog violation*/
	HW_VAR_RWPTR_RX_INTERRUPT_LOOP_BREAK,	
	HW_VAR_AVOID_RX_DPC_WATCHDOG_VIOLATION,
	//---------------- END -------------------
	HW_VAR_TX_COMPLETE_LATER_SUPPORT,

	//--------------- START ------------------	
	// MAC Address Randomization 
	// True  : Support MAC Address Randomization and DeviceAddress ( Support  4 or more MAC register)  
	// Fslse : Just support One of MAC Address Randomization and DeviceAddress  ( Just Support 2 MAC register )
	HW_VAR_MAC_ADRESS_COX_CAP,   // Just For Query !!
	// Write New MAC Address
	HW_VAR_MAC_ADDR_RANDOM    ,  // Just For Set !!
	//---------------- END -------------------

#if INTEL_RTD3_SUPPORT
	HW_VAR_RTD3_SUPPORT,	
#endif

}HW_VARIABLES;

// Defaine the interrupt of AP_IBSS mode
typedef enum _HAL_AP_IBSS_INT_MODE
{
	HAL_AP_IBSS_INT_DISABLE = 0,	// Disable AP/IBSS
	HAL_AP_IBSS_INT_AP = 1,			// Enable AP mode
	HAL_AP_IBSS_INT_IBSS = 2,		// Eanble IBSS mode
	HAL_AP_IBSS_INT_AP_IBSS = 3,	// Enbale both of AP and IBSS mode
}HAL_AP_IBSS_INT_MODE, *PHAL_AP_IBSS_INT_MODE;

typedef enum _HAL_INT_MASK_TYPE
{
	HAL_INT_MASK_CURRENT = 0,
	HAL_INT_MASK_REG = 1,
	HAL_INT_MASK_DEFAULT = 2,
}HAL_INT_MASK_TYPE, *PHAL_INT_MASK_TYPE;

typedef enum _HAL_HW_TIMER_TYPE
{
	HAL_TIMER_NONE = 0,
	HAL_TIMER_TXBF = 1,
	HAL_TIMER_EARLYMODE = 2,
}HAL_HW_TIMER_TYPE, *PHAL_HW_TIMER_TYPE;


typedef enum _HAL_DEF_VARIABLE{
	HAL_DEF_LED,					// Led, 0: disable, 1: enable. 061010, by rcnjko. 
	HAL_DEF_WOWLAN,
	HAL_DEF_ENDPOINTS,				//number of endpoints
	HAL_DEF_MIN_TX_POWER_DBM,		// The min supported Tx power in dBm.
	HAL_DEF_MAX_TX_POWER_DBM,		// The max supported Tx power in dBm.
	HAL_DEF_EFUSE_REPG_SECTION1_FLAG,// Efuse Section1 re-programming flag, added by Roger, 2009.02.19.
	HAL_DEF_EFUSE_REPG_DATA, 		// Efuse Section1 re-programming flag, added by Roger, 2009.02.19.
	HAL_DEF_PCI_SUUPORT_L1_BACKDOOR, // Determine if the L1 Backdoor setting is turned on.
	HAL_DEF_PCI_AMD_L1_SUPPORT, // Determine if the L1 Backdoor setting can be turned on for AMD bridge, added by Roger, 2012.04.30.
	HAL_DEF_THERMAL_VALUE,
	HAL_DEF_USB_IN_TOKEN_REV, 	// For Intel ATOM CPU utilization issue, added by Roger, 2009.06.04.
	HAL_DEF_TX_PWR_PERCENTAGE, // For WNC NEC TxPwr adjustment purpose, added by Roger, 2010.03.09.
	HAL_DEF_USB_SELECTIVE_SUSPEND,
	HAL_DEF_REMOTE_WAKEUP,
	HAL_DEF_TX_FEEDBACK_SUPPORT,  // Determine if it supports Tx Feedback through C2H, by Hana, 2015.02.10 
	// 2011/01/12 MH Add for 9xp compatiable.
	HAL_DEF_BEACON_QUEUE,				// Get the Beacon Queue ID for sending the Beacon packet. If the returned value is beacon queue, it must support HW Beacon. By Bruce, 2011-01-18.
	HAL_DEF_HW_BEACON_SUPPORT,			// Determine if it supports HW Beacon sent in TBTT automatically. By Bruce, 2011-01-18.
	HAL_DEF_HW_P2P_PS_SUPPORT,			// Determine if HW supports HW P2P Power Save function
	HAL_DEF_PRODUCT_ID,
	HAL_DEF_SUB_ID,
	HAL_DEF_EFUSE_USAGE, 		//Get current EFUSE utilization. 2008.12.19. Added by Roger.
	HAL_DEF_EFUSE_BYTES,
	HAL_DEF_AUTOLOAD_STATUS, 	//Get current autoload status, 0: autoload success, 1: autoload fail. 2008.12.19. Added by Roger.
	HAL_DEF_EFUSE_BT_USAGE, 		//Get current BT EFUSE utilization. 2008.12.19. Added by Roger.
	HAL_DEF_EFUSE_BT_BYTES,	
	// 2011/09/07 MH Add for netgear requirement.
	HAL_DEF_GAIN_OFFSET_FCC_LOW,
	HAL_DEF_GAIN_OFFSET_FCC,
	HAL_DEF_GAIN_OFFSET_FCC_HIGH,
	HAL_DEF_GAIN_OFFSET_MKK_LOW,
	HAL_DEF_GAIN_OFFSET_MKK,
	HAL_DEF_GAIN_OFFSET_MKK_HIGH,
	HAL_DEF_GAIN_OFFSET_ETSI_LOW,
	HAL_DEF_GAIN_OFFSET_ETSI,
	HAL_DEF_GAIN_OFFSET_ETSI_HIGH,
	HAL_DEF_AP_IBSS_INTERRUPT,	// Configure the interrupt mask of AP/IBSS mode 
	HAL_DEF_ENABLE_TXOK_INTERRUPT,
	HAL_DEF_CHECK_NIC_UNPLUG,	// Check the interrupt state of NIC if it is unplugged.
	HAL_DEF_USE_RA_MASK,			// Control RAMask
	//3 VHT/HT HW SUPPORT CAP
	HAL_DEF_TX_LDPC,				// TX LDPC support
	HAL_DEF_RX_LDPC,				// RX LDPC support
	HAL_DEF_TX_STBC, 				// TX STBC support
	HAL_DEF_RX_STBC, 				// RX STBC support
	HAL_DEF_RX_AMPDU_FACTOR, 	// For different AMPDU factor in HT Cap, added by Roger, 2011.12.08.
	HAL_DEF_MPDU_DENSITY, 		// For different minimum timing between MPDUs in HT Cap, added by Roger, 2011.12.08.
	HAL_DEF_EXPLICIT_BEAMFORMER,// Explicit  Compressed Steering Capable
	HAL_DEF_EXPLICIT_BEAMFORMEE,// Explicit Compressed Beamforming Feedback Capable
	HAL_DEF_RA_DECISION_RATE, 	// For 88E Rate Adaptive, added by YJ, 2012,01,19
	HAL_DEF_RA_SGI, 				// For 88E Rate Adaptive, added by YJ, 2012,01,19
	HAL_DEF_PT_PWR_STATUS, 		// For 88E Rate Adaptive, added by Wilson, 2012,02,02
	HAL_DEF_INT_MASK,
	HAL_DEF_USB_TX_THREAD,
	HAL_DEF_USB_IO_THREAD,
	HAL_DEF_TX_PAGE_BOUNDARY,
	HAL_DEF_TX_PAGE_BOUNDARY_WOWLAN,
	HAL_DEF_SUPPORT_5G,
	HAL_DEF_RTS_EN,
	HAL_DEF_ANT_DETECT, // Antenna detection mechanism, added by Roger, 2012.11.27.
	HAL_DEF_PCI_ASPM_OSC, // Support for ASPM OSC, added by Roger, 2013.03.27.
	HAL_DEF_IQK_STATUS,
	HAL_DEF_FW_FCS,
	HAL_DEF_FCS_QPKT_BY_MACID,
	HAL_DEF_ANT_DIV,
	HAL_DEF_MAC_ADRESS_COX_CAP
}HAL_DEF_VARIABLE;


// The type used to query whether the interrupt in HAL is toggled.
typedef enum _HAL_INT_TYPE
{
	HAL_INT_TYPE_ANY,				// Any interrupt
	HAL_INT_TYPE_TBDOK,				// Tx Beacon OK
	HAL_INT_TYPE_TBDER,				// Tx Beacon error
	HAL_INT_TYPE_BcnInt,			// For 92C or later, it should be early beacon interrupt.
	HAL_INT_TYPE_PSTIMEOUT,			// PS timer interrupt by TSF
	HAL_INT_TYPE_C2HCMD,			// CPU to Host Command INT Status interrupt
	HAL_INT_TYPE_RXFOVW,			// Rx FIFO over flow
	HAL_INT_TYPE_TXFOVW,			// Tx FIFO over flow
	HAL_INT_TYPE_VIDOK,				// VI queue DMA OK
	HAL_INT_TYPE_VODOK,				// VO queue DMA OK
	HAL_INT_TYPE_BEDOK,				// BE queue DMA OK
	HAL_INT_TYPE_BKDOK,				// BK queue DMA OK
	HAL_INT_TYPE_MGNTDOK,			// Mgnt queue DMA OK
	HAL_INT_TYPE_HIGHDOK,			// High queue DMA OK
	HAL_INT_TYPE_BDOK,				// Beacon queue DMA OK
	HAL_INT_TYPE_CPWM,				// CPU power Mode exchange INT Status
	HAL_INT_TYPE_CPWM2,				// CPU power Mode exchange INT Status
	HAL_INT_TYPE_TSF_BIT32_TOGGLE,	// TSF Timer BIT32 toggle indication interrupt
	HAL_INT_TYPE_RX_OK,				// Receive DMA OK
	HAL_INT_TYPE_RDU,
	HAL_INT_TYPE_HSISR_IND,			// HSISR Indicator (HSIMR & HSISR is true, this bit is set to 1)	. By YJ,120406
	HAL_INT_TYPE_SYS_PDNINT,			// System Interrupt: Hardware Power Down PIN Negedge Interrupt. By YJ,120406	
	HAL_INT_TYPE_SYS_RONINT,			// System Interrupt: Hardware Power Down PIN Negedge Interrupt. By YJ,120406
	HAL_INT_TYPE_GPIO9_INT,
	HAL_INT_TYPE_AMPDU_BURST_TIMER,	// Timer/Counter 4 interrupt.
	HAL_INT_TYPE_BEAMFORMING_TIMER,// Timer/Counter 3 interrupt.
	//==== SDIO Specified Interrupt=====//
	//HAL_INT_TYPE_SDIO_ISR_IND,
	HAL_INT_TYPE_SDIO_GPIO12_0_INT,
	HAL_INT_TYPE_SDIO_SPS_OCP_INT,
	HAL_INT_TYPE_SDIO_RON_INT_EN,
	//HAL_INT_TYPE_SDIO_PDNINT,
}HAL_INT_TYPE, *PHAL_INT_TYPE;

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct _EEPROM_OFFSET{
	u2Byte CmdRegister;
	u2Byte BIT_EEDO;
	u2Byte BIT_EEDI;
	u2Byte BIT_EESK;
	u2Byte BIT_EECS;
	u2Byte BIT_EEM0;
	u2Byte BIT_EEM1;
}EEPROM_OFFSET,*PEEPROM_OFFSET;

#ifdef REMOVE_PACK
#pragma pack()
#endif


// Power save mode configured. 
typedef	enum _RT_PS_MODE	
{
	eActive,	// Active/Continuous access.
	eMaxPs,		// Max power save mode.
	eFastPs,	// Fast power save mode.
	eAutoPs,	// Auto power save mode.
}RT_PS_MODE;

// Power save state of this station.
typedef	enum _RT_PS_STATE
{
	eAwake,
	eAwakening,
	eDozed,
	eDozing
}RT_PS_STATE;

// Inactive Power save state of this station. tynli_Test
typedef	enum _RT_IPS_STATE
{
	eIPSAwake,
	eIPSDozed
}RT_IPS_STATE;


// RF state.
typedef	enum _RT_RF_POWER_STATE
{
	eRfOn,		// RF is on after RFSleep or RFOff
	eRfSleep,	// 802.11 Power Save mode
	eRfOff,		// HW/SW Radio OFF or Inactive Power Save
	//=====Add the new RF state above this line=====//
	eRFMax
}RT_RF_POWER_STATE;

// Device power state.
typedef	enum _RT_DEVICE_POWER_STATE
{
	DeviceStateUnspecified = 0,
    	DevicePowerStateD0,
    	DevicePowerStateD1,
    	DevicePowerStateD2,
    	DevicePowerStateD3,
    	DevicePowerStateMaximum
}RT_DEVICE_POWER_STATE;

typedef	enum _Inactive_POWER_SAVE_STATE
{
	eIpsOff,
	eIpsOn,
	eIpsAuto
}Inactive_POWER_SAVE_STATE;




#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct _RT_ISR_CONTENT
{
	union{
		u4Byte			IntArray[2];
		u4Byte			IntReg4Byte;
		u2Byte			IntReg2Byte;
	};

	u8Byte			IntReg8Byte;
	u4Byte			RxIntLength;
}RT_ISR_CONTENT, *PRT_ISR_CONTENT;

#ifdef REMOVE_PACK
#pragma pack()
#endif

typedef	enum _HAL_INT_MODE
{
	HAL_INT_MODE_LOCAL = 0, // The local interrupt
	HAL_INT_MODE_SYSTEM = 1, // The system interrupt
}HAL_INT_MODE, *PHAL_INT_MODE;

typedef enum _SCAN_OPERATION_BACKUP_OPT{
	SCAN_OPT_BACKUP_BAND0=0,
	SCAN_OPT_BACKUP_BAND1,
	SCAN_OPT_RESTORE,
	SCAN_OPT_MAX
}SCAN_OPERATION_BACKUP_OPT;


// Related IO. 
typedef	enum _IO_TYPE{
	IO_CMD_PAUSE_BAND0_DM_BY_SCAN = 0,	
	IO_CMD_PAUSE_BAND1_DM_BY_SCAN = 1,
	IO_CMD_RESUME_DM_BY_SCAN = 2,
}IO_TYPE,*PIO_TYPE;


//LeisurePS state
typedef enum _LEISURE_POWER_SAVE_STATE
{
	eLpsOff,
	eLpsOn,
	eLpsAuto
}LEISURE_POWER_SAVE_STATE;

// Firmware PS mode for control LPS. by tynli.
typedef enum _FW_CTRL_PS_MODE_NUM{
	FW_PS_ACTIVE_MODE 	 =0,
	 //Legacy
	FW_PS_MIN_MODE	 =1,
	FW_PS_MAX_MODE	 =2,
 	FW_PS_SELF_DEFINED_MODE = 3,
 	//UAPSD
 	FW_PS_UAPSD_MIN_MODE = 4,
 	FW_PS_UAPSD_MAX_MODE = 5,
 	FW_PSE_UAPSD_SELF_DEFINED_MODE = 6,
 	FW_PS_NUM_MODE= 7,
}FW_CTRL_PS_MODE;

typedef enum _FW_IPS_MODE{
	FW_IPS_ACTIVE_MODE 	 =	0,
	FW_IPS_SCAN_MODE 	 =	1,
	FW_IPS_CARD_DISABLE_MODE 	 =	2,
	FW_IPS_RF_OFF_MODE 	 =	3,
 	FW_IPS_MAX,
}FW_CTRL_IPS_MODE;

// Define WoWLAN mode. Added by tynli. 2009.09.02.
typedef enum _WAKE_ON_WLAN_MODE
{
	eWoWLANDisable,
	eWakeOnMagicPacketOnly,
	eWakeOnPatternMatchOnly,
	eWakeOnBothTypePacket
}WAKE_ON_WLAN_MODE;

#define	WAKE_ON_MAGIC_PACKET		BIT0
#define	WAKE_ON_PATTERN_MATCH		BIT1
#define	WAKE_ON_UNICAST_PACKET		BIT2

// After 8188E, Hw supports indicating wake packet type in Rx Desc. 2013.10.01. by tynli.
#define	HW_SUPPORT_PARSING_WAKE_PACKET(_Adapter)		TRUE

// Fw scan offload type
#define	SCAN_OFFLOAD_TYPE_D0		BIT0
#define	SCAN_OFFLOAD_TYPE_D3		BIT1

typedef enum	_RT_P2P_PS_EXE_TYPE
{
	RT_P2P_PS_EXE_BY_NONE = 0,					// Not Specified
	RT_P2P_PS_EXE_BY_HW = 1,					// Execute the P2P Power Save by HW/FW, such as 8723
	RT_P2P_PS_EXE_BY_SW_HW_TIMER = 2,			// Execute the P2P Power Save by Driver and using HW Timer, suach as 8192CE
	RT_P2P_PS_EXE_BY_SW_TIMER = 3,				// Execute the P2P Power Save by Driver but using SW Timer (Workitem), such as 8192CU
}RT_P2P_PS_EXE_TYPE, *PRT_P2P_PS_EXE_TYPE;


#ifdef REMOVE_PACK
#pragma pack(1)
#endif

//
// This is a partial structure of NDIS_PM_PROTOCOL_OFFLOAD which is defined in N62,
// for multi-platform/multi-chipset architecture requirement, we define it in HAL.
// Added by tynli. 2009.08.05.
//
typedef struct _RT_PM_PROTOCOL_OFFLOAD {
	union _RT_PROTOCOL_OFFLOAD_PARAMETERS {
		struct _RT_IPV4_ARP_PARAMETERS {
		  u4Byte	Flags;
		  u1Byte	RemoteIPv4Address[4];
		  u1Byte	HostIPv4Address[4];
		  u1Byte	MacAddress[6];
		} IPv4ARPParameters;
		struct  _RT_IPV6_NS_PARAMETERS {
		  ULONG  Flags;
		  u1Byte	RemoteIPv6Address[16];
		  u1Byte	SolicitedNodeIPv6Address[16];
		  u1Byte	MacAddress[6];
		  u1Byte	TargetIPv6Addresses[2][16];
		} IPv6NSParameters;
		struct _RT_DOT11_RSN_REKEY_PARAMETERS {
		  u4Byte	Flags;
		  u1Byte	KCK[16];
		  u1Byte	KEK[16];
		  u8Byte	KeyReplayCounter;
		} Dot11RSNRekeyParameters;
	} ProtocolOffloadParameters;
} RT_PM_PROTOCOL_OFFLOAD, *PRT_PM_PROTOCOL_OFFLOAD;


typedef struct _RT_PM_IPV4_ARP_PARAMETERS {
	  u4Byte	Flags;
	  u1Byte	RemoteIPv4Address[4];
	  u1Byte	HostIPv4Address[4];
	  u1Byte	MacAddress[6];
} RT_PM_IPV4_ARP_PARAMETERS, *PRT_PM_IPV4_ARP_PARAMETERS;

typedef struct  _RT_PM_IPV6_NS_PARAMETERS {
	  ULONG  Flags;
	  u1Byte	RemoteIPv6Address[16];
	  u1Byte	SolicitedNodeIPv6Address[16];
	  u1Byte	MacAddress[6];
	  u1Byte	TargetIPv6Addresses[2][16];
} RT_PM_IPV6_NS_PARAMETERS, *PRT_PM_IPV6_NS_PARAMETERS;

typedef struct _RT_PM_DOT11_RSN_REKEY_PARAMETERS {
	  u4Byte	Flags;
	  u1Byte	KCK[16];
	  u1Byte	KEK[16];
	  u8Byte	KeyReplayCounter;
} RT_PM_DOT11_RSN_REKEY_PARAMETERS, *PRT_PM_DOT11_RSN_REKEY_PARAMETERS;


//
// Network list offload structures
//
#define NATIVE_802_11_MAX_NETWORKOFFLOAD_SIZE		10

typedef struct _RT_OFFLOAD_NETWORK
{
	u1Byte				ssidbuf[32];
	u4Byte				ssidlen;
	u1Byte				chiper;  // Used PAIRWISE_CHIPER !!
	ULONG				channelNumberHit[4];
	BOOLEAN				bPrivacy;
	u4Byte				SecLvl;
}RT_OFFLOAD_NETWORK,*PRT_OFFLOAD_NETWORK;

typedef struct _RT_NLO_INFO
{
	ULONG					NumDot11OffloadNetwork;
	ULONG  					FastScanPeriod; 
	ULONG 					FastScanIterations; 
	ULONG 					SlowScanPeriod; 	
	RT_OFFLOAD_NETWORK 	dDot11OffloadNetworkList[NATIVE_802_11_MAX_NETWORKOFFLOAD_SIZE];
}RT_NLO_INFO, *PRT_NLO_INFO;

typedef struct _RT_AOAC_REPORT_{
	u8Byte	TxIV;
	u8Byte	ReplayCounterOfEapolKey;
	u1Byte	GroupKey[32];
	u1Byte	KeyIndex;
	u1Byte	SecurityType; 
}RT_AOAC_REPORT, *PRT_AOAC_REPORT;

#ifdef REMOVE_PACK
#pragma pack()
#endif

//
// Determine how we handle the Beacon sent, by Bruce, 2011-01-18.
// By the following definition, we should schedule the timer if we use the SW beacon or handle the TBTT timing.
#define	BEACON_SEND_AUTO_HW				0		// (Beacon Queue) HW updates Beacon buffer automatically (by interrupts or events) and the packets can be sent in TBTT automatically.
#define	BEACON_SEND_AUTO_SW				1		// (Beacon Queue) SW needs to updates the Beacon by self timer but the HW can send the Beacons in TBTT. 
#define	BEACON_SEND_MANUAL				2		// (AC/MGNT/HIGN Queue) SW needs to updates the Beacon and send it by self timer. In other words, we need to handle the TBTT timing.


//
// Cannot perform IO. We can add more conditions to indicate the IO fail status.
// By Bruce, 2008-02-18.
//
#define	RT_PCI_CANNOT_IO(__pAdapter)	\
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved ||	\
			 RT_IS_FUNC_DISABLED((__pAdapter), (DF_IO_BIT | DF_IO_D3_BIT)))
			 
#define RT_USB_CANNOT_IO(__pAdapter) \
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved || \
			 RT_IS_FUNC_DISABLED((__pAdapter), DF_IO_BIT))

#define RT_SDIO_CANNOT_IO(__pAdapter) \
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved || \
			 RT_IS_FUNC_DISABLED((__pAdapter), DF_IO_BIT))

#define	RT_DRIVER_STOP(__pAdapter)	\
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved ||	\
			 (__pAdapter)->bDriverIsGoingToUnload)


#define	RT_DRIVER_HALT(__pAdapter)	\
			((__pAdapter)->bDriverStopped || \
			 (__pAdapter)->bSurpriseRemoved)

#define	HAS_REQUEST_TO_HANDLE(__pAdapter)	\
			((__pAdapter)->pPortCommonInfo->pPortHelper->bDeleteMac == TRUE)

#define MAX_POWER_INDEX 		0x3F


#define	RT_CANNOT_IO(__pAdapter)	RT_SDIO_CANNOT_IO(__pAdapter)


#define GET_UNDECORATED_AVERAGE_RSSI(_Adapter)	\
		(ACTING_AS_AP(_Adapter) || ACTING_AS_IBSS(_Adapter))?		\
			(((HAL_DATA_TYPE *)((_Adapter)->HalData))->EntryMinUndecoratedSmoothedPWDB):	\
			(((HAL_DATA_TYPE *)((_Adapter)->HalData))->UndecoratedSmoothedPWDB)


#define RT_IsSwChnlAndBwInProgress(_Adapter) (	GET_HAL_DATA(_Adapter)->SwChnlInProgress || \
												GET_HAL_DATA(_Adapter)->bSwChnlAndSetBWInProgress || \
												GET_HAL_DATA(_Adapter)->SetBWModeInProgress)

#define RT_ResetSwChnlProgress(_Adapter) \
{\
	GET_HAL_DATA(_Adapter)->SwChnlInProgress = FALSE;\
	GET_HAL_DATA(_Adapter)->bSwChnlAndSetBWInProgress = FALSE;\
}

#define RT_GetBandWidth(_Adapter)		(GET_HAL_DATA(_Adapter)->CurrentChannelBW)
#define RT_GetCenter_Frequency_Index1(_Adapter)		(GET_HAL_DATA(_Adapter)->CurrentCenterFrequencyIndex1)
#define RT_GetRFType(_Adapter) (GET_HAL_DATA(_Adapter)->RF_Type)

//
// Reason to turn off RF, high value indicate high priority.
//
typedef u4Byte RT_RF_CHANGE_SOURCE;
#define RF_CHANGE_BY_SW BIT31
#define RF_CHANGE_BY_HW BIT30
#define RF_CHANGE_BY_PS BIT29
#define RF_CHANGE_BY_IPS BIT28
#define RF_CHANGE_BY_INIT	0	// Do not change the RFOff reason. Defined by Bruce, 2008-01-17.


typedef enum _RX_PACKET_TYPE{
	NORMAL_RX,
	TX_REPORT1,
	TX_REPORT2,
	HIS_REPORT,
	C2H_PACKET
}RX_PACKET_TYPE, *PRX_PACKET_TYPE;



typedef u1Byte
(*NicGetEEPROMSizeHandler)(
	IN	PADAPTER		Adapter
	);

typedef RT_STATUS
(*NicReadAdapterInfoHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicInitializeVariablesHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicDeInitializeVariablesHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicCancelAllTimerHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID	
(*NicReleaseAllTimerHandler)(
	IN	PADAPTER		Adapter
	);

typedef RT_STATUS	
(*NicInitializeAdapterHandler)(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Channel
	);

typedef VOID	
(*NicHalUpdateDefaultSettingHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID	
(*NicHalIntUpdateDefSetHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicHaltAdapterHandler)(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bReset
	);


typedef VOID
(*NicShutdownAdapterHandler)(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bReset
	);

typedef VOID
(*NicSleepAdapterHandler)(
	IN	PADAPTER		Adapter
	);


typedef void
(*NicSetHwRegHandler)(
	IN	PADAPTER		Adapter,
	IN	u1Byte				RegName,
	IN	pu1Byte				val
	);

typedef void
(*NicGetHwRegHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				RegName,
	OUT	pu1Byte				val
	);

typedef BOOLEAN
(*NicGetHalDefVarHandler)(
	IN	PADAPTER		Adapter,
	IN	HAL_DEF_VARIABLE		eVariable,
	IN	PVOID					pValue
	);

typedef BOOLEAN
(*NicSetHalDefVarHandler)(
	IN	PADAPTER		Adapter,
	IN	HAL_DEF_VARIABLE		eVariable,
	IN	PVOID					pValue
	);

typedef BOOLEAN
(*NicGetInterruptHandler)(
	IN	PADAPTER		Adapter,
	IN	HAL_INT_TYPE	intType
	);

typedef u1Byte
(*NicGetHwRateFromMRateHandler)(
	IN	u1Byte	rate
	);

typedef VOID
(*NicQueryRxDescStatusHandler)(
	IN		PADAPTER		Adapter,
	IN		PVOID			pDesc,
	IN OUT	PRT_RFD			pRfd
	);

typedef VOID
(*NicTxFillDescriptorHandler)(
	IN	PADAPTER	Adapter,
	IN	PRT_TCB		pTcb,
	IN	u2Byte		nBufIndex,
	IN	u2Byte		nFragIndex,
	IN	u2Byte		nFragBufferIndex,
	IN	u2Byte		nCurDesc
	);

typedef VOID
(*NicQueryTxDescStatusHandler)(
	IN	PADAPTER		Adapter,
	IN	PVOID			pDesc,
	OUT	PRT_TCB_STATUS	pRtTcbStatus
	);

typedef VOID
(*NicSetWirelessModeHandler)(
	IN	PADAPTER	Adapter,
	IN	u2Byte			bWirelessMode
    );


typedef u2Byte
(*NicGetSupportedWirelessModeHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicSwChnlByDelayHandler)(
	IN	PADAPTER		Adapter,
	IN	u1Byte			channel
	);

typedef VOID
(*NicSwChnlByTimerHandler)(
	IN	PADAPTER		Adapter,
	IN	u1Byte			channel
	);

typedef void
(*NicSetTxPowerLevelHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				channel
	);

typedef void
(*NicGetTxPowerLevelHandler)(
	IN	PADAPTER		Adapter,
	OUT	ps4Byte			powerlevel
	);

typedef BOOLEAN
(*NicUpdateTxPowerDbmHandler)(
	IN	PADAPTER		Adapter,
	IN	s4Byte			powerInDbm
	);


typedef BOOLEAN
(*NicSetTxAntennaHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				SelectedAntenna
	);

typedef VOID
(*NicEnableHWSecurityConfigHandler)(
	IN	PADAPTER		Adapter);

typedef VOID
(*NicDisableHWSecurityConfigHandler)(
	IN	PADAPTER		Adapter);

typedef VOID
(*NicSetKeyHandler)(
	IN	PADAPTER		Adapter,
	IN	u4Byte			KeyIndex,
	IN	pu1Byte			pMacAddr,
	IN	BOOLEAN			IsGroup,
	IN	u4Byte			EncAlgo,
	IN	BOOLEAN			IsWEPKey,//if OID = OID_802_11_WEP
	IN	BOOLEAN			ClearAll
);

typedef VOID
(*NicAllowAllDestAddrHandler)(
	IN	PVOID				Adapter,
	IN	BOOLEAN				bAllowAllDA,
	IN	BOOLEAN				WriteIntoReg
);

typedef VOID
(*NicAllowErrorPacketHandler)(
	IN	PVOID				Adapter,
	IN	BOOLEAN				bAllowErrPkt,
	IN	BOOLEAN				WriteIntoReg
);

typedef VOID
(*NicSetMonitorModeHandler)(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bEnableMonitorMode
);


typedef VOID
(*NicLedControlHandler)(
	IN	PVOID				Adapter,
	IN	LED_CTL_MODE		LedAction
);

typedef VOID
(*NicHalEnableRxHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalDisableRxHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalEnableTxHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalDisableTxHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalLeaveSSHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalEnterSSHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalResetAllTimerHandler)(
	IN	PADAPTER				pAdapter
	);

typedef BOOLEAN
(*NicInterruptRecognizedHandler)(
	IN	PADAPTER		Adapter,
	IN	PVOID			pContent,
	IN	u4Byte			ContentLen
	);

typedef VOID
(*NicDisableInterruptHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicClearInterruptHandler)(
	IN	PADAPTER		Adapter
	);

typedef VOID
(*NicEnableInterruptHandler)(
	IN	PADAPTER		Adapter
	);

//
// Runtime D3 related function handler, added by Roger, 2013.02.05.
//
#if INTEL_RTD3_SUPPORT
typedef VOID
(*NicHalRTD3LeaveHandler)(
	IN	PADAPTER				pAdapter
	);

typedef VOID
(*NicHalRTD3EnterHandler)(
	IN	PADAPTER				pAdapter
	);
#endif

typedef RT_STATUS
(*NicHalSetRsvdPageBndyHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				Type
);

typedef VOID
(*NicHalDownloadRsvdPageHandler)(
	IN	PADAPTER			Adapter
);

typedef VOID
(*NicHalGetAOACReportHandler)(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				AOACRptBuf
);

typedef VOID
(*NicHalDropRxFIFOHandler)(
	IN	PADAPTER			Adapter
);

//
// <Roger_TODO> We should declare SDIO related function handler here!!
// 2010.12.20.
//
typedef VOID
(*NicHalSdioGetCmdAddressHandler)(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				DeviceID,
	IN	u4Byte				Addr,
	OUT	pu4Byte				pCmdAddr
	);

typedef BOOLEAN
(*NicHalSdioSetQueueMappingHandler)(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					NumIn,
	IN	u1Byte					NumOut
	);


typedef BOOLEAN
(*NicHalSdioQueryTxBufferStatusHandler)(
	IN		PADAPTER		Adapter
	);

typedef BOOLEAN
(*NicHalSdioQueryTxBufferAvailableHandler)(
	IN	PADAPTER	Adapter,
	IN	PSDIO_OUT_CONTEXT pContext
	);

typedef BOOLEAN
(*NicHalSdioIoRegCmd52AvailableHandler)(
	IN	PADAPTER	Adapter,
	IN	u4Byte		offset
	);

typedef VOID
(*NicHalSdioTxCompleteHandler)(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext,
	IN	u4Byte					status
	);

typedef BOOLEAN
(*NicRxHandleInterruptHandler)(
	IN 	PADAPTER 		pAdapter
);

typedef VOID
(*NicClearSysInterruptHandler)(
	IN	PADAPTER		Adapter
	);

typedef BOOLEAN
(*NicHalSdioQueryRxDMARequestHandler)(
	IN		PADAPTER		Adapter,
	OUT		pu2Byte			pRxReqLength
	);

typedef VOID
(*NicHalRxAggrHandler)(
	IN	PADAPTER				pAdapter,
	IN	BOOLEAN				Value
	);

typedef BOOLEAN
(*NicHalUsbSetQueuePipeMappingHandler)(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					NumInPipe,
	IN	u1Byte					NumOutPipe
	);

typedef u4Byte
(*NicGetRxPacketShiftBytesHandler)(
	PRT_RFD		pRfd	
);

typedef VOID
(*NICInitHalRATRTableHandler)(
	IN	PADAPTER			Adapter
);

typedef VOID
(*NICUpdateHalRATRTableHandler)(
	IN	PADAPTER			Adapter,	
	IN	POCTET_STRING		posLegcyRate,	
	IN	pu1Byte				pMcsRate,
	IN	PRT_WLAN_STA            pEntry
);

typedef VOID
(*NICResetHalRATRTableHandler)(
	IN	PADAPTER			Adapter
);


typedef VOID
(*NICUpdateHalRAMaskHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				macId,
	IN	PRT_WLAN_STA           pEntry,
	IN	u1Byte				rssi_level
);

typedef VOID
(*NICResetHalRAMaskHandler)(
	IN	PADAPTER			Adapter
);

typedef VOID
(*NICRAPostActionHandler)(
	IN	PVOID			pDM_Odm
);

typedef VOID
(*NICUpdateLPSStatusHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				RegLeisurePsMode,
	IN	u1Byte				RegPowerSaveMode
);

typedef VOID
(*NICUpdateIPSStatusHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				RegInactivePsMode
);

typedef VOID
(*NICSetBWModeHandler)(
	IN	PADAPTER			Adapter,
	IN	CHANNEL_WIDTH	ChnlWidth,
	IN	EXTCHNL_OFFSET	Offset
);

typedef RT_STATUS
(*NICCmdSendPacketHandler)(
	PADAPTER				Adapter,
	PRT_TCB					pTcb,
	PRT_TX_LOCAL_BUFFER 	pBuf,
	u4Byte					BufferLen,
	u1Byte					PacketType,
	BOOLEAN					bLastInitPacket
);

typedef VOID
(*NicSetBeaconRelatedRegistersHandler)(
	IN	PADAPTER		Adapter,
	IN	PVOID			pOpMode,
	IN	u2Byte			BcnInterval,
	IN 	u2Byte			AtimWindow
);

typedef VOID
(*NicDumpHardwareProfileHandler)(
	IN	PADAPTER		Adapter
);

typedef u4Byte
(*NicRxCommandPacketHandler)(
	PADAPTER		Adapter, 
	PRT_RFD 		pRfd
);


typedef VOID
(*NicScanOperationBackupHandler)(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Operation
);

typedef BOOLEAN
(*NicCheckHWStopHandler)(
	IN	PADAPTER		Adapter
);

typedef VOID
(*NicTxFillCmdDescHandler)(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb,
	IN	u1Byte			QueueIndex,
	IN	u2Byte			index,
	IN	BOOLEAN			bFirstSeg,
	IN	BOOLEAN			bLastSeg,
	IN	pu1Byte			VirtualAddress,
	IN	u4Byte			PhyAddressLow,
	IN	u4Byte			BufferLen,
	IN	u4Byte			DescPacketType,
	IN	u4Byte			PktLen
);

typedef BOOLEAN
(*NicGetNmodeSupportBySecCfgHandler)(
	IN	PADAPTER		Adapter);	

typedef BOOLEAN
(*NicGetNmodeSupportBySWSecHandler)(
	IN	PADAPTER		Adapter);		


typedef VOID
(*NicCountTxStatisticsHandler)(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB		pTcb
	
);

typedef BOOLEAN
(*NicQuerySysCpuWakeHandler)(
	IN	PADAPTER		Adapter
);

typedef VOID
(*NicGPIOChangeRFHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				variable
);


typedef BOOLEAN
(*NicSwAntDivCheckBeforeLink)(
	IN		PDM_ODM_T		pDM_Odm
	//IN	PADAPTER			Adapter
	);
	
typedef BOOLEAN
(*NicPathDivCheckBeforeLink)(
	IN		PDM_ODM_T		pDM_Odm
	//IN	PADAPTER			Adapter
	);
	
typedef VOID
(*NicHalSetCTSDynamicBWSelectHandler)(
	IN	PADAPTER			Adapter,
	IN	u1Byte				BWFallBackLevel
);

typedef VOID
(*NicSetFwRelatedForWoWLANHandler)(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN 			bHostIsGoingtoSleep
);

typedef VOID
(*NICSwChnlAndSetBwHandler)(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bSwitchChannel,
	IN	BOOLEAN				bSetBandWidth,
	IN	u1Byte				ChannelNum,
	IN	CHANNEL_WIDTH	ChnlWidth,
	IN	EXTCHNL_OFFSET	ExtChnlOffsetOf40MHz,
	IN	EXTCHNL_OFFSET	ExtChnlOffsetOf80MHz,
	IN	u1Byte				CenterFrequencyIndex1
);

typedef	u4Byte
(*NicFillH2CCommandHandler)(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
);

typedef	u1Byte
(*NicGetFwPsStateDefHandler)(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		FwPsState
);

typedef	BOOLEAN
(*NicCheckCPWMInterruptHandler)(
	IN	PADAPTER	Adapter
);

typedef	VOID
(*NicWaitForH2CQueueEmptyHandler)(
	IN	PADAPTER	Adapter
);

typedef struct _HAL_INTERFACE{
	// Pointer
	pu1Byte						pDescString;

	// Structure pointer
	PEEPROM_OFFSET				pEEPROMOffset;

	// Preamble.
	BOOLEAN						bUseShortPreamble;

	// Capability.
	BOOLEAN						bSupportTbttNotification; // TRUE if the H/W can notify S/W before TBTT.
	
	// CCX supported version
	u1Byte						CcxVerSupported; // 0: means not support CCX, > 0: are the version of CCX supported by this IC.

	// WLAN Device operations. 
	NicGetEEPROMSizeHandler		GetEEPROMSizeHandler;
	NicReadAdapterInfoHandler		ReadAdapterInfoHandler;

	NicInitializeVariablesHandler		InitializeVariablesHandler;
	NicDeInitializeVariablesHandler	DeInitializeVariablesHandler;
	NicCancelAllTimerHandler		CancelAllTimerHandler;
	NicReleaseAllTimerHandler		ReleaseAllTimerHandler;

	NicInitializeAdapterHandler		InitializeAdapterHandler;
	NicHalUpdateDefaultSettingHandler	HalUpdateDefaultSettingHandler;
	NicHalIntUpdateDefSetHandler	HalIntUpdateDefSetHandler;
	
	NicHaltAdapterHandler			HaltAdapterHandler;
	NicShutdownAdapterHandler		ShutdownHandler;
	NicSleepAdapterHandler			SleepAdapterHandler;

	NicSetHwRegHandler			SetHwRegHandler;
	NicGetHwRegHandler			GetHwRegHandler;

	NicGetHalDefVarHandler			GetHalDefVarHandler;
	NicSetHalDefVarHandler			SetHalDefVarHandler;

	NicGetInterruptHandler			GetInterruptHandler;
	
	NicGetHwRateFromMRateHandler		GetHwRateFromMRateHandler;
	
	NicQueryRxDescStatusHandler		QueryRxDescStatusHandler;
	NicTxFillDescriptorHandler			TxFillDescriptorHandler;
	NicQueryTxDescStatusHandler		QueryTxDescStatusHandler;

	NicSetWirelessModeHandler			SetWirelessModeHandler;
	NicGetSupportedWirelessModeHandler	GetSupportedWirelessModeHandler;

	NicSwChnlByDelayHandler			SwChnlByDelayHandler;
	NicSwChnlByTimerHandler			SwChnlByTimerHandler;

	// Tx Power Related.
	NicSetTxPowerLevelHandler			SetTxPowerLevelHandler;
	NicGetTxPowerLevelHandler			GetTxPowerLevelHandler;
	NicUpdateTxPowerDbmHandler		UpdateTxPowerDbmHandler;
	
	NicSetTxAntennaHandler			SetTxAntennaHandler;
	NicSwAntDivCheckBeforeLink			SwAntDivCheckBeforeLinkHandler;
	NicPathDivCheckBeforeLink			PathDivCheckBeforeLinkHandler;
	
	NicHalSetCTSDynamicBWSelectHandler	HalSetCTSDynamicBWSelectHandler;
	
	NicEnableHWSecurityConfigHandler	EnableHWSecCfgHandler;
	NicDisableHWSecurityConfigHandler	DisableHWSecCfgHandler;
	NicSetKeyHandler					SetKeyHandler;
		
	NicAllowAllDestAddrHandler			AllowAllDestAddrHandler;
	NicAllowErrorPacketHandler			AllowErrorPacketHandler;

	NicSetMonitorModeHandler			SetMonitorModeHandler;

	NicLedControlHandler				LedControlHandler;	
	
	NicHalEnableRxHandler				HalEnableRxHandler;//Aadded by Roger, 2007.02.12.
	NicHalDisableRxHandler				HalDisableRxHandler;//Aadded by Roger, 2007.02.12.
	
	NicHalDisableTxHandler				HalDisableTxHandler;//Aadded by Roger, 2009.07.08.
	NicHalEnableTxHandler				HalEnableTxHandler;//Aadded by Roger, 2009.07.08.
	
	NicCountTxStatisticsHandler			HalCountTxStatisticsHandler; //Add for count unicast tx bytes temp 2008-04-14
	NicHalResetAllTimerHandler			HalResetAllTimerHandler;

	NicHalEnterSSHandler				HalEnterSSHandler; //Aadded by Roger, 2007.02.12.
	NicHalLeaveSSHandler				HalLeaveSSHandler; //Aadded by Roger, 2007.02.12.

	NicInterruptRecognizedHandler		InterruptRecognizedHandler;
	NicDisableInterruptHandler			DisableInterruptHandler;
	NicClearInterruptHandler			ClearInterruptHandler;
	NicEnableInterruptHandler			EnableInterruptHandler;	
	NicDumpHardwareProfileHandler		DumpHardwareProfileHandler;	
	
	//
	// Runtime D3 related function handler, added by Roger, 2013.02.05.
	//
#if INTEL_RTD3_SUPPORT
	NicHalRTD3EnterHandler				HalRTD3EnterHandler;
	NicHalRTD3LeaveHandler				HalRTD3LeaveHandler;
#endif
	
	// HW function
	NicHalSetRsvdPageBndyHandler		HalSetRsvdPageBndyHandler;
	NicHalDownloadRsvdPageHandler		HalDownloadRsvdPageHandler;
	NicHalGetAOACReportHandler			HalGetAOACReportHandler;
	NicHalDropRxFIFOHandler			HalDropRxFIFOHandler;

	
	//
	// Bus specific operations.
	//
	NicHalSdioGetCmdAddressHandler		HalSdioGetCmdAddressHandler;
	NicHalSdioSetQueueMappingHandler	HalSdioSetQueueMappingHandler;

	//
	// <Roger_TODO> The following handler should be removed later!!
	//
	NicHalSdioTxCompleteHandler			HalSdioTxCompleteHandler;
	NicHalRxAggrHandler				HalRxAggrHandler;
	NicHalUsbSetQueuePipeMappingHandler	HalUsbSetQueuePipeMappingHandler;
	NicHalSdioQueryTxBufferStatusHandler	HalSdioQueryTxBufferStatusHandler;
	NicHalSdioQueryTxBufferAvailableHandler	HalSdioQueryTxBufferAvailableHandler;
	NicClearSysInterruptHandler			ClearSysInterruptHandler;
	NicHalSdioQueryRxDMARequestHandler	HalSdioQueryRxDMARequestHandler;
	NicRxHandleInterruptHandler			RxHandleInterruptHandler;
	NicHalSdioIoRegCmd52AvailableHandler	HalSdioIoRegCmd52AvailableHandler;


	// Added for 8190 from here
	NicGetRxPacketShiftBytesHandler		GetRxPacketShiftBytesHandler;
	NICInitHalRATRTableHandler			InitHalRATRTableHandler;
	NICUpdateHalRATRTableHandler		UpdateHalRATRTableHandler;
	NICResetHalRATRTableHandler			ResetHalRATRTableHandler;
	NICUpdateHalRAMaskHandler			UpdateHalRAMaskHandler;
	NICResetHalRAMaskHandler			ResetHalRAMaskHandler;
	NICRAPostActionHandler				RAPostActionHandler;
	
	NICUpdateLPSStatusHandler			UpdateLPSStatusHandler;
	NICUpdateIPSStatusHandler			UpdateIPSStatusHandler;
	NICSetBWModeHandler					SetBWModeHandler;
	NICCmdSendPacketHandler				CmdSendPacketHandler;
	NicSetBeaconRelatedRegistersHandler	SetBeaconRelatedRegistersHandler;
	NicRxCommandPacketHandler			RxCommandPacketHandler;
	NicCheckHWStopHandler				TxCheckStuckHandler;
	NicCheckHWStopHandler				RxCheckStuckHandler;
	NicTxFillCmdDescHandler				TxFillCmdDescHandler;	
	NicGetNmodeSupportBySecCfgHandler	GetNmodeSupportBySecCfgHandler;
	NicGetNmodeSupportBySWSecHandler	GetNmodeSupportBySWSecHandler;
	NicScanOperationBackupHandler		ScanOperationBackupHandler;
	NicGPIOChangeRFHandler				GPIOChangeRFHandler;
	NicSetFwRelatedForWoWLANHandler		SetFwRelatedForWoWLANHandler;
	NICSwChnlAndSetBwHandler			SwChnlAndSetBWHandler;
	//NicH2CCmdActionHandler				H2CCmdActionHandler;
	NicFillH2CCommandHandler			FillH2CCommandHandler;
	NicGetFwPsStateDefHandler			GetFwPsStateDefHandler;
	NicCheckCPWMInterruptHandler		CheckCPWMInterruptHandler;
	NicWaitForH2CQueueEmptyHandler	WaitForH2CQueueEmptyHandler;
}HAL_INTERFACE,*PHAL_INTERFACE;


RT_STATUS
HalAssociateNic(
	IN	PVOID			Adapter,
	IN	BOOLEAN			IsDefaultAdapter
	);

VOID
HalDisassociateNic(
	IN	PVOID			Adapter
	);

u2Byte
HalGetFirmwareVerison(
		IN PADAPTER Adapter
	);


u2Byte
HalGetSupportedWirelessMode(
	IN	PADAPTER		pAdapter
	);

u2Byte
HalChangeWirelessMode(
	PADAPTER		Adapter,
	u1Byte			ChannelNum
);

VOID
Hal_PauseTx(
	IN		PADAPTER	Adapter,
	u1Byte	type
	);

//
//2011/01/10 MH Add fro 9xp compatible.
//
typedef enum _SIGNAL_STRENGTH_POLICY{
	SIGNAL_STRENGTH_SCALE_FINE = 0,
	SIGNAL_STRENGTH_SCALE_NORMAL = 1,	
}SIGNAL_STRENGTH_POLICY;

typedef enum _InitialGainOpType{
	IG_Backup=0,
	IG_Restore,
	IG_Max
}InitialGainOpType;


typedef enum _VERSION_8192C{
	VERSION_TEST_CHIP_88C = 0x0000,
	VERSION_TEST_CHIP_92C = 0x0020,
	VERSION_TEST_UMC_CHIP_8723A = 0x0081,
	VERSION_NORMAL_TSMC_CHIP_88C = 0x0008, 
	VERSION_NORMAL_TSMC_CHIP_92C = 0x0028,
	VERSION_NORMAL_TSMC_CHIP_92C_1T2R = 0x0018,
	VERSION_NORMAL_UMC_CHIP_88C_A_CUT = 0x0088,
	VERSION_NORMAL_UMC_CHIP_92C_A_CUT = 0x00a8,
	VERSION_NORMAL_UMC_CHIP_92C_1T2R_A_CUT = 0x0098,		
	VERSION_NORMAL_UMC_CHIP_8723A_1T1R_A_CUT = 0x0089,
	VERSION_NORMAL_UMC_CHIP_8723A_1T1R_B_CUT = 0x1089,	
	VERSION_NORMAL_UMC_CHIP_88C_B_CUT = 0x1088, 
	VERSION_NORMAL_UMC_CHIP_92C_B_CUT = 0x10a8, 
	VERSION_NORMAL_UMC_CHIP_92C_1T2R_B_CUT = 0x1090, 
	VERSION_TEST_CHIP_92D_SINGLEPHY= 0x0022,
	VERSION_TEST_CHIP_92D_DUALPHY = 0x0002,
	VERSION_NORMAL_CHIP_92D_SINGLEPHY= 0x002a,
	VERSION_NORMAL_CHIP_92D_DUALPHY = 0x000a,
	VERSION_NORMAL_CHIP_92D_C_CUT_SINGLEPHY = 0x202a,
	VERSION_NORMAL_CHIP_92D_C_CUT_DUALPHY = 0x200a,
	VERSION_NORMAL_CHIP_92D_D_CUT_SINGLEPHY = 0x302a,
	VERSION_NORMAL_CHIP_92D_D_CUT_DUALPHY = 0x300a,
	VERSION_NORMAL_CHIP_92D_E_CUT_SINGLEPHY = 0x402a,
	VERSION_NORMAL_CHIP_92D_E_CUT_DUALPHY = 0x400a,
	VERSION_TEST_CHIP_1T1R_8812 = 0x0004,
	VERSION_TEST_CHIP_2T2R_8812 = 0x0024,
	VERSION_NORMAL_TSMC_CHIP_1T1R_8812 = 0x100c,
	VERSION_NORMAL_TSMC_CHIP_2T2R_8812 = 0x102c,
	VERSION_NORMAL_TSMC_CHIP_1T1R_8812_C_CUT = 0x200c,
	VERSION_NORMAL_TSMC_CHIP_2T2R_8812_C_CUT = 0x202c,
	VERSION_TEST_CHIP_8821 = 0x0005,
	VERSION_NORMAL_TSMC_CHIP_8821 = 0x000d,
	VERSION_NORMAL_TSMC_CHIP_8821_B_CUT = 0x100d,
	VERSION_NORMAL_TSMC_CHIP_8821_C_CUT = 0x200d,
	VERSION_TEST_CHIP_1T1R_8723B = 0x0106,
	VERSION_NORMAL_SMIC_CHIP_1T1R_8723B = 0x010E,	
	VERSION_NORMAL_SMIC_CHIP_1T1R_8723B_B_CUT = 0x110E,
	VERSION_NORMAL_SMIC_CHIP_1T1R_8723B_D_CUT = 0x310E,
	VERSION_NORMAL_SMIC_CHIP_1T1R_8723B_F_CUT = 0x410E,
	VERSION_TEST_CHIP_8188E = 0x0003,
	VERSION_TEST_CHIP_8188E_I_CUT = 0x8003,
	VERSION_NORMAL_CHIP_8188E = 0x000b,
	VERSION_NORMAL_CHIP_8188E_I_CUT = 0x800b,
	VERSION_TEST_CHIP_1T1R_8814 = 0x0007,
	VERSION_NORMAL_CHIP_1T1R_8814 = 0x000f,
	VERSION_TEST_CHIP_3T3R_8814 = 0x0037,
	VERSION_NORMAL_CHIP_3T3R_8814 = 0x003f,
	VERSION_NORMAL_CHIP_3T3R_8814_B_CUT = 0x103f,

	// New format used since 8703B
	/* 8821B series */
	VERSION_NORMAL_TSMC_CHIP_1T1R_8821B = 0x00010009,
	VERSION_NORMAL_SMIC_CHIP_1T1R_8821B = 0x00011009,
	VERSION_NORMAL_UMC_CHIP_1T1R_8821B	= 0x00012009,
	
	/* 8822B series */
	VERSION_NORMAL_SMIC_CHIP_1T1R_8822B = 0x0001100A,
	VERSION_NORMAL_SMIC_CHIP_2T2R_8822B = 0x0001100A,	

	/* 8703B series */
	VERSION_NORMAL_SMIC_CHIP_1T1R_8703B = 0x0001200B,
	VERSION_NORMAL_SMIC_CHIP_1T1R_8188F = 0x0001200B,
	/* 8723D series */
	VERSION_NORMAL_SMIC_CHIP_1T1R_8723D = 0x0001100D,

}VERSION_8192C,*PVERSION_8192C;

typedef enum _VERSION_CVID{
	VERSION_1_BEFORE_8703B,
	VERSION_2_SINCE_8703B,
}VERSION_CVID, *PVERSION_CVID;

#define CHANNEL_MAX_NUMBER			14+24+21	// 14 + 24 + 14 + 7
#define CHANNEL_MAX_NUMBER_2G		14
#define CHANNEL_MAX_NUMBER_5G		49			// 28 + 14 + 7 = 49
#define CHANNEL_MAX_NUMBER_5G_80M	7			
#define CHANNEL_GROUP_MAX				3+9	// ch1~3, ch4~9, ch10~14 total three groups
#define MAX_PG_GROUP 				13

//=========================================

VOID
HAL_CmnInitPGData(
	IN	PADAPTER		Adapter
	);

VOID 
HAL_DiffTXDummyLen(
	IN PADAPTER			Adapter
);

VOID 
HAL_DiffTXRXLen(
	IN PADAPTER			Adapter
);

VOID
HAL_ReadTypeIDbyEfuse(
	IN	PADAPTER	Adapter
	);

RT_STATUS 
HAL_ReadTypeID(
	IN	PADAPTER	Adapter
	);


VOID
HAL_SetInterfaceIndex(
	IN	PADAPTER	Adapter,
	IN	u1Byte		InterfaceIdx
	);

VOID
HAL_TxAggregation(
	IN	PADAPTER		Adapter,
	IN	u1Byte			queueId
);


VOID
HAL_HiddenSSIDHandleBeforeScan(
	IN 	PADAPTER	Adapter
);

VOID
HAL_SetCorrentChannelAndWirelessModeBeforeScan(
	IN	PADAPTER	Adapter
);

BOOLEAN
HAL_SwitchTo20MHzBeforeScan(
	IN	PADAPTER	Adapter
);


VOID
HAL_ChannelAndBandWidthRecoveryAfterScan(
	IN	PADAPTER	Adapter
);

u1Byte
MptToMgntRate(
	IN 	ULONG	MptRateIdx
);

BOOLEAN
HAL_IsLegalChannel(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		Channel
	);
BOOLEAN
HAL_Support_BW_Advancesetting(
    IN	PADAPTER	Adapter
    );
VOID
HAL_SetTxPowerForAllRate(
	IN	PADAPTER	Adapter,
	IN	ULONG		ulTxPowerData
	);

VOID 
HAL_ReadCloudKey_Ex(
	IN		PADAPTER	Adapter,
	IN		u1Byte		Length,
	IN		u1Byte		offset,
	OUT		u1Byte		*pCloudKey
	);
VOID
HAL_BitMaskWrite1Byte(
	IN	PADAPTER	Adapter,
	IN	u4Byte		regAddr,
	IN	u1Byte		bitMask,
	IN	u1Byte		data
	);
VOID
HAL_BitMaskWrite2Byte(
	IN	PADAPTER	Adapter,
	IN	u4Byte		regAddr,
	IN	u2Byte		bitMask,
	IN	u2Byte		data
	);
VOID
HAL_BitMaskWrite4Byte(
	IN	PADAPTER	Adapter,
	IN	u4Byte		regAddr,
	IN	u4Byte		bitMask,
	IN	u4Byte		data
	);

VOID
HAL_GetHwId(
	IN	PADAPTER	Adapter,
	IN	pu2Byte		pVid,
	IN	pu2Byte		pDid,
	IN	pu2Byte		pSvid,
	IN	pu2Byte		pSmid
	);

VOID
HAL_GetRegulation(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pRegulation24G,
	IN	pu1Byte		pRegulation5G
	);

BOOLEAN
HAL_RegPollAndWait(
	IN	PADAPTER	Adapter,
	IN	u1Byte		pollType,
	IN	u4Byte		timeoutVal,	// unit: ms
	IN	u4Byte		regAddr,
	IN	u4Byte		equValue,
	IN	u4Byte		bitMask
	);

BOOLEAN
HAL_IsTxOwnBitFramework(
	IN	PADAPTER	Adapter
	);

BOOLEAN
HAL_IsOld8051Series(
	IN	PADAPTER	Adapter
	);
	
VOID
EXhalcommon_CheckHang(
	IN	PADAPTER	Adapter
	);
VOID
EXhalcommon_CheckEDCCA(
	IN	PADAPTER	Adapter
	);
u4Byte
EXhalcommon_GetFalseAlmCnt(
	IN	PADAPTER		Adapter,
	IN	u1Byte			type
	);
s4Byte
EXhalcommon_GetWifiRssi(
	IN	PADAPTER	Adapter
	);
u4Byte
EXhalcommon_GetH2cStatistics(
	IN	PADAPTER	Adapter,
	IN	u1Byte		index
	);
u4Byte
EXhalcommon_GetOdmAbility(
	IN	PADAPTER	Adapter
	);
VOID
DropPacketWorkitemCallback(
	IN	PVOID	Context
	);
VOID
ResumeTxBeacon_92C(
	IN	PADAPTER	Adapter
	);

//
// Runtime D3 related function handler, added by Roger, 2013.02.05.
//
#if INTEL_RTD3_SUPPORT

VOID
HalRuntimeD3Enter(
	PADAPTER		Adapter
	);

VOID
HalRuntimeD3Leave(
	PADAPTER		Adapter
	);

#endif

VOID
HAL_8051Reset(
	IN	PADAPTER		Adapter
	);

VOID
HAL_8051PauseIOWrapper(
	IN	PADAPTER		Adapter
);

VOID
HAL_8051EnableIOWrapper(
	IN	PADAPTER		Adapter
);

u1Byte
HAL_GetSDRPErrorHandlingType(
	IN	PADAPTER			pAdapter,
	IN	u4Byte				SdioCmdAddr
);

VOID
HAL_SetWoWLANCAMEntry(
	IN PADAPTER		Adapter
	);

VOID
HAL_ClearGPIOWakeHost(
	IN PADAPTER		Adapter
	);

typedef struct _Path_Diversity_
{
	//Path Diversity
	u4Byte		OFDMTXPath;
	u4Byte		CCKTXPath;
	u1Byte		DefaultRespPath;
	u1Byte		OFDMDefaultRespPath;
	u1Byte		CCKDefaultRespPath;
	u1Byte		TrainingPath;
	u1Byte		TrainingState;
	s4Byte		RSSI_CCK_Path[2];
	s4Byte		RSSI_CCK_Path_cnt[2];
	u4Byte		OFDM_Pkt_Cnt;
	u4Byte		CCK_Pkt_Cnt;
	u1Byte		Timer;
	u1Byte		PathDiv_NoLink_State;
	u1Byte		CCKPathDivEnable;
	
}PD_T, *pPD_T;


//------------------------------------------------------------------------
// Temp H2C cmd functions for common
// These prototypes should be removed after H2C cmd functions are integrated.
//------------------------------------------------------------------------
VOID
HAL_H2CSetFwGlobalInfoCmd(
	IN PADAPTER		Adapter
	);

VOID
HAL_H2CSetFWWoWlanMode(
	IN PADAPTER		Adapter,
	IN BOOLEAN		bFuncEn
	);

VOID
HAL_H2CSetFwKeepAliveCmd(
	IN PADAPTER		Adapter,
	IN BOOLEAN		bFuncEn
	);

VOID
HAL_H2CSetFwDisconnectDecisionCtrlCmd(
	IN PADAPTER		Adapter,
	IN BOOLEAN		bFuncEn
	);

VOID
HAL_H2CSetFwScanOffloadCtrlCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Type,
	IN u1Byte		ScanOffloadEnable,
	IN u1Byte		NLOEnable
	);

VOID
HAL_H2CSetFwRemoteWakeCtrlCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Enable
	);

VOID
HAL_H2CSetFwInactivePSCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Enable,
	IN BOOLEAN		bActiveWakeUp,
	IN BOOLEAN		bForceClkOff
	);

VOID
Hal_InitVars(
	IN PADAPTER		Adapter
);

VOID
Hal_InitCamEntry(
	IN PADAPTER		Adapter
);

u1Byte
HAL_QueryBeamformerCap(
	IN PADAPTER			Adapter
);

u1Byte
HAL_QueryBeamformeeCap(
	IN PADAPTER			Adapter
);

VOID
HAL_DumpHwAllRegisters(
	IN	PADAPTER	Adapter
	);

VOID
Hal_SetRsvdCtrl(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		BitMap
	);

VOID
Hal_ClearRsvdCtrl(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		BitMap
	);

typedef struct _HW_REG_DEFINE{
	u4Byte	regOffset;
	u4Byte	byteNum;
} HW_REG_DEFINE, *PHW_REG_DEFINE;

typedef struct _HW_REG_PREACTION{ 
	u4Byte	writeRegOffset;
	u4Byte	byteNum;
	u4Byte	bitMask;
	u4Byte	value;
	u4Byte	delayMs;
	BOOLEAN	bRestore;
	u4Byte	originalValue;
} HW_REG_PREACTION, *PHW_REG_PREACTION;

typedef struct _HW_REG_VALUE{
	u4Byte	regOffset;
	u4Byte	byteNum;
	u4Byte	regValue;
} HW_REG_VALUE, *PHW_REG_VALUE;

#endif // #ifndef __INC_HALDEF_H
