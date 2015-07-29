
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micintopo.h

Abstract:

    Declaration of topology miniport for the mic (external: headphone).

--*/

#ifndef _SYSVAD_MICINTOPO_H_
#define _SYSVAD_MICINTOPO_H_

NTSTATUS PropertyHandler_MicInTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_MICINTOPO_H_
