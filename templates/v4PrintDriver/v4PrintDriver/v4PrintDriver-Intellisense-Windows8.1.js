/// <reference path="v4PrintDriver-Intellisense.js" />

v4PrintDriverIntellisense.appendInterfaceMethods(
    IPrintSchemaTicket,
    {
        GetParameterInitializer: function (name, namespaceUri) {
            /// <summary>
            /// Method maps to COM IPrintSchemaTicket2::GetParameterInitializer.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="namespaceUri" type="String" />
            /// <returns type="IPrintSchemaParameterInitializer" />
        }
    });

v4PrintDriverIntellisense.appendInterfaceMethods(
    IPrintSchemaCapabilities,
    {
        GetParameterDefinition: function (name, namespaceUri) {
            /// <summary>
            /// Method maps to COM IPrintSchemaCapabilities2::GetParameterDefinition.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="namespaceUri" type="String" />
            /// <returns type="IPrintSchemaParameterDefinition" />
        }
    });

IPrintSchemaParameterInitializer = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaParameterInitializer,
    IPrintSchemaElement,
    {
        /// <field name="Value" type="String/Number">
        /// Property-get/set maps to COM IPrintSchemaParameterInitializer::Value.
        /// </field>
        Value: null,
    });

IPrintSchemaParameterDefinition = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaParameterDefinition,
    IPrintSchemaDisplayableElement,
    {
        /// <field name="UserInputRequired" type="Boolean">
        /// Property-get maps to COM IPrintSchemaParameterDefinition::UserInputRequired.
        /// </field>
        UserInputRequired: null,
        /// <field name="UnitType" type="String">
        /// Property-get maps to COM IPrintSchemaParameterDefinition::UnitType.
        /// </field>
        UnitType: null,
        /// <field name="DataType" type="PrintSchemaParameterDataType">
        /// Property-get maps to COM IPrintSchemaParameterDefinition::DataType.
        /// </field>
        DataType: null,
        /// <field name="RangeMin" type="Number">
        /// Property-get maps to COM IPrintSchemaParameterDefinition::RangeMin.
        /// </field>
        RangeMin: null,
        /// <field name="RangeMax" type="Number">
        /// Property-get maps to COM IPrintSchemaParameterDefinition::RangeMax.
        /// </field>
        RangeMax: null
    });

IPrinterScriptUsbJobContextReturnCodes = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptUsbJobContextReturnCodes,
    null,
    {
        /// <field name="Success" type="Number" integer="true">
        /// Property-get maps to COM IPrinterScriptUsbJobContextReturnCodes::Success.
        /// </field>
        Success: null,
        /// <field name="Failure" type="Number" integer="true">
        /// Property-get maps to COM IPrinterScriptUsbJobContextReturnCodes::Failure.
        /// </field>
        Failure: null,
        /// <field name="Retry" type="Number" integer="true">
        /// Property-get maps to COM IPrinterScriptUsbJobContextReturnCodes::Retry.
        /// </field>
        Retry: null,
        /// <field name="DeviceBusy" type="Number" integer="true">
        /// Property-get maps to COM IPrinterScriptUsbJobContextReturnCodes::DeviceBusy.
        /// </field>
        DeviceBusy: null,
        /// <field name="AbortTheJob" type="Number" integer="true">
        /// Property-get maps to COM IPrinterScriptUsbJobContextReturnCodes::AbortTheJob.
        /// </field>
        AbortTheJob: null
    });

IPrinterScriptUsbWritePrintDataProgress = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptUsbWritePrintDataProgress,
    null,
    {
        /// <field name="ProcessedByteCount" type="Number">
        /// Property-get/set maps to COM IPrinterScriptUsbWritePrintDataProgress::ProcessedByteCount.
        /// </field>
        ProcessedByteCount: null
    });

IPrinterScriptUsbJobContext = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptUsbJobContext,
    null,
    {
        /// <field name="JobPropertyBag" type="IPrinterScriptablePropertyBag">
        /// Property-get maps to COM IPrinterScriptUsbJobContext::JobPropertyBag.
        /// </field>
        JobPropertyBag: null,
        /// <field name="ReturnCodes" type="IPrinterScriptUsbJobContextReturnCodes">
        /// Property-get maps to COM IPrinterScriptUsbJobContext::ReturnCodes.
        /// </field>
        ReturnCodes: null,
        /// <field name="TemporaryStreams" type="Array">
        /// Property-get maps to COM IPrinterScriptUsbJobContext::TemporaryStreams. Provides an array of IPrinterScriptableSequentialStream.
        /// </field>
        TemporaryStreams: null,
        /// <field name="PrintedPageCount" type="Number">
        /// Property-get/set maps to COM IPrinterScriptUsbJobContext::PrintedPageCount.
        /// </field>
        PrintedPageCount: null
    });