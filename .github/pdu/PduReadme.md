# PDU Setup: <br>Eaton Tripp Lite Switched Power Distribution Unit (PDUMH15NET)

## Requirements:
### HW: 
  - USBA-to-MicroUSB cable, needed for 1st time PDU login and password change, if IP address is unknown.
  - Ethernet cable, can be used if the IP address is known.
### SW: 
  - [Tera Term 5.6.1 terminal client:](https://github.com/TeraTermProject/teraterm/releases/tag/v5.6.1)
    - Terminal client for PDU login and configuration via serial.
  - [PuTTY 0.84 terminal client:](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html)
    - `Plink.exe` needed for PowerShell scripting via network.
    - **Note**: PuTTY terminal client GUI does not appear to work, so Tera Term client was used instead, but `Plink.exe` is needed for passing passwords via SSH in PowerShell scripts.

## Serial Connection
### What's my IP address?
- If the PDU is connected to a network with **DHCP**, then we need to find it's IP address first by using serial connection.
  - If the PDU is not connected to a network with DHCP, then its default static IP address should be `169.254.0.1`.
  - If that's the case, then you can login to the PDU via a browser by skipping the the section Network Connection.

Tera Term:
- Connect the PC to the PDU (CONFIG port) with a USBA-to-MiniUSB cable.
- On the PC, go to Device Manager and verify under `Ports (COM & LPT)`, the `USB Serial Device (COM3)` device is present.
- Download and install Tera Term on PC, if not already present.
  - You may use other Terminal Emulation Programs, but this is the one used in the manufacturer's user manual to connect to the WEBCARDLX network interface card.
- Launch Tera Term and select the "Serial" connection type and ensure `Port: "COM3: USB Serial Device (COM3)` is selected, then click "OK".
- When the blinking cursor in the empty prompt stops blinking, hit Enter key to wake up console.
- The `Ubuntu 18.04.6 LTS poweralert-0006674a215f` (<-- MAC Address) should show up in the prompt asking for login.
  - Default login is: `localadmin`
  - Default 1st time password is: `localadmin`
	- Must change password after 1st logon.
	- Change password to `@Password123` to match what the `pdu.ps1` PowerShell script uses.
- Once logged in, type `show network` to display network information such as current IP address.
  - If on DHCP network, note the IP address assigned to the PDU.
  - PDU default if no DHCP:
	- IPv4: `169.254.0.1`
	- Subnet Mask: `255.255.0.0`
- Test pinging the IPv4 address of the PDU from the PC.
  - `ping <IP Address>`
- Now that you have the IP address, it may be easier to switch over to the browser interface to configure the rest of the PDU.

## Network Connection
- Connect an Ethernet cable from the PC to the PDU Ethernet port.
- Since we have the IP of the PDU, it's easier to configure the PDU via a browser interface.
- Open an Internet browser and enter the IPv4 address of the PDU into the browser.
  - If it prompts you about any security warnings, just find a way to accept and continue anyway.
- The `Power Alert LX` web interface should prompt for username and password.
- If 1st time login...
  - Default login is: `localadmin`
  - Default 1st time password is: `localadmin`
	- Must change password after 1st logon.
	- Change password to `@Password123` to match what the `pdu.ps1` PowerShell script uses.
- Select `Load` tab on the left of screen to see all the loads on the 16 power ports.
- Click on the `State` slider to show the Load options, `Turn Off Load` and `Cycle Load`.
- To enable SSH for PowerShell scripting later, select `Network` tab, then `Services`, then enable `SSH Enabled`.

## Scripting
- The `Pdu.ps1` PowerShell script can be used for automating power ON/OFF/CYCLE of the PDU ports.
- Usage: `.\Pdu.ps1 [on | off | cycle]  [1-16]`
- Update the following variables in the `Pdu.ps1` script if necessary.
  - `$PduIP = "169.254.0.1"`
  - `$Password = "@Password123"`
- The script will check first if there's a serial connection first, then network connection if serial is not present, before issuing the PDU command.

## References
- Eaton Tripp Lite Switched PDU (PDUMH15NET): [Manuals & User Guides](https://www.eaton.com/us/en-us/skuPage.PDUMH15NET.html#tab-2)