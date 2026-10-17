using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics.Printing.PrintSupport;
using Windows.Graphics.Printing.PrintTicket;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using System.Collections.ObjectModel;
using System.Linq;

namespace PrintSupportApp
{
    public sealed partial class PrintTicketManipulationExample : Page
    {
        private const string PRINT_TICKET_SCHEMA_NAMESPACE = "http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords";
        private const string IPP_PRINT_TICKET_SCHEMA_NAMESPACE = "http://schemas.microsoft.com/windows/2018/04/printing/printschemakeywords/Ipp";

        public PrintTicketManipulationExample()
        {
            InitializeComponent();
        }

        private PrintSupportSettingsUISession Session { get; set; }
        private WorkflowPrintTicket PrintTicket { get; set; }
        private PrintTicketCapabilities Capabilities { get; set; }

        private ComboBox CreatePrintTicketFeatureComboBox(PrintTicketFeature feature, bool useDefaultEventHandler = true)
        {
            if (feature == null)
            {
                return null;
            }

            var comboBox = new ComboBox
            {
                // Header is displayed in the UI, ontop of the ComboBox.
                Header = feature.DisplayName
            };
            // Construct a new List since IReadOnlyList does not support the 'IndexOf' method.
            var options = new ObservableCollection<PrintTicketOption>(feature.Options);
            // Provide the combo box with a list of options to select from.
            comboBox.ItemsSource = options;
            // Set the selected option to the option set in the print ticket.
            var featureOption = feature.GetSelectedOption();
            PrintTicketOption selectedOption;
            try
            {
                selectedOption = options.Single((option) => (
                    option.Name == featureOption.Name && option.XmlNamespace == featureOption.XmlNamespace));
            }
            // Catch exceptions, because there can be multiple features with the "None" feature name.
            // We need to handle those features seperately.
            catch (System.SystemException exception)
            {
                var nameAttribute = featureOption.XmlNode.Attributes.GetNamedItem("name");
                var attribute = featureOption.XmlNode.OwnerDocument.CreateAttribute("name");

                selectedOption = options.Single((option) => (
                    option.DisplayName == featureOption.DisplayName && option.Name == featureOption.Name && option.XmlNamespace == featureOption.XmlNamespace));

            }
            comboBox.SelectedIndex = options.IndexOf(selectedOption);

            // Disable the combo box if there is only one selection.
            if (options.Count == 1)
            {
                comboBox.IsEnabled = false;
            }
            // Set the event handler for when the selection is changed.
            if (useDefaultEventHandler)
            {
                comboBox.SelectionChanged += PrintTicketFeatureComboBoxSelectionChanged;
            }
            // Set the print ticket feature as the data context. This is used in the SelectionChanged event handler.
            comboBox.DataContext = feature;

            return comboBox;
        }

        private NumberBox CreatePrintTicketParameterNumberBox(PrintTicketParameterInitializer param, bool useDefaultEventHandler = true)
        {
            if (param == null)
            {
                return null;
            }

            var numberBox = new NumberBox
            {
                // Set the initial value of the NumberBox.
                Value = param.Value.GetValueAsInteger()
            };
            // Set the event handler for when the value is changed.
            if (useDefaultEventHandler)
            {
                numberBox.ValueChanged += PrintTicketParameterNumberBoxValueChanged;
            }
            // Set the print ticket parameter as the data context. This is used in the ValueChanged event handler.
            numberBox.DataContext = param;
            // Get the parameter definitions. 
            var capabilities = Capabilities.GetParameterDefinition(param.Name, param.XmlNamespace);
            // Get the property display name set in the XML with a XPath.
            string displayName = capabilities.XmlNode.SelectSingleNode("//*[local-name()='Property'][contains(@name, 'DisplayName')]").InnerText;
            // Header is displayed in the UI, ontop of the NumberBox.
            numberBox.Header = displayName;
            // Set the range limits for the NumberBox.
            numberBox.Minimum = capabilities.RangeMin;
            numberBox.Maximum = capabilities.RangeMax;

            return numberBox;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (e.Parameter == null)
            {
                return;
            }

            Session = e.Parameter as PrintSupportSettingsUISession;
            // WorkflowPrintTicket is the currently set features and properties for the print job.
            PrintTicket = Session.SessionPrintTicket;
            // PrintTicketCapabilities is all of the possible features and properties for the target printer.
            Capabilities = PrintTicket.GetCapabilities();
            // A list of Controls that will be rendered in the UI.
            var printTicketControls = new List<Control>();

            //
            // Example 1.1 Basic print ticket features.
            //

            // Create a list of print ticket features.
            // Each feature is a device capability that has an enumerable list of settings.
            var printTicketFeatures = new List<PrintTicketFeature>
            {
                // Add features defined in PrintTicket properties.
                // These are wrappers for commonly used features.
                PrintTicket.DocumentBindingFeature,
                PrintTicket.DocumentDuplexFeature,
                PrintTicket.DocumentHolePunchFeature,
                PrintTicket.DocumentInputBinFeature,
                PrintTicket.DocumentNUpFeature,
                PrintTicket.DocumentStapleFeature,
                PrintTicket.JobPasscodeFeature,
                PrintTicket.PageMediaSizeFeature,
                PrintTicket.PageMediaTypeFeature,
                PrintTicket.PageOrientationFeature,
                PrintTicket.PageOutputColorFeature,
                PrintTicket.PageOutputQualityFeature,
                PrintTicket.PageResolutionFeature,
                // Add other print ticket features, not defined in PrintTicket properties.
                PrintTicket.GetFeature("JobNUpAllDocumentsContiguously", PRINT_TICKET_SCHEMA_NAMESPACE),
                PrintTicket.GetFeature("JobOutputBin", PRINT_TICKET_SCHEMA_NAMESPACE),
                PrintTicket.GetFeature("JobPageOrder", PRINT_TICKET_SCHEMA_NAMESPACE),
                PrintTicket.GetFeature("PageBorderless", PRINT_TICKET_SCHEMA_NAMESPACE),
                // Add features from a different XML namespace.
                PrintTicket.GetFeature("JobNUpPresentationDirection", IPP_PRINT_TICKET_SCHEMA_NAMESPACE),
            };

            foreach (var feature in printTicketFeatures)
            {
                // If the printer does not support the feature, it will be null.
                if (feature != null)
                {
                    printTicketControls.Add(CreatePrintTicketFeatureComboBox(feature));
                }
            }

            //
            // Example 1.2 Inter-dependencies between print ticket controls.
            // Scenario: The collateComboBox is enabled only when the copiesNumberBox has a value greater than 1.
            //

            var collateFeature = PrintTicket.DocumentCollateFeature;
            var copiesParameter = PrintTicket.GetParameterInitializer("JobCopiesAllDocuments", PRINT_TICKET_SCHEMA_NAMESPACE);

            var collateComboBox = CreatePrintTicketFeatureComboBox(collateFeature);
            var copiesNumberBox = CreatePrintTicketParameterNumberBox(copiesParameter, false);
            if (collateComboBox != null && copiesNumberBox != null)
            {
                // Only enable the collate feature when copies count is greater than 1.
                collateComboBox.IsEnabled = copiesNumberBox.Value > 1;
                // Add a custom event handler to conditionally enable the collateComboBox.
                copiesNumberBox.ValueChanged += (numberBox, eventArgs) =>
                {
                    collateComboBox.IsEnabled = numberBox.Value > 1;
                    PrintTicketParameterNumberBoxValueChanged(numberBox, eventArgs);
                };

                printTicketControls.Add(collateComboBox);
                printTicketControls.Add(copiesNumberBox);
            }
            else if (collateComboBox != null)
            {
                // The print ticket does not have the JobCopiesAllDocuments parameter.
                // Don't need to conditionally enable the collateComboBox.
                printTicketControls.Add(collateComboBox);
            }
            else if (copiesNumberBox != null)
            {
                // The print ticket does not have the DocumentCollateFeature.
                // Don't need to conditionally enable the collateComboBox.
                // Use the default value changed event handler.
                copiesNumberBox.ValueChanged += PrintTicketParameterNumberBoxValueChanged;
                printTicketControls.Add(copiesNumberBox);
            }

            // Add all the controls to be rendered in the UI.
            PrintTicketFeatureComboBoxRepeater.ItemsSource = printTicketControls;
        }

        private void PrintTicketFeatureComboBoxSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var comboBox = sender as ComboBox;
            var feature = comboBox.DataContext as PrintTicketFeature;

            // Disable the ComboBox while saving the print ticket.
            comboBox.IsEnabled = false;
            feature.SetSelectedOption(comboBox.SelectedItem as PrintTicketOption);
            Session.UpdatePrintTicket(PrintTicket);
            comboBox.IsEnabled = true;
        }

        private void PrintTicketParameterNumberBoxValueChanged(NumberBox numberBox, NumberBoxValueChangedEventArgs e)
        {
            var param = numberBox.DataContext as PrintTicketParameterInitializer;

            // Disable the NumberBox while saving the print ticket.
            numberBox.IsEnabled = false;
            PrintTicket.SetParameterInitializerAsInteger(param.Name, param.XmlNamespace, (int)numberBox.Value);
            Session.UpdatePrintTicket(PrintTicket);
            numberBox.IsEnabled = true;
        }
    }
}
