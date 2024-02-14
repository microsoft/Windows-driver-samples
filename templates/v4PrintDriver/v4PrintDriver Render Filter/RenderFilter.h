//
// File Name:
//
//    RenderFilter.h
//
// Abstract:
//
//    RenderFilter provides the interface to the filter pipeline manager.
//    It implements the IPrintPipelineFilter interface
//

#pragma once

namespace v4PrintDriver_Render_Filter
{


//
// Class to maintain the shared state of operation between multiple components
// of the filter. Also provides the callback for the Xps Rasterization Service.
//
class FilterLiveness : public UnknownBase<IXpsRasterizerNotificationCallback>
{
public:
    
    FilterLiveness() :
        m_isAlive(TRUE)
    {}
    
    virtual
    ~FilterLiveness() {}

    BOOL
    IsAlive()
    {
        return m_isAlive;
    }

    void
    Cancel()
    {
        //
        // May be called from a different thread
        //
        m_isAlive = FALSE;
    }

    //
    // IXpsRasterizerNotificationCallback Method
    //
    virtual _Must_inspect_result_
    HRESULT STDMETHODCALLTYPE
    Continue()
    {
        return (m_isAlive) ? (S_OK) : (HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED));
    }

private:
    volatile BOOL m_isAlive;

    //
    // prevent copy semantics
    //
    FilterLiveness(const FilterLiveness&);
    FilterLiveness& operator=(const FilterLiveness&);
};

class RenderFilter : public UnknownBase<IPrintPipelineFilter>
{

public:

    static LONG ms_numObjects; // Number of instances of RenderFilter

    RenderFilter();

    virtual
    ~RenderFilter();

    //
    // IPrintPipelineFilter Methods
    //
    virtual _Check_return_
    HRESULT STDMETHODCALLTYPE
    InitializeFilter(
        _In_ IInterFilterCommunicator       *pICommunicator,
        _In_ IPrintPipelinePropertyBag      *pIPropertyBag,
        _In_ IPrintPipelineManagerControl   *pIPipelineControl
        );

    virtual _Check_return_
    HRESULT STDMETHODCALLTYPE
    ShutdownOperation();

    virtual _Check_return_
    HRESULT STDMETHODCALLTYPE
    StartOperation();

private:
    //
    // prevent copy semantics
    //
    RenderFilter(const RenderFilter&);
    RenderFilter& operator=(const RenderFilter&);

    //
    // Xps package part reader
    //
    IXpsDocumentProvider_t          m_pReader;
    IPrintWriteStream_t             m_pWriter;

    //
    // Pipeline Property Bag
    //
    IPrintPipelinePropertyBag_t     m_pIPropertyBag;

    //
    // IPrintPipelineFilter Methods (throwing)
    //
    VOID
    StartOperation_throws();

    VOID
    InitializeFilter_throws(
        const IInterFilterCommunicator_t   &pICommunicator,
        const IPrintPipelinePropertyBag_t  &pIPropertyBag
        );
    
    //
    // Keeps track of whether the operation has been cancelled
    //
    FilterLiveness_t m_pLiveness;
};

} // namespace v4PrintDriver_Render_Filter
