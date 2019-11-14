---
page_type: sample
description: "Demonstrates how to generate layouts for various keyboards and locales."
languages:
- cpp
products:
- windows
- windows-wdk
---

# Keyboard Layout Samples

The keyboard layout samples demonstrate how to generate layouts for various keyboards and locales.

## Build the sample

Starting in the WDK, you can build the sample in two ways: using the Visual Studio Integrated Development Environment (IDE) or from the command line using the Visual Studio Command Prompt window and the Microsoft Build Engine (MSBuild.exe).

### Building the sample using Visual Studio

1. Open Visual Studio. From the **File** menu, select **Open Project/Solution**. Within your WDK installation, navigate to src\\input\\layout and open the kbd.sln project file.

1. Right-click the solution in the **Solution Explorer** and select **Configuration Manager**.

1. From the **Configuration Manager**, select the **Active Solution Configuration** (for example, Windows 8.1 Debug or Windows 8.1 Release) and the **Active Solution Platform** (for example, Win32) that correspond to the type of build you are interested in.

1. From the **Build** menu, click **Build Solution** (Ctrl+Shift+B).

Previous versions of the WDK used the Windows Build utility (Build.exe) and provided separate build environment windows for each of the supported build configurations. Starting in the WDK, you can use the Visual Studio Command Prompt window for all build configurations.

### Building the sample using the command line (MSBuild)

1. Open a Visual Studio Command Prompt window. Click **Start** and search for **Developer Command Prompt**. If your project is under %PROGRAMFILES%, you need to open the command prompt window using elevated permissions (**Run as administrator**). From this window you can use MsBuild.exe to build any Visual Studio project by specifying the project (.VcxProj) or solutions (.Sln) file.

1. Navigate to the project directory and enter the **MSbuild** command for your target. For example, to perform a clean build of a Visual Studio driver project called kbdus.vcxproj, navigate to the project directory and enter the following MSBuild command: **msbuild /t:clean /t:build .\\kbdus.vcxproj**.

1. If the build succeeds, you will find the driver (kbdus.dll) in the binary output directory corresponding to the target platform, for example src\\input\\layout\\kbdus\\Windows 8.1 Debug.

## Design and Operation

### Keyboard Layout Samples details

The layout DLL is loaded by the window manager when needed. One of the examples is the logon. The default set of the input locales is set in the HKCU registry, according to user's preference, which can be customized by the Regional and Language Options application in Control Panel. The window manager reads the HKCU registry and loads the keyboard layouts accordingly.

The samples under input/layout include the following keyboard layouts:

- kbdus

    US-English keyboard layout

- kbdfr

    French keyboard layout

- kbdgr

    German keyboard layout

- kbd101

    Japanese 101 keyboard layout

- kbd106

    Japanese 106 keyboard layout

### Conversion Tables

A keyboard layout DLL consists of a set of tables. One of the tables converts the scancode to virtual key code, while the other table provides the conversion rule from the virtual key code to the character. Not all the keys or key combinations generate the characters. The modifier keys, such as the SHIFT key or the CTRL key, alter the character generation, but do not generate the characters. The special keys, such as F1-F12 functions keys, the Delete key or the Home key, do not generate the characters either.

The conversion rule from the scancode to the virtual key code is predefined in kbd.h, but can be customized in the layout-specific header files. The layout-specific headers define the keyboard type as it appears in kbd.h, and may redefine some definitions that are specific to each layout.

For the typical keyboard hardware, three types of the scancode to the Virtual Key Code conversion table must be defined in the C source file, including non-extended scancode, E0-prefixed scancode, and E1-prefixed scancode.

The conversion table for the character generation has multiple columns. Each column represents the modifier status and contains the character corresponding to the status. The number of the columns could vary from layout to layout, depending on the number of possible combinations of the modifier keys. Generally speaking, ALT GR-enabled keyboard layouts, such as French or German keyboard layouts, have more columns than non-ALT GR-enabled keyboard, such as US-English keyboard layout.

### Exports from keyboard layout DLL

The layout DLLs export one or more entry points, which are called by the window manager to obtain the table address and the information about the layouts.

For the East Asian keyboard layouts, the keyboard layout DLL has to provide language-specific entry points. Those additional entries expose the language-specific information, such as the kana conversion table, or the special conversion rule, including the VK\_KANJI generation.

## Installation

To install a customized version of a layout DLL, it is recommended to substitute the existing DLL installed by the operating system with the customized DLL. The layout DLLs should be installed in the %windir%\\system32 directory (exceptions may apply on 64-bit platforms). You may need to disable the System File Protection (SFP) before replacing the layout DLLs. After the layout DLLs are installed, you may need to reboot the system to ensure the new layouts are used..

## Loading the Sample

In order to load the layout DLLs, you may need to turn to the Regional and Language Options application in Control Panel and add input locales that use the keyboard layouts you installed.

You may also choose to do it programmatically. LoadKeyboardLayout API can be used to load the keyboard layouts. To activate the keyboard layout, you may choose to specify KLF\_ACTIVATE flag to LoadKeyboardLayout API, or you may need to call ActivateKeyboardLayout API.
