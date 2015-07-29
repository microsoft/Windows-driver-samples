/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    SysvadShared.h

Abstract:

    Header file for common stuffs between sample SWAP APO and the SysVad sample.
*/
#ifndef _SYSVADSHARED_H_
#define _SYSVADSHARED_H_

// {D849C827-B24B-4C3D-A26E-338F4FF07ED5}
//DEFINE_GUID(KSPROPSETID_SysVAD, 0xd849c827, 0xb24b, 0x4c3d, 0xa2, 0x6e, 0x33, 0x8f, 0x4f, 0xf0, 0x7e, 0xd5);


#define STATIC_KSPROPSETID_SysVAD\
    0xd849c827, 0xb24b, 0x4c3d, 0xa2, 0x6e, 0x33, 0x8f, 0x4f, 0xf0, 0x7e, 0xd5
DEFINE_GUIDSTRUCT("D849C827-B24B-4C3D-A26E-338F4FF07ED5", KSPROPSETID_SysVAD);
#define KSPROPSETID_SysVAD DEFINE_GUIDNAMED(KSPROPSETID_SysVAD)


typedef enum{
    KSPROPERTY_SYSVAD_DEFAULTSTREAMEFFECTS
} KSPROPERTY_SYSVAD;

#endif
