---
page_type: sample
description: "Provides information on how to build a Print Support App (PSA) and includes Print Ticket Manipulation and Watermark Manipulation examples."
languages:
- csharp
products:
- windows
- windows-sdk
---

# Print Support App (PSA) sample

Print Support App APIs are for printer manufactures and ISVs to develop apps that can enhance a Windows user's print experience for IPP printers.

You can find more information about PSA at [docs.microsoft.com](https://aka.ms/print/psa).

The Print Ticket Manipulation example illustrates how to customize print preference user interface with printer's capability information.

The Watermark Manipulation example illustrates how to access print data and add watermark string to it.

To build the sample, you can use Microsoft Visual Studio 2019 (Community, Professional, or Enterprise) and Windows Software Development Kit (SDK) build 22000 or later.

## Operating system requirements

Client - Windows 11 build 22000 or later.

## Build the sample

1. Open the solution file in Visual Studio 2019

1. Retarget to use SDK build 22000 or later

1. Select target (x64 or ARM64)

1. Build

## Run the sample

You can deploy PSA as a UWP app from Visual Studio. To run the PSA, you need to associate the PSA with an IPP printer. The printer must use "Microsoft IPP Class Driver" or "Universal Print Class Driver". If it uses drivers from printer manufactures, the PSA will not work.

An [Extension INF](https://docs.microsoft.com/windows-hardware/drivers/install/using-an-extension-inf-file) will associate the PSA's application ID to IPP printer's Hardware Id or Compatible Id.

For more information about using the Extension INF with a PSA, you can find "Print support app association" topic at end of [PSA document](https://aka.ms/print/psa).

Once you have installed the extension INF and IPP printer, you can test the PSA sample from any print enabled application.

1. Run Notepad, type some text.

1. File -> Print

1. Select the IPP printer.

1. Click "Preferences" button.

1. PSA sample's Print Ticket Manipulation Example shows up.

1. Modify print options and close the PSA user interface.

1. Click the "Print" button.

1. PSA sample's Watermark Manipulation Example shows up.

1. Set watermark string, Print

## See also

[Print support app design guide](https://docs.microsoft.com/windows-hardware/drivers/devapps/print-support-app-design-guide)

[Print support app association](https://docs.microsoft.com/windows-hardware/drivers/devapps/print-support-app-association)
