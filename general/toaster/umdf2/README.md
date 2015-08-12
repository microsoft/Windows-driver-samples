Toaster Sample (UMDF Version 2)
===============================
The Toaster (UMDF version 2) sample is an iterative series of samples that demonstrate fundamental aspects of Windows driver development.

The Toaster sample collection is comprised of driver projects (.vcxproj files) that are contained in the umdf2toaster.sln solution file.


Related technologies
--------------------
[User-Mode Driver Framework](http://msdn.microsoft.com/en-us/library/windows/hardware/ff560456)


Run the sample
--------------
The computer where you install the driver is called the *target computer* or the *test computer*. Typically this is a separate computer from where you develop and build the driver package. The computer where you develop and build the driver is called the *host computer*.

The process of moving the driver package to the target computer and installing the driver is called *deploying the driver*. You can deploy a driver sample automatically or manually.

### Automatic deployment (root enumerated)

Before you automatically deploy a driver, you must provision the target computer. For instructions, see [Configuring a Computer for Driver Deployment, Testing, and Debugging](http://msdn.microsoft.com/en-us/library/windows/hardware/).

1.  On the host computer, in Visual Studio, in Solution Explorer, right click **package** (lower case), and choose **Properties**. Navigate to **Configuration Properties \> Driver Install \> Deployment**.
2.  Check **Enable deployment**, and check **Remove previous driver versions before deployment**. For **Target Computer Name**, select the name of a target computer that you provisioned previously. Select **Hardware ID Driver Update**, and enter **root\\toaster** for the hardware ID. Click **OK**.
3.  Because this solution contains many projects, you may find it easier to remove some of them before you build and deploy a driver package. To do so, right click **package** (lower case), and choose **Properties**. Navigate to **Common Properties-\>References** and click **Remove Reference** to remove projects you don't want. (You can add them back later by using **Add New Reference**.) Click **OK**.
4.  On the **Build** menu, choose **Build Solution** or **Rebuild Solution** (if you removed references).
5.  If you removed references and deployment does not succeed, try deleting the contents of the c:\\DriverTest\\Drivers folder on the target machine, and then retry deployment.

### Manual deployment (root enumerated)

Before you manually deploy a driver, you must turn on test signing and install a certificate on the target computer. You also need to copy the [DevCon](http://msdn.microsoft.com/en-us/library/windows/hardware/ff544707) tool to the target computer. For instructions, see [Preparing a Computer for Manual Driver Deployment](http://msdn.microsoft.com/en-us/library/windows/hardware/dn265571).

1.  Copy all of the files in your driver package to a folder on the target computer (for example, c:\\Umdf2toaster).
2.  On the target computer, open a Command Prompt window as Administrator. Navigate to your driver package folder, and enter a command such as:

    **devcon install wdfsimpleum.inf root\\toaster**

### View the root enumerated driver in Device Manager

On the target computer, in a Command Prompt window, enter **devmgmt** to open Device Manager. In Device Manager, on the **View** menu, choose **Devices by type**. In the device tree, locate **Sample WDF Toaster Service + Filter** (for example, this might be under the **Toaster** node).

In Device Manager, on the **View** menu, choose **Devices by connection**. Locate **Sample WDF Toaster Service + Filter** as a child of the root node of the device tree.

Build the sample using MSBuild
------------------------------
As an alternative to building the driver sample in Visual Studio, you can build it in a Visual Studio Command Prompt window. In Visual Studio, on the **Tools** menu, choose **Visual Studio Command Prompt**. In the Visual Studio Command Prompt window, navigate to the folder that has the solution file, Umdf2toaster.sln. Use the MSBuild command to build the solution. Here is an example:

**msbuild /p:configuration="Release" /p:platform="Win32" Umdf2toaster.sln**

For more information about using MSBuild to build a driver package, see [Building a Driver](http://msdn.microsoft.com/en-us/library/windows/hardware/ff554644).

