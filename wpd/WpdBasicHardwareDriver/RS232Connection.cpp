#include "stdafx.h"

#include "RS232Connection.tmh"

/*-----------------------------------------------------------------------------

FUNCTION: RS232Connection() 

PURPOSE:  Constructor. Initializes TTY structure

COMMENTS: This structure is a collection of TTY attributes
          used by all parts of this program

HISTORY:   Date:      Author:     Comment:
            10/27/95  AllenD      Wrote it
            2/14/96   AllenD      Removed npTTYInfo
            12/06/06  DonnMo      Changed return value type
            12/06/06  DonnMo      Replaced macro calls with member-variable settings
            12/06/06  DonnMo      Removed initialization of font-related variables

-----------------------------------------------------------------------------*/
RS232Connection::RS232Connection()
{
    //
    // initialize generial TTY info
    //
    m_hCommPort = NULL;
    m_fConnected = FALSE;
    m_fLocalEcho = FALSE;
    m_bPort = 1;
    m_dwBaudRate = CBR_9600;
    m_bByteSize = 8;
    m_bParity = NOPARITY;
    m_bStopBits = ONESTOPBIT;
    m_fAutowrap = TRUE;
    m_fNewLine = FALSE;
    m_fDisplayErrors = TRUE;

    //
    // timeouts
    //

    //
    // TimeoutsDefault
    // We need ReadIntervalTimeout here to cause the read operations
    // that we do to actually timeout and become overlapped.
    // Specifying 1 here causes ReadFile to return very quickly
    // so that our reader thread will continue execution.
    //

    m_TimeoutsDefault = { 25, 0, 0, 0, 0 };
    m_timeoutsnew = m_TimeoutsDefault;

    //
    // read state and status events
    //
    m_dwReceiveState         = RECEIVE_TTY;
    m_dwEventFlags = EVENTFLAGS_DEFAULT;
    m_chFlag = FLAGCHAR_DEFAULT;

    //
    // Flow Control Settings
    //
    m_fDtrControl = DTR_CONTROL_ENABLE;
    m_fRtsControl = RTS_CONTROL_ENABLE;
    m_chXON = ASCII_XON;
    m_chXOFF = ASCII_XOFF;
    m_wXONLimit = 0;
    m_wXOFFLimit = 0;
    m_fCTSOutFlow = FALSE;
    m_fDSROutFlow = FALSE;
    m_fDSRInFlow = FALSE;
    m_fXonXoffOutFlow = FALSE;
    m_fXonXoffInFlow = FALSE;
    m_fTXafterXoffSent = FALSE;
    m_fNoReading = FALSE;
    m_fNoWriting = FALSE;
    m_fNoEvents = FALSE;
    m_fNoStatus = FALSE;
    m_fDisplayTimeouts = FALSE;

}


/*-----------------------------------------------------------------------------

FUNCTION: SetPortState( void )

PURPOSE: Sets port state based on settings from the user

COMMENTS: Sets up DCB structure and calls SetCommState.
          Sets up new timeouts by calling SetCommTimeouts.

HISTORY: Date:      Author:     Comment:
        1/9/96     AllenD      Wrote it
        12/06/06    DonnMo      Replaced calls to ErrorReporter() with CHECK_HR()

-----------------------------------------------------------------------------*/
HRESULT RS232Connection::SetPortState()
{
    HRESULT hr          = S_OK;
    DCB     dcb         = {0};
    DWORD   dwLastError = 0;
    
    dcb.DCBlength = sizeof(dcb);

    //
    // get current DCB settings
    //
    if (!GetCommState(m_hCommPort, &dcb))
    {  
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "GetCommState() failed within SetPortState().");
        return hr;
    }

    //
    // update DCB rate, byte size, parity, and stop bits size
    //
    dcb.BaudRate = m_dwBaudRate;
    dcb.ByteSize = m_bByteSize;
    dcb.Parity   = m_bParity;
    dcb.StopBits = m_bStopBits;

    //
    // update event flags
    //
    if (m_dwEventFlags & EV_RXFLAG)
    {
        dcb.EvtChar = m_chFlag;      
    }
    else
    {
        dcb.EvtChar = '\0';
    }

    dcb.EofChar = '\n';

    //
    // update flow control settings
    //
    dcb.fDtrControl     = m_fDtrControl;
    dcb.fRtsControl     = m_fRtsControl;

    dcb.fOutxCtsFlow    = m_fCTSOutFlow;
    dcb.fOutxDsrFlow    = m_fDSROutFlow;
    dcb.fDsrSensitivity = m_fDSRInFlow;
    dcb.fOutX           = m_fXonXoffOutFlow;
    dcb.fInX            = m_fXonXoffInFlow;
    dcb.fTXContinueOnXoff = m_fTXafterXoffSent;
    dcb.XonChar         = m_chXON;
    dcb.XoffChar        = m_chXOFF;
    dcb.XonLim          = m_wXONLimit;
    dcb.XoffLim         = m_wXOFFLimit;

    //
    // DCB settings not in the user's control
    //
    dcb.fParity = TRUE;

    //
    // set new state
    //
    if (!SetCommState(m_hCommPort, &dcb))
    {  
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "SetCommState() failed within SetPortState() when setting the new state.");
        return hr;
    }
        

    //
    // set new timeouts
    //
    if (!SetCommTimeouts(m_hCommPort, &m_timeoutsnew))
    {  
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "SetCommTimeouts() failed within SetPortState() when setting the new timeouts.");
        return hr;
    }

    return hr;
}


/*-----------------------------------------------------------------------------

FUNCTION: Connect( void )

PURPOSE: Setup Communication Port with our settings

RETURN: 
    S_OK and handle of com port if successful

-----------------------------------------------------------------------------*/
HRESULT RS232Connection::Connect(_In_ LPCWSTR wszPortName, _Out_ HANDLE *phCommPort)
{
    HRESULT hr          = S_OK;
    DWORD   dwLastError = 0;

    if (phCommPort == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "A NULL phCommPort parameter was received");
        return hr;
    }

    *phCommPort = NULL;

    //
    // retrieve a handle for the com port
    // and configure the port for asynchronous
    // communications.
    //
    m_hCommPort = CreateFileW(wszPortName,
                              GENERIC_READ | GENERIC_WRITE,
                              0, 
                              0, 
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                              0);

    if (m_hCommPort == NULL) 
    {  
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "CreateFileW() failed in RS232Connection::Connect");
        return hr;
    }

    //
    // Save original comm timeouts and set new ones
    //
    if (!GetCommTimeouts( m_hCommPort, &(m_timeoutsorig)))
    {  
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "GetCommTimeouts() failed within RS232Connection::Connect");
        return hr;
    }

    //
    // Set port state
    //
    hr = SetPortState();
    if (FAILED(hr))
    {  
        CHECK_HR(hr, "SetPortState() failed within RS232Connection::Connect");
        return hr;
    }

    //
    // set comm buffer sizes
    //
    if (!SetupComm(m_hCommPort, MAX_READ_BUFFER, MAX_WRITE_BUFFER))
    {
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "SetupComm(SETDTR) failed within RS232Connection::Connect");
        return hr;
    }

    //
    // raise DTR
    //
    if (!EscapeCommFunction(m_hCommPort, SETDTR))
    {  
        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        CHECK_HR(hr, "EscapeCommFunction() failed within RS232Connection::Connect");
        return hr;
    }

    //
    // set overall connect flag
    //
    m_fConnected = TRUE;

    //
    // set the return value
    //
    *phCommPort = m_hCommPort;


    if (SUCCEEDED(hr))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! handle: %p", m_hCommPort);
    }

    return hr;  
}


/*-----------------------------------------------------------------------------

FUNCTION: Disconnect( void )

PURPOSE: Tears down the Communication Port

RETURN:  nothing

-----------------------------------------------------------------------------*/
void RS232Connection::Disconnect()
{
    if (m_hCommPort != NULL)
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FLAG_DEVICE, "%!FUNC! handle: %p", m_hCommPort);

        CloseHandle(m_hCommPort);
        m_hCommPort = NULL;

    }
}