#include <pch.h>

NTSTATUS ProcessRetrieveDeviceAuthentication(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request, _Inout_ ULONG_PTR* Information);
NTSTATUS ProcessAuthenticateDevice(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request);
NTSTATUS ProcessDeauthenticateDevice(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request);
NTSTATUS ProcessUpdateKey(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request);
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
** important to the POS magnetic stripe reader model.
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


    case IOCTL_POINT_OF_SERVICE_MSR_RETRIEVE_DEVICE_AUTHENTICATION:
        status = ProcessRetrieveDeviceAuthentication(device, fileObject, Request, &information);
        break;

    case IOCTL_POINT_OF_SERVICE_MSR_AUTHENTICATE_DEVICE:
        status = ProcessAuthenticateDevice(device, fileObject, Request);
        break;

    case IOCTL_POINT_OF_SERVICE_MSR_DEAUTHENTICATE_DEVICE:
        status = ProcessDeauthenticateDevice(device, fileObject, Request);
        break;

    case IOCTL_POINT_OF_SERVICE_MSR_UPDATE_KEY:
        status = ProcessUpdateKey(device, fileObject, Request);
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
** Implement this function to handle requests for the device authentication information.
** This should be step 1 in authenticating or deauthenticating the device.
*/
NTSTATUS ProcessRetrieveDeviceAuthentication(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request, _Inout_ ULONG_PTR* Information)
{
    // If the caller is not the device owner, fail the request
    if (!PosCxIsDeviceOwner(Device, FileObject))
    {
        return STATUS_ACCESS_DENIED;
    }

    *Information = (ULONG_PTR)sizeof(MSR_RETRIEVE_DEVICE_AUTHENTICATION_DATA);

    PMSR_RETRIEVE_DEVICE_AUTHENTICATION_DATA reqBufferPtr;
    size_t reqBufferSize;
    NTSTATUS status = WdfRequestRetrieveOutputBuffer(
        Request,
        sizeof(MSR_RETRIEVE_DEVICE_AUTHENTICATION_DATA),
        reinterpret_cast<PVOID*>(&reqBufferPtr),
        &reqBufferSize
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // TODO: Fill in the reqBufferPtr with the authentication information for your device.

    return status;
}

/*
** Driver TODO: Add code to handle various get-property cases.
**
** Implement this function to handle requests to authenticate your device.
** This is step 2 in authenticating the device.
*/
NTSTATUS ProcessAuthenticateDevice(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request)
{
    // If the caller is not the device owner, fail the request
    if (!PosCxIsDeviceOwner(Device, FileObject))
    {
        return STATUS_ACCESS_DENIED;
    }

    PMSR_AUTHENTICATE_DEVICE reqBufferPtr;
    size_t reqBufferSize;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(MSR_AUTHENTICATE_DEVICE),
        reinterpret_cast<PVOID*>(&reqBufferPtr),
        &reqBufferSize
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // TODO: Send the authentication information to the device

    return status;
}

/*
** Driver TODO: Add code to handle various get-property cases.
**
** Implement this function to handle requests to deauthenticate your device.
** This is step 2 in deauthenticating the device.
*/
NTSTATUS ProcessDeauthenticateDevice(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request)
{
    // If the caller is not the device owner, fail the request
    if (!PosCxIsDeviceOwner(Device, FileObject))
    {
        return STATUS_ACCESS_DENIED;
    }

    PMSR_DEAUTHENTICATE_DEVICE reqBufferPtr;
    size_t reqBufferSize;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(MSR_DEAUTHENTICATE_DEVICE),
        reinterpret_cast<PVOID*>(&reqBufferPtr),
        &reqBufferSize
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // TODO: Send the deauthentication information to the device

    return status;
}

/*
** Driver TODO: Add code to handle various get-property cases.
**
** Implement this function to handle requests to update the key in your device.
*/
NTSTATUS ProcessUpdateKey(_In_ WDFDEVICE Device, _In_ WDFFILEOBJECT FileObject, _In_ WDFREQUEST Request)
{
    // If the caller is not the device owner, fail the request
    if (!PosCxIsDeviceOwner(Device, FileObject))
    {
        return STATUS_ACCESS_DENIED;
    }

    PMSR_UPDATE_KEY reqBufferPtr;
    size_t reqBufferSize;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(
        Request,
        sizeof(MSR_UPDATE_KEY),
        reinterpret_cast<PVOID*>(&reqBufferPtr),
        &reqBufferSize
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // TODO: Send the updated key information to the device

    return status;
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

    case PosPropertyId::MagneticStripeReaderIsDecodeDataEnabled:
        // BOOL result, true when the app has called SetProperty(MagneticStripeReaderIsDecodeDataEnabled) = TRUE
        {
            BOOL isDecodeDataEnabled = TRUE; // Get this value from device context or by querying the device
            *((BOOL*)outputBuffer) = isDecodeDataEnabled;
            *Information = sizeof(BOOL);
        }
        break;

    case PosPropertyId::MagneticStripeReaderCapabilities:
        {
            // PosMagneticStripeReaderCapabilitiesType result
            // These capabilities are likely hard-coded for the specific device.  In this case, example values are provided and should
            // be replaced with values that match your hardware
            PosMagneticStripeReaderCapabilitiesType capabilities;
            capabilities.PowerReportingType = DriverUnifiedPosPowerReportingType::Standard;
            capabilities.IsStatisticsReportingSupported = TRUE;
            capabilities.IsStatisticsUpdatingSupported = TRUE;
            capabilities.CardAuthenticationLength = 0;
            capabilities.SupportedEncryptionAlgorithms = MsrDataEncryption::MsrDataEncryption_AES;
            capabilities.AuthenticationLevel = DriverMagneticStripeReaderAuthenticationLevel::Optional;
            capabilities.IsIsoSupported = TRUE;
            capabilities.IsJisOneSupported = TRUE;
            capabilities.IsJisTwoSupported = TRUE;
            capabilities.IsTrackDataMaskingSupported = TRUE;
            capabilities.IsTransmitSentinelsSupported = TRUE;
            size_t bytesToCopy = sizeof(PosMagneticStripeReaderCapabilitiesType);
            if (outputBufferLength < bytesToCopy)
            {
                bytesToCopy = outputBufferLength;
                status = STATUS_BUFFER_OVERFLOW;
            }
            memcpy(outputBuffer, &capabilities, bytesToCopy);
            *Information = bytesToCopy;
        }
        break;

    case PosPropertyId::MagneticStripeReaderSupportedCardTypes:
        {
            // Supported card types.  The API understands Bank and AAMVA, but additional, device-specific values can be added as well
            // This property is typically hard coded based on the device type
            MSR_SUPPORTED_CARD_TYPES supportedCardTypes;
            RtlZeroMemory(&supportedCardTypes, sizeof(MSR_SUPPORTED_CARD_TYPES));
            supportedCardTypes.Count = 2;
            supportedCardTypes.CardTypes[0] = (unsigned int)MsrCardType::MsrCardType_Bank;
            supportedCardTypes.CardTypes[1] = (unsigned int)MsrCardType::MsrCardType_Aamva;
            size_t bytesToCopy = sizeof(MSR_SUPPORTED_CARD_TYPES);
            if (outputBufferLength < bytesToCopy)
            {
                bytesToCopy = outputBufferLength;
                status = STATUS_BUFFER_OVERFLOW;
            }
            memcpy(outputBuffer, &supportedCardTypes, bytesToCopy);
            *Information = bytesToCopy;
        }
        break;

    case PosPropertyId::MagneticStripeReaderDeviceAuthenticationProtocol:
        {
            // Returns whether the device supports challenge/response authentication or not.
            *((MsrAuthenticationProtocolType*)outputBuffer) = MsrAuthenticationProtocolType::MsrAuthenticationProtocolType_ChallengeResponse;
            *Information = sizeof(MsrAuthenticationProtocolType);
        }
        break;

    case PosPropertyId::MagneticStripeReaderErrorReportingType:
        {
            // Returns whether the device should report errors at the card or track level.
            // This value is typically retrieved from the device context based on a prior SetProperty(MagneticStripeReaderErrorReportingType)
            *((MsrErrorReportingType*)outputBuffer) = MsrErrorReportingType::MsrErrorReportingType_CardLevel;
            *Information = sizeof(MsrErrorReportingType);
        }
        break;

    case PosPropertyId::MagneticStripeReaderTracksToRead:
        {
            // typically this value is saved in the device context to track which tracks the application wants to read from the card
            MsrTrackIds tracksToRead = (MsrTrackIds)(MsrTrackIds::MsrTrackIds_Track1 | MsrTrackIds::MsrTrackIds_Track2);
            *((MsrTrackIds*)outputBuffer) = tracksToRead;
            *Information = sizeof(MsrTrackIds);
        }
        break;

    case PosPropertyId::MagneticStripeReaderIsTransmitSentinelsEnabled:
        {
            // BOOL property.  True if the data that is sent to the application will include start/end sentinals or not
            // This property can return STATUS_NOT_SUPPORTED if the IsTransmitSentinelsSupported capability is FALSE
            // Otherwise, the boolean that's returned should be captured from the device context or from the device itself
            BOOL isTransmitSentinelsEnabled = TRUE;
            *((BOOL*)outputBuffer) = isTransmitSentinelsEnabled;
            *Information = sizeof(BOOL);
        }
        break;

    case PosPropertyId::MagneticStripeReaderIsDeviceAuthenticated:
        {
            // BOOL property.  True if the the authentication process has been successful
            // This value should come from the device context or from the device itself
            BOOL isAuthenticated = TRUE;
            *((BOOL*)outputBuffer) = isAuthenticated;
            *Information = sizeof(BOOL);
        }
        break;

    case PosPropertyId::MagneticStripeReaderDataEncryptionAlgorithm:
        {
            // Returns the encryption algorithm used to decrypt the card data
            // If the device supports multiple algorithms, it should store the current one in the device context and use that value here
            // (or retrieve the value from the device itself)
            MsrDataEncryption currentEncryptionAlgorithm = MsrDataEncryption::MsrDataEncryption_AES;
            *((MsrDataEncryption*)outputBuffer) = currentEncryptionAlgorithm;
        }
        break;

    default:
        // no other readable properties for magnetic stripe reader
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
            // an MSR read occurs, the driver can disable the device.
            BOOL isDisabledOnDataReceived = *((BOOL*)argumentData);
            UNREFERENCED_PARAMETER(isDisabledOnDataReceived);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::MagneticStripeReaderIsDecodeDataEnabled:
        // BOOL value
        if (argumentLength >= sizeof(BOOL))
        {
            // Typically this value will get cached in the device context so that, when
            // an MSR read occurs, the driver can decode the raw data to get the scan data label.
            BOOL isDecodeDataEnabled = *((BOOL*)argumentData);
            UNREFERENCED_PARAMETER(isDecodeDataEnabled);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::MagneticStripeReaderErrorReportingType:
        // MsrErrorReportingType value
        if (argumentLength >= sizeof(MsrErrorReportingType))
        {
            // Typically this value will get cached in the device context so that, when
            // a failed MSR read occurs, the driver can report the error correctly
            MsrErrorReportingType errorReporting = *((MsrErrorReportingType*)argumentData);
            UNREFERENCED_PARAMETER(errorReporting);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::MagneticStripeReaderTracksToRead:
        // MsrTrackIds value
        if (argumentLength >= sizeof(MsrErrorReportingType))
        {
            // This value will either be sent to the device to limit the tracks that are read,
            // or cached in the device context so that the driver can only report the given tracks
            // during an MSR read.
            MsrTrackIds tracksToRead = *((MsrTrackIds*)argumentData);
            UNREFERENCED_PARAMETER(tracksToRead);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::MagneticStripeReaderIsTransmitSentinelsEnabled:
        // BOOL value
        if (argumentLength >= sizeof(BOOL))
        {
            // This value will either be sent to the device to enable or disable sending sentinel data,
            // or cached in the device context so that the driver can insert or remove the sentinel data during an MSR read.
            BOOL isTransmitSentinelsEnabled = *((BOOL*)argumentData);
            UNREFERENCED_PARAMETER(isTransmitSentinelsEnabled);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case PosPropertyId::MagneticStripeReaderDataEncryptionAlgorithm:
        // MsrDataEncryption value
        if (argumentLength >= sizeof(MsrDataEncryption))
        {
            // This property may be rejected if the SupportedEncryptionAlgorithms capability indicates that no encryption is supported.
            // Otherwise it should set the current decryption algorithm (either by sending it to the device or by caching it for use
            // by the driver during an MSR read).
            MsrDataEncryption encryptionAlgorithm = *((MsrDataEncryption*)argumentData);
            UNREFERENCED_PARAMETER(encryptionAlgorithm);
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    default:
        // no other writable properties for magnetic stripe reader
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
    wcscpy_s(StatisticsData.Header.DeviceInformation.DeviceCategory, L"MSR");
    wcscpy_s(StatisticsData.Header.DeviceInformation.FirmwareRevision, L"<eg, 1.1>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.InstallationDate, L"<installation date>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.Interface, L"<eg, USB>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.ManufactureDate, L"<eg, 2015/03/17>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.ManufacturerName, L"<eg, Conteso>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.MechanicalRevision, L"<eg, 2.0a>");
    wcscpy_s(StatisticsData.Header.DeviceInformation.ModelName, L"<eg, MSR Model M>");
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
    // This is a magnetic stripe reader driver
    outputData->DeviceType = PosDeviceType::PosDeviceType_MagneticStripeReader;
    // This value will be used to set the initial ReadFile buffer size.  A small size that is
    // likely to cover most of the data events is suggested.  The runtime will grow the ReadFile
    // buffer size as needed.
    outputData->RecommendedBufferSize = 128;

    *Information = sizeof(PosDeviceBasicsType);

    return STATUS_SUCCESS;
}
