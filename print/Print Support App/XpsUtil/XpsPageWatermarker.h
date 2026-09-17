#pragma once
#include "XpsPageWatermarker.g.h"

namespace winrt::XpsUtil::implementation
{
    struct XpsPageWatermarker : XpsPageWatermarkerT<XpsPageWatermarker>
    {
        XpsPageWatermarker();

        void SetWatermarkText(hstring const& text, double fontSize, double xRelativeOffset, double yRelativeOffset);
        void SetWatermarkImage(hstring const& imagePath, double dpiX, double dpiY, int32_t width, int32_t height);
        void SetWatermarkImageEnabled(bool enabled);
        void ApplyWatermarks(XpsUtil::XpsPageWrapper const& page);
        void ApplyWatermarksToXpsPage(com_ptr<IXpsOMPage> const& xpsPage);

    private:
        com_ptr<IXpsOMObjectFactory1> m_xpsFactory;

        hstring m_watermarkText;
        double m_watermarkFontSize;
        double m_watermarkXRelativeOffset, m_watermarkYRelativeOffset;

        std::wstring m_imagePath;
        double m_imageDpiX, m_imageDpiY;
        int m_imageWidth, m_imageHeight;
        bool m_imageEnabled;

        void AddWatermarkText(com_ptr<IXpsOMPage> const& page);
        com_ptr<IXpsOMFontResource> CreateFontResource(const std::wstring& fontFilepath);
        void AddTextToPage(com_ptr<IXpsOMPage> const& xpsPage, std::wstring const& text, double fontSize, double xRelativeOffset, double yRelativeOffset);
        void AddTextToPage(com_ptr<IXpsOMPage> const& xpsPage, _In_ std::wstring text, const com_ptr<IXpsOMFontResource>& fontResource, float fontSize, const com_ptr<IXpsOMSolidColorBrush>& colorBrush, XPS_POINT* origin);

        void SetImageProperties(_In_ hstring const& imagePath, _In_ double dpiX, _In_ double dpiY, _In_ int width, _In_ int height);
        bool GetImageProperties(std::wstring& imagePath, double& dpiX, double& dpiY, int& width, int& height);
        void AddWatermarkImage(com_ptr<IXpsOMPage> const& xpsPage);
        void AddImageToPage(com_ptr<IXpsOMPage> const& xpsPage, _In_ const XPS_SIZE& pageDimensions);

        HRESULT CreateImageResource(_In_ const std::wstring imageFileName, _In_ const std::wstring imagePartName, _Out_ IXpsOMImageResource** imageResource);
        HRESULT AddImageToVisualCollection(com_ptr<IXpsOMPage> const& xpsPage, IXpsOMImageResource* imageResource, XPS_SIZE* imageDim, float dpiX, float dpiY, XPS_RECT rectForImage, std::wstring imageFileShortDescription);
        HRESULT CreateRectanglePath(const XPS_RECT& rect, IXpsOMPath** rectPath);
    };
}
namespace winrt::XpsUtil::factory_implementation
{
    struct XpsPageWatermarker : XpsPageWatermarkerT<XpsPageWatermarker, implementation::XpsPageWatermarker>
    {
    };
}
