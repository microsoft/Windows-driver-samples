/****************************** Module Header ******************************\
* Module Name: KBDFR.H
*
* keyboard layout header for French
*
* Copyright (c) 1985-2000, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
*
* created by KBDTOOL v3.11 Thu Aug 24 18:10:18 2000
*
\***************************************************************************/

/*
 * kbd type should be controlled by cl command-line argument
 */
#define KBD_TYPE 4

/*
* Include the basis of all keyboard table values
*/
#include "kbd.h"
#include <dontuse.h>
/***************************************************************************\
* The table below defines the virtual keys for various keyboard types where
* the keyboard differ from the US keyboard.
*
* _EQ() : all keyboard types have the same virtual key for this scancode
* _NE() : different virtual keys for this scancode, depending on kbd type
*
*     +------+ +----------+----------+----------+----------+----------+----------+
*     | Scan | |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |
*     | code | |   type 1 |   type 2 |   type 3 |   type 4 |   type 5 |   type 6 |
\****+-------+_+----------+----------+----------+----------+----------+----------+*/

#undef  T29
#define T29 _EQ(                                      OEM_7                      )
#undef  T0C
#define T0C _EQ(                                      OEM_4                      )
#undef  T10
#define T10 _EQ(                                        'A'                      )
#undef  T11
#define T11 _EQ(                                        'Z'                      )
#undef  T1A
#define T1A _EQ(                                      OEM_6                      )
#undef  T1B
#define T1B _EQ(                                      OEM_1                      )
#undef  T1E
#define T1E _EQ(                                        'Q'                      )
#undef  T27
#define T27 _EQ(                                        'M'                      )
#undef  T28
#define T28 _EQ(                                      OEM_3                      )
#undef  T2C
#define T2C _EQ(                                        'W'                      )
#undef  T32
#define T32 _EQ(                                  OEM_COMMA                      )
#undef  T33
#define T33 _EQ(                                 OEM_PERIOD                      )
#undef  T34
#define T34 _EQ(                                      OEM_2                      )
#undef  T35
#define T35 _EQ(                                      OEM_8                      )

