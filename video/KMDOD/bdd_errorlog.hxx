/******************************Module*Header*******************************\
* Module Name: BDD_ErrorLog.hxx
*
* Basic Display Driver Logging Macros
*
*
* Copyright (c) 2010 Microsoft Corporation
*
\**************************************************************************/
#ifndef _BDD_ERRORLOG_HXX_
#define _BDD_ERRORLOG_HXX_

#define BDD_LOG_ERROR0(Msg) 
#define BDD_LOG_ERROR1(Msg,Param1)
#define BDD_LOG_ERROR2(Msg,Param1,Param2)
#define BDD_LOG_ERROR3(Msg,Param1,Param2,Param3)
#define BDD_LOG_ERROR4(Msg,Param1,Param2,Param3,Param4)
#define BDD_LOG_ERROR5(Msg,Param1,Param2,Param3,Param4,Param5)

//
// Warnings
//

#define BDD_LOG_WARNING0(Msg)
#define BDD_LOG_WARNING1(Msg,Param1)
#define BDD_LOG_WARNING2(Msg,Param1,Param2)
#define BDD_LOG_WARNING3(Msg,Param1,Param2,Param3)
#define BDD_LOG_WARNING4(Msg,Param1,Param2,Param3,Param4)
#define BDD_LOG_WARNING5(Msg,Param1,Param2,Param3,Param4,Param5)

//
// Events (i.e. low-frequency tracing)
//

#define BDD_LOG_EVENT0(Msg)
#define BDD_LOG_EVENT1(Msg,Param1)
#define BDD_LOG_EVENT2(Msg,Param1,Param2)
#define BDD_LOG_EVENT3(Msg,Param1,Param2,Param3)
#define BDD_LOG_EVENT4(Msg,Param1,Param2,Param3,Param4)
#define BDD_LOG_EVENT5(Msg,Param1,Param2,Param3,Param4,Param5)

//
// Information (i.e. high-frequency tracing)
//

#define BDD_LOG_INFORMATION0(Msg)
#define BDD_LOG_INFORMATION1(Msg,Param1)
#define BDD_LOG_INFORMATION2(Msg,Param1,Param2)
#define BDD_LOG_INFORMATION3(Msg,Param1,Param2,Param3)
#define BDD_LOG_INFORMATION4(Msg,Param1,Param2,Param3,Param4)
#define BDD_LOG_INFORMATION5(Msg,Param1,Param2,Param3,Param4,Param5)

//
// Low resource logging macros.
//

#define BDD_LOG_LOW_RESOURCE0(Msg)
#define BDD_LOG_LOW_RESOURCE1(Msg,Param1)
#define BDD_LOG_LOW_RESOURCE2(Msg,Param1,Param2)
#define BDD_LOG_LOW_RESOURCE3(Msg,Param1,Param2,Param3)
#define BDD_LOG_LOW_RESOURCE4(Msg,Param1,Param2,Param3,Param4)
#define BDD_LOG_LOW_RESOURCE5(Msg,Param1,Param2,Param3,Param4,Param5)

//
// Assertion logging macros.
//

#define BDD_LOG_ASSERTION0(Msg) NT_ASSERT(FALSE)
#define BDD_LOG_ASSERTION1(Msg,Param1) NT_ASSERT(FALSE)
#define BDD_LOG_ASSERTION2(Msg,Param1,Param2) NT_ASSERT(FALSE)
#define BDD_LOG_ASSERTION3(Msg,Param1,Param2,Param3) NT_ASSERT(FALSE)
#define BDD_LOG_ASSERTION4(Msg,Param1,Param2,Param3,Param4) NT_ASSERT(FALSE)
#define BDD_LOG_ASSERTION5(Msg,Param1,Param2,Param3,Param4,Param5) NT_ASSERT(FALSE)
#define BDD_ASSERT(exp) {if (!(exp)) {BDD_LOG_ASSERTION0(#exp);}}

#if DBG
#define BDD_ASSERT_CHK(exp) BDD_ASSERT(exp)
#else
#define BDD_ASSERT_CHK(exp) {}
#endif


#endif  //_BDD_ERRORLOG_HXX_

