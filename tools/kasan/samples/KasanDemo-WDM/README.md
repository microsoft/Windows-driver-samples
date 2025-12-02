---
page_type: sample
description: "Demonstrates KASAN."
languages:
- cpp
products:
- windows
- windows-wdk
---

# KasanDemo

This sample demonstrates how KASAN detects illegal memory accesses.

The sample consists of a legacy device driver that contains intentionally incorrect code that performs illegal memory accesses, and a Win32 console agent that interacts with the driver to trigger different types of illegal memory accesses that KASAN successfully detects.

> [!CAUTION]
> This driver contains INTENTIONALLY INCORRECT code. Neither this driver nor its sample programs are intended for use in a production environment. Instead, they are intended for educational purposes.

> [!CAUTION]
> This driver will cause KASAN to bugcheck your system.

## Run the sample

To run this sample, enable KASAN in your system, load the `kasantrigger.sys` driver, and then run the `kasanagent.exe` application.
