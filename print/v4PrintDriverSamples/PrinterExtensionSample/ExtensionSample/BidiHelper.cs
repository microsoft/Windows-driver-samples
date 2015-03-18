// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
//
// Abstract:
//
//     This file contains helper methods that provide a data-binding friendly way to access and parse bidi response data.

using System;
using System.Collections.Generic;
using System.Windows.Media;
using System.Xml;

namespace Microsoft.Samples.Printing.PrinterExtension.Helpers
{
    /// <summary>
    /// Provide a data-binding friendly way to access and parse bidi response data.
    /// </summary>
    public class BidiHelper
    {
        /// <summary>
        /// Parse the bidi response.
        /// </summary>
        /// <param name="bidiResponse">Bidi response XML data.</param>
        public BidiHelper(string bidiResponse)
        {
            BidiResponseParser parser = new BidiResponseParser(bidiResponse);
            InkLevelC = parser.GetInkLevel(Colors.Cyan);
            InkLevelM = parser.GetInkLevel(Colors.Magenta);
            InkLevelY = parser.GetInkLevel(Colors.Yellow);
            InkLevelK = parser.GetInkLevel(Colors.Black);
        }

        /// <summary>
        /// Get the Cyan ink level.
        /// </summary>
        public double InkLevelC
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the Magenta ink level.
        /// </summary>
        public double InkLevelM
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the Yellow ink level.
        /// </summary>
        public double InkLevelY
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the Black ink level.
        /// </summary>
        public double InkLevelK
        {
            get;
            private set;
        }
    }

    /// <summary>
    /// This class parses bidi response xml data and provides wrapper methods that operate upon the xml.
    /// </summary>
    internal class BidiResponseParser
    {
        /// <summary>
        /// Parse the bidi response.
        /// </summary>
        /// <param name="bidiResponse">Bidi response XML data.</param>
        internal BidiResponseParser(string bidiResponse)
        {
            bidiData = new XmlDocument();
            bidiData.LoadXml(bidiResponse);

            namespaceManager = new XmlNamespaceManager(bidiData.NameTable);
            namespaceManager.AddNamespace("bidi", "http://schemas.microsoft.com/windows/2005/03/printing/bidi");
        }

        /// <summary>
        /// Get the ink level for a given color.
        /// </summary>
        /// <param name="color">Color</param>
        /// <returns>Ink level percentage</returns>
        internal double GetInkLevel(Color color)
        {
            XmlElement root = bidiData.DocumentElement;
            XmlNode inkNode = root.SelectSingleNode(CreateInkXPathQuery(color), namespaceManager);
            return double.Parse(inkNode.FirstChild.Value) / 100;
        }

        /// <summary>
        /// Create an XPath query that retrieves the ink level from a standard bidi response.
        /// </summary>
        /// <param name="color"></param>
        /// <returns></returns>
        private static string CreateInkXPathQuery(Color color)
        {
            string colorName = null;

            if (color.Equals(Colors.Black))
            {
                colorName = "Black";
            }
            else if (color.Equals(Colors.Red))
            {
                colorName = "Red";
            }
            else if (color.Equals(Colors.Green))
            {
                colorName = "Green";
            }
            else if (color.Equals(Colors.Blue))
            {
                colorName = "Blue";
            }
            else if (color.Equals(Colors.Cyan))
            {
                colorName = "Cyan";
            }
            else if (color.Equals(Colors.Magenta))
            {
                colorName = "Magenta";
            }
            else if (color.Equals(Colors.Yellow))
            {
                colorName = "Yellow";
            }
            else
            {
                throw new ArgumentException("Unsupported color");
            }

            return "/bidi:Get/Query/Schema[@name='\\Printer.Consumables." + colorName + "Ink" + ":Level']/BIDI_INT";
        }

        private XmlDocument bidiData;
        private XmlNamespaceManager namespaceManager;
    }

}
