/**************************************************************************

    A/V Stream Camera Sample

    Copyright (c) 2014, Microsoft Corporation.

    File:

        macros.h

    Abstract:

        This file contains common error checking macros for the MFT0

    History:

        created 10/10/2014

**************************************************************************/

#pragma once

#define IFFAILED_BREAK(exp) \
{ \
    if ( FAILED(hr= (exp)) ) \
    { \
        break; \
    } \
}

#define IFFALSE_BREAK_HR(exp, retval) \
{ \
    if ( !(exp)) \
    { \
        hr = retval; \
        break; \
    } \
}


#define IFNULL_BREAK_HR(exp, retval) \
{ \
    if ( (exp) == NULL ) \
    { \
        hr = retval; \
        break; \
    } \
}