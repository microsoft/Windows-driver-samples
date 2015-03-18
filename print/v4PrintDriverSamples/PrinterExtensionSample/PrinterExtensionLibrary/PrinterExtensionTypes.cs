// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
//
// Abstract:
//
//     This file defines all types and interfaces that may be used to build a printer
//     extension application.
//

using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace Microsoft.Samples.Printing.PrinterExtension.Types
{
    //
    // Enums and Constants
    //
    /// <summary>
    /// Maps to COM PrintSchemaConstrainedSetting
    /// </summary>
    public enum PrintSchemaConstrainedSetting
    {
        None = 0,
        PrintTicket = 1,
        Admin = 2,
        Device = 3,
    }

    /// <summary>
    /// Maps to COM PrintSchemaSelectionType
    /// </summary>
    public enum PrintSchemaSelectionType
    {
        PickOne = 0,
        PickMany = 1
    }

#if WINDOWS_81_APIS
    /// <summary>
    /// Maps to COM PrintSchemaParameterDataType
    /// </summary>
    public enum PrintSchemaParameterDataType
    {
        Integer = 0,
        NumericString = 1,
        String = 2
    }

    /// <summary>
    /// Maps to COM PrintJobStatus
    /// </summary>
    [Flags]
    public enum PrintJobStatus
    {
        Paused = 0x1,
        Error = 0x2,
        Deleting = 0x4,
        Spooling = 0x8,
        Printing = 0x10,
        Offline = 0x20,
        PaperOut = 0x40,
        Printed = 0x80,
        Deleted = 0x100,
        BlockedDeviceQueue = 0x200,
        UserIntervention = 0x400,
        Restarted = 0x800,
        Complete = 0x1000,
        Retained = 0x2000,
    }
#endif

    public static class PrinterExtensionReason
    {
        // An Enum was the first choice but the list of Guid is designed to be extendable.
        // A read-only property was the second choice however this would have made new copies of the Guid.
        // Using a class with static Guids balances both considerations.

        /// <summary>
        /// In this mode preferences for a print job or default print preferences is expected to be displayed.
        /// Maps to C++ PRINTER_EXTENSION_REASON_PRINT_PREFERENCES
        /// </summary>
        public static Guid PrintPreferences = new Guid("{EC8F261F-267C-469F-B5D6-3933023C29CC}");


        /// <summary>
        /// In this mode a status monitor for the print queue is expected to be displayed.
        /// Maps to C++ PRINTER_EXTENSION_REASON_DRIVER_EVENT
        /// </summary>
        public static Guid DriverEvent = new Guid("{23BB1328-63DE-4293-915B-A6A23D929ACB}");
    }

    public static class PrintSchemaConstants
    {
        /// <summary>
        /// The namespace URI for the Print Schema keywords
        /// </summary>
        public const string KeywordsNamespaceUri = "http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords";
        /// <summary>
        /// The namespace URI for the Print Schema keywords V1.1
        /// </summary>
        public const string KeywordsV11NamespaceUri = "http://schemas.microsoft.com/windows/2013/05/printing/printschemakeywordsv11";
        /// <summary>
        /// The namespace URI for the Print Schema Framework
        /// </summary>
        public const string FrameworkNamespaceUri = "http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework";
    }

    //
    // Interfaces
    //

    // The following interfaces are shared between the "Reference" and "Implementation"
    // project. These interfaces are the public surface for the adapters that will remain
    // internal to the "Implementation" project. It is done this way because the public
    // surface and strong name must be the same for "Reference" and "Implementation".

    /// <summary>
    /// Maps to COM IPrinterExtensionContext
    /// </summary>
    public interface IPrinterExtensionContext
    {
        /// <summary>
        /// Maps to COM IPrinterExtensionContext::PrinterQueue
        /// </summary>
        IPrinterQueue Queue { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::PrintSchemaTicket
        /// </summary>
        IPrintSchemaTicket Ticket { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::DriverProperties
        /// </summary>
        IPrinterPropertyBag DriverProperties { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::UserProperties
        /// </summary>
        IPrinterPropertyBag UserProperties { get; }
    }

    /// <summary>
    /// Maps to COM IPrinterExtensionEventArgs
    /// </summary>
    public interface IPrinterExtensionEventArgs : IPrinterExtensionContext
    {
        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::BidiNotification
        /// </summary>
        string BidiNotification { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::ReasonId
        /// </summary>
        Guid ReasonId { get; }


        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::Request
        /// </summary>
        IPrinterExtensionRequest Request { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::SourceApplication
        /// </summary>
        string SourceApplication { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::DetailedReasonId
        /// </summary>
        Guid DetailedReasonId { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::WindowModal
        /// </summary>
        bool WindowModal { get; }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::WindowParent
        /// </summary>
        IntPtr WindowParent { get; }
    }

#if WINDOWS_81_APIS
    /// <summary>
    /// Maps to COM IPrinterExtensionAsyncOperation
    /// </summary>
    public interface IPrinterExtensionAsyncOperation
    {
        /// <summary>
        /// Maps to COM IPrinterExtensionAsyncOperation::Cancel
        /// </summary>
        void Cancel();
    }
#endif

    /// <summary>
    /// Maps to COM IPrintSchemaElement
    /// </summary>
    public interface IPrintSchemaElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaElement::Name
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaElement::NamespaceUri
        /// </summary>
        string XmlNamespace { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaDisplayableElement
    /// </summary>
    public interface IPrintSchemaDisplayableElement : IPrintSchemaElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaDisplayableElement::DisplayName
        /// </summary>
        string DisplayName { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaOption
    /// </summary>
    public interface IPrintSchemaOption : IPrintSchemaDisplayableElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaOption::Selected
        /// </summary>
        bool Selected { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaOption::Constrained
        /// </summary>
        PrintSchemaConstrainedSetting Constrained { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaPageMediaSizeOption
    /// </summary>
    public interface IPrintSchemaPageMediaSizeOption : IPrintSchemaOption
    {
        /// <summary>
        /// Maps to COM IPrintSchemaPageMediaSizeOption::HeightInMicrons
        /// </summary>
        uint HeightInMicrons { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaPageMediaSizeOption::WidthInMicrons
        /// </summary>
        uint WidthInMicrons { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaNUpOption
    /// </summary>
    public interface IPrintSchemaNUpOption : IPrintSchemaOption
    {
        /// <summary>
        /// Maps to COM IPrintSchemaNUpOption::PagesPerSheet
        /// </summary>
        uint PagesPerSheet { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaFeature
    /// </summary>
    public interface IPrintSchemaFeature : IPrintSchemaDisplayableElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaFeature::SelectedOption
        /// </summary>
        IPrintSchemaOption SelectedOption { get; set; }

        /// <summary>
        /// Maps to COM IPrintSchemaFeature::SelectionType
        /// </summary>
        PrintSchemaSelectionType SelectionType { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaFeature::GetOption
        /// </summary>
        IPrintSchemaOption GetOption(string optionName);

        /// <summary>
        /// Maps to COM IPrintSchemaFeature::GetOption
        /// </summary>
        IPrintSchemaOption GetOption(string optionName, string xmlNamespace);

        /// <summary>
        /// Maps to COM IPrintSchemaFeature::DisplayUI
        /// </summary>
        bool DisplayUI { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaPageImageableSize
    /// </summary>
    public interface IPrintSchemaPageImageableSize : IPrintSchemaElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaPageImageableSize::ExtentHeightInMicrons
        /// </summary>
        uint ExtentHeightInMicrons { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaPageImageableSize::ExtentWidthInMicrons
        /// </summary>
        uint ExtentWidthInMicrons { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaPageImageableSize::ImageableSizeHeightInMicrons
        /// </summary>
        uint ImageableSizeHeightInMicrons { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaPageImageableSize::ImageableSizeWidthInMicrons
        /// </summary>
        uint ImageableSizeWidthInMicrons { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaPageImageableSize::OriginHeightInMicrons
        /// </summary>
        uint OriginHeightInMicrons { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaPageImageableSize::OriginWidthInMicrons
        /// </summary>
        uint OriginWidthInMicrons { get; }
    }

#if WINDOWS_81_APIS
    /// <summary>
    /// Maps to COM IPrintSchemaParameterDefinition
    /// </summary>
    public interface IPrintSchemaParameterDefinition : IPrintSchemaDisplayableElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaParameterDefinition::UserInputRequired
        /// </summary>
        bool UserInputRequired { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaParameterDefinition::UnitType
        /// </summary>
        string UnitType { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaParameterDefinition::DataType
        /// </summary>
        PrintSchemaParameterDataType DataType { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaParameterDefinition::RangeMin
        /// </summary>
        int RangeMin { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaParameterDefinition::RangeMax
        /// </summary>
        int RangeMax { get; }
    }

    /// <summary>
    /// Maps to COM IPrintSchemaParameterInitializer
    /// </summary>
    public interface IPrintSchemaParameterInitializer : IPrintSchemaElement
    {
        /// <summary>
        /// Maps to COM IPrintSchemaParameterInitializer::Value
        /// </summary>
        string StringValue { get; set; }
        /// <summary>
        /// Maps to COM IPrintSchemaParameterInitializer::Value
        /// </summary>
        int IntegerValue { get; set; }
    }
#endif

    /// <summary>
    /// Maps to COM IPrintSchemaCapabilities
    /// </summary>
    public interface IPrintSchemaCapabilities
    {
        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::GetFeatureByKeyName
        /// </summary>
        IPrintSchemaFeature GetFeatureByKeyName(string keyName);

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::GetFeature
        /// </summary>
        IPrintSchemaFeature GetFeature(string featureName);

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::GetFeature
        /// </summary>
        IPrintSchemaFeature GetFeature(string featureName, string xmlNamespace);

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::PageImageableSize
        /// </summary>
        IPrintSchemaPageImageableSize PageImageableSize { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::JobCopiesAllDocumentsMinValue
        /// </summary>
        uint JobCopiesAllDocumentsMaxValue { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::JobCopiesAllDocumentsMaxValue
        /// </summary>
        uint JobCopiesAllDocumentsMinValue { get; }

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::GetSelectedOptionInPrintTicket
        /// </summary>
        IPrintSchemaOption GetSelectedOptionInPrintTicket(IPrintSchemaFeature feature);

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities::GetOptions
        /// </summary>
        IEnumerable<IPrintSchemaOption> GetOptions(IPrintSchemaFeature feature);

        /// <summary>
        /// Replaces COM IPrintSchemaCapabilities::XmlNode
        /// </summary>
        Stream GetReadStream();

        /// <summary>
        /// Replaces COM IPrintSchemaCapabilities::XmlNode
        /// </summary>
        Stream GetWriteStream();

#if WINDOWS_81_APIS
        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities2::GetParameterDefinition
        /// </summary>
        IPrintSchemaParameterDefinition GetParameterDefinition(string parameterName);

        /// <summary>
        /// Maps to COM IPrintSchemaCapabilities2::GetParameterDefinition
        /// </summary>
        IPrintSchemaParameterDefinition GetParameterDefinition(string parameterName, string xmlNamespace);
#endif
    }

    /// <summary>
    /// The EventArgs for the PrintSchemaAsyncOperation
    /// Maps to COM IPrintSchemaAsyncOperationEvent
    /// </summary>
    public class PrintSchemaAsyncOperationEventArgs : EventArgs
    {
        //
        // Event arguments
        //
        /// <summary>
        /// Maps to COM IPrintSchemaAsyncOperationEvent::Completed, parameter 'hrOperation'
        /// </summary>
        public int StatusHResult { get { return _statusHResult; } }

        /// <summary>
        /// Maps to COM IPrintSchemaAsyncOperationEvent::Completed, parameter 'pTicket'
        /// </summary>
        public IPrintSchemaTicket Ticket { get { return _printTicket; } }

        //
        // Implementation details
        //
        internal PrintSchemaAsyncOperationEventArgs(IPrintSchemaTicket printTicket, int statusHResult)
        {
            _statusHResult = statusHResult;
            _printTicket = printTicket;
        }

        private int _statusHResult;
        private IPrintSchemaTicket _printTicket;
    }

    /// <summary>
    /// Maps to COM IPrintSchemaAsyncOperation
    /// </summary>
    public interface IPrintSchemaAsyncOperation
    {
        /// <summary>
        /// Maps to COM IPrintSchemaAsyncOperationEvent::Completed
        /// </summary>
        event EventHandler<PrintSchemaAsyncOperationEventArgs> Completed;

        /// <summary>
        /// Maps to COM IPrintSchemaAsyncOperation::Start
        /// </summary>
        void Start();

        /// <summary>
        /// Maps to COM IPrintSchemaAsyncOperation::Cancel
        /// </summary>
        void Cancel();
    }

    /// <summary>
    /// Maps to COM IPrintSchemaTicket
    /// </summary>
    public interface IPrintSchemaTicket
    {
        /// <summary>
        /// Maps to COM IPrintSchemaTicket::GetFeatureByKeyName
        /// </summary>
        IPrintSchemaFeature GetFeatureByKeyName(string featureName);

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::GetFeature
        /// </summary>
        IPrintSchemaFeature GetFeature(string featureName);

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::GetFeature
        /// </summary>
        IPrintSchemaFeature GetFeature(string featureName, string xmlNamespace);

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::ValidateAsync
        /// </summary>
        IPrintSchemaAsyncOperation ValidateAsync();

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::CommitAsync
        /// </summary>
        IPrintSchemaAsyncOperation CommitAsync(IPrintSchemaTicket printTicketCommit);

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::NotifyXmlChanged
        /// </summary>
        void NotifyXmlChanged();

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::GetCapabilities
        /// </summary>
        IPrintSchemaCapabilities GetCapabilities();

        /// <summary>
        /// Maps to COM IPrintSchemaTicket::JobCopiesAllDocuments
        /// </summary>
        uint JobCopiesAllDocuments { get; set; }

        /// <summary>
        /// Replaces COM IPrintSchemaTicket::XmlNode
        /// </summary>
        Stream GetReadStream();

        /// <summary>
        /// Replaces COM IPrintSchemaTicket::XmlNode
        /// </summary>
        Stream GetWriteStream();

#if WINDOWS_81_APIS
        /// <summary>
        /// Maps to COM IPrintSchemaTicket2::GetParameterInitializer
        /// </summary>
        IPrintSchemaParameterInitializer GetParameterInitializer(string parameterName);

        /// <summary>
        /// Maps to COM IPrintSchemaTicket2::GetParameterInitializer
        /// </summary>
        IPrintSchemaParameterInitializer GetParameterInitializer(string parameterName, string xmlNamespace);
#endif
    }

    /// <summary>
    /// Maps to COM IPrinterPropertyBag
    /// </summary>
    public interface IPrinterPropertyBag
    {
        /// <summary>
        /// Maps to COM IPrinterPropertyBag::GetBool
        /// </summary>
        bool GetBool(string propertyName);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::SetBool
        /// </summary>
        void SetBool(string propertyName, bool value);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::GetInt32
        /// </summary>
        int GetInt(string propertyName);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::SetInt32
        /// </summary>
        void SetInt(string propertyName, int value);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::GetString
        /// </summary>
        string GetString(string propertyName);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::SetString
        /// </summary>
        void SetString(string propertyName, string value);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::GetBytes
        /// </summary>
        byte[] GetBytes(string propertyName);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::SetBytes
        /// </summary>
        void SetBytes(string propertyName, byte[] value);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::GetReadStream
        /// </summary>
        Stream GetReadStream(string propertyName);

        /// <summary>
        /// Maps to COM IPrinterPropertyBag::GetWriteStream
        /// </summary>
        Stream GetWriteStream(string propertyName);

        /// <summary>
        /// Indexer for the properties
        /// </summary>
        /// <param name="name">Property name</param>
        /// <returns>An instance of 'PrinterProperty' used to get/set values</returns>
        PrinterProperty this[string name] { get; }
    }

    /// <summary>
    /// Represents one property returned by the indexer in IPrinterPropertyBag
    /// </summary>
    public class PrinterProperty
    {
        internal PrinterProperty(IPrinterPropertyBag bag, string name)
        {
            _bag = bag;
            _name = name;
        }

        /// <summary>
        /// Prevents default construction
        /// </summary>
        private PrinterProperty()
        {
        }

        /// <summary>
        /// Get/Set a value of type 'bool'
        /// </summary>
        public bool Bool
        {
            get { return _bag.GetBool(_name); }
            set { _bag.SetBool(_name, value); }
        }

        /// <summary>
        /// Get/Set a value of type 'Int32'
        /// </summary>
        public int Int
        {
            get { return _bag.GetInt(_name); }
            set { _bag.SetInt(_name, value); }
        }

        /// <summary>
        /// Get/Set a value of type 'byte[]'
        /// </summary>
        public byte[] Bytes
        {
            get { return _bag.GetBytes(_name); }
            set { _bag.SetBytes(_name, value); }
        }

        /// <summary>
        /// Get/Set a value of type 'string'
        /// </summary>
        public string String
        {
            get { return _bag.GetString(_name); }
            set { _bag.SetString(_name, value); }
        }

        /// <summary>
        /// Get a read/write Stream corresponding to this property name
        /// </summary>
        public Stream WriteStream
        {
            get { return _bag.GetWriteStream(_name); }
        }

        /// <summary>
        /// Get a read-only Stream corresponding to this property name
        /// </summary>
        public Stream ReadStream
        {
            get { return _bag.GetReadStream(_name); }
        }

        //
        // Implementation details
        //
        private IPrinterPropertyBag _bag;
        private string _name;
    }

#if WINDOWS_81_APIS
    /// <summary>
    /// Maps to COM IPrintJob
    /// </summary>
    public interface IPrintJob
    {
        /// <summary>
        /// Maps to COM IPrintJob::Name
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Maps to COM IPrintJob::Id
        /// </summary>
        ulong Id { get; }

        /// <summary>
        /// Maps to COM IPrintJob::PrintedPages
        /// </summary>
        ulong PrintedPages { get; }

        /// <summary>
        /// Maps to COM IPrintJob::TotalPages
        /// </summary>
        ulong TotalPages { get; }

        /// <summary>
        /// Maps to COM IPrintJob::Status
        /// </summary>
        PrintJobStatus Status { get; }

        /// <summary>
        /// Maps to COM IPrintJob::SubmissionTime
        /// </summary>
        DateTime SubmissionTime { get; }

        /// <summary>
        /// Maps to COM IPrintJob::RequestCancel
        /// </summary>
        void RequestCancel();
    }

    /// <summary>
    /// Maps to COM IPrinterQueueViewEvent
    /// </summary>
    public class PrinterQueueViewEventArgs : EventArgs
    {
        /// <summary>
        /// Maps to COM IPrinterQueueViewEvent::OnChanged, parameter 'pCollection'
        /// </summary>
        public IEnumerable<IPrintJob> Collection { get { return _collection; } }

        /// <summary>
        /// Maps to COM IPrinterQueueViewEvent::OnChanged, parameter 'ulViewOffset'
        /// </summary>
        public uint ViewOffset { get { return _viewOffset; } }

        /// <summary>
        /// Maps to COM IPrinterQueueViewEvent::OnChanged, parameter 'ulViewSize'
        /// </summary>
        public uint ViewSize { get { return _viewSize; } }

        /// <summary>
        /// Maps to COM IPrinterQueueViewEvent::OnChanged, parameter 'ulCountJobsInPrintQueue'
        /// </summary>
        public uint CountJobsInPrintQueue { get { return _countJobsInPrintQueue; } }

        #region Implementation details

        internal PrinterQueueViewEventArgs(IEnumerable<IPrintJob> collection, uint viewOffset, uint viewSize, uint countJobsInPrintQueue)
        {
            _collection = collection;
            _viewOffset = viewOffset;
            _viewSize = viewSize;
            _countJobsInPrintQueue = countJobsInPrintQueue;
        }

        private IEnumerable<IPrintJob> _collection;
        private uint _viewOffset;
        private uint _viewSize;
        private uint _countJobsInPrintQueue;

        #endregion
    }

    /// <summary>
    /// Maps to COM IPrinterQueueView
    /// </summary>
    public interface IPrinterQueueView
    {
        /// <summary>
        /// Maps to COM IPrinterQueueView::SetViewRange
        /// </summary>
        void SetViewRange(uint viewOffset, uint viewSize);

        /// <summary>
        /// Maps to COM IPrinterQueueViewEvent::OnChanged
        /// </summary>
        event EventHandler<PrinterQueueViewEventArgs> OnChanged;
    }

    /// <summary>
    /// Maps to COM IPrinterBidiSetRequestCallback
    /// </summary>
    public interface IPrinterBidiSetRequestCallback
    {
        /// <summary>
        /// Maps to COM IPrinterBidiSetRequestCallback::Completed
        /// </summary>
        void Completed(string response, int statusHResult);
    }
#endif

    /// <summary>
    /// Maps to COM IPrinterQueueEvent
    /// </summary>
    public class PrinterQueueEventArgs : EventArgs
    {
        /// <summary>
        /// Maps to COM IPrinterQueueEvent::OnBidiResponseReceived, parameter 'bstrResponse'
        /// </summary>
        public string Response { get { return _response; } }

        /// <summary>
        /// Maps to COM IPrinterQueueEvent::OnBidiResponseReceived, parameter 'hrStatus'
        /// </summary>
        public int StatusHResult { get { return _statusHResult; } }

        //
        // Implementation details
        //
        public PrinterQueueEventArgs(string response, int statusHResult)
        {
            _response = response;
            _statusHResult = statusHResult;
        }

        private int _statusHResult;
        private string _response;
    }

    /// <summary>
    /// Maps to COM IPrinterQueue
    /// </summary>
    public interface IPrinterQueue
    {
        /// <summary>
        /// Maps to COM IPrinterQueue::Handle
        /// </summary>
        IntPtr Handle { get; }

        /// <summary>
        /// Maps to COM IPrinterQueue::Name
        /// </summary>
        string Name { get; }

        /// <summary>
        /// Maps to COM IPrinterQueue::SendBidiQuery
        /// </summary>
        void SendBidiQuery(string bidiQuery);

        /// <summary>
        /// Maps to COM IPrinterQueue::GetProperties
        /// </summary>
        IPrinterPropertyBag GetProperties();

#if WINDOWS_81_APIS
        /// <summary>
        /// Maps to COM IPrinterQueue2::SendBidiSetRequestAsync
        /// </summary>
        /// <param name="callback">Maps to COM callback type IPrinterBidiSetRequestCallback</param>
        /// <returns>IPrinterExtensionAsyncOperation - async operation context</returns>
        IPrinterExtensionAsyncOperation SendBidiSetRequestAsync(string bidiRequest, IPrinterBidiSetRequestCallback callback);

        /// <summary>
        /// Maps to COM IPrinterQueue2::GetPrinterQueueView
        /// </summary>
        IPrinterQueueView GetPrinterQueueView(uint viewOffset, uint viewSize);
#endif

        /// <summary>
        /// Maps to COM IPrinterQueueEvent::OnBidiResponseReceived
        /// </summary>
        event EventHandler<PrinterQueueEventArgs> OnBidiResponseReceived;
    }

    /// <summary>
    /// Maps to COM IPrinterExtensionRequest
    /// </summary>
    public interface IPrinterExtensionRequest
    {
        /// <summary>
        /// Maps to COM IPrinterExtensionRequest::Complete
        /// </summary>
        void Complete();

        /// <summary>
        /// Maps to COM IPrinterExtensionRequest::Cancel
        /// </summary>
        void Cancel(int statusHResult, string logMessage);
    }
}

