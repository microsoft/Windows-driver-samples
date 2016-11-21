#ifndef __INC_HALCOMLED_H
#define __INC_HALCOMLED_H
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalComLed.h
	
Abstract:
	LED related data structure and interface to manipulate LED.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2012-12-23 Isaiah           Create.
	
--*/

//================================================================================
//	LED Behavior Constant.
//================================================================================
// Default LED behavior.
//
#define LED_BLINK_NORMAL_INTERVAL		100
#define LED_BLINK_SLOWLY_INTERVAL		200

#define LED_BLINK_LONG_INTERVAL			400
#define LED_INITIAL_INTERVAL			1800


// LED Customerization 

//NETTRONIX
#define LED_BLINK_NORMAL_INTERVAL_NETTRONIX	100
#define LED_BLINK_SLOWLY_INTERVAL_NETTRONIX	2000

//PORNET
#define LED_BLINK_SLOWLY_INTERVAL_PORNET	1000
#define LED_BLINK_NORMAL_INTERVAL_PORNET	100
#define LED_BLINK_FAST_INTERVAL_BITLAND		30

//AzWave.
#define LED_CM2_BLINK_ON_INTERVAL		250
#define LED_CM2_BLINK_OFF_INTERVAL		4750
#define LED_CM8_BLINK_OFF_INTERVAL		3750	//for QMI

//RunTop
#define LED_RunTop_BLINK_INTERVAL		300

//ALPHA
#define LED_BLINK_NO_LINK_INTERVAL_ALPHA	1000
#define LED_BLINK_NO_LINK_INTERVAL_ALPHA_500MS 500 //add by ylb 20121012 for customer led for alpha
#define LED_BLINK_LINK_INTERVAL_ALPHA		500	//500
#define LED_BLINK_SCAN_INTERVAL_ALPHA		180 	//150
#define LED_BLINK_FASTER_INTERVAL_ALPHA		50
#define LED_BLINK_WPS_SUCESS_INTERVAL_ALPHA	5000

// 111122 by hpfan: Customized for Xavi
#define LED_CM11_BLINK_INTERVAL			300
#define LED_CM11_LINK_ON_INTERVEL		3000

//Netgear
#define LED_BLINK_LINK_INTERVAL_NETGEAR		500
#define LED_BLINK_LINK_SLOWLY_INTERVAL_NETGEAR		1000

#define LED_WPS_BLINK_OFF_INTERVAL_NETGEAR		100
#define LED_WPS_BLINK_ON_INTERVAL_NETGEAR		500


//Belkin AC950
#define LED_BLINK_LINK_INTERVAL_ON_BELKIN		200
#define LED_BLINK_LINK_INTERVAL_OFF_BELKIN		100
#define LED_BLINK_ERROR_INTERVAL_BELKIN		100

//by chiyokolin for Azurewave
#define LED_CM12_BLINK_INTERVAL_5Mbps		160
#define LED_CM12_BLINK_INTERVAL_10Mbps		80
#define LED_CM12_BLINK_INTERVAL_20Mbps		50
#define LED_CM12_BLINK_INTERVAL_40Mbps		40
#define LED_CM12_BLINK_INTERVAL_80Mbps		30
#define LED_CM12_BLINK_INTERVAL_MAXMbps		25

//ECS
#define LED_BLINK_INTERVAL_OVER_2Mbps		500
#define LED_BLINK_INTERVAL_OVER_1Mbps		1000

//Dlink
#define	LED_BLINK_NO_LINK_INTERVAL		1000
#define	LED_BLINK_LINK_IDEL_INTERVAL		100

#define	LED_BLINK_SCAN_ON_INTERVAL		30
#define	LED_BLINK_SCAN_OFF_INTERVAL		300

#define LED_WPS_BLINK_ON_INTERVAL_DLINK		30
#define LED_WPS_BLINK_OFF_INTERVAL_DLINK			300
#define LED_WPS_BLINK_LINKED_ON_INTERVAL_DLINK			5000

//Mitac
#define LED_BLINK_TRX_INYERVAL_MITAC		500
#define LED_BLINK_ON_OFF_INYERVAL_MITAC		100

//ELECOM
#define LED_BLINK_WPS_INTERVAL_ELECOM				500
#define LED_BLINK_TRX_INTERVAL_ELECOM				100




//================================================================================
// LED object.
//================================================================================

typedef	enum _LED_STATE{
	LED_UNKNOWN = 0,
	LED_ON = 1,
	LED_OFF = 2,
	LED_BLINK_NORMAL = 3,
	LED_BLINK_SLOWLY = 4,
	LED_POWER_ON_BLINK = 5,
	LED_SCAN_BLINK = 6, 	// LED is blinking during scanning period, the # of times to blink is depend on time for scanning.
	LED_NO_LINK_BLINK = 7, // LED is blinking during no link state.
	LED_BLINK_StartToBlink = 8, // Customzied for Sercomm Printer Server case
	LED_BLINK_TXRX = 9,
	LED_BLINK_WPS = 10,	// LED is blinkg during WPS communication
	LED_BLINK_WPS_STOP = 11,	//for ALPHA
	LED_BLINK_WPS_STOP_OVERLAP = 12,	//for BELKIN
	LED_BLINK_RUNTOP = 13, 	// Customized for RunTop
	LED_BLINK_CAMEO = 14,
	LED_BLINK_XAVI = 15,
	LED_BLINK_ALWAYS_ON = 16,	
	LED_BLINK_LINK_IN_PROCESS = 17,  //Customized for Belkin AC950
	LED_BLINK_AUTH_ERROR = 18,  //Customized for Belkin AC950
	LED_BLINK_Azurewave_5Mbps = 19,		
	LED_BLINK_Azurewave_10Mbps = 20,		
	LED_BLINK_Azurewave_20Mbps = 21,		
	LED_BLINK_Azurewave_40Mbps = 22,			
	LED_BLINK_Azurewave_80Mbps = 23,		
	LED_BLINK_Azurewave_MAXMbps = 24,		
	LED_BLINK_LINK_IDEL = 25,	
	LED_BLINK_WPS_LINKED = 26,
	LED_BLINK_TIME_BY_REG = 27,
	LED_BLINK_ECS_Below_1Mbps = 28,
	LED_BLINK_ECS_Over_1Mbps = 29,
	LED_BLINK_ECS_Over_2Mbps = 30,
}LED_STATE;

typedef enum _LED_PIN{
	LED_PIN_GPIO0,
	LED_PIN_LED0,
	LED_PIN_LED1,
	LED_PIN_LED2,
	LED_PIN_LED3,
	LED_PIN_LED_ALL
}LED_PIN;



	

//================================================================================
// SDIO LED Definition.
//================================================================================


#define IS_LED_WPS_BLINKING(_LED_SDIO)	(((PLED_SDIO)_LED_SDIO)->CurrLedState==LED_BLINK_WPS \
					|| ((PLED_SDIO)_LED_SDIO)->CurrLedState==LED_BLINK_WPS_STOP \
					|| ((PLED_SDIO)_LED_SDIO)->bLedWPSBlinkInProgress)

#define IS_LED_BLINKING(_LED_SDIO) 	(((PLED_SDIO)_LED_SDIO)->bLedWPSBlinkInProgress \
					||((PLED_SDIO)_LED_SDIO)->bLedScanBlinkInProgress)


typedef	enum _LED_STRATEGY_SDIO{
	SW_LED_MODE0, // SW control 1 LED via GPIO0. It is default option.
	SW_LED_MODE1, // 2 LEDs, through LED0 and LED1. For ALPHA.
	SW_LED_MODE2, // SW control 1 LED via GPIO0, customized for AzWave 8187 minicard.
	SW_LED_MODE3, // SW control 1 LED via GPIO0, customized for Sercomm Printer Server case.
	SW_LED_MODE4, //for Edimax / Belkin
	SW_LED_MODE5, //for Sercomm / Belkin	
	SW_LED_MODE6,	//for 88CU minicard, porting from ce SW_LED_MODE7
	SW_LED_MODE7,	//for ECS
	HW_LED, // HW control 2 LEDs, LED0 and LED1 (there are 4 different control modes, see MAC.CONFIG1 for details.)
}LED_STRATEGY_SDIO, *PLED_STRATEGY_SDIO;

typedef struct _LED_SDIO{
	PVOID				pAdapter;

	LED_PIN				LedPin;	// Identify how to implement this SW led.

	LED_STATE			CurrLedState; // Current LED state.
	BOOLEAN				bLedOn; // TRUE if LED is ON, FALSE if LED is OFF.

	BOOLEAN				bSWLedCtrl;

	BOOLEAN				bLedBlinkInProgress; // TRUE if it is blinking, FALSE o.w..
	// ALPHA, added by chiyoko, 20090106
	BOOLEAN				bLedNoLinkBlinkInProgress;
	BOOLEAN				bLedLinkBlinkInProgress;
	BOOLEAN				bLedStartToLinkBlinkInProgress;	
	BOOLEAN				bLedScanBlinkInProgress;
	BOOLEAN				bLedWPSBlinkInProgress;
	
	u4Byte				BlinkTimes; // Number of times to toggle led state for blinking.
	LED_STATE			BlinkingLedState; // Next state for blinking, either LED_ON or LED_OFF are.

	RT_TIMER			BlinkTimer; // Timer object for led blinking.
	RT_WORK_ITEM		BlinkWorkItem; // Workitem used by BlinkTimer to manipulate H/W to blink LED. 
} LED_SDIO, *PLED_SDIO;

VOID
LedControlSDIO(
	IN	PADAPTER		Adapter,
	IN	LED_CTL_MODE		LedAction
	);


//================================================================================
// Interface to manipulate LED objects.
//================================================================================

#define SwLedOn(_Adapter, _pLed) \
	if(IS_HARDWARE_TYPE_8814A(_Adapter))	\
		SwLedOn_8814A(_Adapter, _pLed);		\
	else if(IS_HARDWARE_TYPE_JAGUAR(_Adapter)) \
		IS_HARDWARE_TYPE_8821(_Adapter) ? SwLedOn_8821A(_Adapter, _pLed) : SwLedOn_8812A(_Adapter, _pLed);        			\
	else if(IS_HARDWARE_TYPE_8723B(_Adapter)||IS_HARDWARE_TYPE_8192E(_Adapter))											\
		IS_HARDWARE_TYPE_8192E(_Adapter) ? SwLedOn_8192E(_Adapter, _pLed) : SwLedOn_8723B(_Adapter, _pLed);				\
	else if(IS_HARDWARE_TYPE_8821B(_Adapter))											\
		SwLedOn_8821B(_Adapter, _pLed);				\
	else if(IS_HARDWARE_TYPE_8822B(_Adapter))											\
		SwLedOn_8822B(_Adapter, _pLed); 		\
	else if(IS_HARDWARE_TYPE_8703B(_Adapter))			\
		SwLedOn_8703B(_Adapter,_pLed);		\
	else if(IS_HARDWARE_TYPE_8188F(_Adapter))			\
		SwLedOn_8188F(_Adapter,_pLed);		\
	else if(IS_HARDWARE_TYPE_8723D(_Adapter))			\
		SwLedOn_8723D(_Adapter,_pLed);		\
	else	 																													\
		IS_HARDWARE_TYPE_8188E(_Adapter) ? SwLedOn_8188E(_Adapter, _pLed) : SwLedOn_Dummy(_Adapter, _pLed)
		
#define SwLedOff(_Adapter, _pLed) \
	if(IS_HARDWARE_TYPE_8814A(_Adapter))	\
		SwLedOff_8814A(_Adapter, _pLed);		\
	else if(IS_HARDWARE_TYPE_JAGUAR(_Adapter)) 																				\
		IS_HARDWARE_TYPE_8821(_Adapter) ? SwLedOff_8821A(_Adapter, _pLed) : SwLedOff_8812A(_Adapter, _pLed);            		\
	else if(IS_HARDWARE_TYPE_8723B(_Adapter)||IS_HARDWARE_TYPE_8192E(_Adapter))											\
		IS_HARDWARE_TYPE_8192E(_Adapter) ? SwLedOff_8192E(_Adapter, _pLed) : SwLedOff_8723B(_Adapter, _pLed);				\
	else if(IS_HARDWARE_TYPE_8821B(_Adapter))											\
		SwLedOff_8821B(_Adapter, _pLed);				\
	else if(IS_HARDWARE_TYPE_8822B(_Adapter))											\
		SwLedOff_8822B(_Adapter, _pLed);				\
	else if(IS_HARDWARE_TYPE_8703B(_Adapter))			\
		SwLedOff_8703B(_Adapter,_pLed);					\
	else if(IS_HARDWARE_TYPE_8188F(_Adapter))			\
		SwLedOff_8188F(_Adapter,_pLed);					\
	else if(IS_HARDWARE_TYPE_8723D(_Adapter))			\
		SwLedOff_8723D(_Adapter,_pLed);					\
	else																														\
		IS_HARDWARE_TYPE_8188E(_Adapter) ? SwLedOff_8188E(_Adapter, _pLed) : SwLedOff_Dummy(_Adapter, _pLed)
	

VOID
InitSwLeds(
	IN	PADAPTER	Adapter
	);
	
VOID
DeInitSwLeds(
	IN	PADAPTER	Adapter
	);

VOID
SwLedOn_Dummy(
    IN	PADAPTER		Adapter,
    IN	PVOID		pLed
    );

VOID
SwLedOff_Dummy(
    IN	PADAPTER		Adapter,
    IN	PVOID		pLed
    );
	
#endif	/*__INC_HALCOMLED_H*/
