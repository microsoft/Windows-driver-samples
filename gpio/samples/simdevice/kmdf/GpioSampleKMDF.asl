
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    GpioSample.asl

Abstract:

    This sample ASL file describes a sample GPIO device and a sample peripheral
    device (managed by the sample KMDF device driver) which consumes IO and
    interrupt resources from the GPIO device. Please note that:

    1. The memory and IO descriptor under the GPIO device are simply examples of
       what can be described. They are commented out to illustrate this point. Actual
       values will vary according to the platform specifications (e.g. GIC and memory)

    2. The sample ASL DSDT definition block defines only the components relevant
       to demonstrate GPIO IO and interrupt resource usage. Rest of the DSDT will vary
       according to platform specifications.

    3. The PNP IDs for the GPIO and peripheral device are for demonstration purposes
       only. Actual values need to reflect those chosen for the actual GPIO or
       peripheral device.

    This ASL file is identical to the UMDF one except for the PNP ID used for the 
    sample peripheral device and GPIO IO/interrupt pins used in its _CRS.

--*/


DefinitionBlock ("DSDT.AML", "DSDT", 0x02, "MSFT", "SAMPLE", 0x1) {

  //
  //  System Bus
  //

  Scope (\_SB) {

    //
    // Sample GPIO device
    //

    Device(GPIO) {
       Name (_ADR, 0)
       Name (_HID, "TEST0001")
       Name (_CID, "TEST0001")
       Name(_UID, 4)

       Method (_CRS, 0x0, NotSerialized) {
         Name (RBUF, ResourceTemplate () {

           //
           // Interrupt resource. In this example, banks 0 & 1 share the same
           // interrupt to the parent controller and similarly banks 2 & 3.
           //
           // N.B. The definition below is chosen for an arbitrary
           //      test platform. It needs to be changed to reflect the hardware
           //      configuration of the actual platform.
           //

           Interrupt(ResourceConsumer, Level, ActiveHigh, Shared, , , ) {50}
           Interrupt(ResourceConsumer, Level, ActiveHigh, Shared, , , ) {50}
           Interrupt(ResourceConsumer, Level, ActiveHigh, Shared, , , ) {51}
           Interrupt(ResourceConsumer, Level, ActiveHigh, Shared, , , ) {51}

           //
           // Memory resource. The definition below is chosen for an arbitrary
           // test platform. It needs to be changed to reflect the hardware
           // configuration of the actual platform.
           //

           Memory32Fixed(ReadWrite, 0x00100000, 0x18)
        })

        Return (RBUF)
       }

       Method (_STA, 0x0, NotSerialized) {
         Return(0xf)
       }

       //
       // Sample peripheral device
       //

       Device (TDEV) {
         Name (_ADR, 0)
         Name (_HID, "TEST0003")
         Name (_CID, "TEST0003")
         Name (_UID, 1)

         Method (_CRS, 0x0, NotSerialized) {
           Name (RBUF, ResourceTemplate () {

             //
             // GPIO Interrupt Resources
             //

             GpioInt(Edge, ActiveHigh, Shared, PullUp, 0, "\\_SB.GPIO", 0, ResourceConsumer,, RawDataBuffer() {1}) {1}

             //
             // GPIO IO Resources
             //

             GpioIo(Exclusive, PullUp, 0, 0,, "\\_SB.GPIO",0, ResourceConsumer, , RawDataBuffer() {1}) {10}
             GpioIo(Exclusive, PullUp, 0, 0,, "\\_SB.GPIO",0, ResourceConsumer, , RawDataBuffer() {1}) {11}
           })

           Return (RBUF)
         }

         Method (_STA, 0x0, NotSerialized) {
             Return(0xf)
         }
       }
     }
   }
 }
