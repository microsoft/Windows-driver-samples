using Windows.ApplicationModel.Activation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace PrintSupportApp
{
    public sealed partial class App : Application
    {
        public App()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used such as when the application is launched to open a specific file.
        /// </summary>
        /// <param name="e">Details about the launch request and process.</param>
        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {
            var rootFrame = Window.Current.Content as Frame;

            // Do not repeat app initialization when the Window already has content,
            // just ensure that the window is active
            if (rootFrame == null)
            {
                // Create a Frame to act as the navigation context and navigate to the first page
                rootFrame = new Frame();
                // Place the frame in the current Window
                Window.Current.Content = rootFrame;
            }

            if (e.PrelaunchActivated == false)
            {
                if (rootFrame.Content == null)
                {
                    // When the navigation stack isn't restored navigate to the first page,
                    // configuring the new page by passing required information as a navigation
                    // parameter
                    rootFrame.Navigate(typeof(UserLaunchMainPage), e.Arguments);
                }
                // Ensure the current window is active
                Window.Current.Activate();
            }
        }

        protected override void OnActivated(IActivatedEventArgs args)
        {
            var rootFrame = new Frame();
            var pageType = typeof(AppInfo);

            if (args.Kind == ActivationKind.PrintSupportSettingsUI)
            {
                pageType = typeof(SettingsActivatedMainPage);
            }
            else if (args.Kind == ActivationKind.PrintSupportJobUI)
            {
                pageType = typeof(JobActivatedMainPage);
            }

            rootFrame.Navigate(pageType, args);
            Window.Current.Content = rootFrame;
            Window.Current.Activate();
        }
    }
}
