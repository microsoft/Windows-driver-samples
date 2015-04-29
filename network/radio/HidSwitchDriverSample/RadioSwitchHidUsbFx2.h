#ifndef _RADIOSWITCHHIDUSBFX2_H_
#define _RADIOSWITCHHIDUSBFX2_H_

#include <initguid.h>
#include <wdm.h>
#include "usbdi.h"
#include "usbdlib.h"
#include <wdf.h>
#include "wdfusb.h"
#include <hidport.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include "trace.h"


#define HID_CLASS_SPEC_RELEASE_1_00         0x0100  // HID Class Specification release 1.00
#define HID_COUNTRY_NON_LOCALIZED           0x00    // Hardware is not localized

#define GENERIC_DESKTOP_REPORT_ID           0x01

#define GENERIC_DESKTOP_USAGE_PAGE          0x01
#define WIRELESS_RADIO_CONTROLS_USAGE       0x0C
#define WIRELESS_RADIO_BUTTON_USAGE         0xC6
#define WIRELESS_RADIO_LED_USAGE            0xC7
#define WIRELESS_RADIO_SLIDER_USAGE         0xC8


#define MODE_SELECTION_BUTTONS_BIT_MASK     0xE0
#define RADIO_SWITCH_BUTTONS_BIT_MASK       0x01
#define SWITCHPACK_SELECTION_FOR_MODE2      0x40
#define SWITCHPACK_SELECTION_FOR_MODE3      0x60
#define SWITCHPACK_SELECTION_FOR_MODE4      0x80
#define SWITCHPACK_SELECTION_FOR_MODE5      0xA0

#define INTERRUPT_ENDPOINT_INDEX            (0)

#define SWITCHPACK_DEBOUNCE_TIME            10 // in millseconds

// Define the vendor commands supported by device
#define HIDFX2_READ_SWITCH_STATE          0xD6
#define HIDFX2_READ_7SEGMENT_DISPLAY      0xD4
#define HIDFX2_READ_BARGRAPH_DISPLAY      0xD7
#define HIDFX2_SET_BARGRAPH_DISPLAY       0xD8
#define HIDFX2_IS_HIGH_SPEED              0xD9
#define HIDFX2_REENUMERATE                0xDA
#define HIDFX2_SET_7SEGMENT_DISPLAY       0xDB

// Bit masks for 7 segment display to be used with vendor command
#define SEG_TOP         0x1
#define SEG_R_UPPER     0x2
#define SEG_R_LOWER     0x4
#define SEG_DOT         0x8
#define SEG_L_LOWER     0x10
#define SEG_MID         0x20
#define SEG_L_UPPER     0x40
#define SEG_BASE        0x80

#define SEGMENT_DISPLAY_1   SEG_R_UPPER | SEG_R_LOWER
#define SEGMENT_DISPLAY_2   SEG_TOP | SEG_R_UPPER | SEG_MID | SEG_L_LOWER | SEG_BASE
#define SEGMENT_DISPLAY_3   SEG_TOP | SEG_R_UPPER | SEG_MID | SEG_R_LOWER | SEG_BASE
#define SEGMENT_DISPLAY_4   SEG_R_UPPER | SEG_R_LOWER | SEG_MID | SEG_L_UPPER
#define SEGMENT_DISPLAY_5   SEG_TOP | SEG_R_LOWER | SEG_MID | SEG_L_UPPER | SEG_BASE
#define SEGMENT_DISPLAY_6   SEG_TOP | SEG_R_LOWER | SEG_L_LOWER | SEG_MID | SEG_L_UPPER | SEG_BASE
#define SEGMENT_DISPLAY_7   SEG_TOP | SEG_R_UPPER | SEG_R_LOWER
#define SEGMENT_DISPLAY_8   SEG_TOP | SEG_R_UPPER | SEG_R_LOWER | SEG_L_UPPER | SEG_L_LOWER | SEG_MID | SEG_BASE

#define BARGRAPH_LED_ALL_ON     0xff
#define BARGRAPH_LED_ALL_OFF    0x00


typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

#include <pshpack1.h>   // make sure packing is handled correctly

typedef struct _HIDFX2_IO_REPORT
{
    unsigned char bReportId;                //Report ID for the collection
    unsigned char bData;                    //one-byte data for switch/LED
}HIDFX2_IO_REPORT, *PHIDFX2_IO_REPORT;


typedef enum _DRIVER_MODE
{
    DM_BUTTON = 1,
    DM_BUTTON_AND_LED,
    DM_SLIDER_SWITCH,
    DM_SLIDER_SWITCH_AND_LED,
    DM_LED_ONLY
} DRIVER_MODE;

#include <poppack.h>

typedef struct _DEVICE_EXTENSION
{
    WDFUSBDEVICE    hUsbDevice;                 //WDF handles for USB Target 
    WDFUSBINTERFACE hUsbInterface;
    WDFUSBPIPE      hInterruptPipe;
    WDFMEMORY       hDeviceDescriptor;          // Device descriptor for the USB device
    unsigned char   bCurrentSwitchState;        // Switch state.
    unsigned char   bLatestToggledSwitch;       // State for the switch that got toggled most recently
    BOOLEAN         fIsPowerUpSwitchState;      //Interrupt endpoints sends switch state when first started .we ignore this
    WDFTIMER        hDebounceTimer;             // Handle debouncing of switchpack
    WDFQUEUE        hInterruptMsgQueue;         // WDF Queue for read IOCTLs from hidclass that get satisfied from USB interrupt endpoint
    DRIVER_MODE     driverMode;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceContext)


//
// Driver routines
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD HidFx2EvtDeviceAdd;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL HidFx2EvtInternalDeviceControl;

EVT_WDF_DEVICE_PREPARE_HARDWARE HidFx2EvtDevicePrepareHardware;

EVT_WDF_DEVICE_D0_ENTRY HidFx2EvtDeviceD0Entry;

EVT_WDF_DEVICE_D0_EXIT HidFx2EvtDeviceD0Exit;

EVT_WDF_USB_READER_COMPLETION_ROUTINE HidFx2EvtUsbInterruptPipeReadComplete;

EVT_WDF_TIMER HidFx2EvtTimerFunction;

EVT_WDF_OBJECT_CONTEXT_CLEANUP HidFx2EvtDriverContextCleanup;

EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE HidFx2EvtIoCanceledOnQueue;


NTSTATUS HidFx2GetHidDescriptor(_In_ WDFDEVICE hDevice, _In_ WDFREQUEST hRequest);

NTSTATUS HidFx2GetReportDescriptor(_In_ WDFDEVICE hDevice, _In_ WDFREQUEST hRequest);

NTSTATUS HidFx2GetDeviceAttributes(_In_ WDFREQUEST hRequest);

NTSTATUS HidFx2ConfigContReaderForInterruptEndPoint(PDEVICE_EXTENSION pDeviceContext);

NTSTATUS HidFx2SendIdleNotification(_In_ WDFREQUEST hRequest);

NTSTATUS HidFx2SetOutput(_In_ WDFREQUEST hRequest);

NTSTATUS HidFx2GetInput(_In_ WDFREQUEST hRequest);

NTSTATUS HidFx2GetSwitchState(_In_ WDFDEVICE hDevice, _Out_ unsigned char *pbSwitchState);

NTSTATUS SendVendorCommand(_In_ WDFDEVICE hDevice, _In_ unsigned char bVendorCommand, _In_ unsigned char bCommandData);

PCWSTR DbgDevicePowerString(_In_ WDF_POWER_DEVICE_STATE powerState);

#endif   //_RADIOSWITCHHIDUSBFX2_H_


