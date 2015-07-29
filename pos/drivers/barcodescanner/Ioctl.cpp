#include <pch.h>

NTSTATUS ProcessGetPropertyRequest(_In_ WDFREQUEST Request, _In_ size_t InputBufferLength, _Inout_ ULONG_PTR* Information);
NTSTATUS ProcessSetPropertyRequest(_In_ WDFREQUEST Request, _In_ size_t InputBufferLength, _Inout_ ULONG_PTR* Information);
NTSTATUS ProcessRetrieveStatisticsRequest(_In_ WDFREQUEST Request, _In_ size_t OutputBufferLength, _Inout_ ULONG_PTR* Information);
NTSTATUS ProcessResetStatisticsRequest(_In_ WDFREQUEST Request);
NTSTATUS ProcessUpdateStatisticsRequest(_In_ WDFREQUEST Request);
NTSTATUS ProcessCheckHealthRequest(_In_ WDFREQUEST Request, _Inout_ ULONG_PTR* Information);
NTSTATUS ProcessGetDeviceBasicsRequest(_In_ WDFREQUEST Request, _Inout_ ULONG_PTR* Information);

/*
** Driver TODO: Complete the implementation of EvtIoDeviceControl for your specific device (if necessary)
**
** WDF calls this callback when a device instance is added to the driver.  Good drivers will do a lot of
** work here to set up everything necessary, such as adding callbacks for PNP power state changes.
** This function defines an IO queue for handling DeviceIoControl and file read requests, both of which are
** important to the POS barcode scanner model.
**
** Note that this is not a complete device add implementation, as the PNP power callbacks are not handled.
** Additionally, driver writers may wish to set up additional queues to serialize device property requests
** (see Ioctl.cpp for more info).
*/
VOID EvtIoDeviceControl(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t OutputBufferLength, _In_ size_t InputBufferLength, _In_ ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(Queue);

    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR information = 0;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    WDFFILEOBJECT fileObject = WdfRequestGetFileObject(Request);

    // These are the set of IOCTLs that your device should handle to work with the Windows.Devices.PointOfService APIs.
    switch (IoControlCode)
    {
    // The first three IOCTLs shouldn't require additional processing other than handing them off to PosCx
    case IOCTL_POINT_OF_SERVICE_CLAIM_DEVICE:
        status = PosCxClaimDevice(device, Request);
        break;

    case IOCTL_POINT_OF_SERVICE_RELEASE_DEVICE:
        status = PosCxReleaseDevice(device, fileObject);
        break;

    case IOCTL_POINT_OF_SERVICE_RETAIN_DEVICE:
        status = PosCxRetainDevice(device, Request);
        break;


    case IOCTL_POINT_OF_SERVICE_GET_PROPERTY:
        status = ProcessGetPropertyRequest(Request, InputBufferLength, &information);
        break;

    case IOCTL_POINT_OF_SERVICE_SET_PROPERTY:
        status = ProcessSetPropertyRequest(Request, InputBufferLength, &information);
        break;

    case IOCTL_POINT_OF_SERVICE_RETRIEVE_STATISTICS:
        status = ProcessRetrieveStatisticsRequest(Request, OutputBufferLength, &information);
        break;

    case IOCTL_POINT_OF_SERVICE_RESET_STATISTICS:
        status = ProcessResetStatisticsRequest(Request);
        break;

    case IOCTL_POINT_OF_SERVICE_UPDATE_STATISTICS:
        status = ProcessUpdateStatisticsRequest(Request);
        break;

    case IOCTL_POINT_OF_SERVICE_CHECK_HEALTH:
        status = ProcessCheckHealthRequest(Request, &information);
        break;

    // The Get Device Basics IOCTL is always the first IOCTL called by an application using the Windows.Devices.PointOfService APIs.
    // Use it to determine when to call PosCxMarkPosApp (see notes about apps marked this way in IoRead.cpp)
    case IOCTL_POINT_OF_SERVICE_GET_DEVICE_BASICS:
        status = ProcessGetDeviceBasicsRequest(Request, &information);
        (void)PosCxMarkPosApp(device, fileObject, TRUE);
        break;

    default:
        // Your device may support additional IOCTLs.  In this sample, we return failure for anything else.
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (status != STATUS_PENDING)
    {
        WdfRequestCompleteWithInformation(Request, status, information);
    }
}

/*
** Driver TODO: Add code to handle various get-property cases.
**
** Implement this function to handle property get requests.
*/
NTSTATUS ProcessGetPropertyRequest(_In_ WDFREQUEST Request, _In_ size_t InputBufferLength, _Inout_ ULONG_PTR* Information)
{
    if (Information == nullptr || Request == nullptr || InputBufferLength < sizeof(PosPropertyId))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // POS properties are identified by the property ID that's transmitted in the input buffer.
    PosPropertyId* propertyId;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(PosPropertyId),
        reinterpret_cast<PVOID*>(&propertyId),
        nullptr
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // All get properties will need access to the output buffer in order to return results.
    // The minimum size returned is a UINT32
    void* outputBuffer;
    size_t outputBufferLength;
    status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(UINT32),
        &outputBuffer,
        &outputBufferLength
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Handle this set of readable properties
    switch (*propertyId)
    {
    case PosPropertyId::IsEnabled:
        // BOOL result, true when the app has called SetProperty(IsEnabled) = TRUE
        {
            BOOL isEnabled = TRUE; // Get this value from device context or by querying the device
            *((BOOL*)outputBuffer) = isEnabled;
            *Information = sizeof(BOOL);
        }
        break;

    case PosPropertyId::IsDisabledOnDataReceived:
        // BOOL result, true when the app has called SetProperty(IsDisabledOnDataReceived) = TRUE
        {
            BOOL isDisabledOnDataReceived = TRUE; // Get this value from device context or by querying the device
            *((BOOL*)outputBuffer) = isDisabledOnDataReceived;
            *Information = sizeof(BOOL);
        }
        break;

    case PosPropertyId::BarcodeScannerIsDecodeDataEnabled:
        // BOOL result, true when the app has called SetProperty(BarcodeScannerIsDecodeDataEnabled) = TRUE
        {
            BOOL isDecodeDataEnabled = TRUE; // Get this value from device context or by querying the device
            *((BOOL*)outputBuffer) = isDecodeDataEnabled;
            *Information = sizeof(BOOL);
        }
        break;

    case PosPropertyId::BarcodeScannerCapabilities:
        {
            // PosBarcodeScannerCapabilitiesType2 result
            // These capabilities are likely hard-coded for the specific device
            PosBarcodeScannerCapabilitiesType2 capabilities;
            capabilities.PosBarcodeScannerCapabilities.IsImagePreviewSupported = TRUE;
            capabilities.PosBarcodeScannerCapabilities.IsStatisticsReportingSupported = TRUE;
            capabilities.PosBarcodeScannerCapabilities.IsStatisticsUpdatingSupported = TRUE;
            capabilities.PosBarcodeScannerCapabilities.PowerReportingType = DriverUnifiedPosPowerReportingType::Standard;
            capabilities.IsSoftwareTriggerSupported = TRUE;
            size_t bytesToCopy = sizeof(PosBarcodeScannerCapabilitiesType2);
            if (outputBufferLength < bytesToCopy)
            {
                bytesToCopy = outputBufferLength;
                status = STATUS_BUFFER_OVERFLOW;
            }
            memcpy(outputBuffer, &capabilities, bytesToCopy);
            *Information = bytesToCopy;
        }
        break;

    case PosPropertyId::BarcodeScannerSupportedSymbologies:
        {
            // Returns a length-prefixed array of symbologies
            BarcodeSymbology exampleSymbologies[] = { BarcodeSymbology::Upca, BarcodeSymbology::Ean13, BarcodeSymbology::Pdf417 };
            UINT32* outputData = (UINT32*)outputBuffer;
            size_t copiedDataLength = 0;
            if (outputBufferLength >= sizeof(UINT32))
            {
                *outputData = ARRAYSIZE(exampleSymbologies);
                copiedDataLength += sizeof(UINT32);
            }
            for (UINT32 index = 0; index < ARRAYSIZE(exampleSymbologies); ++index)
            {
                if (copiedDataLength + sizeof(UINT32) > outputBufferLength)
                {
                    break;
                }
                // Ensure the output array elements are stored as UINT32s
                outputData[index + 1] = (UINT32)exampleSymbologies[index];
                copiedDataLength += sizeof(UINT32);
            }
            *Information = copiedDataLength;
        }
        break;

    case PosPropertyId::BarcodeScannerSupportedProfiles:
        {
            // Profiles are sets of settings that can be applied together.  This property returns an encoded array of profile names
            LPCWSTR exampleProfiles[] = { L"Profile1", L"Profile2" };

            if (outputBufferLength >= sizeof(PosProfileType))
            {
                PosProfileType* header = (PosProfileType*)outputBuffer;
                header->BufferSize = sizeof(PosProfileType);
                header->ProfileCount = 0;

                *Information = header->BufferSize;

                for (UINT32 profileIndex = 0; profileIndex < ARRAYSIZE(exampleProfiles); ++profileIndex)
                {
                    UINT32 stringLen;
                    if (!NT_SUCCESS(status = RtlSizeTToUInt32(wcslen(exampleProfiles[profileIndex]), &stringLen)))
                    {
                        break;
                    }

                    UINT32 stringLenInBytes = stringLen*sizeof(WCHAR);
                
                    // If there's enough room in the output buffer, we can use the previous length as the starting point to copy the profile string
                    UINT32 previousLength = header->BufferSize;

                    // each string has it's own header (that just contains the string length in bytes)
                    header->BufferSize += sizeof(PosStringType);
                    header->BufferSize += stringLenInBytes;
                    ++(header->ProfileCount);

                    if (outputBufferLength >= header->BufferSize)
                    {
                        // There's enough room for this string
                        PosStringType* stringHeader = (PosStringType*)((BYTE*)outputBuffer + previousLength);
                        stringHeader->DataLengthInBytes = stringLenInBytes;

                        WCHAR* stringStart = (WCHAR*)((BYTE*)outputBuffer + previousLength + sizeof(PosStringType));
                    
                        // memcpy because we don't null terminate these strings
                        memcpy(stringStart, exampleProfiles[profileIndex], stringLenInBytes);

                        *Information = header->BufferSize;
                    }
                }

                if (header->BufferSize > outputBufferLength)
                {
                    status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        break;
    default:
        // no other readable properties for barcode scanner
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/*
** Driver TODO: Add code to handle various set-property cases.
**
** Implement this function to handle property set requests.
*/
NTSTATUS ProcessSetPropertyRequest(_In_ WDFREQUEST Request, _In_ size_t InputBufferLength, _Inout_ ULONG_PTR* Information)
{
    if (Information == nullptr || Request == nullptr || InputBufferLength < sizeof(PosPropertyId))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // POS properties are identified by the property ID that's transmitted in the input buffer.
    // The data that is used to set the property immediately follows the property ID, so the input buffer must be big enough to contain both.
    PosPropertyId* propertyId;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(PosPropertyId),
        reinterpret_cast<PVOID*>(&propertyId),
        nullptr
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    size_t argumentLength = InputBufferLength - sizeof(PosPropertyId);
    void* argumentData = (void*)(propertyId + 1);

    // Handle this set of writable properties
    switch (*propertyId)
    {
    case PosPropertyId::IsEnabled:
        // BOOL value
        if (argumentLength >= sizeof(BOOL))
        {
            // The driver should use this value to ensure the device is ready to take data.
            // The value may also need to be cached in a device context object so that it can be returned in GetProperty(IsEnabled)
            BOOL isEnabled = *((BOOL*)argumentData);
            UNREFERENCED_PARAMETER(isEnabled);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::IsDisabledOnDataReceived:
        // BOOL value
        if (argumentLength >= sizeof(BOOL))
        {
            // Typically this value will get cached in the device context so that, when
            // a barcode scan occurs, the driver can disable the device.
            BOOL isDisabledOnDataReceived = *((BOOL*)argumentData);
            UNREFERENCED_PARAMETER(isDisabledOnDataReceived);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::BarcodeScannerIsDecodeDataEnabled:
        // BOOL value
        if (argumentLength >= sizeof(BOOL))
        {
            // Typically this value will get cached in the device context so that, when
            // a barcode scan occurs, the driver can decode the raw data to get the scan data label.
            BOOL isDecodeDataEnabled = *((BOOL*)argumentData);
            UNREFERENCED_PARAMETER(isDecodeDataEnabled);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::BarcodeScannerActiveSymbologies:
        // UINT32 array with length prefix value
        if (argumentLength >= sizeof(UINT32))
        {
            UINT32 symbologyCount = *((UINT32*)argumentData);

            // knowing the number of symbology values in the input buffer, we know how big the buffer should be.
            // Add 1 to count to include the count value itself.
            UINT32 requiredArgumentLength = (symbologyCount + 1) * sizeof(UINT32);

            if (argumentLength >= requiredArgumentLength)
            {
                // The driver can copy this array into the device context, so that it can look up the symbology
                // of an incoming scan to see whether its valid to pass on to the application.
                // Alternatively, if the hardware supports restricting the data to specific symbologies, the
                // driver can do that now.
                for (UINT32 index = 0; index < symbologyCount; ++index)
                {
                    BarcodeSymbology symbologyValue = (BarcodeSymbology)((UINT32*)argumentData)[index + 1];
                    UNREFERENCED_PARAMETER(symbologyValue);
                }
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::BarcodeScannerActiveProfile:
        // PosStringType value (length prefixed wide string)
        if (argumentLength >= sizeof(PosStringType))
        {
            PosStringType* header = (PosStringType*)argumentData;
            if (argumentLength >= (sizeof(PosStringType)+header->DataLengthInBytes))
            {
                WCHAR* profileName = (WCHAR*)(header + 1);
                size_t profileLength = header->DataLengthInBytes / sizeof(WCHAR);

                // Determine the profile requested and apply the settings if found
                if (!wcsncmp(L"Profile1", profileName, profileLength))
                {
                    // For example, apply Profile1 settings
                }
            }
        }
        break;

    default:
        // no other writable properties for barcode scanner
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/*
** Driver TODO: Replace the data in the ProcessRetrieveStatisticsRequest with your own statistics data
**
** Implement this function to handle retrieve statistics requests.
*/
NTSTATUS ProcessRetrieveStatisticsRequest(_In_ WDFREQUEST Request, _In_ size_t OutputBufferLength, _Inout_ ULONG_PTR* Information)
{
    if (Information == nullptr || Request == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    struct
    {
        PosStatisticsHeader Header;
        PosValueStatisticsEntry Entries[1];
    } StatisticsData;

    StatisticsData.Header.DataLength = sizeof(StatisticsData);
    wcscpy_s(StatisticsData.Header.DeviceInformation.DeviceCategory, L"Scanner");
    wcscpy_s(StatisticsData.Header.DeviceInformation.FirmwareRevision, L"<eg, 1.1>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.InstallationDate, L"<installation date>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.Interface, L"<eg, USB>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.ManufactureDate, L"<eg, 2015/03/17>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.ManufacturerName, L"<eg, Conteso>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.MechanicalRevision, L"<eg, 2.0a>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.ModelName, L"<eg, Scanner III>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.SerialNumber, L"<eg, 12345>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.UnifiedPOSVersion, L"1.14");
    StatisticsData.Header.EntryCount = 1;
    wcscpy_s(StatisticsData.Entries[0].EntryName, L"<device specific statistics value>");
    StatisticsData.Entries[0].Value = (LONG)1;

    // This IOCTL is called twice by the Windows.Devices.PointOfService APIs
    // The first time will just retrieve the header to determine how big the buffer needs to be.
    PVOID outputBuffer;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(PosStatisticsHeader),
        &outputBuffer,
        nullptr
        );

    if (!NT_SUCCESS(status))
    {
        *Information = sizeof(StatisticsData);
        return status;
    }

    if (OutputBufferLength == sizeof(PosStatisticsHeader))
    {
        memcpy(outputBuffer, &(StatisticsData.Header), sizeof(PosStatisticsHeader));
        *Information = sizeof(PosStatisticsHeader);
    }
    else if (OutputBufferLength < sizeof(StatisticsData))
    {
        *Information = sizeof(StatisticsData);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        memcpy(outputBuffer, &StatisticsData, sizeof(StatisticsData));
        *Information = sizeof(StatisticsData);
    }

    return status;
}

/*
** Driver TODO: loop over statisticsEntry[0]...statisticsEntry[inputBuffer->EntryCount - 1] and reset each statistics value named
**
** Implement this function to handle statistics reset requests.
*/
NTSTATUS ProcessResetStatisticsRequest(_In_ WDFREQUEST Request)
{
    if (Request == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // The input buffer must be PosStatisticsHeader followed by one or more PosValueStatisticsEntry (where the value is ignored, just the name is used to 
    // reset the statistics value).
    PosStatisticsHeader* inputBuffer;
    size_t totalLength;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(PosStatisticsHeader),
        (PVOID*)&inputBuffer,
        &totalLength);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (inputBuffer->DataLength > totalLength)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    size_t entryLength = inputBuffer->DataLength - sizeof(PosStatisticsHeader);
    if (
        entryLength % sizeof(PosValueStatisticsEntry) ||
        (entryLength / sizeof(PosValueStatisticsEntry)) != inputBuffer->EntryCount ||
        inputBuffer->EntryCount == 0
        )
    {
        return STATUS_INVALID_PARAMETER;
    }

    PosValueStatisticsEntry* statisticsEntry = (PosValueStatisticsEntry*) (inputBuffer + 1);

    for (UINT32 index = 0; index < inputBuffer->EntryCount; ++index)
    {
        // reset this value:
        statisticsEntry[index].EntryName;
    }

    return STATUS_SUCCESS;
}

/*
** Driver TODO: loop over statisticsEntry[0]...statisticsEntry[inputBuffer->EntryCount - 1] and update each statistics value named
**
** Implement this function to handle statistics update requests.
*/
NTSTATUS ProcessUpdateStatisticsRequest(_In_ WDFREQUEST Request)
{
    if (Request == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // The input buffer must be PosStatisticsHeader followed by one or more PosValueStatisticsEntry
    PosStatisticsHeader* inputBuffer;
    size_t totalLength;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(PosStatisticsHeader),
        (PVOID*)&inputBuffer,
        &totalLength);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (inputBuffer->DataLength > totalLength)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    size_t entryLength = inputBuffer->DataLength - sizeof(PosStatisticsHeader);
    if (
        entryLength % sizeof(PosValueStatisticsEntry) ||
        (entryLength / sizeof(PosValueStatisticsEntry)) != inputBuffer->EntryCount ||
        inputBuffer->EntryCount == 0
        )
    {
        return STATUS_INVALID_PARAMETER;
    }

    PosValueStatisticsEntry* statisticsEntry = (PosValueStatisticsEntry*)(inputBuffer + 1);

    for (UINT32 index = 0; index < inputBuffer->EntryCount; ++index)
    {
        // update the statistics entry:
        statisticsEntry[index].EntryName;
        // with the value:
        statisticsEntry[index].Value;
    }

    return STATUS_SUCCESS;
}

/*
** Driver TODO: Add code to ProcessCheckHealthRequest to handle different health check cases.  The result should be a localized string that is returned to the user in the output buffer of the IOCTL.
**
** Implement this function to handle health check requests.
*/
NTSTATUS ProcessCheckHealthRequest(_In_ WDFREQUEST Request, _Inout_ ULONG_PTR* Information)
{
    if (Information == nullptr || Request == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    DriverUnifiedPosHealthCheckLevel* level;

    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(DriverUnifiedPosHealthCheckLevel),
        (PVOID*)&level,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PosStringType* outputBuffer;
    size_t outputBufferLength;
    status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(PosStringType),
        (void**)(&outputBuffer),
        &outputBufferLength
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    switch (*level)
    {
    case DriverUnifiedPosHealthCheckLevel::POSInternal:
    case DriverUnifiedPosHealthCheckLevel::External:
    case DriverUnifiedPosHealthCheckLevel::Interactive:
        {
            // Handle the specific health check level, depending on the applicability to your device.
            // Return the result as a string that the user can use to determine whether the device is
            // operational or needs attention.
            LPCWSTR result = L"OK";
            size_t lengthInBytes = wcslen(result) * sizeof(WCHAR);
            status = RtlSizeTToUInt32(lengthInBytes, &(outputBuffer->DataLengthInBytes));
            if (NT_SUCCESS(status))
            {
                *Information = sizeof(PosStringType);
                if (outputBufferLength >= sizeof(PosStringType)+outputBuffer->DataLengthInBytes)
                {
                    void* outputStringStart = (void*)(outputBuffer + 1);
                    memcpy(outputStringStart, result, outputBuffer->DataLengthInBytes);
                    *Information += outputBuffer->DataLengthInBytes;
                }
                else
                {
                    status = STATUS_BUFFER_OVERFLOW;
                }
            }
        }
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/*
** Driver TODO:
**
** Implement this function to handle the initial handshake IOCTL for Windows.Devices.PointOfService API <-> Driver communication.
** This sample will likely work for most cases.
*/
NTSTATUS ProcessGetDeviceBasicsRequest(_In_ WDFREQUEST Request, _Inout_ ULONG_PTR* Information)
{
    if (Information == nullptr || Request == nullptr)
    {
        return STATUS_INVALID_PARAMETER;
    }

    UINT32* runtimeVersion;
    PosDeviceBasicsType* outputData;

    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(UINT32),
        (PVOID*)&runtimeVersion,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (*runtimeVersion < POS_VERSION_1_2)
    {
        // This runtime was for earlier versions of windows and doesn't support software trigger
    }

    status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(PosDeviceBasicsType),
        (PVOID*)&outputData,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Tell the runtime what version of the POS IOCTL interface this driver supports so that
    // it won't send IOCTLs that the driver doesn't support.
    outputData->Version = POS_DRIVER_VERSION;
    // This is a barcode scanner driver
    outputData->DeviceType = PosDeviceType::PosDeviceType_BarcodeScanner;
    // This value will be used to set the initial ReadFile buffer size.  A small size that is
    // likely to cover most of the data events is suggested.  The runtime will grow the ReadFile
    // buffer size as needed.
    outputData->RecommendedBufferSize = 128;

    *Information = sizeof(PosDeviceBasicsType);

    return STATUS_SUCCESS;
}
