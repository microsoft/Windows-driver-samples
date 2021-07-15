---
page_type: sample
description: "Provides examples of how to build Print Support App (PSA).
Print Ticket Manipulation Example illustrate how to customize print preference user interface with printer's capability information
Watermark Manipulation Example illustrate how to access print data and add watermark string to it."
languages:
- csharp
products:
- windows
- windows-sdk
---

# Print Support App (PSA) sample

Print Support App APIs are for printer manufactures and ISVs to develop apps that can enhance a Windows user's print experience for IPP printers. 
You can find more information about PSA at [docs.microsoft.com](https://aka.ms/print/psa).

To build this sample, you can use Microsoft Visual Studio 2019 (Community, Professional, or Enterprise) and Windows Software Development Kit (SDK) build 22000 or later. 

# Operating system requirements

Client - Windows 11 build 22000 or later. 

# Build the sample

1. Open the solution file in Visual Studio 2019
1. Retarget to use 22000 SDK
1. Select target (x64 or ARM64)
1. Build

# Run the sample
You can deploy PSA as UWP app from Visual Studio. To run PSA, you need to associate PSA with an IPP printer. The printer must use "Microsoft IPP Class Driver" or "Universal Print Class Driver". If it uses drivers from printer manufactures, PSA will not work. 
[Extension INF](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/using-an-extension-inf-file) can associate PSA's application ID to IPP printer's Hardware Id or Compatible Id.
For more information about extension INF, you can find "Print support app association" topic at end of [PSA document](https://aka.ms/print/psa).

Once you install extension INF and IPP printer, you can test it from any printing applications.
1. Run Notepad, type some text. 
1. File -> Print
1. Select the IPP printer.
1. Click "Preferences" button.
1. PSA sample's Print Ticket Manipulation Example shows up.
1. Modify print options and close PSA user interface.
1. Click "Print" button.
1. PSA sample's Watermark Manipulation Example shows up.
1. Set watermark string, Print
