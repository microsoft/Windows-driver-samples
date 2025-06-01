using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Devices.Enumeration;
using System.Threading.Tasks;

namespace PrintSupportApp
{
    /// <summary>
    /// The very basic sample of getting the IPP Printer Url by
    /// quering the System.Devices.Aep.DeviceAddress device property with
    /// Windows.Devices.Enumeration's DeviceInformation.FindAllAsync.
    /// </summary>
    public sealed partial class GetIppPrinterUrl : Page
    {
        public GetIppPrinterUrl()
        {
            this.InitializeComponent();
            DataContext = this;
        }

        public string Manufacturer { get; set; }

        private bool QueryIsStarted
        {
            set
            {
                if (value)
                {
                    this.StartQueryButton.IsEnabled = false;
                    QueryProgressBar.Visibility = Visibility.Visible;
                    QueryResultListView.Items.Clear();
                }
                else
                {
                    this.StartQueryButton.IsEnabled = true;
                    QueryProgressBar.Visibility = Visibility.Collapsed;
                }
            }
        }

        private void OnStartQueryButtonClicked(object sender, RoutedEventArgs e)
        {
            QueryIsStarted = true;

            // Association Endpoint Query
            string aqsPrintAepFilter = "System.Devices.Aep.Category:~~\"PrintFax.Printer\"" +
                " AND System.Devices.Aep.ProtocolId:=\"{9f73fafd-2343-4bf9-a532-ad20e2674bea}\"" + // filter by IPP protocol id
                (!String.IsNullOrEmpty(Manufacturer) ? $" AND System.Devices.Aep.Manufacturer:~~\"{Manufacturer}\"" : String.Empty);

            DeviceInformation.FindAllAsync(
                      aqsPrintAepFilter
                    , new List<string> { "System.Devices.Aep.DeviceAddress", "System.Devices.Aep.Category",
                            "System.Devices.Aep.Manufacturer", "System.Devices.Aep.ProtocolId" }
                    , DeviceInformationKind.AssociationEndpoint).AsTask().ContinueWith(aepQueryTask =>
            {
                if (aepQueryTask.Status == TaskStatus.RanToCompletion)
                {
                    ShowIppPrinterUrls(aepQueryTask.Result);
                }
                else
                {
                    QueryResultListView.Items.Add("Query has failed");
                }
                QueryIsStarted = false;
            }, System.Threading.Tasks.TaskScheduler.FromCurrentSynchronizationContext());
        }

        private string GetPropertyAsString(DeviceInformation deviceInfo, string propertyName)
        {
            object objectPropertyValue = null;
            if (deviceInfo.Properties.TryGetValue(propertyName, out objectPropertyValue))
            {
                return objectPropertyValue.ToString();
            }
            return null;
        }

        // Parse query result and update UI
        private void ShowIppPrinterUrls(DeviceInformationCollection associatedEndpointDeviceInfos)
        {
            foreach (var deviceInfo in associatedEndpointDeviceInfos)
            {
                var printerName = GetPropertyAsString(deviceInfo, "System.ItemNameDisplay");
                var printerUrl = GetPropertyAsString(deviceInfo, "System.Devices.Aep.DeviceAddress");
                // Note isPaired will be always false for 10X by design
                var isPaired = GetPropertyAsString(deviceInfo, "System.Devices.Aep.IsPaired");

                QueryResultListView.Items.Add($"Printer: '{printerName}', Url: '{printerUrl}', IsPaired '{isPaired}'");
            }

            if (QueryResultListView.Items.Count == 0)
            {
                QueryResultListView.Items.Add("No printers found.");
            }
        }
    }
}
