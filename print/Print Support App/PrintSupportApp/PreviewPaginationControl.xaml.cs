using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace PrintSupportApp
{
    /// <summary>
    /// PreviewPaginationControl encapsulates logic to show the current and total pages,
    /// along with buttons to navigate forward and backward.
    /// </summary>
    public sealed partial class PreviewPaginationControl : UserControl
    {
        /// <summary>
        /// Fired when the "previous page" button is clicked.
        /// </summary>
        public event RoutedEventHandler PrevButtonClicked;
        /// <summary>
        /// Fired when the "next page" button is clicked.
        /// </summary>
        public event RoutedEventHandler NextButtonClicked;

        /// <summary>
        /// Specifies the current page. If set to null, "-" will be displayed and pagination
        /// controls will be disabled.
        /// </summary>
        public uint? CurrentPage
        {
            get => _currentPage;
            set
            {
                _currentPage = value;
                CurrentPageLabel.Text = CurrentPage.HasValue ? CurrentPage.Value.ToString() : "-";

                EvaluateButtonEnabledStates();
            }
        }
        private uint? _currentPage = null;

        /// <summary>
        /// Specifies the total number of pages. If set to null, "-" will be displayed and
        /// pagination controls will be disabled.
        /// </summary>
        public uint? TotalPages
        {
            get => _totalPages;
            set
            {
                _totalPages = value;
                TotalPagesLabel.Text = TotalPages.HasValue ? TotalPages.Value.ToString() : "-";
                
                EvaluateButtonEnabledStates();
            }
        }
        private uint? _totalPages = null;


        public PreviewPaginationControl()
        {
            InitializeComponent();
            IsEnabledChanged += WatermarkPreview_IsEnabledChanged;
        }

        /// <summary>
        /// Updates the IsEnabled state of the next- and previous-page buttons.
        /// If the PreviewPaginationControl is disabled, both buttons will be disabled.
        /// 
        /// Otherwise, each button wil be enabled if we know the number of pages in the document,
        /// and pressing the button would not lead to an out-of-bounds page.
        /// </summary>
        private void EvaluateButtonEnabledStates()
        {
            PreviousPageButton.IsEnabled = IsEnabled && CurrentPage.HasValue && CurrentPage.Value > 1;
            NextPageButton.IsEnabled     = IsEnabled && CurrentPage.HasValue && TotalPages.HasValue && CurrentPage.Value < TotalPages;
        }

        /// <summary>
        /// When `IsEnabled` is changed directly on this object, we want to disable the child pagination buttons
        /// </summary>
        private void WatermarkPreview_IsEnabledChanged(object sender, DependencyPropertyChangedEventArgs e) => EvaluateButtonEnabledStates();

        // ------------------
        // Pass along the previous/next page button events to an interested party
        private void PreviousPageButton_Click(object sender, RoutedEventArgs e) => PrevButtonClicked(sender, e);
        private void NextPageButton_Click(object sender, RoutedEventArgs e) => NextButtonClicked(sender, e);
        // ------------------
    }
}
