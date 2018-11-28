/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    NmeaProcessor.cpp

Abstract:

    This file contains the NMEA parser.

Environment:

    Windows User-Mode Driver Framework

Revision History:

--*/

#include "precomp.h"
#include "Trace.h"
#include "Defaults.h"
#include "HelperUtility.h"
#include "NmeaProcessor.h"
#include "FixSession.h"
#include "FixHandler.h"

#include <cfgmgr32.h>

#include "NmeaProcessor.tmh"


CNmeaProcessor::CNmeaProcessor() :
    _HandleComm(INVALID_HANDLE_VALUE)
{
    InitializeCriticalSection(&_Lock);

    memset(_NmeaMessageBuf, 0, sizeof(_NmeaMessageBuf));
}

CNmeaProcessor::~CNmeaProcessor()
{
    CloseHandler(_HandleComm);

    DeleteCriticalSection(&_Lock);
}

bool
CNmeaProcessor::SetCommHandleForSerial()
{
    std::wstring portNumCandidate;
    bool foundPort = false;
    HRESULT hr = S_OK;
    DWORD bytesWritten = 0;
    ULONG interfaceListSize = 0;

    // Get the size of the buffer necessary for the list of volumes
    CONFIGRET cr = CM_Get_Device_Interface_List_Size(&interfaceListSize,
                                                     (LPGUID)&GUID_DEVINTERFACE_COMPORT,
                                                     nullptr,
                                                     CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    // size 1 is for null terminator
    if (cr != CR_SUCCESS || interfaceListSize <= 1) 
    {
        // No existing device interface has been created.
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_NMEA, "CM_Get_Device_Interface_List_Size failed with %x", cr);
        return false;
    }

    // Found at least one existing device interface.
    std::unique_ptr<WCHAR[]> interfaceList(new WCHAR[interfaceListSize]);
    cr = CM_Get_Device_Interface_List((LPGUID)&GUID_DEVINTERFACE_COMPORT,
                                      nullptr,
                                      interfaceList.get(),
                                      interfaceListSize,
                                      CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS)
    {
        // No existing device interface has been created.
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_NMEA, "CM_Get_Device_Interface_List failed with %x", cr);
        return false;
    }

    // Loop through all devices with this COMPORT interface. This is a double-null terminated string.
    for (PWSTR tempInterfacePtr = interfaceList.get(); *tempInterfacePtr; tempInterfacePtr += wcslen(tempInterfacePtr) + 1)
    {
        hr = CreateInterfaceForSerial(tempInterfacePtr, &_HandleComm);

        // If we can create the interface, determine if a NMEA messeage comes out of the port
        if (SUCCEEDED(hr))
        {
            // Make sure _NmeaMessageBuf ends with null-terminated character by not filling the last index.
            if (!ReadFile(_HandleComm, _NmeaMessageBuf, sizeof(_NmeaMessageBuf) - 1, &bytesWritten, nullptr))
            {
                CloseHandler(_HandleComm);
                continue;
            }
            memset(&_NmeaMessageBuf[bytesWritten], 0, sizeof(_NmeaMessageBuf) - bytesWritten);

            // Validate that the output is a NMEA format
            if (!IsNMEASentence(_NmeaMessageBuf))
            {
                CloseHandler(_HandleComm);
                continue;
            }

            // A valid COM port is found. Stop the processing
            foundPort = true;
            break;
        }
    }

    return foundPort;
}

HRESULT
CNmeaProcessor::CreateInterfaceForSerial(
    _In_ LPCWSTR SerialPortName,
    _Out_ HANDLE* HandleComm
)
{
    HRESULT hr = S_OK;
    DCB dcbSerialParams = {};
    COMMTIMEOUTS timeouts = {};
    HANDLE handleComm = INVALID_HANDLE_VALUE;

    // Create Serial interface
    handleComm = CreateFileW(SerialPortName,
                             GENERIC_READ,
                             0,
                             0,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0);

    if (handleComm == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Get current DCB
    if (!GetCommState(handleComm, &dcbSerialParams))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Set new serial parameters
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_4800;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(handleComm, &dcbSerialParams))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Initialize timeout parameters in milliseconds
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 50;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(handleComm, &timeouts))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    *HandleComm = handleComm;

Exit:
    if (FAILED(hr))
    {
        CloseHandler(handleComm);
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_NMEA, "CreateInterfaceForSerial failed at %ws with hr: %x", SerialPortName, hr);
    }
    
    return hr;
}

HRESULT
CNmeaProcessor::ReadNmeaSentenceFromSerial()
{
    EnterCriticalSection(&_Lock);
    
    HRESULT hr = S_OK;
    DWORD bytesWritten;

    // If the communication handle is not set, try to create it
    if (_HandleComm == INVALID_HANDLE_VALUE || _HandleComm == nullptr)
    {
        if (!SetCommHandleForSerial())
        {
            hr = E_FAIL;
            goto Exit;
        }
    }

    // Make sure _NmeaMessageBuf ends with null-terminated character by not filling the last index.
    if (!ReadFile(_HandleComm, _NmeaMessageBuf, sizeof(_NmeaMessageBuf) - 1, &bytesWritten, nullptr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandler(_HandleComm);
        goto Exit;
    }
    memset(&_NmeaMessageBuf[bytesWritten], 0, sizeof(_NmeaMessageBuf) - bytesWritten);

Exit:
    LeaveCriticalSection(&_Lock);

    if (FAILED(hr))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_NMEA, "ReadNmeaSentenceFromSerial failed with hr: %x", hr);
    }
    
    return hr;
}

NTSTATUS
CNmeaProcessor::ReadNextNmeaSentence(
    _Out_ PGNSS_FIXDATA GnssFixData
)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Initialize the output. If reading or parsing fails, the output cannot be used as valid data
    GnssFixData->FixLevelOfDetails = 0;
    GnssFixData->IsFinalFix = 0;

    // Read NMEA messages
    //
    // TODO: You can choose target HW interface among available options such as Serial and Bluetooth.
    //       ReadNmeaSentenceFromSerial provides an example of retrieving the incoming data from Serial interface.
    //
    status = ReadNmeaSentenceFromSerial();
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_NMEA, "ReadNmeaSentenceFromSerial failed with %!STATUS!", status);
        return status;
    }

    // Parse the NMEA messages
    status = ParseNmeaSentence(GnssFixData);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_NMEA, "ParseNmeaSentence failed with %!STATUS!", status);
        return status;
    }

    return status;
}

bool
CNmeaProcessor::IsNMEASentence(
    _In_ PCSTR NmeaBuf
)
{
    std::size_t sentenceStart = 0;
    std::string nmeaStr(NmeaBuf);

    // Fast-forward to the first sentence by identifying the first start character '$'
    if (!FindFirstStartOfSequence(nmeaStr, &sentenceStart))
    {
        return false;
    }

    // If the checksum is correct in the sequence, we confrim that this is a NMEA sentence
    return IsValidChecksum(nmeaStr.substr(sentenceStart));
}

bool
CNmeaProcessor::FindFirstStartOfSequence(
    _In_ const std::string& NmeaMessageStr,
    _Out_ std::size_t* StartIndex
)
{
    std::size_t found;

    // Find start of sequence
    found = NmeaMessageStr.find_first_of('$');
    if (found == std::string::npos)
    {
        return false;
    }

    if (StartIndex != nullptr)
    {
        *StartIndex = found;
    }

    return true;
}

bool
CNmeaProcessor::FindFirstChecksumDelimeter(
    _In_ const std::string& NmeaMessageStr,
    _Out_ std::size_t* StartIndex
)
{
    std::size_t found;

    // Find checksum delimiter, which is followed by the checksum
    found = NmeaMessageStr.find_first_of('*');
    if (found == std::string::npos)
    {
        return false;
    }

    if (StartIndex != nullptr)
    {
        *StartIndex = found;
    }
    
    return true;
}

bool
CNmeaProcessor::FindFirstEndingOfSequence(
    _In_ const std::string& NmeaMessageStr,
    _Out_ std::size_t* StartIndex
)
{
    std::size_t foundCR;
    std::size_t foundLF;

    //
    // Find consecutive CR + LF, and set LF as a ending index of sequence in the sentence
    //

    foundCR = NmeaMessageStr.find_first_of('\r');
    if (foundCR == std::string::npos)
    {
        return false;
    }

    foundLF = NmeaMessageStr.substr(foundCR).find_first_of('\n');
    if (foundLF != 1)
    {
        return false;
    }

    if (StartIndex != nullptr)
    {
        *StartIndex = foundCR + 1;
    }

    return true;
}

bool
CNmeaProcessor::IsValidChecksum(
    _In_ std::string NmeaMessageStr
)
{
    BYTE calculatedChecksum = 0;
    std::size_t startSequence;
    std::size_t checksumDelimeter;

    // Validate that there is start of sequence
    if (!FindFirstStartOfSequence(NmeaMessageStr, &startSequence))
    {
        return false;
    }

    // Validate that there is checksum delimiter
    if (!FindFirstChecksumDelimeter(NmeaMessageStr, &checksumDelimeter))
    {
        return false;
    }

    // Validate that there is end of sequence
    if (!FindFirstEndingOfSequence(NmeaMessageStr, nullptr))
    {
        return false;
    }

    // Calculate checksum on the fly to compare with the value in checksum field
    for (std::size_t i = startSequence + 1; i < checksumDelimeter; i++)
    {
        calculatedChecksum = calculatedChecksum ^ NmeaMessageStr[i];
    }

    // Validate the checksum field, which consists of two characters
    BYTE valueInChecksumField = static_cast<BYTE>(stoul(NmeaMessageStr.substr(checksumDelimeter + 1, 2), nullptr, 16));
    if (calculatedChecksum != valueInChecksumField)
    {
        return false;
    }

    return true;
}

NTSTATUS
CNmeaProcessor::ParseNmeaSentence(
    _Out_ PGNSS_FIXDATA GnssFixData
)
{
    NTSTATUS status = STATUS_SUCCESS;
    std::string nmeaMessageStr(_NmeaMessageBuf);
    std::size_t sentenceStart = 0;
    std::size_t checksumDelimeter = 0;
    std::size_t sentenceEnd = 0;

    while (sentenceEnd != std::string::npos)
    {
        // Fast-forward to the first sentence by identifying the first start character '$'
        if (!FindFirstStartOfSequence(nmeaMessageStr, &sentenceStart))
        {
            break;
        }
        nmeaMessageStr = nmeaMessageStr.substr(sentenceStart);

        // Validate the checksum
        if (!IsValidChecksum(nmeaMessageStr))
        {
            // We discard the buffer if current sentence is not valid
            // It would be safer to start over and read a new buffer
            break;
        }

        // Find where checksum delimeter is located. This points out the end of current sentence body
        if (!FindFirstChecksumDelimeter(nmeaMessageStr, &checksumDelimeter))
        {
            break;
        }
        
        // Parse sentence body
        ParseSentenceWrapper(nmeaMessageStr, checksumDelimeter, GnssFixData);

        // Go to the end of the current sentence
        if (!FindFirstEndingOfSequence(nmeaMessageStr, &sentenceEnd))
        {
            break;
        }
        nmeaMessageStr = nmeaMessageStr.substr(sentenceEnd);
    }

    return status;
}

void
CNmeaProcessor::ParseSentenceWrapper(
    _In_ std::string NmeaMessageStr,
    _In_ std::size_t SentenceBodySize,
    _Out_ PGNSS_FIXDATA GnssFixData
)
{
    std::string addrField;
    
    // Parse sentence type from address field and select a correct body parser for the fields
    addrField = NmeaMessageStr.substr(1, 5);
    if (addrField == "GPGGA")
    {
        ParseGPGGASentence(NmeaMessageStr.substr(0, SentenceBodySize), GnssFixData);
    }
    else if (addrField == "GPRMC")
    {
        ParseGPRMCSentence(NmeaMessageStr.substr(0, SentenceBodySize), GnssFixData);
    }
    else
    {
        //
        // TODO: Add more sentence parsers as needed
        // For example, GPGSA and GPGSV are used for satellite information in detail (GNSS_SATELLITEINFO)
        //
    }
}

void
CNmeaProcessor::ParseGPGGASentence(
    _In_ const std::string& NmaSentence,
    _Out_ PGNSS_FIXDATA GnssFix
)
{    
    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    double degree = 0;
    double mins = 0;
    double horizontalDilution = 0;
    ULONG qualityIndicator = 0;
    ULONG satelliteCount = 0;
    NTSTATUS status = STATUS_SUCCESS;
    std::vector<std::string> nmeaFieldList;
    SYSTEMTIME systemTime = {};
    FILETIME fileTime = {};
    const std::size_t numOfGPGGAFields = 14;

    try
    {
        ConstructNmeaFieldList(&nmeaFieldList, NmaSentence);

        // Ensure GPGGA sentence contains the correct number of fields to avoid the out-of-range access
        if (nmeaFieldList.size() < numOfGPGGAFields)
        {
            throw std::out_of_range("GPGGA does not contain all the required number of fields");
        }

        // Retrieve UTC Time
        GetSystemTime(&systemTime); //Initialize the system time first. GGA does not set all the fields
        const std::string& time = nmeaFieldList[1];
        systemTime.wHour = static_cast<WORD>(std::stoi(time.substr(0, 2)));
        systemTime.wMinute = static_cast<WORD>(std::stoi(time.substr(2, 2)));
        systemTime.wSecond = static_cast<WORD>(std::stoi(time.substr(4, 2)));
        systemTime.wMilliseconds = static_cast<WORD>(std::stoi(time.substr(7)));

        // Calculate latitude
        const std::string& latitudeInfo = nmeaFieldList[2];
        degree = std::stod(latitudeInfo.substr(0, 2));
        mins = std::stod(latitudeInfo.substr(2, 7));
        latitude = degree + (mins / 60.0);
        if (nmeaFieldList[3].compare("S") == 0)
        {
            latitude = latitude * -1;
        }

        // Calculate longitude
        const std::string& longitudeInfo = nmeaFieldList[4];
        degree = stod(longitudeInfo.substr(0, 3));
        mins = stod(longitudeInfo.substr(3, 7));
        longitude = degree + (mins / 60.0);
        if (nmeaFieldList[5].compare("W") == 0)
        {
            longitude = longitude * -1;
        }

        // Retrieve GPS quality indicator
        qualityIndicator = stoi(nmeaFieldList[6]);

        // Retrieve Satellites used
        satelliteCount = stoi(nmeaFieldList[7]);

        // Retrieve horizontal dilution of precision
        horizontalDilution = stod(nmeaFieldList[8]);

        // Retrieve altitude
        altitude = stod(nmeaFieldList[9]);
    }
    catch (const std::exception& e)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR,
                    TRACE_NMEA,
                    "Caught exception while executing ParseGPGGASentence, error '%s' encountered while parsing '%s'", e.what(), NmaSentence.c_str());
    }

    // Assign the values to output structure
    SystemTimeToFileTime(&systemTime, &(GnssFix->FixTimeStamp));
    GnssFix->BasicData.Latitude = latitude;
    GnssFix->BasicData.Longitude = longitude;
    GnssFix->BasicData.Altitude = altitude;
    GnssFix->IsFinalFix = qualityIndicator == 0 ? FALSE : TRUE;
    GnssFix->AccuracyData.HorizontalAccuracy = static_cast<ULONG>(horizontalDilution + 0.5); //HorizontalAccuracy is defined as LONG
    GnssFix->SatelliteData.SatelliteCount = satelliteCount;
    GnssFix->FixLevelOfDetails |= (GNSS_FIXDETAIL_BASIC | GNSS_FIXDETAIL_ACCURACY);
    GnssFix->FixStatus = status;
}

void
CNmeaProcessor::ParseGPRMCSentence(
    _In_ const std::string& NmaSentence,
    _Out_ PGNSS_FIXDATA GnssFix
)
{
    double latitude = 0;
    double longitude = 0;
    double degree = 0;
    double mins = 0;
    double speedKnots = 0;
    double course = 0;
    NTSTATUS status = STATUS_SUCCESS;
    std::vector<std::string> nmeaFieldList;
    SYSTEMTIME systemTime = {};
    FILETIME fileTime = {};
    const std::size_t numOfGPRMCFields = 13;

    try
    {
        ConstructNmeaFieldList(&nmeaFieldList, NmaSentence);
    
        // Ensure GPRMC sentence contains the correct number of fields to avoid the out-of-range access
        if (nmeaFieldList.size() < numOfGPRMCFields)
        {
            throw std::out_of_range("GPRMC does not contain all the required number of fields");
        }

        // Retrieve UTC Time
        GetSystemTime(&systemTime); //Initialize the system time first. RMC does not set all the fields
        const std::string& time = nmeaFieldList[1];
        systemTime.wHour = static_cast<WORD>(std::stoi(time.substr(0, 2)));
        systemTime.wMinute = static_cast<WORD>(std::stoi(time.substr(2, 2)));
        systemTime.wSecond = static_cast<WORD>(std::stoi(time.substr(4, 2)));
        systemTime.wMilliseconds = static_cast<WORD>(std::stoi(time.substr(7)));

        // Calculate latitude
        const std::string& latitudeInfo = nmeaFieldList[3];
        degree = std::stod(latitudeInfo.substr(0, 2));
        mins = std::stod(latitudeInfo.substr(2, 7));
        latitude = degree + (mins / 60.0);
        if (nmeaFieldList[4].compare("S") == 0)
        {
            latitude = latitude * -1;
        }

        // Calculate longitude
        const std::string& longitudeInfo = nmeaFieldList[5];
        degree = stod(longitudeInfo.substr(0, 3));
        mins = stod(longitudeInfo.substr(3, 7));
        longitude = degree + (mins / 60.0);
        if (nmeaFieldList[6].compare("W") == 0)
        {
            longitude = longitude * -1;
        }

        // Retrieve speed
        speedKnots = stod(nmeaFieldList[7]);

        // Retrieve course
        course = stod(nmeaFieldList[8]);

        // Retrieve UTC Date
        const std::string& utcDate = nmeaFieldList[9];
        systemTime.wDay = static_cast<WORD>(std::stoi(utcDate.substr(0, 2)));
        systemTime.wDay = static_cast<WORD>(std::stoi(utcDate.substr(0, 2)));
        systemTime.wMonth = static_cast<WORD>(std::stoi(utcDate.substr(2, 2)));
        systemTime.wYear = static_cast<WORD>(std::stoi(utcDate.substr(4, 2)));
    }
    catch (const std::exception& e)
    {
        status = STATUS_UNSUCCESSFUL;
        TraceEvents(TRACE_LEVEL_ERROR,
                    TRACE_NMEA,
                    "Caught exception while executing ParseGPRMCSentence, error '%s' encountered while parsing '%s'", e.what(), NmaSentence.c_str());
    }
    
    // Assign the values to output structure
    SystemTimeToFileTime(&systemTime, &(GnssFix->FixTimeStamp));
    GnssFix->BasicData.Latitude = latitude;
    GnssFix->BasicData.Longitude = longitude;
    GnssFix->BasicData.Speed = ConvertKnotsToMeterPerSecond(speedKnots);
    GnssFix->BasicData.Heading = course;
    GnssFix->FixLevelOfDetails |= GNSS_FIXDETAIL_BASIC;
    GnssFix->FixStatus = status;
}

double
CNmeaProcessor::ConvertKnotsToMeterPerSecond(
    _In_ double Knots
)
{
    return Knots * 0.514444;
}

void
CNmeaProcessor::ConstructNmeaFieldList(
    _Out_ std::vector<std::string>* NmeaFieldList,
    _In_ const std::string NmaSentence
)
{
    std::stringstream nmeaStream(NmaSentence);
    std::string nmeaField;

    // Put each field into the vector, excluding the field delimiter
    while (getline(nmeaStream, nmeaField, ','))
    {
        NmeaFieldList->push_back(nmeaField);
    }
}

void
CNmeaProcessor::CloseHandler(
    _Out_ HANDLE& Handle
)
{
    if (Handle != nullptr)
    {
        CloseHandle(Handle);
        Handle = nullptr;
    }
}
