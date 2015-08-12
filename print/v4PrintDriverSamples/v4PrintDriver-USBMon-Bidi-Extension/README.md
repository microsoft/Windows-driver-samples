Print Driver USB Monitor and Bidi Sample
========================================

This sample demonstrates how to support bidirectional (Bidi) communication over the USB bus, using JavaScript and XML. This sample supports bidirectional status while not printing, and unsolicited status from the printer while printing.

The following files are included in the sample:

-   USBMON\_Bidi\_JavaScript\_File.js. This JavaScript file demonstrates the implementation of a Bidi support for USBMon with a v4 print driver. The JavaScript file supports three functions: getSchemas() is used to make Bidi GET queries to a device, setSchema() is used to make a single Bidi SET query to the device, and getStatus() is called repeatedly during printing in order to retrieve unsolicited status from the printer using the data from the read channel of the device.
-   USBMON\_Bidi\_XML\_File.xml. This XML file demonstrates how to build a Bidi Schema extension for USB. It describes the supported schema elements that can be queried or set, along with their restrictions.

For more information, see [USB Bidi Extender](http://msdn.microsoft.com/en-us/library/windows/hardware/jj659903(v=vs.85).aspx).

**Note** This sample is for the v4 print driver model.

**Note** When you make calls to printerStream.read() in the sample, the printer returns an array which includes an additional element that represents the array length. The following JavaScript code can be used to copy the returned array into a new array, and also to remove the additional element.

```
var readBuffer = [];
var readBytes = 0;
var readSize = 4096;

readBuffer = printerStream.read( readSize );
readBytes = readBuffer.length;

var cleanArray = [];
           
for ( i = 0; i < readBytes; i++ ) {
    cleanArray[i] = readBuffer.shift();
}
```

