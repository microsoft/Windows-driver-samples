//
// Compile with:
//   asl.exe sample.asl
//
// Copy ACPITABL.dat to %windir%\system32, turn on testsigning, and reboot.
//

DefinitionBlock("ACPITABL.dat", "SSDT", 5, "MSFT", "TCPCI", 1)
{
    Scope(\_SB)
    {
//      // TODO: Power resource for the reset power rail. This is used for platform-level device reset.
//      // Each of the devices that use this power resource must declare it.
//      PowerResource(PWFR, 0x5, 0x0)
//      {
//          Method(_RST, 0x0, NotSerialized)    { }
//          // TODO: Placeholder methods as power resources need _ON, _OFF, _STA.
//          Method(_STA, 0x0, NotSerialized)
//          {
//              Return(0xF)
//          }
//
//          Method(_ON_, 0x0, NotSerialized)    { }
//
//          Method(_OFF, 0x0, NotSerialized)    { }
//
//      } // PowerResource()

        // UCM-TCPCI device. Can be named anything.
        Device(USBC)
        {
            // This device needs to be enumerated by ACPI, so it needs a HWID.
            // Your INF should match on it.
            Name(_HID, "USBC0001")
            Method(_CRS, 0x0, NotSerialized)
            {
                Name (RBUF, ResourceTemplate ()
                {
                    //
                    // Sample I2C and GPIO resources. TODO: Modify to match your
                    // platform's underlying controllers and connections.
                    // \_SB.I2C and \_SB.GPIO are paths to predefined I2C
                    // and GPIO controller instances.
                    //
                    I2CSerialBus(0x50, ControllerInitiated, 400000, AddressingMode7Bit, "\\_SB.I2C1")
                    GpioInt(Level, ActiveLow, Exclusive, PullDown, 0, "\\_SB.GPI0") {5}

                })
                Return(RBUF)
            }

 //         // Declare PWFR as the reset power rail.
 //         // This is used for platform-level device reset.
 //         // https://msdn.microsoft.com/en-us/library/windows/hardware/dn928420(v=vs.85).aspx
 //         Name(_PRR, Package(One)
 //         {
 //             \_SB.PWFR
 //         })

            // Inside the scope of the UCM-TCPCI device, you need to define one "connector" device.
            // It can be named anything.
            Device(CON0)
            {
                // This device is not meant to be enumerated by ACPI, hence you should not assign a
                // HWID to it. Instead, use _ADR to assign address 0 to it.
                Name(_ADR, 0x00000000)

                // _PLD as defined in the ACPI spec. The GroupToken and GroupPosition are used to
                // derive a unique "Connector ID". This PLD should correlate with the PLD associated
                // with the XHCI device for the same port.
                Name(_PLD, Package()
                {
                    Buffer()
                    {
                        0x82,                   // Revision 2, ignore color.
                        0x00,0x00,0x00,         // Color (ignored).
                        0x00,0x00,0x00,0x00,    // Width and height.
                        0x69,                   // User visible; Back panel; VerticalPos:Center.
                        0x0c,                   // HorizontalPos:0; Shape:Vertical Rectangle; GroupOrientation:0.
                        0x80,0x00,              // Group Token:0; Group Position:1; So Connector ID is 1.
                        0x00,0x00,0x00,0x00,    // Not ejectable.
                        0xFF,0xFF,0xFF,0xFF     // Vert. and horiz. offsets not supplied.
                    }
                })

                // _UPC as defined in the ACPI spec.
                Name(_UPC, Package()
                {
                    0x01,                       // Port is connectable.
                    0x09,                       // Connector type: Type C connector - USB2 and SS with switch.
                    0x00000000,                 // Reserved0 must be zero.
                    0x00000000                  // Reserved1 must be zero.
                })

                Name(_DSD, Package()
                {
                    // The UUID for Type-C connector capabilities.
                    ToUUID("6b856e62-40f4-4688-bd46-5e888a2260de"),

                    // The data structure which contains the connector capabilities. Each package
                    // element contains two elements: the capability type ID, and the capability data
                    // (which depends on the capability type). Note that any information defined here
                    // will override similar information described by the driver itself. For example, if
                    // the driver claims the port controller is DRP-capable, but ACPI says it is UFP-only
                    // ACPI will take precedence.
                    Package()
                    {
                        Package() {1, 4},      // Supported operating modes (DRP).
                        Package() {2, 1},      // Supported Type-C sourcing capabilities (DefaultUSB).
                        Package() {3, 0},      // Audio accessory capable (False).
                        Package() {4, 1},      // Is PD supported (True).
                        Package() {5, 3},      // Supported power roles (Sink and Source).
                        Package()
                        {
                            6,                  // Capability type ID of PD Source Capabilities.
                            Package()
                            {
                                0x0001905A      // Source PDO #0: Fixed:5V, 900mA. No need to describe fixed bits.
                            }
                        },
                        Package()
                        {
                            7,                  // Capability type ID of PD Sink Capabilities.
                            Package ()
                            {
                                0x00019096      // Sink PDO #0: Fixed:5V, 1.5A. No need to describe fixed bits.
                            }
                        },
                        Package()
                        {
                            8,                  // Capability type ID of supported PD Alternate Modes.
                            // TODO: If your device supports alternate modes, update this with the SVID
                            // and Mode of the alternate modes.
                            // If your device does not support any alternate modes, then you may delete this
                            // package with ID #8. UcmTcpciCx will not enter an alternate mode if this
                            // package is not present.
                            Package()
                            {
                                0x1111, 0x22222222
                            }
                        }
                    }
                })
            } // Device(CON0)
        } // Device(USBC)
    } // Scope(\_SB)
} // DefinitionBlock
