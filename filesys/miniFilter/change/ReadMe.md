Change File System Minifilter Driver
====================================

The Change minifilter is a transaction-aware filter that monitors file changes in real time.

This filter tracks if the files are 'dirty' by intercepting write I/O requests. This provides a way to track modifications to a file. Additionally, this filter handles the case where the transaction commits or rollbacks.

The primary tasks of the filter for tracking a transacted file are the following:

1.  In the post create callback, if a transacted file is open with attribute FILE\_WRITE\_DATA or FILE\_APPEND\_DATA, then enlist its file context into the transaction context.
2.  In the pre-operation callback, if the operation needs to be dirty, such as IRP\_MJ\_WRITE and the file is part of a transaction, update the transacted dirty record instead of the non-transacted dirty record.
3.  In the kernel transaction manager (KTM) notification callback, if the transaction is committed, then propagate the dirty information from the transacted dirty record to the non-transacted dirty record; if rollback, do not propagate.
4.  Properly remove the context structure in the TransactionContextCleanup routine.

## Universal Windows Driver Compliant
This sample builds a Universal Windows Driver. It uses only APIs and DDIs that are included in OneCoreUAP.

