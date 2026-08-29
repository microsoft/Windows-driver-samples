#include "pch.h"
#include "SynchronizedSequentialStream.h"
#include <algorithm> 

using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Security::Cryptography;

namespace winrt::XpsUtil::implementation
{
    SynchronizedSequentialStream::SynchronizedSequentialStream()
    {
        m_streamWritten.attach(::CreateEvent(nullptr, false, false, nullptr));
        winrt::check_bool(static_cast<bool>(m_streamWritten));

        m_outputStream = m_storage.GetOutputStreamAt(0);
    }

    HRESULT SynchronizedSequentialStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
    {
        return E_NOTIMPL;
    }

    HRESULT SynchronizedSequentialStream::Write(const void* pv, ULONG cb, ULONG* pcbWritten) noexcept try
    {
        IBuffer buf = CryptographicBuffer::CreateFromByteArray(
                         std::vector<BYTE>(static_cast<BYTE*>(const_cast<void*>(pv)), static_cast<BYTE*>(const_cast<void*>(pv)) + cb));

        {
            auto lock = m_streamAccess.LockExclusive();
            auto progress = m_outputStream.WriteAsync(buf).get();
        }

        SetEvent(m_streamWritten.get());

        *pcbWritten = cb;

        return S_OK;
    }
    catch (...) { return winrt::to_hresult(); }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<IBuffer, unsigned int>
        SynchronizedSequentialStream::ReadAsync(IBuffer buffer, unsigned int count,
                                                            InputStreamOptions options)
    {
        co_await winrt::resume_background();

        winrt::Windows::Storage::Streams::IBuffer retBuffer = nullptr;
        uint64_t bytesAvailable = WaitForXBytes(count);

        auto lock = m_streamAccess.LockExclusive();
        auto inputSteam = m_storage.GetInputStreamAt(m_readIndex);
        uint64_t bytesToRead = (count < bytesAvailable) ? count : bytesAvailable;
        retBuffer = inputSteam.ReadAsync(buffer, bytesToRead, options).get();

        m_readIndex += bytesToRead;

        co_return retBuffer;
    }

    uint64_t SynchronizedSequentialStream::WaitForXBytes(uint64_t noOfbytes)
    {

        size_t bytesAvailable = GetAvailableBytesToRead();
        while ((noOfbytes > bytesAvailable) && !m_streamClosed)
        {
            DWORD waitReturn = ::WaitForSingleObject(m_streamWritten.get(), INFINITE);
            if (waitReturn != WAIT_OBJECT_0)
            {
                throw winrt::hresult_error(E_UNEXPECTED);
            }

            bytesAvailable = GetAvailableBytesToRead();
        }
        return bytesAvailable;
    }

    uint64_t SynchronizedSequentialStream::GetAvailableBytesToRead()
    {
        auto lock = m_streamAccess.LockExclusive();
        return (m_storage.Size() - m_readIndex);
    }

    void SynchronizedSequentialStream::Close()
    {
        m_streamClosed = true;
        SetEvent(m_streamWritten.get());
    }
}
