/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2001, Microsoft Corporation.

    File:

        imagecapture.h

    Abstract:

        Image Capture Pin - A specialization of a capture pin.

        CImageCapturePin defines a unique list of data ranges for the image 
        pin.

    History:

        created 3/8/2001

**************************************************************************/
#pragma once

#include "capture.h"

class CImageCapturePin : public CCapturePin
{
public:
    static NTSTATUS DispatchCreate(
        _In_    PKSPIN Pin,
        _In_    PIRP Irp
    );

private:
    virtual
    NTSTATUS Close(
        _In_    PIRP Irp
    );

    CImageCapturePin(
        _In_    PKSPIN Pin
    );
};
