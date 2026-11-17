using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.Storage.Streams;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media.Imaging;
using XpsUtil;

namespace PrintSupportApp
{
    public sealed partial class WatermarkPreviewControl : UserControl
    {
        private BitmapImage image = new BitmapImage();
        public XpsPageWrapper PreviewPage;

        // Callback to allow an interested party to perform modifications to the XpsPageWrapper before the preview is shown.
        // This will be called each time the preview is rendered, with a copy of a single-page document representing the
        // current page being previewed. Modifications made to the XpsPageWrapper will be shown in the preview pane.
        public Action<XpsPageWrapper> ModifyPageBeforePreview;

        private CancellationTokenSource ActiveRenderTask;

        public WatermarkPreviewControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Starts a background task to update the preview. If set, the ModifyDocumentBeforePreview
        /// callback will be invoked (on a background thread) to allow the caller to make desired
        /// modifications to the preview page before it is rendered.
        /// </summary>
        public void StartPreviewRefresh()
        {
            // Cancel currently active preview refresh, if any
            // (this could happen often during live refresh if the user types quickly)
            if (ActiveRenderTask != null)
            {
                ActiveRenderTask.Cancel();
            }

            // Create and store a new CancellationToken for the new Task
            ActiveRenderTask = new CancellationTokenSource();
            CancellationToken ct = ActiveRenderTask.Token;

            // Start a new Task to refresh the preview
            Task.Run(async () =>
            {
                try
                {
                    await PreviewRefresh(ct);
                }
                // The operation will be cancelled if a newer one is started before this one finishes.
                catch (OperationCanceledException) { }
            }, ActiveRenderTask.Token);
        }

        private async Task PreviewRefresh(CancellationToken ct)
        {
            if (PreviewPage == null)
            {
                // Nothing to do; probably the document isn't loaded yet
                return;
            }

            // If a newer preview is in progress; cancel showing this one to avoid race conditions and doing unnecessary work
            ct.ThrowIfCancellationRequested();

            // Make a clone of this single page, so we don't dirty the underlying pages
            XpsPageWrapper clonedPage = PreviewPage.Clone();

            // If the callback has been set, allow the interested object to modify the single-page document as it wishes.
            if (ModifyPageBeforePreview != null)
            {
                ModifyPageBeforePreview(clonedPage);
            }

            // Convert the first page of the single-page document to a BMP
            IRandomAccessStream bmpStream = clonedPage.RenderPageToBMP();
            bmpStream.Seek(0);

            ct.ThrowIfCancellationRequested();
            // Jump to the main UI thread to update the preview image
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                image.SetSource(bmpStream);
                WatermarkPreview.Source = image;
            });
        }

        /// <summary>
        /// Pass along event to an interested party when the image is tapped.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WatermarkPreview_Tapped(object sender, TappedRoutedEventArgs e)
        {
            if (WatermarkPreviewTapped != null)
            {
                WatermarkPreviewTapped(sender, e);
            } 
        }
        public event TappedEventHandler WatermarkPreviewTapped;
    }
}
