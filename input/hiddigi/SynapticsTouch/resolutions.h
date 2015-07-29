/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        resolutions.h

    Abstract:

        Contains resolution translation defines and types

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

#define TOUCH_SCREEN_PROPERTIES_REG_KEY L"\\Registry\\Machine\\System\\TOUCH\\SCREENPROPERTIES"
#define TOUCH_DEFAULT_RESOLUTION_X  480
#define TOUCH_DEFAULT_RESOLUTION_Y  800
#define TOUCH_DEVICE_RESOLUTION_X   816
#define TOUCH_DEVICE_RESOLUTION_Y   1394

typedef struct _TOUCH_SCREEN_PROPERTIES
{
    ULONG TouchSwapAxes;
    ULONG TouchInvertXAxis;
    ULONG TouchInvertYAxis;
    ULONG TouchPhysicalWidth;
    ULONG TouchPhysicalHeight;
    ULONG TouchPhysicalButtonHeight;
    ULONG TouchPillarBoxWidthLeft;
    ULONG TouchPillarBoxWidthRight;
    ULONG TouchLetterBoxHeightTop;
    ULONG TouchLetterBoxHeightBottom;
    ULONG TouchAdjustedWidth;
    ULONG TouchAdjustedHeight;
    ULONG DisplayPhysicalWidth;
    ULONG DisplayPhysicalHeight;
    ULONG DisplayAdjustedButtonHeight;
    ULONG DisplayPillarBoxWidthLeft;
    ULONG DisplayPillarBoxWidthRight;
    ULONG DisplayLetterBoxHeightTop;
    ULONG DisplayLetterBoxHeightBottom;
    ULONG DisplayAdjustedWidth;
    ULONG DisplayAdjustedHeight;
    ULONG DisplayViewableWidth;
    ULONG DisplayViewableHeight;
} TOUCH_SCREEN_PROPERTIES, *PTOUCH_SCREEN_PROPERTIES;

VOID
TchGetScreenProperties(
    IN PTOUCH_SCREEN_PROPERTIES Props
    );

VOID
TchTranslateToDisplayCoordinates(
    IN PUSHORT X,
    IN PUSHORT Y,
    IN PTOUCH_SCREEN_PROPERTIES Props
    );
