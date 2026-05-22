using Windows.Graphics.Printing.PrintSupport;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;

namespace PrintSupportApp
{
    public sealed partial class SettingsActivatedMainPage : Page
    {
        public PrintSupportSettingsUISession Session { get; set; }

        public SettingsActivatedMainPage()
        {
            InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (e.Parameter is PrintSupportSettingsActivatedEventArgs settingsActivatedEventArgs)
            {
                Session = settingsActivatedEventArgs.Session;

                string openedFrom = Session.LaunchKind.ToString();
                PrintTicketManipulationExampleNavigationItem.Content = $"Print Ticket Manipulation Example ({openedFrom})";
            }
        }

        private void NavViewItemInvoked(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            string tag = args.InvokedItemContainer.Tag as string;
            if (tag == "PrintTicketManipulationExample")
            {
                contentFrame.Navigate(typeof(PrintTicketManipulationExample), Session);
            }
        }

        private void NavViewLoaded(object sender, RoutedEventArgs e)
        {
            // NavView doesn't load any page by default, so load the first page.
            NavigationViewControl.SelectedItem = NavigationViewControl.MenuItems[0];
            contentFrame.Navigate(typeof(PrintTicketManipulationExample), Session);
        }
    }
}
