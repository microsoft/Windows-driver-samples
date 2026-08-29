using System;
using System.Collections.Generic;
using System.Linq;
using Windows.ApplicationModel.Background;
using Windows.Devices.Printers;
using Windows.Graphics.Printing.Workflow;
using Windows.Storage.Streams;
using XpsUtil;

namespace Tasks
{
    public sealed class PrintSupportWorkflowBackgroundTask : IBackgroundTask
    {
        // Can be completed after PrintWorkflowPdlTargetStream.CompleteStreamSubmission is complete.
        public BackgroundTaskDeferral TaskInstanceDeferral { get; set; }

        public void Run(IBackgroundTaskInstance taskInstance)
        {
            TaskInstanceDeferral = taskInstance.GetDeferral();

            if (taskInstance.TriggerDetails is PrintWorkflowJobTriggerDetails jobDetails)
            {
                var session = jobDetails.PrintWorkflowJobSession;
                session.JobStarting += OnJobStarting;
                session.PdlModificationRequested += OnPdlModificationRequested;

                // Make sure to register all the event handlers before PrintWorkflowJobBackgroundSession.Start is called.
                session.Start();
            }
        }

        private void OnJobStarting(PrintWorkflowJobBackgroundSession sender, PrintWorkflowJobStartingEventArgs args)
        {
            // Not fully implemented yet.
            args.SetSkipSystemRendering();
            args.GetDeferral().Complete();
        }

        private bool HasPdlConverter(string documentFormat)
        {
            switch (documentFormat)
            {
                case "image/pwg-raster":
                case "application/PCLm":
                case "application/pdf":
                    return true;
            }
            return false;
        }

        private string GetDocumentFormat(IppPrintDevice printer)
        {
            //
            // Example: Getting IPP attributes from the printer.
            //

            // Create a list of IPP attributes that we are requesting.
            var requestedAttributes = new List<string>
                {
                    "document-format-default",
                    "document-format-supported"
                };
            var attributes = printer.GetPrinterAttributes(requestedAttributes);

            // Lookup the IPP attribute from the map.
            string defaultFormat = attributes["document-format-default"].GetKeywordArray().First();

            // If the default format does not have a PDL converter, pick the first supported format.
            if (!HasPdlConverter(defaultFormat))
            {
                foreach (string documentFormat in attributes["document-format-supported"].GetKeywordArray())
                {
                    if (HasPdlConverter(documentFormat))
                    {
                        return documentFormat;
                    } 
                }
            }
            return defaultFormat;
        }

        private void OnPdlModificationRequested(PrintWorkflowJobBackgroundSession sender, PrintWorkflowPdlModificationRequestedEventArgs args)
        {
            var deferral = args.GetDeferral();
            args.UILauncher.LaunchAndCompleteUIAsync().AsTask().ContinueWith(launchUiTask =>
            {
                if (launchUiTask.Result == PrintWorkflowUICompletionStatus.UserCanceled)
                {
                    deferral.Complete();
                    TaskInstanceDeferral.Complete();
                    return;
                }

                string documentFormat = GetDocumentFormat(args.PrinterJob.Printer);

                // Add custom job attributes.
                var jobAttributes = new Dictionary<string, IppAttributeValue>();

                if (!string.IsNullOrEmpty(LocalStorageUtil.GetJobPasswordEncryptionMethod()))
                {
                    var operationAttributeCollection = new Dictionary<string, IppAttributeValue>
                    {
                        {"job-password",  IppAttributeValue.CreateOctetString(LocalStorageUtil.GetEncryptedJobPassword())},
                        {"job-password-encryption", IppAttributeValue.CreateKeyword(LocalStorageUtil.GetJobPasswordEncryptionMethod())}
                    };
                    jobAttributes.Add("msft-operation-attribute-col", IppAttributeValue.CreateCollection(operationAttributeCollection));
                    LocalStorageUtil.ClearJobPassword();
                }

                var targetStream = args.CreateJobOnPrinterWithAttributes(jobAttributes, documentFormat);

                if (string.Equals(args.SourceContent.ContentType, "application/OXPS", StringComparison.OrdinalIgnoreCase))
                {
                    //
                    // Example: Adding watermarks to a XPS document.
                    //

                    // Get the XPS document data stream from the source content.
                    var xpsContentStream = args.SourceContent.GetInputStream();
                    PrintWorkflowObjectModelSourceFileContent xpsContentObjectModel = new PrintWorkflowObjectModelSourceFileContent(xpsContentStream);

                    XpsPageWatermarker watermarker = ConfigureWatermarker();

                    // Adds the watermark to the XPS document.
                    var document = new XpsSequentialDocument(xpsContentObjectModel);

                    document.XpsGenerationFailed += (doc, e) => {
                        args.Configuration.AbortPrintFlow(PrintWorkflowJobAbortReason.JobFailed);
                        deferral.Complete();
                        TaskInstanceDeferral.Complete();
                    };

                    IInputStream watermarkedStream = document.GetWatermarkedStream(watermarker);

                    //
                    // Example: Custom Rendering of XPS document to printer supported PDLs.
                    //
                    PrintWorkflowPdlConverter pdlConverter = null;
                    switch (documentFormat)
                    {
                        case "image/pwg-raster":
                            pdlConverter = args.GetPdlConverter(PrintWorkflowPdlConversionType.XpsToPwgr);
                            break;
                        case "application/PCLm":
                            pdlConverter = args.GetPdlConverter(PrintWorkflowPdlConversionType.XpsToPclm);
                            break;
                        case "application/pdf":
                            pdlConverter = args.GetPdlConverter(PrintWorkflowPdlConversionType.XpsToPdf);
                            break;
                    }

                    // Convert the XPS document to the printer supported PDL and write it to the targetStream.
                    pdlConverter.ConvertPdlAsync(args.PrinterJob.GetJobPrintTicket(),
                                                 watermarkedStream,
                                                 targetStream.GetOutputStream()
                                                 ).AsTask().Wait();
                }
                else
                {
                    //Currently we only support XPS type watermark edit, everything else we will print out the raw data without any modification
                    RandomAccessStream.CopyAsync(args.SourceContent.GetInputStream(), targetStream.GetOutputStream()).AsTask().Wait();
                }

                // Mark the stream submission as Succeeded.
                targetStream.CompleteStreamSubmission(PrintWorkflowSubmittedStatus.Succeeded);
                
                deferral.Complete();
                TaskInstanceDeferral.Complete();
            });
        }

        private XpsPageWatermarker ConfigureWatermarker()
        {
            XpsPageWatermarker watermarker = new XpsPageWatermarker();

            LocalStorageUtil.GetWatermarkTextPropertiesFromLocalStorage(out string watermarkText, out int fontSize, out double xOffset, out double yOffset);
            watermarker.SetWatermarkText(watermarkText, fontSize, xOffset, yOffset);

            bool usingImage = LocalStorageUtil.GetImagePropertiesFromLocalStorage(out string imageFile, out double dpiX, out double dpiY, out int imageWidth, out int imageHeight);

            watermarker.SetWatermarkImageEnabled(usingImage && imageFile != null);
            if (usingImage && imageFile != null)
            {
                watermarker.SetWatermarkImage(imageFile, dpiX, dpiY, imageWidth, imageHeight);
            }

            return watermarker;
        }
    }
}
