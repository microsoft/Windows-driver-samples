/**************************************************************************

    AVStream Simulated Hardware Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        filter.h

    Abstract:

        This file contains the filter level header for the capture filter.

    History:

        created 3/12/2001

**************************************************************************/

class CCaptureFilter {

private:

    //
    // The AVStream filter object associated with this CCaptureFilter.
    //
    PKSFILTER m_Filter;
   
    //
    // Cleanup():
    //
    // This is the bag cleanup callback for the CCaptureFilter.  Not providing
    // one would cause ExFreePool to be used.  This is not good for C++
    // constructed objects.  We simply delete the object here.
    //
    static
    void
    Cleanup (
        IN CCaptureFilter *CapFilter
        )
    {
        delete CapFilter;
    }

public:

    //
    // CCaptureFilter():
    //
    // The capture filter object constructor.  Since the new operator will
    // have zeroed the memory, do not bother initializing any NULL or 0
    // fields.  Only initialize non-NULL, non-0 fields.
    //
    CCaptureFilter (
        IN PKSFILTER Filter
        ) :
        m_Filter (Filter)
    {
    }

    //
    // ~CCaptureFilter():
    //
    // The capture filter destructor.
    //
    ~CCaptureFilter (
        )
    {
    }

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
        IN PKSFILTER Filter,
        IN PIRP Irp
        );

};


