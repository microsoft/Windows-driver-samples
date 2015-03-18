/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    lsidmi.h

Abstract:

    Private header file for lsi_u3.sys modules.  This contains private
    structure and function declarations as well as constant values which do
    not need to be exported.

Environment:

    kernel mode only

Notes:


Revision History:

--*/


/*
*************************************************************************
*                                                                       *
*   Copyright 1994-2008 LSI Corporation. All rights reserved.           *
*                                                                       *
************************************************************************/


#ifndef _LSIDMI_
#define _LSIDMI_


// IO Control Codes
// bit 31 of this code must be on to denote it is a user-defined code
#define DMI_GET_DATA    0x80444D49      // last 3 bytes are 'DMI'
#define NVCONFIG_IOCTL  0x804E5643      // last 3 bytes are 'NVC'

// DMI data structure for retrieval by IOCTL function
// Original structure is version 4.00 (IOCTL Signature)
typedef struct _DMI_DATA
{
    UCHAR DevSpeed[SYM_MAX_TARGETS];    // sync speed as in SYMNVM.H
    UCHAR DevWidth[SYM_MAX_TARGETS];    // width in bits
    UCHAR MaxAttachments;
    UCHAR MaxXferRate;                  // sync speed as in SYMNVM.H
    UCHAR MaxWidth;                     // width in bits
    UCHAR DeviceId;                     // PCI device ID
    UCHAR HW_Revision;                  // bits 7-4 of CTEST3 >> 4
    UCHAR ScsiBusId;                    // adapter SCSI ID
    UCHAR SignalType;                   // bits 7-6 of STEST4 >> 6
    UCHAR DriverVersion[80];            // version string as in .rc file
// Version 4.01 adds boolean (uchar) flag for PCI Hot Swap
// Set when power cycle occurs, cleared when IOCTL call reads data
    BOOLEAN HotSwap;                    // TRUE if hot swap has occurred
} DMI_DATA, *PDMI_DATA;

// SrbBuffer structure to handle IO_Control call
typedef struct {
    SRB_IO_CONTROL sic;
    UCHAR ucDataBuffer[512];
} SRB_BUFFER, *PSRB_BUFFER;

#endif

// return codes for NVConfig utility
#define SRB_GOOD_ACCESS         0x00000000
#define SRB_ERROR_NO_NVM        0x00000001
#define SRB_ERROR_ON_READ       0x00000002
#define SRB_ERROR_ON_WRITE      0x00000003
