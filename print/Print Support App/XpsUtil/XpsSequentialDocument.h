#pragma once
#include "XpsSequentialDocument.g.h"
#include <winrt/Windows.Graphics.Printing.Workflow.h>
#include "SynchronizedSequentialStream.h"
#include <queue>
#include <chrono>
#include <functional>

namespace winrt::XpsUtil::implementation
{
    struct XpsSequentialDocument : XpsSequentialDocumentT<XpsSequentialDocument, IPrintWorkflowXpsReceiver2>
    {
        XpsSequentialDocument() = default;
        XpsSequentialDocument(Windows::Graphics::Printing::Workflow::PrintWorkflowObjectModelSourceFileContent const& sourceFileContent);

        void StartXpsOMGeneration();
        Windows::Storage::Streams::IInputStream XpsSequentialDocument::GetWatermarkedStream(XpsUtil::XpsPageWatermarker const& watermarker);

        winrt::event_token PageAdded(Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint32_t> const& handler);
        void PageAdded(winrt::event_token const& token) noexcept;

        winrt::event_token DocumentClosed(Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint32_t> const& handler);
        void DocumentClosed(winrt::event_token const& token) noexcept;

        winrt::event_token XpsGenerationFailed(Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint64_t> const& handler);
        void XpsGenerationFailed(winrt::event_token const& token) noexcept;

        // Start of IPrintWorkflowXpsReceiver interface methods
        HRESULT STDMETHODCALLTYPE SetDocumentSequencePrintTicket(_In_opt_ IStream* documentSequencePrintTicket) noexcept;
        HRESULT STDMETHODCALLTYPE SetDocumentSequenceUri(_In_ PCWSTR documentSequenceUri) noexcept;
        HRESULT STDMETHODCALLTYPE AddDocumentData(_In_ UINT32 documentId, _In_opt_ IStream* documentPrintTicket, _In_ PCWSTR documentUri) noexcept;
        HRESULT STDMETHODCALLTYPE AddPage(_In_ UINT32 documentId, _In_ UINT32 pageId, _In_opt_ IXpsOMPageReference* pageReference, _In_ PCWSTR pageUri) noexcept;
        HRESULT STDMETHODCALLTYPE Close() noexcept;
        HRESULT STDMETHODCALLTYPE Failed(HRESULT xpsError) noexcept;
        // End of IPrintWorkflowXpsReceiver interface methods

        uint32_t PageCount();
        winrt::XpsUtil::XpsPageWrapper GetPage(UINT32 pageNumber);


    private:
        com_ptr<IPrintWorkflowObjectModelSourceFileContentNative> m_sourceFileContent;
        com_ptr<XpsPageWatermarker> m_watermarker = nullptr;

        com_ptr<winrt::XpsUtil::implementation::SynchronizedSequentialStream> m_outputStream;
        com_ptr<IXpsOMObjectFactory1> m_xpsFactory;
        com_ptr<IXpsOMPackageWriter> m_xpsOMPackageWriter;


        ULONG m_referenceCount;
        event<Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint32_t>> m_pageAddedEventHandler;
        event<Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint32_t>> m_documentClosedEventHandler;
        event<Windows::Foundation::TypedEventHandler<XpsUtil::XpsSequentialDocument, uint64_t>> m_xpsGenerationFailedEventHandler;

        std::vector<com_ptr<IXpsOMPage>> m_pages;
    };
}
namespace winrt::XpsUtil::factory_implementation
{
    struct XpsSequentialDocument : XpsSequentialDocumentT<XpsSequentialDocument, implementation::XpsSequentialDocument>
    {
    };
}
