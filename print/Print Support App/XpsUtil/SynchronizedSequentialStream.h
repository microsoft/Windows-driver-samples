#pragma once
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Security.Cryptography.h"
//#include "XpsPageWrapper.g.h"
#include <wrl/client.h>
#include <wrl/async.h>

namespace winrt::XpsUtil::implementation
{
    struct SynchronizedSequentialStream : winrt::implements<SynchronizedSequentialStream, ISequentialStream, winrt::Windows::Storage::Streams::IInputStream, winrt::Windows::Foundation::IClosable>
    {
        SynchronizedSequentialStream();

        // ISequentialStream
        STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead);
        STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten) noexcept;

        // IInputStream
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Windows::Storage::Streams::IBuffer, unsigned int> ReadAsync(winrt::Windows::Storage::Streams::IBuffer buffer, unsigned int count, winrt::Windows::Storage::Streams::InputStreamOptions options);

        // IClosable
        void Close();

    private:
        uint64_t WaitForXBytes(uint64_t m_index);
        uint64_t GetAvailableBytesToRead();

        winrt::Windows::Storage::Streams::InMemoryRandomAccessStream m_storage;
        winrt::Windows::Storage::Streams::IOutputStream m_outputStream;

        bool m_streamClosed = false;
        uint64_t m_readIndex = 0;

        winrt::handle m_streamWritten;
        Microsoft::WRL::Wrappers::SRWLock m_streamAccess;
    };
}
