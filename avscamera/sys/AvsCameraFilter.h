/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2015, Microsoft Corporation.

    File:

        AvsCameraFilter.h

    Abstract:

        Filter class implementation.  Derived from CCapturefilter.
        This class overloads basic filter behavior for our device.

    History:

        created 02/24/2015

**************************************************************************/

class CAvsCameraFilter : public CCaptureFilter
{
public:
    //
    //  Ctor
    //
    CAvsCameraFilter(
        _In_    PKSFILTER Filter
    );

    //
    //  Dtor
    //
    virtual
    ~CAvsCameraFilter ();

    //
    // DispatchCreate():
    //
    // This is the filter creation dispatch for the capture filter.  It
    // creates the CCaptureFilter object, associates it with the AVStream
    // object, and bags it for easy cleanup later.
    //
    static
    NTSTATUS
    DispatchCreate (
        _In_    PKSFILTER Filter,
        _In_    PIRP Irp
    );

    //  Example of adding a new, custom property.
    DECLARE_PROPERTY_HANDLERS( ULONG, CustomDummy )

protected:
    DWORD       m_CustomValue;
};
