#include "stdafx.h"
#include "multipinmft.h"
#include "basepin.h"
#include "custompin.h"
#ifdef MF_WPP
#include "custompin.tmh"
#endif


//
// Implementation notes
// Implement this file only if you have custom media pins streaming 
// This implementation just streams from the driver and sends it back
// to the pipeline. This method can be used to stream 3A/Statistic Pins. 
// 

CCustomPin::CCustomPin(
    _In_ IMFAttributes *pAttributes,
    _In_ ULONG PinId,
    _In_ CMultipinMft *pParent
    )
    :CInPin( pAttributes, PinId, pParent )
{

}
//
// Process input calls the pin here.. In this code sample we
// don't send it anywhere. It is simply shortcircuited back to the pipeline
//
STDMETHODIMP CCustomPin::SendSample(
    _In_ IMFSample *pSample
    )
{
    //
    // Log sample and exit.. The pipeline will just keep on churning more samples till 
    // We go into the stop state
    //
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Custom Pin %d recieved Sample %p", streamId(), pSample);

    return S_OK;
}

//
// State change transitions on the custom pins are different than the other pins
// The other known pins i.e. preview, record, image etc are exposed out to the pipeline
// and the pipeline will set state on it, reset state, For the other pins we tell the 
// pipeline that we have samples and the pipeline picks it up. 
//
// The CUSTOM pin differs in that it's state has to be managed to by the device MFT. 
// This code snippet just varies state transitions on the custom pin when the Device MFT
// changes state to STREAMING/END_STREAMING on it's exposed pins. 
//
// The below function tells the pipeline to change the state on the custom pin by sending
// METransformInputStreamStateChanged event with the streamid attribute. The pipeline will
// call back in the custom pin at getprefferedmediatype and finally setinputstreamstate
// which will signal back here singnalling sucessful state transition.
//

STDMETHODIMP_(DeviceStreamState) CCustomPin::SetState(
    _In_ DeviceStreamState State
    )
{
    HRESULT hr = S_OK;
    
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! Id:%d Transition into state: %d ", streamId(), State);

    DeviceStreamState oldState = setPreferredStreamState( State );

    if ( oldState != State )
    {
        ComPtr<IMFMediaType> preferredMediaType = nullptr;

        if (State == DeviceStreamState_Run)
        {
            GetMediaTypeAt(0, preferredMediaType.ReleaseAndGetAddressOf());
        }

        setPreferredMediaType(preferredMediaType.Get());
        setPreferredStreamState( State );
        
        {
            //
            // Notify the device transform manager that we have changed state
            //
            ComPtr<IMFMediaEvent> pEvent = nullptr;
            DMFTCHECKHR_GOTO( MFCreateMediaEvent(METransformInputStreamStateChanged, GUID_NULL, S_OK, NULL, pEvent.ReleaseAndGetAddressOf()), done );
            DMFTCHECKHR_GOTO( pEvent->SetUINT32(MF_EVENT_MFT_INPUT_STREAM_ID, streamId()), done );
            DMFTCHECKHR_GOTO( Parent()->QueueEvent(pEvent.Get()), done );
        }
        //
        // Wait to be notified back from the pipeline. 
        //
        DMFTCHECKHR_GOTO( WaitForSetInputPinMediaChange(),done );
    }
done:
    DMFTRACE(DMFT_GENERAL, TRACE_LEVEL_INFORMATION, "%!FUNC! exiting %x = %!HRESULT!", hr, hr);
    return oldState;
}
