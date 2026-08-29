using Windows.Foundation;
using Windows.Graphics.Printing.Workflow;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;

namespace PrintSupportApp
{
    public sealed partial class JobActivatedMainPage : Page
    {
        public PrintWorkflowJobUISession Session { get; set; }

        private static Deferral PdlDataAvailableDeferral { get; set; }

        private static Deferral SessionJobNotificationDeferral { get; set; }

        public JobActivatedMainPage()
        {
            InitializeComponent();
        }

        public static void CloseDialog()
        {
            if (SessionJobNotificationDeferral != null)
            {
                SessionJobNotificationDeferral.Complete();
            }

            if (PdlDataAvailableDeferral != null)
            {
                PdlDataAvailableDeferral.Complete();
            }

            Application.Current.Exit();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (e.Parameter is PrintWorkflowJobActivatedEventArgs jobActivatedEventArgs)
            {
                Session = jobActivatedEventArgs.Session;
                Session.JobNotification += OnSessionJobNotification;
                Session.PdlDataAvailable += OnSessionPdlDataAvailable;
                Session.Start();
            }
        }

        private void OnSessionJobNotification(PrintWorkflowJobUISession sender, PrintWorkflowJobNotificationEventArgs args)
        {
            SessionJobNotificationDeferral = args.GetDeferral();

            // Note: OnSessionJobNotification is not called in an UI thread, so we must use the CoreWindow Dispatcher to run any code that updates the UI.
            // Use `_ =` to squash CS4014 warning and indicate it's okay for execution of this method to continue while the UI thread work is done.
            _ = Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                SetNavigationViewSelectedItem("JobNotificationExample");
                contentFrame.Navigate(typeof(JobNotificationExample), args);
            });
        }

        private void OnSessionPdlDataAvailable(PrintWorkflowJobUISession sender, PrintWorkflowPdlDataAvailableEventArgs args)
        {
            PdlDataAvailableDeferral = args.GetDeferral();

            // Note: OnSessionPdlDataAvailable is not called in an UI thread, so we must use the CoreWindow Dispatcher to run any code that updates the UI.
            // Use `_ =` to squash CS4014 warning and indicate it's okay for execution of this method to continue while the UI thread work is done.
            _ = Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                SetNavigationViewSelectedItem("WatermarkManipulationExample");
                contentFrame.Navigate(typeof(WatermarkManipulationExample), args);
            });
        }

        // Helper method to update the selected menu item.
        private void SetNavigationViewSelectedItem(string tag)
        {
            foreach (Framework​Element item in NavigationViewControl.MenuItems)
            {
                if (item.Tag.ToString() == tag)
                {
                    NavigationViewControl.SelectedItem = item;
                    break;
                }
            }
        }
    }
}
