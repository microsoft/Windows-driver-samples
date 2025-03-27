// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    v4PrintDriver-Intellisense.js
//
// Abstract:
//
//    This file defines intellisense to be used by JavaScript extensions in v4 print drivers.

var v4PrintDriverIntellisense = {
    /// <summary>Intended for use by v4 print driver JavaScript Intellisense.</summary>
    createInterface: function (childInterface, baseType, prototype) {
        /// <summary>Intended for use by v4 print driver JavaScript Intellisense.</summary>
        childInterface.__class = true;

        if (prototype) {
            childInterface.prototype = prototype;
        }

        if (baseType) {
            childInterface.__baseType = baseType;
            childInterface.__basePrototypePending = true;
            v4PrintDriverIntellisense.resolveInheritance(childInterface);
        }
    },
    appendInterfaceMethods: function (baseType, prototype) {
        /// <summary>Intended for use by v4 print driver JavaScript Intellisense.</summary>
        for (var memberName in prototype) {
            baseType.prototype[memberName] = prototype[memberName];
        }
    },
    resolveInheritance: function (childInterface) {
        /// <summary>Intended for use by v4 print driver JavaScript Intellisense.</summary>
        var baseType = childInterface.__baseType;
        if (!baseType) {
            return;
        }

        if (baseType.__baseType) {
            resolveInheritance(baseType);
        }

        if (!childInterface.__basePrototypePending) {
            return;
        }

        for (var memberName in baseType.prototype) {
            var memberValue = baseType.prototype[memberName];
            if (!childInterface.prototype[memberName]) {
                childInterface.prototype[memberName] = memberValue;
            }
        }

        delete childInterface.__basePrototypePending;
    }
}

IPrintSchemaElement = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaElement,
    null,
    {
        /// <field name="XmlNode" type="XML DOM">
        /// Property-get maps to COM IPrintSchemaElement::XmlNode.
        /// </field>
        XmlNode: null,
        /// <field name="Name" type="String">
        /// Property-get maps to COM IPrintSchemaElement::Name.
        /// </field>
        Name: null,
        /// <field name="NamespaceUri" type="String">
        /// Property-get maps to COM IPrintSchemaElement::NamespaceUri.
        /// </field>
        NamespaceUri: null
    });

IPrintSchemaDisplayableElement = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaDisplayableElement,
    IPrintSchemaElement,
    {
        /// <field name="DisplayName" type="String">
        /// Property-get maps to COM IPrintSchemaDisplayableElement::DisplayName.
        /// </field>
        DisplayName: null
    });


IPrintSchemaOption = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaOption,
    IPrintSchemaDisplayableElement,
    {
        /// <field name="Selected" type="Boolean">
        /// Property-get maps to COM IPrintSchemaOption::Selected.
        /// </field>
        Selected: null,
        /// <field name="Constrained"  type="PrintSchemaConstrainedSetting">
        /// Property-get maps to COM IPrintSchemaOption::Constrained.
        /// </field>
        Constrained: null,
        GetPropertyValue: function (name, namespaceUri) {
            /// <summary>
            /// Method maps to COM IPrintSchemaOption::GetPropertyValue.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="namespaceUri" type="String" />
            /// <returns type="XML DOM" />
        },
        /// <field name="PagesPerSheet" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaNUpOption::PagesPerSheet. Valid for NUp option only.
        /// </field>
        PagesPerSheet: null,
        /// <field name="WidthInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageMediaSizeOption::WidthInMicrons. Valid for PageMediaSize option only.
        /// </field>
        WidthInMicrons: null,
        /// <field name="HeightInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageMediaSizeOption::HeightInMicrons. Valid for PageMediaSize option only.
        /// </field>
        HeightInMicrons: null

    });

IPrintSchemaOptionCollection = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaOptionCollection,
    null,
    {
        /// <field name="Count" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaOptionCollection::Count.
        /// </field>
        Count: null,
        GetAt: function (index) {
            /// <summary>
            /// Property-get maps to COM IPrintSchemaOptionCollection::GetAt.
            /// </summary>
            /// <param name="index" type="Number" integer="true" />
            /// <returns type="IPrintSchemaOption" />
        }
    });


IPrintSchemaFeature = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaFeature,
    IPrintSchemaDisplayableElement,
    {
        /// <field name="SelectedOption" type="IPrintSchemaOption">
        /// Property-set/get maps to COM IPrintSchemaFeature::SelectedOption.
        /// </field>
        SelectedOption: null,
        /// <field name="SelectionType" type="PrintSchemaSelectionType">
        /// Property-get maps to COM IPrintSchemaFeature::SelectionType.
        /// </field>
        SelectionType: null,
        GetOption: function (name, namespaceUri) {
            /// <summary>
            /// Method maps to COM IPrintSchemaFeature::GetOption.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="namespaceUri" type="String" />
            /// <returns type="IPrintSchemaOption" />
        },
        /// <field name="DisplayUI" type="Boolean">
        /// Property-get maps to COM IPrintSchemaFeature::DisplayUI.
        /// </field>
        DisplayUI: null
    });


IPrintSchemaPageImageableSize = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaPageImageableSize,
    IPrintSchemaElement,
    {
        /// <field name="ImageableSizeWidthInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageImageableSize::ImageableSizeWidthInMicrons.
        /// </field>
        ImageableSizeWidthInMicrons: null,
        /// <field name="ImageableSizeHeightInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageImageableSize::ImageableSizeHeightInMicrons.
        /// </field>
        ImageableSizeHeightInMicrons: null,
        /// <field name="OriginWidthInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageImageableSize::OriginWidthInMicrons.
        /// </field>
        OriginWidthInMicrons: null,
        /// <field name="OriginHeightInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageImageableSize::OriginHeightInMicrons.
        /// </field>
        OriginHeightInMicrons: null,
        /// <field name="ExtentWidthInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageImageableSize::ExtentWidthInMicrons.
        /// </field>
        ExtentWidthInMicrons: null,
        /// <field name="ExtentHeightInMicrons" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaPageImageableSize::ExtentHeightInMicrons.
        /// </field>
        ExtentHeightInMicrons: null
    });


IPrintSchemaCapabilities = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaCapabilities,
    IPrintSchemaElement,
    {
        GetFeatureByKeyName: function (keyName) {
            /// <summary>
            /// Method maps to COM IPrintSchemaCapabilities::GetFeatureByKeyName.
            /// </summary>
            /// <param name="keyName" type="String" />
            /// <returns type="IPrintSchemaFeature" />
        },
        GetFeature: function (name, namespaceUri) {
            /// <summary>
            /// Method maps to COM IPrintSchemaCapabilities::GetFeature.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="namespaceUri" type="String" />
            /// <returns type="IPrintSchemaFeature" />
        },
        /// <field name="PageImageableSize" type="IPrintSchemaPageImageableSize">
        /// Property-get maps to COM IPrintSchemaCapabilities::PageImageableSize.
        /// </field>
        PageImageableSize: null,
        /// <field name="JobCopiesAllDocumentsMinValue" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaCapabilities::JobCopiesAllDocumentsMinValue.
        /// </field>
        JobCopiesAllDocumentsMinValue: null,
        /// <field name="JobCopiesAllDocumentsMaxValue" type="Number" integer="true">
        /// Property-get maps to COM IPrintSchemaCapabilities::JobCopiesAllDocumentsMaxValue.
        /// </field>
        JobCopiesAllDocumentsMaxValue: null,
        GetSelectedOptionInPrintTicket: function (feature) {
            /// <summary>
            /// Method maps to COM IPrintSchemaCapabilities::GetSelectedOptionInPrintTicket.
            /// </summary>
            /// <param name="feature" type="IPrintSchemaFeature" />
            /// <returns type="IPrintSchemaOption" />
        },
        GetOptions: function (feature) {
            /// <summary>
            /// Method maps to COM IPrintSchemaCapabilities::GetOptions.
            /// </summary>
            /// <param name="feature" type="IPrintSchemaFeature" />
            /// <returns type="IPrintSchemaOptionCollection" />
        }
    });


IPrintSchemaTicket = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrintSchemaTicket,
    IPrintSchemaElement,
    {
        GetFeatureByKeyName: function (keyName) {
            /// <summary>
            /// Method maps to COM IPrintSchemaTicket::GetFeatureByKeyName.
            /// </summary>
            /// <param name="keyName" type="String" />
            /// <returns type="IPrintSchemaFeature" />
        },
        GetFeature: function (name, namespaceUri) {
            /// <summary>
            /// Method maps to COM IPrintSchemaTicket::GetFeature.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="namespaceUri" type="String" />
            /// <returns type="IPrintSchemaFeature" />
        },
        NotifyXmlChanged: function () {
            /// <summary>
            /// Method maps to COM IPrintSchemaTicket::NotifyXmlChanged.
            /// </summary>
        },
        GetCapabilities: function () {
            /// <summary>
            /// Method maps to COM IPrintSchemaTicket::GetCapabilities.
            /// </summary>
            /// <returns type="IPrintSchemaCapabilities" />
        },
        /// <field name="JobCopiesAllDocuments" type="Number" integer="true">
        /// Property-get/put maps to IPrintSchemaTicket::JobCopiesAllDocuments.
        /// </field>
        JobCopiesAllDocuments: null
    });


IPrinterScriptableSequentialStream = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptableSequentialStream,
    null,
    {
        Read: function (count) {
            /// <summary>
            /// Method maps to COM IPrinterScriptableSequentialStream::Read.
            /// </summary>
            /// <param name="count" type="Number" integer="true" />
            /// <returns type="Array" />
        },
        Write: function (array) {
            /// <summary>
            /// Method maps to COM IPrinterScriptableSequentialStream::Write.
            /// </summary>
            /// <param name="array" type="Array" />
            /// <returns type="Number" integer="true"/>
        }
    });

IPrinterScriptableStream = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptableStream,
    IPrinterScriptableSequentialStream,
    {
        Commit: function () {
            /// <summary>
            /// Method maps to COM IPrinterScriptableStream::Commit.
            /// </summary>
        },
        Seek: function (offset, streamSeek) {
            /// <summary>
            /// Method maps to COM IPrinterScriptableStream::Seek
            /// </summary>
            /// <param name="offset" type="Number" integer="true" />
            /// <param name="streamSeek" type="STREAM_SEEK" />
            /// <returns type="Number" integer="true"/>
        },
        SetSize: function (size) {
            /// <summary>
            /// Method maps to COM IPrinterScriptableStream::SetSize.
            /// </summary>
            /// <param name="size" type="Number" integer="true" />
        }
    });


IPrinterScriptablePropertyBag = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptablePropertyBag,
    null,
    {
        GetBool: function (name) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::GetBool.
            /// </summary>
            /// <param name="name" type="String" />
            /// <returns type="Boolean" />
        },
        SetBool: function (name, value) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::SetBool.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="value" type="Boolean" />
        },
        GetInt32: function (name) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::GetInt32.
            /// </summary>
            /// <param name="name" type="String" />
            /// <returns type="Number" integer="true"/>
        },
        SetInt32: function (name, value) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::SetInt32.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="value" type="Number" integer="true" />
        },
        GetString: function (name) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::GetString.
            /// </summary>
            /// <param name="name" type="String" />
            /// <returns type="String" />
        },
        SetString: function (name, value) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::SetString.
            /// </summary>
            /// <param name="name" type="String" />
            /// <param name="value" type="String" />
        },
        GetReadStream: function (name) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::GetReadStream.
            /// </summary>
            /// <param name="name" type="String" />
            /// <returns type="IPrinterScriptableStream" />
        },
        GetWriteStream: function (name) {
            /// <summary>
            /// Method maps to COM IPrinterScriptablePropertyBag::GetWriteStream.
            /// </summary>
            /// <param name="name" type="String" />
            /// <returns type="IPrinterScriptableStream" />
        }
    });


IPrinterScriptContext = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterScriptContext,
    null,
    {
        /// <field name="DriverProperties" type="IPrinterScriptablePropertyBag">
        /// Property-get maps to COM IPrinterScriptContext::DriverProperties.
        /// </field>
        DriverProperties: null,
        /// <field name="QueueProperties" type="IPrinterScriptablePropertyBag">
        /// Property-get maps to COM IPrinterScriptContext::QueueProperties.
        /// </field>
        QueueProperties: null,
        /// <field name="UserProperties" type="IPrinterScriptablePropertyBag">
        /// Property-get maps to COM IPrinterScriptContext::UserProperties.
        /// </field>
        UserProperties: null
    });

IPrinterBidiSchemaElement = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterBidiSchemaElement,
    null,
    {
        /// <field name="Name" type="String">
        /// Property-get maps to COM IPrinterBidiSchemaElement::Name.
        /// </field>
        Name: null,
        /// <field type="PrinterBidiSchemaElementType">
        /// Property-get maps to COM IPrinterBidiSchemaElement::BidiType.
        /// </field>
        BidiType: null,
        /// <field type="Object">
        /// Property-get maps to COM IPrinterBidiSchemaElement::Value.
        /// </field>
        Value: null
    });

IPrinterBidiSchemaResponses = function () { }
v4PrintDriverIntellisense.createInterface(
    IPrinterBidiSchemaResponses,
    null,
    {
        AddNull: function (schema) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddNull.
            /// </summary>
            /// <param name="schema" type="String" />
        },
        AddString: function (schema, value) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddString.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="value" type="String" />
        },
        AddText: function (schema, value) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddText.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="value" type="String" />
        },
        AddEnum: function (schema, value) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddEnum.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="value" type="String" />
        },
        AddInt32: function (schema, value) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddInt32.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="value" type="Number" integer="true" />
        },
        AddBool: function (schema, value) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddBool.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="value" type="Boolean" />
        },
        AddFloat: function (schema, value) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddFloat.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="value" type="Number" />
        },
        AddBlob: function (schema, array) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddBlob.
            /// </summary>
            /// <param name="schema" type="String" />
            /// <param name="array" type="Array" />
        },
        AddRequeryKey: function (queryKey) {
            /// <summary>
            /// Method maps to COM IPrinterBidiSchemaResponses::AddRequeryKey.
            /// </summary>
            /// <param name="queryKey" type="String" />
        }
    });
