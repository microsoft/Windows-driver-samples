#pragma once

#define MIN_PROPID 2

class CWiaItem {
public:
    CWiaItem();
    ~CWiaItem();
    HRESULT SetIWiaItem(IWiaItem2 *pIWiaItem);
    void Release();

    HRESULT ReadRequiredPropertyLong(PROPID PropertyID, _Out_ LONG *plPropertyValue);
    HRESULT ReadRequiredPropertyBSTR(PROPID PropertyID, _Outptr_ BSTR *pbstrPropertyValue);
    HRESULT ReadRequiredPropertyGUID(PROPID PropertyID, _Out_ GUID *pguidPropertyValue);

private:
    IWiaPropertyStorage *m_pIWiaPropStg;
protected:
};
