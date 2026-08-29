using Windows.ApplicationModel;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;

namespace PrintSupportApp
{
    public sealed partial class AppInfo : Page
    {
        public AppInfo()
        {
            InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            PackageFamilyNameTextBox.Text = Package.Current.Id.FamilyName;
        }
    }
}
