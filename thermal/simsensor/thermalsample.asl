DefinitionBlock ("ACPITABL.DAT", "SSDT", 0x02, "MSFT", "simulatr", 0x1) {
    Scope (\_SB) {
        Device (SMTC) {
            Name (_HID, "STCL0001")
            Name (_UID, 1)
        }

        ThermalZone(SMSN) {
            Name (_HID, "MSNS0042")
            Name (_UID, 1)
            Name (_PSV, 3110)
            Name (_CRT, 3280)
            Name (_TSP, 300)
            Name (_TC1, 4)
            Name (_TC2, 2)
            Name (_TZD, Package () { \_SB.SMTC } )
        }
    }
}

