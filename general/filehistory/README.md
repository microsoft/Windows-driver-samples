File History Sample
==================

The FileHistory sample is a console application that starts the file history service, if it is stopped, and schedules regular backups. The application requires, as a command-line parameter, the path name of a storage device to use as the default backup target.

This sample application uses the [File History API](http://msdn.microsoft.com/en-us/library/windows/hardware/hh829789). The File History API enables third parties to automatically configure the File History feature on a Windows platform and customize it in accordance with their unique needs.


Run the sample
--------------

The name of the built sample application is Fhsetup.exe. To run this application, open a command window and enter a command that has the following format:

`fhsetup <path>`

The `path` command-line parameter is the path name of a storage device to use as the default backup target. The following are examples:

`fhsetup D:\`

`fhsetup \\server\share`

If the specified target is inaccessible, read-only, an invalid drive type (such as a CD), already being used for file history, or part of the protected namespace, the application fails the request and does not enable file history on the target.

