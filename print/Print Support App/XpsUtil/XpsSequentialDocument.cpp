#include <chrono>
#include "SynchronizedSequentialStream.h"
#include "pch.h"
#include "XpsPageWatermarker.h"
#include "XpsSequentialDocument.h"
#include "XpsSequentialDocument.g.cpp"
#include <winrt/Windows.Storage.Streams.h>
#include <shlwapi.h>
#include <shcore.h>
#include <XpsRasSvc.h>
#include <XpsPageWrapper.h>

using namespace winrt::Windows::Storage::Streams;

namespace winrt::XpsUtil::implementation
{
    /// <summary>
    /// Creates a new XpsSequentialDocument that will receive a document from `sourceFileContent`
    /// </summary>
    /// <param name="sourceFileContent">The object to receive an XPS document from</param>
    XpsSequentialDocument::XpsSequentialDocument(Windows::Graphics::Printing::Workflow::PrintWorkflowObjectModelSourceFileContent const& sourceFileContent)
    {
        m_sourceFileContent = sourceFileContent.as<IPrintWorkflowObjectModelSourceFileContentNative>();

        // Create a stream and an XPS object factory. The m_outputStream will be returned in ToStream(), and will
        // contain the contents of the created document as a stream.
        m_xpsFactory = create_instance<IXpsOMObjectFactory1>(guid_of<XpsOMObjectFactory>());
    }

    /// <summary>
    /// Begin reading in the page from the given source file, applying watermark as each page is read if watermarker is supplied
    /// </summary>
    void XpsSequentialDocument::StartXpsOMGeneration()
    {
        m_sourceFileContent->StartXpsOMGeneration(this);
    }

    /// <summary>
    /// Gets a stream representing an XPS document, where each page will be watermarked with the given watermarker. This
    /// sets the watermark and then starts generation.
    /// As each page is read, it will be watermarked using `watermarker`.
    ///
    /// These changes will be applied to each page, and are non-reversible. If you want to make a temporary change to a
    /// page, e.g. for a preview, you should instead pass an XpsPageWrapper from GetPage().Clone() to your watermarker.
    /// </summary>
    /// <param name="sourceFileContent">The object to receive an XPS document from</param>
    /// <param name="watermarker"></param>
    Windows::Storage::Streams::IInputStream XpsSequentialDocument::GetWatermarkedStream(XpsUtil::XpsPageWatermarker const& watermarker)
    {
        m_outputStream = winrt::make_self<winrt::XpsUtil::implementation::SynchronizedSequentialStream>();

        m_watermarker = watermarker.as<XpsPageWatermarker>();

        m_sourceFileContent->StartXpsOMGeneration(this);

        return m_outputStream.as<IInputStream>();
    }

    /// <summary>
    /// Callback for when a page is added to the document. Note that the returned page number is 1-indexed.
    /// </summary>
    /// <param name="handler"></param>
    /// <returns></returns>
    winrt::event_token XpsSequentialDocument::PageAdded(Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint32_t> const& handler)
    {
        return m_pageAddedEventHandler.add(handler);
    }

    void XpsSequentialDocument::PageAdded(winrt::event_token const& token) noexcept
    {
        m_pageAddedEventHandler.remove(token);
    }

    /// <summary>
    /// Callback invoked when the document is done reading from the source and is completely ready for use.
    /// </summary>
    /// <param name="handler"></param>
    /// <returns></returns>
    winrt::event_token XpsSequentialDocument::DocumentClosed(Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint32_t> const& handler)
    {
        return m_documentClosedEventHandler.add(handler);
    }

    void XpsSequentialDocument::DocumentClosed(winrt::event_token const& token) noexcept
    {
        m_documentClosedEventHandler.remove(token);
    }

    /// <summary>
    /// Callback invoked when the document cannot be read for an unknown reason. This document should be treated as invalid.
    /// </summary>
    /// <param name="handler">
    /// Handler that receives the document for which generation failed, and a 64-bit integer representing the underlying error code
    /// </param>
    /// <returns></returns>
    winrt::event_token XpsSequentialDocument::XpsGenerationFailed(Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint64_t> const& handler)
    {
        return m_xpsGenerationFailedEventHandler.add(handler);
    }

    void XpsSequentialDocument::XpsGenerationFailed(winrt::event_token const& token) noexcept
    {
        m_xpsGenerationFailedEventHandler.remove(token);
    }

    // Start of IPrintWorkflowXpsReceiver interface methods
    HRESULT __stdcall XpsSequentialDocument::SetDocumentSequencePrintTicket(IStream* documentSequencePrintTicket) noexcept
    {
        return S_OK;
    }
    HRESULT __stdcall XpsSequentialDocument::SetDocumentSequenceUri(PCWSTR documentSequenceUri) noexcept try 
    {
        if (m_outputStream != nullptr)
        {
            com_ptr<IOpcPartUri> documentSequencePartName;
            check_hresult(m_xpsFactory->CreatePartUri(documentSequenceUri, documentSequencePartName.put()));

            com_ptr<IOpcPartUri> spDiscardControlPartName;
            check_hresult(m_xpsFactory->CreatePartUri(L"/DiscardControl.xml", spDiscardControlPartName.put()));

            check_hresult(m_xpsFactory->CreatePackageWriterOnStream(m_outputStream.as<ISequentialStream>().get(), true, XPS_INTERLEAVING_ON, documentSequencePartName.get(), NULL, NULL, NULL, spDiscardControlPartName.get(), m_xpsOMPackageWriter.put()));
        }

        return S_OK;
    } catch (...) { return winrt::to_hresult(); }

    HRESULT __stdcall XpsSequentialDocument::AddDocumentData(UINT32 documentId, IStream* documentPrintTicket, PCWSTR documentUri) noexcept try
    {
        if (m_xpsOMPackageWriter != nullptr)
        {
            com_ptr<IOpcPartUri> documentPartUri;
            check_hresult(m_xpsFactory->CreatePartUri(documentUri, documentPartUri.put()));

            // Start writing new document (as soon as received) 
            check_hresult(m_xpsOMPackageWriter->StartNewDocument(documentPartUri.get(), nullptr, nullptr, nullptr, nullptr));
        }

        return S_OK;
    } catch (...) { return winrt::to_hresult(); }

    HRESULT __stdcall XpsSequentialDocument::AddPage(UINT32 documentId, UINT32 pageId, IXpsOMPageReference* pageReference, PCWSTR pageUri) noexcept try
    {
        // Get the page and its dimensions
        com_ptr<IXpsOMPage> xpsOMPage;
        check_hresult(pageReference->GetPage(xpsOMPage.put()));

        if (m_xpsOMPackageWriter != nullptr)
        {
            XPS_SIZE pageDimensions { };
            check_hresult(xpsOMPage->GetPageDimensions(&pageDimensions));

            com_ptr<IOpcPartUri> opcPagePartUri;
            check_hresult(m_xpsFactory->CreatePartUri(pageUri, opcPagePartUri.put()));

            if (m_watermarker)
            {
                m_watermarker->ApplyWatermarksToXpsPage(xpsOMPage);
            }

            // Finished modifying, so add page to package
            check_hresult(m_xpsOMPackageWriter->AddPage(xpsOMPage.get(), &pageDimensions, nullptr, nullptr, nullptr, nullptr));
        }

        m_pages.push_back(xpsOMPage);

        try
        {
            m_pageAddedEventHandler(*this, pageId);
        }
        // Ignore any exceptions the event handler throws
        catch (...) {}

        return S_OK;
    } catch (...) { return winrt::to_hresult(); }

    HRESULT __stdcall XpsSequentialDocument::Close() noexcept try
    {

        if (m_xpsOMPackageWriter != nullptr)
        {
            check_hresult(m_xpsOMPackageWriter->Close());
        }

        if (m_outputStream != nullptr) {
            m_outputStream->Close();
        }

        try
        {
            m_documentClosedEventHandler(*this, PageCount());
        }
        // Ignore any exceptions the event handler throws
        catch (...) { }

        return S_OK;
    } catch (...) { return winrt::to_hresult(); }

    HRESULT __stdcall XpsSequentialDocument::Failed(HRESULT xpsError) noexcept try
    {
        if (m_outputStream != nullptr) {
            m_outputStream->Close();
        }

        try
        {
            m_xpsGenerationFailedEventHandler(*this, xpsError);
        }
        // Ignore any exceptions the event handler throws
        catch (...) {}

        return S_OK;
    }
    catch (...) { return winrt::to_hresult(); }
    // End of IPrintWorkflowXpsReceiver interface methods

    /// <summary>
    /// Get the current total number of pages in the document
    /// </summary>
    /// <returns>Current page count</returns>
    uint32_t XpsSequentialDocument::PageCount()
    {
        return (uint32_t)m_pages.size();
    }

    /// <summary>
    /// Get page at index. Note that the index is **1-indexed**: the first page is retrieved with GetPage(1)
    /// </summary>
    /// <param name="pageNumber">1-indexed requested page number</param>
    /// <returns></returns>
    winrt::XpsUtil::XpsPageWrapper XpsSequentialDocument::GetPage(UINT32 pageNumber)
    {
        return make<winrt::XpsUtil::implementation::XpsPageWrapper>(m_pages.at(pageNumber - 1));
    }
}
