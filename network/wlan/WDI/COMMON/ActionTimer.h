
#ifndef __INC_ACTION_TIMER_H
#define __INC_ACTION_TIMER_H


// Use Handle instead of pointer for private access ------
typedef u1Byte 	ACTION_TIMER_HANDLE;
#define 	INVALID_ACTION_TIMER_HANDLE		0
#define 	MAX_ACTION_TIMER_HANDLE			255
// ----------------------------------------------


typedef enum _ACTION_TIMER_REFERENCED_CLOCK
{
	CLOCK_INVALID = 0,
	SW_OS_SYSTEM_CLOCK,
	HW_TSF_CLOCK

} ACTION_TIMER_REFERENCED_CLOCK, *PACTION_TIMER_REFERENCED_CLOCK;


typedef enum _ACTION_TIMER_HW_TIMER
{
	HW_TIMER_NO_USE = 0,
	HW_TSF_CLOCK_PS_TIMER,
	HW_TIMER_MAX
	
} ACTION_TIMER_HW_TIMER, *PACTION_TIMER_HW_TIMER;



#define 	US_CLOCK_TO_MS(_usClock)		((_usClock) / 1000)

#define  ACTION_TIMER_HW_TIMER_INTERRUPT_DELAY_US	(50)

#define  ACTION_TIMER_ITEM_FIRE_TOLERANCE_TIME_US	(1000)
#define  ACTION_TIMER_ITEM_MAX_PENDING_TIME_MS		(30 * 1000)

#define  ACTION_TIMER_ITEM_MAX_TOTAL_EXECUTION_TIME_US		(3000)
#define  ACTION_TIMER_ITEM_MAX_SINGLE_EXECUTION_TIME_US		(1000)

typedef	struct _ACTION_TIMER_ITEM ACTION_TIMER_ITEM, *PACTION_TIMER_ITEM;

typedef enum _ACTION_TYPE
{
	ACTION_TYPE_INVALID = 0, 
	ACTION_TYPE_MULTICHANNEL_SWITCH,
	ACTION_TYPE_CUSTOMIZED_SCAN,
	ACTION_TYPE_P2P_POWERSAVE,
	ACTION_TYPE_ALL,
	ACTION_TYPE_MAX
	
} ACTION_TYPE, *PACTION_TYPE;


typedef VOID
(*ACTION_TIMER_ITEM_CALLBACK)(
	const ACTION_TIMER_ITEM * const	pOneShotActionItem
);

typedef	struct _ACTION_TIMER_ITEM
{
	// Internal Use for ActionTimer ---------------------------------------------
	RT_LIST_ENTRY				List;
	ACTION_TIMER_HANDLE			ActionOwner;		// Owner of this action item
	// ---------------------------------------------------------------------

	// Fill the Following to Register an Item --------------------------------------------------------------------
	ACTION_TYPE 				ActionType;			// Category for a set of action items implementing a single feature
	u8Byte						usTimeout;			// Absolute system us time specifying the timeout
	PVOID						pContext;			// User-specified data context
	ACTION_TIMER_ITEM_CALLBACK	CallbackFunc;		// Timeout callback function
	// ---------------------------------------------------------------------------------------------------
	
} ACTION_TIMER_ITEM, *PACTION_TIMER_ITEM;


typedef struct _ACTION_TIMER 
{
	RT_LIST_ENTRY			List;
	
	ACTION_TIMER_HANDLE		TimerHandle;

	ACTION_TIMER_REFERENCED_CLOCK	ClockType;

	// For Tsf Shared Hardware Timer -------------
	ACTION_TIMER_HW_TIMER	HwTimer;
	// ---------------------------------------
	
	// For SW_OS_SYSTEM_CLOCK ---------------
	RT_TIMER				SoftTimer;
	// ---------------------------------------

	RT_LIST_ENTRY			TimeOrderedActionItemQueue;	// Protected by ACTION_TIMER_COMMON_CONTEXT.ActionTimerLock
	u4Byte					uNumberOfActionItems;			// Protected by ACTION_TIMER_COMMON_CONTEXT.ActionTimerLock
	
} ACTION_TIMER, *PACTION_TIMER;


typedef struct _ACTION_TIMER_COMMON_CONTEXT {

	RT_SPIN_LOCK	ActionTimerLock;	

	// Preallocated Hw Action Timer (Indexed by ACTION_TIMER_HW_TIMER) -------------------------
	ACTION_TIMER	HwActionTimer[HW_TIMER_MAX];
	// ------------------------------------------------------------------------------------
	
	RT_LIST_ENTRY	ActionTimerQueue;				// Protected by ACTION_TIMER_COMMON_CONTEXT.ActionTimerLock
	u4Byte			uNumberOfActionTimers;			// Protected by ACTION_TIMER_COMMON_CONTEXT.ActionTimerLock
	u1Byte			LastAssignedTimerHandle;		// Protected by ACTION_TIMER_COMMON_CONTEXT.ActionTimerLock


	// Check if the dynamic memory is returned completely ------
	u4Byte 			uAllocatedMemory;
	u4Byte			uReleasedMemory;
	// --------------------------------------------------
		
} ACTION_TIMER_COMMON_CONTEXT, *PACTION_TIMER_COMMON_CONTEXT;

#define  ACTION_TIMER_SIZE_OF_COMMON_CONTEXT 		sizeof(ACTION_TIMER_COMMON_CONTEXT)



VOID
ActionTimerInitializeCommonContext(
	PADAPTER pAdapter
);

VOID
ActionTimerDeInitializeCommonContext(
	PADAPTER	pAdapter
);

VOID
ActionTimerRegisterActionItem(
	PADAPTER				pAdapter,
	PACTION_TIMER_ITEM		pInputDataItem,
	ACTION_TIMER_HANDLE	 	ActionTimerHandle
);

ACTION_TIMER_HANDLE
ActionTimerAllocate(
	PADAPTER 	pAdapter,
	const char*	pTimerName,
	ACTION_TIMER_REFERENCED_CLOCK	ClockType,
	ACTION_TIMER_HW_TIMER	HwTimerSelect
);

VOID
ActionTimerRelease(
	PADAPTER 				pAdapter,
	ACTION_TIMER_HANDLE		ActionTimerHandle
);

VOID
ActionTimerFlushActionItem(
	PADAPTER 				pAdapter,
	ACTION_TIMER_HANDLE		ActionTimerHandle,
	ACTION_TYPE				ActionType
);

VOID
ActionTimerHwTimerCallback(
	PADAPTER					pAdapter,
	ACTION_TIMER_HW_TIMER		HwTimer
);

#endif
