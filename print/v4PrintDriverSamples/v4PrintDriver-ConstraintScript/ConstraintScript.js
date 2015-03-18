// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    ConstraintScript.js
//
// Abstract:
//
//    Sample Javascript constraints file for v4 printer drivers.

//
// Declaration of various enums/constants that may be useful when modifying this sample.
//

// Add a reference that provides intellisense
/// <reference path="v4PrintDriver-Intellisense.js" />

// --------------------------------------------------------------------------
// Note: To disable intellisense for Windows 8.1 APIs, please delete the line below
/// <reference path="v4PrintDriver-Intellisense-Windows8.1.js" />
// --------------------------------------------------------------------------

var psfPrefix = "psf";
var pskNs = "http://schemas.microsoft.com/windows/2003/08/printing/printschemakeywords";
var pskV11Ns = "http://schemas.microsoft.com/windows/2013/05/printing/printschemakeywordsv11";
var psfNs = "http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework";

var PrintSchemaConstrainedSetting = {
    PrintSchemaConstrainedSetting_None: 0,
    PrintSchemaConstrainedSetting_PrintTicket: 1,
    PrintSchemaConstrainedSetting_Admin: 2,
    PrintSchemaConstrainedSetting_Device: 3
};

var PrintSchemaParameterDataType = {
    PrintSchemaParameterDataType_Integer: 0,
    PrintSchemaParameterDataType_NumericString: 1,
    PrintSchemaParameterDataType_String: 2
};

var STREAM_SEEK = {
    STREAM_SEEK_SET: 0,
    STREAM_SEEK_CUR: 1,
    STREAM_SEEK_END: 2
};

var PrintSchemaSelectionType = {
    PrintSchemaSelectionType_PickOne: 0,
    PrintSchemaSelectionType_PickMany: 1
};

function validatePrintTicket(printTicket, scriptContext) {
    /// <summary>
    ///     Validates a print ticket.
    ///
    ///     This example expresses the constraint that if 'ISOA4' PageMediaSize is selected,
    ///     the 'PhotographicGlossy' PageMediaType has to be selected.
    ///     Should another 'PageMediaType' option be selected, this method sets the selected option's name
    ///     to 'PhotographicGlossy' and indicates that the print ticket was modified to make it valid.
    /// </summary>
    /// <param name="printTicket" type="IPrintSchemaTicket">
    ///     Print ticket to be validated.
    /// </param>
    /// <param name="scriptContext" type="IPrinterScriptContext">
    ///     Script context object.
    /// </param>
    /// <returns type="Number" integer="true">
    ///     Integer value indicating validation status.
    ///         1 - Print ticket is valid and was not modified.
    ///         2 - Print ticket was modified to make it valid.
    ///         0 - Print ticket is invalid  (not demonstrated by this example).
    /// </returns>

    var retVal = 1;

    // Set the selection namespace on the printTicket's XmlNode. This instance allows us to query for
    // nodes belonging to the 'pskNs' namespace.
    setSelectionNamespace(
        printTicket.XmlNode,
        psfPrefix,
        psfNs);

    // If the print ticket has an invalid combination of PageMediaSize and PageMediaType options, fix it,
    // and return '2' to indicate the print ticket has been modified.
    if (constraintSample.isMediaTypeConstrainedByMediaSize(printTicket)) {
        var printTicketMediaTypeFeature = printTicket.GetFeature("PageMediaType");

        var pskPrefix = getPrefixForNamespace(
                        printTicket.XmlNode,
                        pskNs);

        // Retrieve the only allowed 'PageMediaType' option, from the print capabilities.
        // Note: Retrieving the print capabilities is a very expensive operation, and should be performed only if necessary.
        var printCapabilities = printTicket.GetCapabilities();
        var printCapsMediaTypeFeature = printCapabilities.GetFeature("PageMediaType");
        var allowedPageMediaTypeOption = printCapsMediaTypeFeature.GetOption(constraintSample.allowedPageMediaType);

        // Replace the constrained print ticket option with the allowed one.
        printTicketMediaTypeFeature.SelectedOption = allowedPageMediaTypeOption;

        retVal = 2;
    }

    // Below demonstrates correct usage of IPrintSchemaTicket2 APIs so that the script does not terminate
    // when running on a Windows 8 version of PrintConfig.dll.
    if (printSchemaApiHelpers.supportsIPrintSchemaTicket2(printTicket)) {
        var param = printTicket.GetParameterInitializer("JobCopiesAllDocuments");
    }

    return retVal;
}


function completePrintCapabilities(printTicket, scriptContext, printCapabilities) {
    /// <summary>
    ///      This example demonstrates how drivers can alter the print capabilities' 'PageImageableSize' values
    ///      based on a 'PageBorderless' feature, or based on 'PageOrientation'.
    ///
    ///      What this example does:
    ///
    ///      1. Retrieve the 'PageOrientation' feature from the print ticket.
    ///      2. Retrieve the 'PageBorderless' feature from the print ticket.
    ///      3. If 'Landscape' is the selected option for the 'PageOrientation' feature,
    ///         set custom  'PageImageableSize' margins in the print capabilities document.
    ///      4. Else if 'PageBorderless' is the selected option for the 'PageBorderless' feature,
    ///         set custom 'PageImageableSize' margins in the print capabilities.
    /// </summary>
    /// <param name="printTicket" type="IPrintSchemaTicket" mayBeNull="true">
    ///     If not 'null', the print ticket's settings are used to customize the print capabilities.
    /// </param>
    /// <param name="scriptContext" type="IPrinterScriptContext">
    ///     Script context object.
    /// </param>
    /// <param name="printCapabilities" type="IPrintSchemaCapabilities">
    ///     Print capabilities object to be customized.
    /// </param>

    // This sample does not customize the default print capabilities (i.e. when no print ticket is passed in).
    if (!printTicket) {
        return;
    }

    // Below demonstrates correct usage of IPrintSchemaCapabilities2 APIs so that the script does not terminate
    // when running on a Windows 8 version of PrintConfig.dll.
    if (printSchemaApiHelpers.supportsIPrintSchemaCapabilities2(printCapabilities)) {
        var param = printCapabilities.GetParameterDefinition("JobCopiesAllDocuments");
    }

    setSelectionNamespace(
        printTicket.XmlNode,
        psfPrefix,
        psfNs);

    setSelectionNamespace(
        printCapabilities.XmlNode,
        psfPrefix,
        psfNs);

    var ticketPskPrefix = getPrefixForNamespace(printTicket.XmlNode, pskNs);

    // Check the if 'Borderless' is the selected option for the 'PageBorderless'
    // Feature in the print ticket.
    var isBorderlessPrinting = false;
    var borderlessFeatureXmlNode = printTicket.GetFeature("PageBorderless");
    if (borderlessFeatureXmlNode) {
        var borderlessOptionName = borderlessFeatureXmlNode.SelectedOption.Name;
        if (borderlessOptionName === "Borderless") {
            isBorderlessPrinting = true;
        }
    }

    // Similarly check if 'Landscape' is the selected option for the 'PageOrientation'
    // Feature in the print ticket.
    var isLandscapeOrientation = false;
    var orientationFeature = printTicket.GetFeature("PageOrientation");
    if (orientationFeature) {
        var orientationOptionName = orientationFeature.SelectedOption.Name;
        if (orientationOptionName === "Landscape") {
            isLandscapeOrientation = true;
        }
    }

    var imageableSizeProperty = null;
    var imageableAreaProperty = null;

    // Adjust the 'PageImageableSize' values depending on whether this is borderless
    // printing or landscape orientation.
    if (isLandscapeOrientation) {
        // Custom values for print capabilities properties 'OriginWidth' and 'OriginHeight'.
        var originWidth = 5001;
        var originHeight = 5001;

        // Set the 'PageImageableArea' margin property values in the print capabilities document.
        imageableSizeProperty = getProperty(
                                        printCapabilities.XmlNode,
                                        pskNs,
                                        "PageImageableSize");
        imageableAreaProperty = getProperty(
                                        imageableSizeProperty,
                                        pskNs,
                                        "ImageableArea");
        setSubPropertyValue(
            imageableAreaProperty,
            pskNs,
            "OriginWidth",
            originWidth);
        setSubPropertyValue(
            imageableAreaProperty,
            pskNs,
            "OriginHeight",
            originHeight);
    } else if (isBorderlessPrinting) {
        // Retrieve the 'PageMediaSize' feature from the print ticket. Retrieve the ScoredProperties
        // 'MediaSizeWidth', 'MediaSizeHeight' from that feature, and use these values to set the
        // 'PageImageableSize' margins in the print capabilities document.
        var pageMediaSizeFeature = printTicket.GetFeature("PageMediaSize");
        if (!pageMediaSizeFeature) {
            return;
        }

        var pageMediaSizeSelectedOption = pageMediaSizeFeature.SelectedOption;
        if (!pageMediaSizeSelectedOption) {
            return;
        }

        var mediaWidthValueNode = pageMediaSizeSelectedOption.GetPropertyValue("MediaSizeWidth");
        var mediaHeightValueNode = pageMediaSizeSelectedOption.GetPropertyValue("MediaSizeHeight");
        var mediaSizeWidth = mediaWidthValueNode.firstChild.nodeValue;
        var mediaSizeHeight = mediaHeightValueNode.firstChild.nodeValue;

        // Set the values for the 'PageImageableSize' property in the print capabilities document.
        imageableSizeProperty = getProperty(
                                        printCapabilities.XmlNode,
                                        pskNs,
                                        "PageImageableSize");
        imageableAreaProperty = getProperty(
                                        imageableSizeProperty,
                                        pskNs,
                                        "ImageableArea");
        setSubPropertyValue(
            imageableAreaProperty,
            pskNs,
            "OriginWidth",
            0);
        setSubPropertyValue(
            imageableAreaProperty,
            pskNs,
            "OriginHeight",
            0);
        setSubPropertyValue(
            imageableAreaProperty,
            pskNs,
            "ExtentHeight",
            parseInt(
                mediaSizeHeight));
        setSubPropertyValue(
            imageableAreaProperty,
            pskNs,
            "ExtentWidth",
            parseInt(
                mediaSizeWidth));
    }

    // If the input print ticket has disallowed PageMediaSize and PageMediaType options
    // (as expressed in 'validatePrintTicket' function above), mark the constrained options as 'constrained by
    // print ticket settings' i.e. 'psk:PrintTicketSettings'.
    if (constraintSample.isMediaTypeConstrainedByMediaSize(printTicket)) {
        var mediaTypeFeature = printTicket.GetFeature("PageMediaType");
        var mediaTypeOptions = printCapabilities.GetOptions(mediaTypeFeature);

        for (i = 0; i < mediaTypeOptions.Count; i++) {
            var mediaTypeOption = mediaTypeOptions.GetAt(i);

            // The only option that is not constrained, as expressed in 'validatePrintTicket' function above.
            if ((mediaTypeOption.Name === constraintSample.allowedPageMediaType) &&
                (mediaTypeOption.NamespaceUri === pskNs)) {
                continue;
            }

            // If an option is already marked constrained, there is no need to mark it once again.
            if (!mediaTypeOption.Constrained) {
                var pskPrefix = getPrefixForNamespace(
                        printTicket.XmlNode,
                        pskNs);

                mediaTypeOption.XmlNode.setAttribute("constrained", pskPrefix + ":PrintTicketSettings");
            }
        }
    }
}

// Demonstrates a simple example of how to express print ticket constraints via the
// 'validatePrintTicket' and 'completePrintCapabilities' extension functions.
var constraintSample = {
    // The PageMediaSize option that constrains/limits the allowed PageMediaType options.
    constrainingMediaSize : "ISOA4",

    // The only PageMediaType option that not constrained by the constraining PageMediaSize option.
    allowedPageMediaType : "PhotographicGlossy",

    isMediaTypeConstrainedByMediaSize : function(printTicket) {
        /// <summary>
        ///     Determines if a print ticket is constrained (i.e. if the 'constrainingMediaSize' PageMediaSize option  is
        ///     present, and constrains the PageMediaType option present in the print ticket).
        /// </summary>
        /// <param name="printTicket" type="IPrintSchemaTicket">
        ///     Print ticket to be checked for constrained options.
        /// </param>
        /// <returns type="Boolean">
        ///         true - PageMediaType option and PageMediaSize option are incompatible.
        ///         false - PageMediaType option and PageMediaSize option are compatible.
        /// </returns>

        // Retrieve the "PageMediaSize", "PageMediaType" features and their selected option names
        // from the print ticket.
        var mediaSizeFeature = printTicket.GetFeature("PageMediaSize");
        var mediaTypeFeature = printTicket.GetFeature("PageMediaType");

        if (mediaSizeFeature && mediaTypeFeature) {
            // Verify if the PageMediaSize selected option is 'psk:ISOA4'.
            var mediaSizeOptionNamespaceUri = mediaSizeFeature.SelectedOption.NamespaceUri;
            var mediaSizeOptionName = mediaSizeFeature.SelectedOption.Name;

            if ((mediaSizeOptionNamespaceUri === pskNs) &&
                (mediaSizeOptionName === constraintSample.constrainingMediaSize)) {

                var mediaTypeOptionNamespaceUri = mediaTypeFeature.SelectedOption.NamespaceUri;
                var mediaTypeOptionName = mediaTypeFeature.SelectedOption.Name;

                // If the print ticket contains anything other than the allowed PageMediaType option,
                // return 'true' to indicate so.
                if ((mediaTypeOptionNamespaceUri !== pskNs) ||
                    (mediaTypeOptionName !== constraintSample.allowedPageMediaType)) {
                    return true;
                }
            }
        }
        
        return false;
    }
}


////*************************************************************
////                                                            *
////             Utility functions                              *
////                                                            *
////*************************************************************

function setPropertyValue(propertyNode, value) {
    /// <summary>
    ///     Set the value contained in the 'Value' node under a 'Property'
    ///     or a 'ScoredProperty' node in the print ticket/print capabilities document.
    /// </summary>
    /// <param name="propertyNode" type="IXMLDOMNode">
    ///     The 'Property'/'ScoredProperty' node.
    /// </param>
    /// <param name="value" type="variant">
    ///     The value to be stored under the 'Value' node.
    /// </param>
    /// <returns type="IXMLDOMNode" mayBeNull="true" locid="R:propertyValue">
    ///     First child 'Property' node if found, Null otherwise.
    /// </returns>
    var valueNode = getPropertyFirstValueNode(propertyNode);
    if (valueNode) {
        var child = valueNode.firstChild;
        if (child) {
            child.nodeValue = value;
            return child;
        }
    }
    return null;
}


function setSubPropertyValue(parentProperty, keywordNamespace, subPropertyName, value) {
    /// <summary>
    ///     Set the value contained in an inner Property node's 'Value' node (i.e. 'Value' node in a Property node
    ///     contained inside another Property node).
    /// </summary>
    /// <param name="parentProperty" type="IXMLDOMNode">
    ///     The parent property node.
    /// </param>
    /// <param name="keywordNamespace" type="String">
    ///     The namespace in which the property name is defined.
    /// </param>
    /// <param name="subPropertyName" type="String">
    ///     The name of the sub-property node.
    /// </param>
    /// <param name="value" type="variant">
    ///     The value to be set in the sub-property node's 'Value' node.
    /// </param>
    /// <returns type="IXMLDOMNode" mayBeNull="true">
    ///     Refer setPropertyValue.
    /// </returns>
    if (!parentProperty ||
        !keywordNamespace ||
        !subPropertyName) {
            return null;
        }
    var subPropertyNode = getProperty(
                            parentProperty,
                            keywordNamespace,
                            subPropertyName);
    return setPropertyValue(
            subPropertyNode,
            value);
}

function getScoredProperty(node, keywordNamespace, scoredPropertyName) {
    /// <summary>
    ///     Retrieve a 'ScoredProperty' element in a print ticket/print capabilities document.
    /// </summary>
    /// <param name="node" type="IXMLDOMNode">
    ///     The scope of the search i.e. the parent node.
    /// </param>
    /// <param name="keywordNamespace" type="String">
    ///     The namespace in which the element's 'name' attribute is defined.
    /// </param>
    /// <param name="scoredPropertyName" type="String">
    ///     The ScoredProperty's 'name' attribute (without the namespace prefix).
    /// </param>
    /// <returns type="IXMLDOMNode" mayBeNull="true">
    ///     The node on success, 'null' on failure.
    /// </returns>

    // Note: It is possible to hard-code the 'psfPrefix' variable in the tag name since the
    // SelectionNamespace property has been set against 'psfPrefix'
    // in validatePrintTicket/completePrintCapabilities.
    return searchByAttributeName(
                node,
                psfPrefix + ":ScoredProperty",
                keywordNamespace,
                scoredPropertyName);
}

function getProperty(node, keywordNamespace, propertyName) {
    /// <summary>
    ///     Retrieve a 'Property' element in a print ticket/print capabilities document.
    /// </summary>
    /// <param name="node" type="IXMLDOMNode">
    ///     The scope of the search i.e. the parent node.
    /// </param>
    /// <param name="keywordNamespace" type="String">
    ///     The namespace in which the element's 'name' attribute is defined.
    /// </param>
    /// <param name="propertyName" type="String">
    ///     The Property's 'name' attribute (without the namespace prefix).
    /// </param>
    /// <returns type="IXMLDOMNode" mayBeNull="true">
    ///     The node on success, 'null' on failure.
    /// </returns>
    return searchByAttributeName(
            node,
            psfPrefix + ":Property",
            keywordNamespace,
            propertyName);
}

function setSelectedOptionName(printSchemaFeature, keywordPrefix, optionName) {
    /// <summary>
    ///      Set the 'name' attribute of a Feature's selected option
    ///      Note: This function should be invoked with Feature type that is retrieved
    ///            via either PrintCapabilties->GetFeature() or PrintTicket->GetFeature().
    ///
    ///      Caution: Setting only the 'name' attribute can result in an invalid option element.
    ///            Some options require their entire subtree to be updated.
    /// </summary>
    /// <param name="printSchemaFeature" type="IPrintSchemaFeature">
    ///     Feature variable.
    /// </param>
    /// <param name="keywordPrefix" type="String">
    ///     The prefix for the optionName parameter.
    /// </param>
    /// <param name="optionName" type="String">
    ///     The name (without prefix) to set as the 'name' attribute.
    /// </param>
    if (!printSchemaFeature ||
        !printSchemaFeature.SelectedOption ||
        !printSchemaFeature.SelectedOption.XmlNode) {
            return;
        }
    printSchemaFeature.SelectedOption.XmlNode.setAttribute(
        "name",
        keywordPrefix + ":" + optionName);
}


////*************************************************************
////                                                            *
////             Functions used by utility functions            *
////                                                            *
////*************************************************************

function getPropertyFirstValueNode(propertyNode) {
    /// <summary>
    ///     Retrieve the first 'value' node found under a 'Property' or 'ScoredProperty' node.
    /// </summary>
    /// <param name="propertyNode" type="IXMLDOMNode">
    ///     The 'Property'/'ScoredProperty' node.
    /// </param>
    /// <returns type="IXMLDOMNode" mayBeNull="true">
    ///     The 'Value' node on success, 'null' on failure.
    /// </returns>
    if (!propertyNode) {
        return null;
    }

    var nodeName = propertyNode.nodeName;
    if ((nodeName.indexOf(":Property") < 0) &&
        (nodeName.indexOf(":ScoredProperty") < 0)) {
            return null;
        }

    var valueNode = propertyNode.selectSingleNode(psfPrefix + ":Value");
    return valueNode;
}

function searchByAttributeName(node, tagName, keywordNamespace, nameAttribute) {
    /// <summary>
    ///      Search for a node that with a specific tag name and containing a
    ///      specific 'name' attribute
    ///      e.g. &lt;Bar name=\"ns:Foo\"&gt; is a valid result for the following search:
    ///           Retrieve elements with tagName='Bar' whose nameAttribute='Foo' in
    ///           the namespace corresponding to prefix 'ns'.
    /// </summary>
    /// <param name="node" type="IXMLDOMNode">
    ///     Scope of the search i.e. the parent node.
    /// </param>
    /// <param name="tagName" type="String">
    ///     Restrict the searches to elements with this tag name.
    /// </param>
    /// <param name="keywordNamespace" type="String">
    ///     The namespace in which the element's name is defined.
    /// </param>
    /// <param name="nameAttribute" type="String">
    ///     The 'name' attribute to search for.
    /// </param>
    /// <returns type="IXMLDOMNode" mayBeNull="true">
    ///     IXMLDOMNode on success, 'null' on failure.
    /// </returns>
    if (!node ||
        !tagName ||
        !keywordNamespace ||
        !nameAttribute) {
            return null;
        }

    // Please refer to:
    // http://blogs.msdn.com/b/benkuhn/archive/2006/05/04/printticket-names-and-xpath.aspx
    // for more information on this XPath query.
    var xPathQuery = "descendant::"
                    + tagName
                    + "[substring-after(@name,':')='"
                    + nameAttribute
                    + "']"
                    + "[name(namespace::*[.='"
                     + keywordNamespace
                     + "'])=substring-before(@name,':')]"
                     ;

    return node.selectSingleNode(xPathQuery);
}

function setSelectionNamespace(xmlNode, prefix, namespace) {
    /// <summary>
    ///     This function sets the 'SelectionNamespaces' property on the XML Node.
    ///     For more details: http://msdn.microsoft.com/en-us/library/ms756048(VS.85).aspx
    /// </summary>
    /// <param name="xmlNode" type="IXMLDOMNode">
    ///     The node on which the property is set.
    /// </param>
    /// <param name="prefix" type="String">
    ///     The prefix to be associated with the namespace.
    /// </param>
    /// <param name="namespace" type="String">
    ///     The namespace to be added to SelectionNamespaces.
    /// </param>
    xmlNode.setProperty(
        "SelectionNamespaces",
        "xmlns:"
            + prefix
            + "='"
            + namespace
            + "'"
        );
}

function getPrefixForNamespace(node, namespace) {
    /// <summary>
    ///     This function returns the prefix for a given namespace.
    ///     Example: In 'psf:printTicket', 'psf' is the prefix for the namespace.
    ///     xmlns:psf="http://schemas.microsoft.com/windows/2003/08/printing/printschemaframework"
    /// </summary>
    /// <param name="node" type="IXMLDOMNode">
    ///     A node in the XML document.
    /// </param>
    /// <param name="namespace" type="String">
    ///     The namespace for which prefix is returned.
    /// </param>
    /// <returns type="String">
    ///     Returns the namespace corresponding to the prefix.
    /// </returns>

    if (!node) {
        return null;
    }

    // Navigate to the root element of the document.
    var rootNode = node.documentElement;

    // Query to retrieve the list of attribute nodes for the current node
    // that matches the namespace in the 'namespace' variable.
    var xPathQuery = "namespace::node()[.='"
                + namespace
                + "']";
    var namespaceNode = rootNode.selectSingleNode(xPathQuery);
    var prefix = namespaceNode.baseName;

    return prefix;
}

var printSchemaApiHelpers = {
    supportsIPrintSchemaCapabilities2: function (printCapabilities) {
        /// <summary>
        ///     Determines if the IPrintSchemaCapabilities2 APIs are supported on the 'printCapabilities' object.
        /// </summary>
        /// <param name="printCapabilities" type="IPrintSchemaCapabilities">
        ///     Print capabilities object.
        /// </param>
        /// <returns type="Boolean">
        ///     true - the interface APIs are supported.
        ///     false - the interface APIs are not supported.
        /// </returns>

        var supported = true;

        try {
            if (typeof printCapabilities.getParameterDefinition === "undefined") {
                supported = false;
            }
        }
        catch (exception) {
            supported = false;
        }

        return supported;
    },
    supportsIPrintSchemaTicket2: function(printTicket) {
        /// <summary>
        ///     Determines if the IPrintSchemaTicket2 APIs are supported on the 'printTicket' object.
        /// </summary>
        /// <param name="printTicket" type="IPrintSchemaTicket">
        ///     Print ticket object.
        /// </param>
        /// <returns type="Boolean">
        ///     true - the interface APIs are supported.
        ///     false - the interface APIs are not supported.
        /// </returns>

        var supported = true;

        try {
            if (typeof printTicket.getParameterInitializer === "undefined") {
                supported = false;
            }
        }
        catch (exception) {
            supported = false;
        }

        return supported;
    }
}