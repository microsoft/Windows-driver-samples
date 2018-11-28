/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    NmeaProcessor.h

Abstract:

    This module contains NMEA parser class used by other classes within the Umdf Gnss Driver.
    For the NMEA documentation that this parser is based of, refer to https://en.wikipedia.org/wiki/NMEA_0183

Environment:

    Windows User-Mode Driver Framework

--*/

#pragma once

// We enumerate through MAX_PORT and automatically find a right port to be used for NMEA messages
#define MAX_COM_PORT 256

// This buffer size is not defied by specific standard. It can change as needed.
// If it is too large, handle the case where the buffer includes same type of NMEA messages multiple times.
#define NMEA_READ_BUFFER_SIZE_BYTES 256

class CNmeaProcessor
{
public:
    CNmeaProcessor();
    ~CNmeaProcessor();

    NTSTATUS ReadNextNmeaSentence(_Out_ PGNSS_FIXDATA GnssFixData);

private:
    NTSTATUS ParseNmeaSentence(_Out_ PGNSS_FIXDATA GnssFixData);
    void ParseSentenceWrapper(_In_ std::string NmeaMessageStr, _In_ std::size_t SentenceBodySize, _Out_ PGNSS_FIXDATA GnssFixData);
    void ParseGPGGASentence(_In_ const std::string& NmaSentence, _Out_ PGNSS_FIXDATA GnssFixData);
    void ParseGPRMCSentence(_In_ const std::string& NmaSentence, _Out_ PGNSS_FIXDATA GnssFixData);

    void ConstructNmeaFieldList(_Out_ std::vector<std::string>* NmeaFieldList, _In_ const std::string NmaSentence);
    
    static double ConvertKnotsToMeterPerSecond(_In_ double Knots);
    static void CloseHandler(_Out_ HANDLE& Handle);
    static HRESULT CreateInterfaceForSerial(_In_ LPCWSTR SerialPortName, _Out_ HANDLE* HandleComm);

    HRESULT ReadNmeaSentenceFromSerial();

    bool SetCommHandleForSerial();
    bool IsNMEASentence(_In_ PCSTR NmeaBuf);
    bool IsValidChecksum(_In_ std::string NmeaMessageStr);

    bool FindFirstStartOfSequence(_In_ const std::string& NmeaMessageStr, _Out_ std::size_t* StartIndex);
    bool FindFirstChecksumDelimeter(_In_ const std::string& NmeaMessageStr, _Out_ std::size_t* StartIndex);
    bool FindFirstEndingOfSequence(_In_ const std::string& NmeaMessageStr, _Out_ std::size_t* StartIndex);

    CRITICAL_SECTION _Lock;
    HANDLE _HandleComm = INVALID_HANDLE_VALUE;
    CHAR _NmeaMessageBuf[NMEA_READ_BUFFER_SIZE_BYTES];
};
