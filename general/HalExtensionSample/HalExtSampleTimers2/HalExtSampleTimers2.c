/*++

Copyright (c) 2011 Microsoft Corporation

Module Name:

    HalExtSampleTimers2.c

Abstract:

    This file implements a HAL Extension Module for the fictitious EG2Timer.

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
// The EG2Timer is a fast-access low latency deadline-based timer designed
// for high-performance timekeeping operations. It consists of one counter
// counter and three match registers against that counter. Each match register
// consists of a 32-bit value that is compared on each clock cycle to the
// current count value. If the values match and the match register is enabled
// for generating an interrupt, an edge triggered interrupt will be fired. The
// register can also be configured with an "interval" value such that the
// match register is automatically moved forward by the given interval to
// generate periodic interrupts. The main counter is 32 bits and runs at 15MHz.
// The register layout of the timer block appears below.
//
// Offset   Register                   Size
// 0x00     GlobalControl              4
// 0x04     CounterValue               4
// 0x08     Timer0_Match               4
// 0x0C     Timer0_Interval            4
// 0x10     Timer0_Control             4
// 0x14     Timer1_Match               4
// 0x18     Timer1_Interval            4
// 0x1C     Timer1_Control             4
// 0x20     Timer2_Match               4
// 0x24     Timer2_Interval            4
// 0x28     Timer2_Control             4
//
// Register Descriptions:
//
// GlobalControl - Controls global state relating to the timer block. On reset,
//     this register's value is 0. This register is Read/Write.
//
//     Bit    Function
//     31-1   Reserved. Read as 0, always write 0 to maintain future
//                compatibility.
//
//     0      Enabled. When set to 1, the main counter is enabled and will
//                count. When set to 0, the main counter is disabled and will
//                not run.
//
// CounterValue - A Read/Write register containing the current value of the
//     counter. Writes to this register must be done with caution as they do
//     not alter or adjust the contents of the match registers. On reset, the
//     value of this register is 0.
//
// TimerN_Match - A Read/Write register that contains the match value that this
//     interrupt is primed against. When the global counter equals the match
//     value, an interrupt will be generated. If the interrupt is set for
//     periodic mode, the Interval value will be automatically added to the
//     match value when the interrupt occurs. On reset, the value of this
//     register is 0.
//
// TimerN_Interval - A Read/Write register that contains the periodic interval
//     to add to the match register if the interrupt is armed for periodic
//     mode. On reset, the value of this register is 0.
//
// TimerN_Control - A Read/Write register containing a bitfield that controls
//     the behavior of the match register and associated interrupt. On reset,
//     the value of this register is 0.
//
//     Bit    Function
//     31-2   Reserved. Read as 0, always write 0 to maintain future
//                compatibility.
//
//     1      Periodic. When set to 1, the value in the Interval register will
//                automatically be added to the value in the Match register and
//                written back to the Match register when a match occurs. When
//                set to 0, the Match register will not change when a match
//                occurs.
//
//     0      InterruptEnable. When set to 1, an interrupt will be generated
//                when a match occurs. When set to 0, no interrupt will be
//                generated when a match occurs. The periodic bit is still live
//                however, the Match register will continue to get accumulated
//                with the Interval register on matches.
//

//
// ---------------------------------------------------------------- Definitions
//

//
// Define the total size of the register block, which is 1 page.
//

#define EG2TIMER_BLOCK_SIZE 0x1000

//
// Define the size of one timer's match register block, in ULONGs.
//

#define EG2MATCH_SIZE 3

//
// Define the total number of match registers.
//

#define EG2MATCH_COUNT 3

//
// Define the timer's bit width.
//

#define EG2TIMER_BIT_WIDTH 32

//
// Define the counter's frequency, in Hertz.
//

#define EG2TIMER_FREQUENCY 15000000

//
// Define the global control bits.
//

#define EG2TIMER_GLOBAL_CONTROL_ENABLE    0x00000001

//
// Define bits for the control register.
//

#define EG2TIMER_MATCH_INTERRUPT_ENABLE 0x00000001
#define EG2TIMER_MATCH_PERIODIC         0x00000002

//
// Define the special offset used to indicate this timer is the counter
// itself.
//

#define EG2TIMER_COUNTER_OFFSET 0xFFFFFFFF

//
// ------------------------------------------------------ Data Type Definitions
//


//
// Define the registers and their offsets, in ULONGs.
//

typedef enum _EG2TIMER_REGISTER {
    Eg2TimerGlobalControl = 0,
    Eg2TimerCurrentCount = 1,
    Eg2TimerMatch = 2,
    Eg2TimerInterval = 3,
    Eg2TimerControl = 4
} EG2TIMER_REGISTER, *PEG2TIMER_REGISTER;

//
// Define the format of the private data structure. The offset member stores the
// offset to the match register, in ULONGs. The value 0 is reserved for the
// global counter.
//

typedef struct _EG2TIMER_DATA {
    ULONG Offset;
    TIMER_MODE Mode;
    ULONG Period;
} EG2TIMER_DATA, *PEG2TIMER_DATA;

//
// --------------------------------------------------------------------- Macros
//

//
// The following macros are used to read from and write to the timer. The first
// parameter is the offset in ULONGs to apply to the requested register. The
// second parameter is the register to read or write. For write functions, the
// third parameter is the value to write.
//
// READ_REGISTER_ULONG and WRITE_REGISTER_ULONG should always be used to
// ensure that the proper barriers and flushes are in place for doing direct
// hardware accesses.
//

#define READ_EG2TIMER(_TimerOffset, _Register)           \
    READ_REGISTER_ULONG((PULONG)Eg2TimerBase + (_TimerOffset) + (_Register))

#define WRITE_EG2TIMER(_TimerOffset, _Register, _Value)                       \
    WRITE_REGISTER_ULONG((PULONG)Eg2TimerBase + (_TimerOffset) + (_Register), \
                         (_Value))

//
// ----------------------------------------------- Internal Function Prototypes
//

NTSTATUS
Eg2TimerRegister (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    );

_Function_class_(TIMER_INITIALIZE)
NTSTATUS
Eg2TimerInitialize (
    __in PVOID TimerData
    );

_Function_class_(TIMER_QUERY_COUNTER)
ULONGLONG
Eg2TimerQueryCounter (
    __in PVOID TimerData
    );

_Function_class_(TIMER_ACKNOWLEDGE_INTERRUPT)
VOID
Eg2TimerAcknowledgeInterrupt (
    __in PVOID TimerData
    );

_Function_class_(TIMER_ARM_TIMER)
NTSTATUS
Eg2TimerArm (
    __in PVOID TimerData,
    __in TIMER_MODE Mode,
    __in ULONGLONG TickCount
    );

_Function_class_(TIMER_STOP)
VOID
Eg2TimerStop (
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

ULONGLONG Eg2TimerPhysicalAddress = 0xBEEF7AC0;

//
// Define the GSIVs for each timer's interrupt. This is also a candidate for
// information to be retrieved out of the CSRT.
//

ULONG Eg2TimerGsi[EG2MATCH_COUNT] = {
    40,
    41,
    42,
};

//
// Define the mapped virtual address of the timer block.
//

PVOID Eg2TimerBase = NULL;

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

    Status = Eg2TimerRegister(Handle, ResourceGroup);
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
Eg2TimerRegister (
    __in ULONG Handle,
    __in PCSRT_RESOURCE_GROUP_HEADER ResourceGroup
    )

/*++

Routine Description:

    This routine registers the EG2 Timer hardware.

Arguments:

    Handle - Supplies the HAL Extension handle which must be passed to other
        HAL Extension APIs.

    ResourceGroup - Supplies a pointer to the Resource Group which the
        HAL Extension has been installed on.

Return Value:

    NT status code.

--*/

{

    EG2TIMER_DATA InternalData;
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

    PhysicalAddress.QuadPart = Eg2TimerPhysicalAddress;
    Status = HalRegisterPermanentAddressUsage(PhysicalAddress,
                                              EG2TIMER_BLOCK_SIZE);

    if (!NT_SUCCESS(Status)) {
        goto RegisterEnd;
    }

    //
    // Register the main counter as a non-interrupt generating timer, as it can
    // be used completely independently of the match registers as long as it
    // is never written to.
    //

    RtlZeroMemory(&NewTimer, sizeof(TIMER_INITIALIZATION_BLOCK));
    RtlZeroMemory(&InternalData, sizeof(EG2TIMER_DATA));
    INITIALIZE_TIMER_HEADER(&NewTimer);
    NewTimer.CounterBitWidth = EG2TIMER_BIT_WIDTH;
    NewTimer.CounterFrequency = EG2TIMER_FREQUENCY;

    //
    // Set the pointer to the internal data and its size. The pointer can
    // be the same for each timer (and a local variable) because a *copy*
    // of this data will be made for each timer registered. This is the
    // extensions only chance to dynamically allocate memory.
    //

    NewTimer.InternalData = &InternalData;
    NewTimer.InternalDataSize = sizeof(EG2TIMER_DATA);
    NewTimer.Interrupt.Mode = Latched;
    NewTimer.Interrupt.Polarity = InterruptActiveHigh;

    //
    // This must be set to indicate that this is a custom third-party timer.
    // The HAL will fail the registration if this is not set correctly.
    //

    NewTimer.KnownType = TimerUnknown;

    //
    // The timer does not support a divisor. The GSI data can be hardcoded
    // like it is here or pulled out of the resource from the CSRT table.
    // Filling in extra functions doesn't hurt as the HAL will never call
    // anything but Initialize and QueryCounter on timers that don't
    // generate interrupts.
    //

    NewTimer.MaxDivisor = 1;
    NewTimer.FunctionTable.Initialize = Eg2TimerInitialize;
    NewTimer.FunctionTable.QueryCounter = Eg2TimerQueryCounter;
    NewTimer.FunctionTable.AcknowledgeInterrupt = Eg2TimerAcknowledgeInterrupt;
    NewTimer.FunctionTable.ArmTimer = Eg2TimerArm;
    NewTimer.FunctionTable.Stop = Eg2TimerStop;
    NewTimer.Capabilities = TIMER_COUNTER_READABLE;
    InternalData.Offset = EG2TIMER_COUNTER_OFFSET;
    ResourceDescriptorHeader.Uid = EG2TIMER_COUNTER_OFFSET;
    Status = RegisterResourceDescriptor(Handle,
                                        ResourceGroup,
                                        &ResourceDescriptorHeader,
                                        &NewTimer);

    if (!NT_SUCCESS(Status)) {
        goto RegisterEnd;
    }


    //
    // Register each match register as a separate non-readable timer with the
    // HAL. Since this timer is deadline-based, it can do pseudo-periodic
    // mode and lossless rate transitions.
    //

    NewTimer.Capabilities = TIMER_ONE_SHOT_CAPABLE |
                            TIMER_PERIODIC_CAPABLE |
                            TIMER_PSEUDO_PERIODIC_CAPABLE |
                            TIMER_GENERATES_LINE_BASED_INTERRUPTS;

    for (TimerIndex = 0; TimerIndex < EG2MATCH_COUNT; TimerIndex += 1) {
        NewTimer.Interrupt.Gsi = Eg2TimerGsi[TimerIndex];
        InternalData.Offset = EG2MATCH_SIZE * TimerIndex;
        ResourceDescriptorHeader.Uid = EG2MATCH_SIZE * TimerIndex;
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
Eg2TimerInitialize (
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
    NTSTATUS Status;
    PEG2TIMER_DATA Timer;

    Timer = (PEG2TIMER_DATA)TimerData;

    //
    // Map the timer if no one has done that yet.
    //

    if (Eg2TimerBase == NULL) {
        PhysicalAddress.QuadPart = Eg2TimerPhysicalAddress;
        Eg2TimerBase = HalMapIoSpace(PhysicalAddress,
                                     EG2TIMER_BLOCK_SIZE,
                                     MmNonCached);

        if (Eg2TimerBase == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto InitializeEnd;
        }
    }

    //
    // Start the counter ticking in free running mode, and mask all interrupts.
    // The counter must *not* be reset here, otherwise an Initialize call on the
    // counter would affect the match register timers, which as far as the HAL
    // is concerned are completely independent from one another.
    //

    WRITE_EG2TIMER(0, Eg2TimerGlobalControl, EG2TIMER_GLOBAL_CONTROL_ENABLE);
    if (Timer->Offset != EG2TIMER_COUNTER_OFFSET) {
        WRITE_EG2TIMER(Timer->Offset, Eg2TimerControl, 0);
    }

    Status = STATUS_SUCCESS;

InitializeEnd:
    return Status;
}

_Function_class_(TIMER_QUERY_COUNTER)
ULONGLONG
Eg2TimerQueryCounter (
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

    PEG2TIMER_DATA Timer;

    Timer = (PEG2TIMER_DATA)TimerData;

    NT_ASSERT(Timer->Offset == EG2TIMER_COUNTER_OFFSET);

    return READ_EG2TIMER(Timer->Offset, Eg2TimerCurrentCount);
}

_Function_class_(TIMER_ACKNOWLEDGE_INTERRUPT)
VOID
Eg2TimerAcknowledgeInterrupt (
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

    ULONG MatchValue;
    PEG2TIMER_DATA Timer;

    //
    // No action is necessary here as far as acknowledging the interrupt.
    // If the current mode is pseudo-periodic, the next interrupt must be
    // armed now.
    //

    Timer = (PEG2TIMER_DATA)TimerData;

    NT_ASSERT(Timer->Offset != EG2TIMER_COUNTER_OFFSET);
    NT_ASSERT(Timer->Mode != TimerModeInvalid);

    if (Timer->Mode == TimerModePseudoPeriodic) {

        NT_ASSERT(Timer->Period != 0);

        //
        // Read the deadline that just passed, add the period, and then write
        // the new deadline.
        //

        MatchValue = READ_EG2TIMER(Timer->Offset, Eg2TimerMatch);
        MatchValue += Timer->Period;
        WRITE_EG2TIMER(Timer->Offset, Eg2TimerMatch, MatchValue);
    }

    return;
}

_Function_class_(TIMER_ARM_TIMER)
NTSTATUS
Eg2TimerArm (
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
    ULONG MatchValue;
    PEG2TIMER_DATA Timer;

    Timer = (PEG2TIMER_DATA)TimerData;

    NT_ASSERT(TickCount != 0);
    NT_ASSERT(TickCount <= 0xFFFFFFFF);
    NT_ASSERT(Timer->Offset != EG2TIMER_COUNTER_OFFSET);

    //
    // This will never occur.
    //

    if ((TickCount > 0xFFFFFFFF) || (TickCount == 0)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Disable the timer while it's being programmed to avoid spurious
    // interrupts.
    //

    WRITE_EG2TIMER(Timer->Offset, Eg2TimerControl, 0);
    ControlRegister = EG2TIMER_MATCH_INTERRUPT_ENABLE;

    //
    // For periodic mode, set the periodic interval register.
    //

    if (Mode == TimerModePeriodic) {
        ControlRegister |= EG2TIMER_MATCH_PERIODIC;
        WRITE_EG2TIMER(Timer->Offset, Eg2TimerInterval, (ULONG)TickCount);
    }

    Timer->Mode = Mode;
    Timer->Period = (ULONG)TickCount;

    //
    // Calculate and write in the first match value.
    //

    MatchValue = READ_EG2TIMER(Timer->Offset, Eg2TimerCurrentCount);
    MatchValue += (ULONG)TickCount;
    WRITE_EG2TIMER(Timer->Offset, Eg2TimerMatch, MatchValue);

    //
    // Enable the interrupt.
    //

    WRITE_EG2TIMER(Timer->Offset, Eg2TimerControl, ControlRegister);
    return STATUS_SUCCESS;
}

_Function_class_(TIMER_STOP)
VOID
Eg2TimerStop (
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

    PEG2TIMER_DATA Timer;

    Timer = (PEG2TIMER_DATA)TimerData;

    //
    // Clear the interrupt enable bit.
    //

    WRITE_EG2TIMER(Timer->Offset, Eg2TimerControl, 0);
    return;
}
