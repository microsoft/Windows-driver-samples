---
page_type: sample
description: "Demonstrates how to use UMDF version 1 to write a near-field proximity driver."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Near-Field Proximity Sample Driver (UMDF Version 1)

> [!WARNING]
> UMDF 2 is the latest version of UMDF and supersedes UMDF 1.  All new UMDF drivers should be written using UMDF 2.  No new features are being added to UMDF 1 and there is limited support for UMDF 1 on newer versions of Windows 10.  Universal Windows drivers must use UMDF 2.
>
> For more info, see [Getting Started with UMDF](https://docs.microsoft.com/windows-hardware/drivers/wdf/getting-started-with-umdf-version-2).

This sample demonstrates how to use User-Mode Driver Framework (UMDF) version 1 to write a near-field proximity driver.

Typically, a near-field proximity driver would use near-field technologies such as Near Field Communication (NFC), TransferJet, or Bump. However, this sample uses a TCP/IPv6 network connection and a static configuration between two machines to simulate near-field interaction.

## See also

[Getting Started with UMDF](https://docs.microsoft.com/windows-hardware/drivers/wdf/getting-started-with-umdf-version-2)

[UMDF 1.x Design Guide](https://docs.microsoft.com/windows-hardware/drivers/wdf/user-mode-driver-framework-design-guide)
