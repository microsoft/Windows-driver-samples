/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    CtrlPath.H

Abstract:

    This module declares functions for the miniport's control path.

--*/

#ifndef _CTRLPATH_H
#define _CTRLPATH_H


MINIPORT_OID_REQUEST                MPOidRequest;
#if (NDIS_SUPPORT_NDIS680)
MINIPORT_SYNCHRONOUS_OID_REQUEST    MPSynchronousOidRequest;
#endif

#endif // _CTRLPATH_H

