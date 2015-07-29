/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name: Sauron.cpp


Abstract: Implementation of CSauron, Windows Media Player Visualization
          that communicates with Firefly to blink the mouse light to
          the beat of music.


Environment:

    User mode only.

--*/

#include "stdafx.h"
#include "iSauron.h"
#include "Sauron.h"
#include "luminous.h"
#include <strsafe.h>

/////////////////////////////////////////////////////////////////////////////
// CSauron::CSauron
// Constructor

CSauron::CSauron() :
m_clrForeground(0x0000FF),
m_nPreset(0)
{
    m_Luminous = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CSauron::~CSauron
// Destructor

CSauron::~CSauron()
{
    if (m_Luminous) {

        delete m_Luminous;
        m_Luminous = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CSauron:::FinalConstruct
// Called when an effect is first loaded. Use this function to do one-time
// intializations that could fail (i.e. creating offscreen buffers) instead
// of doing this in the constructor, which cannot return an error.

HRESULT CSauron::FinalConstruct()
{
    if (m_Luminous == NULL) {

        m_Luminous = new CLuminous();

        if (m_Luminous) {

            m_Luminous->Open();
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CSauron:::FinalRelease
// Called when an effect is unloaded. Use this function to free any
// resources allocated in FinalConstruct.

void CSauron::FinalRelease()
{
    if (m_Luminous) {

        m_Luminous->Set(TRUE);
        m_Luminous->Close();

        delete m_Luminous;

        m_Luminous = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
// CSauron::Render
// Called when an effect should render itself to the screen.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::Render(TimedLevel *pLevels, HDC hdc, RECT *prc)
{
    // Fill background with black
    HBRUSH hWipeBrush = ::CreateSolidBrush( 0 );
    HBRUSH hFillBrush = ::CreateSolidBrush( m_clrForeground  );
    HPEN hNewPen = ::CreatePen( PS_SOLID, 0, m_clrForeground );
    HPEN hOldPen = static_cast<HPEN>(::SelectObject( hdc, hNewPen ));
    BOOL setLight;
    int vertical, left;
    int right = 0;
    int chunkSize = 3;
    int c, barWidth;
    int x, y;
    float barWidthFloat, barIndex, barCount;
    RECT rect;

    // draw using the current preset
    switch (m_nPreset)
    {
    case PRESET_BARS:
    case PRESET_FLASH:
        {
            setLight = FALSE;

            //
            // Compute average of 20-80Hz band
            //
            vertical = 0;
            for(c = 0; c < chunkSize; c++) {

                vertical += pLevels->frequency[0][c];
            }
            vertical /= chunkSize;

            //
            // If it passes our threshold, check the upper bands as well
            //
            if (vertical > 170) {

                //
                // Take the average from 21090 to 22050Hz
                //
                vertical = 0;
                for(c = 0; c < chunkSize; c++) {

                    vertical += pLevels->frequency[0][SA_BUFFER_SIZE - c - 1];
                }
                vertical /= chunkSize;
                if (vertical < 10) {

                    //
                    // No high frequency noise, call it a beat
                    //
                    setLight = TRUE;
                }
            }

            if (m_Luminous) {

                m_Luminous->Set(setLight);
            }

            if (m_nPreset == PRESET_FLASH) {

                if (setLight) {

                    ::FillRect( hdc, prc, hFillBrush );

                } else {

                    ::FillRect( hdc, prc, hWipeBrush );
                }

            } else {

                //
                // Walk through the frequencies until we run out of levels or drawing surface.
                //
                x = 0;
                barWidthFloat = 1.0;
                barWidth = 1;
                barIndex = 0.0;
                barCount = 75.0;
                for(x=0; x < SA_BUFFER_SIZE; x += barWidth) {

                    barWidth = (int) (((float) barWidth) * barWidthFloat);
                    barWidthFloat *= (float) 1.011;

                    vertical = 0;
                    for(c=0; c<barWidth; c++) {

                        if ((x + c) >= SA_BUFFER_SIZE) {

                            break;
                        }

                        if (pLevels->frequency[0][x+c] > vertical) {

                            vertical = pLevels->frequency[0][x+c];
                        }
                    }

                    y = static_cast<int>(((prc->bottom - prc->top)/256.0f) * vertical);

                    left = (int) (((prc->right - prc->left) * barIndex / barCount) + (float) prc->left);
                    right = (int) (((prc->right - prc->left) * (barIndex+1) / barCount) + (float) (prc->left - 1));

                    ::SetRect(
                        &rect,
                        left,
                        prc->top,
                        right,
                        prc->bottom - y
                        );

                    ::FillRect( hdc, &rect, hWipeBrush );

                    ::SetRect(
                        &rect,
                        right,
                        prc->top,
                        right+1,
                        prc->bottom
                        );

                    ::FillRect( hdc, &rect, hWipeBrush );

                    ::SetRect(
                        &rect,
                        left,
                        prc->bottom - y,
                        right,
                        prc->bottom
                        );

                    ::FillRect( hdc, &rect, hFillBrush );

                    barIndex += 1.0;
                }

                ::SetRect(
                    &rect,
                    right,
                    prc->top,
                    prc->right,
                    prc->bottom
                    );

                ::FillRect( hdc, &rect, hWipeBrush );
            }
        }
        break;
    }

    if ( hWipeBrush )
    {
        ::DeleteObject( hWipeBrush );
    }

    if ( hFillBrush )
    {
        ::DeleteObject( hFillBrush );
    }

    if (hNewPen)
    {
        ::SelectObject( hdc, hOldPen );
        ::DeleteObject( hNewPen );
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::MediaInfo
// Everytime new media is loaded, this method is called to pass the
// number of channels (mono/stereo), the sample rate of the media, and the
// title of the media
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::MediaInfo(LONG /* lChannelCount */, 
                                LONG /* lSampleRate */,
                                BSTR /* bstrTitle */ )
{
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CSauron::GetCapabilities
// Returns the capabilities of this effect. Flags that can be returned are:
//  EFFECT_CANGOFULLSCREEN      -- effect supports full-screen rendering
//  EFFECT_HASPROPERTYPAGE      -- effect supports a property page
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::GetCapabilities(DWORD * pdwCapabilities)
{
    if (NULL == pdwCapabilities)
    {
        return E_POINTER;
    }

    *pdwCapabilities = 0;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::GetTitle
// Invoked when a host wants to obtain the title of the effect
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::GetTitle(BSTR* bstrTitle)
{
    USES_CONVERSION;

    if (NULL == bstrTitle)
    {
        return E_POINTER;
    }

    CComBSTR bstrTemp;
    bstrTemp.LoadString(IDS_EFFECTNAME);

    if ((!bstrTemp) || (0 == bstrTemp.Length()))
    {
        return E_FAIL;
    }

    *bstrTitle = bstrTemp.Detach();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::GetPresetTitle
// Invoked when a host wants to obtain the title of the given preset
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::GetPresetTitle(LONG nPreset, BSTR *bstrPresetTitle)
{
    USES_CONVERSION;

    if (NULL == bstrPresetTitle)
    {
        return E_POINTER;
    }

    if ((nPreset < 0) || (nPreset >= PRESET_COUNT))
    {
        return E_INVALIDARG;
    }

    CComBSTR bstrTemp;

    switch (nPreset)
    {
    case PRESET_BARS:
        bstrTemp.LoadString(IDS_BARSPRESETNAME);
        break;
    case PRESET_FLASH:
        bstrTemp.LoadString(IDS_FLASHPRESETNAME);
        break;
    }

    if ((!bstrTemp) || (0 == bstrTemp.Length()))
    {
        return E_FAIL;
    }

    *bstrPresetTitle = bstrTemp.Detach();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::GetPresetCount
// Invoked when a host wants to obtain the number of supported presets
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::GetPresetCount(LONG *pnPresetCount)
{
    if (NULL == pnPresetCount)
    {
        return E_POINTER;
    }

    *pnPresetCount = PRESET_COUNT;

    return (S_OK);
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::SetCurrentPreset
// Invoked when a host wants to change the index of the current preset
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::SetCurrentPreset(LONG nPreset)
{
    if ((nPreset < 0) || (nPreset >= PRESET_COUNT))
    {
        return E_INVALIDARG;
    }

    m_nPreset = nPreset;

    return (S_OK);
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::GetCurrentPreset
// Invoked when a host wants to obtain the index of the current preset
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::GetCurrentPreset(LONG *pnPreset)
{
    if (NULL == pnPreset)
    {
        return E_POINTER;
    }

    *pnPreset = m_nPreset;

    return (S_OK);
}

//////////////////////////////////////////////////////////////////////////////
// CSauron::get_foregroundColor
// Property get to retrieve the foregroundColor prop via the public interface.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::get_foregroundColor(BSTR *pVal)
{
    return ColorToWz( pVal, m_clrForeground);
}


//////////////////////////////////////////////////////////////////////////////
// CSauron::put_foregroundColor
// Property put to set the foregroundColor prop via the public interface.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSauron::put_foregroundColor(BSTR newVal)
{
    return WzToColor(newVal, &m_clrForeground);
}


//////////////////////////////////////////////////////////////////////////////
// CSauron::WzToColor
// Helper function used to convert a string into a COLORREF.
//////////////////////////////////////////////////////////////////////////////
HRESULT CSauron::WzToColor(const WCHAR *pwszColor, COLORREF *pcrColor)
{
    if (NULL == pwszColor)
    {
        //NULL color string passed in
        return E_POINTER;
    }

    if (0 == lstrlenW(pwszColor))
    {
        //Empty color string passed in
        return E_INVALIDARG;
    }

    if (NULL == pcrColor)
    {
        //NULL output color DWORD passed in
        return E_POINTER;
    }

    if (lstrlenW(pwszColor) != 7)
    {
        //hex color string is not of the correct length
        return E_INVALIDARG;
    }

    DWORD dwRet = 0;
    for (int i = 1; i < 7; i++)
    {
        // shift dwRet by 4
        dwRet <<= 4;
        // and add in the value of this string

        if ((pwszColor[i] >= L'0') && (pwszColor[i] <= L'9'))
        {
            dwRet += pwszColor[i] - '0';
        }
        else if ((pwszColor[i] >= L'A') && (pwszColor[i] <= L'F'))
        {
            dwRet += 10 + (pwszColor[i] - L'A');
        }
        else if ((pwszColor[i] >= L'a') && (pwszColor[i] <= L'f'))
        {
            dwRet += 10 + (pwszColor[i] - L'a');
        }
        else
        {
           //Invalid hex digit in color string
            return E_INVALIDARG;
        }
    }

    *pcrColor = SwapBytes(dwRet);

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CSauron::ColorToWz
// Helper function used to convert a COLORREF to a BSTR.
//////////////////////////////////////////////////////////////////////////////
HRESULT CSauron::ColorToWz( BSTR* pbstrColor, COLORREF crColor)
{
    _ASSERT( NULL != pbstrColor );
    _ASSERT( (crColor & 0x00FFFFFF) == crColor );

    *pbstrColor = NULL;

    WCHAR wsz[8];
    HRESULT hr  = S_OK;

    hr = StringCbPrintfW( wsz, sizeof(wsz), L"#%06X", SwapBytes(crColor) );
    if (FAILED(hr)) {
        return hr;
    }

    *pbstrColor = ::SysAllocString( wsz );

    if (!pbstrColor)
    {
        hr = E_FAIL;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// CSauron::SwapBytes
// Used to convert between a DWORD and COLORREF.  Simply swaps the lowest
// and 3rd order bytes.
//////////////////////////////////////////////////////////////////////////////
inline DWORD CSauron::SwapBytes(DWORD dwRet)
{
    return ((dwRet & 0x0000FF00) | ((dwRet & 0x00FF0000) >> 16) | ((dwRet & 0x000000FF) << 16));
}


