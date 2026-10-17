#include "pch.h"
#include "XpsPageWatermarker.h"
#include "XpsPageWatermarker.g.cpp"
#include <XpsPageWrapper.h>

namespace winrt::XpsUtil::implementation
{
    /// <summary>
    /// Create a new XpsPageWatermarker. By default, no change will be applied to the page.
    /// </summary>
    XpsPageWatermarker::XpsPageWatermarker() : m_imagePath(L""), m_watermarkText(L"")
    {
        m_xpsFactory = create_instance<IXpsOMObjectFactory1>(guid_of<XpsOMObjectFactory>());
    }

    /// <summary>
    /// Apply the requested watermarks to the given page wrapper. This will irreversibly modify the underlying page.
    /// </summary>
    /// <param name="page"></param>
    void XpsPageWatermarker::ApplyWatermarks(XpsUtil::XpsPageWrapper const& page)
    {
        com_ptr<IXpsOMPage> xpsPage = get_self<winrt::XpsUtil::implementation::XpsPageWrapper>(page)->GetPage();

        ApplyWatermarksToXpsPage(xpsPage);
    }

    /// <summary>
    /// Apply the requested watermarks to the given IXpsOMPage. This irreversibly modifies the page.
    /// </summary>
    /// <param name="xpsPage"></param>
    void XpsPageWatermarker::ApplyWatermarksToXpsPage(com_ptr<IXpsOMPage> const& xpsPage)
    {
        if (!m_watermarkText.empty()) {
            AddWatermarkText(xpsPage);
        }

        if (m_imageEnabled && !m_imagePath.empty()) {
            AddWatermarkImage(xpsPage);
        }
    }

    /// <summary>
    /// Set the desired attributes for the watermark text
    /// </summary>
    /// <param name="text">String to overlay on the page</param>
    /// <param name="fontSize">Font size of the overlayed text</param>
    /// <param name="xRelativeOffset">Relative offset in the X axis (left-right) of the bottom left corner of the text. Double between 0 and 1.</param>
    /// <param name="yRelativeOffset">Relative offset in the Y axis (up-down) of the bottom left corner of the text. Double between 0 and 1.</param>
    void XpsPageWatermarker::SetWatermarkText(hstring const& text, double fontSize, double xRelativeOffset, double yRelativeOffset)
    {
        if (m_watermarkXRelativeOffset< 0 || m_watermarkXRelativeOffset > 1 || m_watermarkYRelativeOffset < 0 || m_watermarkYRelativeOffset > 1)
        {
            throw hresult_invalid_argument();
        }

        m_watermarkText = text;
        m_watermarkFontSize = fontSize;
        m_watermarkXRelativeOffset = xRelativeOffset;
        m_watermarkYRelativeOffset = yRelativeOffset;
    }

    void XpsPageWatermarker::SetWatermarkImage(hstring const& imagePath, double dpiX, double dpiY, int32_t width, int32_t height)
    {
        SetImageProperties(imagePath, dpiX, dpiY, width, height);
    }

    /// <summary>
    /// Sets whether to apply a watermark image. If false, 
    /// </summary>
    /// <param name="enabled"></param>
    void XpsPageWatermarker::SetWatermarkImageEnabled(bool enabled)
    {
        m_imageEnabled = enabled;
    }

    void XpsPageWatermarker::AddWatermarkText(com_ptr<IXpsOMPage> const& page)
    {
        AddTextToPage(page, std::wstring(m_watermarkText), m_watermarkFontSize, m_watermarkXRelativeOffset, m_watermarkYRelativeOffset);
    }

    com_ptr<IXpsOMFontResource> XpsPageWatermarker::CreateFontResource(const std::wstring& fontFilepath)
    {
        // Create the font URI, the URI is the path in the XPS package.
        com_ptr<IOpcPartUri> fontUri;
        check_hresult(m_xpsFactory->CreatePartUri(L"/Resources/Fonts/WatermarkFont.odttf", fontUri.put()));

        // Load an existing font file to an IStream.
        com_ptr<IStream> fontStream;
        check_hresult(m_xpsFactory->CreateReadOnlyStreamOnFile(fontFilepath.c_str(), fontStream.put()));

        // Create the font resource.
        com_ptr<IXpsOMFontResource> fontResource;
        check_hresult(m_xpsFactory->CreateFontResource(fontStream.get(),
            XPS_FONT_EMBEDDING_NORMAL,
            fontUri.get(),
            FALSE, // isObfSourceStream
            fontResource.put()));
        return fontResource;
    }

    void XpsPageWatermarker::AddTextToPage(com_ptr<IXpsOMPage> const& xpsPage, _In_ std::wstring text, const com_ptr<IXpsOMFontResource>& fontResource, float fontSize, const com_ptr<IXpsOMSolidColorBrush>& colorBrush, XPS_POINT* origin)
    {
        // Create a new Glyphs object and set its properties.
        com_ptr<IXpsOMGlyphs> xpsGlyphs;
        check_hresult(m_xpsFactory->CreateGlyphs(fontResource.get(), xpsGlyphs.put()));
        check_hresult(xpsGlyphs->SetOrigin(origin));
        check_hresult(xpsGlyphs->SetFontRenderingEmSize(fontSize));
        check_hresult(xpsGlyphs->SetFillBrushLocal(colorBrush.get()));

        // Some properties are inter-dependent so they
        // must be changed by using a GlyphsEditor.
        com_ptr<IXpsOMGlyphsEditor> glyphsEditor;
        check_hresult(xpsGlyphs->GetGlyphsEditor(glyphsEditor.put()));
        check_hresult(glyphsEditor->SetUnicodeString(text.c_str()));
        check_hresult(glyphsEditor->ApplyEdits());

        // Add the new Glyphs object to the page
        com_ptr<IXpsOMVisualCollection> pageVisuals;
        check_hresult(xpsPage->GetVisuals(pageVisuals.put()));
        check_hresult(pageVisuals->Append(xpsGlyphs.get()));
    }

    void XpsPageWatermarker::AddTextToPage(com_ptr<IXpsOMPage> const& xpsPage, std::wstring const& text, double fontSize, double xRelativeOffset, double yRelativeOffset)
    {
        // Create the color brush for the font.
        XPS_COLOR xpsColorBodyText = {};
        xpsColorBodyText.colorType = XPS_COLOR_TYPE_SRGB;
        // Change the alpha value to make the watermark text transparent.
        xpsColorBodyText.value.sRGB.alpha = 0x40;

        com_ptr<IXpsOMSolidColorBrush> xpsTextColorBrush;
        check_hresult(m_xpsFactory->CreateSolidColorBrush(&xpsColorBodyText, nullptr, xpsTextColorBrush.put()));

        // Create the XPS_POINT for where the text begins.
        XPS_SIZE pageDimensions = { 0 };
        check_hresult(xpsPage->GetPageDimensions(&pageDimensions));
        XPS_POINT textOrigin = { pageDimensions.width * static_cast<float>(xRelativeOffset), pageDimensions.height * static_cast<float>(yRelativeOffset) };

        auto fontResource = CreateFontResource(L"Assets/Fonts/Roboto-Black.ttf");

        AddTextToPage(xpsPage, text, fontResource, static_cast<float>(fontSize), xpsTextColorBrush, &textOrigin);
    }

    void XpsPageWatermarker::AddWatermarkImage(com_ptr<IXpsOMPage> const& xpsPage)
    {
        XPS_SIZE pageDimensions{ };
        check_hresult(xpsPage->GetPageDimensions(&pageDimensions));
        AddImageToPage(xpsPage, pageDimensions);
    }

    void XpsPageWatermarker::AddImageToPage(_In_ com_ptr<IXpsOMPage> const& xpsPage, _In_ const XPS_SIZE& pageDimensions)
    {
        std::wstring imageFileName = L"";
        double dpiX = 0.0L, dpiY = 0.0L;
        int imageHeight = 0, imageWidth = 0;

        if (GetImageProperties(imageFileName, dpiX, dpiY, imageWidth, imageHeight))
        {
            std::wstring imagePartName = L"Assets/Images/PrintSupportImage.jpg";
            std::wstring imageFileShortDescription = L"PSA Added Logo";

            //Dimensions of the image in pixels
            XPS_SIZE imageDim = { static_cast<float>(imageWidth), static_cast<float>(imageHeight) };
            //Image position on Page
            XPS_RECT rectForImage = { 0, 0, pageDimensions.width / 10, pageDimensions.height / 10 };

            com_ptr<IXpsOMImageResource> imageResource;
            check_hresult(CreateImageResource(imageFileName, imagePartName, imageResource.put()));
            check_hresult(AddImageToVisualCollection(
                xpsPage,
                imageResource.get(),
                &imageDim,
                static_cast<float>(dpiX),
                static_cast<float>(dpiY),
                rectForImage,
                imageFileShortDescription));
        }
    }

    void XpsPageWatermarker::SetImageProperties(_In_ hstring const& imagePath, _In_ double dpiX, _In_ double dpiY, _In_ int width, _In_ int height)
    {
        m_imagePath = std::wstring(imagePath);
        m_imageDpiX = dpiX;
        m_imageDpiY = dpiY;
        m_imageWidth = width;
        m_imageHeight = height;
    }

    bool XpsPageWatermarker::GetImageProperties(_Out_ std::wstring& imagePath, _Out_ double& dpiX, _Out_ double& dpiY, _Out_ int& width, _Out_ int& height)
    {
        imagePath = m_imagePath;
        dpiX = m_imageDpiX;
        dpiY = m_imageDpiY;
        width = m_imageWidth;
        height = m_imageHeight;

        // Sanity check
        if (!imagePath.empty() && imagePath[0] != L'\0' && dpiX != 0.0L && dpiY != 0.0L && width != 0 && height != 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    HRESULT XpsPageWatermarker::CreateImageResource(_In_ const std::wstring imageFileName, _In_ const std::wstring imagePartName, _Out_ IXpsOMImageResource** imageResource)
    {
        if (imageFileName.empty() || imagePartName.empty())
        {
            return E_INVALIDARG;
        }

        com_ptr<IStream> imageStream;
        com_ptr<IOpcPartUri> imagePartUri;

        m_xpsFactory->CreateReadOnlyStreamOnFile(imageFileName.c_str(), imageStream.put());
        m_xpsFactory->CreatePartUri(imagePartName.c_str(), imagePartUri.put());

        XPS_IMAGE_TYPE imageType = XPS_IMAGE_TYPE_JPEG; // set to type of image being read in
        check_hresult(m_xpsFactory->CreateImageResource(
            imageStream.get(),
            imageType,
            imagePartUri.get(),
            imageResource));

        return S_OK;
    }

    HRESULT XpsPageWatermarker::AddImageToVisualCollection(
        com_ptr<IXpsOMPage> const& xpsPage, IXpsOMImageResource* imageResource, XPS_SIZE* imageDim, float dotsPerInchX,
        float dotsPerInchY, XPS_RECT rectForImage, std::wstring shortDescription)
    {
        if (imageResource == nullptr)
        {
            return E_INVALIDARG;
        }

        XPS_RECT viewPort = { 0.0, 0.0, 0.0, 0.0 };
        XPS_RECT viewBox = { 0.0, 0.0, 0.0, 0.0 };

        com_ptr<IXpsOMPath> imageRectPath;
        com_ptr<IXpsOMImageBrush> imageBrush;
        com_ptr<IXpsOMVisualCollection> pageVisuals;

        // Describe image source dimensions and set viewbox to be the
        // entire image DPI width of image.
        //  Example:
        //    600 image pixels, 300 dpi -> 2 inches -> 2 * 96 = 192 DIP width
        viewBox.width = imageDim->width * 96.0f / dotsPerInchX;
        viewBox.height = imageDim->height * 96.0f / dotsPerInchY;

        // destination rectangle
        viewPort.x = rectForImage.x;
        viewPort.y = rectForImage.y;
        viewPort.width = rectForImage.width;
        viewPort.height = rectForImage.height;

        // Create the image brush.
        check_hresult(m_xpsFactory->CreateImageBrush(imageResource, &viewBox, &viewPort, imageBrush.put()));

        // Create the path that describes the outline of the image on the page.
        check_hresult(CreateRectanglePath(rectForImage, imageRectPath.put()));

        // Set the accessibility description for the path object as required.
        check_hresult(imageRectPath->SetAccessibilityShortDescription(shortDescription.c_str()));

        // Set the image brush to be the fill brush for this path.
        check_hresult(imageRectPath->SetFillBrushLocal(imageBrush.get()));

        // Get the list of visuals for this page...
        check_hresult(xpsPage->GetVisuals(pageVisuals.put()));

        // ...and add the completed path to the list.
        check_hresult(pageVisuals->Append(imageRectPath.get()));

        return S_OK;
    }

    HRESULT XpsPageWatermarker::CreateRectanglePath(_In_ const XPS_RECT& rect, _Out_ IXpsOMPath** rectPath)
    {
        if (rectPath == nullptr)
        {
            return E_INVALIDARG;
        }

        com_ptr<IXpsOMGeometryFigure> rectFigure;
        com_ptr<IXpsOMGeometry> imageRectGeometry;
        com_ptr<IXpsOMGeometryFigureCollection> geomFigureCollection;

        // Define start point and three of the four sides of the rectangle.
        // The fourth side is implied by setting the path type to CLOSED.
        XPS_POINT startPoint = { rect.x, rect.y };
        XPS_SEGMENT_TYPE segmentTypes[3] = { XPS_SEGMENT_TYPE_LINE,
                                            XPS_SEGMENT_TYPE_LINE,
                                            XPS_SEGMENT_TYPE_LINE };
        FLOAT segmentData[6] = { rect.x, rect.y + rect.height,
                                rect.x + rect.width, rect.y + rect.height,
                                rect.x + rect.width, rect.y };
        BOOL segmentStrokes[3] = { TRUE, TRUE, TRUE };

        // Create a closed geometry figure using the three segments defined above.
        check_hresult(m_xpsFactory->CreateGeometryFigure(&startPoint, rectFigure.put()));
        check_hresult(rectFigure->SetIsClosed(TRUE));
        check_hresult(rectFigure->SetIsFilled(TRUE));
        check_hresult(rectFigure->SetSegments(3, 6, segmentTypes, segmentData, segmentStrokes));

        // Create a geometry that consists of the figure created above.
        check_hresult(m_xpsFactory->CreateGeometry(imageRectGeometry.put()));
        check_hresult(imageRectGeometry->GetFigures(geomFigureCollection.put()));
        check_hresult(geomFigureCollection->Append(rectFigure.get()));

        // Create the path that consists of the geometry created above
        //  and return the pointer in the parameter passed in to the function.
        check_hresult(m_xpsFactory->CreatePath(reinterpret_cast<IXpsOMPath**>(rectPath)));
        check_hresult((*rectPath)->SetGeometryLocal(imageRectGeometry.get()));

        // The calling method will release IXpsOMPath when it is done with it.

        return S_OK;
    }
}
