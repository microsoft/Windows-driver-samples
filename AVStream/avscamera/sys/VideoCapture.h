/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        videocapture.h

    Abstract:

        Video Capture Pin definition.  
        
        CVideoCapturePin derived from CCapturePin.

    History:

        created 3/8/2001

**************************************************************************/

#pragma once

#include "capture.h"

class CVideoCapturePin : public CCapturePin
{
public:
    static NTSTATUS DispatchCreate (IN PKSPIN Pin, IN PIRP Irp);

private:
    CVideoCapturePin(IN PKSPIN Pin);
};
