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
//     This file contains Adapters that wrap the PrinterExtension COM Interop types.
//
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Runtime;
using System.Runtime.InteropServices;
using Microsoft.Samples.Printing.PrinterExtension.Types;

namespace Microsoft.Samples.Printing.PrinterExtension
{
    // The following three classes are constructable adapters for the root of the
    // object model.  The balance of the types will typically be interfaces. This
    // choice was made so we can share the interface file between projects and enforce
    // the same public surface from both the "Reference" and "Implementation" projects.

    #region PrinterExtension adapter classes

    /// <summary>
    /// Wraps an COM pointer to IPrinterExtensionContext
    /// </summary>
    public class PrinterExtensionContext : IPrinterExtensionContext
    {
        /// <summary>
        /// Wraps an opaque COM pointer to IPrinterExtensionContext and provides usable methods
        /// </summary>
        /// <param name="comContext">Opaque COM pointer to IPrinterExtensionContext</param>
        public PrinterExtensionContext(Object comContext)
        {
            _context = (PrinterExtensionLib.IPrinterExtensionContext)comContext;
        }

        #region IPrinterExtensionContext methods

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::PrinterQueue
        /// </summary>
        public IPrinterQueue Queue
        {
            get { return new PrinterQueue(_context.PrinterQueue); }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::PrintSchemaTicket
        /// </summary>
        public IPrintSchemaTicket Ticket
        {
            get { return new PrintSchemaTicket(_context.PrintSchemaTicket); }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::DriverProperties
        /// </summary>
        public IPrinterPropertyBag DriverProperties
        {
            get
            {
                try
                {
                    return new PrinterPropertyBag(_context.DriverProperties, PrintPropertyBagType.DriverProperties);
                }
                catch (Exception)
                {
                    // If the property bag is not found, instead of 
                    // throwing an exception, return null, which is more appropriate for a property 'get' operation.
                    return null;
                }
            }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::UserProperties
        /// </summary>
        public IPrinterPropertyBag UserProperties
        {
            get { return new PrinterPropertyBag(_context.UserProperties, PrintPropertyBagType.UserProperties); }
        }

        #endregion

        #region Implementation details

        private PrinterExtensionLib.IPrinterExtensionContext _context;

        // Prevent default construction
        private PrinterExtensionContext()
        {
        }

        #endregion
    }

    /// <summary>
    /// Wraps an COM pointer to IPrinterExtensionEventArgs
    /// </summary>
    public class PrinterExtensionEventArgs : EventArgs, IPrinterExtensionEventArgs
    {
        /// <summary>
        /// Wraps an opaque COM pointer to IPrinterExtensionEventArgs and provides usable methods
        /// </summary>
        /// <param name="comContext">Opaque COM pointer to IPrinterExtensionEventArgs</param>
        public PrinterExtensionEventArgs(Object eventArgs)
        {
            _eventArgs = (PrinterExtensionLib.IPrinterExtensionEventArgs)eventArgs;
            _context = new PrinterExtensionContext(eventArgs);
        }

        #region IPrinterExtensionEventArgs methods

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::BidiNotification
        /// </summary>
        public string BidiNotification
        {
            get { return _eventArgs.BidiNotification; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::ReasonId
        /// </summary>
        public Guid ReasonId
        {
            get { return _eventArgs.ReasonId; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::Request
        /// </summary>
        public IPrinterExtensionRequest Request
        {
            get { return new PrinterExtensionRequest(_eventArgs.Request); }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::SourceApplication
        /// </summary>
        public string SourceApplication
        {
            get { return _eventArgs.SourceApplication; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::DetailedReasonId
        /// </summary>
        public Guid DetailedReasonId
        {
            get { return _eventArgs.DetailedReasonId; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::WindowModal
        /// </summary>
        public bool WindowModal
        {
            get
            {
                if (_eventArgs.WindowModal != 0)
                {
                    return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEventArgs::WindowParent
        /// </summary>
        public IntPtr WindowParent
        {
            get { return _eventArgs.WindowParent; }
        }

        #endregion

        #region IPrinterExtensionContext methods

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::PrinterQueue
        /// </summary>
        public IPrinterQueue Queue
        {
            get { return _context.Queue; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::PrintSchemaTicket
        /// </summary>
        public IPrintSchemaTicket Ticket
        {
            get { return _context.Ticket; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::DriverProperties
        /// </summary>
        public IPrinterPropertyBag DriverProperties
        {
            get { return _context.DriverProperties; }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionContext::UserProperties
        /// </summary>
        public IPrinterPropertyBag UserProperties
        {
            get { return _context.UserProperties; }
        }

        #endregion

        #region Implementation details

        private PrinterExtensionLib.IPrinterExtensionEventArgs _eventArgs;

        /// <summary>
        /// Containment - since multiple inheritance is not possible in C#.
        /// </summary>
        private PrinterExtensionContext _context;

        #endregion
    }

    /// <summary>
    /// This class provides wraps IPrinterExtensionContextCollection in a IEnumerable interface
    /// </summary>
    public sealed class PrinterQueuesEnumeratedEventArgs : EventArgs, IEnumerable<IPrinterExtensionContext>
    {

        #region IEnumerable<IPrinterExtensionContext> methods

        public IEnumerator<IPrinterExtensionContext> GetEnumerator()
        {
            for (uint i = 0; i < _contextCollection.Count; i++)
            {
                yield return new PrinterExtensionContext(_contextCollection.GetAt(i));
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return (IEnumerator)GetEnumerator();
        }

        #endregion

        #region Implementation details

        internal PrinterQueuesEnumeratedEventArgs(PrinterExtensionLib.IPrinterExtensionContextCollection contextCollection)
        {
            _contextCollection = contextCollection;
        }

        private PrinterExtensionLib.IPrinterExtensionContextCollection _contextCollection;

        #endregion
    }
    
#if WINDOWS_81_APIS
    internal sealed class PrinterExtensionAsyncOperation : IPrinterExtensionAsyncOperation
    {
        #region IPrinterExtensionAsyncOperation methods

        public void Cancel()
        {
            _asyncOperation.Cancel();
        }

        #endregion

        #region Implementation methods

        internal PrinterExtensionAsyncOperation(PrinterExtensionLib.IPrinterExtensionAsyncOperation asyncOperation)
        {
            _asyncOperation = asyncOperation;
        }

        private PrinterExtensionLib.IPrinterExtensionAsyncOperation _asyncOperation;

        #endregion
    }
#endif
    #endregion

    #region COM Adapter Classes

    //
    // The following class provide an adapter that exposes a 'Stream' and wraps a
    // COM pointer to PrinterExtensionLib.IStream
    //
    internal class PrinterExtensionLibIStreamAdapter : Stream, IDisposable
    {
        public PrinterExtensionLibIStreamAdapter(PrinterExtensionLib.IStream stream, bool canWrite = false, bool canSeek = false, bool canRead = true)
        {
            if (stream != null)
            {
                _printerExtensionIStream = stream;
            }
            else
            {
                throw new ArgumentNullException("stream");
            }
            _streamValidation = new StreamValidation(canWrite, canSeek, canRead);
        }

        ~PrinterExtensionLibIStreamAdapter()
        {
            Dispose(false);
        }

        #region Overridden Stream methods

        public override int Read(byte[] buffer, int offset, int count)
        {
            _streamValidation.ValidateRead(buffer, offset, count);

            uint bytesRead = 0;

            // Pin the byte array so that it will not be moved by the garbage collector
            byte[] tempBuffer = new byte[count];
            GCHandle gcHandle = GCHandle.Alloc(tempBuffer, GCHandleType.Pinned);
            try
            {
                _printerExtensionIStream.RemoteRead(out tempBuffer[0], Convert.ToUInt32(count), out bytesRead);

                Array.Copy(tempBuffer, 0, buffer, offset, (int)bytesRead); // Safe to cast. Cannot be bigger than 'int count'
            }
            finally
            {
                gcHandle.Free();
            }

            return (int)bytesRead; // Safe to cast; bytesRead can never be larger than 'int count'
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            _streamValidation.ValidateWrite(buffer, offset, count);

            uint written;

            // Pin the byte array so that it will not be moved by the garbage collector
            byte[] tempBuffer = new byte[count];
            GCHandle gcHandle = GCHandle.Alloc(tempBuffer, GCHandleType.Pinned);
            try
            {
                Array.Copy(buffer, offset, tempBuffer, 0, count);
                _printerExtensionIStream.RemoteWrite(ref tempBuffer[0], Convert.ToUInt32(count), out written);
            }
            finally
            {
                gcHandle.Free();
            }

            if ((int)written < count)
            {
                throw new IOException();
            }
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            _streamValidation.ValidateSeek(offset, origin);

            uint istreamSeekOrigin = 0;

            switch (origin)
            {
                case SeekOrigin.Begin:
                    istreamSeekOrigin = (uint)PrinterExtensionLib.tagSTREAM_SEEK.STREAM_SEEK_SET;
                    break;

                case SeekOrigin.Current:
                    istreamSeekOrigin = (uint)PrinterExtensionLib.tagSTREAM_SEEK.STREAM_SEEK_CUR;
                    break;

                case SeekOrigin.End:
                    istreamSeekOrigin = (uint)PrinterExtensionLib.tagSTREAM_SEEK.STREAM_SEEK_END;
                    break;
            }

            PrinterExtensionLib._LARGE_INTEGER dlibMove;
            PrinterExtensionLib._ULARGE_INTEGER plibNewPosition;

            dlibMove.QuadPart = offset;

            _printerExtensionIStream.RemoteSeek(dlibMove, istreamSeekOrigin, out plibNewPosition);
            return Convert.ToInt64(plibNewPosition.QuadPart);
        }

        public override long Length
        {
            get
            {
                _streamValidation.ValidateSeek();

                PrinterExtensionLib.tagSTATSTG statstg;
                _printerExtensionIStream.Stat(out statstg, 1 /* STATSFLAG_NONAME*/ );
                return Convert.ToInt64(statstg.cbSize.QuadPart);
            }
        }
        public override long Position
        {
            get { return Seek(0, SeekOrigin.Current); }
            set { Seek(value, SeekOrigin.Begin); }
        }

        public override void SetLength(long value)
        {
            _streamValidation.ValidateSeek();

            PrinterExtensionLib._ULARGE_INTEGER libNewSize;
            libNewSize.QuadPart = Convert.ToUInt64(value);
            _printerExtensionIStream.SetSize(libNewSize);
        }

        public override void Flush()
        {
            _printerExtensionIStream.Commit(0);
        }

        public override bool CanRead
        {
            get { return _streamValidation.CanRead; }
        }

        public override bool CanWrite
        {
            get { return _streamValidation.CanWrite; }
        }

        public override bool CanSeek
        {
            get { return _streamValidation.CanSeek; }
        }

        #endregion

        #region IDisposable methods

        protected override void Dispose(bool disposing)
        {
            if (_disposed)
            {
                return;
            }

            try
            {
                if (disposing)
                {
                    _streamValidation.Dispose();
                }

                if (_printerExtensionIStream != null)
                {
                    Marshal.ReleaseComObject(_printerExtensionIStream);
                    _printerExtensionIStream = null;
                }
            }
            finally
            {
                base.Dispose(disposing);
            }
            _disposed = true;
        }

        #endregion

        #region Implementation details

        // Prevent default construction
        private PrinterExtensionLibIStreamAdapter() { }

        private bool _disposed = false;
        private PrinterExtensionLib.IStream _printerExtensionIStream = null;
        private StreamValidation _streamValidation = null;

        #endregion
    }


    //
    // The following class provide an adapter that exposes a 'Stream' and wraps a
    // COM pointer to the standard COM 'IStream' interface
    //
    internal class ComIStreamAdapter : Stream, IDisposable
    {
        public ComIStreamAdapter(System.Runtime.InteropServices.ComTypes.IStream stream, bool canWrite = false, bool canSeek = false, bool canRead = true)
        {
            if (stream != null)
            {
                _comIstream = stream;
            }
            else
            {
                throw new ArgumentNullException("stream");
            }
            _streamValidation = new StreamValidation(canWrite, canSeek, canRead);
        }

        ~ComIStreamAdapter()
        {
            Dispose(false);
        }

        #region Overridden Stream methods

        public override int Read(byte[] buffer, int offset, int count)
        {
            _streamValidation.ValidateRead(buffer, offset, count);

            uint bytesRead = 0;

            // Pin the byte array so that it will not be moved by the garbage collector
            byte[] tempBuffer = new byte[count];
            GCHandle gcHandle = GCHandle.Alloc(tempBuffer, GCHandleType.Pinned);
            IntPtr bytesReadPtr = Marshal.AllocHGlobal(sizeof(int));
            try
            {
                _comIstream.Read(tempBuffer, count, bytesReadPtr);
                bytesRead = (uint)Marshal.ReadInt32(bytesReadPtr);

                Array.Copy(tempBuffer, 0, buffer, offset, (int)bytesRead); // Safe to cast. Cannot be bigger than 'int count'
            }
            finally
            {
                Marshal.FreeHGlobal(bytesReadPtr);
                gcHandle.Free();
            }

            return (int)bytesRead; // Safe to cast; bytesRead can never be larger than 'int count'
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            _streamValidation.ValidateWrite(buffer, offset, count);

            uint written;

            // Pin the byte array so that it will not be moved by the garbage collector
            byte[] tempBuffer = new byte[count];
            GCHandle gcHandle = GCHandle.Alloc(tempBuffer, GCHandleType.Pinned);
            IntPtr writeCountPointer = Marshal.AllocHGlobal(sizeof(int));
            try
            {
                Array.Copy(buffer, offset, tempBuffer, 0, count);

                _comIstream.Write(tempBuffer, count, writeCountPointer);
                written = (uint)Marshal.ReadInt32(writeCountPointer); // safe to cast. 'written' is always non-negative
            }
            finally
            {
                gcHandle.Free();
                Marshal.FreeHGlobal(writeCountPointer);
            }

            if ((int)written < count)
            {
                throw new IOException();
            }
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            _streamValidation.ValidateSeek(offset, origin);

            uint istreamSeekOrigin = 0;

            switch (origin)
            {
                case SeekOrigin.Begin:
                    istreamSeekOrigin = (uint)PrinterExtensionLib.tagSTREAM_SEEK.STREAM_SEEK_SET;
                    break;

                case SeekOrigin.Current:
                    istreamSeekOrigin = (uint)PrinterExtensionLib.tagSTREAM_SEEK.STREAM_SEEK_CUR;
                    break;

                case SeekOrigin.End:
                    istreamSeekOrigin = (uint)PrinterExtensionLib.tagSTREAM_SEEK.STREAM_SEEK_END;
                    break;
            }

            IntPtr seekPositionPointer = Marshal.AllocHGlobal(sizeof(long));
            long seekPosition = 0;
            try
            {
                _comIstream.Seek(offset, (int)istreamSeekOrigin, seekPositionPointer);
                seekPosition = Marshal.ReadInt64(seekPositionPointer);
            }
            finally
            {
                Marshal.FreeHGlobal(seekPositionPointer);
            }

            return seekPosition;
        }

        public override long Length
        {
            get
            {
                _streamValidation.ValidateSeek();

                System.Runtime.InteropServices.ComTypes.STATSTG statstg;
                _comIstream.Stat(out statstg, 1 /* STATSFLAG_NONAME*/ );
                return statstg.cbSize;
            }
        }
        public override long Position
        {
            get { return Seek(0, SeekOrigin.Current); }
            set { Seek(value, SeekOrigin.Begin); }
        }

        public override void SetLength(long value)
        {
            _streamValidation.ValidateSeek();
            _comIstream.SetSize(value);
        }

        public override void Flush()
        {
            _comIstream.Commit(0);
        }

        public override bool CanRead
        {
            get { return _streamValidation.CanRead; }
        }

        public override bool CanWrite
        {
            get { return _streamValidation.CanWrite; }
        }

        public override bool CanSeek
        {
            get { return _streamValidation.CanSeek; }
        }

        #endregion

        #region IDisposable methods

        protected override void Dispose(bool disposing)
        {
            if (_disposed)
            {
                return;
            }

            try
            {
                if (disposing)
                {
                    _streamValidation.Dispose();
                }

                if (_comIstream != null)
                {
                    Marshal.ReleaseComObject(_comIstream);
                    _comIstream = null;
                }
            }
            finally
            {
                base.Dispose(disposing);
            }
            _disposed = true;
        }

        #endregion

        #region Implementation details

        // Prevent default construction
        private ComIStreamAdapter() { }

        private bool _disposed = false;
        private System.Runtime.InteropServices.ComTypes.IStream _comIstream = null;
        private StreamValidation _streamValidation = null;

        #endregion
    }

    internal class StreamValidation : IDisposable
    {
        internal StreamValidation(bool canWrite = false, bool canSeek = false, bool canRead = true)
        {
            _canWrite = canWrite;
            _canSeek = canSeek;
            _canRead = canRead;
        }

        internal void ValidateRead(byte[] buffer, int offset, int count)
        {
            if (!_canRead)
            {
                throw new NotSupportedException();
            }
            if (_disposed == true)
            {
                throw new ObjectDisposedException("COM IStream");
            }
            if (buffer == null)
            {
                throw new ArgumentNullException("buffer");
            }
            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }
            if ((buffer.Length - offset) < count)
            {
                throw new ArgumentException();
            }
        }

        internal void ValidateWrite(byte[] buffer, int offset, int count)
        {
            if (!_canWrite)
            {
                throw new NotSupportedException();
            }
            if (_disposed == true)
            {
                throw new ObjectDisposedException("COM IStream");
            }
            if (buffer == null)
            {
                throw new ArgumentNullException("buffer");
            }
            if (offset < 0)
            {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }
            if ((buffer.Length - offset) < count)
            {
                throw new ArgumentException("Insufficient buffer size");
            }
        }

        internal void ValidateSeek(long offset, SeekOrigin origin)
        {
            ValidateSeek();
            if ((origin < SeekOrigin.Begin) || (origin > SeekOrigin.End))
            {
                throw new ArgumentException("Invalid value", "origin");
            }
        }

        internal void ValidateSeek()
        {
            if (!_canSeek)
            {
                throw new NotSupportedException();
            }
            if (_disposed == true)
            {
                throw new ObjectDisposedException("COM IStream");
            }
        }

        public bool CanRead
        {
            get { return _canRead; }
        }

        public bool CanWrite
        {
            get { return _canWrite; }
        }

        public bool CanSeek
        {
            get { return _canSeek; }
        }

        #region IDisposable methods

        public void Dispose()
        {
            _disposed = true;
        }

        #endregion

        #region Implementation details

        private bool _disposed = false;
        private bool _canWrite = false;
        private bool _canSeek = false;
        private bool _canRead = true;

        #endregion
    }

    #endregion

    #region PrintSchema Adapter Classes

    //
    // Following are concrete implementation of the PrinterExtension interfaces
    // These classes wrap the underlying COM interfaces.
    //

    internal class PrintSchemaOption : IPrintSchemaOption
    {

        #region IPrintSchemaOption methods

        public bool Selected
        {
            get { return (0 == _option.Selected) ? false : true; }
        }

        public PrintSchemaConstrainedSetting Constrained
        {
            get { return (PrintSchemaConstrainedSetting)_option.Constrained; }
        }

        #endregion

        #region IPrintSchemaDisplayableElement methods

        public string DisplayName { get { return _option.DisplayName; } }
        public string Name { get { return _option.Name; } }
        public string XmlNamespace { get { return _option.NamespaceUri; } }

        #endregion

        #region Implementation details

        internal PrintSchemaOption(PrinterExtensionLib.IPrintSchemaOption option)
        {
            _option = option;
        }

        internal PrinterExtensionLib.IPrintSchemaOption InteropOption
        {
            get { return _option; }
            set { _option = value; }
        }

        //
        // Create the correct 'PrintSchemaOption' subclass, possibly exposing one of the these interfaces
        // 1. IPrintSchemaPageMediaSizeOption
        // 2. IPrintSchemaNUpOption
        //
        internal static IPrintSchemaOption CreateOptionSubclass(PrinterExtensionLib.IPrintSchemaOption option)
        {
            // IPrintSchemaNUpOption option
            if (option is PrinterExtensionLib.IPrintSchemaNUpOption)
            {
                return new PrintSchemaNUpOption(option);
            }

            // IPrintSchemaPageMediaSizeOption option
            if (option is PrinterExtensionLib.IPrintSchemaPageMediaSizeOption)
            {
                return new PrintSchemaPageMediaSizeOption(option);
            }

            return new PrintSchemaOption(option);
        }

        internal PrinterExtensionLib.IPrintSchemaOption _option;

        // Prevent default constuction
        private PrintSchemaOption() { }

        #endregion
    }

    internal sealed class PrintSchemaPageMediaSizeOption : PrintSchemaOption, IPrintSchemaPageMediaSizeOption
    {
        #region IPrintSchemaPageMediaSizeOption methods

        public uint HeightInMicrons
        {
            get { return _pageMediaSizeOption.HeightInMicrons; }
        }

        public uint WidthInMicrons
        {
            get { return _pageMediaSizeOption.WidthInMicrons; }
        }

        #endregion

        #region Implementation details

        internal PrintSchemaPageMediaSizeOption(PrinterExtensionLib.IPrintSchemaOption option)
            : base(option)
        {
            _pageMediaSizeOption = _option as PrinterExtensionLib.IPrintSchemaPageMediaSizeOption;
            if (null == _pageMediaSizeOption)
            {
                throw new NotImplementedException("Could not retrieve IPrintSchemaPageMediaSizeOption interface.");
            }
        }

        private PrinterExtensionLib.IPrintSchemaPageMediaSizeOption _pageMediaSizeOption;

        #endregion
    }

    internal sealed class PrintSchemaNUpOption : PrintSchemaOption, IPrintSchemaNUpOption
    {
        #region IPrintSchemaNUpOption methods

        public uint PagesPerSheet
        {
            get { return _nupOption.PagesPerSheet; }
        }

        #endregion

        #region Implementation details

        internal PrintSchemaNUpOption(PrinterExtensionLib.IPrintSchemaOption option) :
            base(option)
        {
            _nupOption = _option as PrinterExtensionLib.IPrintSchemaNUpOption;
            if (null == _nupOption)
            {
                throw new NotImplementedException("Could not retrieve IPrintSchemaNUpOption interface.");
            }
        }

        private PrinterExtensionLib.IPrintSchemaNUpOption _nupOption;

        #endregion
    }

    /// <summary>
    /// This class provides wraps IPrintSchemaOptionCollection in a IEnumerable interface
    /// </summary>
    internal sealed class PrintSchemaOptionsCollection : IEnumerable<IPrintSchemaOption>
    {

        #region IEnumerable<IPrintSchemaOption> methods

        public IEnumerator<IPrintSchemaOption> GetEnumerator()
        {
            for (uint i = 0; i < _optionCollection.Count; i++)
            {
                yield return new PrintSchemaOption(_optionCollection.GetAt(i));
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return (IEnumerator)GetEnumerator();
        }

        #endregion

        #region Implementation details

        internal PrintSchemaOptionsCollection(PrinterExtensionLib.IPrintSchemaOptionCollection optionCollection)
        {
            _optionCollection = optionCollection;
        }

        private PrinterExtensionLib.IPrintSchemaOptionCollection _optionCollection;

        #endregion
    }

    internal sealed class PrintSchemaFeature : IPrintSchemaFeature
    {
        #region IPrintSchemaFeature methods

        public PrintSchemaSelectionType SelectionType
        {
            get { return (PrintSchemaSelectionType)_feature.SelectionType; }
        }

        public IPrintSchemaOption GetOption(string name)
        {
            return GetOption(name, PrintSchemaConstants.KeywordsNamespaceUri);
        }

        public IPrintSchemaOption GetOption(string name, string xmlNamespace)
        {
            PrinterExtensionLib.IPrintSchemaOption option = _feature.GetOption(name, xmlNamespace);
            if (option != null)
            {
                return PrintSchemaOption.CreateOptionSubclass(option);
            }

            return null;
        }

        public IPrintSchemaOption SelectedOption
        {
            get
            {
                return PrintSchemaOption.CreateOptionSubclass(_feature.SelectedOption);
            }
            set
            {
                _feature.SelectedOption = (value as PrintSchemaOption).InteropOption;
            }
        }

        public bool DisplayUI
        {
            get
            {
                return (0 == _feature.DisplayUI) ? false : true;
            }
        }

        #endregion

        #region IPrintSchemaDisplayableElement methods

        public string DisplayName { get { return _feature.DisplayName; } }
        public string Name { get { return _feature.Name; } }
        public string XmlNamespace { get { return _feature.NamespaceUri; } }

        #endregion

        #region Implementation details

        internal PrintSchemaFeature(PrinterExtensionLib.IPrintSchemaFeature feature)
        {
            _feature = feature;
        }

        internal PrinterExtensionLib.IPrintSchemaFeature InteropFeature
        {
            get { return _feature; }
        }

        private PrinterExtensionLib.IPrintSchemaFeature _feature;

        #endregion
    }

    internal sealed class PrintSchemaPageImageableSize : IPrintSchemaPageImageableSize
    {
        #region IPrintSchemaPageImageableSize methods

        public uint ExtentHeightInMicrons
        {
            get { return _pageImageableSize.ExtentHeightInMicrons; }
        }

        public uint ExtentWidthInMicrons
        {
            get { return _pageImageableSize.ExtentWidthInMicrons; }
        }

        public uint ImageableSizeHeightInMicrons
        {
            get { return _pageImageableSize.ImageableSizeHeightInMicrons; }
        }

        public uint ImageableSizeWidthInMicrons
        {
            get { return _pageImageableSize.ImageableSizeWidthInMicrons; }
        }

        public uint OriginHeightInMicrons
        {
            get { return _pageImageableSize.OriginHeightInMicrons; }
        }

        public uint OriginWidthInMicrons
        {
            get { return _pageImageableSize.OriginWidthInMicrons; }
        }

        #endregion

        #region IPrintSchemaElement methods

        public string Name { get { return _pageImageableSize.Name; } }
        public string XmlNamespace { get { return _pageImageableSize.NamespaceUri; } }

        #endregion

        #region Implementation details

        internal PrintSchemaPageImageableSize(PrinterExtensionLib.IPrintSchemaPageImageableSize pageImageableSize)
        {
            _pageImageableSize = pageImageableSize;
        }

        private PrinterExtensionLib.IPrintSchemaPageImageableSize _pageImageableSize;

        #endregion
    }

#if WINDOWS_81_APIS
    internal sealed class PrintSchemaParameterDefinition : IPrintSchemaParameterDefinition
    {
        #region IPrintSchemaParameterDefinition methods

        public bool UserInputRequired
        {
            get
            {
                if (_parameter.UserInputRequired != 0)
                {
                    return true;
                }
                return false;
            }
        }

        public string UnitType
        {
            get { return _parameter.UnitType; }
        }

        public PrintSchemaParameterDataType DataType
        {
            get { return (PrintSchemaParameterDataType)_parameter.DataType; }
        }

        public int RangeMin
        {
            get { return _parameter.RangeMin; }
        }

        public int RangeMax
        {
            get { return _parameter.RangeMax; }
        }
        #endregion

        #region IPrintSchemaDisplayableItem methods

        public string DisplayName { get { return _parameter.DisplayName; } }
        public string Name { get { return _parameter.Name; } }
        public string XmlNamespace { get { return _parameter.NamespaceUri; } }

        #endregion

        #region Implementation details

        internal PrintSchemaParameterDefinition(PrinterExtensionLib.IPrintSchemaParameterDefinition parameter)
        {
            _parameter = parameter;
        }

        private PrinterExtensionLib.IPrintSchemaParameterDefinition _parameter;

        #endregion
    }

    internal sealed class PrintSchemaParameterInitializer : IPrintSchemaParameterInitializer
    {
        #region IPrintSchemaParameterInitializer methods

        public string StringValue
        {
            get
            {
                object value = _parameter.get_Value();
                return (string)value;
            }
            set
            {
                _parameter.set_Value(value);
            }
        }

        public int IntegerValue
        {
            get
            {
                object value = _parameter.get_Value();
                return (int)value;
            }
            set
            {
                _parameter.set_Value(value);
            }
        }
        #endregion

        #region IPrintSchemaElement methods

        public string Name { get { return _parameter.Name; } }
        public string XmlNamespace { get { return _parameter.NamespaceUri; } }

        #endregion

        #region Implementation details

        internal PrintSchemaParameterInitializer(PrinterExtensionLib.IPrintSchemaParameterInitializer parameter)
        {
            _parameter = parameter;
        }

        PrinterExtensionLib.IPrintSchemaParameterInitializer _parameter;

        #endregion
    }
#endif

    internal sealed class PrintSchemaCapabilities : IPrintSchemaCapabilities
    {
        #region IPrintSchemaCapabilities methods

        public IPrintSchemaFeature GetFeatureByKeyName(string keyName)
        {
            PrinterExtensionLib.IPrintSchemaFeature feature = _capabilities.GetFeatureByKeyName(keyName);
            if (feature != null)
            {
                return new PrintSchemaFeature(feature);
            }

            return null;
        }

        public IPrintSchemaFeature GetFeature(string featureName)
        {
            return GetFeature(featureName, PrintSchemaConstants.KeywordsNamespaceUri);
        }

        public IPrintSchemaFeature GetFeature(string featureName, string xmlNamespace)
        {
            PrinterExtensionLib.IPrintSchemaFeature feature = _capabilities.GetFeature(featureName, xmlNamespace);
            if (feature != null)
            {
                return new PrintSchemaFeature(feature);
            }

            return null;
        }

        public IPrintSchemaPageImageableSize PageImageableSize
        {
            get { return new PrintSchemaPageImageableSize(_capabilities.PageImageableSize); }
        }

        public uint JobCopiesAllDocumentsMaxValue
        {
            get
            {
                uint value = _capabilities.JobCopiesAllDocumentsMaxValue;
                if (value == 0)
                {
                    throw new NotSupportedException("Property \"JobCopiesAllDocumentsMaxValue\" not found in print capabilities.");
                }

                return value;
            }
        }

        public uint JobCopiesAllDocumentsMinValue
        {
            get
            {
                uint value = _capabilities.JobCopiesAllDocumentsMinValue;
                if (value == 0)
                {
                    throw new NotSupportedException("Property \"JobCopiesAllDocumentsMinValue\" not found in print capabilities.");
                }

                return value;
            }
        }

        public IPrintSchemaOption GetSelectedOptionInPrintTicket(IPrintSchemaFeature feature)
        {
            PrintSchemaFeature f = feature as PrintSchemaFeature;
            PrinterExtensionLib.IPrintSchemaOption option = _capabilities.GetSelectedOptionInPrintTicket(f.InteropFeature);

            if (option != null)
            {
                return PrintSchemaOption.CreateOptionSubclass(option);
            }

            return null;
        }

        public IEnumerable<IPrintSchemaOption> GetOptions(IPrintSchemaFeature pFeature)
        {
            return new PrintSchemaOptionsCollection(
                _capabilities.GetOptions(
                    (pFeature as PrintSchemaFeature).InteropFeature)
                );
        }

        public Stream GetReadStream()
        {
            return new ComIStreamAdapter(XmlStream,
                                         false, // canWrite
                                         true,  // canSeek
                                         true   // canRead
                                         );
        }

        public Stream GetWriteStream()
        {
            return new ComIStreamAdapter(XmlStream,
                                         true,  // canWrite
                                         true,  // canSeek
                                         false  // canRead
                                         );
        }

#if WINDOWS_81_APIS
        public IPrintSchemaParameterDefinition GetParameterDefinition(string parameterName)
        {
            return GetParameterDefinition(parameterName, PrintSchemaConstants.KeywordsNamespaceUri);
        }

        public IPrintSchemaParameterDefinition GetParameterDefinition(string parameterName, string xmlNamespace)
        {
            PrinterExtensionLib.IPrintSchemaParameterDefinition parameter = _capabilities2.GetParameterDefinition(parameterName, xmlNamespace);
            if (parameter != null)
            {
                return new PrintSchemaParameterDefinition(parameter);
            }

            return null;
        }
#endif

        private System.Runtime.InteropServices.ComTypes.IStream XmlStream
        {
            get
            {
                System.Runtime.InteropServices.ComTypes.IStream istream = _capabilities.XmlNode as System.Runtime.InteropServices.ComTypes.IStream;

                return istream;
            }
        }

        #endregion

        #region Implementation details

        internal PrintSchemaCapabilities(PrinterExtensionLib.IPrintSchemaCapabilities caps)
        {
            _capabilities = caps;
#if WINDOWS_81_APIS
            _capabilities2 = (PrinterExtensionLib.IPrintSchemaCapabilities2)caps;
#endif
        }

        private PrinterExtensionLib.IPrintSchemaCapabilities _capabilities;
#if WINDOWS_81_APIS
        private PrinterExtensionLib.IPrintSchemaCapabilities2 _capabilities2;
#endif
        #endregion
    }

    internal class PrintSchemaAsyncOperation : IPrintSchemaAsyncOperation
    {
        #region IPrintSchemaAsyncOperation methods

        public void Cancel()
        {
            _asyncOperation.Cancel();
        }

        public void Start()
        {
            _asyncOperation.Start();
        }

        public event EventHandler<PrintSchemaAsyncOperationEventArgs> Completed;

        #endregion

        #region Implementation details

        internal PrintSchemaAsyncOperation(PrinterExtensionLib.PrintSchemaAsyncOperation asyncOperation)
        {
            _asyncOperation = asyncOperation;
            _asyncOperation.Completed += _asyncOperation_Completed;
        }

        void _asyncOperation_Completed(PrinterExtensionLib.IPrintSchemaTicket printTicket, int hrOperation)
        {
            if (Completed != null)
            {
                IPrintSchemaTicket ticket = new PrintSchemaTicket(printTicket);
                Completed(this, new PrintSchemaAsyncOperationEventArgs(ticket, hrOperation));
            }

            // This subscriber object (current object) holds a reference to the publishing object (i.e the underlying COM object)
            // because it is a class member, and the publishing object holds a reference to the subscriber via the registered delegate.
            // This implies neither object will be garbage collected until the application terminates.
            // It's expected the event is fired once per instance so unsubscribing the delegate has no side effects.
            _asyncOperation.Completed -= _asyncOperation_Completed;
            Marshal.ReleaseComObject(_asyncOperation);
            _asyncOperation = null;
        }

        private PrinterExtensionLib.PrintSchemaAsyncOperation _asyncOperation;

        #endregion
    }

    internal class PrintSchemaTicket : IPrintSchemaTicket
    {

        #region IPrintSchemaTicket methods

        public IPrintSchemaCapabilities GetCapabilities()
        {
            return new PrintSchemaCapabilities(_printTicket.GetCapabilities());
        }

        public IPrintSchemaFeature GetFeature(string featureName)
        {
            return GetFeature(featureName, PrintSchemaConstants.KeywordsNamespaceUri);
        }

        public IPrintSchemaFeature GetFeature(string featureName, string xmlNamespace)
        {
            PrinterExtensionLib.IPrintSchemaFeature feature = _printTicket.GetFeature(featureName, xmlNamespace);
            if (feature != null)
            {
                return new PrintSchemaFeature(feature);
            }

            return null;
        }

        public IPrintSchemaFeature GetFeatureByKeyName(string keyName)
        {
            PrinterExtensionLib.IPrintSchemaFeature feature = _printTicket.GetFeatureByKeyName(keyName);
            if (feature != null)
            {
                return new PrintSchemaFeature(feature);
            }

            return null;
        }

        public uint JobCopiesAllDocuments
        {
            get
            {
                uint value = _printTicket.JobCopiesAllDocuments;
                if (value == 0)
                {
                    throw new NotSupportedException("Property \"JobCopiesAllDocuments\" not found in print ticket.");
                }

                return value;
            }
            set
            {
                _printTicket.JobCopiesAllDocuments = value;
            }
        }

        public Stream GetReadStream()
        {
            return new ComIStreamAdapter(XmlStream,
                                         false, // canWrite
                                         true,  // canSeek
                                         true   // canRead
                                         );
        }

        public Stream GetWriteStream()
        {
            return new ComIStreamAdapter(XmlStream,
                                         true,  // canWrite
                                         true,  // canSeek
                                         false  // canRead
                                         );
        }

        private System.Runtime.InteropServices.ComTypes.IStream XmlStream
        {
            get
            {
                System.Runtime.InteropServices.ComTypes.IStream istream = _printTicket.XmlNode as System.Runtime.InteropServices.ComTypes.IStream;

                return istream;
            }
        }

        public IPrintSchemaAsyncOperation ValidateAsync()
        {
            PrinterExtensionLib.PrintSchemaAsyncOperation interopAsyncOperation;
            _printTicket.ValidateAsync(out interopAsyncOperation);
            return new PrintSchemaAsyncOperation(interopAsyncOperation);
        }

        public IPrintSchemaAsyncOperation CommitAsync(IPrintSchemaTicket printTicketCommit)
        {
            PrinterExtensionLib.IPrintSchemaTicket interopTicket = (printTicketCommit as PrintSchemaTicket)._printTicket;
            PrinterExtensionLib.PrintSchemaAsyncOperation interopAsyncOperation;
            _printTicket.CommitAsync(interopTicket, out interopAsyncOperation);
            return new PrintSchemaAsyncOperation(interopAsyncOperation);
        }

        public void NotifyXmlChanged()
        {
            _printTicket.NotifyXmlChanged();
        }
#if WINDOWS_81_APIS
        public IPrintSchemaParameterInitializer GetParameterInitializer(string parameterName)
        {
            return GetParameterInitializer(parameterName, PrintSchemaConstants.KeywordsNamespaceUri);
        }

        public IPrintSchemaParameterInitializer GetParameterInitializer(string parameterName, string xmlNamespace)
        {
            PrinterExtensionLib.IPrintSchemaParameterInitializer parameter = _printTicket2.GetParameterInitializer(parameterName, xmlNamespace);
            if (parameter != null)
            {
                return new PrintSchemaParameterInitializer(parameter);
            }

            return null;
        }
#endif

        #endregion

        #region Implementation details

        internal PrintSchemaTicket(PrinterExtensionLib.IPrintSchemaTicket printTicket)
        {
            _printTicket = printTicket;
#if WINDOWS_81_APIS
            _printTicket2 = (PrinterExtensionLib.IPrintSchemaTicket2)printTicket;
#endif
        }

        private PrinterExtensionLib.IPrintSchemaTicket _printTicket;
#if WINDOWS_81_APIS
        private PrinterExtensionLib.IPrintSchemaTicket2 _printTicket2;
#endif

        #endregion
    }

    internal enum PrintPropertyBagType
    {
        QueueProperties,
        DriverProperties,
        UserProperties
    }

    internal class PrinterPropertyBag : IPrinterPropertyBag
    {
        #region IPrinterPropertyBag methods

        public bool GetBool(string propertyName)
        {
            try
            {
                int integerEquivalent = _bag.GetBool(propertyName);
                bool boolEquivalent = true;
                if (integerEquivalent == 0)
                {
                    boolEquivalent = false;
                }
                return boolEquivalent;
            }
            catch (ArgumentException e)
            {
                // Fix the type of exception thrown when the property does not exist.
                if (ShouldConvertExceptionType(e))
                {
                    throw new FileNotFoundException("", e);
                }
                throw;
            }
        }

        public byte[] GetBytes(string propertyName)
        {
            try
            {
                uint count = 0;
                IntPtr intptrData = Marshal.AllocCoTaskMem(IntPtr.Size);
                _bag.GetBytes(
                     propertyName,
                     out count,
                     intptrData);

                byte[] data = new byte[count];
                Marshal.Copy(Marshal.ReadIntPtr(intptrData), data, 0, (int)count);
                Marshal.FreeCoTaskMem(Marshal.ReadIntPtr(intptrData));
                Marshal.FreeCoTaskMem(intptrData);
                return data;
            }
            catch (ArgumentException e)
            {
                // Fix the type of exception thrown when the property does not exist.
                if (ShouldConvertExceptionType(e))
                {
                    throw new FileNotFoundException("", e);
                }
                throw;
            }
        }

        public int GetInt(string propertyName)
        {
            try
            {
                return _bag.GetInt32(propertyName);
            }
            catch (ArgumentException e)
            {
                // Fix the type of exception thrown when the property does not exist.
                if (ShouldConvertExceptionType(e))
                {
                    throw new FileNotFoundException("", e);
                }
                throw;
            }
        }

        public string GetString(string propertyName)
        {
            try
            {
                return _bag.GetString(propertyName);
            }
            catch (ArgumentException e)
            {
                // Fix the type of exception thrown when the property does not exist.
                if (ShouldConvertExceptionType(e))
                {
                    throw new FileNotFoundException("", e);
                }
                throw;
            }
        }

        public void SetBool(string propertyName, bool value)
        {
            int integerEquivalent = 1;
            if (value == false)
            {
                integerEquivalent = 0;
            }

            _bag.SetBool(propertyName, integerEquivalent);
        }

        public void SetBytes(string propertyName, byte[] data)
        {
            // Pin the byte array so that it will not be moved by the garbage collector
            // This would not be required if the COM Interop function took in a byte[] parameter
            // as opposed to a byte parameter
            GCHandle gcHandle = GCHandle.Alloc(data, GCHandleType.Pinned);
            try
            {
                _bag.SetBytes(propertyName, Convert.ToUInt32(data.Length), ref data[0]);
            }
            finally
            {
                gcHandle.Free();
            }
        }

        public void SetInt(string propertyName, int value)
        {
            _bag.SetInt32(propertyName, value);
        }

        public void SetString(string propertyName, string value)
        {
            _bag.SetString(propertyName, value);
        }

        public Stream GetReadStream(string propertyName)
        {
            try
            {
                return new PrinterExtensionLibIStreamAdapter(_bag.GetReadStream(propertyName), false, true, true);
            }
            catch (COMException e)
            {
                // Fix the type of exception thrown when the property does not exist.
                if (ShouldConvertExceptionType(e))
                {
                    throw new FileNotFoundException("", e);
                }
                throw;
            }
        }

        public Stream GetWriteStream(string propertyName)
        {
            return new PrinterExtensionLibIStreamAdapter(_bag.GetWriteStream(propertyName), true, true, false);
        }

        #endregion

        #region Indexer

        public PrinterProperty this[string name]
        {
            get { return new PrinterProperty(this, name); }
        }

        #endregion

        #region Implementation details

        internal PrinterPropertyBag(PrinterExtensionLib.IPrinterPropertyBag bag, PrintPropertyBagType type)
        {
            _bag = bag;
            _type = type;
        }

        /// <summary>
        /// Check if exception thrown when a property is not found in a property bag needs to be converted.
        /// </summary>
        /// <param name="e"></param>
        /// <returns>True if the exception type needs to be converted.</returns>
        private bool ShouldConvertExceptionType(Exception e)
        {
            // Only the driver property bag throws exceptions other than 'FileNotFoundException'
            // when a property is not found.
            if (_type != PrintPropertyBagType.DriverProperties)
            {
                return false;
            }
            else if (e is ArgumentException)
            {
                return true;
            }
            else if (e is COMException)
            {
                // Since there is no portable way to check the HRESULT across classic .Net and 
                // .Net for Windows Store apps, all COMExceptions encountered are converted.
                return true;
            }
            return false;
        }

        private PrinterExtensionLib.IPrinterPropertyBag _bag;
        private PrintPropertyBagType _type;

        #endregion
    }

#if WINDOWS_81_APIS
    public sealed class PrintJob : IPrintJob
    {
        #region IPrintJob methods

        public string Name
        {
            get { return _job.Name; }
        }

        public ulong Id
        {
            get { return _job.Id; }
        }

        public ulong PrintedPages
        {
            get { return _job.PrintedPages; }
        }

        public ulong TotalPages
        {
            get { return _job.TotalPages; }
        }

        public PrintJobStatus Status
        {
            get { return (PrintJobStatus)_job.Status; }
        }

        public DateTime SubmissionTime
        {
            get { return _job.SubmissionTime; }
        }

        public void RequestCancel()
        {
            _job.RequestCancel();
        }

        #endregion

        #region Implementation details

        internal PrintJob(PrinterExtensionLib.IPrintJob job)
        {
            _job = job;
        }

        PrinterExtensionLib.IPrintJob _job;

        #endregion
    }

    /// <summary>
    /// This class provides wraps IPrintJobCollection in a IEnumerable interface
    /// </summary>
    public sealed class PrintJobCollection : IEnumerable<IPrintJob>
    {
        #region IEnumerable<IPrintJob> methods

        public IEnumerator<IPrintJob> GetEnumerator()
        {
            for (uint i = 0; i < _jobCollection.Count; i++)
            {
                yield return new PrintJob(_jobCollection.GetAt(i));
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return (IEnumerator)GetEnumerator();
        }

        #endregion

        #region Implementation details

        internal PrintJobCollection(PrinterExtensionLib.IPrintJobCollection jobCollection)
        {
            _jobCollection = jobCollection;
        }

        private PrinterExtensionLib.IPrintJobCollection _jobCollection;

        #endregion
    }

    internal sealed class PrinterQueueView : IPrinterQueueView
    {
        #region IPrinterQueueView methods

        public void SetViewRange(uint viewOffset, uint viewSize)
        {
            _view.SetViewRange(viewOffset, viewSize);
        }

        public event EventHandler<PrinterQueueViewEventArgs> OnChanged
        {
            add
            {
                if (_onChanged == null)
                {
                    _view.OnChanged += _view_OnChanged;
                }
                _onChanged += value;
            }
            remove
            {
                _onChanged -= value;
                if (_onChanged == null)
                {
                    _view.OnChanged -= _view_OnChanged;
                }
            }
        }

        #endregion

        #region Implementation details

        internal PrinterQueueView(PrinterExtensionLib.PrinterQueueView view)
        {
            _view = view;
        }

        void _view_OnChanged(PrinterExtensionLib.IPrintJobCollection pCollection, uint ulViewOffset, uint ulViewSize, uint ulCountJobsInPrintQueue)
        {
            if (_onChanged != null)
            {
                _onChanged(this, new PrinterQueueViewEventArgs(new PrintJobCollection(pCollection), ulViewOffset, ulViewSize, ulCountJobsInPrintQueue));
            }
        }

        private PrinterExtensionLib.PrinterQueueView _view;
        private event EventHandler<PrinterQueueViewEventArgs> _onChanged;

        #endregion
    }

    internal sealed class PrinterBidiSetRequestCallback : PrinterExtensionLib.IPrinterBidiSetRequestCallback
    {
        #region IPrinterBidiSetRequestCallback methods

        public void Completed(string response, int statusHResult)
        {
            _callback.Completed(response, statusHResult);
        }

        #endregion

        #region Implementation details

        internal PrinterBidiSetRequestCallback(IPrinterBidiSetRequestCallback callback)
        {
            _callback = callback;
        }

        private IPrinterBidiSetRequestCallback _callback;

        #endregion
    }
#endif

    internal sealed class PrinterQueue : IPrinterQueue
    {
        #region IPrinterQueue methods

        public string Name
        {
            get { return _queue.Name; }
        }

        public void SendBidiQuery(string bidiQuery)
        {
            _queue.SendBidiQuery(bidiQuery);
        }

        public IntPtr Handle
        {
            get { return _queue.Handle; }
        }

        public IPrinterPropertyBag GetProperties()
        {
            return new PrinterPropertyBag(_queue.GetProperties(), PrintPropertyBagType.QueueProperties);
        }
        public event EventHandler<PrinterQueueEventArgs> OnBidiResponseReceived
        {
            add
            {
                if (_onBidiResponseReceived == null)
                {
                    _queue.OnBidiResponseReceived += _queue_OnBidiResponseReceived;
                }
                _onBidiResponseReceived += value;
            }
            remove
            {
                _onBidiResponseReceived -= value;
                if (_onBidiResponseReceived == null)
                {
                    _queue.OnBidiResponseReceived -= _queue_OnBidiResponseReceived;
                }
            }
        }

#if WINDOWS_81_APIS
        public IPrinterExtensionAsyncOperation SendBidiSetRequestAsync(string bidiRequest, IPrinterBidiSetRequestCallback callback)
        {
            PrinterBidiSetRequestCallback comCallback = new PrinterBidiSetRequestCallback(callback);
            PrinterExtensionLib.IPrinterBidiSetRequestCallback comCallbackInterface = comCallback;
            return new PrinterExtensionAsyncOperation(_queue2.SendBidiSetRequestAsync(bidiRequest, comCallbackInterface));
        }

        public IPrinterQueueView GetPrinterQueueView(uint viewOffset, uint viewSize)
        {
            return new PrinterQueueView(_queue2.GetPrinterQueueView(viewOffset, viewSize));
        }
#endif

        #endregion

        #region Implementation details

        internal PrinterQueue(PrinterExtensionLib.PrinterQueue queue)
        {
            _queue = queue;
#if WINDOWS_81_APIS
            _queue2 = (PrinterExtensionLib.IPrinterQueue2)queue;
#endif
        }

        private void _queue_OnBidiResponseReceived(string bstrResponse, int hrStatus)
        {
            if (_onBidiResponseReceived != null)
            {
                _onBidiResponseReceived(this, new PrinterQueueEventArgs(bstrResponse, hrStatus));
            }
        }

        private event EventHandler<PrinterQueueEventArgs> _onBidiResponseReceived;
#if WINDOWS_81_APIS
        private PrinterExtensionLib.IPrinterQueue2 _queue2;
#endif
        private PrinterExtensionLib.PrinterQueue _queue;

        #endregion
    }

    internal sealed class PrinterExtensionRequest : IPrinterExtensionRequest
    {
        #region IPrinterExtensionRequest methods

        public void Complete()
        {
            _request.Complete();
        }

        public void Cancel(int hr, string logMessage)
        {
            _request.Cancel(hr, logMessage);
        }

        #endregion

        #region Implementation details

        internal PrinterExtensionRequest(PrinterExtensionLib.IPrinterExtensionRequest request)
        {
            _request = request;
        }

        private PrinterExtensionLib.IPrinterExtensionRequest _request;

        #endregion
    }

    #endregion
}

