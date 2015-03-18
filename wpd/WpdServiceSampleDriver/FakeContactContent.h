#pragma once

/**
 * This class represents an abstraction of a contact content object
 * Driver implementors should replace this with their own
 * device I/O classes/libraries.
 */

class FakeContactContent : public FakeContent
{
public:
    FakeContactContent()
    {
        Format              = FORMAT_AbstractContact;
        ContentType         = WPD_CONTENT_TYPE_CONTACT;
        RequiredScope       = CONTACTS_SERVICE_ACCESS;
        CanDelete           = true;
        VersionIdentifier   = 0;
    }

    FakeContactContent(const FakeContactContent& src)
    {
        *this = src;
    }

    ~FakeContactContent()
    {
    }

    FakeContactContent& operator= (const FakeContactContent& src)
    {
        FamilyName                  = src.FamilyName;
        GivenName                   = src.GivenName;
        VersionIdentifier           = src.VersionIdentifier;
        
        return *this;
    }

    HRESULT GetValue(
        _In_  REFPROPERTYKEY         Key, 
        _In_  IPortableDeviceValues* pStore);

    HRESULT WriteValue(
        _In_  REFPROPERTYKEY         Key, 
        _In_  REFPROPVARIANT         Value);

    HRESULT GetPropertyAttributes(
        _In_  REFPROPERTYKEY         Key, 
        _In_  IPortableDeviceValues* pAttributes);

    HRESULT GetSupportedProperties(
        _In_  IPortableDeviceKeyCollection* pKeys);
  
    HRESULT WriteValues(
        _In_  IPortableDeviceValues* pValues,
        _In_  IPortableDeviceValues* pResults,
        _Out_ bool*                  pbObjectChanged);

private:
    void UpdateVersion();

public:    
    // Custom properties defined by the contacts service
    CAtlStringW             FamilyName;
    CAtlStringW             GivenName;

private:
    // Indicates whether the object has been updated
    DWORD                   VersionIdentifier;
};

HRESULT GetSupportedContactProperties(
    _In_  IPortableDeviceKeyCollection* pKeys);

HRESULT GetContactPropertyAttributes(
    _In_  REFPROPERTYKEY                Key, 
    _In_  IPortableDeviceValues*        pAttributes);
