//
// File Name:
//
//    precomp.h
//
// Abstract:
//
//    Precompiled header for the Xps Rendering Filter template
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
#include <memory>

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