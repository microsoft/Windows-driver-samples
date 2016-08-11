/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  FileConv.h
*
*  Project:     Production Scanner Driver Sample
*
*  Description: This file contains definitions for the utility functions
*               used by the sample driver for image file format conversions.
*
***************************************************************************/

#pragma once

HRESULT
InitializeGDIPlus(
    _Out_ ULONG_PTR *ppToken);

HRESULT
ShutdownGDIPlus(
    _In_ ULONG_PTR pToken);

HRESULT
ConvertImageToDIB(
    _In_        Image    *pInputImage,
    _Outptr_    IStream **ppOutputStream,
    _Out_opt_   LONG     *plImageWidth = NULL,
    _Out_opt_   LONG     *plImageHeight = NULL,
    _Out_opt_   LONG     *plOutputImageBPL = NULL);

HRESULT
ConvertDibToRaw(
    _In_  IStream  *pInputStream,
    _Out_ IStream **ppOutputStream);


