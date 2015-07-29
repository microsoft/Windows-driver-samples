#pragma once

//
// Sensor device structures and definitions
//

// 
// Format of the 9-byte receive packet (device to PC):
// 
// ORIGINAL TEMP_SENSOR PACKET FORMAT: [ VALUE (4 bytes) | INTERVAL (5 bytes) ]
//
// Format of the multi-byte receive packet (device to PC):
//
// NEW PACKET FORMAT:      [ SENSOR_ID (1 byte) | ELEMENT_COUNT (1 byte) | ELEMENT_SIZE (1 byte) | ELEMENTS (size bytes) | INTERVAL (5 bytes) ]
// COMPASS PACKET FORMAT:  [ SENSOR_ID = 1      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 3      | HEADING 3-bytes   | INTERVAL (5 bytes) ]
// SENSIRON PACKET FORMAT: [ SENSOR_ID = 2      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 7      | TEMP 4-bytes | HUMIDITY 3-bytes    | INTERVAL (5 bytes) ]
// FLEX PACKET FORMAT:     [ SENSOR_ID = 3      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 5      | PRESSURE 5-bytes   | INTERVAL (5 bytes) ]
// PING PACKET FORMAT:     [ SENSOR_ID = 4      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 5      | PRESSURE 5-bytes   | INTERVAL (5 bytes) ]
// PIR PACKET FORMAT:      [ SENSOR_ID = 5      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 1      | STATE 1-byte      | INTERVAL (5 bytes) ]
// MEMSIC PACKET FORMAT:   [ SENSOR_ID = 6      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 6      | X-Axis Gs 3-bytes |  Y-Axis Gs 3-bytes | INTERVAL (5 bytes) ]
// QTI PACKET FORMAT:      [ SENSOR_ID = 7      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 4      | PRESSURE 4-bytes   | INTERVAL (5 bytes) ]
// PIEZO PACKET FORMAT:    [ SENSOR_ID = 8      | ELEMENT_COUNT = 1      | ELEMENT_SIZE = 1      | STATE 1-byte        | INTERVAL (5 bytes)
// HITACHI PACKET FORMAT:  [ SENSOR_ID = 9      | ELEMENT_COUNT = 3      | ELEMENT_SIZE = 4      | X-Axis Gs 4-bytes |  Y-Axis Gs 4-bytes | Z-Axis Gs 4-bytes | INTERVAL (5 bytes) ]
//
//
// Format of the 6-byte send packet (PC to device):
// The final NULL byte signals the SERIN DEC formatter to stop reading
//
// [ INTERVAL (5 bytes) | 0 (1 byte) ]
//
#define INTERVAL_DATA_LENGTH        6   // count of bytes for interval
#define MIN_DATA_LENGTH             4   // minimum count of bytes for any sensor
#define MAX_DATA_LENGTH             21  // maximum count of bytes for any sensor
#define MAX_AMOUNT_TO_READ          (MAX_DATA_LENGTH+INTERVAL_DATA_LENGTH)  // maximum total bytes to read

#define MEMSIC_DATA_LENGTH          9  // count of data bytes for Memsic dual-axis accelerometer
#define HITACHI_DATA_LENGTH         15  // count of data bytes for Hitachi tri-axis accelerometer
#define COMPASS_DATA_LENGTH         6   // count of data bytes for Compass
#define SENSIRON_DATA_LENGTH        10  // count of data bytes for Sensiron temp/humidity sensor
#define PING_DATA_LENGTH            8   // count of data bytes for Ping distance sensor
#define FLEX_DATA_LENGTH            8   // count of data bytes for Flexiforce pressure sensor
#define PIR_DATA_LENGTH             4   // count of data bytes for PIR
#define QTI_DATA_LENGTH             7   // count of data bytes for QTI
#define PIEZO_DATA_LENGTH           4   // count of data bytes for Piezo

#define MEMSIC_AMOUNT_TO_READ       (MEMSIC_DATA_LENGTH+INTERVAL_DATA_LENGTH)   // total Memsic byte count
#define HITACHI_AMOUNT_TO_READ      (HITACHI_DATA_LENGTH+INTERVAL_DATA_LENGTH)  // total Hitachi byte count
#define COMPASS_AMOUNT_TO_READ      (COMPASS_DATA_LENGTH+INTERVAL_DATA_LENGTH)  // total Compass byte count
#define SENSIRON_AMOUNT_TO_READ     (SENSIRON_DATA_LENGTH+INTERVAL_DATA_LENGTH) // total Sensiron byte count
#define PING_AMOUNT_TO_READ         (PING_DATA_LENGTH+INTERVAL_DATA_LENGTH)     // total Ping byte count
#define FLEX_AMOUNT_TO_READ         (FLEX_DATA_LENGTH+INTERVAL_DATA_LENGTH)     // total Flexiforce byte count
#define PIR_AMOUNT_TO_READ          (PIR_DATA_LENGTH+INTERVAL_DATA_LENGTH)      // total PIR byte count
#define QTI_AMOUNT_TO_READ          (QTI_DATA_LENGTH+INTERVAL_DATA_LENGTH)      // total QTI byte count
#define PIEZO_AMOUNT_TO_READ        (PIEZO_DATA_LENGTH+INTERVAL_DATA_LENGTH)    // total Piezo byte count

#define DEVICE_ID                   0   // byte 0 contains the Device ID
#define ELEMENT_SIZE                1   // byte 1 contains the Element Size
#define ELEMENT_COUNT               2   // byte 2 contains the Element Count

// Update interval range in milliseconds
#define SENSOR_UPDATE_INTERVAL_MIN  10      // milliseconds
#define SENSOR_UPDATE_INTERVAL_MAX  60000   // milliseconds
#define SENSOR_UPDATE_INTERVAL_STEP 1       // milliseconds

// TODO: Change these range values to match your sensor hardware
#define SENSOR_READING_MIN          1   // ?Units
#define SENSOR_READING_MAX          378 // ?Units
#define SENSOR_READING_STEP         1   // ?Units

// TODO: Change this to match the default COM port to use
#define COM_PORT_NAME               L"COM1"
#define NUM_READSTAT_HANDLES        4
#define MAX_STATUS_LENGTH           100

// Ascii definitions
#define ASCII_BEL                   0x07
#define ASCII_BS                    0x08
#define ASCII_LF                    0x0A
#define ASCII_CR                    0x0D
#define ASCII_XON                   0x11
#define ASCII_XOFF                  0x13

// Miscellaneous definitions
#define MAX_READ_BUFFER             2048
#define MAX_WRITE_BUFFER            1024
#define EVENTFLAGS_DEFAULT          EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD
#define FLAGCHAR_DEFAULT            '\n'

// Read states
#define RECEIVE_TTY                 0x01
#define RECEIVE_CAPTURED            0x02

class RS232Connection
{

public:
    RS232Connection();

    ~RS232Connection()
    {
        Disconnect();
    }

    HRESULT Connect(_In_ LPCWSTR wszPortName, _Out_ HANDLE *phCommPort);
    void Disconnect();

private:
    HRESULT SetPortState();

private:

    //
    // Required for the RS232 initialization and communication.
    //

    // TTY member variables and defines
    HANDLE       m_hCommPort;
    DWORD        m_dwEventFlags;
    CHAR         m_chFlag;
    CHAR         m_chXON;
    CHAR         m_chXOFF;
    WORD         m_wXONLimit;
    WORD         m_wXOFFLimit;
    DWORD        m_fRtsControl;
    DWORD        m_fDtrControl;
    BOOL         m_fConnected; 
    BOOL         m_fTransferring; 
    BOOL         m_fRepeating;
    BOOL         m_fLocalEcho;
    BOOL         m_fNewLine;
    BOOL         m_fDisplayErrors; 
    BOOL         m_fAutowrap;
    BOOL         m_fCTSOutFlow;
    BOOL         m_fDSROutFlow;
    BOOL         m_fDSRInFlow;
    BOOL         m_fXonXoffOutFlow;
    BOOL         m_fXonXoffInFlow;
    BOOL         m_fTXafterXoffSent;
    BOOL         m_fNoReading;
    BOOL         m_fNoWriting;
    BOOL         m_fNoEvents;
    BOOL         m_fNoStatus;
    BOOL         m_fDisplayTimeouts;
    BYTE         m_bPort;
    BYTE         m_bByteSize;
    BYTE         m_bParity;
    BYTE         m_bStopBits;
    DWORD        m_dwBaudRate;
    COMMTIMEOUTS m_timeoutsorig;
    COMMTIMEOUTS m_timeoutsnew;
    COMMTIMEOUTS m_TimeoutsDefault;
    DWORD        m_dwReceiveState;
};