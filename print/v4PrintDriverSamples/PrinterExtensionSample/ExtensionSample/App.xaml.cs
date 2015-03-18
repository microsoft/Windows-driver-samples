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
//     This file contains the entry point to the application.

using System;
using System.Windows;
using System.Runtime;

using Microsoft.Samples.Printing.PrinterExtension.Types;
using Microsoft.Samples.Printing.PrinterExtension.Helpers;

using System.Windows.Interop;

namespace Microsoft.Samples.Printing.PrinterExtension
{
    /// <summary>
    /// Interaction logic for App.xaml.
    /// </summary>
    public partial class App : Application
    {
        /// <summary>
        /// This is the event handler invoked on various driver events.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="eventArgs"></param>
        private static void OnDriverEvent(object sender, PrinterExtensionEventArgs eventArgs)
        {
            //
            // Display the print preferences window.
            //

            if (eventArgs.ReasonId.Equals(PrinterExtensionReason.PrintPreferences))
            {
                PrintPreferenceWindow printPreferenceWindow = new PrintPreferenceWindow();
                printPreferenceWindow.Initialize(eventArgs);

                //
                // Set the caller application's window as parent/owner of the newly created printing preferences window.
                //

                WindowInteropHelper wih = new WindowInteropHelper(printPreferenceWindow);
                wih.Owner = eventArgs.WindowParent;

                //
                // Display a modal/non-modal window based on the 'WindowModal' parameter.
                //

                if (eventArgs.WindowModal)
                {
                    printPreferenceWindow.ShowDialog();
                }
                else
                {
                    printPreferenceWindow.Show();

                    // Flash the window to draw the user's attention. This is required
                    // because the printer extension may be drawn behind the parent window.
                    // The return value of FlashWindow can be safely ignored if there is no need
                    // to know if the window has focus or not.
                    WindowHelper.FlashWindow(wih.Handle);
                }
            }
            else if (eventArgs.ReasonId.Equals(PrinterExtensionReason.DriverEvent))
            {
                //
                // Handle driver events here.
                //
            }
        }

        /// <summary>
        /// Perform initialization tasks for the printer extension in this event handler.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Application_Startup(object sender, StartupEventArgs e)
        {
            //
            // It is recommended that exactly one instance of the PrinterExtensionManager be created per instance of
            // the printer extension.
            //

            if (manager != null)
            {
                return;
            }
            manager = new PrinterExtensionManager();

            //
            // Enable events to be received on one printer driver id.
            //
            // Note: The order of adding the delegate to PrinterExtensionManager.OnDriverEvent
            //       and invoking PrinterExtensionManager::EnableEvents is important.
            //       Adding the delegate should be done first.
            //

            manager.OnDriverEvent += OnDriverEvent;

            //
            // It is recommended that an instance of a printer extension invoke PrinterExtensionManager::EnableEvents
            // for exactly one printer driver id. The printer driver id could come in from a command line argument,
            // thereby allowing one application binary to dynamically invoke PrinterExtensionManager::EnableEvents against
            // the appropriate printer driver id.
            //

            manager.EnableEvents(Guid.Parse(PrinterDriverID));
        }

        /// <summary>
        /// Perform uninitialization tasks for the printer extension in this event handler.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Application_Exit(object sender, ExitEventArgs e)
        {
            manager.OnDriverEvent -= OnDriverEvent;
            manager.DisableEvents();

            manager = null;
        }

        /// <summary>
        /// This is the printer driver id, as defined in the printer driver manifest file.
        /// Please replace this GUID with the printer driver id from your manifest file.
        ///
        /// It is recommended that you invoke PrinterExtensionManager::EnableEvents() on exactly
        /// one printer driver id. The id could come in from a command line argument, thereby enabling
        /// one application binary to work with multiple printer driver ids.
        /// </summary>
        private const string PrinterDriverID = "{E0691E8D-F7CC-456E-A7B5-D1FC19BA2279}";

        /// <summary>
        /// Instance of the PrinterExtensionManager. It is recommended that you have only instance
        /// of the PrinterExtensionManager per application instance.
        /// </summary>
        private static PrinterExtensionManager manager = null;
    }
}
