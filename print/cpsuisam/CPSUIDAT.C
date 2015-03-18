/*++

Copyright (c) 1990-2003 Microsoft Corporation
All Rights Reserved


Module Name:

    cpsuidat.c


Abstract:

    This module contains all the predefined data


--*/


#include "precomp.h"
#pragma hdrstop

#define DBG_CPSUIFILENAME   DbgTVPage



#define DBG_TVTESTCB        0x00000001
#define DBG_PUSHDLGPROC     0x00000002

DEFINE_DBGVAR(0);

#define ARRAY_SIZE(x)       (sizeof(x)/sizeof(x[0]))


HINSTANCE   hInstApp = NULL;

TCHAR   TitleName[]       = TEXT("Common Property Sheet UI Sample");
TCHAR   ClassName[]       = TEXT("CPSUISampleClass");
TCHAR   MenuName[]        = TEXT("CPSUISampleMenu");
TCHAR   OptItemName[]     = TEXT("CPSUI TreeView Sample");
TCHAR   szWinSpool[]      = TEXT("WinSpool.Drv");
CHAR    szDocPropSheets[] = "DocumentPropertySheets";

BOOL    UpdatePermission = TRUE;
BOOL    UseStdAbout      = TRUE;


INT_PTR
CALLBACK
PushButtonProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

CPSUICALLBACK
TVTestCallBack(
    _In_ PCPSUICBPARAM   pCPSUICBParam
    );


OPTPARAM    NoYesOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        (LPTSTR)IDS_CPSUI_NO,                   // pData
        IDI_CPSUI_NO,                           // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        (LPTSTR)IDS_CPSUI_YES,                  // pData
        IDI_CPSUI_YES,                          // IconID
        1                                       // lParam
    }
};

EXTCHKBOX   ECB_EP_ECB = {

        sizeof(EXTCHKBOX),
        0,
        TEXT("Include Icon"),
        (LPTSTR)IDS_CPSUI_SLASH_SEP,
        TEXT("Icon"),
        IDI_CPSUI_GRAPHIC };


OPTPARAM    ECB_EP_OP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("None"),                           // pData
        IDI_CPSUI_EMPTY,                        // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("Extended CheckBox"),              // pData
        IDI_CPSUI_EMPTY,                        // IconID
        1                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("Extended Push"),                  // pData
        IDI_CPSUI_EMPTY,                        // IconID
        0                                       // lParam
    }
};


OPTTYPE ECB_EP_OT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_3STATES,                           // Type
        0,                                      // Flags OPTTF_xxxx
        3,                                      // Count
        0,                                      // BegCtrlID
        ECB_EP_OP,                              // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTTYPE NoYesOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_2STATES,                           // Type
        0,                                      // Flags OPTTF_xxxx
        2,                                      // Count
        0,                                      // BegCtrlID
        NoYesOP,                                // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTPARAM    TVOT3StatesOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("States 1"),                       // pData
        IDI_CPSUI_PORTRAIT,                     // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("States 2"),                       // pData
        IDI_CPSUI_LANDSCAPE,                    // IconID
        1                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("States 3"),                       // pData
        IDI_CPSUI_ROT_LAND,                     // IconID
        0                                       // lParam
    }
};

OPTTYPE TVOT2StatesOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_2STATES,                           // Type
        0,                                      // Flags OPTTF_xxxx
        2,                                      // Count
        0,                                      // BegCtrlID
        TVOT3StatesOP,                          // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTTYPE TVOT3StatesOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_3STATES,                           // Type
        0,                                      // Flags OPTTF_xxxx
        3,                                      // Count
        0,                                      // BegCtrlID
        TVOT3StatesOP,                          // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTPARAM    MinRangeOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(""),                               // pData (postfix)
        IDI_CPSUI_EMPTY,                        // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        NULL,                                   // pData (help line)
        (DWORD)MIN_INT,                         // IconID
        MAX_INT                                 // lParam
    }
};

OPTTYPE MinRangeOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_UDARROW,                           // Type
        0,                                      // Flags OPTTF_xxxx
        2,                                      // Count
        0,                                      // BegCtrlID
        MinRangeOP,                             // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTPARAM    MaxRangeOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(""),                               // pData (postfix)
        IDI_CPSUI_EMPTY,                        // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        NULL,                                   // pData (help line)
        (DWORD)MIN_INT,                         // IconID
        MAX_INT                                 // lParam
    }
};

OPTTYPE MaxRangeOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_UDARROW,                           // Type
        0,                                      // Flags OPTTF_xxxx
        2,                                      // Count
        0,                                      // BegCtrlID
        MaxRangeOP,                             // pOptParam
        0                                       // Style, OTS_xxxx
    };


OPTPARAM    MinMaxRangeOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("%"),                              // pData (postfix)
        IDI_CPSUI_SCALING,                      // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        NULL,                                   // pData (help line)
        (DWORD)MIN_INT,                         // IconID
        MAX_INT                                 // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        NULL,                                   // pData (help line)
        2,                                      // IconID
        50                                      // lParam
    }
};

OPTTYPE TVOTUDArrowOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_UDARROW,                           // Type
        0,                                      // Flags OPTTF_xxxx
        2,                                      // Count
        0,                                      // BegCtrlID
        MinMaxRangeOP,                          // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTTYPE TVOTTrackBarOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_TRACKBAR,                          // Type
        0,                                      // Flags OPTTF_xxxx
        3,                                      // Count
        0,                                      // BegCtrlID
        MinMaxRangeOP,                         // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTTYPE TVOTScrollBarOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_SCROLLBAR,                         // Type
        0,                                      // Flags OPTTF_xxxx
        3,                                      // Count
        0,                                      // BegCtrlID
        MinMaxRangeOP,                         // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTPARAM    TVOTLBCBOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        (LPTSTR)IDS_CPSUI_NOTINSTALLED,         // pData
        IDI_CPSUI_SEL_NONE,                     // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(" 1 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(" 2 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(" 4 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(" 6 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },
    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT(" 8 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },
    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("10 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },
    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("12 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },
    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("14 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("16 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("18 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },
    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("20 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("24 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("32 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("48 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("64 MB"),                         // pData
        IDI_CPSUI_MEM,                          // IconID
        0                                       // lParam
    },
};


OPTTYPE TVOTListBoxOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_LISTBOX,                           // Type
        0,                                      // Flags OPTTF_xxxx
        COUNT_ARRAY(TVOTLBCBOP),                // Count
        0,                                      // BegCtrlID
        TVOTLBCBOP,                             // pOptParam
        0                                       // Style, OTS_xxxx
    };


OPTTYPE TVOTComboBoxOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_COMBOBOX,                           // Type
        0,                                      // Flags OPTTF_xxxx
        COUNT_ARRAY(TVOTLBCBOP),                // Count
        0,                                      // BegCtrlID
        TVOTLBCBOP,                             // pOptParam
        0                                       // Style, OTS_xxxx
    };

TCHAR   TVOTEditBoxBuf[128] = TEXT("Confidential");

OPTPARAM    TVOTEditBoxOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("*Mark*"),                         // pData
        IDI_CPSUI_WATERMARK,                    // IconID
        0                                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("Type in WaterMark text"),         // pData
        COUNT_ARRAY(TVOTEditBoxBuf),            // IconID
        0                                       // lParam
    }
};


OPTTYPE TVOTEditBoxOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_EDITBOX,                           // Type
        0,                                      // Flags OPTTF_xxxx
        2,                                      // Count
        0,                                      // BegCtrlID
        TVOTEditBoxOP,                          // pOptParam
        0                                       // Style, OTS_xxxx
    };

OPTPARAM    TVOTChkBoxOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        CHKBOXS_FALSE_TRUE,                     // style
        TEXT("Let's do it!"),                   // pData
        IDI_CPSUI_TELEPHONE,                        // IconID
        0                                       // lParam
    }
};



OPTTYPE TVOTChkBoxOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_CHKBOX,                            // Type
        0,                                      // Flags OPTTF_xxxx
        1,                                      // Count
        0,                                      // BegCtrlID
        TVOTChkBoxOP,                           // pOptParam
        0                                       // Style, OTS_xxxx
    };


OPTPARAM    ChkBoxTypeOP[] = {

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_FALSE_TRUE"),             // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_FALSE_TRUE                      // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_NO_YES"),                 // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_NO_YES                          // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_OFF_ON"),                 // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_OFF_ON                          // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_FALSE_PDATA"),            // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_FALSE_PDATA                     // lParam
    },


    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_NO_PDATA"),               // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_NO_PDATA                        // lParam
    },


    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_OFF_PDATA"),              // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_OFF_PDATA                       // lParam
    },

    {
        sizeof(OPTPARAM),                       // cbSize
        0,                                      // OPTPF_xxx
        0,                                      // style
        TEXT("CHKBOXS_NONE_PDATA"),             // pData
        IDI_CPSUI_EMPTY,                        // IconID
        CHKBOXS_NONE_PDATA                      // lParam
    }
};


OPTTYPE ChkBoxTypeOT = {

        sizeof(OPTTYPE),                        // cbSize
        TVOT_LISTBOX,                           // Type
        0,                                      // Flags OPTTF_xxxx
        COUNT_ARRAY(ChkBoxTypeOP),              // Count
        0,                                      // BegCtrlID
        ChkBoxTypeOP,                           // pOptParam
        0                                       // Style, OTS_xxxx
    };

EXTCHKBOX   TV_ECB = {

        sizeof(EXTCHKBOX),
        ECBF_OVERLAY_WARNING_ICON,
        TEXT("Extended CheckBox Test"),
        (LPTSTR)IDS_CPSUI_SLASH_SEP,
        TEXT("Got Checked!!!"),
        IDI_APPLE };

EXTPUSH TV_EP = {

        sizeof(EXTPUSH),
        0,
        TEXT("Extended Push Test"),
        NULL, //DLGPROC
        IDI_APPLE,
        0 };



OPTITEM TVTestOptItems[] = {

    { sizeof(OPTITEM), LEVEL_0, 0, OPTIF_NONE, 0,
      (LPTSTR)TEXT("TreeView Test"), IDI_CPSUI_QUESTION,   NULL,
       NULL, 0, DMPUB_TVTEST },

        { sizeof(OPTITEM), LEVEL_1, 0, OPTIF_COLLAPSE, 0,
          (LPTSTR)TEXT("CPSUI Options"), 0,   NULL,
           NULL, 0, DMPUB_TVOPT },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
              (LPTSTR)TEXT("Extended Type"), 0,   &ECB_EP_ECB,
               &ECB_EP_OT, 0, DMPUB_TVOPT_ECB_EP },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Disabled Extended"), 0,   NULL,
                   &NoYesOT, 0, DMPUB_EXT_DISABLED },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Overlay 'Warning' Icon"), 0,   NULL,
                  &NoYesOT, 0, DMPUB_EXT_OVERLAY_WARNING },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Overlay 'Stop' Icon"), 0,   NULL,
                  &NoYesOT, 0, DMPUB_EXT_OVERLAY_STOP },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Overlay 'No' Icon"), 0,   NULL,
                  &NoYesOT, 0, DMPUB_EXT_OVERLAY_NO },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_COLLAPSE, 0,
              (LPTSTR)TEXT("OptItem/OptType"), 0,   NULL,
               NULL, 0, DMPUB_TVOPT },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Disabled OptType"), 0,   NULL,
                   &NoYesOT, 0, DMPUB_TVOPT_DISABLED },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Overlay 'Warning' Icon"), 0,   NULL,
                  &NoYesOT, 0, DMPUB_TVOPT_OVERLAY_WARNING },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Overlay 'Stop' Icon"), 0,   NULL,
                  &NoYesOT, 0, DMPUB_TVOPT_OVERLAY_STOP },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Overlay 'No' Icon"), 0,   NULL,
                  &NoYesOT, 0, DMPUB_TVOPT_OVERLAY_NO },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_COLLAPSE, 0,
              (LPTSTR)TEXT("ScrollBar/TrackBar Ranges"), 0,   NULL,
               NULL, 0, DMPUB_TVOPT },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  TEXT("Min Range"), MIN_INT, NULL,
                   &MinRangeOT, 0, DMPUB_MINRANGE },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  TEXT("Max. Range"), MAX_INT, NULL,
                   &MaxRangeOT, 0, DMPUB_MAXRANGE },

        { sizeof(OPTITEM), LEVEL_1, 0, OPTIF_NONE, 0,
          TEXT("TVOT Test"), IDI_CPSUI_QUESTION,   NULL,
           NULL, 0, DMPUB_TVOPT_TEST },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              (LPTSTR)TEXT("TVOT_2STATES"), 0, NULL,
               &TVOT2StatesOT, 0, DMPUB_2STATES },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_3STATES"), 0, NULL,
               &TVOT3StatesOT, 0, DMPUB_3STATES },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_UDARROW"), 55, NULL,
               &TVOTUDArrowOT, 0, DMPUB_UDARROW },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_TRACKBAR"), 100, NULL,
               &TVOTTrackBarOT, 0, DMPUB_TRACKBAR },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_SCROLLBAR"), 210, NULL,
               &TVOTScrollBarOT, 0, DMPUB_SCROLLBAR },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_LISTBOX"), 2, NULL,
               &TVOTListBoxOT, 0, DMPUB_LISTBOX },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Include 16x16 Icons"), 1,   NULL,
                  &NoYesOT, 0, DMPUB_TVOPT_ICONS },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_COMBOBOX"), 4, NULL,
               &TVOTComboBoxOT, 0, DMPUB_COMBOBOX },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK | OPTIF_COLLAPSE, 0,
                  (LPTSTR)TEXT("Include 16x16 Icons"), 1,   NULL,
                  &NoYesOT, 0, DMPUB_TVOPT_ICONS },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_EDITBOX"), 0, NULL,  // fill in TVOTEditBoxBuf later
               &TVOTEditBoxOT, 0, DMPUB_EDITBOX },

            { sizeof(OPTITEM), LEVEL_2, 0, OPTIF_CALLBACK, 0,
              TEXT("TVOT_CHKBOX"), 0, NULL,
               &TVOTChkBoxOT, 0, DMPUB_CHKBOX },

                { sizeof(OPTITEM), LEVEL_3, 0, OPTIF_CALLBACK, 0,
                  TEXT("CheckBox Type"), 0,   NULL,
                   &ChkBoxTypeOT, 0, DMPUB_CHKBOX_TYPE }

};



INT_PTR
CALLBACK
PushButtonProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

#ifndef DBG
    UNREFERENCED_PARAMETER(lParam);
#endif

    switch (Msg) {

    case WM_INITDIALOG:                /* message: initialize dialog box */

        CPSUIDBG(DBG_PUSHDLGPROC,
                ("PushButtonProc lParam=%s", (LPSTR)lParam));
        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDOK:
        case IDCANCEL:

            EndDialog(hDlg, (BOOL)(LOWORD(wParam) == IDOK));
            return(TRUE);
        }
    }

    return(FALSE);
}



INT_PTR
APIENTRY
MyAboutProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    This is the about... callback which pop up appliation's own about


--*/
{
    PCOMPROPSHEETUI pCPSUI = NULL;
    TCHAR           Buf[128] = {0};
    HRESULT         hr = S_OK;

    switch (Msg) {

    case WM_INITDIALOG:                /* message: initialize dialog box */

        pCPSUI = (PCOMPROPSHEETUI)lParam;

        hr = pCPSUI ? S_OK : E_INVALIDARG;

        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(Buf, ARRAY_SIZE(Buf), TEXT("About %s"), pCPSUI->pCallerName);
        }

        if (SUCCEEDED(hr))
        {
            hr = SetWindowText(hDlg, Buf) ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(Buf, ARRAY_SIZE(Buf), 
                                TEXT("%s  Version %u.%u"),
                                pCPSUI->pCallerName,
                                (UINT)HIBYTE(pCPSUI->CallerVersion),
                                (UINT)LOBYTE(pCPSUI->CallerVersion));
        }

        if (SUCCEEDED(hr))
        {
            hr = SetDlgItemText(hDlg, IDD_ABOUT1, Buf) ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(Buf, ARRAY_SIZE(Buf), 
                                 TEXT("%s  Version %u.%u"),
                                 pCPSUI->pOptItemName,
                                 (UINT)HIBYTE(pCPSUI->OptItemVersion),
                                 (UINT)LOBYTE(pCPSUI->OptItemVersion));
        }

        if (SUCCEEDED(hr))
        {
            hr = SetDlgItemText(hDlg, IDD_ABOUT2, Buf) ? S_OK : E_FAIL;
        }

        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDOK:
        case IDCANCEL:

            EndDialog(hDlg, (BOOL)(LOWORD(wParam) == IDOK));
            return(TRUE);
        }
    }

    return (FALSE);
}



POPTITEM
FindOptItem(
    POPTITEM    pOptItem,
    UINT        cOptItem,
    BYTE        DMPubID
    )
{
    while (cOptItem--) {

        if (pOptItem->DMPubID == DMPubID) {

            return(pOptItem);
        }

        ++pOptItem;
    }

    return(NULL);
}



CPSUICALLBACK
TVTestCallBack(
    _In_ PCPSUICBPARAM   pCPSUICBParam
    )
{
    POPTITEM    pCurItem = pCPSUICBParam->pCurItem;
    POPTITEM    pItem;
    BYTE        DMPubID;
    LONG        Sel;
    LONG        Action = CPSUICB_ACTION_NONE;


    DMPubID = pCurItem->DMPubID;
    Sel     = pCurItem->Sel;

    switch (pCPSUICBParam->Reason) {

    case CPSUICB_REASON_OPTITEM_SETFOCUS:

        CPSUIDBG(DBG_TVTESTCB, ("Got OPTITEM_SETFOCUS CallBack now"));

        switch (DMPubID)
        {
        case DMPUB_MINRANGE:
            {
                pItem = FindOptItem(pCPSUICBParam->pOptItem,
                                    pCPSUICBParam->cOptItem,
                                    DMPUB_MAXRANGE);

                if (pItem)
                {
                    pCurItem->pOptType->pOptParam[1].lParam = (LONG)pItem->Sel;
                }
            }

            break;

        case DMPUB_MAXRANGE:
            {
                pItem = FindOptItem(pCPSUICBParam->pOptItem,
                                    pCPSUICBParam->cOptItem,
                                    DMPUB_MINRANGE);

                if (pItem)
                {
                    pCurItem->pOptType->pOptParam[1].IconID = (DWORD)pItem->Sel;
                }
            }

            break;

        }

        break;

    case CPSUICB_REASON_ABOUT:

        DialogBoxParam(hInstApp,
                       MAKEINTRESOURCE(DLGABOUT),
                       pCPSUICBParam->hDlg,
                       MyAboutProc,
                       (LPARAM)(pCPSUICBParam->pOldSel));

        break;

    case CPSUICB_REASON_ECB_CHANGED:

        CPSUIDBG(DBG_TVTESTCB, ("Got ECB_CHANGED CallBack now"));

        switch (DMPubID) {

        case DMPUB_TVOPT_ECB_EP:

            if (pCurItem->Flags & OPTIF_ECB_CHECKED) {

                TV_EP.IconID  =
                TV_ECB.IconID = IDI_APPLE;

            } else {

                TV_EP.IconID  =
                TV_ECB.IconID = 0;
            }

            Action = CPSUICB_ACTION_OPTIF_CHANGED;
            break;

        default:

            break;
        }

        break;

    case CPSUICB_REASON_SEL_CHANGED:

        CPSUIDBG(DBG_TVTESTCB, ("Got SEL_CHANGED CallBack now"));

        switch (DMPubID)
        {
        case DMPUB_CHKBOX_TYPE:
            {
                pItem = FindOptItem(pCPSUICBParam->pOptItem,
                                    pCPSUICBParam->cOptItem,
                                    DMPUB_CHKBOX);

                 if (pItem)
                 {
                    pItem->pOptType->pOptParam[0].Style = (BYTE)pCurItem->Sel;

                    pItem->Flags |= OPTIF_CHANGED;
                    Action        = CPSUICB_ACTION_OPTIF_CHANGED;
                 }
            }
            break;

        case DMPUB_TVOPT_ICONS:

            pItem = pCurItem - 1;

            if (Sel) {

                pItem->pOptType->Style &= ~OTS_LBCB_NO_ICON16_IN_ITEM;

            } else {

                pItem->pOptType->Style |= OTS_LBCB_NO_ICON16_IN_ITEM;
            }

            break;

        case DMPUB_EXT_OVERLAY_WARNING:

            if (Sel) {

                TV_EP.Flags  |= EPF_OVERLAY_WARNING_ICON;
                TV_ECB.Flags |= ECBF_OVERLAY_WARNING_ICON;

            } else {

                TV_EP.Flags  &= ~EPF_OVERLAY_WARNING_ICON;
                TV_ECB.Flags &= ~ECBF_OVERLAY_WARNING_ICON;
            }

            break;

        case DMPUB_EXT_OVERLAY_STOP:

            if (Sel) {

                TV_EP.Flags  |= EPF_OVERLAY_STOP_ICON;
                TV_ECB.Flags |= ECBF_OVERLAY_STOP_ICON;

            } else {

                TV_EP.Flags  &= ~EPF_OVERLAY_STOP_ICON;
                TV_ECB.Flags &= ~ECBF_OVERLAY_STOP_ICON;
            }

            break;

        case DMPUB_EXT_OVERLAY_NO:

            if (Sel) {

                TV_EP.Flags  |= EPF_OVERLAY_NO_ICON;
                TV_ECB.Flags |= ECBF_OVERLAY_NO_ICON;

            } else {

                TV_EP.Flags  &= ~EPF_OVERLAY_NO_ICON;
                TV_ECB.Flags &= ~ECBF_OVERLAY_NO_ICON;
            }

            break;

        case DMPUB_MINRANGE:

            MinMaxRangeOP[1].IconID = (DWORD)Sel;
            break;

        case DMPUB_MAXRANGE:

            MinMaxRangeOP[1].lParam = (LONG)Sel;
            break;

        case DMPUB_TVOPT_OVERLAY_WARNING:
        case DMPUB_TVOPT_OVERLAY_STOP:
        case DMPUB_TVOPT_OVERLAY_NO:

            Action = CPSUICB_ACTION_REINIT_ITEMS;

        case DMPUB_TVOPT_ECB_EP:

            if (Sel) {

                pCurItem->Flags &= ~OPTIF_EXT_DISABLED;

            } else {

                pCurItem->Flags |= OPTIF_EXT_DISABLED;
            }

            pCurItem->Flags |= OPTIF_CHANGED;
            Action           = CPSUICB_ACTION_OPTIF_CHANGED;

            //
            // Fall through
            //

        case DMPUB_TVOPT_DISABLED:
        case DMPUB_EXT_DISABLED:

            pItem = FindOptItem(pCPSUICBParam->pOptItem,
                                    pCPSUICBParam->cOptItem,
                                    DMPUB_2STATES);

            if (pItem) {

                do {

                    if ((pItem->DMPubID >= DMPUB_2STATES) &&
                        (pItem->DMPubID <= DMPUB_CHKBOX)) {

                        switch (DMPubID) {

                        case DMPUB_TVOPT_OVERLAY_WARNING:

                            if (Sel) {

                                pItem->Flags |= OPTIF_OVERLAY_WARNING_ICON;

                            } else {

                                pItem->Flags &= ~OPTIF_OVERLAY_WARNING_ICON;
                            }

                            break;

                        case DMPUB_TVOPT_OVERLAY_STOP:

                            if (Sel) {

                                pItem->Flags |= OPTIF_OVERLAY_STOP_ICON;

                            } else {

                                pItem->Flags &= ~OPTIF_OVERLAY_STOP_ICON;
                            }

                            break;

                        case DMPUB_TVOPT_OVERLAY_NO:

                            if (Sel) {

                                pItem->Flags |= OPTIF_OVERLAY_NO_ICON;

                            } else {

                                pItem->Flags &= ~OPTIF_OVERLAY_NO_ICON;
                            }

                            break;

                        case DMPUB_TVOPT_DISABLED:

                            if (Sel) {

                                pItem->pOptType->Flags |= OPTTF_TYPE_DISABLED;

                            } else {

                                pItem->pOptType->Flags &= ~OPTTF_TYPE_DISABLED;
                            }

                            break;

                        case DMPUB_EXT_DISABLED:

                            if (Sel) {

                                pItem->Flags |= OPTIF_EXT_DISABLED;

                            } else {

                                pItem->Flags &= ~OPTIF_EXT_DISABLED;
                            }

                            break;

                        case DMPUB_TVOPT_ECB_EP:

                            switch (Sel) {

                            case 0:

                                pItem->Flags |= OPTIF_EXT_HIDE;
                                break;

                            case 1:

                                pItem->Flags     &= ~(OPTIF_EXT_HIDE        |
                                                      OPTIF_EXT_IS_EXTPUSH);
                                pItem->pExtChkBox = &TV_ECB;

                                break;

                            case 2:

                                pItem->Flags    &= ~OPTIF_EXT_HIDE;
                                pItem->Flags    |= OPTIF_EXT_IS_EXTPUSH;
                                pItem->pExtPush  = &TV_EP;
                                break;
                            }
                        }
                    }

                    pItem->Flags |= OPTIF_CHANGED;

                } while ((pItem++)->DMPubID != DMPUB_CHKBOX);
            }

            break;

        default:

            break;
        }

        break;

    case CPSUICB_REASON_PUSHBUTTON:

        CPSUIDBG(DBG_TVTESTCB, ("Got PUSH Button CallBack now"));
        break;

    case CPSUICB_REASON_EXTPUSH:

        DialogBoxParam(hInstApp,
                       MAKEINTRESOURCE(EXTPUSH_DLG),
                       pCPSUICBParam->hDlg,
                       PushButtonProc,
                       (LPARAM)(pCPSUICBParam->pOldSel));
        break;

    default:

        break;
    }

    return(Action);
}



BOOL
SetupComPropSheetUI(
    _In_ PCOMPROPSHEETUI pCPSUI
    )
{
    static BOOL UpdateEditBox = TRUE;
    UINT        i;


    ZeroMemory(pCPSUI, sizeof(COMPROPSHEETUI));

    pCPSUI->cbSize         = sizeof(COMPROPSHEETUI);
    pCPSUI->hInstCaller    = hInstApp;
    pCPSUI->pCallerName    = (LPTSTR)TitleName;
    pCPSUI->CallerVersion  = 0x100;
    pCPSUI->pOptItemName   = (LPTSTR)OptItemName;
    pCPSUI->OptItemVersion = 0x400;
    pCPSUI->UserData       = (ULONG_PTR)pCPSUI;
    pCPSUI->pHelpFile      = (LPTSTR)TEXT("CPSUISam.hlp");
    pCPSUI->pfnCallBack    = TVTestCallBack;
    pCPSUI->pOptItem       = TVTestOptItems;
    pCPSUI->cOptItem       = COUNT_ARRAY(TVTestOptItems);
    pCPSUI->Flags          = 0;
    pCPSUI->pDlgPage       = CPSUI_PDLGPAGE_TREEVIWONLY;
    pCPSUI->cDlgPage       = 0;


    if (UpdateEditBox) {

        //
        // Fixup compiler warning for the LPSTR and LONG
        //

        for (i = 0; i < COUNT_ARRAY(TVTestOptItems); i++) {

            if (TVTestOptItems[i].DMPubID == DMPUB_EDITBOX) {

                TVTestOptItems[i].pSel = (LPTSTR)TVOTEditBoxBuf;
                UpdateEditBox = FALSE;
                break;
            }
        }
    }

    if (UpdatePermission) {

        pCPSUI->Flags |= CPSUIF_UPDATE_PERMISSION;
    }

    if (!UseStdAbout) {

        pCPSUI->Flags |= CPSUIF_ABOUT_CALLBACK;
    }

    return(TRUE);
}
