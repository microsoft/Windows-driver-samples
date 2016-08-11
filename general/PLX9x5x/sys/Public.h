/*++
    Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/
  
//
// The following value is arbitrarily chosen from the space defined 
// by Microsoft as being "for non-Microsoft use"
//
// NOTE: we use OSR's GUID_OSR_PLX_INTERFACE GUID value so that we 
// can use OSR's PLxTest program      :-)
//
// {29D2A384-2E47-49b5-AEBF-6962C22BD7C2}
DEFINE_GUID (GUID_PLX_INTERFACE, 
   0x29d2a384, 0x2e47, 0x49b5, 0xae, 0xbf, 0x69, 0x62, 0xc2, 0x2b, 0xd7, 0xc2);

//
// This IOCTL toggles the driver's DMA Single Transfer Requirement
// on and off each time it is called.
//
#define IOCTL_PLX9X5X_TOGGLE_SINGLE_TRANSFER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0, METHOD_BUFFERED, FILE_ANY_ACCESS)

