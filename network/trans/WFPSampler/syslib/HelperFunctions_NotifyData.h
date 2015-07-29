////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_NotifyData.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with NOTIFY_DATA.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add filterID
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_NOTIFY_DATA_H
#define HELPERFUNCTIONS_NOTIFY_DATA_H

typedef struct NOTIFY_DATA_
{
   UINT32      notificationType;
   UINT32      calloutID;
   UINT64      filterID;
   const GUID* pFilterKey;
}NOTIFY_DATA, *PNOTIFY_DATA;

#endif /// HELPERFUNCTIONS_NOTIFY_DATA_H