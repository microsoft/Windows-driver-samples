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
//     This file contains as the interaction logic for ValidationModaldialog.
//     This dialog prevents the user from making changes to print ticket settings when asynchronous
//     validation is being performed.

using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;

using Microsoft.Samples.Printing.PrinterExtension.Types;

namespace Microsoft.Samples.Printing.PrinterExtension
{
    /// <summary>
    /// Interaction logic for ValidationModalDialog.xaml
    /// This dialog prevents the user from changing print preferences when validation is in progress.
    /// </summary>
    public partial class ValidationModalDialog : UserControl
    {
        public ValidationModalDialog()
        {
            InitializeComponent();
            Visibility = Visibility.Hidden;
        }

        /// <summary>
        /// Starts the asynchronous operation.
        /// </summary>
        /// <param name="asyncOperationToStart">Async operation context.</param>
        public void StartAsyncOperation(IPrintSchemaAsyncOperation asyncOperationToStart)
        {
            this.asyncOperationContext = asyncOperationToStart;
            Visibility = Visibility.Visible;
            asyncOperationToStart.Completed += asyncOperation_Completed;
            asyncOperationToStart.Start();
        }


        /// <summary>
        /// This method is invoked from a different thread once asynchronous validation is completed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void asyncOperation_Completed(object sender, PrintSchemaAsyncOperationEventArgs e)
        {
            ValidationHResult = e.StatusHResult;
            HideWindow();

            if (Completed != null)
            {
                Completed(this, e);
            }
        }

        /// <summary>
        /// Hides the current window.
        /// </summary>
        private void HideWindow()
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                Visibility = Visibility.Hidden;
            }));
        }


        /// <summary>
        ///  Result of the validation operation.
        /// </summary>
        public int ValidationHResult
        {
            get;
            private set;
        }

        /// <summary>
        /// Invoked then the "Cancel" button is clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CancelValidationButton_Click(object sender, RoutedEventArgs e)
        {
            asyncOperationContext.Cancel();
            HideWindow();
        }

        /// <summary>
        /// Invoked when the asynchronous operation is completed.
        /// </summary>
        public event EventHandler<PrintSchemaAsyncOperationEventArgs> Completed;

        /// <summary>
        /// Asynchronous operation context.
        /// </summary>
        private IPrintSchemaAsyncOperation asyncOperationContext;
    }
}
