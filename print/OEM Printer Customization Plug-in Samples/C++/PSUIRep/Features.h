//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2001 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Features.h
//

#pragma once

////////////////////////////////////////////////////////
//      Defines and Macros
////////////////////////////////////////////////////////



////////////////////////////////////////////////////////
//      Type Definitions
////////////////////////////////////////////////////////

// Struct used to map keyword to information
// such as display name and stickiness.
typedef struct _tagKeywordMap
{
    PCSTR   pszKeyword;
    PCWSTR  pwszModule;
    UINT    uDisplayNameID;
    DWORD   dwMode;
    DWORD   dwFlags;

} KEYWORDMAP, *PKEYWORDMAP;


// Struct container for info about
// feature that is in conflict.
typedef struct  _tagConflict
{
    PCSTR   pszFeatureKeyword;
    PCWSTR  pszFeature;
    PSTR    pszOptionKeyword;
    PWSTR   pszOption;
    PSTR    pmszReasons;
    DWORD   dwReasonsSize;

} CONFLICT, *PCONFLICT;



////////////////////////////////////////////////////////
//      Class Definitions
////////////////////////////////////////////////////////

// Class wrapper and container for Core Driver Feature Options.
class COptions
{
    private:

        // Option Infomation
        class OPTION_INFO
        {
            public:
                PKEYWORDMAP pMapping;
                PWSTR       pszDisplayName;

            public:
                OPTION_INFO()
                {
                    pMapping        = NULL;
                    pszDisplayName  = NULL;
                }

                virtual ~OPTION_INFO(){}
        };
        typedef class OPTION_INFO   *POPTION_INFO;

        // Data Members
        WORD            m_wOptions;     // Count of the number of options contained for an instance of the class.
        BYTE            m_cType;        // CPSUI Option Type (i.e. what to set pOptItem->pOptType->Type to).
        PSTR            m_pmszRaw;      // The RAW multi NULL terminated string buffer used for IPrintCoreUI2::EnumOptions().
        PCSTR           m_pszFeature;   // Pointer to the feature for which the enumerate options belong.
        PCSTR          *m_ppszOptions;  // String list pointer that points to begining of each of the strings in multi-SZ pointed to by m_pmszRaw.
        POINT           m_ptRange;      // Option range for features, such as %JobTimeout, that have a range of possible vaules not a small selection list.
        DWORD           m_dwSize;       // Size of m_pmszRaw buffer.
        PWSTR           m_pszUnits;     // String that contains the unit specifier for options, such as %JobTimeout, which need Units (i.e. seconds).
        HANDLE          m_hHeap;        // Heap to do allocations from.
        POPTION_INFO    m_pInfo;        // Array of Info about each option for a feature.

        union {                         // Current selected option for a feature.
            LONG    m_Sel;              // This is what pOptItem->m_pSel or pOptItem->m_Sel
            LPTSTR  m_pSel;             // will be set to.
        };

    public:
        COptions();
        virtual ~COptions();

        // Populate Options list for specified keyword.
        HRESULT COptions::Acquire(_In_ HANDLE hHeap,
                                  CUIHelper &Helper,
                                  _In_ POEMUIOBJ poemuiobj,
                                  _In_ PCSTR pszFeature);

        // Returns number of feature options contained in class instance.
        inline WORD GetCount() const {return m_wOptions;}

        // Returns selection.
        inline LPTSTR GetSelection() const {return m_pSel;}

        // Return nth options keyword.
        PCSTR GetKeyword(WORD wIndex) const;

        // Return nth option Display Name.
        PCWSTR GetName(WORD wIndex) const;

        // Find option with matching keyword string.
        WORD FindOption(_In_opt_ PCSTR pszOption, WORD wDefault) const;

        // Initializes options portion of OPTITEM.
        HRESULT InitOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem);

        // Refresh option selection.
        HRESULT RefreshSelection(CUIHelper &Helper, POEMUIOBJ poemuiobj);

    private:
        void Init();
        void Clear();
        void FreeOptionInfo();
        HRESULT GetOptionsForSpecialFeatures(CUIHelper &Helper, POEMUIOBJ poemuiobj);
        HRESULT GetOptionsForOutputPSLevel(CUIHelper &Helper, POEMUIOBJ poemuiobj);
        HRESULT GetOptionSelectionString(CUIHelper &Helper, POEMUIOBJ poemuiobj, _Outptr_result_maybenull_ PSTR *ppszSel);
        HRESULT GetOptionSelectionLong(CUIHelper &Helper, POEMUIOBJ poemuiobj);
        HRESULT GetOptionSelectionShort(CUIHelper &Helper, POEMUIOBJ poemuiobj);
        HRESULT GetOptionSelectionIndex(CUIHelper &Helper, POEMUIOBJ poemuiobj);
};


// Class wrapper and container for Cor Driver Features.
class CFeatures
{
    private:

        // Feature Infomation
        class FEATURE_INFO
        {
            public:
                PKEYWORDMAP pMapping;
                PWSTR       pszDisplayName;
                COptions    Options;
                DWORD       dwMode;

            public:
                FEATURE_INFO()
                {
                    pMapping        = NULL;
                    pszDisplayName  = NULL;
                    dwMode          = 0;
                }

                virtual ~FEATURE_INFO() {}
        };
        typedef class FEATURE_INFO  *PFEATURE_INFO;

        WORD            m_wFeatures;        // Count of the number of features.
        WORD            m_wDocFeatures;     // Count of the number of Document sticky features.
        WORD            m_wPrintFeatures;   // Count of the number of Printer sticky features.
        PSTR            m_pmszRaw;          // Buffer for multi-SZ for call to IPrintCoreUI2::EnumFeatures.
        PCSTR          *m_ppszKeywords;     // String list that points to each of the strings in m_pmszRaw.
        DWORD           m_dwSize;           // Size of m_pmszRaw.
        HANDLE          m_hHeap;            // Heap to do allocations from.
        PFEATURE_INFO   m_pInfo;            // Array of feature information about each of the enumerate features.

    public:
        CFeatures();
        virtual ~CFeatures();

        // Populates the Feature list
        HRESULT Acquire(_In_ HANDLE hHeap, CUIHelper &Helper, _In_ POEMUIOBJ poemuiobj);

        // Returns number of features contained in class instance.
        WORD GetCount(DWORD dwMode = 0) const;

        // Returns feature keyword.
        PCSTR GetKeyword(WORD wIndex, DWORD dwMode = 0) const;

        // Return feature Display Name.
        PCWSTR GetName(WORD wIndex, DWORD dwMode = 0) const;

        // Returns pointer to option class for nth feature.
        COptions* GetOptions(WORD wIndex, DWORD dwMode = 0) const;

        // Initializes OPTITEM for the feature.
        HRESULT InitOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem, WORD wIndex, DWORD dwMode);

    private:
        void Init();
        void Clear();
        void FreeFeatureInfo();
        WORD GetModelessIndex(WORD wIndex, DWORD dwMode) const;
};



////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT DetermineFeatureDisplayName(_In_ HANDLE hHeap, CUIHelper &Helper, POEMUIOBJ poemuiobj,
                                    _In_ PCSTR pszKeyword, const PKEYWORDMAP pMapping,
                                    _Outptr_result_maybenull_ PWSTR *ppszDisplayName);
HRESULT DetermineOptionDisplayName(_In_ HANDLE hHeap, CUIHelper &Helper, POEMUIOBJ poemuiobj,
                                   _In_ PCSTR pszFeature, _In_ PCSTR pszOption,
                                   const PKEYWORDMAP pMapping, _Outptr_result_maybenull_ PWSTR *ppszDisplayName);
HRESULT DetermineStickiness(CUIHelper &Helper, POEMUIOBJ poemuiobj, PCSTR pszKeyword,
                            const PKEYWORDMAP pMapping,PDWORD pdwMode);

PKEYWORDMAP FindKeywordMapping(PKEYWORDMAP pKeywordMap, WORD wMapSize, PCSTR pszKeyword);
HRESULT GetDisplayNameFromMapping(_In_ HANDLE hHeap, PKEYWORDMAP pMapping, _Outptr_result_maybenull_ PWSTR *ppszDisplayName);

BOOL IsFeatureOptitem(POPTITEM pOptItem);
HRESULT SaveFeatureOptItems(_In_ HANDLE hHeap, CUIHelper *pHelper, POEMUIOBJ poemuiobj,
                            HWND hWnd, POPTITEM pOptItem, WORD wItems);
HRESULT GetWhyConstrained(_In_ HANDLE hHeap, CUIHelper *pHelper, POEMUIOBJ poemuiobj,
                          _In_ PCSTR pszFeature, _In_ PCSTR pszOption, _Inout_ PSTR *ppmszReason,
                          _Inout_ PDWORD pdwSize);
HRESULT GetChangedFeatureOptions(_In_ HANDLE hHeap, POPTITEM pOptItem, WORD wItems,
                                 _Outptr_result_maybenull_ PSTR *ppmszPairs, _Out_ PWORD pdwPairs, _Out_ PDWORD pdwSize);
PSTR GetOptionKeywordFromOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem);
PWSTR GetOptionDisplayNameFromOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem);
HRESULT RefreshOptItemSelection(CUIHelper *pHelper, POEMUIOBJ poemuiobj, POPTITEM pOptItems,
                                WORD wItems);
HRESULT GetFirstConflictingFeature(_In_ HANDLE hHeap, CUIHelper *pHelper, POEMUIOBJ poemuiobj,
                                   POPTITEM pOptItem, WORD wItems, PCONFLICT pConflict);


