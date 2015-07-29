/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    secureelement.h

Abstract:

    This header file defines the secure element object 

Environment:

    User Mode

--*/
#pragma once

class CSecureElement
{
public:
    CSecureElement(const GUID& SecureElementId, SECURE_ELEMENT_TYPE SecureElementType);
    ~CSecureElement();

public:
    NTSTATUS SetEmulationMode(SECURE_ELEMENT_CARD_EMULATION_MODE Mode);

public:
    GUID GetIdentifier()
    {
        return m_SecureElementId;
    }
    SECURE_ELEMENT_TYPE GetType()
    {
        return m_SecureElementType;
    }
    SECURE_ELEMENT_CARD_EMULATION_MODE GetEmulationMode()
    {
        return m_EmulationMode;
    }
    PLIST_ENTRY GetListEntry()
    {
        return &m_ListEntry;
    }
    static CSecureElement* FromListEntry(PLIST_ENTRY pEntry)
    {
        return (CSecureElement*) CONTAINING_RECORD(pEntry, CSecureElement, m_ListEntry);
    }

private:
    GUID                                m_SecureElementId;      // Secure Element GUID identifier
    SECURE_ELEMENT_TYPE                 m_SecureElementType;    // Secure Element type
    SECURE_ELEMENT_CARD_EMULATION_MODE  m_EmulationMode;        // Emulation mode of the Secure Element
    LIST_ENTRY                          m_ListEntry;            // Secure Element list entry
};
