---
page_type: sample
description: "Demonstrates how to write a registry filter driver."
languages:
- cpp
products:
- windows
- windows-wdk
---

# RegFltr Sample Driver

The RegFltr sample shows how to write a [registry filter driver](hhttps://docs.microsoft.com/windows-hardware/drivers/kernel/filtering-registry-calls).In addition to providing some basic examples, this sample demonstrates the following:

- How to handle transactional registry operations.

- How and when to capture input parameters.

- Issues and workarounds for version 1.0 of registry filtering.

- Changes in version 1.1 of registry filtering.

- How to use version 1 of the [**REG\_CREATE\_KEY\_INFORMATION**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_reg_create_key_information) and [**REG\_OPEN\_KEY\_INFORMATION**](https://docs.microsoft.com/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_reg_create_key_information) data structures.

The RegFltr sample contains several examples of user-mode and kernel-mode registry-filtering operations. Each example comes with its own corresponding registry callback routine, and performs the following steps:

1. Does some setup work.

1. Registers the callback routine.

1. Performs one or more registry operations.

1. Unregisters the callback routine.

1. Verifies that the sample completed correctly.

The sample driver is a minimal driver that is not intended to be used on production systems. To keep the samples simple, the registry callback routines provided do not check for all possible situations and error conditions. This sample is designed to demonstrate typical scenarios and no other registry filtering driver is expected to be active.
