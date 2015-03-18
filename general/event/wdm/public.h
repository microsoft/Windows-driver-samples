/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Event.h

--*/

#ifndef __PUBLIC__
#define __PUBLIC__


#include "devioctl.h"
#include <dontuse.h>

typedef enum {
    IRP_BASED , 
    EVENT_BASED 
} NOTIFY_TYPE;

typedef struct _REGISTER_EVENT 
{
    NOTIFY_TYPE Type;
    HANDLE  hEvent;
    LARGE_INTEGER DueTime; // requested DueTime in 100-nanosecond units

} REGISTER_EVENT , *PREGISTER_EVENT ;

#define SIZEOF_REGISTER_EVENT  sizeof(REGISTER_EVENT )


#define IOCTL_REGISTER_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )


#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME       "event"

#endif // __PUBLIC__
