#pragma once
#include "XpsPageWrapper.g.h"

namespace winrt::XpsUtil::implementation
{
    struct XpsPageWrapper : XpsPageWrapperT<XpsPageWrapper>
    {
        XpsPageWrapper(com_ptr<IXpsOMPage> page);
        XpsUtil::XpsPageWrapper Clone();

        Windows::Storage::Streams::IRandomAccessStream RenderPageToBMP();

        com_ptr<IXpsOMPage> GetPage();

    private:
        com_ptr<IXpsOMPage> m_page;

        void ConvertBMPtoStream(_In_ com_ptr<IUnknown> bitmap, _In_ com_ptr<IStream> bitmapStream);
    };
}
