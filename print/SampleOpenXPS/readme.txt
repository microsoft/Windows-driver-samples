File Name:	SampleOXPS.zip
Version:	1.0
License:	These sample documents are covered by the terms of the OpenXPSTestFiles_EndUserLicenseAgreement.doc file included in this ZIP archive.
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

The sample documents in this ZIP archive were generated from a variety of sources, including those generated from
the Windows Presentation Foundation in WinFX, from Office 2007, from the Microsoft XPS Document Writer (MXDW), and a few
that were either hand-built from scratch or hand-modified from another source. They have been included to provide
you with a few documents that exercise a variety of features of the XML Paper Specification.

In addition, we have included a few high-quality documents in the Showcase directory to highlight some of the XPS advantages in terms of screen-to-print fidelity. We've also created some documents that are intended to fail, violating at least one conformance rule. These are in the ConformanceViolations directory.

Comments on various files is denoted following the file beginning with '--'.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

These sample Test Files include:


QualityLogicMinBar/ -- Minimum bar test documents provided by Quality Logic
	mb01.oxps
	mb02.oxps
	mb03.oxps
	mb04.oxps
	mb05.oxps
	mb06.oxps
	mb07.oxps
	mb08.oxps
	mb09.oxps
	

ConformanceViolations/  -- All these documents violate one conformance rule. They should fail or generate an error.
	MarkupCompatibility/
		M1.1a.oxps  -- Unrecognized element from a non-ignorable namespace
		M1.2a.oxps  -- Unrecognized element from Markup Compatibility namespace
		M2.14a.oxps -- ProcessContent referencing an element whose namespace is not also listed as Ignorable
		M2.17a.oxps -- Xml:lang attribute on a ProcessContent element
		M2.20a.oxps -- PreserveElements attribute with no matching Ignorable attribute
		M2.24a.oxps -- PreserveAttributes attribute with no matching Ignorable attribute
		M2.27a.oxps -- Non-understood and ignorable namespace also identified in the MustUnderstand attribute value
		M2.27b.oxps -- Non-understood namespace identified in the MustUnderstand attribute value
		M2.30a.oxps -- AlternateContent element with no Choice child elements
		M2.31a.oxps -- AlternateContent element with multiple Fallback child elements
		M2.32a.oxps -- AlternateContent element with Fallback child element occurring before Choice child element
		M2.33a.oxps -- Nested AlternateContent elements
		M2.35a.oxps -- AlternateContent element with unprefixed Ignorable attribute. Namespace identified in Ignorable attribute is used.
		M2.35b.oxps -- AlternateContent element with unprefixed Ignorable attribute. Namespace identified in Ignorable attribute is not used.
		M2.36a.oxps -- AlternateContent having unrecognized child element that is not ignored
		M2.41a.oxps -- Choice element having unrecognized and non-ignored attribute
		M2.42a.oxps -- Choice element having MustUnderstand attribute referencing bad namespace
		M2.43a.oxps -- Xml:lang attribute on AlternateContent element
		M2.44a.oxps -- Choice element missing Requires attribute
		M2.48a.oxps -- Choice element having unprefixed attribute other than Requires
		M2.49a.oxps -- Xml:lang attribute on Choice element
		M2.50a.oxps -- Requires attribute referencing non-understood namespace
		M2.52a.oxps -- Prefixed Requires attribute applied to Choice element
		M2.55a.oxps -- Unprefixed attribute applied to a Fallback element
		M2.56a.oxps -- Xml:lang attribute on Fallback element

	OpenPackagingConvetions/
		M1.1a.oxps -- InvalidZip -  Simple Text with empty reference to fdseq and empty fdseq name in zip
		M1.2a.oxps -- Simple Text without FPAGE Content Type
		M1.2b.oxps -- Simple Text without JPEG Content Type
		M1.3a.oxps -- Simple Text with empty segment in PARTNAME in fdseq
		M1.5a.oxps -- Simple Text PARTNAME with forward slash as last char
		M1.6a.oxps -- Simple Text PARTNAME with nonpchar in fdseq with orig doc name
		M1.6b.oxps -- Simple Text PARTNAME with nonpchar in fdseq
		M1.7a.oxps -- Simple Text core prop contains fwd slash pct encode and reference also contains pct encode
		M1.7b.oxps -- Simple Text PARTNAME contains '%/'
		M1.8a.oxps -- Simple Text PARTNAME contains '%c'
		M1.10a.oxps -- Simple Text PARTNAME only dot
		M1.11a.oxps -- Simple Text Fixed Doc Seq Part contains content type part name
		M1.17a.oxps -- Invalid comma token in content type
		M1.18a.oxps -- Content Types Leading Whitespace
		M1.18b.oxps -- Content Types LWS between type and subtype
		M1.20a.oxps -- Simple Text Doc with Content Type including param
		M1.22a.oxps -- Simple Text Content Types with ISO encoding
		M1.26a.oxps -- Relationship Missing ID
		M1.26b.oxps -- Relationship with invalid XML Identifier
		M1.26c.oxps -- Relationships with duplicate IDs
		M1.27a.oxps -- Relationship with missing Type
		M1.28a.oxps -- Relationship with missing Target
		M2.5a.oxps -- Content Type with duplicate default elements
		M2.5b.oxps -- Content Type with duplicate override elements
		M2.6a.oxps -- Content Type with Content Type but no Extension attrib
		M2.6b.oxps -- Content Type with Extension but no Content Type attrib
		M2.7a.oxps -- Content Type with Override with no ContentType attrib
		M2.7b.oxps -- Content Type with Override with no PartName attrib
		M2.18a.oxps -- Simple Text with Non-Interleaved as two consecutive parts
		M2.24a.oxps -- Simple Text with fdseq converted to single dot char
		M3.9a.oxps -- Simple Text with dig sig with Object containing PCDATA
		M3.10a.oxps -- Simple Text with dig sig with fragment ident after Content Type
		M3.14a.oxps -- Simple Text with dig sig Relationships XForm without Canon XForm
		
	OXPS/
		M1.2a.oxps -- Package is not a zip archive, it’s plain text instead
		M2.3a.oxps -- Missing FixedDocumentSequence part
		M2.3b.oxps -- Missing fixedrepresentation relationship
		M2.3c.oxps -- Two fixedrepresentation relationships
		M2.4a.oxps -- Missing FixedDocument part
		M2.4b.oxps -- No <DocumentReference> to a FixedDocument
		M2.5a.oxps -- Missing FixedPage part
		M2.5b.oxps -- No <PageContent> to FixedPage
		M2.6a.oxps -- Missing Font Part
		M2.6b.oxps -- Missing FontURI attribute
		M2.10a.oxps -- Missing required resource relationship for Font in FixedPage
		M2.13a.oxps -- Missing fixedrepresentation relationship
		M2.13b.oxps -- Two fixedrepresentation relationships
		M2.14a.oxps -- Fixedrepresentation relationship pointing to FixedDocucment
		M2.18a.oxps -- Corrupt PNG image
		M2.25a.oxps -- Corrupt TIF image
		M2.36a.oxps -- FixedPage with 2 thumbnails
		M2.37a.oxps -- FixedPage with TIF thumbnail
		M2.59a.oxps -- Two PrintTickets related to FixedDocumentSequence
		M2.71a.oxps -- DTD in Relationship markup
		M2.71b.oxps -- DTD in Content_Types markup
		M2.71c.oxps -- DTD in FixedPage markup
		M2.73a.oxps -- Xml:id attribute in FixedPage markup
		M2.73b.oxps -- Xsi:SchemaLocation attribute on <FixedPage> element
		M2.74a.oxps -- <Path> having Fill attrib and <Path.Fill> child element
		M2.75a.oxps -- Xml:space attribute on <FixedPage> element
		M2.75b.oxps -- Xml:space attribute on <Path> element
		M2.76a.oxps -- Invalid value for xml:lang attribute on <FixedPage> element
		M2.80.oxps --  <DocumentReference> uses absolute URIs
		M2.81.oxps -- <PageContent> uses absolute URIs
		M2.82.oxps -- FixedPage part uses absolute URIs
		M3.2a.oxps -- <DocucmentReference> Source attribute pointing to Fixed Page instead of FixedDocucment
		M3.3a.oxps -- Multiple <DocumentReference> elements pointing to same FixedDocument
		M3.5a.oxps -- <PageContent> Source attrib pointing to FixedDocucment instead of FixedPage
		M3.6a.oxps -- Multiple <PageContent> elements pointing to the same FixedPage from difference FixedDocuments
		M3.6b.oxps -- Multiple <PageContent> elements pointing to same FixedPage (from the same FixedDocument)
		M4.3a.oxps -- <PathGeometry> element with Figures attribute and <Path.Figure> child element
		M5.2a.oxps -- <Glyphs> with empty UnicodeString attribute and no Indices attribute
		M5.2b.oxps -- <Glyphs> with UnicodeString attribute that contains “{}” and no Indices attribute
		M5.4a.oxps -- <Glyphs> with an Indices attribute that contains an invalid Glyph Index for the specified font
		M5.4b.oxps -- <Glyphs> that has more entries in the indices attribute than the UnicodeString attribute
		M5.7a.oxps -- <Glyphs> that has a UnicodeString attribute that starts with '{'
		M5.15a.oxps -- <Glyphs> that has both BidiLevel and isSideways attributes
		M6.2a.oxps -- <ImageBrush> with x:Key attribute not in a <ResourceDictionary> 
		M6.3a.oxps -- <ImageBrush> that references a font part
		M7.5a.oxps -- Remote resource dictionary that references another remote resource dictionary 
		M12.2a.oxps -- 2 references to the same FixedDocument
		M12.3b.oxps -- FixedPage that is referenced twice from the same FixedDocuments
		M12.5a.oxps -- FixedDocumentSequence with two PrintTickets
		M12.7a.oxps -- ContentType attribute containing a Parameter (ContentType=”image/jpeg;q=0”)

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=