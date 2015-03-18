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
//     This file contains as the interaction logic and data-binding code/sources for the WPF print preferences window.

using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;

using System.Reflection;
using System.IO;

using System.Xml.Linq;

using System.Runtime.InteropServices;

using Microsoft.Samples.Printing.PrinterExtension.Types;
using Microsoft.Samples.Printing.PrinterExtension.Helpers;

namespace Microsoft.Samples.Printing.PrinterExtension
{
    /// <summary>
    /// Interaction logic for PrintPreferenceWindow.xaml.
    /// </summary>
    public partial class PrintPreferenceWindow : Window, INotifyPropertyChanged
    {
        public PrintPreferenceWindow()
        {
            InitializeComponent();
        }

        /// <summary>
        /// This method sets up data binding sources and performs other initialization tasks.
        /// </summary>
        /// <param name="eventArgs"></param>
        public void Initialize(PrinterExtensionEventArgs eventArgs)
        {
            //
            // Populate the data binding sources.
            //

            DataContext = this;

            printerExtensionEventArgs = eventArgs;
            PrinterQueue = eventArgs.Queue;
            displayedPrintTicket = eventArgs.Ticket;

            //
            // Send a bidi query requesting ink levels.
            //
            // Please note: As this event will fire many times, it is recommended to maintain event
            // listeners for the life time of the application. Furthermore, the relationship to this
            // being invoked and the calling SendBidiQuery() is not 1:1; in fact, it is *:N, where the 
            // listener may be called several times with bidi updates.
            // 
            //

            PrinterQueue.OnBidiResponseReceived += OnBidiResponseReceived;
            PrinterQueue.SendBidiQuery("\\Printer.consumables");
        }

        #region UI code

        /// <summary>
        /// This event handler is invoked when the window is closing. It is important to Cancel or Complete the request when the window is closing.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void PrintPreferenceWindow_Closing(object sender, EventArgs e)
        {
            // Since we are a different process from the printing application we need to hand focus back when complete.
            WindowHelper.SetForegroundWindow(printerExtensionEventArgs.WindowParent);

            if (!requestCompleted)
            {
                printerExtensionEventArgs.Request.Cancel((int)HRESULT.S_FALSE, "The user canceled the operation.");
                requestCompleted = true;
            }
        }

        /// <summary>
        ///  Button click event handler.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Button_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;

            switch (clickedButton.Name)
            {
                case "CancelButton":
                    CancelRequestAndCloseWindow();
                    break;

                case "OkButton":
                    //
                    // Validate the print ticket asynchronously. The event handler is invoked when the validation is completed.
                    //

                    IPrintSchemaAsyncOperation asyncOperation = displayedPrintTicket.ValidateAsync();

                    //
                    // Pop up a modal dialog that prevents the user from changing selections when validation is in progress.
                    // Since this dialog lives as long as the parent window, it is not mandatory to unregister the delegate for the
                    // 'Completed' event.
                    //

                    ValidationModalDialog.Completed += PrintTicketValidateCompleted; // This operation is idempotent.
                    ValidationModalDialog.StartAsyncOperation(asyncOperation);
                    break;

                case "VerifyButton":
                    //
                    // Force WPF Data binding to refresh the UI. This operation retrieves
                    // a fresh print capabilities for the print ticket based on the user selections.
                    //

                    PropertyChanged(this, new PropertyChangedEventArgs("PrintSchemaHelperSource"));
                    break;
            }
        }

        /// <summary>
        /// Close the window in a thread-safe way.
        /// </summary>
        private void CloseWindow()
        {
            this.Dispatcher.BeginInvoke(new Action(() =>
            {
                this.Close();
            }));
        }

        #endregion


        #region Data binding sources

        /// <summary>
        /// The Print queue for which this window is being displayed.
        /// </summary>
        public IPrinterQueue PrinterQueue { get; private set; }

        /// <summary>
        /// The title for the ink status display.
        /// </summary>
        public string InkStatusTitle { get; private set; }

        /// <summary>
        /// Retrieve a new instance of PrintSchemaHelper, based on the current print ticket being displayed.
        /// PrintSchemaHelper encapsulates all the features and options required to populate the print preferences UI.
        /// </summary>
        public PrintSchemaHelper PrintSchemaHelperSource
        {
            get
            {
                //
                // Below is the list of features that will be displayed in the print preferences window.
                // The features are declared here for convenience/readability.
                //
                // In performant code, this array would be allocated only once per run of the application.
                //

                string[] featureNames = {
                                            "DocumentNUp",
                                            "PageMediaSize",
                                            "DocumentInputBin",
                                            "PageOrientation",
                                            "PageMediaType",
                                            "PageBorderless",
                                            "JobInputBin",
                                            "PageOutputColor",
                                            "DocumentCollate",
                                            "DocumentDuplex"
                                        };

                return new PrintSchemaHelper(displayedPrintTicket, featureNames);
            }
        }

        /// <summary>
        /// Encapsulates the information required to populate ink level.
        /// </summary>
        public BidiHelper BidiHelperSource { get; private set; }

        /// <summary>
        /// This event is raised when a data from a binding source is modified.
        /// </summary>
        public event PropertyChangedEventHandler PropertyChanged;

        #endregion

        #region Ink level display
        /// <summary>
        /// This is the method invoked when a bidi response is received.
        /// </summary>
        /// <param name="sender">IPrinterQueue object.</param>
        /// <param name="e">The results of the bidi response.</param>
        private void OnBidiResponseReceived(object sender, PrinterQueueEventArgs e)
        {
            if (e.StatusHResult != (int)HRESULT.S_OK)
            {
                MockInkStatus();
                return;
            }

            //
            // Display the ink levels from the mock data.
            //

            BidiHelperSource = new BidiHelper(e.Response);
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs("BidiHelperSource"));
            }
            InkStatusTitle = "Ink status (Live data)";
        }

        /// <summary>
        /// This method is invoked when there is an error retrieving Bidi information.
        /// A mock bidi response is loaded from resource and displayed.
        /// </summary>
        private void MockInkStatus()
        {
            //
            // Load mock bidi response resource.
            //

            Assembly a = Assembly.GetExecutingAssembly();
            Stream xmlData = a.GetManifestResourceStream("PrinterExtensionSample.bidi_Ink_mock.xml");
            StreamReader sr = new StreamReader(xmlData);
            string xmlString = sr.ReadToEnd();

            //
            // Display the ink levels from the mock data.
            //

            BidiHelperSource = new BidiHelper(xmlString);
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs("BidiHelperSource"));
            }
            InkStatusTitle = "Ink status (Mocked data)";
        }
        #endregion

        #region PrintSchema-related code
        /// <summary>
        /// Cancel the current printer extension event and close the current window.
        /// </summary>
        private void CancelRequestAndCloseWindow()
        {
            printerExtensionEventArgs.Request.Cancel((int)HRESULT.S_FALSE, "User canceled the operation");
            requestCompleted = true;
            CloseWindow();
        }

        /// <summary>
        /// Invoked when asynchronous print ticket validation is complete.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PrintTicketValidateCompleted(object sender, PrintSchemaAsyncOperationEventArgs e)
        {
            //
            // Print ticket validation completed successfully i.e. print ticket selections are not constrained.
            // The print ticket needs to be committed, and then window can be closed.
            //

            if (e.StatusHResult == (int)HRESULT.S_PT_NO_CONFLICT)
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    CommitPrintTicketAsync(e.Ticket);
                }));

            }
            else
            {
                //
                // The ticket selections are constrained.
                //

                this.Dispatcher.Invoke(new Action(() =>
                {
                    HandleTicketConstraints(e.Ticket);
                }));
            }
        }

        /// <summary>
        /// Invoked when there are constraints in the print ticket selections.
        /// </summary>
        /// <param name="validatedTicket"></param>
        private void HandleTicketConstraints(IPrintSchemaTicket validatedTicket)
        {
            //
            // Retrieved localized display strings from a resource file/
            //
            string selectionConflictsFound = PrinterExtensionSample.Strings.SelectionConflictsFound;
            string selectionConflictsTitle = PrinterExtensionSample.Strings.SelectionConflictsTitle;

            MessageBoxResult result = MessageBox.Show(
                                        this,
                                        selectionConflictsFound,
                                        selectionConflictsTitle,
                                        MessageBoxButton.YesNoCancel);

            if (result == MessageBoxResult.Yes)
            {
                CommitPrintTicketAsync(validatedTicket);
            }
            else
            {
                PropertyChanged(this, new PropertyChangedEventArgs("PrintSchemaHelperSource"));
            }
        }

        /// <summary>
        /// Commits the input print ticket asynchronously. The completed event handler is expected to close the window.
        /// </summary>
        /// <param name="validatedTicket"></param>
        private void CommitPrintTicketAsync(IPrintSchemaTicket validatedTicket)
        {
            IPrintSchemaAsyncOperation commitAsyncOperation = printerExtensionEventArgs.Ticket.CommitAsync(validatedTicket);
            commitAsyncOperation.Completed += PrintTicketCommitCompleted;
            commitAsyncOperation.Start();
        }

        /// <summary>
        /// Invoked when the user's selections have been committed into the print ticket.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PrintTicketCommitCompleted(object sender, PrintSchemaAsyncOperationEventArgs e)
        {
            CompleteRequestAndCloseWindow();
        }

        /// <summary>
        /// Complete the current printer extension request and close the current window.
        /// </summary>
        private void CompleteRequestAndCloseWindow()
        {
            //
            // It is important to invoke the IPrinterExtensionRequest::Complete method from the thread the
            // class instance was create on (i.e. the UI thread).
            //

            this.Dispatcher.Invoke(new Action(() =>
            {
                printerExtensionEventArgs.Request.Complete();
            }));

            requestCompleted = true;
            CloseWindow();
        }

        /// <summary>
        /// Demonstrates how to modify print ticket XML. This piece of code does not perform any functionality. 
        /// It serves to demonstrate the usage of IPrintSchemaTicket::GetReadStream()/GetWriteStream()
        /// </summary>
        private void ModifyPrintTicketXml()
        {
            //
            // Load the ticket XML (as a Stream) into an XElement object.
            //

            XElement ticketRootXElement = null;
            using (Stream ticketReadStream = displayedPrintTicket.GetReadStream())
            {
                ticketRootXElement = XElement.Load(ticketReadStream);
            }

            //
            // Perform any modifications on the XElement object.
            //


            //
            // Write the changes back to the print ticket.
            //
            using (Stream ticketWriteStream = displayedPrintTicket.GetWriteStream())
            {
                ticketRootXElement.Save(ticketWriteStream);
            }
        }

        #endregion

        /// <summary>
        /// The arguments passed in for this print preferences event.
        /// </summary>
        private PrinterExtensionEventArgs printerExtensionEventArgs = null;

        /// <summary>
        /// Reflects the currently displayed print preference options.
        /// </summary>
        private IPrintSchemaTicket displayedPrintTicket = null;

        /// <summary>
        /// Determines if IPrinterExtensionRequest::Complete()/Cancel() has been invoked for this Window.
        /// instance
        /// </summary>
        private bool requestCompleted = false;
    }

    /// <summary>
    /// This class transforms the boolean 'IPrintSchemaOption.Constrained' into a visual form.
    /// </summary>
    public class OptionConstrainedToDisplayColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value == null)
            {
                return "Black";
            }

            //
            // If the option is not constrained, it will be diplayed in black.
            //

            PrintSchemaConstrainedSetting constrained = (PrintSchemaConstrainedSetting)value;
            if (constrained == PrintSchemaConstrainedSetting.None)
            {
                return "Black";
            }

            //
            // If the option is constrained, it will be displayed in red.
            //

            return "Red";
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Provides a friendlier way to use HRESULT error codes.
    /// </summary>
    enum HRESULT : int
    {
        S_OK = 0x0000,
        S_FALSE = 0x0001,
        S_PT_NO_CONFLICT = 0x40001,
        E_INVALIDARG = unchecked((int)0x80070057),
        E_OUTOFMEMORY = unchecked((int)0x8007000E),
        ERROR_NOT_FOUND = unchecked((int)0x80070490)
    }
}
