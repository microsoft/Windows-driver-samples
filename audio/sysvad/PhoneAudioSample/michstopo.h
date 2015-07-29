
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    michstopo.h

Abstract:

    Declaration of topology miniport for the mic (external: headset).

--*/

#ifndef _SYSVAD_MICHSTOPO_H_
#define _SYSVAD_MICHSTOPO_H_

NTSTATUS PropertyHandler_MicHsTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_MICHSTOPO_H_
