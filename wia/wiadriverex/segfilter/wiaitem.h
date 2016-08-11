#pragma once

#define MIN_PROPID 2

class CWiaItem {
public:
    CWiaItem();
    ~CWiaItem();
    HRESULT SetIWiaItem(_In_ IWiaItem2 *pIWiaItem);

    void Release();

    HRESULT ReadPropertyLong(PROPID PropertyID, _Out_ LONG *plPropertyValue);
    HRESULT ReadPropertyGUID(PROPID PropertyID, _Out_ GUID *pguidPropertyValue);
    HRESULT ReadPropertyBSTR(PROPID PropertyID, _Out_ BSTR *pbstrPropertyValue);

    HRESULT WritePropertyLong(PROPID PropertyID, LONG lPropertyValue);
    HRESULT WritePropertyGUID(PROPID PropertyID, GUID guidPropertyValue);

private:
    IWiaPropertyStorage *m_pIWiaPropStg;
protected:
};
