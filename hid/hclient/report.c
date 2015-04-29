/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    report.c

Abstract:

    This module contains the code for reading/writing hid reports and 
    translating those HID reports into useful information. 

Environment:

    User mode

--*/

#include <stdlib.h>
#include <wtypes.h>
#include "hidsdi.h"
#include "hid.h"


BOOLEAN
Read (
   PHID_DEVICE    HidDevice
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, obtain a read report and unpack the values
   into the InputData array.
--*/
{
    DWORD       bytesRead;
    BOOLEAN     result = FALSE;

    if (!ReadFile (HidDevice->HidDevice,
                  HidDevice->InputReportBuffer,
                  HidDevice->Caps.InputReportByteLength,
                  &bytesRead,
                  NULL)) 
    {
        goto Done;
    }

    ASSERT (bytesRead == HidDevice->Caps.InputReportByteLength);
    if (bytesRead != HidDevice->Caps.InputReportByteLength)
    {
        goto Done;
    }

    result = UnpackReport (HidDevice->InputReportBuffer,
                           HidDevice->Caps.InputReportByteLength,
                           HidP_Input,
                           HidDevice->InputData,
                           HidDevice->InputDataLength,
                           HidDevice->Ppd);
Done:
    return result;
}

BOOLEAN
ReadOverlapped (
    PHID_DEVICE     HidDevice,
    HANDLE          CompletionEvent
   )
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, obtain a read report and unpack the values
   into the InputData array.
--*/
{
    static OVERLAPPED  overlap;
    DWORD       bytesRead;
    BOOL        readStatus;

    /*
    // Setup the overlap structure using the completion event passed in to
    //  to use for signalling the completion of the Read
    */

    memset(&overlap, 0, sizeof(OVERLAPPED));
    
    overlap.hEvent = CompletionEvent;
    
    /*
    // Execute the read call saving the return code to determine how to 
    //  proceed (ie. the read completed synchronously or not).
    */

    readStatus = ReadFile ( HidDevice -> HidDevice,
                            HidDevice -> InputReportBuffer,
                            HidDevice -> Caps.InputReportByteLength,
                            &bytesRead,
                            &overlap);
                          
    /*
    // If the readStatus is FALSE, then one of two cases occurred.  
    //  1) ReadFile call succeeded but the Read is an overlapped one.  Here,
    //      we should return TRUE to indicate that the Read succeeded.  However,
    //      the calling thread should be blocked on the completion event
    //      which means it won't continue until the read actually completes
    //    
    //  2) The ReadFile call failed for some unknown reason...In this case,
    //      the return code will be FALSE
    */        

    if (!readStatus) 
    {
        return (ERROR_IO_PENDING == GetLastError());
    }

    /*
    // If readStatus is TRUE, then the ReadFile call completed synchronously,
    //   since the calling thread is probably going to wait on the completion
    //   event, signal the event so it knows it can continue.
    */

    else 
    {
        SetEvent(CompletionEvent);
        return (TRUE);
    }
}

BOOLEAN
Write (
   PHID_DEVICE    HidDevice
)
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, take the information in the HID_DATA array
   pack it into multiple write reports and send each report to the HID device
--*/
{
    DWORD     bytesWritten;
    PHID_DATA pData;
    ULONG     Index;
    BOOLEAN   Status;
    BOOLEAN   WriteStatus;

    /*
    // Begin by looping through the HID_DEVICE's HID_DATA structure and setting
    //   the IsDataSet field to FALSE to indicate that each structure has
    //   not yet been set for this Write call.
    */

    pData = HidDevice -> OutputData;

    for (Index = 0; Index < HidDevice -> OutputDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    /*
    // In setting all the data in the reports, we need to pack a report buffer
    //   and call WriteFile for each report ID that is represented by the 
    //   device structure.  To do so, the IsDataSet field will be used to 
    //   determine if a given report field has already been set.
    */

    Status = TRUE;

    pData = HidDevice -> OutputData;
    for (Index = 0; Index < HidDevice -> OutputDataLength; Index++, pData++) 
    {

        if (!pData -> IsDataSet) 
        {
            /*
            // Package the report for this data structure.  PackReport will
            //    set the IsDataSet fields of this structure and any other 
            //    structures that it includes in the report with this structure
            */

            PackReport (HidDevice->OutputReportBuffer,
                     HidDevice->Caps.OutputReportByteLength,
                     HidP_Output,
                     pData,
                     HidDevice->OutputDataLength - Index,
                     HidDevice->Ppd);

            /*
            // Now a report has been packaged up...Send it down to the device
            */

            WriteStatus = WriteFile (HidDevice->HidDevice,
                                  HidDevice->OutputReportBuffer,
                                  HidDevice->Caps.OutputReportByteLength,
                                  &bytesWritten,
                                  NULL) && (bytesWritten == HidDevice -> Caps.OutputReportByteLength);

            Status = Status && WriteStatus;                         
        }
    }
    return (Status);
}

BOOLEAN
SetFeature (
    PHID_DEVICE    HidDevice
)
/*++
RoutineDescription:
Given a struct _HID_DEVICE, take the information in the HID_DATA array
pack it into multiple reports and send it to the hid device via HidD_SetFeature()
--*/
{
    PHID_DATA pData;
    ULONG     Index;
    BOOLEAN   Status;
    BOOLEAN   FeatureStatus;
    /*
    // Begin by looping through the HID_DEVICE's HID_DATA structure and setting
    //   the IsDataSet field to FALSE to indicate that each structure has
    //   not yet been set for this SetFeature() call.
    */

    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    /*
    // In setting all the data in the reports, we need to pack a report buffer
    //   and call WriteFile for each report ID that is represented by the 
    //   device structure.  To do so, the IsDataSet field will be used to 
    //   determine if a given report field has already been set.
    */

    Status = TRUE;

    pData = HidDevice -> FeatureData;
    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        if (!pData -> IsDataSet) 
        {
            /*
            // Package the report for this data structure.  PackReport will
            //    set the IsDataSet fields of this structure and any other 
            //    structures that it includes in the report with this structure
            */

            PackReport (HidDevice->FeatureReportBuffer,
                     HidDevice->Caps.FeatureReportByteLength,
                     HidP_Feature,
                     pData,
                     HidDevice->FeatureDataLength - Index,
                     HidDevice->Ppd);

            /*
            // Now a report has been packaged up...Send it down to the device
            */

            FeatureStatus =(HidD_SetFeature (HidDevice->HidDevice,
                                          HidDevice->FeatureReportBuffer,
                                          HidDevice->Caps.FeatureReportByteLength));

            Status = FeatureStatus && Status;
        }
    }
    return (Status);
}

BOOLEAN
GetFeature (
   PHID_DEVICE    HidDevice
)
/*++
RoutineDescription:
   Given a struct _HID_DEVICE, fill in the feature data structures with
   all features on the device.  May issue multiple HidD_GetFeature() calls to
   deal with multiple report IDs.
--*/
{
    ULONG     Index;
    PHID_DATA pData;
    BOOLEAN   FeatureStatus;
    BOOLEAN   Status;

    /*
    // As with writing data, the IsDataSet value in all the structures should be
    //    set to FALSE to indicate that the value has yet to have been set
    */

    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        pData -> IsDataSet = FALSE;
    }

    /*
    // Next, each structure in the HID_DATA buffer is filled in with a value
    //   that is retrieved from one or more calls to HidD_GetFeature.  The 
    //   number of calls is equal to the number of reportIDs on the device
    */

    Status = TRUE; 
    pData = HidDevice -> FeatureData;

    for (Index = 0; Index < HidDevice -> FeatureDataLength; Index++, pData++) 
    {
        /*
        // If a value has yet to have been set for this structure, build a report
        //    buffer with its report ID as the first byte of the buffer and pass
        //    it in the HidD_GetFeature call.  Specifying the report ID in the
        //    first specifies which report is actually retrieved from the device.
        //    The rest of the buffer should be zeroed before the call
        */

        if (!pData -> IsDataSet) 
        {
            memset(HidDevice -> FeatureReportBuffer, 0x00, HidDevice->Caps.FeatureReportByteLength);

            HidDevice -> FeatureReportBuffer[0] = (UCHAR) pData -> ReportID;

            FeatureStatus = HidD_GetFeature (HidDevice->HidDevice,
                                              HidDevice->FeatureReportBuffer,
                                              HidDevice->Caps.FeatureReportByteLength);

            /*
            // If the return value is TRUE, scan through the rest of the HID_DATA
            //    structures and fill whatever values we can from this report
            */


            if (FeatureStatus) 
            {
                FeatureStatus = UnpackReport ( HidDevice->FeatureReportBuffer,
                                           HidDevice->Caps.FeatureReportByteLength,
                                           HidP_Feature,
                                           HidDevice->FeatureData,
                                           HidDevice->FeatureDataLength,
                                           HidDevice->Ppd);
            }

            Status = Status && FeatureStatus;
        }
   }

   return (Status);
}


BOOLEAN
UnpackReport (
   _In_reads_bytes_(ReportBufferLength)PCHAR ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN OUT   PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
)
/*++
Routine Description:
   Given ReportBuffer representing a report from a HID device where the first
   byte of the buffer is the report ID for the report, extract all the HID_DATA
   in the Data list from the given report.
--*/
{
    ULONG       numUsages; // Number of usages returned from GetUsages.
    ULONG       i;
    UCHAR       reportID;
    ULONG       Index;
    ULONG       nextUsage;
    BOOLEAN     result = FALSE;

    reportID = ReportBuffer[0];

    for (i = 0; i < DataLength; i++, Data++) 
    {
        if (reportID == Data->ReportID) 
        {
            if (Data->IsButtonData) 
            {
                numUsages = Data->ButtonData.MaxUsageLength;

                Data->Status = HidP_GetUsages (ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages,
                                               &numUsages,
                                               Ppd,
                                               ReportBuffer,
                                               ReportBufferLength);

                if (HIDP_STATUS_SUCCESS != Data->Status)
                {
                    goto Done;
                }
                
                //
                // Get usages writes the list of usages into the buffer
                // Data->ButtonData.Usages newUsage is set to the number of usages
                // written into this array.
                // A usage cannot not be defined as zero, so we'll mark a zero
                // following the list of usages to indicate the end of the list of
                // usages
                //
                // NOTE: One anomaly of the GetUsages function is the lack of ability
                //        to distinguish the data for one ButtonCaps from another
                //        if two different caps structures have the same UsagePage
                //        For instance:
                //          Caps1 has UsagePage 07 and UsageRange of 0x00 - 0x167
                //          Caps2 has UsagePage 07 and UsageRange of 0xe0 - 0xe7
                //
                //        However, calling GetUsages for each of the data structs
                //          will return the same list of usages.  It is the 
                //          responsibility of the caller to set in the HID_DEVICE
                //          structure which usages actually are valid for the
                //          that structure. 
                //      

                /*
                // Search through the usage list and remove those that 
                //    correspond to usages outside the define ranged for this
                //    data structure.
                */
                
                for (Index = 0, nextUsage = 0; Index < numUsages; Index++) 
                {
                    if (Data -> ButtonData.UsageMin <= Data -> ButtonData.Usages[Index] &&
                            Data -> ButtonData.Usages[Index] <= Data -> ButtonData.UsageMax) 
                    {
                        Data -> ButtonData.Usages[nextUsage++] = Data -> ButtonData.Usages[Index];
                        
                    }
                }

                if (nextUsage < Data -> ButtonData.MaxUsageLength) 
                {
                    Data->ButtonData.Usages[nextUsage] = 0;
                }
            }
            else 
            {
                Data->Status = HidP_GetUsageValue (
                                                ReportType,
                                                Data->UsagePage,
                                                0,               // All Collections.
                                                Data->ValueData.Usage,
                                                &Data->ValueData.Value,
                                                Ppd,
                                                ReportBuffer,
                                                ReportBufferLength);

                if (HIDP_STATUS_SUCCESS != Data->Status)
                {
                    goto Done;
                }

                Data->Status = HidP_GetScaledUsageValue (
                                                       ReportType,
                                                       Data->UsagePage,
                                                       0, // All Collections.
                                                       Data->ValueData.Usage,
                                                       &Data->ValueData.ScaledValue,
                                                       Ppd,
                                                       ReportBuffer,
                                                       ReportBufferLength);

                if (HIDP_STATUS_SUCCESS != Data->Status &&
                    HIDP_STATUS_NULL != Data->Status)
                {
                    goto Done;
                }

            } 
            Data -> IsDataSet = TRUE;
        }
    }

    result = TRUE;

Done:
    return (result);
}


BOOLEAN
PackReport (
   _Out_writes_bytes_(ReportBufferLength)PCHAR ReportBuffer,
   IN  USHORT               ReportBufferLength,
   IN  HIDP_REPORT_TYPE     ReportType,
   IN  PHID_DATA            Data,
   IN  ULONG                DataLength,
   IN  PHIDP_PREPARSED_DATA Ppd
   )
/*++
Routine Description:
   This routine takes in a list of HID_DATA structures (DATA) and builds 
      in ReportBuffer the given report for all data values in the list that 
      correspond to the report ID of the first item in the list.  

   For every data structure in the list that has the same report ID as the first
      item in the list will be set in the report.  Every data item that is 
      set will also have it's IsDataSet field marked with TRUE.

   A return value of FALSE indicates an unexpected error occurred when setting
      a given data value.  The caller should expect that assume that no values
      within the given data structure were set.

   A return value of TRUE indicates that all data values for the given report
      ID were set without error.
--*/
{
    ULONG       numUsages; // Number of usages to set for a given report.
    ULONG       i;
    ULONG       CurrReportID;
    BOOLEAN     result = FALSE;

    /*
    // All report buffers that are initially sent need to be zero'd out
    */

    memset (ReportBuffer, (UCHAR) 0, ReportBufferLength);

    /*
    // Go through the data structures and set all the values that correspond to
    //   the CurrReportID which is obtained from the first data structure 
    //   in the list
    */

    CurrReportID = Data -> ReportID;

    for (i = 0; i < DataLength; i++, Data++) 
    {
        /*
        // There are two different ways to determine if we set the current data
        //    structure: 
        //    1) Store the report ID were using and only attempt to set those
        //        data structures that correspond to the given report ID.  This
        //        example shows this implementation.
        //
        //    2) Attempt to set all of the data structures and look for the 
        //        returned status value of HIDP_STATUS_INVALID_REPORT_ID.  This 
        //        error code indicates that the given usage exists but has a 
        //        different report ID than the report ID in the current report 
        //        buffer
        */

        if (Data -> ReportID == CurrReportID) 
        {
            if (Data->IsButtonData) 
            {
                numUsages = Data->ButtonData.MaxUsageLength;
                Data->Status = HidP_SetUsages (ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages,
                                               &numUsages,
                                               Ppd,
                                               ReportBuffer,
                                               ReportBufferLength);
            }
            else
            {
                Data->Status = HidP_SetUsageValue (ReportType,
                                                   Data->UsagePage,
                                                   0, // All Collections.
                                                   Data->ValueData.Usage,
                                                   Data->ValueData.Value,
                                                   Ppd,
                                                   ReportBuffer,
                                                   ReportBufferLength);
            }

            if (HIDP_STATUS_SUCCESS != Data->Status)
            {
                goto Done;
            }
        }
    }   

    /*
    // At this point, all data structures that have the same ReportID as the
    //    first one will have been set in the given report.  Time to loop 
    //    through the structure again and mark all of those data structures as
    //    having been set.
    */

    for (i = 0; i < DataLength; i++, Data++) 
    {
        if (CurrReportID == Data -> ReportID)
        {
            Data -> IsDataSet = TRUE;
        }
    }

    result = TRUE;

Done:
    return result;
}

