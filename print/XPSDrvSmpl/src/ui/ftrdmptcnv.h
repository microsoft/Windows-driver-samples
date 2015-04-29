/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   ftrdmptcnv.h

Abstract:

   Defines the interface for a DevMode <-> PrintTicket feature conversion class
   and an abstract class that handles the print core helper interface pointer.

--*/

#pragma once

template <typename _T = INT>
struct GPDStringToOption
{
    PCSTR pszOption;
    _T    option;
};

class IFeatureDMPTConvert
{
public:
    IFeatureDMPTConvert(){}

    virtual ~IFeatureDMPTConvert(){}

    virtual HRESULT STDMETHODCALLTYPE
    ConvertPrintTicketToDevMode(
        _In_    IXMLDOMDocument2* pPrintTicket,
        _In_    ULONG             cbDevmode,
        _Inout_ PDEVMODE          pDevmode,
        _In_    ULONG             cbDrvPrivateSize,
        _In_    PVOID             pPrivateDevmode
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE
    ConvertDevModeToPrintTicket(
        _In_    ULONG             cbDevmode,
        _In_    PDEVMODE          pDevmode,
        _In_    ULONG             cbDrvPrivateSize,
        _In_    PVOID             pPrivateDevmode,
        _Inout_ IXMLDOMDocument2* pPrintTicket
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE
    CompletePrintCapabilities(
        _In_opt_ IXMLDOMDocument2* pPrintTicket,
        _Inout_  IXMLDOMDocument2* pPrintCapabilities
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE
    PublishPrintTicketHelperInterface(
        _In_ IPrintCoreHelperUni* pHelper
        ) = 0;


    virtual HRESULT STDMETHODCALLTYPE
    ValidatePrintTicket(
        _Inout_ IXMLDOMDocument2* pPrintTicket
        ) = 0;
};

template <typename _T>
class CFeatureDMPTConvert : public IFeatureDMPTConvert
{
public:
    CFeatureDMPTConvert(){}

    virtual ~CFeatureDMPTConvert(){}

    /*++

    Routine Name:

        ConvertPrintTicketToDevMode

    Routine Description:


        The plug-in is passed an input Print Ticket that is fully populated,
        and a devmode. This method controls the calls to update the DevMode
        to reflect the settings defined in the PrintTicket.

        The template this class is instantiated using defines the data
        type passed between routines. Derived classes define this data
        and then act on it as appropriate to the feature.

    Arguments:

        pPrintTicket     - pointer to input Print Ticket
        cbDevmode        - size in bytes of input full devmode
        pDevmode         - pointer to input full devmode buffer
        cbDrvPrivateSize - buffer size in bytes of plug-in private devmode
        pPrivateDevmode  - pointer to plug-in private devmode buffer

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT STDMETHODCALLTYPE
    ConvertPrintTicketToDevMode(
        _In_    IXMLDOMDocument2* pPrintTicket,
        _In_    ULONG             cbDevmode,
        _Inout_ PDEVMODE          pDevmode,
        _In_    ULONG             cbDrvPrivateSize,
        _In_    PVOID             pPrivateDevmode
        )
    {
        HRESULT hr = S_OK;

        //
        // Validate parameters
        //
        if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
        {
            if (cbDevmode < sizeof(DEVMODE) ||
                cbDrvPrivateSize == 0)
            {
                hr = E_INVALIDARG;
            }
        }

        if (SUCCEEDED(hr))
        {
            //
            // Initialise the GPD settings from the devmode and merge with
            // the PrintTicket settings
            //
            _T ptData;

            if (SUCCEEDED(hr = GetPTDataSettingsFromDM(pDevmode, cbDevmode, pPrivateDevmode, cbDrvPrivateSize, &ptData)) &&
                SUCCEEDED(hr = MergePTDataSettingsWithPT(pPrintTicket, &ptData)))
            {
                //
                // Set the combined options in the devmode
                //
                hr = SetPTDataInDM(ptData, pDevmode, cbDevmode, pPrivateDevmode, cbDrvPrivateSize);
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        ConvertDevModeToPrintTicket

    Routine Description:

        Unidrv will call the routine with an Input PrintTicket that is
        populated with public and Unidrv private features. This method
        controls the calls to update the PrintTicket to reflect the
        settings defined in the devmode.

        The template this class is instantiated using defines the data
        type passed between routines. Derived classes define this data
        and then act on it as appropriate to the feature.

    Arguments:

        cbDevmode - size in bytes of input full devmode
        pDevmode - pointer to input full devmode buffer
        cbDrvPrivateSize - buffer size in bytes of plug-in private devmode
        pPrivateDevmode - pointer to plug-in private devmode buffer
        pPrintTicket - pointer to input Print Ticket


    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    HRESULT STDMETHODCALLTYPE
    ConvertDevModeToPrintTicket(
        _In_    ULONG             cbDevmode,
        _In_    PDEVMODE          pDevmode,
        _In_    ULONG             cbDrvPrivateSize,
        _In_    PVOID             pPrivateDevmode,
        _Inout_ IXMLDOMDocument2* pPrintTicket
        )
    {
        HRESULT hr = S_OK;

        //
        // Validate parameters
        //
        if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pPrivateDevmode, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
        {
            if (cbDevmode < sizeof(DEVMODE) ||
                cbDrvPrivateSize == 0)
            {
                hr = E_INVALIDARG;
            }
        }

        if (SUCCEEDED(hr))
        {
            //
            // Initialise the GPD settings from the devmode
            //
            _T ptData;

            if (SUCCEEDED(hr = GetPTDataSettingsFromDM(pDevmode, cbDevmode, pPrivateDevmode, cbDrvPrivateSize, &ptData)))
            {
                //
                // Set the options in the PrintTicket
                //
                hr = SetPTDataInPT(ptData, pPrintTicket);
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        PublishPrintTicketHelperInterface

    Routine Description:

        This routine is stores the print core helper interface for use by
        derived feature specific DM<->PT conversion classes.

    Arguments:

        pHelper - Pointer to core helper interface

    Return Value:

        HRESULT
        S_OK - On success
        E_* - On error

    --*/
    HRESULT STDMETHODCALLTYPE
    PublishPrintTicketHelperInterface(
        _In_ IPrintCoreHelperUni* pHelper
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pHelper, E_POINTER)) &&
            m_pCoreHelper == NULL)
        {
            m_pCoreHelper = pHelper;
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        ValidatePrintTicket

    Routine Description:

        Default validate implementation - just returns S_NO_CONFLICT. If the
        derived feature specific class needs to validate the PrintTicket it
        should provide its own implementation and handle the PrintTicket
        appropriately.

    Arguments:

        pPrintTicket - Pointer to input Print Ticket.

    Return Value:

        HRESULT
        S_NO_CONFLICT - On success
        E_* - On error

    --*/
    virtual HRESULT STDMETHODCALLTYPE
    ValidatePrintTicket(
        _Inout_ IXMLDOMDocument2* pPrintTicket
        )
    {
        HRESULT hr = S_NO_CONFLICT;

        if (SUCCEEDED(hr = CHECK_POINTER(pPrintTicket, E_POINTER)))
        {
            hr = S_NO_CONFLICT;
        }

        ERR_ON_HR(hr);
        return hr;
    }

protected:
    /*++

    Routine Name:

        GetPTDataSettingsFromDM

    Routine Description:

        This pure virtual function ensures the concrete feature converter class
        implements a means of retrieving the data type defined by that class from
        the DevMode.

    Arguments:

        pDevmode - pointer to input devmode buffer.
        cbDevmode - size in bytes of full input devmode.
        pPrivateDevmode - pointer to input private devmode buffer.
        cbDrvPrivateSize - size in bytes of private devmode.
        pDataSettings - Pointer to data structure defined by a template argument to be updated.

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    GetPTDataSettingsFromDM(
        _In_    PDEVMODE pDevmode,
        _In_    ULONG    cbDevmode,
        _In_    PVOID    pPrivateDevmode,
        _In_    ULONG    cbDrvPrivateSize,
        _Out_   _T*      pDataSettings
        ) = 0;

    /*++

    Routine Name:

        MergePTDataSettingsWithPT

    Routine Description:

        This pure virtual function ensures the concrete feature converter class
        implements a means of merging the data type defined by that class from
        the PrintTicket settings.

    Arguments:

        pPrintTicket  - Pointer to the PrintTicket to merge the data structure with.
        pDataSettings - Pointer to data structure defined by a template argument to be updated.

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    MergePTDataSettingsWithPT(
        _In_    IXMLDOMDocument2* pPrintTicket,
        _Inout_ _T*               pDrvSettings
        ) = 0;

    /*++

    Routine Name:

        SetPTDataInDM

    Routine Description:

        This pure virtual function ensures the concrete feature converter class
        implements a means of setting the data type defined by that class in the
        DevMode.

    Arguments:

        dataSettings - const reference to the data structure defined by a template argument to be update from.
        pDevmode - pointer to input devmode buffer.
        cbDevmode - size in bytes of full input devmode.
        pPrivateDevmode - pointer to input private devmode buffer.
        cbDrvPrivateSize - size in bytes of private devmode.

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    SetPTDataInDM(
        _In_    CONST _T& dataSettings,
        _Inout_ PDEVMODE  pDevmode,
        _In_    ULONG     cbDevmode,
        _Inout_ PVOID     pPrivateDevmode,
        _In_    ULONG     cbDrvPrivateSize
        ) = 0;

    /*++

    Routine Name:

        SetPTDataInPT

    Routine Description:

        This pure virtual function ensures the concrete feature converter class
        implements a means of setting the data type defined by that class in the
        PrintTicket.

    Arguments:

        dataSettings - const reference to the data structure defined by a template argument to be update from.
        pPrintTicket - Pointer to the PrintTicket to update the data structure with.

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    virtual HRESULT
    SetPTDataInPT(
        _In_    CONST _T&         dataSettings,
        _Inout_ IXMLDOMDocument2* pPrintTicket
        ) = 0;

    /*++

    Routine Name:

        GetOptionFromGPDString

    Routine Description:

        This template function takes as a template parameter the type of the
        data to be retrieved from the DevMode. A table is passed through that
        provides the look-up between the GPD string and the data type to be
        returned. The routine then iterates through the options for the feature
        specified matching against look-up and returning the corresponding value.

    Arguments:

        pDevmode    - pointer to input DevMode.
        cbDevmode   - count of bytes in the input devmode.
        pszFeature  - string defining the feature name to match against.
        pOptTable   - pointer to the look-up table matching the GPD option strings to the option value.
        cOptEntries - the number of entries in the look-up table
        result      - reference to the result value to be updated (type defined by the template argument)


    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    template<typename _U>
    HRESULT
    GetOptionFromGPDString(
        _In_   PDEVMODE                     pDevmode,
        _In_   ULONG                        cbDevmode,
        _In_z_ PCSTR                        pszFeature,
        _In_   CONST GPDStringToOption<_U>* pOptTable,
        _In_   CONST  UINT                  cOptEntries,
        _Out_  _U&                          result
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pszFeature, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pOptTable, E_POINTER)))
        {
            if (cbDevmode < sizeof(DEVMODE))
            {
                hr = E_INVALIDARG;
            }
        }

        if (SUCCEEDED(hr))
        {
            BOOL bFound = FALSE;

            if (cOptEntries > 0)
            {
                PCSTR pszOption = NULL;

                if (SUCCEEDED(hr = m_pCoreHelper->GetOption(pDevmode, cbDevmode, pszFeature, &pszOption)))
                {
                    CStringXDA cstrOption(pszOption);
                    for (UINT index = 0; index < cOptEntries && !bFound; index++)
                    {
                        if (cstrOption == pOptTable->pszOption)
                        {
                            result = pOptTable->option;
                            bFound = TRUE;
                        }
                        else
                        {
                            pOptTable++;
                        }
                    }
                }
            }

            if (!bFound)
            {
                hr = E_ELEMENT_NOT_FOUND;
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }

    /*++

    Routine Name:

        SetGPDStringFromOption

    Routine Description:

        This template function takes as a template parameter the type of the
        data to be set in the DevMode. A table is passed through that provides
        the look-up between the GPD string and the data type to be set. The routine
        then iterates through the options for the feature specified matching against
        the look-up and setting the corresponding value in the DevMode.

    Arguments:

        pDevmode    - pointer to input DevMode.
        cbDevmode   - count of bytes in the input devmode.
        pszFeature  - string defining the feature name to match against.
        pOptTable   - pointer to the look-up table matching the GPD option strings to the option value.
        cOptEntries - the number of entries in the look-up table
        result      - const reference to the result value to update from (type defined by the template argument)

    Return Value:

        HRESULT
        S_OK - On success
        E_*  - On error

    --*/
    template<typename _U>
    HRESULT
    SetGPDStringFromOption(
        _Inout_ PDEVMODE                     pDevmode,
        _In_    ULONG                        cbDevmode,
        _In_z_  PCSTR                        pszFeature,
        _In_    CONST GPDStringToOption<_U>* pOptTable,
        _In_    CONST UINT                   cOptEntries,
        _In_    CONST _U&                    option
        )
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = CHECK_POINTER(pDevmode, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pszFeature, E_POINTER)) &&
            SUCCEEDED(hr = CHECK_POINTER(pOptTable, E_POINTER)))
        {
            if (cbDevmode < sizeof(DEVMODE))
            {
                hr = E_INVALIDARG;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (cOptEntries > 0)
            {
                for (UINT index = 0; index < cOptEntries; index++)
                {
                    if (option == pOptTable->option)
                    {
                        PRINT_FEATURE_OPTION featureOption[1] = {
                            pszFeature,
                            pOptTable->pszOption
                        };

                        DWORD dwResult = 0;
                        DWORD cPairsWritten = 0;

                        hr = m_pCoreHelper->SetOptions(pDevmode, cbDevmode, TRUE, featureOption, 1, &cPairsWritten, &dwResult);

                        break;
                    }
                    else
                    {
                        pOptTable++;
                    }
                }
            }
        }

        ERR_ON_HR(hr);
        return hr;
    }


protected:
    CComPtr<IPrintCoreHelperUni> m_pCoreHelper;
};

