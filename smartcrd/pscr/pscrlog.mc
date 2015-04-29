;// BUILD Version: 0001    // Increment this if a change has global effects

;/*++
;
;Copyright (c) 1997	SCM Microsystems, Inc
;
;Module Name:
;
;    PscrLog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Notes:
;
;    The file pscrlog.mc was reviewed by LCA in June 2011 and per license is
;    acceptable for Microsoft use under Dealpoint ID 178449.
;
;Revision History:
;
;--*/
;
;#ifndef __PSCRLOG_H__
;#define __PSCRLOG_H__
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
			   Smartcard=0x10:FACILITY_SCARD
              )

MessageId=0x0001 Facility=Smartcard Severity=Error SymbolicName=PSCR_NO_DEVICE_FOUND
Language=English
No PSCR Smart Card reader found in the system. 
.
MessageId=0x0002 Facility=Smartcard Severity=Error SymbolicName=PSCR_CANT_INITIALIZE_READER
Language=English
The reader inserted is not working properly.
Please try to change the 'Input/Output Range' and/or 'Interrupt Request' 
settings in Device Manager.
.
MessageId=0x0003 Facility=Smartcard Severity=Error SymbolicName=PSCR_INSUFFICIENT_RESOURCES
Language=English
Insufficient system resources to start device.
.
MessageId=0x0004 Facility=Smartcard Severity=Error SymbolicName=PSCR_ERROR_INTERRUPT
Language=English
Can't connect to the interrupt provided by the system.
Please try to change the 'Interrupt Request' setting in Device Manager.
.
MessageId=0x0005 Facility=Smartcard Severity=Error SymbolicName=PSCR_ERROR_IO_PORT
Language=English
No I/O port specified or port can not be mapped.
Please try to change the 'Input/Output Range' setting in the Device Manager.
.
MessageId=0x0006 Facility=Smartcard Severity=Error SymbolicName=PSCR_ERROR_CLAIM_RESOURCES
Language=English
Resources can not be claimed or an resource conflict exists.
.
MessageId=0x0007 Facility=Io Severity=Error SymbolicName=PSCR_NO_MEMORY
Language=English
The system does not have enough memory.
.
MessageId=0x0007 Facility=Smartcard Severity=Warning SymbolicName=PSCR_WRONG_FIRMWARE
Language=English
Your reader needs firmware version 2.30 or higher to work with this driver. 
.
;#endif // __PSCRLOG_H__
