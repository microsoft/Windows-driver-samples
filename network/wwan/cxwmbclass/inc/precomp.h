//
//    Copyright (C) Microsoft.  All rights reserved.
//

#pragma once

#include <ntddk.h>
#include <ntstrsafe.h>

#include <wdm.h>
#include <wdf.h>
#include <wdfminiport.h>
#include <usbspec.h>
#include <usb.h>
#include <usbdlib.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <wdfusb.h>
#include <netadaptercx.h>
#include <netdevice.h>
#include <mbbcx.h>
#include <net/returncontext.h>
#include <net/virtualaddress.h>
#include <net/mdl.h>
#include <ndis.h>

#include <netiodef.h>

#define INITGUID
#include <devpkey.h>

#include <limits.h>

#include "mbbncm.h"

#include "mbbmessages.h"
#define MBB_MAX_NUMBER_OF_SESSIONS 17 // including the default session for the physical/primary interface
#define MBB_DEFAULT_SESSION_ID 0      // for physical/primay interface. Must be 0. Do NOT change it.
#define MBB_INVALID_SESSION_ID MBB_MAX_NUMBER_OF_SESSIONS

#define WMBCLASS_RADIO_STATE_OFF 0
#define WMBCLASS_RADIO_STATE_ON 1

#include "BusInterface.h"

#include "align.h"
#include "device.h"
#include "adapter.h"
#include "data.h"
#include "txqueue.h"
#include "util.h"
#include "usbbus.h"
#include "utils.h"
