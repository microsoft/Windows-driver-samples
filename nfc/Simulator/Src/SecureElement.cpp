/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    secureelement.cpp

Abstract:

    Implements the secure element class

Environment:

    User-mode only.

--*/

#include "Internal.h"
#include "SecureElement.tmh"

CSecureElement::CSecureElement(
    const GUID& SecureElementId,
    SECURE_ELEMENT_TYPE SecureElementType
    )
    : m_SecureElementId(SecureElementId),
      m_SecureElementType(SecureElementType),
      m_EmulationMode(EmulationOff)
{
    InitializeListHead(&m_ListEntry);
}

CSecureElement::~CSecureElement()
{
}

NTSTATUS CSecureElement::SetEmulationMode(SECURE_ELEMENT_CARD_EMULATION_MODE EmulationMode)
{
    MethodEntry("...");

    NTSTATUS Status = STATUS_SUCCESS;

    if (m_EmulationMode == EmulationMode) {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto Exit;
    }

    m_EmulationMode = EmulationMode;

Exit:
    MethodReturn(Status, "Status = %!STATUS!", Status);
}