---
topic: sample
description: Demonstrates how to use UMDF version 1 to write a near-field proximity driver.
languages:
- cpp
products:
- windows
---

<!---
    name: Near-Field Proximity Sample Driver (UMDF Version 1)
    platform: UMDF1
    language: cpp
    category: Proximity
    description: Demonstrates how to use UMDF version 1 to write a near-field proximity driver.
    samplefwlink: http://go.microsoft.com/fwlink/p/?LinkId=620200
--->

# Near-Field Proximity Sample Driver (UMDF Version 1)

This sample demonstrates how to use User-Mode Driver Framework (UMDF) version 1 to write a near-field proximity driver.

Typically, a near-field proximity driver would use near-field technologies such as Near Field Communication (NFC), TransferJet, or Bump. However, this sample uses a TCP/IPv6 network connection and a static configuration between two machines to simulate near-field interaction.

## Related technologies

[User-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560456)
