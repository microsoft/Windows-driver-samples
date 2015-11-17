/*++

Copyright (c) 2011 Microsoft Corporation

Module Name:

    HalExtSampleTimers.c

Abstract:

    This file implements a HAL Extension Module for the fictitious EGTimer.

Author:

    Evan Green (evgreen) 8-Jan-2011

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <nthalext.h>

//
// -------------------------------------------------------------- Specification
//

//
// The EGTimer is a fast-access low-latency timer designed for high-performance
// timekeeping operations. It consists of 4 timers with identical register
// definitions offset from each other. Each timer contains a 32-bit
// counter and is capable of generating one-shot or periodic interrupts using
// a "Reload Value" register. The counter counts up and generates an interrupt
// once it rolls over from its maximum value of 0xFFFFFFFF to 0 if the
// interrupt enable bit is set. The counter always runs at a frequency of
// 10MHz. The register layout of the timer appears below:
//
// Offset   Register                    Size
// 0x00     Timer0_Control              4
// 0x04     Timer0_ReloadValue          4
// 0x08     Timer0_CurrentCount         4
// 0x0C     Timer0_InterruptAcknowledge 4
// 0x10     Timer1_Control              4
// 0x14     Timer1_ReloadValue          4
// 0x18     Timer1_CurrentCount         4
// 0x1C     Timer1_InterruptAcknowledge 4
// 0x20     Timer2_Control              4
// 0x24     Timer2_ReloadValue          4
// 0x28     Timer2_CurrentCount         4
// 0x2C     Timer2_InterruptAcknowledge 4
// 0x30     Timer3_Control              4
// 0x34     Timer3_ReloadValue          4
// 0x38     Timer3_CurrentCount         4
// 0x3C     Timer3_InterruptAcknowledge 4
//
// Register Descriptions:
//
// TimerN_Control - This is a Read/Write register containing a bitfield of
//     control values that govern the timer's operation. The initial value of
//     this register at reset is 0. The defined bits are listed below. 0 should
//     always be written to undefined bits, and will always be read as 0.
//
//     Bit    Function
//     31-3   Reserved. Read as 0, always write 0 to maintain future
//                compatibility.
//
//     2      Periodic. When set to 1, the timer will automatically reload
//                itself with the value in the ReloadValue register.
//                When set to 0, the timer's counter will be disabled by
//                clearing the "Counter Enabled" bit when it overflows
//                from 0xFFFFFFFF to 0.
//
//     1      Interrupt Enable. When set to 1, the timer will generate an
//                interrupt when the counter overflows from 0xFFFFFFFF to 0.
//                When set to 0, the timer will not generate an interrupt upon
//                overflow.
//
//     0      Counter Enable. When set to 1, the counter is enabled and
//                running. When set to 0, the counter is disable and will not
//                run.
//
// TimerN_ReloadValue - This is a Read/Write register containing the value to
//     reload the counter with when the timer is in Periodic mode and overflows
//     from 0xFFFFFFFF to 0.  Writing a value to this register also immediately
//     writes the same value to the current count register. The initial value
//     of this register at reset is 0.
//
// TimerN_CurrentCount - This is a Read/Write register containg the current
//     value of the counter. This value can be overwritten at any time. It is
//     also overwritten by a write to the ReloadValue register. The initial
//     value of this register at reset is 0.
//
// TimerN_InterruptAcknowledge - This is a Read/Write register. On reads, it
//     returns 1 if an interrupt is pending that has yet to be acknowledged.
//     Writing a value of 0 clears the pending interrupt. This initial value of
//     this register at reset is 0.
//

//
// ---------------------------------------------------------------- Definitions
//

//
// Define the total size of the register block, which is 1 page.
//

#define EGTIMER_BLOCK_SIZE 0x1000

//
// Define the size of one timer's registers.
//

#define EGTIMER_SIZE 0x10

//
// Define the total number of timers.
//

#define EGTIMER_COUNT 4

//
// Define the timer's bit width.
//

#define EGTIMER_BIT_WIDTH 32

//
// Define the counter's frequency, in Hertz.
//

#define EGTIMER_FREQUENCY 10000000

//
// Define bits for the control register.
//

#define EGTIMER_CONTROL_ENABLE           0x00000001
#define EGTIMER_CONTROL_INTERRUPT_ENABLE 0x00000002
#define EGTIMER_CONTROL_PERIODIC         0x00000004

//
// ------------------------------------------------------ Data Type Definitions
//


//
// Define the registers and their offsets, in ULONGs.
//

typedef enum _EGTIMER_REGISTER {
    EgTimerControl = 0,
    EgTimerReloadValue = 1,
    EgTimerCurrentCount = 2,
    EgTimerInterruptAcknowledge = 3
} EGTIMER_REGISTER, *PEGTIMER_REGISTER;

//
// Define the format of the private data structure. The TimerIndex member
// allows the extension to pass the same functions for all timer an identify
// which timer is being referred to.
//

typedef struct _EGTIMER_DATA {
    ULONG TimerIndex;
} EGTIMER_DATA, *PEGTIMER_DATA;

//
// --------------------------------------------------------------------- Macros
//

//
// The following macros are used to read from and write to the timer. The first
// parameter is the offset from the entire timer block where this timer is
// found (which timer), and the second parameter is the register to read or
// write. For the write function, the third parameter is the value to write.
// READ_REGISTER_ULONG and WRITE_REGISTER_ULONG should always be used to
// ensure that the proper barriers and flushes are in place for doing direct
// hardware accesses.
//

#define READ_EGTIMER(_TimerIndex, _Register)           \
    READ_REGISTER_ULONG((PULONG)((PUCHAR)EgTimerBase + \
                                 ((_TimerIndex) * EGTIMER_SIZE)) + (_Register))

#define WRITE_EGTIMER(_TimerIndex, _Register, _Value)                   \
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)EgTimerBase +                 \
                                  ((_TimerIndex) * EGTIMER_SIZE)) +     \
                                  (_Register),                          \
                                  _Value)

//
// ----------------------------------------------- Internal Function Prototypes
//

NTSTATUS
EgTimerRegister (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    );

_Function_class_(TIMER_INITIALIZE)
NTSTATUS
EgTimerInitialize (
    __in PVOID TimerData
    );

_Function_class_(TIMER_QUERY_COUNTER)
ULONGLONG
EgTimerQueryCounter (
    __in PVOID TimerData
    );

_Function_class_(TIMER_ACKNOWLEDGE_INTERRUPT)
VOID
EgTimerAcknowledgeInterrupt (
    __in PVOID TimerData
    );

_Function_class_(TIMER_ARM_TIMER)
NTSTATUS
EgTimerArm (
    __in PVOID TimerData,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    );

_Function_class_(TIMER_STOP)
VOID
EgTimerStop (
    __in PVOID TimerData
    );

//
// -------------------------------------------------------------------- Globals
//

//
// Define the physical address of the timer block. This information can either
// be hardcoded like it is here or fetched out of the CSRT resource passed to
// the extension.
//

ULONGLONG EgTimerPhysicalAddress = 0x0BADF00D;

//
// Define the GSIVs for each timer's interrupt. This is also a candidate for
// information to be retrieved out of the CSRT.
//

ULONG EgTimerGsi[EGTIMER_COUNT] = {
    32,
    33,
    34,
    35
};

//
// Define the mapped virtual address of the timer block.
//

PVOID EgTimerBase = NULL;

//
// ------------------------------------------------------------------ Functions
//

NTSTATUS
AddResourceGroup (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

/*++

Routine Description:

    This routine identifies and registers all of the Resource Descriptors
    in the specified Resource Group.

Arguments:

    Handle - Supplies the HAL Extension handle which must be passed to other
        HAL Extension APIs.

    ResourceGroup - Supplies a pointer to the Resource Group which the
        HAL Extension has been installed on.

Return Value:

    NTSTATUS code.

--*/

{

    NTSTATUS Status;

    //
    // Register the main timer block.
    //

    Status = EgTimerRegister(Handle, ResourceGroup);
    if (!NT_SUCCESS(Status)) {
        goto AddResourceGroupEnd;
    }

    Status = STATUS_SUCCESS;

AddResourceGroupEnd:
    return Status;
}

//
// --------------------------------------------------------- Internal Functions
//

NTSTATUS
EgTimerRegister (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

/*++

Routine Description:

    This routine registers the EG Timer hardware.

Arguments:

    Handle - Supplies the HAL Extension handle which must be passed to other
        HAL Extension APIs.

    ResourceGroup - Supplies a pointer to the Resource Group which the
        HAL Extension has been installed on.

Return Value:

    NT status code.

--*/

{

    EGTIMER_DATA InternalData;
    TIMER_INITIALIZATION_BLOCK NewTimer;
    PHYSICAL_ADDRESS PhysicalAddress;
    CSRT_RESOURCE_DESCRIPTOR_HEADER ResourceDescriptorHeader;
    NTSTATUS Status;
    ULONG TimerIndex;

    //
    // DEV HACK: Makeup a resource type until we get correct CSRT parsing.
    //

    ResourceDescriptorHeader.Type = CSRT_RD_TYPE_TIMER;
    ResourceDescriptorHeader.Subtype = CSRT_RD_SUBTYPE_TIMER;
    ResourceDescriptorHeader.Length = sizeof(CSRT_RESOURCE_DESCRIPTOR_HEADER);

    //
    // Register the entire timer block's address usage with the HAL. This
    // address space should be shown to the HAL as reserved even if the timer
    // is not going to be registered or used so that the system knows that
    // region of *physical* address space is occupied.
    //

    PhysicalAddress.QuadPart = EgTimerPhysicalAddress;
    Status = HalRegisterPermanentAddressUsage(PhysicalAddress,
                                              EGTIMER_BLOCK_SIZE);

    if (!NT_SUCCESS(Status)) {
        goto RegisterEnd;
    }

    //
    // Register each timer with the HAL.
    //

    for (TimerIndex = 0; TimerIndex < EGTIMER_COUNT; TimerIndex += 1) {

        //
        // Initialize the timer structure.
        //

        RtlZeroMemory(&NewTimer, sizeof(TIMER_INITIALIZATION_BLOCK));
        RtlZeroMemory(&InternalData, sizeof(EGTIMER_DATA));
        INITIALIZE_TIMER_HEADER(&NewTimer);
        NewTimer.CounterBitWidth = EGTIMER_BIT_WIDTH;
        NewTimer.CounterFrequency = EGTIMER_FREQUENCY;

        //
        // Set the pointer to the internal data and its size. The pointer can
        // be the same for each timer (and a local variable) because a *copy*
        // of this data will be made for each timer registered. This is the
        // extensions only chance to dynamically allocate memory.
        //

        NewTimer.InternalData = &InternalData;
        NewTimer.InternalDataSize = sizeof(EGTIMER_DATA);
        NewTimer.Interrupt.Mode = LevelSensitive;
        NewTimer.Interrupt.Polarity = InterruptActiveHigh;

        //
        // This must be set to indicate that this is a custom third-party timer.
        // The HAL will fail the registration if this is not set correctly.
        //

        NewTimer.KnownType = TimerUnknown;

        //
        // The timer does not support a divisor. The GSI data can be hardcoded
        // like it is here or pulled out of the resource from the CSRT table.
        //

        NewTimer.MaxDivisor = 1;
        NewTimer.Interrupt.Gsi = EgTimerGsi[TimerIndex];
        NewTimer.FunctionTable.Initialize = EgTimerInitialize;
        NewTimer.FunctionTable.QueryCounter = EgTimerQueryCounter;
        NewTimer.FunctionTable.AcknowledgeInterrupt =
                                                   EgTimerAcknowledgeInterrupt;

        NewTimer.FunctionTable.ArmTimer = EgTimerArm;
        NewTimer.FunctionTable.Stop = EgTimerStop;
        NewTimer.Capabilities = TIMER_COUNTER_READABLE |
                                TIMER_ONE_SHOT_CAPABLE |
                                TIMER_PERIODIC_CAPABLE |
                                TIMER_GENERATES_LINE_BASED_INTERRUPTS;

        InternalData.TimerIndex = TimerIndex;
        ResourceDescriptorHeader.Uid = TimerIndex;
        Status = RegisterResourceDescriptor(Handle,
                                            ResourceGroup,
                                            &ResourceDescriptorHeader,
                                            &NewTimer);

        if (!NT_SUCCESS(Status)) {
            goto RegisterEnd;
        }
    }

    Status = STATUS_SUCCESS;

RegisterEnd:
    return Status;
}

_Function_class_(TIMER_INITIALIZE)
NTSTATUS
EgTimerInitialize (
    __in PVOID TimerData
    )

/*++

Routine Description:

    This routine is responsible for initializing the timer hardware. This is
    guaranteed to be the first timer routine called by the HAL. It must prepare
    the timer for use by beginning the timer's counter ticking if the counter
    is readable, setting the intial input clock divisor to 1 if applicable,
    and masking all interrupts. If the timer's stop routine is called, this
    routine will be called before the timer is queried or armed again. It will
    not be called between every rearming of the timer.

    This routine will not be called concurrently with any other calls to
    this HAL Timer extension. For per-processor timers, this routine will be
    called once on each processor. A failure on any processor blocks the timer's
    use on all processors.

Arguments:

    TimerData - Supplies a pointer to the timer's private context. The contents
        of this pointer were specified when the timer was initially registered,
        and may be modified inside this routine. The HAL does not interpret any
        data deferenced from this pointer.

Return Value:

    Returns an NT status code indicating success or failure. If a successful
    status code is returned then the HAL may subsequently call further routines
    in this HAL extension to query or arm the timer. If a failure code is
    returned, this HAL extension will not attempt to use this timer unless the
    Initialize routine is called again and succeeds.

--*/

{

    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG RegisterValue;
    NTSTATUS Status;
    PEGTIMER_DATA Timer;

    Timer = (PEGTIMER_DATA)TimerData;

    //
    // Map the timer if no one has done that yet.
    //

    if (EgTimerBase == NULL) {
        PhysicalAddress.QuadPart = EgTimerPhysicalAddress;
        EgTimerBase = HalMapIoSpace(PhysicalAddress,
                                    EGTIMER_BLOCK_SIZE,
                                    MmNonCached);

        if (EgTimerBase == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto InitializeEnd;
        }
    }

    //
    // Start the counter ticking in free running mode, and mask all interrupts.
    // The counter doesn't necessarily need to be reset to 0.
    //

    WRITE_EGTIMER(Timer->TimerIndex, EgTimerReloadValue, 0);
    RegisterValue = EGTIMER_CONTROL_ENABLE | EGTIMER_CONTROL_PERIODIC;
    WRITE_EGTIMER(Timer->TimerIndex, EgTimerControl, RegisterValue);
    Status = STATUS_SUCCESS;

InitializeEnd:
    return Status;
}

_Function_class_(TIMER_QUERY_COUNTER)
ULONGLONG
EgTimerQueryCounter (
    __in PVOID TimerData
    )

/*++

Routine Description:

    This routine queries the timer hardware and retrieves the current counter
    value.

    Timers are assumed to always count *up*. If the actual timer hardware counts
    down, then this routine should subtract the current count from the maximum
    counter value so that values appear to count up. This routine may be called
    concurrently on multiple processors and must be reentrant. This routine is
    extremely performance sensitive, as it may be used to back the system
    performance counter.

Arguments:

    TimerData - Supplies a pointer to the timer's private context, whose
        initial content was supplied when the timer was registered.

Return Value:

    Returns the hardware's current count.

--*/

{

    PEGTIMER_DATA Timer;

    Timer = (PEGTIMER_DATA)TimerData;

    return READ_EGTIMER(Timer->TimerIndex, EgTimerCurrentCount);
}

_Function_class_(TIMER_ACKNOWLEDGE_INTERRUPT)
VOID
EgTimerAcknowledgeInterrupt (
    __in PVOID TimerData
    )

/*++

Routine Description:

    This routine performs any actions necessary to acknowledge and quiesce a
    timer interrupt. For per-processor timers, this routine may be called
    concurrently on multiple processors. This routine will be called on every
    timer interrupt at the hardware priority level of that interrupt, so this
    routine is extremely performance sensitive. For timers running in
    pseudo-periodic mode, this routine must rearm the timer for the same
    interval as it was armed with without introducing delay into the interrupt
    interval. Only deadline-based timers support pseudo-periodic mode.

Arguments:

    TimerData - Supplies a pointer to the timer's private context, whose
        initial content was supplied when the timer was registered.

Return Value:

    None.

--*/

{

    PEGTIMER_DATA Timer;

    Timer = (PEGTIMER_DATA)TimerData;

    //
    // Acknowledge the interrupt by writing to the interrupt acknowledge
    // register.
    //

    WRITE_EGTIMER(Timer->TimerIndex, EgTimerInterruptAcknowledge, 0);
    return;
}

_Function_class_(TIMER_ARM_TIMER)
NTSTATUS
EgTimerArm (
    __in PVOID TimerData,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    )

/*++

Routine Description:

    This routine arms a timer to fire an interrupt after the given number of
    timer ticks. For timers that only interrupt on rollovers, this simply
    enables the interrupt, the tick count parameter is ignored. If the timer
    is currently armed for a different mode or tick count, this call is
    expected to replace that programming. This routine will not get called
    concurrently with other timer calls, except on per-processor timers, where
    it may get called concurrently on different processors.

Arguments:

    TimerData - Supplies a pointer to the timer's private context, whose
        initial content was supplied when the timer was registered.

    Mode - Supplies the mode to arm the timer in, which will be one of the
        modes the HAL extension advertised support for when registering the
        timer. The modes are as follows:

        OneShot - Arms the timer to fire the given number of ticks from now.
            Only one interrupt is expected to come in. The HAL does not make
            assumptions on whether or not the expiration of this interrupt
            causes the counter to stop. The Query Counter routine will not be
            called while a timer is armed to fire an interrupt.

        Periodic - Arms the timer to fire periodically with an interval of the
            given number of ticks. The first interrupt should happen
            approximately the given number of ticks from when the arm timer
            function was invoked.

        PseudoPeriodic - Arms the timer with the same functional behavior as
            periodic mode, with the knowledge that the timer will have to rearm
            itself during the acknowledge interrupt routine. This mode is
            expected to have slightly worse performance than pure periodic mode,
            but is expected to generate periodic interrupts at the exact rate
            specified.

    TickCount - Supplies the number of ticks from now that the timer should
        assert its interrupt in. For timers that are only capable of
        interrupting on rollovers from their maximum value to 0, this parameter
        is ignored.

Return Value:

    Returns and NTSTATUS code indicating success or failure. If the timer
    returns success, then the interrupt is expected to come in the specified
    number of ticks from when the function was called, with a tolerance of
    however long the function took to execute. If the routine fails, then no
    timer routines will be called again until the timer Initialize routine is
    called again and succeeds. In most cases, returning a failure code results
    in a system bugcheck.

--*/

{

    ULONG ControlRegister;
    PEGTIMER_DATA Timer;

    Timer = (PEGTIMER_DATA)TimerData;

    NT_ASSERT(TickCount != 0);
    NT_ASSERT(TickCount <= 0xFFFFFFFF);
    NT_ASSERT(Mode != TimerModePseudoPeriodic);

    //
    // This will never occur.
    //

    if ((TickCount > 0xFFFFFFFF) || (TickCount == 0)) {
        return STATUS_INVALID_PARAMETER;
    }

    ControlRegister = EGTIMER_CONTROL_ENABLE | EGTIMER_CONTROL_INTERRUPT_ENABLE;
    if (Mode == TimerModePeriodic) {
        ControlRegister |= EGTIMER_CONTROL_PERIODIC;
    }

    //
    // Disable the timer while it's being programmed to avoid spurious
    // interrupts.
    //

    WRITE_EGTIMER(Timer->TimerIndex, EgTimerControl, 0);

    //
    // Write the reload register to set both the current count value and the
    // reload value if the interrupt is periodic.
    //

    WRITE_EGTIMER(Timer->TimerIndex,
                  EgTimerReloadValue,
                  0 - (ULONG)TickCount);

    //
    // Enable the timer.
    //

    WRITE_EGTIMER(Timer->TimerIndex, EgTimerControl, ControlRegister);
    return STATUS_SUCCESS;
}

_Function_class_(TIMER_STOP)
VOID
EgTimerStop (
    __in PVOID TimerData
    )

/*++

Routine Description:

    This routine stops a timer from ticking. After this function returns, the
    timer should not generate any more interrupts, and reads to its counter
    might return the same value every time.

Arguments:

    TimerData - Supplies a pointer to the timer's private context, whose
        initial content was supplied when the timer was registered.

Return Value:

    None, this function must succeed.

--*/

{

    PEGTIMER_DATA Timer;

    Timer = (PEGTIMER_DATA)TimerData;

    //
    // All that technically needs to be done to stop the timer from firing is
    // to clear the interrupt enable bit. Stopping the timer entirely works too.
    //

    WRITE_EGTIMER(Timer->TimerIndex, EgTimerControl, 0);
    return;
}

