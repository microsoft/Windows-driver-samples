;///////////////////////////////////////////////////////////////////////////////////////////////////
;///
;/// Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
;///
;///   Module Name:
;///      Framework_WFPSamplerService_Msg.mc
;///
;///   Abstract:
;///      Message text for WFPSampler service (WFPSamplerService.exe)
;///
;///   Author:
;///      Dusty Harper      (DHarper)
;///
;///   Revision History:
;///
;///      [ Month ][Day] [Year] - [Revision]-[ Comments ]
;///      May       01,   2010  -     1.0   -  Creation
;///
;///   Notes:
;///      A .mc file is compiled by the MC tool to generate a .h file and
;///      a .rc (resource compiler script) file.
;///
;///      Comments in .mc files start with a ";".
;///      Comment lines are generated directly in the .h file, without the leading ";"
;///
;///      See mc.hlp for more help on .mc files and the MC tool.
;///
;///////////////////////////////////////////////////////////////////////////////////////////////////

;#ifndef  FRAMEWORK_WFP_SAMPLER_SERVICE_MESSAGES
;#define FRAMEWORK_WFP_SAMPLER_SERVICE_MESSAGES

;/// HEADER SECTION

SeverityNames = (Success       = 0x0:STATUS_SEVERITY_SUCCESS
                 Informational = 0x1:STATUS_SEVERITY_INFORMATIONAL
                 Warning       = 0x2:STATUS_SEVERITY_WARNING
                 Error         = 0x3:STATUS_SEVERITY_ERROR
                )

FacilityNames = (System  = 0x0:FACILITY_SYSTEM
                 Runtime = 0x2:FACILITY_RUNTIME
                 Stubs   = 0x3:FACILITY_STUBS
                 Io      = 0x4:FACILITY_IO_ERROR_CODE
                )

LanguageNames = (English = 0x409:MSG00409
                )

;/// CATEGORY EVENTS

MessageIdTypedef = UINT16

MessageId    = 0x1
SymbolicName = NETWORK_CATEGORY
Language     = English
Network Events
.

;/// MESSAGE DEFINITIONS

MessageIdTypedef = UINT32

MessageId    = 0x1
Severity     = Error
Facility     = Runtime
SymbolicName = SVC_ERROR
Language     = English
An error has occurred (%2)
.

;#endif /// FRAMEWORK_WFP_SAMPLER_SERVICE_MESSAGES