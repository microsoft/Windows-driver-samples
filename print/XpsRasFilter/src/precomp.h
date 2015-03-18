// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    precomp.h
//
// Abstract:
//
//    Precompiled header for the Xps Rasterization Service sample filter.
//

#pragma once

//
// Define this as a usermode driver for analysis purposes
//
#include <DriverSpecs.h>
__user_driver

//
// Standard Annotation Language include
//
#include <sal.h>

//
// Windows includes
//
#include <windows.h>

// Standard includes
#include <cstring>
#include <intsafe.h>
#include <new>

// STL
#include <vector>

//
// COM includes
//
#include <objbase.h>
#include <oleauto.h>

//
// Filter pipeline includes
//
#include <winspool.h>
#include <filterpipeline.h>
#include <filterpipelineutil.h>
#include <prntvpt.h>

//
// ATL
//
#include <atlbase.h>

//
// WIC
//
#include <wincodec.h>

//
// MSXML
//
#include <msxml6.h>

//
// OPC Layer
//
#include <msopc.h>

//
// Xps Object Model
//
#include <XpsObjectModel.h>

//
// Xps Rasterization Service
//
#include <xpsrassvc.h>

