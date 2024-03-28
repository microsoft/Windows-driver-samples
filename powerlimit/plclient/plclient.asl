DefinitionBlock ("ACPITABL.DAT", "SSDT", 0x02, "MSFT", "simulatr", 0x1) {
    Device (\_SB.SOC0) {
        Name (_HID, "PLCL0001")
        Name (_UID, 1)
    }
    Device (\_SB.GPU1) {
        Name (_HID, "PLCL0001")
        Name (_UID, 2)
    }
}