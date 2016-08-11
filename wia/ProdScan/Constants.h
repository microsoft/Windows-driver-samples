/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  File Title:  Constants.h
*
*  Project:     Production Scanner Driver Sample
*
*  Description: Contains declaration for macro definitions and constant
*               variables used internally by the Production Scanner Driver Sample.
*               Character string constants declared here represent names
*               that should not be localized (i.e. put into resources)
*
***************************************************************************/

#pragma once

//
// Minimum and maximum scan area dimensions are hard-coded for this sample driver
// to a size equivalent with a US Letter page size (8.5" x 11"). Note the minimum
// sizes are configured to the same sizes as maximums, reason for this being that
// the sample driver does not crop the test image to transfer. However, for property
// negotiation only the minimum sizes can be changed to lower values, just that
// the driver will ignore a smaller crop frame when scanning. A real driver should
// not do this and must always configure the real scan area dimensions, min and max:
//
#define MAX_SCAN_AREA_WIDTH  8500
#define MAX_SCAN_AREA_HEIGHT 11000
#define MIN_SCAN_AREA_WIDTH  MAX_SCAN_AREA_WIDTH
#define MIN_SCAN_AREA_HEIGHT MAX_SCAN_AREA_HEIGHT

//
// Minimum and maximum imprinter area dimensions are hard-coded as well for this
// sample driver. These dimensions match the sample test imprinter image size:
//
#define IMPRINTER_MAX_WIDTH  640
#define IMPRINTER_MAX_HEIGHT 480
#define IMPRINTER_MIN_WIDTH  IMPRINTER_MAX_WIDTH
#define IMPRINTER_MIN_HEIGHT IMPRINTER_MAX_HEIGHT

//
// This sample driver does not support landscape orientation:
//
#define LANDSCAPE_SUPPOPRTED 0

//
// The optical/native and only scan resolution for the sample driver is 300 DPI:
//
#define OPTICAL_RESOLUTION 300

//
// When scanning from the Feeder with WIA_IPS_PAGES set to ALL_PAGES or from the Auto
// source (when the the WIA_IPS_PAGES is not accessible) this sample driver has
// a hard-coded limit of the number of pages to 'scan' and transfer:
//
#define MAX_SCAN_PAGES 200

//
// The following constants define the number of pages "scanned" when the sample
// driver simulates that it detects a job separator page or a multi-feed error
// when job separators and/or multi-feed detection is enabled:
//
#define JOB_SEPARATOR_AT_PAGE 3
#define MULTI_FEED_AT_PAGE    5

//
// WIA Item Tree item names (hard-coded):
//
#define WIA_DRIVER_ROOT_NAME              L"Root"
#define WIA_DRIVER_FLATBED_NAME           L"Flatbed"
#define WIA_DRIVER_FEEDER_NAME            L"Feeder"
#define WIA_DRIVER_AUTO_NAME              L"Auto"
#define WIA_DRIVER_IMPRINTER_NAME         L"Imprinter"
#define WIA_DRIVER_ENDORSER_NAME          L"Endorser"
#define WIA_DRIVER_BARCODE_READER_NAME    L"Barcode Reader"
#define WIA_DRIVER_PATCH_CODE_READER_NAME L"Patch Code Reader"
#define WIA_DRIVER_MICR_READER_NAME       L"MICR Reader"

//
// File name extension constants used for WIA_IPA_FILENAME_EXTENSION:
//
#define FILE_EXT_BMP                      L"BMP"
#define FILE_EXT_JPG                      L"JPG"
#define FILE_EXT_RAW                      L"RAW"
#define FILE_EXT_CSV                      L"CSV"
#define FILE_EXT_TXT                      L"TXT"
#define FILE_EXT_XML                      L"XML"

//
// Valid characters for the pretended imprinter and endorser units for this sample driver:
//
#define SAMPLE_IMPRINTER_VALID_CHARS      L"0123456789abcdefghijklmnoprstuvwxzyABCDEFGHIJKLMNOPRSTUVWXZY$~!@#$*%^(){}[]|+-=<>.?_: ,"
#define SAMPLE_ENDORSER_VALID_CHARS       L"0123456789ademnloprSstx ,"

//
// Byte Order Mark (BOM) for the imprinter/endorser text when packaged in WiaImgFmt_CSV and
// WiaImgFmt_TXT files. This text must be encoded UTF-16 little-endian byte order, double-byte
// fixed size characters only (no surrogate pairs), and must be prefixed with this BOM:
//
const BYTE g_bBOM[] =
{
    0xFF,
    0xFE
};

//
// Valid barcode types claimed for the barcode reader implemented by this sample driver:
//
const LONG g_lSupportedBarcodeTypes[] =
{
    WIA_BARCODE_UPCA,
    WIA_BARCODE_UPCE,
    WIA_BARCODE_CODABAR,
    WIA_BARCODE_NONINTERLEAVED_2OF5,
    WIA_BARCODE_INTERLEAVED_2OF5,
    WIA_BARCODE_CODE39,
    WIA_BARCODE_CODE39_MOD43,
    WIA_BARCODE_CODE39_FULLASCII,
    WIA_BARCODE_CODE93,
    WIA_BARCODE_CODE128,
    WIA_BARCODE_CODE128A,
    WIA_BARCODE_CODE128B,
    WIA_BARCODE_CODE128C,
    WIA_BARCODE_GS1128,
    WIA_BARCODE_GS1DATABAR,
    WIA_BARCODE_ITF14,
    WIA_BARCODE_EAN8,
    WIA_BARCODE_EAN13,
    WIA_BARCODE_POSTNETA,
    WIA_BARCODE_POSTNETB,
    WIA_BARCODE_POSTNETC,
    WIA_BARCODE_POSTNET_DPBC,
    WIA_BARCODE_PLANET,
    WIA_BARCODE_INTELLIGENT_MAIL,
    WIA_BARCODE_POSTBAR,
    WIA_BARCODE_RM4SCC,
    WIA_BARCODE_HIGH_CAPACITY_COLOR,
    WIA_BARCODE_MAXICODE,
    WIA_BARCODE_PDF417
};

//
// Valid patch code types claimed for the patch code reader implemented by this sample driver:
//
const LONG g_lSupportedPatchCodeTypes[] =
{
    WIA_PATCH_CODE_1,
    WIA_PATCH_CODE_2,
    WIA_PATCH_CODE_3,
    WIA_PATCH_CODE_4,
    WIA_PATCH_CODE_6,
    WIA_PATCH_CODE_T
};

//
// Constant table of standard WIA document sizes, in Portrait orientation.
// Dimensions are defined in 1/1000ths of an inch. The table is included
// for use in a modified driver based on this sample, the sample driver
// in its current form supporting from this list only WIA_PAGE_LETTER:
//

typedef struct _WIA_PAGE_SIZE_COMBINATION
{
    LONG m_lPageSize;
    LONG m_lPageWidth;
    LONG m_lPageHeight;
} WIA_PAGE_SIZE_COMBINATION, *PWIA_WIA_PAGE_SIZE_COMBINATION;

const WIA_PAGE_SIZE_COMBINATION g_DefinedPageSizeCombinations[] =
{
    { WIA_PAGE_A4,             8267,  11692 },
    { WIA_PAGE_LETTER,         8500,  11000 },
    { WIA_PAGE_USLEGAL,        8500,  14000 },
    { WIA_PAGE_USLEDGER,      11000,  17000 },
    { WIA_PAGE_USSTATEMENT,    5500,   8500 },
    { WIA_PAGE_BUSINESSCARD,   3543,   2165 },
    { WIA_PAGE_ISO_A0,        33110,  46811 },
    { WIA_PAGE_ISO_A1,        23385,  33110 },
    { WIA_PAGE_ISO_A2,        16535,  23385 },
    { WIA_PAGE_ISO_A3,        11692,  16535 },
    { WIA_PAGE_ISO_A5,         5826,   8267 },
    { WIA_PAGE_ISO_A6,         4133,   5826 },
    { WIA_PAGE_ISO_A7,         2913,   4133 },
    { WIA_PAGE_ISO_A8,         2047,   2913 },
    { WIA_PAGE_ISO_A9,         1456,   2047 },
    { WIA_PAGE_ISO_A10,        1023,   1456 },
    { WIA_PAGE_ISO_B0,        39370,  55669 },
    { WIA_PAGE_ISO_B1,        27834,  39370 },
    { WIA_PAGE_ISO_B2,        19685,  27834 },
    { WIA_PAGE_ISO_B3,        13897,  19685 },
    { WIA_PAGE_ISO_B4,         9842,  13897 },
    { WIA_PAGE_ISO_B5,         6929,   9842 },
    { WIA_PAGE_ISO_B6,         4921,   6929 },
    { WIA_PAGE_ISO_B7,         3464,   4921 },
    { WIA_PAGE_ISO_B8,         2440,   3464 },
    { WIA_PAGE_ISO_B9,         1732,   2440 },
    { WIA_PAGE_ISO_B10,        1220,   1732 },
    { WIA_PAGE_ISO_C0,        36102,  51062 },
    { WIA_PAGE_ISO_C1,        25511,  36102 },
    { WIA_PAGE_ISO_C2,        18031,  25511 },
    { WIA_PAGE_ISO_C3,        12755,  18031 },
    { WIA_PAGE_ISO_C4,         9015,  12755 },
    { WIA_PAGE_ISO_C5,         6377,   9015 },
    { WIA_PAGE_ISO_C6,         4488,   6377 },
    { WIA_PAGE_ISO_C7,         3188,   4488 },
    { WIA_PAGE_ISO_C8,         2244,   3188 },
    { WIA_PAGE_ISO_C9,         1574,   2244 },
    { WIA_PAGE_ISO_C10,        1102,   1574 },
    { WIA_PAGE_JIS_B0,        40551,  57322 },
    { WIA_PAGE_JIS_B1,        28661,  40551 },
    { WIA_PAGE_JIS_B2,        20275,  28661 },
    { WIA_PAGE_JIS_B3,        14330,  20275 },
    { WIA_PAGE_JIS_B4,        10118,  14330 },
    { WIA_PAGE_JIS_B5,         7165,  10118 },
    { WIA_PAGE_JIS_B6,         5039,   7165 },
    { WIA_PAGE_JIS_B7,         3582,   5039 },
    { WIA_PAGE_JIS_B8,         2519,   3582 },
    { WIA_PAGE_JIS_B9,         1771,   2519 },
    { WIA_PAGE_JIS_B10,        1259,   1771 },
    { WIA_PAGE_JIS_2A,        46811,  66220 },
    { WIA_PAGE_JIS_4A,        66220,  93622 },
    { WIA_PAGE_DIN_2B,        55669,  78740 },
    { WIA_PAGE_DIN_4B,        78740, 111338 }
};

//
// Maximum number of colors for the color dropout feature implemented by this sample driver:
//
const LONG g_lMaxDropColors = 3;
const LONG g_lDefaultDropColors[] = { 0, 0, 0 };
