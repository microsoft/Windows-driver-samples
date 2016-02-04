


//
// Different states of current NIC.
//
typedef
enum _NIC_STATE
{
   nic_state_uninitialized,
   nic_state_initialized,
   nic_state_pre_assoc_started,
   nic_state_pre_assoc_ended,
   nic_state_post_assoc_started,
   nic_state_onex_in_progress,
   nic_state_post_assoc_ended,

   nic_state_max // should be the last one.
}
NIC_STATE, *PNIC_STATE;


//////////////////////////////////
// Adapter lifetime management  //
//////////////////////////////////

//
// Adapter Data Structure
//

struct _ADAPTER_DETAILS
{
    // Adapter list and lifetime management.
    LIST_ENTRY                      Link;
    DWORD                           dwRefCount;
    NIC_STATE                       NicState;


    // Framework reference.
    HANDLE                          hDot11SvcHandle;

    // Handler functions for different stages of connection.
    LPTHREAD_START_ROUTINE          pPerformPostAssociateCompletionRoutine;
    POST_ASSOCIATE_FUNCTION         pPerformPostAssociateRoutine;
    STOP_POST_ASSOCIATE_FUNCTION    pStopPostAssociateRoutine;


    // Data for current connection.
    HANDLE                          hConnectSession;
    BOOL                            bModifyCurrentProfile;
    PONEX_DATA                      pOnexData;
    GUID                            currentGuidUIRequest;
    DWORD                           dwResponseLen;
    _Field_size_bytes_(dwResponseLen) BYTE*                        pbResponse;
    HANDLE                          hUIResponse;
    PIHV_CONNECTIVITY_PROFILE       pConnectivityProfile;
    PIHV_SECURITY_PROFILE           pSecurityProfile;
};



//
// Initialize the AdapterList data structure.
//
VOID
InitAdapterDetailsList
(
    VOID
);


//
// Free the AdapterList data structure - contention is not supported.
//
VOID
DeinitAdapterDetailsList
(
    VOID
);

//
// Add a single adapter to the list. Starting refcount is one.
//
DWORD
InitAdapterDetails
(
   HANDLE            hDot11SvcHandle,
   PHANDLE           phIhvExtAdapter
);


//
// Add a reference to a particular adapter and find the pointer to the context.
//
DWORD
ReferenceAdapterDetails
(
    HANDLE              hIhvExtAdapter,
    PADAPTER_DETAILS*   ppAdapterDetails
);


//
// Find adapter context pointer using UI request GUID.
//
DWORD
ReferenceAdapterDetailsByUIRequestGuid
(
    GUID*               pguidUIRequest,
    PADAPTER_DETAILS*   ppAdapterDetails,
    HANDLE*             phIhvExtAdapter
);



//
// Dereference an adapter - resources will be freed when the refcount
// goes to zero.
//
VOID
DerefenceAdapterDetails
(
    HANDLE      hIhvExtAdapter
);

