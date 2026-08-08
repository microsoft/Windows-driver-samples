using Windows.ApplicationModel.Background;
using Windows.Data.Xml.Dom;
using Windows.Graphics.Printing.PrintSupport;

namespace Tasks
{
    public sealed class PrintSupportExtensionBackGroundTask : IBackgroundTask
    {
        // Can be completed in OnTaskInstanceCanceled.
        public BackgroundTaskDeferral TaskInstanceDeferral { get; set; }

        public void Run(IBackgroundTaskInstance taskInstance)
        {
            TaskInstanceDeferral = taskInstance.GetDeferral();
            taskInstance.Canceled += OnTaskInstanceCanceled;

            if (taskInstance.TriggerDetails is PrintSupportExtensionTriggerDetails extensionDetails)
            {
                var session = extensionDetails.Session;
                session.PrintTicketValidationRequested += OnSessionPrintTicketValidationRequested;
                session.PrintDeviceCapabilitiesChanged += OnSessionPrintDeviceCapabilitiesChanged;

                // Make sure to register all the event handlers before PrintSupportExtensionSession.Start is called.
                session.Start();
            }
        }

        private void OnTaskInstanceCanceled(IBackgroundTaskInstance sender, BackgroundTaskCancellationReason reason)
        {
            TaskInstanceDeferral.Complete();
        }

        private void OnSessionPrintTicketValidationRequested(PrintSupportExtensionSession sender, PrintSupportPrintTicketValidationRequestedEventArgs args)
        {
            // Not fully implemented yet.
            args.SetPrintTicketValidationStatus(WorkflowPrintTicketValidationStatus.Resolved);
            args.GetDeferral().Complete();
        }

        private void OnSessionPrintDeviceCapabilitiesChanged(PrintSupportExtensionSession sender, PrintSupportPrintDeviceCapabilitiesChangedEventArgs args)
        {
            var pdc = args.GetCurrentPrintDeviceCapabilities();

            // IMPORTANT:
            // If you are adding a custom option, please make sure to add a custom namespace schema as well.
            // If you try to add a custom option that is not defined in the current 'printschemakeywords', then the print stack will silently fail your print job...

            // Add the custom namesapce uri to the XML document.
            pdc.DocumentElement.SetAttribute("xmlns:contoso", "http://schemas.contoso.com/keywords");
            // Add the custom media type.
            AddCustomMediaType(ref pdc, "http://schemas.contoso.com/keywords", "contoso:ContosoMediaType");

            args.UpdatePrintDeviceCapabilities(pdc);
            args.GetDeferral().Complete();
        }

        private void AddCustomMediaType(ref XmlDocument pdc, string namespaceUri, string mediaType)
        {
            // See linked below for the XML.Dom.Xmldocument class documentation.
            // https://docs.microsoft.com/en-us/uwp/api/windows.data.xml.dom.xmldocument
            //
            // NOTE: This example does not show error handling for simplicty. You should ensure that the properties being modified exists in the PDC, before trying to set its attributes.

            // Select the 'media-type-supported' XML section.
            var defaultPageMediaTypeNode = pdc.SelectSingleNodeNS(
                "//psk:PageMediaType//*[@psf2:default='true']",
                "xmlns:psk=\"http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords\" xmlns:psf2=\"http://schemas.microsoft.com/windows/2013/12/printing/printschemaframework2\"");

            // Get the owner document so that we can add new elements to the currently selected XML section.
            var document = defaultPageMediaTypeNode.OwnerDocument;
            // Create the new XML element for our custom media type.
            var newNode = document.CreateElementNS(namespaceUri, mediaType);
            newNode.SetAttributeNS("http://schemas.microsoft.com/windows/2013/12/printing/printschemaframework2", "psf2:psftype", "Option");
            newNode.SetAttributeNS("http://schemas.microsoft.com/windows/2013/12/printing/printschemaframework2", "psf2:default", "false");

            var parent = defaultPageMediaTypeNode.ParentNode;
            parent.AppendChild(newNode);
        }
    }
}
