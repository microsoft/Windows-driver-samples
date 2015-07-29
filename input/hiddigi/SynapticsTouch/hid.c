/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        hid.c

    Abstract:

        Code for handling HID related requests

    Environment:

        Kernel mode

    Revision History:

--*/

#include "internal.h"
#include "controller.h"
#include "hid.h"
#include "hid.tmh"


//
// HID Report Descriptor for a touch device
//

#ifndef _SAMPLE_DESCRIPTOR_
#error This HID descriptor only works for a touch screen with resolution = 768 x 1280 and dimensions = 2.32" x 3.82"
#endif

const UCHAR gReportDescriptor[] = {
    0x05, 0x0d,	                        // USAGE_PAGE (Digitizers)          
    0x09, 0x04,	                        // USAGE (Touch Screen)             
    0xa1, 0x01,                         // COLLECTION (Application)         
    0x85, REPORTID_MTOUCH,              //   REPORT_ID (Touch)              
    0x09, 0x22,                         //   USAGE (Finger)                 
    0xa1, 0x02,                         //   COLLECTION (Logical)  
    0x09, 0x42,                         //     USAGE (Tip Switch)           
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          
    0x75, 0x01,                         //     REPORT_SIZE (1)              
    0x95, 0x01,                         //     REPORT_COUNT (1)             
    0x81, 0x02,                         //       INPUT (Data,Var,Abs) 
    0x09, 0x32,	                        //     USAGE (In Range)             
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)         
    0x95, 0x06,                         //     REPORT_COUNT (6)  
    0x81, 0x03,                         //       INPUT (Cnst,Ary,Abs)
    0x75, 0x08,                         //     REPORT_SIZE (8)
    0x09, 0x51,                         //     USAGE (Contact Identifier)
    0x95, 0x01,                         //     REPORT_COUNT (1)             
    0x81, 0x02,                         //       INPUT (Data,Var,Abs) 
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desk..
    0x26, 0x00, 0x03,                   //     LOGICAL_MAXIMUM (768)        NOTE: ADJUST LOGICAL MAXIMUM FOR X BASED ON TOUCH SCREEN RESOLUTION!
    0x75, 0x10,                         //     REPORT_SIZE (16)             
    0x55, 0x0e,                         //     UNIT_EXPONENT (-2)           
    0x65, 0x13,                         //     UNIT (Inch,EngLinear)
    0x09, 0x30,                         //     USAGE (X)                    
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         
    0x46, 0xe8, 0x00,                   //     PHYSICAL_MAXIMUM (232)       NOTE: ADJUST PHYSICAL MAXIMUM FOR X BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)         
    0x46, 0x7e, 0x01,                   //     PHYSICAL_MAXIMUM (382)       NOTE: ADJUST PHYSICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
    0x26, 0x00, 0x05,                   //     LOGICAL_MAXIMUM (1280)       NOTE: ADJUST LOGICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN RESOLUTION!
    0x09, 0x31,                         //     USAGE (Y)                
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)
    0xc0,                               //   END_COLLECTION
    0xa1, 0x02,                         //   COLLECTION (Logical)  
    0x05, 0x0d,	                        //     USAGE_PAGE (Digitizers)  
    0x09, 0x42,                         //     USAGE (Tip Switch)           
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          
    0x75, 0x01,                         //     REPORT_SIZE (1)              
    0x95, 0x01,                         //     REPORT_COUNT (1)             
    0x81, 0x02,                         //       INPUT (Data,Var,Abs) 
    0x09, 0x32,	                        //     USAGE (In Range)             
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         
    0x95, 0x06,                         //     REPORT_COUNT (6)  
    0x81, 0x03,                         //       INPUT (Cnst,Ary,Abs)
    0x75, 0x08,                         //     REPORT_SIZE (8)
    0x09, 0x51,                         //     USAGE (Contact Identifier)
    0x95, 0x01,                         //     REPORT_COUNT (1)             
    0x81, 0x02,                         //       INPUT (Data,Var,Abs) 
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desk..
    0x26, 0x00, 0x03,                   //     LOGICAL_MAXIMUM (768)            NOTE: ADJUST LOGICAL MAXIMUM FOR X BASED ON TOUCH SCREEN RESOLUTION!
    0x75, 0x10,                         //     REPORT_SIZE (16)             
    0x55, 0x0e,                         //     UNIT_EXPONENT (-2)           
    0x65, 0x13,                         //     UNIT (Inch,EngLinear)
    0x09, 0x30,                         //     USAGE (X)                    
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         
    0x46, 0xe8, 0x00,                   //     PHYSICAL_MAXIMUM (232)       NOTE: ADJUST PHYSICAL MAXIMUM FOR X BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)         
    0x46, 0x7e, 0x01,                   //     PHYSICAL_MAXIMUM (382)       NOTE: ADJUST PHYSICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN DIMENSION (100ths of an inch)!
    0x26, 0x00, 0x05,                   //     LOGICAL_MAXIMUM (1280)       NOTE: ADJUST LOGICAL MAXIMUM FOR Y BASED ON TOUCH SCREEN RESOLUTION!
    0x09, 0x31,                         //     USAGE (Y)                    
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)
    0xc0,                               //   END_COLLECTION
    0x05, 0x0d,	                        //   USAGE_PAGE (Digitizers)    
    0x09, 0x54,	                        //   USAGE (Actual count)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x08,                         //   REPORT_SIZE (8)    
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x85, REPORTID_MAX_COUNT,           //   REPORT_ID (Feature)              
    0x09, 0x55,                         //   USAGE(Maximum Count)
    0x25, 0x02,                         //   LOGICAL_MAXIMUM (2)
    0xb1, 0x02,                         //   FEATURE (Data,Var,Abs) 
    0xc0,                               // END_COLLECTION
    0x09, 0x0E,                         // USAGE (Configuration)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_FEATURE,             //   REPORT_ID (Feature)
    0x09, 0x22,                         //   USAGE (Finger)              
    0xa1, 0x00,                         //   COLLECTION (physical)
    0x09, 0x52,                         //     USAGE (Input Mode)         
    0x09, 0x53,                         //     USAGE (Device Index
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)      
    0x25, 0x0a,                         //     LOGICAL_MAXIMUM (10)
    0x75, 0x08,                         //     REPORT_SIZE (8)         
    0x95, 0x02,                         //     REPORT_COUNT (2)         
    0xb1, 0x02,                         //     FEATURE (Data,Var,Abs)
    0xc0,                               //   END_COLLECTION
    0xc0,                               // END_COLLECTION
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
#ifdef HID_MOUSE_PATH_SUPPORT
    0x09, 0x02,                         // USAGE (Mouse)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_MOUSE,               //   REPORT_ID (Mouse)
    0x09, 0x01,                         //   USAGE (Pointer)
    0xa1, 0x00,                         //   COLLECTION (Physical)
    0x05, 0x09,                         //     USAGE_PAGE (Button)
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)
    0x29, 0x02,                         //     USAGE_MAXIMUM (Button 2)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)
    0x95, 0x06,                         //     REPORT_COUNT (6)
    0x81, 0x03,                         //       INPUT (Cnst,Var,Abs)
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         //     USAGE (X)
    0x09, 0x31,                         //     USAGE (Y)
    0x75, 0x10,                         //     REPORT_SIZE (16)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x7f,                   //     LOGICAL_MAXIMUM (32767)
    0x81, 0x02,                         //       INPUT (Data,Var,Abs)
    0xc0,                               //   END_COLLECTION
    0xc0,                               // END_COLLECTION
#endif
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0xEE,                         // USAGE (HID_USAGE_KEYBOARD_MOBILE)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_CAPKEY,              //   REPORT_ID
    0x05, 0x07,                         //   USAGE_PAGE (Key Codes)
    0x09, 0x3B,                         //   USAGE(F2 Key)  - Start/Home
    0x09, 0x3C,                         //   USAGE(F3 Key)  - Search
    0x09, 0x29,                         //   USAGE(Esc Key) - Back
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x03,                         //   REPORT_COUNT (3)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x95, 0x1d,                         //   REPORT_COUNT (29)
    0x81, 0x03,                         //   INPUT (Cnst,Var,Abs)
    0xc0,                               // END_COLLECTION
};
const ULONG gdwcbReportDescriptor = sizeof(gReportDescriptor);

//
// HID Descriptor for a touch device
//
const HID_DESCRIPTOR gHidDescriptor =
{
    sizeof(HID_DESCRIPTOR),             //bLength
    HID_HID_DESCRIPTOR_TYPE,            //bDescriptorType
    HID_REVISION,                       //bcdHID
    0,                                  //bCountry - not localized
    1,                                  //bNumDescriptors
    {                                   //DescriptorList[0]
        HID_REPORT_DESCRIPTOR_TYPE,     //bReportType
        sizeof(gReportDescriptor)       //wReportLength
    }
};

NTSTATUS
TchReadReport(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request,
    OUT BOOLEAN *Pending
    )
/*++

Routine Description:

   Handles read requests from HIDCLASS, by forwarding the request.

Arguments:

   Device - Handle to WDF Device Object

   Request - Handle to request object
   
   Pending - flag to monitor if the request was sent down the stack

Return Value:

   On success, the function returns STATUS_SUCCESS
   On failure it passes the relevant error code to the caller.

--*/
{
    PDEVICE_EXTENSION devContext;
    NTSTATUS status;
    
    devContext = GetDeviceContext(Device);
    
    status = WdfRequestForwardToIoQueue(
            Request,
            devContext->PingPongQueue);
    
    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Failed to forward HID request to I/O queue - %!STATUS!",
            status);

        goto exit;
    }
    
    if (NULL != Pending)
    {
        *Pending = TRUE;
    }

    //
    // Service any interrupt that may have asserted while the framework had
    // interrupts disabled, or occurred before a read request was queued.
    //
    if (devContext->ServiceInterruptsAfterD0Entry == TRUE)
    {
        HID_INPUT_REPORT hidReport;
        BOOLEAN servicingComplete = FALSE;

        while (servicingComplete == FALSE)
        {
            TchServiceInterrupts(
                devContext->TouchContext,
                &devContext->I2CContext,
                &hidReport,
                devContext->InputMode,
                &servicingComplete);
        }

        devContext->ServiceInterruptsAfterD0Entry = FALSE;
    }
    
exit:

    return status;
}

NTSTATUS 
TchGetString(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
/*++

Routine Description:

    Returns string requested by the HIDCLASS driver

Arguments:

    Device - Handle to WDF Device Object

    Request - Handle to request object

Return Value:

    NTSTATUS indicating success or failure

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG_PTR lenId;
    NTSTATUS status;
    PWSTR strId;

    UNREFERENCED_PARAMETER(Device);
    
    status = STATUS_SUCCESS;
    
    irp = WdfRequestWdmGetIrp(Request);
    irpSp = IoGetCurrentIrpStackLocation(irp);
    switch ((ULONG_PTR)irpSp->Parameters.DeviceIoControl.Type3InputBuffer &
            0xffff)
    {
        case HID_STRING_ID_IMANUFACTURER:
            strId = gpwstrManufacturerID;
            break;

        case HID_STRING_ID_IPRODUCT:
            strId = gpwstrProductID;
            break;

        case HID_STRING_ID_ISERIALNUMBER:
            strId = gpwstrSerialNumber;
            break;

        default:
            strId = NULL;
            break;
    }

    lenId = strId ? (wcslen(strId)*sizeof(WCHAR) + sizeof(UNICODE_NULL)) : 0;
    if (strId == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < lenId)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        RtlCopyMemory(irp->UserBuffer, strId, lenId);
        irp->IoStatus.Information = lenId;
    }

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error getting device string - %!STATUS!",
            status);
    }
    
    return status;
}

NTSTATUS
TchGetHidDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
/*++

Routine Description:

    Finds the HID descriptor and copies it into the buffer provided by the 
    Request.

Arguments:

    Device - Handle to WDF Device Object

    Request - Handle to request object

Return Value:

    NTSTATUS indicating success or failure

--*/
{
    WDFMEMORY memory;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Device);
    
    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error getting HID descriptor request memory - %!STATUS!",
            status);
        goto exit;
    }

    //
    // Use hardcoded global HID Descriptor
    //
    status = WdfMemoryCopyFromBuffer(
        memory,
        0,
        (PUCHAR) &gHidDescriptor,
        sizeof(gHidDescriptor));

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error copying HID descriptor to request memory - %!STATUS!",
            status);
        goto exit;
    }

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, sizeof(gHidDescriptor));

exit:

    return status;
}

NTSTATUS
TchGetReportDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
/*++

Routine Description:

    Finds the report descriptor and copies it into the buffer provided by the
    Request.

Arguments:

    Device - Handle to WDF Device Object

    Request - Handle to request object

Return Value:

    NT status code.
	 success - STATUS_SUCCESS 
	 failure:
     STATUS_INVALID_PARAMETER - An invalid parameter was detected.

--*/
{
    WDFMEMORY memory;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Device);
    
    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error getting HID report descriptor request memory - %!STATUS!",
            status);
        goto exit;
    }

    //
    // Use hardcoded Report descriptor
    //
    status = WdfMemoryCopyFromBuffer(
        memory,
        0,
        (PUCHAR) gReportDescriptor,
        gdwcbReportDescriptor);

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error copying HID report descriptor to request memory - %!STATUS!",
            status);
        goto exit;
    }

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, gdwcbReportDescriptor);

exit:

    return status;
}

NTSTATUS
TchGetDeviceAttributes(
    IN WDFREQUEST Request
    )
/*++

Routine Description:

    Fill in the given struct _HID_DEVICE_ATTRIBUTES

Arguments:

    Request - Pointer to Request object.

Return Value:

    NTSTATUS indicating success or failure

--*/
{
    PHID_DEVICE_ATTRIBUTES deviceAttributes;
    NTSTATUS status;
    
    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof (HID_DEVICE_ATTRIBUTES),
        &deviceAttributes,
        NULL);

    if (!NT_SUCCESS(status)) 
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error retrieving device attribute output buffer - %!STATUS!",
            status);
        goto exit;
    }

    deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
    deviceAttributes->VendorID = gOEMVendorID;
    deviceAttributes->ProductID = gOEMProductID;
    deviceAttributes->VersionNumber = gOEMVersionID;

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, sizeof (HID_DEVICE_ATTRIBUTES));

exit:
   
    return status;
}

NTSTATUS
TchSetFeatureReport(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
/*++

Routine Description:

   Emulates setting a HID Feature report

Arguments:

   Device  - Framework device object
   Request - Framework request object

Return Value:

   NTSTATUS indicating success or failure

--*/
{
    PDEVICE_EXTENSION devContext;
    PHID_XFER_PACKET featurePacket;
    WDF_REQUEST_PARAMETERS params;
    NTSTATUS status;
    
    devContext = GetDeviceContext(Device);
    status = STATUS_SUCCESS;

    //
    // Validate Request Parameters
    //

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.InputBufferLength < 
        sizeof(HID_XFER_PACKET))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    featurePacket = 
        (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;

    if (featurePacket == NULL)
    { 
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    //
    // Process Request
    //

    switch (*(PUCHAR)featurePacket->reportBuffer)
    {
        case REPORTID_FEATURE:
        {
            PHID_FEATURE_REPORT inputModeReport =
                (PHID_FEATURE_REPORT) featurePacket->reportBuffer;

            if (featurePacket->reportBufferLen < sizeof(HID_FEATURE_REPORT))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto exit;
            }

            if ((inputModeReport->InputMode == MODE_MOUSE) ||
                (inputModeReport->InputMode == MODE_MULTI_TOUCH))
            {
                devContext->InputMode = inputModeReport->InputMode;
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
                goto exit;
            }

            break;
        }

        default:
        {
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
    }

exit:

    return status;
}

NTSTATUS
TchGetFeatureReport(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
/*++

Routine Description:

   Emulates retrieval of a HID Feature report

Arguments:

   Device  - Framework device object
   Request - Framework request object

Return Value:

   NTSTATUS indicating success or failure

--*/
{
    PDEVICE_EXTENSION devContext;
    PHID_XFER_PACKET featurePacket;
    WDF_REQUEST_PARAMETERS params;
    NTSTATUS status;
    
    devContext = GetDeviceContext(Device);
    status = STATUS_SUCCESS;

    //
    // Validate Request Parameters
    //

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.OutputBufferLength < 
        sizeof(HID_XFER_PACKET))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    featurePacket = 
        (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;

    if (featurePacket == NULL)
    { 
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    } 

    //
    // Process Request
    //

    switch (*(PUCHAR)featurePacket->reportBuffer)
    {
        case REPORTID_FEATURE:
        {
            PHID_FEATURE_REPORT inputModeReport = 
                (PHID_FEATURE_REPORT) featurePacket->reportBuffer;

            if (featurePacket->reportBufferLen < sizeof(HID_FEATURE_REPORT))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto exit;
            }

            inputModeReport->InputMode = devContext->InputMode;

            break;
        }

        case REPORTID_MAX_COUNT:
        {
            PHID_MAX_COUNT_REPORT maxCountReport = 
                (PHID_MAX_COUNT_REPORT) featurePacket->reportBuffer;
           
            if (featurePacket->reportBufferLen < sizeof(HID_MAX_COUNT_REPORT))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto exit;
            }

            maxCountReport->MaxCount = OEM_MAX_TOUCHES;

            break;
        }

        default:
        {
            status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
    }

exit:

    return status;
}
