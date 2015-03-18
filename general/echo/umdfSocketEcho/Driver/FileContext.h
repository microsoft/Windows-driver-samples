/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    filecontext.h

Abstract:

    This header file defines the structure type for file  context associated with the file object 

Environment:

    user mode only

Revision History:

--*/


#pragma once

typedef struct _FileContext
{
    CConnection *pConnection ;

    CComPtr<IWDFIoTarget> pFileTarget;

}FileContext;
