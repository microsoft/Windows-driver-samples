
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerhptopo.h

Abstract:

    Declaration of topology miniport for the speaker (external: headphone).

--*/

#ifndef _SYSVAD_SPEAKERHPTOPO_H_
#define _SYSVAD_SPEAKERHPTOPO_H_

NTSTATUS PropertyHandler_SpeakerHpTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_SPEAKERHPTOPO_H_
