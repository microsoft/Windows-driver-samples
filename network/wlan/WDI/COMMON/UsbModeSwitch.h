#ifndef __INC_USBMODESWITCH_H
#define __INC_USBMODESWITCH_H

typedef enum _USB_MODE
{
	USB_MODE_AUTO, 
	USB_MODE_U2, 
	USB_MODE_U3,
}USB_MODE, *PUSB_MODE;


typedef struct _USB_MODE_MECH
{
	BOOLEAN			bUsbModeMechOn;
	u1Byte			CurUsbMode; // USB mode read from HW when initialization. This represent real USB mode now. (0:unknown, 1:usb2, 2: usb3)
	u1Byte			RegUsbMode; // USB mode read from Registry when inialization. This represent USB mode set last time. (0:unknown, 1:usb2, 2: usb3)
	u1Byte			SetUsbMode; // USB mode set. This represent USB mode going to be set be the mechanism. (0:unknown, 1:usb2, 2: usb3)
	u1Byte			HubUsbMode;	// USB mode set. This represent USB mode going to be set be the mechanism. (0:unknown, 1:usb2, 2: usb3)
	u1Byte			RegForcedUsbMode; //Forced USB mode. (0:not define, 1:usb2, 2: usb3)
	u1Byte			RegUsbCurMode; 		//Current USB mode. (0:not define, 1:usb2, 2: usb3)
	u1Byte			RegUsbSwFast;			// COntrol USB switch ability.
	u1Byte			RegUsbWps;				// Check if WPS will not return USB switch true;
	u1Byte			RegUsbSp;				// Thss round is enabled ue to usb switch and only scan one channel.
	u1Byte			RegUsbChnl;				// After USB switch, we only need to scan the channel at first.
	u1Byte			RegUsbSwitchSpeed;		// USB switch speed level
	u1Byte			RegUsbSwitchThL;		// RSSI Low Threshold to Switch USB
	u1Byte			RegUsbSwitchThH;	// RSSI High Threshold to Switch USB
	u1Byte			RegUsbSwitchThRssi;	// RSSI thresh for first link
	u1Byte			RegUsbSwBy;		// USB switch by service or driver itself.
	u1Byte			RegUsbRfSet;		// USB switch by RF setting
	u1Byte			UsbSwitchRefreshCnt;		// USB switch fast mode thresh value.
	u1Byte			UsbSwitchByCnt;		// USB switch fast mode thresh value.
	u8Byte			Usb2BitMap;			// FOr controlling USB2.0 TX SS power index utility
	u1Byte			Usb2PwrIdx;
	BOOLEAN			bHardToRaise;
	BOOLEAN			bWpsRunning;
	BOOLEAN			bFirstScanOver;  //To distinguish NEC WPS dummy profile
	RT_WORK_ITEM	UsbModeSwitchWorkitem;
	RT_TIMER		UsbModeSwitchTimer;
	u1Byte			SwByUIChnl;
	BOOLEAN			bPauseEDCCAForConnection;
} USB_MODE_MECH, *PUSB_MODE_MECH;

#define UsbModeSwitch_SetUsbModeMechOn(_Adapter, _UsbModeOn)	\
		(_Adapter)->UsbModeMechanism.bUsbModeMechOn = (_UsbModeOn)
#define UsbModeSwitch_SetRegUsbMode(_Adapter, _RegUsbMode)	\
		(_Adapter)->UsbModeMechanism.RegUsbMode = (_RegUsbMode)
#define UsbModeSwitch_SetCurUsbMode(_Adapter, _CurUsbMode)	\
		(_Adapter)->UsbModeMechanism.CurUsbMode = (_CurUsbMode)
#define UsbModeSwitch_SetForcedUsbMode(_Adapter, _ForcedUsbMode)	\
		(_Adapter)->UsbModeMechanism.RegForcedUsbMode = (_ForcedUsbMode)
#define	UsbModeSwitch_SetCurrentUsbMode(_Adapter, _CurrentUsbMode)	\
		(_Adapter)->UsbModeMechanism.RegUsbCurMode = (_CurrentUsbMode)
#define	UsbModeSwitch_SetCurrentHubUsbMode(_Adapter, _CurrentHubUsbMode)	\
		(_Adapter)->UsbModeMechanism.HubUsbMode = (_CurrentHubUsbMode)
#define	UsbModeSwitch_SetUsbRaiseStatus(_Adapter, _bUsbRaiseStatus)	\
		(_Adapter)->UsbModeMechanism.bHardToRaise = (_bUsbRaiseStatus)
#define	UsbModeSwitch_SetUsbWpsStatus(_Adapter, _bWpsStatus)	\
		(_Adapter)->UsbModeMechanism.bWpsRunning = (_bWpsStatus)
#define	UsbModeSwitch_SetFirstScanOver(_Adapter, _bFirstScanOver)	\
		(_Adapter)->UsbModeMechanism.bFirstScanOver = (_bFirstScanOver)
#define	UsbModeSwitch_SetUsbSwitchFast(_Adapter, _UsbSwitchFast)	\
		(_Adapter)->UsbModeMechanism.RegUsbSwFast = (_UsbSwitchFast)	
#define	UsbModeSwitch_SetUsbSwitchWps(_Adapter, _UsbSwitchWps)	\
		(_Adapter)->UsbModeMechanism.RegUsbWps = (_UsbSwitchWps)	
#define	UsbModeSwitch_SetUsbSwitchPostive(_Adapter, _UsbSwitchPositive)	\
		(_Adapter)->UsbModeMechanism.RegUsbSp = (_UsbSwitchPositive)	
#define	UsbModeSwitch_SetUsbSwitchChannel(_Adapter, _UsbSwitchChnl)	\
		(_Adapter)->UsbModeMechanism.RegUsbChnl = (_UsbSwitchChnl)	
#define	UsbModeSwitch_SetUsbSwitchSpeed(_Adapter, _UsbSwitchSpeed)	\
		(_Adapter)->UsbModeMechanism.RegUsbSwitchSpeed = (_UsbSwitchSpeed)	
#define	UsbModeSwitch_SetUsbSwitchThL(_Adapter, _UsbSwitchThL)	\
		(_Adapter)->UsbModeMechanism.RegUsbSwitchThL = (_UsbSwitchThL)	
#define	UsbModeSwitch_SetUsbSwitchThH(_Adapter, _UsbSwitchThH)	\
		(_Adapter)->UsbModeMechanism.RegUsbSwitchThH = (_UsbSwitchThH)
#define	UsbModeSwitch_SetUsbSwitchThRssi(_Adapter, _UsbSwitchThRssi)	\
		(_Adapter)->UsbModeMechanism.RegUsbSwitchThRssi = (_UsbSwitchThRssi)
#define	UsbModeSwitch_SetUsbSwitchBy(_Adapter, _UsbSwitchBy)	\
		(_Adapter)->UsbModeMechanism.RegUsbSwBy = (_UsbSwitchBy)
#define	UsbModeSwitch_SetUsbSwitchRfSet(_Adapter, _UsbSwitchRfSet)	\
		(_Adapter)->UsbModeMechanism.RegUsbRfSet = (_UsbSwitchRfSet)


VOID
UsbModeSwitchInit(
	PADAPTER	Adapter
	);

VOID
UsbModeSwitchDeinit(
	PADAPTER	Adapter
	);

BOOLEAN
UsbModeSwitchCheck(
	PADAPTER	Adapter,
	pu1Byte		BSSID
	);

u4Byte
UsbModeCurrentStateQuery(
	PADAPTER	Adapter
	);

BOOLEAN
UsbModeSwitchStartTransition(
	PADAPTER	Adapter
	);

BOOLEAN
UsbModeSwitchConfirm(
	PADAPTER	Adapter
	);

VOID
UsbModeForcedTransition(
	PADAPTER	Adapter
	);

VOID
UsbModeSetHubUsbType(
	PADAPTER	Adapter,
	pu1Byte		UsbHostType
	);

u1Byte
UsbModeQueryHubUsbType(
	PADAPTER	Adapter
	);

u1Byte
UsbModeQueryDeviceUsbType(
	PADAPTER	Adapter
	);

u1Byte
UsbModeQuerySwitchPositive(
	PADAPTER	Adapter
	);

u1Byte
UsbModeQuerySwitchChannel(
	PADAPTER	Adapter
	);
	
VOID
UsbModeSwitchCheckExt(
	PADAPTER	Adapter
	);

u1Byte
UsbModeQueryRfTransmitPath(
	PADAPTER	Adapter,
	u1Byte		RfType
	);

VOID
UsbModeSelectAMPDUPreTx(
	PADAPTER			Adapter
	);

VOID
UsbModeSelectRaMask(
	PADAPTER			Adapter,
	pu8Byte				BitMap,
	CHANNEL_WIDTH		BandWidth
	);

VOID
UsbModeSelect3ssPwrInU2(
	PADAPTER			Adapter,
	CHANNEL_WIDTH		BandWidth,
	u1Byte				Channel
	);

u1Byte
UsbModeQueryRfSet(
	PADAPTER			Adapter
	);

#endif
