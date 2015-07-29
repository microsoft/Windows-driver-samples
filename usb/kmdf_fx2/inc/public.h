/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    public.h

Abstract:

Environment:

    User & Kernel mode

--*/

#ifndef _PUBLIC_H
#define _PUBLIC_H

#include <initguid.h>

// {573E8C73-0CB4-4471-A1BF-FAB26C31D384}
DEFINE_GUID(GUID_DEVINTERFACE_OSRUSBFX2, 
            0x573e8c73, 0xcb4, 0x4471, 0xa1, 0xbf, 0xfa, 0xb2, 0x6c, 0x31, 0xd3, 0x84);

#pragma warning(push)
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

//
// Define the structures that will be used by the IOCTL 
//  interface to the driver
//

//
// BAR_GRAPH_STATE
//
// BAR_GRAPH_STATE is a bit field structure with each
//  bit corresponding to one of the bar graph on the 
//  OSRFX2 Development Board
//
#include <pshpack1.h>
typedef struct _BAR_GRAPH_STATE {

    union {
 
        struct {
            //
            // Individual bars starting from the 
            //  top of the stack of bars 
            //
            // NOTE: There are actually 10 bars, 
            //  but the very top two do not light
            //  and are not counted here
            //
            UCHAR Bar1 : 1;
            UCHAR Bar2 : 1;
            UCHAR Bar3 : 1;
            UCHAR Bar4 : 1;
            UCHAR Bar5 : 1;
            UCHAR Bar6 : 1;
            UCHAR Bar7 : 1;
            UCHAR Bar8 : 1;
        };

        //
        // The state of all the bar graph as a single
        // UCHAR
        //
        UCHAR BarsAsUChar;

    };

}BAR_GRAPH_STATE, *PBAR_GRAPH_STATE;

//
// SWITCH_STATE
//
// SWITCH_STATE is a bit field structure with each
//  bit corresponding to one of the switches on the 
//  OSRFX2 Development Board
//
typedef struct _SWITCH_STATE {

    union {
        struct {
            //
            // Individual switches starting from the 
            //  left of the set of switches
            //
            UCHAR Switch1 : 1;
            UCHAR Switch2 : 1;
            UCHAR Switch3 : 1;
            UCHAR Switch4 : 1;
            UCHAR Switch5 : 1;
            UCHAR Switch6 : 1;
            UCHAR Switch7 : 1;
            UCHAR Switch8 : 1;
        };

        //
        // The state of all the switches as a single
        // UCHAR
        //
        UCHAR SwitchesAsUChar;

    };


}SWITCH_STATE, *PSWITCH_STATE;

#include <poppack.h>

#pragma warning(pop)

#define IOCTL_INDEX             0x800
#define FILE_DEVICE_OSRUSBFX2   0x65500

#define IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_OSRUSBFX2,     \
                                                     IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_READ_ACCESS)
                                                   
#define IOCTL_OSRUSBFX2_RESET_DEVICE  CTL_CODE(FILE_DEVICE_OSRUSBFX2,     \
                                                     IOCTL_INDEX + 1, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_REENUMERATE_DEVICE  CTL_CODE(FILE_DEVICE_OSRUSBFX2, \
                                                    IOCTL_INDEX  + 3,  \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX  + 4, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)


#define IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 5, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)


#define IOCTL_OSRUSBFX2_READ_SWITCHES   CTL_CODE(FILE_DEVICE_OSRUSBFX2, \
                                                    IOCTL_INDEX + 6, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)


#define IOCTL_OSRUSBFX2_GET_7_SEGMENT_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2, \
                                                    IOCTL_INDEX + 7, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)


#define IOCTL_OSRUSBFX2_SET_7_SEGMENT_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2, \
                                                    IOCTL_INDEX + 8, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 9, \
                                                    METHOD_OUT_DIRECT, \
                                                    FILE_READ_ACCESS)

#endif
