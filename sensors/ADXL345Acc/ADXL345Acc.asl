// Steps to update the SSDT:
// 0a. Copy ASL.exe to your development board 
// 0b. Open a command prompt as administrator and navigate to the directory where you have copied ASL.exe
// 1. Decompile the SSDT ("asl.exe /tab=ssdt")
// 2. Open ssdt.asl in a text editor
// 3. Insert a scope entry (e.g. Scope(_SB_) { ADXL345 device node goes here })
// 4. Compile ssdt.asl ("asl.exe ssdt.asl")
// 5. Load updated SSDT ("asl /loadtable ssdt.aml")
// 6. Restart your development board

// ADXL345 accelerometer device node
Device(SPBA)
{
    Name(_HID, "ADXL345Acc")
    Name(_UID, 1)

    Method(_CRS, 0x0, NotSerialized)
    {
        Name(RBUF, ResourceTemplate()
        {          
            I2CSerialBus(0x53, ControllerInitiated, 400000, AddressingMode7Bit, "\\_SB.I2C3", 0, ResourceConsumer) 
            GpioInt(Edge, ActiveHigh, Exclusive, PullDown, 0, "\\_SB.GPO2") {0x17}
        })
        Return(RBUF)
    } // Method (_CRS ...)

    Method(_DSM, 0x4, NotSerialized)
    {
        If(LEqual(Arg0, Buffer(0x10)
        {
            0x1e, 0x54, 0x81, 0x76, 0x27, 0x88, 0x39, 0x42, 0x8d, 0x9d, 0x36, 0xbe, 0x7f, 0xe1, 0x25, 0x42
        }))
        {
            If(LEqual(Arg2, Zero))
            {
                Return(Buffer(One)
                {
                    0x03
                })
            }
            If(LEqual(Arg2, One))
            {
                Return(Buffer(0x4)
                {
                    0x00, 0x01, 0x02, 0x03
                })
            }
        }
        Else
        {
            Return(Buffer(One)
            {
                0x00
            })
        }
    } // Method(_DSM ...)
} // Device(SPBA)
