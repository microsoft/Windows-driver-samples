/*++

Copyright (c) 1990-2003 Microsoft Corporation
All Rights Reserved


Module Name:

    cpsuidat.h


Abstract:

    This module


[Environment:]

    Windows 2000, Windows XP, Windows Server 2003 - Common Printer Driver UI DLL


--*/


#define COUNT_ARRAY(a)                  (sizeof(a) / sizeof((a)[0]))


#define OPTIF_NONE                      0

#define LEVEL_0                         0
#define LEVEL_1                         1
#define LEVEL_2                         2
#define LEVEL_3                         3
#define LEVEL_4                         4


#define DMPUB_TVTEST                    (DMPUB_USER +  1)
#define DMPUB_TVOPT                     (DMPUB_USER +  2)
#define DMPUB_TVOPT_ECB_EP              (DMPUB_USER +  3)
#define DMPUB_TVOPT_ICONS               (DMPUB_USER +  4)
#define DMPUB_TVOPT_DISABLED            (DMPUB_USER +  5)
#define DMPUB_TVOPT_TEST                (DMPUB_USER +  6)
#define DMPUB_2STATES                   (DMPUB_USER +  7)
#define DMPUB_3STATES                   (DMPUB_USER +  8)
#define DMPUB_UDARROW                   (DMPUB_USER +  9)
#define DMPUB_TRACKBAR                  (DMPUB_USER + 10)
#define DMPUB_SCROLLBAR                 (DMPUB_USER + 11)
#define DMPUB_LISTBOX                   (DMPUB_USER + 12)
#define DMPUB_COMBOBOX                  (DMPUB_USER + 13)
#define DMPUB_EDITBOX                   (DMPUB_USER + 14)
#define DMPUB_PUSHBUTTON                (DMPUB_USER + 15)
#define DMPUB_CHKBOX                    (DMPUB_USER + 16)
#define DMPUB_CHKBOX_TYPE               (DMPUB_USER + 17)
#define DMPUB_TVOPT_OVERLAY_WARNING     (DMPUB_USER + 18)
#define DMPUB_TVOPT_OVERLAY_STOP        (DMPUB_USER + 19)
#define DMPUB_TVOPT_OVERLAY_NO          (DMPUB_USER + 20)
#define DMPUB_EXT_OVERLAY_WARNING       (DMPUB_USER + 21)
#define DMPUB_EXT_OVERLAY_STOP          (DMPUB_USER + 22)
#define DMPUB_EXT_OVERLAY_NO            (DMPUB_USER + 23)
#define DMPUB_EXT_DISABLED              (DMPUB_USER + 24)
#define DMPUB_MINRANGE                  (DMPUB_USER + 25)
#define DMPUB_MAXRANGE                  (DMPUB_USER + 26)

#define MAX_INT                         0x00007fff
#define MIN_INT                         0xFFFF8000



BOOL
SetupComPropSheetUI
(
    _In_ PCOMPROPSHEETUI pCPSUI
);
