NFC Simulator Driver Sample
===========================
This sample demonstrates how to use User-Mode Driver Framework (UMDF) to write a Near-Field Communication (NFC) universal driver along with best practices.

This sample uses a TCP/IPv6 network connection and a static configuration between two machines to allow simulation of near-field interaction.

Installing the driver
--------------------- 
To install the NfcSimulator driver:
Copy the driver binary, NfcSimulator.inf to a directory on your test machine. Change to the directory containing the inf and binaries Next run devcon.exe as follows: 

    devcon.exe install NfcSimulator.inf root\NfcSimulator

Create a Windows Firewall rule to allow the NfcSimulator to receive proximity simulation requests over the network. For example, run WF.msc, and create a new inbound rule that opens port 9299. 