/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    UcmCallbacks.h

Abstract:

    Type-C Platform Policy Manager. Interface for callbacks from UcmCx.

Environment:

    Kernel-mode only.

--*/

#pragma once

EXTERN_C_START

EVT_UCM_CONNECTOR_SET_DATA_ROLE Ucm_EvtConnectorSetDataRole;
EVT_UCM_CONNECTOR_SET_POWER_ROLE Ucm_EvtConnectorSetPowerRole;

EXTERN_C_END