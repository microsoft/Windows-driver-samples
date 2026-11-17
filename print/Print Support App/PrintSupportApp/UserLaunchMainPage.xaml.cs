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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace PrintSupportApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class UserLaunchMainPage : Page
    {
        public UserLaunchMainPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
        }

        private void NavViewItemInvoked(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            string tag = args.InvokedItemContainer.Tag as string;
            if (tag == "GetIppPrinterUrl")
            {
                contentFrame.Navigate(typeof(GetIppPrinterUrl), args);
            }
            else
            {
                contentFrame.Navigate(typeof(AppInfo), args);
            }
        }

        private void NavViewLoaded(object sender, RoutedEventArgs e)
        {
            // NavView doesn't load any page by default, so load the first page.
            NavigationViewControl.SelectedItem = NavigationViewControl.MenuItems[0];
            contentFrame.Navigate(typeof(AppInfo), e);
        }
    }
}
