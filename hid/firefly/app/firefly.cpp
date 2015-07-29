/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Firefly.c

Abstract:

    App to change the tail light state of MS optical mouse with the help of
    firefly driver. This app uses WMI interface to talk to the driver.

Environment:

    User Mode console application

--*/
#include "luminous.h"
#include <dontuse.h>

#define USAGE  \
_T("Usage: Flicker <-0 | -1 | -2>\n\
    \t\t-0 turns off light \n\
    \t\t-1 turns on light \n\
    \t\t-2 flashes light ")

int __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    int                                 i, j, k;
    BOOL                            bAdjustLight = FALSE;
    BOOL                            bSuccessful;
    ULONG                          lightSetting = 0;
    CLuminous                    *luminous;

    if (argc == 2) {

        if (argv[1][0] == '-') {

            if ((argv[1][1] >= '0') && (argv[1][1] <= '2')) {

                bAdjustLight = TRUE;
                lightSetting = (argv[1][1] - '0');
            }
        }
     }

    if  (FALSE == bAdjustLight)
    {
        _tprintf(USAGE);
        exit(0);
    }

    luminous = new CLuminous();

    if (luminous == NULL) {

        _tprintf(_T("Problem creating Luminous\n"));
        return 0;
    }

    if (!luminous->Open()) {

        _tprintf(_T("Problem opening Luminous\n"));
        delete(luminous);
        return 0;
    }

    if (bAdjustLight) {

        if (lightSetting < 2) {

            bSuccessful = luminous->Set((BOOL) lightSetting);

            if (bSuccessful) {

                _tprintf(_T("Adjusted light to %x\n"), lightSetting);

            } else {

                _tprintf(_T("Problem occured while adjusting light: %x\n"), GetLastError());
            }

        } else {

            k=0;
            j=1;
            for(i = 500; (i>0)&&(j>0); i-=j) {

                j = (i*9/100);
                Sleep(i);
                if(!luminous->Set((BOOL) k)) {
                    _tprintf(_T("Set operation on Luminous failed.\n"));
                    goto End;
                }
                k=1-k;
            }
            for(i = 12; i<500; i+=j) {

                j = (i*9/100);
                Sleep(i);
                if(!luminous->Set((BOOL) k)) {
                    _tprintf(_T("Set operation on Luminous failed.\n"));
                    goto End;
                }
                k=1-k;
            }
            if (k) {
                if(!luminous->Set((BOOL) k)) {
                    _tprintf(_T("Set operation on Luminous failed.\n"));
                }
            }
        }
    }

    luminous->Set(TRUE);

End:
    luminous->Close();

    delete(luminous);

    return 0;
}


