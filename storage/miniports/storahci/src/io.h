/*++

Copyright (C) Microsoft Corporation, 2009

Module Name:

    io.h

Abstract:



Notes:

Revision History:

--*/

#pragma once

ULONG
GetSlotToActivate(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ ULONG                   TargetSlots
    );

UCHAR
GetSingleIo(
    PAHCI_CHANNEL_EXTENSION ChannelExtension
    );

BOOLEAN
ActivateQueue(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ BOOLEAN AtDIRQL
    );

VOID
AddQueue (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PSTORAHCI_QUEUE     Queue,
    _In_ PSTORAGE_REQUEST_BLOCK Srb,
    _In_ ULONG Signature,
    _In_ UCHAR Tag
    );

PSTORAGE_REQUEST_BLOCK
RemoveQueue (
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PSTORAHCI_QUEUE Queue,
    _In_ ULONG Signature,
    _In_ UCHAR Tag
    );

VOID
AhciCompleteIssuedSRBs(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR SrbStatus,
    _In_ BOOLEAN AtDIRQL
  );

VOID
SRBtoATA_CFIS(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PSLOT_CONTENT SlotContent
  );

VOID
SRBtoATAPI_CFIS(
    PAHCI_CHANNEL_EXTENSION ChannelExtension,
    PSLOT_CONTENT SlotContent
  );

VOID
CfistoATA_CFIS(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSLOT_CONTENT SlotContent
  );

ULONG
SRBtoPRDT(
    _In_ PVOID ChannelExtension,
    _In_ PSLOT_CONTENT SlotContent
  );

VOID
SRBtoCmdHeader(
    _In_ PVOID ChannelExtension,
    _In_ PSLOT_CONTENT SlotContent,
    _In_ ULONG Length,
    _In_ BOOLEAN Reset
  );

BOOLEAN
AhciProcessIo(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    );

BOOLEAN
AhciFormIo(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  Srb,
    _In_ BOOLEAN AtDIRQL
    );

PSCSI_REQUEST_BLOCK
BuildRequestSenseSrb(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSTORAGE_REQUEST_BLOCK  FailingSrb
    );

VOID
AhciPortFailAllIos(
    _In_ PAHCI_CHANNEL_EXTENSION ChannelExtension,
    _In_ UCHAR   SrbStatus,
    _In_ BOOLEAN AtDIRQL
    );

HW_DPC_ROUTINE AhciPortSrbCompletionDpcRoutine;

HW_DPC_ROUTINE AhciPortBusChangeDpcRoutine;

