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
//     This file contains helper methods that provide a data-binding friendly way to access PrintSchema APIs.

using System;
using System.Collections.Generic;
using System.Text;

using System.Runtime.InteropServices;

using Microsoft.Samples.Printing.PrinterExtension.Types;

namespace Microsoft.Samples.Printing.PrinterExtension.Helpers
{
    /// <summary>
    /// Contains helper methods that provide a data-binding friendly way to access PrintSchema APIs.
    /// </summary>
    public class PrintSchemaHelper
    {
        /// <summary>
        /// Constructor. Warning constructing this object is expensive, and is best performed
        /// asynchronously.
        /// </summary>
        /// <param name="ticket">The print ticket for which features/options will be retrieved</param>
        /// <param name="featureNameCollection">List of features requested</param>
        internal PrintSchemaHelper(IPrintSchemaTicket ticket, IEnumerable<String> featureNameCollection)
        {
            _ticket = ticket;
            _featureNameCollection = featureNameCollection;
            _capabilities = _ticket.GetCapabilities();
        }

        /// <summary>
        /// Retrieve the list of features from the current print ticket.
        /// </summary>
        public List<PrintSchemaFeatureHelper> Features
        {
            get
            {
                _featureHelperCollection = new List<PrintSchemaFeatureHelper>();

                //
                // Retrieve the list of features supported by the driver
                //

                foreach (string name in _featureNameCollection)
                {
                    //
                    // If the feature is not present in the print ticket or the print capabilities,
                    // ignore it and continue.
                    //

                    IPrintSchemaFeature ticketFeature = _ticket.GetFeatureByKeyName(name);
                    if (ticketFeature == null)
                    {
                        continue;
                    }

                    IPrintSchemaFeature capabilitiesFeature = _capabilities.GetFeatureByKeyName(name);
                    if (capabilitiesFeature == null)
                    {
                        continue;
                    }

                    // If the feature is not meant to be displayed on the UI, ignore it and continue.
                    if (!capabilitiesFeature.DisplayUI)
                    {
                        continue;
                    }

                    _featureHelperCollection.Add(new PrintSchemaFeatureHelper(ticketFeature, _capabilities, capabilitiesFeature));
                }

                return _featureHelperCollection;
            }
        }

        /// <summary>
        /// List of features requested.
        /// </summary>
        private IEnumerable<string> _featureNameCollection;

        /// <summary>
        /// Helper objects that wrap an IPrintSchemaFeature object.
        /// </summary>
        private List<PrintSchemaFeatureHelper> _featureHelperCollection = null;

        /// <summary>
        /// Print ticket passed into this class.
        /// </summary>
        private IPrintSchemaTicket _ticket = null;

        /// <summary>
        /// Print capabilities object.
        /// </summary>
        private IPrintSchemaCapabilities _capabilities = null;
    }

    /// <summary>
    /// Contains helper methods that provide a data-binding friendly way to access IPrintSchemaFeature APIs.
    /// 
    /// Note: This sample does not handle Print Ticket/Print Capabilities Options which rely on parameters to
    /// be specified, such as psk:Custom , psk:CustomSquare, or psk:CustomMediaSize. If these options are
    /// supported by compatible print drivers, then the printer extension should be modified to support them
    /// appropriately.
    /// </summary>
    public class PrintSchemaFeatureHelper
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="ticketFeature">Object retrieved via a call to IPrintSchemaTicket::GetFeature/GetFeatureByKeyName</param>
        /// <param name="capabilities">Print capabilities object</param>
        /// <param name="capabilitiesFeature">Object retrieved via a call to IPrintSchemaCapabilities::GetFeature/GetFeatureByKeyName</param>
        internal PrintSchemaFeatureHelper(IPrintSchemaFeature ticketFeature, IPrintSchemaCapabilities capabilities, IPrintSchemaFeature capabilitiesFeature)
        {
            //
            // Populate the properties exposed by this class.
            //

            DisplayName = capabilitiesFeature.DisplayName;
            Options = new List<IPrintSchemaOption>(capabilities.GetOptions(ticketFeature));

            foreach (IPrintSchemaOption option in Options)
            {
                if (option.Selected)
                {
                    _selectedOption = option;
                    break;
                }
            }

            _printTicketFeature = ticketFeature;
        }

        /// <summary>
        /// Returns the display name for the current IPrintSchemaFeature object.
        /// </summary>
        public string DisplayName
        {
            get;
            private set;
        }

        /// <summary>
        /// Retrieve the list of options supported for the current feature.
        /// </summary>
        public List<IPrintSchemaOption> Options
        {
            get;
            private set;
        }

        /// <summary>
        /// A 'get' invocation on this property returns the selected option for this print ticket feature.
        /// A 'set' invocation on this property sets the selected option for this print ticket feature.
        /// </summary>
        public IPrintSchemaOption SelectedOption
        {
            get
            {
                return _selectedOption;
            }
            set
            {
                _selectedOption = value;
                _printTicketFeature.SelectedOption = _selectedOption;
            }
        }

        /// <summary>
        /// Feature object retrieved from the print ticket.
        /// </summary>
        private IPrintSchemaFeature _printTicketFeature = null;

        /// <summary>
        /// Selected option for the print ticket feature.
        /// </summary>
        private IPrintSchemaOption _selectedOption = null;

    }
}
