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
//     This file contains an Adapter that wrap the PrinterExtensionManager COM Interop type.
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
    public class PrinterExtensionManager
    {
        public PrinterExtensionManager()
        {
            _manager = new PrinterExtensionLib.PrinterExtensionManager();
        }

        #region IPrinterExtensionManager methods

        /// <summary>
        /// Maps to COM IPrinterExtensionManager::DisableEvents
        /// </summary>
        public void DisableEvents()
        {
            _manager.DisableEvents();
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionManager::EnableEvents
        /// </summary>
        public void EnableEvents(Guid printerDriverId)
        {
            _manager.EnableEvents(printerDriverId);
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEvent::OnDriverEvent
        /// </summary>
        public event EventHandler<PrinterExtensionEventArgs> OnDriverEvent
        {
            add
            {
                if (_onDriverEvent == null)
                {
                    _manager.OnDriverEvent += OnDriverEventReceiver;
                }
                _onDriverEvent += value;
            }
            remove
            {
                _onDriverEvent -= value;
                if (_onDriverEvent == null)
                {
                    _manager.OnDriverEvent -= OnDriverEventReceiver;
                }
            }
        }

        /// <summary>
        /// Maps to COM IPrinterExtensionEvent::OnPrinterQueuesEnumerated
        /// </summary>
        public event EventHandler<PrinterQueuesEnumeratedEventArgs> OnPrinterQueuesEnumerated
        {
            add
            {
                if (_onPrinterQueuesEnumerated == null)
                {
                    _manager.OnPrinterQueuesEnumerated += OnPrinterQueuesEnumeratedReceiver;
                }
                _onPrinterQueuesEnumerated += value;
            }
            remove
            {
                _onPrinterQueuesEnumerated -= value;
                if (_onPrinterQueuesEnumerated == null)
                {
                    _manager.OnPrinterQueuesEnumerated -= OnPrinterQueuesEnumeratedReceiver;
                }
            }
        }

        #endregion

        #region Implementation details

        private void OnDriverEventReceiver(PrinterExtensionLib.IPrinterExtensionEventArgs pEventArgs)
        {
            if (_onDriverEvent != null)
            {
                _onDriverEvent(this, new PrinterExtensionEventArgs(pEventArgs));
            }
        }

        private void OnPrinterQueuesEnumeratedReceiver(PrinterExtensionLib.IPrinterExtensionContextCollection contextCollection)
        {
            if (_onPrinterQueuesEnumerated != null)
            {
                _onPrinterQueuesEnumerated(this, new PrinterQueuesEnumeratedEventArgs(contextCollection));
            }
        }

        private event EventHandler<PrinterExtensionEventArgs> _onDriverEvent;
        private event EventHandler<PrinterQueuesEnumeratedEventArgs> _onPrinterQueuesEnumerated;

        private PrinterExtensionLib.PrinterExtensionManager _manager;

        #endregion
    }
}