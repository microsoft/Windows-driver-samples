using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Tasks;
using Windows.ApplicationModel.VoiceCommands;
using Windows.Devices.Printers;
using Windows.Graphics.Imaging;
using Windows.Graphics.Printing.Workflow;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using XpsUtil;

namespace PrintSupportApp
{
    public sealed partial class WatermarkManipulationExample : Page
    {
        private XpsSequentialDocument document { get; set; }

        private WatermarkOptions WatermarkOptions = new WatermarkOptions();
        private XpsPageWatermarker Watermarker = new XpsPageWatermarker();

        public uint? CurrentPage = null;
        public uint? TotalPages = null;

        private double TextXOffset = 0.1;
        private double TextYOffset = 0.1;
        private bool IsPositioningWatermark = false;

        public WatermarkManipulationExample()
        {
            InitializeComponent();
            DataContext = this;
            LocalStorageUtil.ResetWatermarkTextAndImage();
            FetchImageAttributes();

            WatermarkPreview.ModifyPageBeforePreview = ApplyModificationsToPreview;
        }

        /// <summary>
        /// Fetch the attributes of the image at ImageFilePath.
        /// Called at startup to populate the DPI and dimension attributes of the Contoso logo.
        /// </summary>
        private async void FetchImageAttributes()
        {
            // Load Contoso logo on startup
            var imageUri = new Uri("ms-appx:///" + ImageFilePath);
            var file = await StorageFile.GetFileFromApplicationUriAsync(imageUri);
            SoftwareBitmap contosoLogo = await GetSoftwareBitmapFromFileAsync(file);

            WatermarkOptions.LogoFilePath = ImageFilePath;
            WatermarkOptions.LogoDpiX = contosoLogo.DpiX;
            WatermarkOptions.LogoDpiY = contosoLogo.DpiY;
            WatermarkOptions.LogoHeight = contosoLogo.PixelHeight;
            WatermarkOptions.LogoWidth = contosoLogo.PixelWidth;

            RenderPreview();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (e.Parameter == null)
            {
                return;
            }

            if (e.Parameter is PrintWorkflowPdlDataAvailableEventArgs args)
            {
                OnPdlDataAvailable(args);
            }
        }

        /// <summary>
        /// Example of getting a custom IPP attribute from a printer.
        /// </summary>
        private void InitJobPasswordField(IppPrintDevice printer)
        {
            var requestedAttributes = new string[]
            {
                "job-password-supported",
                "job-password-encryption-supported"
            };
            var attributes = printer.GetPrinterAttributes(requestedAttributes);

            if (requestedAttributes.All(k => attributes.ContainsKey(k)))
            {
                var maxPasswordLength = 0;
                var jobPasswordSuppoted = attributes["job-password-supported"];
                if (jobPasswordSuppoted.Kind == IppAttributeValueKind.Integer)
                {
                    maxPasswordLength = jobPasswordSuppoted.GetIntegerArray().First();
                }

                if (maxPasswordLength > 0)
                {
                    var encryptionSupported = attributes["job-password-encryption-supported"];
                    if (encryptionSupported.Kind == IppAttributeValueKind.Keyword)
                    {
                        jobPasswordEncryptionComboBox.ItemsSource = encryptionSupported.GetKeywordArray();
                        jobPasswordEncryptionComboBox.SelectedIndex = 0;
                    }

                    jobPasswordEncryptionComboBox.Visibility = Visibility.Visible;
                    jobPasswordTextBox.Visibility = Visibility.Visible;
                    jobPasswordTextBox.MaxLength = maxPasswordLength;
                }
            }
        }

        public void OnPdlDataAvailable(PrintWorkflowPdlDataAvailableEventArgs args)
        {
            InitJobPasswordField(args.PrinterJob.Printer);

            PrintWorkflowConfig = args.Configuration;
            //By Default preview button and preview section will be visible

            //This is only for XPS preview
            if (string.Equals(args.SourceContent.ContentType, "application/OXPS", StringComparison.OrdinalIgnoreCase))
            {
                PreviewLoadingProgressRing.IsActive = true;
                PreviewLoadingProgressRing.Visibility = Visibility.Visible;
                PrintButton.IsEnabled = false;

                PreviewPaginator.TotalPages = null;
                PreviewPaginator.CurrentPage = null;

                var xpsContentStream = args.SourceContent.GetInputStream();
                PrintWorkflowObjectModelSourceFileContent xpsContentObjectModel = new PrintWorkflowObjectModelSourceFileContent(xpsContentStream);
                document = new XpsSequentialDocument(xpsContentObjectModel);
                bool hasGotFirstPage = false;

                // Note: pageNum is ONE-INDEXED
                document.PageAdded += async (sender, pageNum) =>
                {
                    if (pageNum > TotalPages.GetValueOrDefault(0))
                    {
                        TotalPages = pageNum;

                        await PreviewPaginator.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                        {
                            PreviewPaginator.TotalPages = TotalPages;
                        });
                    }

                    if (!hasGotFirstPage)
                    {
                        hasGotFirstPage = true;

                        await PreviewPaginator.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                        {
                            PreviewPaginator.CurrentPage = CurrentPage = 1;
                            WatermarkPreview.PreviewPage = sender.GetPage(1);
                            RenderPreview();

                            PrintButton.IsEnabled = true;
                        });
                    }
                };

                document.DocumentClosed += async (sender, e) =>
                {
                    await PreviewPaginator.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                    {
                        PreviewLoadingProgressRing.IsActive = false;
                        PreviewLoadingProgressRing.Visibility = Visibility.Collapsed;

                        RenderPreview();
                    });
                };

                document.XpsGenerationFailed += async (sender, e) =>
                {
                    await XpsGenerationErrorTextBlock.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                    {
                        PreviewLoadingProgressRing.IsActive = false;

                        // Disable all controls except "cancel"
                        PrintButton.IsEnabled = false;
                        WatermarkTextBox.IsEnabled = false;
                        TextSizeBox.IsEnabled = false;
                        ImageCheckBox.IsEnabled = false;
                        PreviewPaginator.IsEnabled = false;
                        PositionWatermarkButton.IsEnabled = false;
                        PreviewPaginator.CurrentPage = PreviewPaginator.TotalPages = null;

                        XpsGenerationErrorTextBlock.Visibility = Visibility.Visible;
                    });
                };

                document.StartXpsOMGeneration();
            }
            else
            {
                //We make preview button and preview section will be invisible
                PreviewScrollViewer.Visibility = Visibility.Collapsed;
            }
        }

        private void OnPrintButtonClick(object sender, RoutedEventArgs e)
        {
            var button = sender as Button;
            button.IsEnabled = false;
            button.Content = "Sending Job To Printer...";

            if (!string.IsNullOrEmpty(WatermarkOptions.Text))
            {
                bool isValid = UpdateWatermarkOptions();
                if (isValid)
                {
                    LocalStorageUtil.SaveWatermarkTextPropertiesToLocalStorage(WatermarkOptions.Text, WatermarkOptions.TextFontSize, WatermarkOptions.TextXOffset, WatermarkOptions.TextYOffset);
                }
                else
                {
                    LocalStorageUtil.ResetWatermarkTextAndImage();
                }
            }

            if (!string.IsNullOrEmpty(jobPasswordTextBox.Text))
            {
                LocalStorageUtil.SaveAndEncryptJobPassword(jobPasswordTextBox.Text, jobPasswordEncryptionComboBox.SelectedItem.ToString());
            }

            LocalStorageUtil.SaveImagePropertiesToLocalStorage(WatermarkOptions.LogoEnabled ? WatermarkOptions.LogoFilePath : null, WatermarkOptions.LogoDpiX, WatermarkOptions.LogoDpiY, WatermarkOptions.LogoWidth, WatermarkOptions.LogoHeight);

            JobActivatedMainPage.CloseDialog();
        }

        /// <summary>
        /// Called when the cancel button is clicked. Cancels the current print workflow and closes the window without printing the job.
        /// </summary>
        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            PrintWorkflowConfig.AbortPrintFlow(PrintWorkflowJobAbortReason.UserCanceled);
            JobActivatedMainPage.CloseDialog();
        }

        /// <summary>
        /// Updates the WatermarkOptions object with the current options chosen in the UI, and triggers a refresh of the preview page.
        /// Call this method as often as you'd like; it will return immediately and WatermarkPreview.StartPreviewRefresh will
        /// start a background job to perform the update. Each call will cancel previous unfinished background render updates.
        /// </summary>
        private void RenderPreview()
        {
            // Update the WatermarkOptions object based on the current UI state.
            // If invalid, an error message will show and we won't update the preview.
            if (UpdateWatermarkOptions())
            {
                // Kick off the preview refresh.
                // This starts rendering the preview on a background thread and returns immediately.
                // If StartPreviewRefresh is called again before the previous refresh finishes, the previous refresh will be cancelled.
                WatermarkPreview.StartPreviewRefresh();
                InputErrorTextBlock.Visibility = Visibility.Collapsed;
            }
            else
            {
                InputErrorTextBlock.Visibility = Visibility.Visible;
            }
        }

        /// <summary>
        /// Called by the WatermarkPreview on a background thread during each new preview render.
        /// </summary>
        /// <param name="previewPage">Changes made to previewPage will be shown in the render</param>
        private void ApplyModificationsToPreview(XpsPageWrapper previewPage)
        {
            lock (Watermarker)
            {
                Watermarker.SetWatermarkText(WatermarkOptions.Text, WatermarkOptions.TextFontSize, WatermarkOptions.TextXOffset, WatermarkOptions.TextYOffset);
                Watermarker.SetWatermarkImage(WatermarkOptions.LogoFilePath, WatermarkOptions.LogoDpiX, WatermarkOptions.LogoDpiY, WatermarkOptions.LogoWidth, WatermarkOptions.LogoHeight);
                Watermarker.SetWatermarkImageEnabled(WatermarkOptions.LogoEnabled);

                Watermarker.ApplyWatermarks(previewPage);
            }
        }

        /// <summary>
        /// When the "position watermark" button is clicked, we will wait for the next click on the watermark preview
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PositionWatermarkButton_Click(object sender, RoutedEventArgs e)
        {
            PositionWatermarkPrompt.Visibility = Visibility.Visible;
            PositionWatermarkButton.IsEnabled = false;
            IsPositioningWatermark = true;
        }

        /// <summary>
        /// Called when the user clicks on the preview image. Use the click point to position the text watermark.
        /// </summary>
        private void WatermarkPreview_Tapped(object sender, Windows.UI.Xaml.Input.TappedRoutedEventArgs e)
        {
            if (IsPositioningWatermark)
            {
                IsPositioningWatermark = false;
                PositionWatermarkPrompt.Visibility = Visibility.Collapsed;
                PositionWatermarkButton.IsEnabled = true;

                Image previewImage = sender as Image;
                Windows.Foundation.Point tapPoint = e.GetPosition(previewImage);

                // Calculate the new offset based on where the user tapped
                TextXOffset = tapPoint.X / previewImage.ActualWidth;
                TextYOffset = tapPoint.Y / previewImage.ActualHeight;

                RenderPreview();
            }
        }


        /// <summary>
        /// Called when the previous-page button in the PreviewPaginator has been pressed
        /// </summary>
        /// <param name="sender">The underlying button sending the event</param>
        /// <param name="e">The original event</param>
        private void PreviousPageCommand(object sender, RoutedEventArgs e)
        {
            if (CurrentPage < 2)
            {
                return;
            }
            CurrentPage--;
            PreviewPaginator.CurrentPage = CurrentPage;
            WatermarkPreview.PreviewPage = document.GetPage(PreviewPaginator.CurrentPage.GetValueOrDefault(1));

            RenderPreview();
        }

        /// <summary>
        /// Called when the next-page button in the PreviewPaginator has been pressed
        /// </summary>
        /// <param name="sender">The underlying button sending the event</param>
        /// <param name="e">The original event</param>
        private void NextPageCommand(object sender, RoutedEventArgs e)
        {
            if (CurrentPage > (TotalPages - 1))
            {
                return;
            }
            CurrentPage++;
            PreviewPaginator.CurrentPage = CurrentPage;
            WatermarkPreview.PreviewPage = document.GetPage(PreviewPaginator.CurrentPage.GetValueOrDefault(1));

            RenderPreview();
        }

        // ------------------
        // On a change to any part of the UI options controls, simply call RenderPreview.
        // RenderPreview will take care of updating WatermarkOptions and refreshing the page.
        private void WatermarkTextBox_TextChanged(object sender, TextChangedEventArgs e) => RenderPreview();
        private void TextSizeBox_TextChanged(object sender, TextChangedEventArgs e) => RenderPreview();
        private void ImageCheckBox_Click(object sender, RoutedEventArgs e) => RenderPreview();
        // ------------------


        /// <summary>
        /// Parse the current UI state and update WatermarkOptions.
        /// </summary>
        /// <returns>true if the options are well-formed and within proper ranges, else false</returns>
        private bool UpdateWatermarkOptions()
        {
            WatermarkOptions.Text = WatermarkTextBox.Text;

            try
            {
                WatermarkOptions.TextFontSize = int.Parse(TextSizeBox.Text);
                WatermarkOptions.TextXOffset = TextXOffset;
                WatermarkOptions.TextYOffset = TextYOffset;
            }
            catch (FormatException)
            {
                return false;
            }

            if (WatermarkOptions.TextXOffset > 1 || WatermarkOptions.TextXOffset < 0 || WatermarkOptions.TextYOffset < 0 || WatermarkOptions.TextYOffset > 1)
            {
                return false;
            }

            // Update the image state based on the checkbox status
            WatermarkOptions.LogoEnabled = ImageCheckBox.IsChecked.GetValueOrDefault();

            return true;
        }

        /// <summary>
        /// Get the metadata for the chosen logo image so that its size and resolution can be determined
        /// Typical usage would be
        ///     softwareBitmap = await GetSoftwareBitmapFromFileAsync(file);
        /// </summary>
        /// <param name="file">Image file</param>
        /// <returns>Task<SoftwareBitmap></returns>
        private static async Task<SoftwareBitmap> GetSoftwareBitmapFromFileAsync(StorageFile file)
        {
            SoftwareBitmap softwareBitmap = null;

            if (file != null)
            {
                using (var fileStream = await file.OpenAsync(FileAccessMode.Read))
                {
                    var decoder = await BitmapDecoder.CreateAsync(fileStream);

                    // Get the SoftwareBitmap representation of the file
                    softwareBitmap = await decoder.GetSoftwareBitmapAsync();
                }
            }
            return softwareBitmap;
        }


        // Accessors for resources required by Xaml
        public string ImageFilePath { get => imageFilePath; set => imageFilePath = value; }
        public string ImageCheckBoxLabel { get; } = "Add a logo at the top left";
        public string ClickPreviewToPositionWatermarkPrompt { get; } = "Click on the preview to position the watermark";
        public string InputErrorPrompt { get; } = "Please make sure TextSize is a Int, xPos & yPos are Doubles between [0,1]";
        public string XpsGenerationErrorPrompt { get; } = "An error occurred while reading the document.";
        public string TextSizeHeader { get; } = "Text Size";
        public string XPositionHeader { get; } = "X Position";
        public string YPositionHeader { get; } = "Y Position";

        private string imageFilePath = "Assets\\contoso_logo.jpg";
        private PrintWorkflowConfiguration PrintWorkflowConfig;

    }
}
