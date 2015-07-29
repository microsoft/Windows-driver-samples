//
// Sample ASL code to demonstrate WMI query, set, method and event operations
//
// Insert this code at an appropriate place in the bios ASL source, rebuild
// the DSDT and replace the original DSDT via the registry or reflashing.
// NT should recognize the pnp0c14 device and by the magic of pnp install 
// wmiacpi.sys and bind it to this ASL. Note that you can have as many
// devices as needed, however each device must have a unique _UID.
//


            Device(AMW0)
                    {
                       // pnp0c14 is pnp id assigned to WMI mapper
                       Name(_HID, "*pnp0c14")
                       Name(_UID, 0x0)

//
// Description of data, events and methods supported by this ASL device.
                       Name(_WDG, Buffer() {

//
// Query - Set Guids.
//
                           // {ABBC0F6a-8EA1-11d1-00A0-C90629100000}
                           // Query - Set Package Guid
                           0x6a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 65,        // Object Id (AA)
                           4,             // Instance Count
                           0x01,          // Flags (WMIACPI_REGFLAG_EXPENSIVE)

                           // {ABBC0F6b-8EA1-11d1-00A0-C90629100000}
                           // Query - Set String Guid
                           0x6b, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 66,        // Object Id (AB)
                           4,             // Instance Count
                           0x05,          // Flags (WMIACPI_REGFLAG_EXPENSIVE |
                                          //        WMIACPI_REGFLAG_STRING)

                           // Query - Set ULONG Guid
                           // {ABBC0F6c-8EA1-11d1-00A0-C90629100000}
                           0x6c, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 67,        // Object Id (AC)
                           4,             // Instance Count
                           0x01,          // Flags (WMIACPI_REGFLAG_EXPENSIVE)


//
// Method Guids
//    Method Id 1 is get with no input
//    Method Id 2 is set with no output
//    Method Id 3 is fire event

                           // Package
                           // {ABBC0F6d-8EA1-11d1-00A0-C90629100000}
                           0x6d, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 65,        // Object Id (BA)
                           4,             // Instance Count
                           0x02,          // Flags (WMIACPI_REGFLAG_METHOD)

                           // String
                           // {ABBC0F6e-8EA1-11d1-00A0-C90629100000}
                           0x6e, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 66,        // Object Id (BB)
                           4,             // Instance Count
                           0x06,          // Flags (WMIACPI_REGFLAG_METHOD)
                                          //        WMIACPI_REGFLAG_STRING)

                           // ULONG
                           // {ABBC0F6f-8EA1-11d1-00A0-C90629100000}
                           0x6f, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 67,        // Object Id (BC)
                           4,             // Instance Count
                           0x02,          // Flags (WMIACPI_REGFLAG_METHOD)



//
// Event Guids

                           // Package
                           // {ABBC0F70-8EA1-11d1-00A0-C90629100000}
                           0x70, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xd0, 0,       // Notification Id
                           1,             // Instance Count
                           0x08,          // Flags (WMIACPI_REGFLAG_EVENT)

                           // String
                           // {ABBC0F71-8EA1-11d1-00A0-C90629100000}
                           0x71, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xd1, 0,       // Notification Id
                           1,             // Instance Count
                           0x0c,          // Flags (WMIACPI_REGFLAG_EVENT)
                                          //        WMIACPI_REGFLAG_STRING)

                           // ULONG
                           // {ABBC0F72-8EA1-11d1-00A0-C90629100000}
                           0x72, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xd2, 0,       // Notification Id
                           1,             // Instance Count
                           0x08           // Flags (WMIACPI_REGFLAG_EVENT)

                       })


                       // Storage for 4 instances of Package
                       Name(SAA0, Buffer(0x10)
                       {
                           1,0,0,0, 2,0,0,0, 3,0,0,0, 4,0,0,0
                       })

                       Name(SAA1, Buffer(0x10)
                       {
                           1,0,0,0, 2,0,0,0, 3,0,0,0, 4,0,0,0
                       })

                       Name(SAA2, Buffer(0x10)
                       {
                           1,0,0,0, 2,0,0,0, 3,0,0,0, 4,0,0,0
                       })

                       Name(SAA3, Buffer(0x10)
                       {
                           1,0,0,0, 2,0,0,0, 3,0,0,0, 4,0,0,0
                       })


                       // Storage for 4 instances of string
                       // Maximum length for string is 80 chars
                       Name(SAB0, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()1234567890-_=+[]{}")

                       Name(SAB1, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()1234567890-_=+[]{}")

                       Name(SAB2, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()1234567890-_=+[]{}")

                       Name(SAB3, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()1234567890-_=+[]{}")

                       // Storage for 4 instances of ULONG
                       Name(SAC0, 0)
                       Name(SAC1, 1)
                       Name(SAC2, 2)
                       Name(SAC3, 3)


                       // Collection Control flags
                       Name(CCAA, 0)
                       Name(CCAB, 0)
                       Name(CCAC, 0)

                       // Event Control flags
                       Name(ECD0, 0)
                       Name(ECD1, 0)
                       Name(ECD2, 0)


                       // Collection control for AA
                       // Arg0 has 0 for disable, non 0 for enable
                       Method(WCAA, 1) {

                           if (LEqual(Arg0, Zero))
                           {
                               // We are disabling
                               if (LEqual(CCAA, Zero))
                               {
                                   // ERROR - Collection Control was already disabled
                                   Name(Foo, "WMIACPI: ASL: WCAA called, but CCAA is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 14, 0)
                               }
                           } else {
                               // We are enabling
                               if (LNotEqual(CCAA, Zero))
                               {
                                   // ERROR - Collection Control was already enabled
                                   Name(Foo1, "WMIACPI: ASL: WCAA called, but CCAA is 1\n")
                                   Store(Foo1, Debug)
                                   Fatal(0xa0, 13, 0)
                               }
                           }
                           Store(Arg0, CCAA)
                       }

                       // Collection control for AB
                       // Arg0 has 0 for disable, non 0 for enable
                       Method(WCAB, 1) {

                           if (LEqual(Arg0, Zero))
                           {
                               // We are disabling
                               if (LEqual(CCAB, Zero))
                               {
                                   // ERROR - Collection Control was already disabled
                                   Name(Foo, "WMIACPI: ASL: WCAB called, but CCAB is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 16, 0)
                               }
                           } else {
                               // We are enabling
                               if (LNotEqual(CCAB, Zero))
                               {
                                   // ERROR - Collection Control was already enabled
                                   Name(Foo1, "WMIACPI: ASL: WCAB called, but CCAB is 1\n")
                                   Store(Foo1, Debug)
                                   Fatal(0xa0, 15, 0)
                               }
                           }
                           Store(Arg0, CCAB)
                       }

                       // Collection control for AC
                       // Arg0 has 0 for disable, non 0 for enable
                       Method(WCAC, 1) {

                           if (LEqual(Arg0, Zero))
                           {
                               // We are disabling
                               if (LEqual(CCAC, Zero))
                               {
                                   // ERROR - Collection Control was already disabled
                                   Name(Foo, "WMIACPI: ASL: WCAC called, but CCAC is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 18, 0)
                               }
                           } else {
                               // We are enabling
                               if (LNotEqual(CCAC, Zero))
                               {
                                   // ERROR - Collection Control was already enabled
                                   Name(Foo1, "WMIACPI: ASL: WCAC called, but CCAC is 1\n")
                                   Store(Foo1, Debug)
                                   Fatal(0xa0, 17, 0)
                               }
                           }
                           Store(Arg0, CCAC)
                       }


                       // Event control for D0
                       // Arg0 has 0 for disable, non 0 for enable
                       Method(WED0, 1) {

                           if (LEqual(Arg0, Zero))
                           {
                               // We are disabling
                               if (LEqual(ECD0, Zero))
                               {
                                   // ERROR - Event Control was already disabled
                                   Name(Foo, "WMIACPI: ASL: WED0 called, but ECD0 is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 20, 0)
                               }
                           } else {
                               // We are enabling
                               if (LNotEqual(ECD0, Zero))
                               {
                                   // ERROR - Event Control was already enabled
                                   Name(Foo1, "WMIACPI: ASL: WED0 called, but WED0 is 1\n")
                                   Store(Foo1, Debug)
                                   Fatal(0xa0, 19, 0)
                               }
                           }
                           Store(Arg0, ECD0)
                       }


                       // Event control for D1
                       // Arg0 has 0 for disable, non 0 for enable
                       Method(WED1, 1) {

                           if (LEqual(Arg0, Zero))
                           {
                               // We are disabling
                               if (LEqual(ECD1, Zero))
                               {
                                   // ERROR - Event Control was already disabled
                                   Name(Foo, "WMIACPI: ASL: WED1 called, but ECD1 is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 22, 0)
                               }
                           } else {
                               // We are enabling
                               if (LNotEqual(ECD1, Zero))
                               {
                                   // ERROR - Event Control was already enabled
                                   Name(Foo1, "WMIACPI: ASL: WED1 called, but WED1 is 1\n")
                                   Store(Foo1, Debug)
                                   Fatal(0xa0, 21, 0)
                               }
                           }
                           Store(Arg0, ECD1)
                       }

                       // Event control for D2
                       // Arg0 has 0 for disable, non 0 for enable
                       Method(WED2, 1) {

                           if (LEqual(Arg0, Zero))
                           {
                               // We are disabling
                               if (LEqual(ECD2, Zero))
                               {
                                   // ERROR - Event Control was already disabled
                                   Name(Foo, "WMIACPI: ASL: WED2 called, but ECD2 is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 24, 0)
                               }
                           } else {
                               // We are enabling
                               if (LNotEqual(ECD2, Zero))
                               {
                                   // ERROR - Event Control was already enabled
                                   Name(Foo1, "WMIACPI: ASL: WED2 called, but WED2 is 1\n")
                                   Store(Foo1, Debug)
                                   Fatal(0xa0, 23, 0)
                               }
                           }
                           Store(Arg0, ECD2)
                       }

                       // Get value for package A
                       Method(GETA, 1)
                       {
                           //
                           // Return data corresponding to instance specified
                           if (LEqual(Arg0, Zero)) {
                               Return(SAA0)
                           }

                           if (LEqual(Arg0, 1)) {
                               Return(SAA1)
                           }

                           if (LEqual(Arg0, 2)) {
                               Return(SAA2)
                           }

                           if (LEqual(Arg0, 3)) {
                               Return(SAA3)
                           }

                           Name(Foo, "WMIACPI: ASL: GETA called with InstanceIndex = ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 2, 0)
                       }


                       // Get value for package A
                       Method(GETB, 1)
                       {
                           //
                           // Return data corresponding to instance specified
                           if (LEqual(Arg0, Zero)) {
                               Return(SAB0)
                           }

                           if (LEqual(Arg0, 1)) {
                               Return(SAB1)
                           }

                           if (LEqual(Arg0, 2)) {
                               Return(SAB2)
                           }

                           if (LEqual(Arg0, 3)) {
                               Return(SAB3)
                           }

                           Name(Foo, "WMIACPI: ASL: GETB called with InstanceIndex = ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 4, 0)
                       }

                       Method(GETC, 1)
                       {
                           //
                           // Return data corresponding to instance specified
                           if (LEqual(Arg0, Zero)) {
                               Return(SAC0)
                           }

                           if (LEqual(Arg0, 1)) {
                               Return(SAC1)
                           }

                           if (LEqual(Arg0, 2)) {
                               Return(SAC2)
                           }

                           if (LEqual(Arg0, 3)) {
                               Return(SAC3)
                           }

                           Name(Foo, "WMIACPI: ASL: GETC called with InstanceIndex = ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 6, 0)
                       }

                       //
                       // Package Query data block
                       // Arg0 has the instance being queried
                       Method(WQAA, 1) {
                           if (LEqual(CCAA, Zero))
                           {
                               // ERROR - Collection Control was not enabled
                               Name(Foo, "WMIACPI: ASL: WQAA called, but CCAA is 0\n")
                               Store(Foo, Debug)
                               Name(Foo1, "aa")
                               Store(Debug, Foo1)
                               Fatal(0xa0, 1, 0)
                           }

                           Return(GETA(Arg0))

                       }

                       //
                       // String Query data block
                       // Arg0 has the instance being queried
                       Method(WQAB, 1) {
                           if (LEqual(CCAB, Zero))
                           {
                               // ERROR - Collection Control was not enabled
                               Name(Foo, "WMIACPI: ASL: WQAB called, but CCAB is 0\n")
                               Store(Foo, Debug)
                               Fatal(0xa0, 3, 0)
                           }
                           Return(GETB(Arg0))
                       }

                       //
                       // ULONG Query data block
                       // Arg0 has the instance being queried
                       Method(WQAC, 1) {
                           if (LEqual(CCAC, Zero))
                           {
                               // ERROR - Collection Control was not enabled
                               Name(Foo, "WMIACPI: ASL: WQAC called, but CCAC is 0\n")
                               Store(Foo, Debug)
                               Fatal(0xa0, 5, 0)
                           }
                           Return(GETC(Arg0))
                       }

                       Method(SETA, 2)
                       {
                           //
                           // Return data corresponding to instance specified
                           if (LEqual(Arg0, Zero)) {
                               Store(Arg1, SAA0)
                               Return(SAA0)
                           }

                           if (LEqual(Arg0, 1)) {
                               Store(Arg1, SAA1)
                               Return(SAA1)
                           }

                           if (LEqual(Arg0, 2)) {
                               Store(Arg1, SAA2)
                               Return(SAA2)
                           }

                           if (LEqual(Arg0, 3)) {
                               Store(Arg1, SAA3)
                               Return(SAA3)
                           }

                           Name(Foo, "WMIACPI: ASL: SETA called with InstanceIndex = ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 8, 0)
                       }

                       Method(SETB, 2)
                       {
                           //
                           // Return data corresponding to instance specified
                           if (LEqual(Arg0, Zero)) {
                               Store(Arg1, SAB0)
                               Return(SAB0)
                           }

                           if (LEqual(Arg0, 1)) {
                               Store(Arg1, SAB1)
                               Return(SAB1)
                           }

                           if (LEqual(Arg0, 2)) {
                               Store(Arg1, SAB2)
                               Return(SAB2)
                           }

                           if (LEqual(Arg0, 3)) {
                               Store(Arg1, SAB3)
                               Return(SAB3)
                           }

                           Name(Foo, "WMIACPI: ASL: SETB called with InstanceIndex = ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 10, 0)
                       }

                       Method(SETC, 2)
                       {
                           //
                           // Return data corresponding to instance specified
                           if (LEqual(Arg0, Zero)) {
                               Store(Arg1, SAC0)
                               Return(SAC0)
                           }

                           if (LEqual(Arg0, 1)) {
                               Store(Arg1, SAC1)
                               Return(SAC1)
                           }

                           if (LEqual(Arg0, 2)) {
                               Store(Arg1, SAC2)
                               Return(SAC2)
                           }

                           if (LEqual(Arg0, 3)) {
                               Store(Arg1, SAC3)
                               Return(SAC3)
                           }

                           Name(Foo, "WMIACPI: ASL: SETC called with InstanceIndex = ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 12, 0)
                       }

                       //
                       // Set Data Block - Package
                       // Arg0 has the instance being queried
                       // Arg1 has the new value for the data block instance
                       Method(WSAA, 2) {
                           if (LEqual(CCAA, Zero))
                           {
                               // ERROR - Collection Control was not enabled
                               Name(Foo, "WMIACPI: ASL: WSAA called, but CCAA is 0\n")
                               Store(Foo, Debug)
                               Fatal(0xa0, 7, 0)
                           }

                           Return(SETA(Arg0, Arg1))
                       }

                       //
                       // Set Data Block - String
                       // Arg0 has the instance being queried
                       // Arg1 has the new value for the data block instance
                       Method(WSAB, 2) {
                           if (LEqual(CCAB, Zero))
                           {
                               // ERROR - Collection Control was not enabled
                               Name(Foo, "WMIACPI: ASL: WSAB called, but CCAB is 0\n")
                               Store(Foo, Debug)
                               Fatal(0xa0, 9, 0)
                           }

                           Return(SETB(Arg0, Arg1))
                       }


                       //
                       // Set Data Block - ULONG
                       // Arg0 has the instance being queried
                       // Arg1 has the new value for the data block instance
                       Method(WSAC, 2) {
                           if (LEqual(CCAC, Zero))
                           {
                               // ERROR - Collection Control was not enACled
                               Name(Foo, "WMIACPI: ASL: WSAC called, but CCAC is 0\n")
                               Store(Foo, Debug)
                               Fatal(0xa0, 11, 0)
                           }
                           Return(SETC(Arg0, Arg1))
                       }

                       //
                       // Validate that instance index is between 0 and 3
                       Method(VINS, 1)
                       {
                           if (LLess(Arg0, Zero))
                           {
                               Name(Foo, "WMIACPI: ASL: VINS called with InstanceIndex = ")
                               Store(Foo, Debug)
                               Store(Arg0, Debug)
                               Fatal(0xa0, 27, 0)
                           }

                           if (LGreater(Arg0, 3))
                           {
                               Name(Foo1, "WMIACPI: ASL: VINS called with InstanceIndex = ")
                               Store(Foo1, Debug)
                               Store(Arg0, Debug)
                               Fatal(0xa0, 28, 0)
                           }
                       }


                       //
                       // Package Method data block
                       // Arg0 has the instance being queried
                       // Arg1 has the method id
                       // Arg2 has the data passed
                       Method(WMBA, 3) {

                           // MethodId 1 - Get
                           if (LEqual(Arg1, 1))
                           {
                               Return(GETA(Arg0))
                           }


                           // MethodId 2 - Set
                           if (LEqual(Arg1, 2))
                           {
                               SETA(Arg0, Arg2)
                               Return(0)
                           }

                           // MethodId 3 - Event Generate
                           if (LEqual(Arg1, 3))
                           {
                               VINS(Arg0)
                               if (LEqual(ECD0, Zero))
                               {
                                   // ERROR - Event Control was not enabled
                                   Name(Foo, "WMIACPI: ASL: WMBA called, but ECD0 is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 25, 0)
                                   
                               }
                               Notify(AMW0, 0xd0)
                               Return(0)
                           }

                           Name(Foo1, "WMIACPI: ASL: WMBA passed method id ")
                           Store(Foo1, Debug)
                           Store(Arg1, Debug)
                           Fatal(0xa0, 26, 0)

                       }

                       //
                       // String Method data block
                       // Arg0 has the instance being queried
                       // Arg1 has the method id
                       // Arg2 has the data passed
                       Method(WMBB, 3) {

                           // MethodId 1 - Get
                           if (LEqual(Arg1, 1))
                           {
                               Return(GETB(Arg0))
                           }


                           // MethodId 2 - Set
                           if (LEqual(Arg1, 2))
                           {
                               SETB(Arg0, Arg2)
                               Return(0)
                           }

                           // MethodId 3 - Event Generate
                           if (LEqual(Arg1, 3))
                           {
                               VINS(Arg0)
                               if (LEqual(ECD1, Zero))
                               {
                                   // ERROR - Event Control was not enabled
                                   Name(Foo, "WMIACPI: ASL: WMBB called, but ECD1 is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 29, 0)
                                   
                               }
                               Notify(AMW0, 0xd1)
                               Return(0)
                           }

                           Name(Foo1, "WMIACPI: ASL: WMBB passed method id ")
                           Store(Foo1, Debug)
                           Store(Arg1, Debug)
                           Fatal(0xa0, 30, 0)

                       }

                       //
                       // ULONG Method data block
                       // Arg0 has the instance being queried
                       // Arg1 has the method id
                       // Arg2 has the data passed
                       Method(WMBC, 3) {

                           // MethodId 1 - Get
                           if (LEqual(Arg1, 1))
                           {
                               Return(GETC(Arg0))
                           }


                           // MethodId 2 - Set
                           if (LEqual(Arg1, 2))
                           {
                               SETC(Arg0, Arg2)
                               Return(0)
                           }

                           // MethodId 3 - Event Generate
                           if (LEqual(Arg1, 3))
                           {
                               VINS(Arg0)
                               if (LEqual(ECD2, Zero))
                               {
                                   // ERROR - Event Control was not enabled
                                   Name(Foo, "WMIACPI: ASL: WMBC called, but ECD2 is 0\n")
                                   Store(Foo, Debug)
                                   Fatal(0xa0, 31, 0)
                                   
                               }
                               Notify(AMW0, 0xd2)
                               Return(0)
                           }

                           Name(Foo1, "WMIACPI: ASL: WMBC passed method id ")
                           Store(Foo1, Debug)
                           Store(Arg1, Debug)
                           Fatal(0xa0, 32, 0)
                       }




                       //
                       // More info about an event
                       // Arg0 is the event id that was fired
                       Method(_WED, 1) 
                       {
                           if (LEqual(Arg0, 0xd0)) 
                           {
                               Return(GETA(0))
                           }

                           if (LEqual(Arg0, 0xd1)) 
                           {
                               Return(GETB(0))
                           }

                           if (LEqual(Arg0, 0xd2)) 
                           {
                               Return(GETC(0))
                           }

                           Name(Foo, "WMIACPI: ASL: _WED passed event code ")
                           Store(Foo, Debug)
                           Store(Arg0, Debug)
                           Fatal(0xa0, 33, 0)

                       }

                    }


