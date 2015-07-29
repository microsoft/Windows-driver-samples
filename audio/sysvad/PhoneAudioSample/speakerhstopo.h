
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakerhstopo.h

Abstract:

    Declaration of topology miniport for the speaker (external: headset).

--*/

#ifndef _SYSVAD_SPEAKERHSTOPO_H_
#define _SYSVAD_SPEAKERHSTOPO_H_

NTSTATUS PropertyHandler_SpeakerHsTopoFilter(_In_ PPCPROPERTY_REQUEST PropertyRequest);

#endif // _SYSVAD_SPEAKERHSTOPO_H_
