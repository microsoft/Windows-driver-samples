/**************************************************************************
*
*  Copyright © Microsoft Corporation
*
*  Title: MiniDrv.cpp
*
*  Description: This file contains the IWiaMiniDrv interface implementation
*               for the Production Scanner Driver Sample, plus C++ constructor and
*               destructor code for the main driver's objects, CWiaDriver.
*
***************************************************************************/

#include "stdafx.h"

HINSTANCE g_hInst = NULL;

/**************************************************************************\
*
* CWiaDriver constructor
*
\**************************************************************************/

CWiaDriver::CWiaDriver(
    _In_opt_ LPUNKNOWN punkOuter) :
    m_cRef(1),
    m_punkOuter(NULL),
    m_pIDrvItemRoot(NULL),
    m_lClientsConnected(0),
    m_bstrDeviceID(NULL),
    m_bstrRootFullItemName(NULL),
    m_pIStiDevice(NULL),
    m_hDeviceKey(NULL),
    m_bFeederStarted(FALSE)
{
    //
    // See if we are aggregated. If we are (almost always the case)
    // save the pointer to the controlling IUnknown, so subsequent
    // calls will be delegated. If not, set the same pointer to "this":
    //
    if (punkOuter)
    {
        m_punkOuter = punkOuter;
    }
    else
    {
        //
        // This cast is needed in order to point to right virtual table:
        //
        m_punkOuter = reinterpret_cast<IUnknown*>(static_cast<INonDelegatingUnknown*>(this));
    }

    memset(m_wszDevicePath, 0, sizeof(m_wszDevicePath));


    //
    // Warning: do not initialize the entire contents of m_config and m_status to 0,
    // this will erase the function pointer tables for the CBasicDynamicArray members!
    //

    m_hrLastEdviceError = STI_ERROR_NO_ERROR;

    m_hWiaEvent = NULL;
    m_hWiaEventStoredCopy = NULL;
    m_bstrScanAvailableItem = NULL;

    //
    // IStiUSD::Initialize not executed yet:
    //
    m_bInitialized = FALSE;

    //
    // Initialize the critical section for DestroyDriverItemTree:
    //
    InitializeCriticalSection(&m_csDestroyDriverItemTree);

    WIAS_TRACE((g_hInst, "Driver object (%p, process: %u) created", this, GetCurrentProcessId()));
}

/**************************************************************************\
*
* CWiaDriver destructor
*
\**************************************************************************/

CWiaDriver::~CWiaDriver()
{
    DWORD dwProcessId = GetCurrentProcessId();
    DWORD dwThreadId = GetCurrentThreadId();

    WIAS_TRACE((g_hInst, "Destroying driver object (%p, process: %u, thread: %u)..",
        this, dwProcessId, dwThreadId));

    //
    // Free the memory allocated for the global device ID and root item name:
    //
    if (m_bstrDeviceID)
    {
        SysFreeString(m_bstrDeviceID);
        m_bstrDeviceID = NULL;
    }

    if (m_bstrRootFullItemName)
    {
        SysFreeString(m_bstrRootFullItemName);
        m_bstrRootFullItemName = NULL;
    }

    //
    // Free WIA_FORMAT_INFO arrays:
    //
    m_tFormatInfo.Destroy();
    m_tFormatInfoImprinterEndorser.Destroy();
    m_tFormatInfoBarcodeReader.Destroy();
    m_tFormatInfoPatchCodeReader.Destroy();
    m_tFormatInfoMicrReader.Destroy();

    //
    // Free cached driver capability array:
    //
    m_tCapabilityManager.Destroy();

    //
    // Unlink and release the cached IWiaDrvItem root item interface:
    //
    DestroyDriverItemTree();

    //
    // The driver item tree is destroyed, the critical section can be deleted.
    // Make sure there is no concurrent thread releasing the Root item from
    // within IWiaMiniDrv::drvUnInitializeWia and delete the critical section:
    //
    EnterCriticalSection(&m_csDestroyDriverItemTree);
    LeaveCriticalSection(&m_csDestroyDriverItemTree);
    DeleteCriticalSection(&m_csDestroyDriverItemTree);

    if (m_bstrScanAvailableItem)
    {
        SysFreeString(m_bstrScanAvailableItem);
        m_bstrScanAvailableItem = NULL;
    }

    //
    // The WIA service may release the driver object during a scanner status update
    // operation, for example following an unexpected device disconnect event.
    // When this happens we must wait for the critical section to be released before
    // deleting it, otherwise the thread owning the CS may remain in an undefined state:
    //

    m_bInitialized = FALSE;

    WIAS_TRACE((g_hInst, "Driver object (%p, process: %u, thread: %u) destroyed", this, dwProcessId, dwThreadId));
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvInitializeWia. Initializes the mini-driver in
* the context of a new WIA application session and creates if needed the
* unique Driver Item Tree describing the item architecture and item names
* that the WIA service will use to create duplicate Application Item Trees
* for each new WIA application session opened with the driver. When the WIA
* service executes this method the driver must receive a character string
* containing the device’s unique identifier along with the IStiDevice COM
* interface pointer describing the current device. The driver must create
* the driver item tree if it hasn’t been built yet. Finally, the driver
* must return back to the WIA service the pointer to the Root item in the
* Driver Item Tree.
*
* Parameters:
*
*    pWiasContext         - pointer to the item context
*    lFlags               - reserved (set to 0)
*    bstrDeviceID         - string containing the device's unique identifier
*    bstrRootFullItemName - string containing the full name of the root item
*    pStiDevice           - points to an IStiDevice interface
*    pIUnknownOuter       - (optional) to receive an IUnknown interface address
*    ppIDrvItemRoot       - receives the address of the IWiaDrvItem interface
*                           for the root item
*    ppIUnknownInner      - unsupported and always set to NULL (all this driver's WIA
*                           functionality is covered through its IWiaMiniDrv interface)
*    plDevErrVal          - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvInitializeWia(
    _Inout_   BYTE*         pWiasContext,
              LONG          lFlags,
    _In_      BSTR          bstrDeviceID,
    _In_      BSTR          bstrRootFullItemName,
    _In_      IUnknown*     pStiDevice,
    _In_      IUnknown*     pIUnknownOuter,
    _Out_     IWiaDrvItem** ppIDrvItemRoot,
    _Out_     IUnknown**    ppIUnknownInner,
    _Out_     LONG*         plDevErrVal)
{
    UNREFERENCED_PARAMETER(pIUnknownOuter);
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;

    //
    // (1/2) Uncomment the code below to enable a basic safety guard against premature
    // drvInitializeWia call made by WIA Service before IStiUSD::Initialize completes:
    //
    // If IWiaMiniDrv::drvInitializeWia is called before IStiUSD::Initialize
    // is complete wait up to 1 minute (10 msec x 6000 times) and retry:
    //
    // const LONG lMaxWaitCycles = 6000;
    // const LONG lWaitInterval = 10;
    // LONG lWaitCycles = 0;
    //

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!plDevErrVal))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;
        *ppIDrvItemRoot = NULL;
        *ppIUnknownInner = NULL;

        if (!m_bstrDeviceID)
        {
            m_bstrDeviceID = SysAllocString(bstrDeviceID);
            if (!m_bstrDeviceID)
            {
                hr = E_OUTOFMEMORY;
                WIAEX_ERROR((g_hInst, "Failed to allocate BSTR DeviceID string, hr = 0x%08X", hr));
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!m_pIStiDevice)
        {
            m_pIStiDevice = reinterpret_cast<IStiDevice*>(pStiDevice);
        }

        if (!m_bstrRootFullItemName)
        {
            m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);
            if (!m_bstrRootFullItemName)
            {
                hr = E_OUTOFMEMORY;
                WIAEX_ERROR((g_hInst, "Failed to allocate BSTR Root full item name string, hr = 0x%08X", hr));
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!m_pIDrvItemRoot)
        {
            //
            // (2/2) Uncomment the code below to enable a basic safety guard against premature
            // drvInitializeWia call made by WIA Service before IStiUSD::Initialize completes:
            //
            // The WIA service may call IWiaMiniDrv::drvInitializeWia before the
            // IStiUSD::Initialize call is completed. Temporarily block creating the Driver Item
            // Tree until IStiUSD::Initialize is complete:
            //
            // if (!m_bInitialized)
            // {
            //     WIAS_TRACE((g_hInst, "Driver not intialized yet, wait.."));
            //
            //     //
            //     // Wait up to 10 msec x 6000 = 1 minute for IStiUSD::Initialize to complete:
            //     //
            //     while ((!m_bInitialized) && ((++lWaitCycles) <= lMaxWaitCycles))
            //     {
            //         Sleep(lWaitInterval);
            //     }
            //
            //     if (m_bInitialized)
            //     {
            //         WIAS_TRACE((g_hInst, "Driver intialized now"));
            //     }
            //     else
            //     {
            //         WIAEX_ERROR((g_hInst, "Maxmimum timeout reached, driver still not initialized"));
            //     }
            // }
            //

            //
            // Create the Driver Item Tree matching the current scanner configuration:
            //
            hr = BuildDriverItemTree();
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Driver Item Tree, hr = 0x%08X", hr));
            }
        }
        else
        {
            //
            // The Driver Item Tree already exists. The root item of this item tree
            // should be returned to the WIA service:
            //
            hr = S_OK;
        }
    }

    //
    // Increment the client connection count only when the driver has
    // successfully created all the necessary Driver Item Tree items:
    //
    if (SUCCEEDED(hr))
    {
        *ppIDrvItemRoot = m_pIDrvItemRoot;
        InterlockedIncrement(&m_lClientsConnected);
        WIAS_TRACE((g_hInst,"drvInitializeWia, %d client(s) are currently connected to this driver",  m_lClientsConnected));
    }


    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvInitializeWia 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvInitItemProperties. The WIA service builds the
* Application Item Tree and then asks the driver to initialize each item
* executing IWiaMiniDvr::drvInitItemProperties on each before to hand the
* entire tree over to the application (when completing the IWiaDevMgr::
* CreateDevice or IWiaDevMgr2::CreateDevice call the application makes
* to open a new WIA session). When this method is called the driver is given
* the context of the Application Tree Item to be initialized. The driver must
* populate the item with WIA properties, fully initialized with their names,
* access flags, valid and current values.
*
* This driver supports to create and initialize one of each of the following items:
*
* Root (WIA_CATEGORY_ROOT)
* Flatbed (WIA_CATEGORY_FLATBED, no children)
* Feeder (WIA_CATEGORY_FEEDER, no children)
* Auto (WIA_CATEGORY_AUTO)
* Imprinter (WIA_CATEGORY_IMPRINTER)
* Endorser (WIA_CATEGORY_ENDORSER)
* Barcode Reader (WIA_CATEGORY_BARCODE_READER)
* Patch Code Reader (WIA_CATEGORY_PATCH_CODE_READER)
* MICR Reader (WIA_CATEGORY_MICR_READER)
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvInitItemProperties(
    _Inout_ BYTE* pWiasContext,
            LONG  lFlags,
    _Out_   LONG* plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;
    LONG lItemFlags  = 0;

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!plDevErrVal))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;

        //
        // Read WIA_IPA_ITEM_FLAGS to identify the item to be initialized:
        //
        hr = wiasReadPropLong(pWiasContext, WIA_IPA_ITEM_FLAGS, &lItemFlags, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_ITEM_FLAGS property, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        if (lItemFlags & WiaItemTypeRoot)
        {
            //
            // This is the Root item, initialize the Root item properties as well as the
            // Root mini-driver item context containing the scan destination names:
            //

            WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Root.."));

            hr = InitializeRootItemProperties(pWiasContext);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize root item properties, hr = 0x%08X", hr));
            }
        }
        else if ((lItemFlags & WiaItemTypeProgrammableDataSource) &&
            (lItemFlags & WiaItemTypeTransfer) &&
            (lItemFlags & WiaItemTypeFile))
        {
            //
            // This is a child programmable data source item - detect which one from the item name:
            //

            IWiaDrvItem *pIWiaDrvItem = NULL;
            BSTR bstrItemName = NULL;

            hr = wiasGetDrvItem(pWiasContext, &pIWiaDrvItem);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to retrieve the current driver item to initialize, hr = 0x%08X", hr));
            }

            if (SUCCEEDED(hr))
            {
                hr = pIWiaDrvItem->GetItemName(&bstrItemName);
                if (FAILED (hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to get the item name, hr = 0x%08X", hr));
                }
            }

            if (SUCCEEDED(hr))
            {
                if (!wcscmp(WIA_DRIVER_FLATBED_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Flatbed.."));

                    hr = InitializeChildItemProperties(pWiasContext, FLAT);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the flatbed item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_FEEDER_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Feeder.."));

                    hr = InitializeChildItemProperties(pWiasContext, FEED);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the feeder item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_AUTO_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Auto.."));

                    hr = InitializeChildItemProperties(pWiasContext, AUTO_SOURCE);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the automatic input source item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_IMPRINTER_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Imprinter.."));

                    hr = InitializeChildItemProperties(pWiasContext, IMPRINTER);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the imprinter item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_ENDORSER_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Endorser.."));

                    hr = InitializeChildItemProperties(pWiasContext, ENDORSER);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the endorser item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_BARCODE_READER_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Barcode Reader.."));

                    hr = InitializeChildItemProperties(pWiasContext, BARCODE_READER);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the barcode reader item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_PATCH_CODE_READER_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for Patch Code Reader.."));

                    hr = InitializeChildItemProperties(pWiasContext, PATCH_CODE_READER);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the patch code reader item's property set, hr = 0x%08X", hr));
                    }
                }
                else if (!wcscmp(WIA_DRIVER_MICR_READER_NAME, bstrItemName))
                {
                    WIAS_TRACE((g_hInst,"IWiaMiniDrv::drvInitItemProperties called for MICR Reader.."));

                    hr = InitializeChildItemProperties(pWiasContext, MICR_READER);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to initialize the MICR reader item's property set, hr = 0x%08X", hr));
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                    WIAEX_ERROR((g_hInst, "Unsupported item (item name: %ws), hr = 0x%08X", bstrItemName, hr));
                }

                if (bstrItemName)
                {
                    SysFreeString(bstrItemName);
                }
            }
        }
        else if (lItemFlags & WiaItemTypeGenerated)
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "WiaItemTypeGenerated items are not supported by this driver, hr = 0x%08X", hr));
        }
        else
        {
            hr = E_INVALIDARG;
            WIAEX_ERROR((g_hInst, "Unsupported item (item flags: 0x%X), hr = 0x%08X", lItemFlags, hr));
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvInitItemProperties 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvValidateItemProperties. The WIA Service calls
* IWiaMinIDrv::drvValidateItemProperties for properties that an application
* requested to be changed through a IWiaPropertyStorage::WriteMultiple call.
* The driver should validate each individual set request against the set of
* valid property values in the current context and if validation is successful
* it must update all dependent properties.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    nPropSpec    - indicates the number of properties in the pPropSpec array
*    pPropSpec    - list of PROPSPEC elements for the properties to be validated
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvValidateItemProperties(
    _Inout_                      BYTE     *pWiasContext,
                                 LONG      lFlags,
                                 ULONG     nPropSpec,
    _In_reads_(nPropSpec) const PROPSPEC *pPropSpec,
    _Out_                        LONG     *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;
    WIA_PROPERTY_CONTEXT PropertyContext = {};
    PROPID *pPropID = NULL;
    BOOL bPropertyContext = FALSE;
    LONG lDocumentHandlingSelect = FLAT;
    LONG lItemType = 0;

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!pPropSpec) || (!plDevErrVal) || (!nPropSpec))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;

        hr = wiasGetItemType(pWiasContext, &lItemType);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to get item type, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        if (lItemType & WiaItemTypeRoot)
        {
            //
            // Root item properties for this sample driver do not need any additional validation:
            //
            hr = S_OK;
        }
        else
        {
            GUID guidItemCategory = {};

            //
            // Read WIA_IPA_ITEM_CATEGORY to figure out which child item this is
            // (the item names can be also used for this identification purpose,
            // and should be used if there is more than one item with the same
            // item category):
            //
            hr = wiasReadPropGuid(pWiasContext, WIA_IPA_ITEM_CATEGORY, &guidItemCategory, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_ITEM_CATEGORY, hr = 0x%08X", hr));
            }

            //
            // We need to create an array of property IDs for the properties to be added to the default
            // ones existing in the property context to be built with wiasCreatePropContext
            //
            if (SUCCEEDED(hr))
            {
                pPropID = (PROPID*) CoTaskMemAlloc(sizeof(PROPID) * nPropSpec);
                if (pPropID)
                {
                    for (ULONG i = 0; i < nPropSpec; i++)
                    {
                        pPropID[i] = pPropSpec[i].propid;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    WIAEX_ERROR((g_hInst, "Out of memory, hr = 0x%08X", hr));
                }
            }

            //
            // Create a propery context for the properties being validated:
            //
            if (SUCCEEDED(hr))
            {
                if (IsEqualGUID(WIA_CATEGORY_FLATBED, guidItemCategory))
                {
                    lDocumentHandlingSelect = FLAT;
                }
                else if (IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory))
                {
                    lDocumentHandlingSelect = FEED;
                }
                else if (IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory))
                {
                    lDocumentHandlingSelect = AUTO_SOURCE;
                }
                else if (IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory))
                {
                    lDocumentHandlingSelect = IMPRINTER;
                }
                else if (IsEqualGUID(WIA_CATEGORY_ENDORSER, guidItemCategory))
                {
                    lDocumentHandlingSelect = ENDORSER;
                }
                else if (IsEqualGUID(WIA_CATEGORY_BARCODE_READER, guidItemCategory))
                {
                    lDocumentHandlingSelect = BARCODE_READER;
                }
                else if (IsEqualGUID(WIA_CATEGORY_PATCH_CODE_READER, guidItemCategory))
                {
                    lDocumentHandlingSelect = PATCH_CODE_READER;
                }
                else if (IsEqualGUID(WIA_CATEGORY_MICR_READER, guidItemCategory))
                {
                    lDocumentHandlingSelect = MICR_READER;
                }

                hr = wiasCreatePropContext(nPropSpec, (PROPSPEC*)pPropSpec, nPropSpec, pPropID, &PropertyContext);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to create WIA property context to validate %u properties, hr = 0x%08X", nPropSpec, hr));
                }
                else
                {
                    bPropertyContext = TRUE;
                }
            }

            //
            // Validate format properties and update as necessary:
            //
            // WIA_IPA_DATATYPE
            // WIA_IPA_DEPTH
            // WIA_IPA_CHANNELS_PER_PIXEL
            // WIA_IPA_BITS_PER_CHANNEL
            // WIA_IPA_FORMAT
            // WIA_IPA_FILENAME_EXTENSION
            // WIA_IPA_TYMED
            // WIA_IPA_COMPRESSION
            //
            if (SUCCEEDED(hr))
            {
                hr = ValidateFormatProperties(pWiasContext, &PropertyContext, lDocumentHandlingSelect);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to validate format properties, hr = 0x%08X", hr));
                }
            }

            if ((FLAT == lDocumentHandlingSelect) || (FEED == lDocumentHandlingSelect))
            {
                //
                // Validate scan region/document size properties:
                //
                // WIA_IPS_PAGE_SIZE
                // WIA_IPS_ORIENTATION
                // WIA_IPS_PAGE_WIDTH
                // WIA_IPS_PAGE_HEIGHT
                // WIA_IPS_XPOS
                // WIA_IPS_YPOS
                // WIA_IPS_XEXTENT
                // WIA_IPS_YEXTENT
                // WIA_IPS_XRES
                // WIA_IPS_YRES
                // WIA_IPS_XSCALING
                // WIA_IPS_YSCALING
                // WIA_IPS_LONG_DOCUMENT
                //
                if (SUCCEEDED(hr))
                {
                    hr = ValidateRegionProperties(pWiasContext, &PropertyContext, lDocumentHandlingSelect);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to validate region properties, hr = 0x%08X", hr));
                    }
                }

                //
                // Validate image information properties:
                //
                // WIA_IPA_PIXELS_PER_LINE
                // WIA_IPA_NUMBER_OF_LINES
                // WIA_IPA_BYTES_PER_LINE
                //
                if (SUCCEEDED(hr))
                {
                    hr = ValidateImageInfoProperties(pWiasContext, &PropertyContext, lDocumentHandlingSelect);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to validate image information properties, hr = 0x%08X", hr));
                    }
                }

                //
                // Validate color drop properties:
                //
                // WIA_IPS_COLOR_DROP_RED
                // WIA_IPS_COLOR_DROP_GREEN and
                // WIA_IPS_COLOR_DROP_BLUE
                //
                if (SUCCEEDED(hr))
                {
                    hr = ValidateColorDropProperties(pWiasContext, &PropertyContext, lDocumentHandlingSelect);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to validate color drop properties, hr = 0x%08X", hr));
                    }
                }

                //
                // Validate other feeder specific properties:
                //
                // WIA_IPS_DOCUMENT_HANDLING_SELECT
                // WIA_IPS_PAGES
                //
                if (SUCCEEDED(hr) && (FEED == lDocumentHandlingSelect))
                {
                    hr = ValidateFeedProperties(pWiasContext, &PropertyContext);
                    if (FAILED(hr))
                    {
                        WIAEX_ERROR((g_hInst, "Failed to validate feeder specific properties, hr = 0x%08X", hr));
                    }
                }
            }
            else if ((IMPRINTER == lDocumentHandlingSelect) || (ENDORSER == lDocumentHandlingSelect))
            {
                hr = ValidateImprinterEndorserProperties(pWiasContext, &PropertyContext, lDocumentHandlingSelect);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to validate imprinter/endorser specific properties, hr = 0x%08X", hr));
                }
            }
            else if (BARCODE_READER == lDocumentHandlingSelect)
            {
                hr = ValidateBarcodeReaderProperties(pWiasContext, &PropertyContext);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to validate barcode reader specific properties, hr = 0x%08X", hr));
                }
            }
            else if (PATCH_CODE_READER == lDocumentHandlingSelect)
            {
                hr = ValidatePatchCodeReaderProperties(pWiasContext, &PropertyContext);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to validate patch code reader specific properties, hr = 0x%08X", hr));
                }
            }
            else if (MICR_READER == lDocumentHandlingSelect)
            {
                hr = ValidateMicrReaderProperties(pWiasContext, &PropertyContext);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to validate MICR reader specific properties, hr = 0x%08X", hr));
                }
            }

            //
            // Validate all changed properties against their (for some of the above properties) updated valid values:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasValidateItemProperties(pWiasContext, nPropSpec, pPropSpec);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to validate properties using wiasValidateItemProperties, hr = 0x%08X", hr));
                }
            }

            //
            // Free the property context created, if any:
            //
            if (bPropertyContext)
            {
                HRESULT FreePropContextHR = wiasFreePropContext(&PropertyContext);
                if (FAILED(FreePropContextHR))
                {
                    WIAEX_ERROR((g_hInst, "wiasFreePropContext failed, hr = 0x%08X", FreePropContextHR));
                }
            }
        }
    }

    if (pPropID)
    {
        CoTaskMemFree(pPropID);
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvValidateItemProperties 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvWriteItemProperties. When this method is called
* the driver is given the chance to send to the scanner device the settings
* dictated by the current property values.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    pmdtc        - the device transfer context
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvWriteItemProperties(
    _Inout_  BYTE*                     pWiasContext,
             LONG                      lFlags,
    _In_     PMINIDRV_TRANSFER_CONTEXT pmdtc,
    _Out_    LONG*                     plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;
    LONG lItemType = 0;

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!pmdtc) || (!plDevErrVal))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;

        hr = wiasGetItemType(pWiasContext, &lItemType);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to get item type, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr) && (lItemType & WiaItemTypeRoot))
    {
        hr = E_FAIL;
        WIAEX_ERROR((g_hInst, "Acquisitions are not supported from the Root item, hr = 0x%08X", hr));
    }

    //
    // Apply to the device the scan settings described by the current WIA property configuration.
    // Note that this is not the best time to ask the scanner device to validate settings: validation
    // should be performed during IWiaMiniDrv/CWiaDriver::drvValidateItemProperties.
    //
    // ...
    //

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvWriteItemProperties 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvReadItemProperties. Reads the device item
* properties. When a client application tries to read a WIA item's properties
* the WIA service will first notify the driver by calling this method.
* The driver should then update any property values that need to be updated
* in real-time from the device every time the application attempts to read
* them (e.g. WIA_DPS_DOCUMENT_HANDLING_STATUS).
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    nPropSpec    - number of properties in pPropSpec array
*    pPropSpec    - list of properties to be read
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvReadItemProperties(
    _In_  BYTE*           pWiasContext,
          LONG            lFlags,
          ULONG           nPropSpec,
    _In_  const PROPSPEC* pPropSpec,
    _Out_ LONG*           plDevErrVal)
{
    UNREFERENCED_PARAMETER(nPropSpec);
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;
    LONG lItemFlags  = 0;

    //
    // Omitted on pupose, not really usefull without full property information:
    //
    // WIAEX_TRACE_BEGIN;
    //

    if ((!pWiasContext) || (!pPropSpec) || (!plDevErrVal))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;

        hr = wiasReadPropLong(pWiasContext, WIA_IPA_ITEM_FLAGS, &lItemFlags, NULL, TRUE);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_ITEM_FLAGS property, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // The following properties require to be updated at run-time:
        //
        // For the Root item:
        //
        // WIA_DPS_DOCUMENT_HANDLING_STATUS  (*)
        // WIA_DPA_CONNECT_STATUS (*)
        // WIA_DPS_SCAN_AVAILABLE_ITEM
        //
        // * - not updated by this sample driver since no HW device connection exists,
        //     but should be updated by a real scanner driver
        //
        // For the Flatbed and Feeder items:
        //
        // None
        //

        if (lItemFlags & WiaItemTypeRoot)
        {
            //
            // Update WIA_DPS_SCAN_AVAILABLE_ITEM with the last globally stored
            // (per driver instance) item name signaled with an unconsumed scan ready event:
            //
            hr = UpdateScanAvailableItemProperty(pWiasContext);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to update the WIA_DPS_SCAN_AVAILABLE_ITEM property (%ws), hr = 0x%08X",
                    m_bstrScanAvailableItem ? m_bstrScanAvailableItem : L"<empty string>", hr));
            }
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvReadItemProperties 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvLockWiaDevice. The IWiaMiniDrv::drvLockWiaDevice
* method locks the hardware device so that only the current minidriver can
* access it. This sample driver returns S_OK without doing anything special.
* Note that the WIA Service expects this method to succeed for a properly
* installed driver and a working scanner device. See also IStiUSD::LockDevice.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK
*
\**************************************************************************/

HRESULT CWiaDriver::drvLockWiaDevice(
   _In_     BYTE* pWiasContext,
            LONG  lFlags,
   _Out_    LONG* plDevErrVal)
{
    UNREFERENCED_PARAMETER(pWiasContext);
    UNREFERENCED_PARAMETER(lFlags);

    *plDevErrVal = 0;

    return S_OK;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvUnLockWiaDevice. The sample driver returns S_OK.
* The WIA Service expects this method to succeed for a properly installed
* driver and a working scanner device). See also IStiUSD::UnlockDevice.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK
*
\**************************************************************************/

HRESULT CWiaDriver::drvUnLockWiaDevice(
    _In_  BYTE*  pWiasContext,
          LONG   lFlags,
    _Out_ LONG*  plDevErrVal)
{
    UNREFERENCED_PARAMETER(pWiasContext);
    UNREFERENCED_PARAMETER(lFlags);

    *plDevErrVal = 0;

    return S_OK;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvAnalyzeItem. This sample driver returns
* E_NOTIMPL as it does not support image item analysis.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    E_NOTIMPL
*
\**************************************************************************/

HRESULT CWiaDriver::drvAnalyzeItem(
    _In_  BYTE*  pWiasContext,
          LONG   lFlags,
    _Out_ LONG*  plDevErrVal)
{
    UNREFERENCED_PARAMETER(pWiasContext);
    UNREFERENCED_PARAMETER(lFlags);

    WIAEX_ERROR((g_hInst, "IWiaMiniDrv::drvAnalyzeItem, this method is not implemented or supported for this driver"));

    m_hrLastEdviceError = STIERR_UNSUPPORTED;

    *plDevErrVal = 0;

    return E_NOTIMPL;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvGetDeviceErrorStr. This driver returns
* E_NOTIMPL because no localized text descriptions of the generic errors
* signaled by IWiaMiniDrv calls are available.
*
* Parameters:
*
*    lFlags           - reserved (set to 0)
*    lDevErrVal       - the device error value to be mapped to a string
*    ppszDevErrStr    - receives the address of a string describing the error
*    plDevErr         - a status code for this method
*
* Return Value:
*
*    E_NOTIMPL
*
\**************************************************************************/

HRESULT CWiaDriver::drvGetDeviceErrorStr(
    LONG            lFlags,
    LONG            lDevErrVal,
    _Out_ LPOLESTR* ppszDevErrStr,
    _Out_ LONG*     plDevErr)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(lDevErrVal);

    WIAEX_ERROR((g_hInst, "IWiaMiniDrv::drvGetDeviceErrorStr, this method is not implemented or supported for this driver"));

    if (plDevErr)
    {
        *plDevErr = WIA_ERROR_INVALID_COMMAND;
    }

    if (ppszDevErrStr)
    {
        *ppszDevErrStr = NULL;
    }

    m_hrLastEdviceError = STIERR_UNSUPPORTED;

    return E_NOTIMPL;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvUnInitializeWia. Destroys the Driver Item Tree.
*
* Parameters:
*
*    None
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::DestroyDriverItemTree()
{
    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    //
    // By design the WIA service allows an application to release the Root item at the
    // same time as the WIA service releases the WIA driver object following a device
    // disconnection. If there are no other applications connected to the driver the
    // driver would attempt to unlink and release the Root item concurrently from two
    // different threads - one thread executing IWiaMiniDrv::drvUnInitializeWia, the
    // other thread executing INonDelegating::NonDelegatingRelease:
    //
    EnterCriticalSection(&m_csDestroyDriverItemTree);

    if (m_pIDrvItemRoot)
    {
        WIAS_TRACE((g_hInst,"Unlinking WIA item tree"));

        hr = m_pIDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);
        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to unlink WIA item tree before being released, hr = 0x%08X", hr));
        }

        //
        // Proceed releasing the Root item even if the tree could not be unlinked:
        //

        WIAS_TRACE((g_hInst, "Releasing IDrvItemRoot interface"));

        __try
        {
            m_pIDrvItemRoot->Release();
        }
#pragma prefast(suppress:__WARNING_EXCEPTIONEXECUTEHANDLER, "Note that EXCEPTION_EXECUTE_HANDLER may mask exceptions that may be individually handled otherwise")
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            hr = WIA_ERROR_ITEM_DELETED;
            WIAEX_ERROR((g_hInst, "Exception 0x%08X when calling Release on the Root item, item no longer valid, hr = 0x%08X",
                GetExceptionCode(), hr));
        }

        m_pIDrvItemRoot = NULL;

        //
        // Keep the current scanner configuration data as well as the transfer format
        // information array initialized after it, until the scanner device signals
        // that the configuration has been changed or the driver is unloaded.
        //
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    LeaveCriticalSection(&m_csDestroyDriverItemTree);

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvInitializeWia. Creates the Driver Item Tree.
* Called during IWiaMiniDrv::drvInitializeWia when no Driver Item Tree exists
* and also during IWiaMiniDrv::drvDeviceCommand for WIA_CMD_SYNCHRONIZE and
* WIA_CMD_BUILD_DEVICE_TREE. Note that the scanner configuration can be read
* for the first time (in this driver session) during IStiUSD::Initialize.
*
* Parameters:
*
*    None
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::BuildDriverItemTree()
{
    HRESULT hr = S_OK;
    BSTR bstrRootItemName = NULL;
    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = NULL;

    //
    // All child items implemented by this sample driver except the Auto, Barcode, Patch Code and MICR Reader
    // items, support all item flags as the Flatbed and Feeder items do minus the WiaItemTypeImage flag. The
    // Imprinter and Endorser items support the same item flags as Flatbed and Feeder, including WiaItemTypeImage,
    // since these sample Imprinter and Endorser items support graphics data transfers.
    //
    const LONG lRootItemFlags = WiaItemTypeFolder | WiaItemTypeDevice | WiaItemTypeRoot;
    const LONG lCommonChildItemFlags = WiaItemTypeTransfer | WiaItemTypeFile | WiaItemTypeProgrammableDataSource;

    WIAEX_TRACE_BEGIN;

    //
    // The method creates the Driver Item Tree only if it doesn't exist:
    //
    if (!m_pIDrvItemRoot)
    {
        WIAS_TRACE((g_hInst, "Building Driver Item Tree...."));

        if (!m_bInitialized)
        {
            hr = E_UNEXPECTED;
            WIAEX_ERROR((g_hInst, "Driver not fully initialized, cannot create Driver Item Tree, hr = 0x%08X", hr));
        }

        //
        // Reinitialize the WIA_FORMAT_INFO arrays:
        //
        if (SUCCEEDED(hr))
        {
            hr = InitializeFormatInfoArrays();
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to initialize WIA_FORMAT_INFO array, hr = 0x%08X", hr));
            }
        }

        //
        // Create the default WIA root item. Note that we need item context data for the root item as well as children:
        //

        if (SUCCEEDED(hr))
        {
            bstrRootItemName = SysAllocString(WIA_DRIVER_ROOT_NAME);
            if (!bstrRootItemName)
            {
                hr = E_OUTOFMEMORY;
                WIAEX_ERROR((g_hInst, "Failed to allocate memory for the root item name, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = wiasCreateDrvItem(lRootItemFlags, bstrRootItemName, m_bstrRootFullItemName,
                (IWiaMiniDrv*)this, sizeof(WIA_DRIVER_ITEM_CONTEXT),
                (BYTE **)&pWiaDriverItemContext, &m_pIDrvItemRoot);
            if (SUCCEEDED(hr) && ((!pWiaDriverItemContext) || (!m_pIDrvItemRoot)))
            {
                hr = E_POINTER;
            }
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the WIA root item (hr = 0x%08X)", hr));
            }
        }

        //
        // Initialize the item context data for the root item:
        //
        if (SUCCEEDED(hr))
        {
            memset(pWiaDriverItemContext, 0, sizeof(WIA_DRIVER_ITEM_CONTEXT));

            //
            // The Root item is not using this image cache as it does not allow uploads (or download transfers):
            //
            pWiaDriverItemContext->m_pUploadedImage = NULL;
        }

        //
        // Create child items that represent programmable data sources:
        //

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_FLATBED_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                WiaItemTypeImage | lCommonChildItemFlags, WIA_CATEGORY_FLATBED, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Flatbed item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_FEEDER_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                WiaItemTypeImage | lCommonChildItemFlags, WIA_CATEGORY_FEEDER, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Feeder item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_AUTO_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                lCommonChildItemFlags, WIA_CATEGORY_AUTO, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Auto item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_IMPRINTER_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                WiaItemTypeImage | lCommonChildItemFlags, WIA_CATEGORY_IMPRINTER, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Imprinter item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_ENDORSER_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                WiaItemTypeImage | lCommonChildItemFlags, WIA_CATEGORY_ENDORSER, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Endorser item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_BARCODE_READER_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                lCommonChildItemFlags, WIA_CATEGORY_BARCODE_READER, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Barcode Reader item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_PATCH_CODE_READER_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                lCommonChildItemFlags, WIA_CATEGORY_PATCH_CODE_READER, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the Path Code Reader item, hr = 0x%08X", hr));
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = CreateWIAChildItem(WIA_DRIVER_MICR_READER_NAME, (IWiaMiniDrv*)this, m_pIDrvItemRoot,
                lCommonChildItemFlags, WIA_CATEGORY_MICR_READER, NULL);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to create the MICR Reader item, hr = 0x%08X", hr));
            }
        }

        if (bstrRootItemName)
        {
            SysFreeString(bstrRootItemName);
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE_FUNC_HR;

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvDeviceCommand. The method IWiaMiniDrv::
* drvDeviceCommand is called by the WIA service to issue a WIA service
* or application generated command to the driver. The WIA service only
* calls the IWiaMiniDrv::drvDeviceCommand method for a command that the
* driver reports to be supported during IWiaMiniDrv::drvGetCapabilities.
* This driver creates or destroys the Driver Item Tree as requested.
*
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    pguidCommand - WIA command GUID
*    ppWiaDrvItem - always set to NULL (this driver does not need to create
*                   an additional item when a WIA command is executed)
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvDeviceCommand(
    _Inout_ BYTE*         pWiasContext,
            LONG          lFlags,
    _In_    const GUID*   pguidCommand,
    _Out_   IWiaDrvItem** ppWiaDrvItem,
    _Out_   LONG*         plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    if ((!pWiasContext) || (!pguidCommand) || (!plDevErrVal))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    //
    // The following commands are supported by this driver:
    //
    // WIA_CMD_SYNCHRONIZE:        deletes and recreates the Driver Item Tree
    // WIA_CMD_DELETE_DEVICE_TREE: deletes the Driver Item Tree
    // WIA_CMD_BUILD_DEVICE_TREE:  creates the Driver Item Tree
    // WIA_CMD_START_FEEDER:       starts the scanner feeder motor, preparing for scan
    // WIA_CMD_STOP_FEEDER:        stops the scanner feeder motor
    //
    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;
        *ppWiaDrvItem = NULL;

        if (IsEqualGUID(WIA_CMD_SYNCHRONIZE, *pguidCommand))
        {
            WIAS_TRACE((g_hInst, "WIA_CMD_SYNCHRONIZE"));

            //
            // Delete the current Driver Item Tree:
            //
            hr = DestroyDriverItemTree();
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to delete the current DIT for WIA_CMD_SYNCHRONIZE, hr = 0x%08X", hr));
            }

            //
            // Re-create the Driver Item Tree according with the current device configuration:
            //
            if (SUCCEEDED(hr))
            {
                hr = BuildDriverItemTree();
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to re-create the DIT for WIA_CMD_SYNCHRONIZE, hr = 0x%08X", hr));
                }

                //
                // Queue tree updated event, regardless of whether BuildDriverItemTree succeeded,
                // since we can't guarantee that the tree was left in the same condition:
                //
                QueueWIAEvent(pWiasContext, WIA_EVENT_TREE_UPDATED);
            }
        }
        else if (IsEqualGUID(WIA_CMD_DELETE_DEVICE_TREE, *pguidCommand))
        {
            WIAS_TRACE((g_hInst, "WIA_CMD_DELETE_DEVICE_TREE"));

            if (!m_pIDrvItemRoot)
            {
                hr = E_FAIL;
                WIAEX_ERROR((g_hInst, "WIA_CMD_DELETE_DEVICE_TREE called when no DIT exists, hr = 0x%08X", hr));
            }

            if (SUCCEEDED(hr))
            {
                hr = DestroyDriverItemTree();
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to delete the current DIT for WIA_CMD_DELETE_DEVICE_TREE, hr = 0x%08X", hr));
                }
            }
        }
        else if (IsEqualGUID(WIA_CMD_BUILD_DEVICE_TREE, *pguidCommand))
        {
            WIAS_TRACE((g_hInst, "WIA_CMD_BUILD_DEVICE_TREE"));

            if (m_pIDrvItemRoot)
            {
                hr = E_FAIL;
                WIAEX_ERROR((g_hInst, "WIA_CMD_BUILD_DEVICE_TREE called when DIT already exists, hr = 0x%08X", hr));
            }

            if (SUCCEEDED(hr))
            {
                hr = BuildDriverItemTree();
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to re-create the DIT for WIA_CMD_BUILD_DEVICE_TREE, hr = 0x%08X", hr));
                }

                //
                // Queue tree updated event, regardless ofwhether BuildDriverItemTree succeeded,
                // since we can't guarantee that the tree was left in the same condition:
                //
                QueueWIAEvent(pWiasContext, WIA_EVENT_TREE_UPDATED);
            }
        }
        else if (IsEqualGUID(WIA_CMD_START_FEEDER, *pguidCommand) || IsEqualGUID(WIA_CMD_STOP_FEEDER, *pguidCommand))
        {
            GUID guidItemCategory = GUID_NULL;
            LONG lFeederMotorControl = WIA_FEEDER_CONTROL_AUTO;

            if (IsEqualGUID(WIA_CMD_START_FEEDER, *pguidCommand))
            {
                WIAS_TRACE((g_hInst, "WIA_CMD_START_FEEDER"));
            }
            else
            {
                WIAS_TRACE((g_hInst, "WIA_CMD_STOP_FEEDER"));
            }

            //
            // The feeder motor commands are valid only on the Feeder item:
            //
            hr = wiasReadPropGuid(pWiasContext, WIA_IPA_ITEM_CATEGORY, &guidItemCategory, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Failed to read WIA_IPA_ITEM_CATEGORY, hr = 0x%08X", hr));
            }

            if (SUCCEEDED(hr) && (!IsEqualGUID(guidItemCategory, WIA_CATEGORY_FEEDER)))
            {
                hr = WIA_ERROR_INVALID_COMMAND;
                WIAEX_ERROR((g_hInst, "WIA_CMD_START/STOP_FEEDER commands are valid only on the Feeder item, hr = 0x%08X", hr));
            }

            //
            // WIA_IPS_FEEDER_CONTROL must be set to WIA_FEEDER_CONTROL_MANUAL:
            //
            if (SUCCEEDED(hr))
            {
                hr = wiasReadPropLong(pWiasContext, WIA_IPS_FEEDER_CONTROL, &lFeederMotorControl, NULL, TRUE);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to read WIA_IPS_FEEDER_CONTROL property, hr = 0x%08X", hr));
                }

                if (SUCCEEDED(hr) && (WIA_FEEDER_CONTROL_MANUAL != lFeederMotorControl))
                {
                    hr = WIA_ERROR_INVALID_COMMAND;
                    WIAEX_ERROR((g_hInst, "WIA_CMD_START/STOP_FEEDER commands are valid only when WIA_IPS_FEEDER_CONTROL is set to WIA_FEEDER_CONTROL_MANUAL, hr = 0x%08X", hr));
                }
            }

            if (SUCCEEDED(hr))
            {
                if (IsEqualGUID(WIA_CMD_START_FEEDER, *pguidCommand))
                {
                    hr = StartFeeder();
                }
                else
                {
                    hr = StopFeeder();
                }
            }
        }
        else
        {
            hr = E_NOTIMPL;
            WIAEX_ERROR((g_hInst, "The requested WIA command is not implemented or supported by this driver"));
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvDeviceCommand 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvGetCapabilities. The WIA service calls
* IWiaMiniDrv::drvGetCapabilities to obtain a list of hardware command
* capabilities and/or STI/WIA device events supported by the driver.
*
* Parameters:
*
*    pWiasContext   - pointer to the item context (may be NULL)
*    lFlags         - reserved (set to 0)
*    pcelt          - receives the number of elements in the array
*                     pointed to by the ppCapabilities parameter
*    ppCapabilities - receives the address of the first element
*                     of an array of WIA_DEV_CAP_DRV structures that
*                     contain the GUIDs of events and commands that
*                     the device supports
*    plDevErrVal    - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if successful, an error HRESULT otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvGetCapabilities(
   _In_opt_  BYTE*             pWiasContext,
             LONG              ulFlags,
   _Out_     LONG*             pcelt,
   _Out_     WIA_DEV_CAP_DRV** ppCapabilities,
   _Out_     LONG*             plDevErrVal)
{
    UNREFERENCED_PARAMETER(pWiasContext);
    UNREFERENCED_PARAMETER(ulFlags);

    HRESULT hr = S_OK;
    BOOL bAddCapabilities = FALSE;
    BOOL bGetCommands = FALSE;
    BOOL bGetEvents = FALSE;

    WIAEX_TRACE_BEGIN;

    //
    // Note that pWiasContext may be NULL when the driver signals an event
    // before the Driver Item Tree is created and WIA service makes this call.
    // It is also unused so we won't verify it:
    //
    if ((!pcelt) || (!ppCapabilities) || (!plDevErrVal))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;
        *pcelt = 0;
        *ppCapabilities = NULL;

        bAddCapabilities = (BOOL)(!m_tCapabilityManager.GetNumCapabilities());
    }

    //
    // Add WIA_EVENT_DEVICE_CONNECTED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_DEVICE_CONNECTED, IDS_EVENT_DEVICE_CONNECTED_NAME, IDS_EVENT_DEVICE_CONNECTED_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_DEVICE_CONNECTED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_DEVICE_CONNECTED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_DEVICE_DISCONNECTED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_DEVICE_DISCONNECTED, IDS_EVENT_DEVICE_DISCONNECTED_NAME,
            IDS_EVENT_DEVICE_DISCONNECTED_DESCRIPTION, WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_DEVICE_DISCONNECTED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_DEVICE_DISCONNECTED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_POWER_SUSPEND:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_POWER_SUSPEND, IDS_EVENT_POWER_SUSPEND_NAME, IDS_EVENT_POWER_SUSPEND_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_DEVICE_DISCONNECTED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_POWER_SUSPEND to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_POWER_RESUME:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_POWER_RESUME, IDS_EVENT_POWER_RESUME_NAME, IDS_EVENT_POWER_RESUME_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_DEVICE_CONNECTED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_POWER_RESUME to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_TREE_UPDATED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_TREE_UPDATED, IDS_EVENT_TREE_UPDATED_NAME,
            IDS_EVENT_TREE_UPDATED_DESCRIPTION, WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_TREE_UPDATED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_TREE_UPDATED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // The sample driver does not signal at run time any of the events initialized below:
    //
    // WIA_EVENT_SCAN_IMAGE
    // WIA_EVENT_DEVICE_NOT_READY
    // WIA_EVENT_DEVICE_READY
    // WIA_EVENT_FLATBED_LID_OPEN
    // WIA_EVENT_FLATBED_LID_CLOSED
    // WIA_EVENT_FEEDER_LOADED
    // WIA_EVENT_FEEDER_EMPTIED
    // WIA_EVENT_COVER_OPEN
    // WIA_EVENT_COVER_CLOSED
    //
    // To signal one of these events, set the m_hWiaEvent and then WIA will issue a IStiUSD::GetNotificationData
    // to receive the GUID of the particular event that is signaled:
    //
    //  if (!SetEvent(m_hWiaEvent))
    //  {
    //      dwErr = ::GetLastError();
    //      hr = HRESULT_FROM_WIN32(dwErr);
    //      if (SUCCEEDED(hr))
    //      {
    //          hr = E_FAIL;
    //      }
    //      WIAEX_ERROR((g_hInst, "SetEvent(WiaEvent) failed (0x%08X), hr = 0x%08X", dwErr, hr));
    //  }
    //
    //

    //
    // Add WIA_EVENT_SCAN_IMAGE:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_SCAN_IMAGE, IDS_EVENT_SCAN_IMAGE_NAME, IDS_EVENT_SCAN_IMAGE_DESCRIPTION,
            WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT, (LPCWSTR)WIA_ICON_SCAN_BUTTON_PRESS);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_SCAN_IMAGE to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_DEVICE_NOT_READY:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_DEVICE_NOT_READY, IDS_EVENT_DEVICE_NOT_READY_NAME, IDS_EVENT_DEVICE_NOT_READY_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_DEVICE_NOT_READY);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_DEVICE_NOT_READY to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_DEVICE_READY:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_DEVICE_READY, IDS_EVENT_DEVICE_READY_NAME, IDS_EVENT_DEVICE_READY_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_DEVICE_READY);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_DEVICE_READY to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_FLATBED_LID_OPEN:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_FLATBED_LID_OPEN, IDS_EVENT_FLATBED_LID_OPEN_NAME, IDS_EVENT_FLATBED_LID_OPEN_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_FLATBED_LID_OPEN);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_FLATBED_LID_OPEN to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_FLATBED_LID_CLOSED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_FLATBED_LID_CLOSED, IDS_EVENT_FLATBED_LID_CLOSED_NAME, IDS_EVENT_FLATBED_LID_CLOSED_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_FLATBED_LID_CLOSED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_FLATBED_LID_CLOSED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_FEEDER_LOADED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_FEEDER_LOADED, IDS_EVENT_FEEDER_LOADED_NAME, IDS_EVENT_FEEDER_LOADED_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_FEEDER_LOADED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_FEEDER_LOADED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_FEEDER_EMPTIED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_FEEDER_EMPTIED, IDS_EVENT_FEEDER_EMPTIED_NAME, IDS_EVENT_FEEDER_EMPTIED_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_FEEDER_EMPTIED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_FEEDER_EMPTIED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_COVER_OPEN:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_COVER_OPEN, IDS_EVENT_COVER_OPEN_NAME, IDS_EVENT_COVER_OPEN_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_COVER_OPEN);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_COVER_OPEN to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_EVENT_COVER_CLOSED:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_EVENT_COVER_CLOSED, IDS_EVENT_COVER_CLOSED_NAME, IDS_EVENT_COVER_CLOSED_DESCRIPTION,
            WIA_NOTIFICATION_EVENT, (LPCWSTR)WIA_ICON_COVER_CLOSED);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_EVENT_COVER_CLOSED to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_CMD_SYNCRONIZE:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_CMD_SYNCHRONIZE, IDS_CMD_SYNCHRONIZE_NAME,
            IDS_CMD_SYNCHRONIZE_DESCRIPTION, 0, (LPCWSTR)WIA_ICON_SYNCHRONIZE);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_CMD_SYNCHRONIZE to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_CMD_DELETE_DEVICE_TREE:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_CMD_DELETE_DEVICE_TREE, IDS_CMD_DELETE_DEVICE_TREE_NAME,
            IDS_CMD_DELETE_DEVICE_TREE_DESCRIPTION, 0, (LPCWSTR)WIA_ICON_DELETE_DEVICE_TREE);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_CMD_DELETE_DEVICE_TREE to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_CMD_BUILD_DEVICE_TREE:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_CMD_BUILD_DEVICE_TREE, IDS_CMD_BUILD_DEVICE_TREE_NAME,
            IDS_CMD_BUILD_DEVICE_TREE_DESCRIPTION, 0, (LPCWSTR)WIA_ICON_BUILD_DEVICE_TREE);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_CMD_BUILD_DEVICE_TREE to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_CMD_START_FEEDER:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_CMD_START_FEEDER, IDS_CMD_START_FEEDER_NAME,
            IDS_CMD_START_FEEDER_DESCRIPTION, 0, (LPCWSTR)WIA_ICON_START_FEEDER);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_CMD_START_FEEDER to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    //
    // Add WIA_CMD_STOP_FEEDER:
    //
    if (SUCCEEDED(hr) && bAddCapabilities)
    {
        hr = m_tCapabilityManager.AddCapability(WIA_CMD_STOP_FEEDER, IDS_CMD_STOP_FEEDER_NAME,
            IDS_CMD_STOP_FEEDER_DESCRIPTION, 0, (LPCWSTR)WIA_ICON_STOP_FEEDER);

        if (FAILED(hr))
        {
            WIAEX_ERROR((g_hInst, "Failed to add WIA_CMD_STOP_FEEDER to the list of capabilities, hr = 0x%08X", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        bGetCommands = (BOOL)(WIA_DEVICE_COMMANDS == (ulFlags & WIA_DEVICE_COMMANDS));
        bGetEvents = (BOOL)(WIA_DEVICE_EVENTS == (ulFlags & WIA_DEVICE_EVENTS));

        if ((bGetCommands) && (bGetEvents))
        {
            *ppCapabilities = m_tCapabilityManager.GetCapabilities();
            *pcelt = m_tCapabilityManager.GetNumCapabilities();
            WIAS_TRACE((g_hInst, "drvGetCapabilities, application is asking for Commands and Events, we have %d total capabilities", *pcelt));
        }
        else if (bGetCommands)
        {
            *ppCapabilities = m_tCapabilityManager.GetCommands();
            *pcelt = m_tCapabilityManager.GetNumCommands();
            WIAS_TRACE((g_hInst, "drvGetCapabilities, application is asking for Commands, we have %d", *pcelt));
        }
        else if (bGetEvents)
        {
            *ppCapabilities = m_tCapabilityManager.GetEvents();
            *pcelt = m_tCapabilityManager.GetNumEvents();
            WIAS_TRACE((g_hInst,"drvGetCapabilities, application is asking for Events, we have %d", *pcelt));
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvGetCapabilities 0x%08X (%u events, %u commands)",
        hr, m_tCapabilityManager.GetNumEvents(), m_tCapabilityManager.GetNumCommands()));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvDeleteItem. Deletes a driver item. The items
* supported by this sample driver cannot be deleted by the application.
*
* Parameters:
*
*    pWiasContext - pointer to the item context
*    lFlags       - reserved (set to 0)
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    STG_E_ACCESSDENIED to signal access denied (items cannot be deleted)
*
\**************************************************************************/

HRESULT CWiaDriver::drvDeleteItem(
    _Inout_ BYTE* pWiasContext,
            LONG  lFlags,
    _Out_   LONG* plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(pWiasContext);

    HRESULT hr = STG_E_ACCESSDENIED;

    WIAEX_ERROR((g_hInst, "This item cannot be deleted, hr = 0x%08X", hr));

    m_hrLastEdviceError = hr;

    *plDevErrVal = 0;

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvDeleteItem 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvFreeDrvItemContext. When a driver item is
* deleted, the WIA service frees the driver item context. This method
* informs the driver that the context is ready to be freed. The driver
* must free any memory allocated for the given driver item context.
* The driver won’t support items that can be deleted by the application
* but this method is going to be called by the WIA service the entire
* item tree is released at the end of a session.
*
* Parameters:
*
*    lFlags       - reserved (set to 0)
*    pSpecContext - points to a device-specific context
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK
*
\**************************************************************************/

HRESULT CWiaDriver::drvFreeDrvItemContext(
          LONG    lFlags,
    _In_reads_bytes_(sizeof(WIA_DRIVER_ITEM_CONTEXT))
          BYTE    *pSpecContext,
    _Out_ LONG    *plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    *plDevErrVal = 0;

    WIA_DRIVER_ITEM_CONTEXT *pWiaDriverItemContext = (WIA_DRIVER_ITEM_CONTEXT *)pSpecContext;

    if (pWiaDriverItemContext && pWiaDriverItemContext->m_pUploadedImage)
    {
        pWiaDriverItemContext->m_pUploadedImage->Release();
        pWiaDriverItemContext->m_pUploadedImage = NULL;
    }

    WIAS_TRACE((g_hInst, "IWiaMiniDrv::drvFreeDrvItemContext 0x%08X", S_OK));

    //
    // This method must not fail
    //

    return S_OK;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvGetWiaFormatInfo. This method creates an array
* of WIA_FORMAT_INFO structures that describe the media types and image
* formats that the driver supports.
*
* Parameters:
*
*    pWiasContext - points to a device-specific context
*    lFlags       - reserved (set to 0)
*    pcelt        - receives the number of items in the ppwfi array
*    ppwfi        - array of WIA_FORMAT_INFO structures filled on return
*    plDevErrVal  - always set to 0 by this driver (drvGetDeviceErrorStr unsupported)
*
* Return Value:
*
*    S_OK if succeeds, a standard COM error code otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvGetWiaFormatInfo(
    _In_  BYTE*             pWiasContext,
          LONG              lFlags,
    _Out_ LONG*             pcelt,
    _Out_ WIA_FORMAT_INFO** ppwfi,
    _Out_ LONG*             plDevErrVal)
{
    UNREFERENCED_PARAMETER(lFlags);

    HRESULT hr = S_OK;
    GUID guidItemCategory = WIA_CATEGORY_ROOT;
    LONG lNumFormats = 0;
    WIA_FORMAT_INFO *pFormatInfo = NULL;

    WIAEX_TRACE_BEGIN;

    if ((!plDevErrVal) || (!pcelt) || (!ppwfi))
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        *plDevErrVal = 0;
    }

    if (pWiasContext)
    {
        if (SUCCEEDED(hr))
        {
            hr = wiasReadPropGuid(pWiasContext, WIA_IPA_ITEM_CATEGORY, &guidItemCategory, NULL, TRUE);
            if (FAILED(hr))
            {
                WIAEX_ERROR((g_hInst, "Error reading current WIA_IPA_ITEM_CATEGORY, hr = 0x%08X", hr));
            }
        }

        //
        // This method is allowed to be called only on a transfer capable child item:
        //
        if (SUCCEEDED(hr))
        {
            if (IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Feeder..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_FLATBED, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Flatbed..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Auto image source..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Imprinter..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_ENDORSER, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Endorser..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_BARCODE_READER, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Barcode Reader..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_PATCH_CODE_READER, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the Patch Code Reader..", hr));
            }
            else if (IsEqualGUID(WIA_CATEGORY_MICR_READER, guidItemCategory))
            {
                WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for the MICR Reader..", hr));
            }
            else
            {
                if (IsEqualGUID(WIA_CATEGORY_ROOT, guidItemCategory))
                {
                    WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for Root..", hr));
                }
                else
                {
                    WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for an unknown item, assuming Root..", hr));
                }
                guidItemCategory = WIA_CATEGORY_ROOT;
            }
        }
    }
    else
    {
        WIAS_TRACE((g_hInst, "drvGetWiaFormatInfo called for a NULL item context, assuming Root..", hr));
        guidItemCategory = WIA_CATEGORY_ROOT;
    }

    //
    // Check the number of available formats
    //
    if (SUCCEEDED(hr))
    {
        //
        // Note that the format info arrays are initialized on IStiUSD::Initialize
        // so they may be available even before the Driver Item Tree is created:
        //
        if (IsEqualGUID(WIA_CATEGORY_FEEDER, guidItemCategory) ||
            IsEqualGUID(WIA_CATEGORY_FLATBED, guidItemCategory) ||
            IsEqualGUID(WIA_CATEGORY_AUTO, guidItemCategory) ||
            IsEqualGUID(WIA_CATEGORY_ROOT, guidItemCategory))
        {
            lNumFormats = m_tFormatInfo.Size();
            pFormatInfo = (WIA_FORMAT_INFO *)m_tFormatInfo.Array();
        }
        else if (IsEqualGUID(WIA_CATEGORY_IMPRINTER, guidItemCategory) ||
            IsEqualGUID(WIA_CATEGORY_ENDORSER, guidItemCategory))
        {
            lNumFormats = m_tFormatInfoImprinterEndorser.Size();
            pFormatInfo = (WIA_FORMAT_INFO *)m_tFormatInfoImprinterEndorser.Array();
        }
        else if (IsEqualGUID(WIA_CATEGORY_BARCODE_READER, guidItemCategory))
        {
            lNumFormats = m_tFormatInfoBarcodeReader.Size();
            pFormatInfo = (WIA_FORMAT_INFO *)m_tFormatInfoBarcodeReader.Array();
        }
        else if (IsEqualGUID(WIA_CATEGORY_PATCH_CODE_READER, guidItemCategory))
        {
            lNumFormats = m_tFormatInfoPatchCodeReader.Size();
            pFormatInfo = (WIA_FORMAT_INFO *)m_tFormatInfoPatchCodeReader.Array();
        }
        else if (IsEqualGUID(WIA_CATEGORY_MICR_READER, guidItemCategory))
        {
            lNumFormats = m_tFormatInfoMicrReader.Size();
            pFormatInfo = (WIA_FORMAT_INFO *)m_tFormatInfoMicrReader.Array();
        }

        if ((!lNumFormats) || (!pFormatInfo))
        {
            hr = E_FAIL;
            WIAEX_ERROR((g_hInst, "Unexpected, the format array must be initialized first, hr = 0x%08X", hr));
        }
    }

    //
    // Return the requested list:
    //
    if (SUCCEEDED(hr))
    {
        *pcelt = lNumFormats;
        *ppwfi = pFormatInfo;
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvGetWiaFormatInfo 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvNotifyPnpEvent. The WIA service notifies the
* driver of a supported PnP system event by calling this method.
* A real driver could respond to these events for example by releasing and
* redoing its connection with the Hardware device (and possibly signaling
* to a locally connected device that it can enter or resume power saving itself)
* when the computer goes into and exists stand-by and hibernation.
*
* The driver must check the pEventGUID parameter to determine what event
* is being processed. The events that are processed by the driver are:
*
* WIA_EVENT_POWER_SUSPEND       - system is going to suspend/sleep mode
* WIA_EVENT_POWER_RESUME        - system is waking up from suspend/sleep mode
* WIA_EVENT_DEVICE_DISCONNECTED - device is disconnected, driver will be unloaded
* WIA_EVENT_CANCEL_IO           - pending IO is being cancelled
*
* The driver does not execute any special action on the following events:
*
* WIA_EVENT_DEVICE_CONNECTED (*)- driver initialization is being done on IStiUSD::Initialize
*
* (* - The driver logs a trace message following WIA_EVENT_DEVICE_CONNECTED
*      and ensures the current device connection state is correctly recorded)
*
* Parameters:
*
*    pEventGUI    - GUID identifying the event
*    bstrDeviceID - string containing the device's unique identifier
*    ulReserved   - reserved for system use
*
* Return Value:
*
*    S_OK if succeeds, a standard COM error code otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvNotifyPnpEvent(
    _In_ const GUID* pEventGUID,
    _In_ BSTR        bstrDeviceID,
         ULONG       ulReserved)
{
    UNREFERENCED_PARAMETER(ulReserved);

    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    if (!pEventGUID)
    {
        hr = E_INVALIDARG;
        WIAEX_ERROR((g_hInst, "Invalid parameter, hr = 0x%08X", hr));
    }

    if (SUCCEEDED(hr))
    {
        if (bstrDeviceID)
        {
            WIAS_TRACE((g_hInst, "PnP event notification received for device ID %ws", bstrDeviceID));
        }

        if (WIA_EVENT_POWER_SUSPEND == *pEventGUID)
        {
            WIAS_TRACE((g_hInst, "WIA_EVENT_POWER_SUSPEND"));

            //
            // Disable WIA events:
            //
            SetNotificationHandle(NULL);
        }
        else if (WIA_EVENT_POWER_RESUME == *pEventGUID)
        {
            WIAS_TRACE((g_hInst, "WIA_EVENT_POWER_RESUME"));

            if ((!m_hWiaEventStoredCopy) || (INVALID_HANDLE_VALUE == m_hWiaEventStoredCopy))
            {
                hr = E_UNEXPECTED;
                WIAEX_ERROR((g_hInst, "Failed to re-enable WIA events for WIA_EVENT_POWER_RESUME, invalid event handle, hr = 0x%08X", hr));
            }

            if (SUCCEEDED(hr))
            {
                //
                // Re-enable WIA events:
                //
                hr = SetNotificationHandle(m_hWiaEventStoredCopy);
                if (FAILED(hr))
                {
                    WIAEX_ERROR((g_hInst, "Failed to re-enable WIA events on WIA_EVENT_POWER_RESUME, hr = 0x%08X", hr));
                }
            }
        }
        else if (WIA_EVENT_DEVICE_CONNECTED == *pEventGUID)
        {
            WIAS_TRACE((g_hInst, "WIA_EVENT_DEVICE_CONNECTED"));

            //
            // When the driver receives this event the driver is already loaded and initialized
            // and a new driver object is already created. Ensure the correct connection state
            // is recorded  - note that this is not required  as when the driver is reloaded
            // SetDeviceConnected(TRUE) is already executed from within the CWiaDriver
            // constructor when the new driver object instance is created.
            //
        }
        else if (WIA_EVENT_DEVICE_DISCONNECTED == *pEventGUID)
        {
            WIAS_TRACE((g_hInst, "WIA_EVENT_DEVICE_DISCONNECTED"));

            //
            // The driver is notified that the scanner device is disconnected and that the WIA
            // service will soon unload the driver. The device communication interface will be
            // uninitialized when the driver will be unloaded. The SetDeviceConnected(FALSE) call
            // below will record the current state so when the device communication interface will
            // be uninitialized (when the driver object will be released by the WIA service before
            // unloading the driver) the driver won't log excessive errors to the WIA trace log
            // when device  operation requests will fail because the device won't be available):
            //

            //
            // Disable WIA events:
            //
            SetNotificationHandle(NULL);
        }
        else if (WIA_EVENT_CANCEL_IO == *pEventGUID)
        {
            WIAS_TRACE((g_hInst, "WIA_EVENT_CANCEL_IO"));
        }
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvNotifyPnpEvent 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Implements IWiaMiniDrv::drvUnInitializeWia. The WIA service calls the
* IWiaMiniDrv::drvUnInitializeWia method when the resources associated
* with an application item tree are no longer needed. The driver must
* free all resources allocated for this application session.
*
* Parameters:
*
*    pWiasContext - points to a device-specific context
*
* Return Value:
*
*    S_OK if succeeds, a standard COM error code otherwise
*
\**************************************************************************/

HRESULT CWiaDriver::drvUnInitializeWia(
    _Inout_ BYTE* pWiasContext)
{
    HRESULT hr = S_OK;

    WIAEX_TRACE_BEGIN;

    //
    // Watch out to not prematurely fail this request... we must
    // succeed freeing resources when this is called no matter what.
    //

    if (InterlockedDecrement(&m_lClientsConnected) < 0)
    {
        WIAS_TRACE((g_hInst,
            "drvUnInitializeWia, the client connection counter decremented below zero. Assuming no clients are currently connected and automatically setting to 0"));
        m_lClientsConnected = 0;
    }

    WIAS_TRACE((g_hInst, "drvUnInitializeWia, %d client(s) are currently connected to this driver",  m_lClientsConnected));

    if (!m_lClientsConnected)
    {
        //
        // When the last client disconnects, destroy the WIA item tree.
        // This should reduce the idle memory foot print of this driver:
        //
        DestroyDriverItemTree();
    }

    if (!pWiasContext)
    {
        WIAEX_ERROR((g_hInst, "drvUnInitializeWia called with NULL item context parameter (!!!); still, request completed, hr = 0x%08X", hr));
    }

    if (FAILED(hr))
    {
        m_hrLastEdviceError = hr;
    }

    WIAEX_TRACE((g_hInst, "IWiaMiniDrv::drvUnInitializeWia 0x%08X", hr));

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvDeviceCommand.Starts the scanner feeder.
*
* Parameters:
*
*    None
*
* Return Value:
*
*    S_OK if successful or an error HRESULT otherwise
*
\**************************************************************************/

HRESULT
CWiaDriver::StartFeeder()
{
    HRESULT hr = S_OK;

    if (!m_bFeederStarted)
    {
        //
        // Start feeder
        //
        // ...
        //

        m_bFeederStarted = TRUE;
        WIAS_TRACE((g_hInst, "Feeder started"));
    }

    return hr;
}

/**************************************************************************\
*
* Helper for CWiaDriver::drvDeviceCommand.Stops the scanner feeder.
*
* Parameters:
*
*    None
*
* Return Value:
*
*    S_OK if successful or an error HRESULT otherwise
*
\**************************************************************************/

HRESULT
CWiaDriver::StopFeeder()
{
    HRESULT hr = S_OK;

    if (m_bFeederStarted)
    {
        //
        // Stop feeder
        //
        // ...
        //

        m_bFeederStarted = FALSE;
        WIAS_TRACE((g_hInst, "Feeder stopped"));
    }

    return hr;
}
