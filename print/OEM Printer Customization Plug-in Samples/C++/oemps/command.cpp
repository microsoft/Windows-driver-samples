//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   command.cpp
//
//  PURPOSE:  Source module for OEM customized Command(s).
//

#include "precomp.h"
#include "oemps.h"
#include "debug.h"
#include "command.h"
#include "resource.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

////////////////////////////////////////////////////////
//      Internal String Literals
////////////////////////////////////////////////////////

const CHAR TEST_BEGINSTREAM[]                   = "%%Test: Before begin stream\r\n";
const CHAR TEST_PSADOBE[]                       = "%%Test: Before %!PS-Adobe\r\n";
const CHAR TEST_PAGESATEND[]                    = "%%Test: Replace driver's %%PagesAtend\r\n";
const CHAR TEST_PAGES[]                         = "%%Test: Replace driver's %%Pages: (atend)\r\n";
const CHAR TEST_DOCUMENTPROCESSCOLORS[]         = "%%Test: Replace driver's %%DocumentProcessColors: (atend)\r\n";
const CHAR TEST_COMMENTS[]                      = "%%Test: Before %%EndComments\r\n";
const CHAR TEST_DEFAULTS[]                      = "%%Test: Before %%BeginDefaults and %%EndDefaults\r\n";
const CHAR TEST_BEGINPROLOG[]                   = "%%Test: After %%BeginProlog\r\n";
const CHAR TEST_ENDPROLOG[]                     = "%%Test: Before %%EndProlog\r\n";
const CHAR TEST_BEGINSETUP[]                    = "%%Test: After %%BeginSetup\r\n";
const CHAR TEST_ENDSETUP[]                      = "%%Test: Before %%EndSetup\r\n";
const CHAR TEST_BEGINPAGESETUP[]                = "%%Test: After %%BeginPageSetup\r\n";
const CHAR TEST_ENDPAGESETUP[]                  = "%%Test: Before %%EndpageSetup\r\n";
const CHAR TEST_PAGETRAILER[]                   = "%%Test: After %%PageTrailer\r\n";
const CHAR TEST_TRAILER[]                       = "%%Test: After %%Trailer\r\n";
const CHAR TEST_PAGENUMBER[]                    = "%%Test: Replace driver's %%Page:\r\n";
const CHAR TEST_PAGEORDER[]                     = "%%Test: Replace driver's %%PageOrder:\r\n";
const CHAR TEST_ORIENTATION[]                   = "%%Test: Replace driver's %%Orientation:\r\n";
const CHAR TEST_BOUNDINGBOX[]                   = "%%Test: Replace driver's %%BoundingBox:\r\n";
const CHAR TEST_DOCNEEDEDRES[]                  = "%%Test: Append to driver's %%DocumentNeededResourc\r\n";
const CHAR TEST_DOCSUPPLIEDRES[]                = "%%Test: Append to driver's %%DocumentSuppliedResou\r\n";
const CHAR TEST_EOF[]                           = "%%Test: After %%EOF\r\n";
const CHAR TEST_ENDSTREAM[]                     = "%%Test: After the last byte of job stream\r\n";
const CHAR TEST_DOCUMENTPROCESSCOLORSATEND[]    = "%%Test: DocumentProcessColorsAtend\r\n";
const CHAR TEST_VMSAVE[]                        = "%%Test: %%VMSave\r\n";
const CHAR TEST_VMRESTORE[]                     = "%%Test: %%VMRestore\r\n";
const CHAR TEST_PLATECOLOR[]                    = "%%Test: %%PlateColor:\r\n";
const CHAR TEST_SHOWPAGE[]                      = "%%Test: %%SowPage:\r\n";
const CHAR TEST_PAGEBBOX[]                      = "%%Test: %%PageBox:\r\n";
const CHAR TEST_ENDPAGECOMMENTS[]               = "%%Test: %%EndPageComments:\r\n";


////////////////////////////////////////////////////////////////////////////////////
//    The PSCRIPT driver calls this OEM function at specific points during output
//    generation. This gives the OEM DLL an opportunity to insert code fragments
//    at specific injection points in the driver's code. It should use
//    DrvWriteSpoolBuf for generating any output it requires.

HRESULT PSCommand(PDEVOBJ pdevobj, DWORD dwIndex, PVOID pData, DWORD cbSize,
                  IPrintOemDriverPS* pOEMHelp, PDWORD pdwReturn)
{
    PCSTR   pProcedure  = NULL;
    DWORD   dwLen       = 0;
    DWORD   dwSize      = 0;
    HRESULT hResult     = E_FAIL;

    VERBOSE("Entering OEMCommand...\r\n");

    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(pData);

    switch (dwIndex)
    {
        case PSINJECT_BEGINSTREAM:
            VERBOSE("OEMCommand PSINJECT_BEGINSTREAM\n");
            pProcedure = TEST_BEGINSTREAM;
            break;

        case PSINJECT_PSADOBE:
            VERBOSE("OEMCommand PSINJECT_PSADOBE\n");
            pProcedure = TEST_PSADOBE;
            break;

        case PSINJECT_PAGESATEND:
            VERBOSE("OEMCommand PSINJECT_PAGESATEND\n");
            pProcedure = TEST_PAGESATEND;
            break;

        case PSINJECT_PAGES:
            VERBOSE("OEMCommand PSINJECT_PAGES\n");
            pProcedure = TEST_PAGES;
            break;

        case PSINJECT_DOCNEEDEDRES:
            VERBOSE("OEMCommand PSINJECT_DOCNEEDEDRES\n");
            pProcedure = TEST_DOCNEEDEDRES;
            break;

        case PSINJECT_DOCSUPPLIEDRES:
            VERBOSE("OEMCommand PSINJECT_DOCSUPPLIEDRES\n");
            pProcedure = TEST_DOCSUPPLIEDRES;
            break;

        case PSINJECT_PAGEORDER:
            VERBOSE("OEMCommand PSINJECT_PAGEORDER\n");
            pProcedure = TEST_PAGEORDER;
            break;

        case PSINJECT_ORIENTATION:
            VERBOSE("OEMCommand PSINJECT_ORIENTATION\n");
            pProcedure = TEST_ORIENTATION;
            break;

        case PSINJECT_BOUNDINGBOX:
            VERBOSE("OEMCommand PSINJECT_BOUNDINGBOX\n");
            pProcedure = TEST_BOUNDINGBOX;
            break;

        case PSINJECT_DOCUMENTPROCESSCOLORS:
            VERBOSE("OEMCommand PSINJECT_DOCUMENTPROCESSCOLORS\n");
            pProcedure = TEST_DOCUMENTPROCESSCOLORS;
            break;

        case PSINJECT_COMMENTS:
            VERBOSE("OEMCommand PSINJECT_COMMENTS\n");
            pProcedure = TEST_COMMENTS;
            break;

        case PSINJECT_BEGINDEFAULTS:
            VERBOSE("OEMCommand PSINJECT_BEGINDEFAULTS\n");
            pProcedure = TEST_DEFAULTS;
            break;

        case PSINJECT_ENDDEFAULTS:
            VERBOSE("OEMCommand PSINJECT_BEGINDEFAULTS\n");
            pProcedure = TEST_DEFAULTS;
            break;

        case PSINJECT_BEGINPROLOG:
            VERBOSE("OEMCommand PSINJECT_BEGINPROLOG\n");
            pProcedure = TEST_BEGINPROLOG;
            break;

        case PSINJECT_ENDPROLOG:
            VERBOSE("OEMCommand PSINJECT_ENDPROLOG\n");
            pProcedure = TEST_ENDPROLOG;
            break;

        case PSINJECT_BEGINSETUP:
            VERBOSE("OEMCommand PSINJECT_BEGINSETUP\n");
            pProcedure = TEST_BEGINSETUP;
            break;

        case PSINJECT_ENDSETUP:
            VERBOSE("OEMCommand PSINJECT_ENDSETUP\n");
            pProcedure = TEST_ENDSETUP;
            break;

        case PSINJECT_BEGINPAGESETUP:
            VERBOSE("OEMCommand PSINJECT_BEGINPAGESETUP\n");
            pProcedure = TEST_BEGINPAGESETUP;
            break;

        case PSINJECT_ENDPAGESETUP:
            VERBOSE("OEMCommand PSINJECT_ENDPAGESETUP\n");
            pProcedure = TEST_ENDPAGESETUP;
            break;

        case PSINJECT_PAGETRAILER:
            VERBOSE("OEMCommand PSINJECT_PAGETRAILER\n");
            pProcedure = TEST_PAGETRAILER;
            break;

        case PSINJECT_TRAILER:
            VERBOSE("OEMCommand PSINJECT_TRAILER\n");
            pProcedure = TEST_TRAILER;
            break;

        case PSINJECT_PAGENUMBER:
            VERBOSE("OEMCommand PSINJECT_PAGENUMBER\n");
            pProcedure = TEST_PAGENUMBER;
            break;

        case PSINJECT_EOF:
            VERBOSE("OEMCommand PSINJECT_EOF\n");
            pProcedure = TEST_EOF;
            break;

        case PSINJECT_ENDSTREAM:
            VERBOSE("OEMCommand PSINJECT_ENDSTREAM\n");
            pProcedure = TEST_ENDSTREAM;
            break;

        case PSINJECT_DOCUMENTPROCESSCOLORSATEND:
            VERBOSE("OEMCommand PSINJECT_DOCUMENTPROCESSCOLORSATEND\n");
            pProcedure = TEST_DOCUMENTPROCESSCOLORSATEND;
            break;

        case PSINJECT_VMSAVE:
            VERBOSE("OEMCommand PSINJECT_VMSAVE\n");
            pProcedure = TEST_VMSAVE;
            break;

        case PSINJECT_VMRESTORE:
            VERBOSE("OEMCommand PSINJECT_VMRESTORE\n");
            pProcedure = TEST_VMRESTORE;
            break;

        case PSINJECT_PLATECOLOR:
            VERBOSE("OEMCommand PSINJECT_PLATECOLOR\n");
            pProcedure = TEST_PLATECOLOR;
            break;

        case PSINJECT_SHOWPAGE:
            VERBOSE("OEMCommand PSINJECT_SHOWPAGE\n");
            pProcedure = TEST_SHOWPAGE;
            break;

        case PSINJECT_PAGEBBOX:
            VERBOSE("OEMCommand PSINJECT_PAGEBBOX\n");
            pProcedure = TEST_PAGEBBOX;
            break;

        case PSINJECT_ENDPAGECOMMENTS:
            VERBOSE("OEMCommand PSINJECT_ENDPAGECOMMENTS\n");
            pProcedure = TEST_ENDPAGECOMMENTS;
            break;

        default:
            ERR("Undefined PSCommand!\r\n");
            *pdwReturn = ERROR_NOT_SUPPORTED;
            return E_NOTIMPL;
    }

    // INVARIANT: should have injection string.

    if(NULL != pProcedure)
    {
        // Write PostScript to spool file.
        dwLen = (DWORD)strlen(pProcedure);
        if (dwLen > 0)
        {
            hResult = pOEMHelp->DrvWriteSpoolBuf(pdevobj, const_cast<PSTR>(pProcedure), dwLen, &dwSize);
        }

        // Set return values.
        if(SUCCEEDED(hResult) && (dwLen == dwSize))
        {
            *pdwReturn = ERROR_SUCCESS;
        }
        else
        {
            // Return a meaningful
            // error value.
            *pdwReturn = GetLastError();
            if(ERROR_SUCCESS == *pdwReturn)
            {
                *pdwReturn = ERROR_WRITE_FAULT;
            }

            // Make sure we return failure
            // if the write didn't succeded.
            if(SUCCEEDED(hResult))
            {
                hResult = HRESULT_FROM_WIN32(*pdwReturn);
            }
        }
    }

    return hResult;
}


