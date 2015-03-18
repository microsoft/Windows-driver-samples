File Name:	SampleXps.zip
Version:	1.0
License:	These sample documents are covered by the terms of the XPSTestFiles_EndUserLicenseAgreement.doc file included in this ZIP archive.
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
	mb01.xps
	mb02.xps
	mb03.xps
	mb04.xps
	mb05.xps
	mb06.xps
	mb07.xps
	mb08.xps
	mb09.xps
	

ConformanceViolations/  -- All these documents violate one conformance rule. They should fail or generate an error.
	MarkupCompatibility/
		M1.1a.xps  -- Unrecognized element from a non-ignorable namespace
		M1.2a.xps  -- Unrecognized element from Markup Compatibility namespace
		M2.13a.xps -- ProcessContent attribute on an element without an Ignorable attribute
		M2.14a.xps -- ProcessContent referencing an element whose namespace is not also listed as Ignorable
		M2.17a.xps -- Xml:lang attribute on a ProcessContent element
		M2.20a.xps -- PreserveElements attribute with no matching Ignorable attribute
		M2.24a.xps -- PreserveAttributes attribute with no matching Ignorable attribute
		M2.27a.xps -- Non-understood and ignorable namespace also identified in the MustUnderstand attribute value
		M2.27b.xps -- Non-understood namespace identified in the MustUnderstand attribute value
		M2.30a.xps -- AlternateContent element with no Choice child elements
		M2.31a.xps -- AlternateContent element with multiple Fallback child elements
		M2.32a.xps -- AlternateContent element with Fallback child element occurring before Choice child element
		M2.33a.xps -- Nested AlternateContent elements
		M2.35a.xps -- AlternateContent element with unprefixed Ignorable attribute. Namespace identified in Ignorable attribute is used.
		M2.35b.xps -- AlternateContent element with unprefixed Ignorable attribute. Namespace identified in Ignorable attribute is not used.
		M2.36a.xps -- AlternateContent having unrecognized child element that is not ignored
		M2.41a.xps -- Choice element having unrecognized and non-ignored attribute
		M2.42a.xps -- Choice element having MustUnderstand attribute referencing bad namespace
		M2.43a.xps -- Xml:lang attribute on AlternateContent element
		M2.44a.xps -- Choice element missing Requires attribute
		M2.48a.xps -- Choice element having unprefixed attribute other than Requires
		M2.49a.xps -- Xml:lang attribute on Choice element
		M2.50a.xps -- Requires attribute referencing non-understood namespace
		M2.52a.xps -- Prefixed Requires attribute applied to Choice element
		M2.55a.xps -- Unprefixed attribute applied to a Fallback element
		M2.56a.xps -- Xml:lang attribute on Fallback element

	OpenPackagingConvetions/
		M1.1a.xps -- InvalidZip -  Simple Text with empty reference to fdseq and empty fdseq name in zip
		M1.1b.xps -- Simple Text without PARTNAME in fdseq
		M1.2a.xps -- Simple Text without FPAGE Content Type
		M1.2b.xps -- Simple Text without JPEG Content Type
		M1.3a.xps -- Simple Text with empty segment in PARTNAME in fdseq
		M1.5a.xps -- Simple Text PARTNAME with forward slash as last char
		M1.6a.xps -- Simple Text PARTNAME with nonpchar in fdseq with orig doc name
		M1.6b.xps -- Simple Text PARTNAME with nonpchar in fdseq
		M1.7a.xps -- Simple Text core prop contains fwd slash pct encode and reference also contains pct encode
		M1.7c.xps -- Simple Text PARTNAME contains '%/'
		M1.8a.xps -- Simple Text PARTNAME contains '%c'
		M1.10a.xps -- Simple Text PARTNAME only dot
		M1.11a.xps -- Simple Text Fixed Doc Seq Part contains content type part name
		M1.17a.xps -- Invalid comma token in content type
		M1.18a.xps -- Content Types Leading Whitespace
		M1.18b.xps -- Content Types LWS between type and subtype
		M1.20a.xps -- Simple Text Doc with Content Type including param
		M1.22a.xps -- Simple Text Content Types with ISO encoding
		M1.26a.xps -- Relationship Missing ID
		M1.26b.xps -- Relationship with invalid XML Identifier
		M1.26c.xps -- Relationships with duplicate IDs
		M1.27a.xps -- Relationship with missing Type
		M1.28a.xps -- Relationship with missing Target
		M2.5a.xps -- Content Type with duplicate default elements
		M2.5b.xps -- Content Type with duplicate override elements
		M2.6a.xps -- Content Type with Content Type but no Extension attrib
		M2.6b.xps -- Content Type with Extension but no Content Type attrib
		M2.7a.xps -- Content Type with Override with no ContentType attrib
		M2.7b.xps -- Content Type with Override with no PartName attrib
		M2.18a.xps -- Simple Text with Non-Interleaved as two consecutive parts
		M2.24a.xps -- Simple Text with fdseq converted to single dot char
		M3.1a.xps -- Simple Text Doc with core props relationship pointing to thumbnail
		M3.1b.xps -- Simple Text Doc with two core props relationships
		M3.2a.xps -- Simple Text Core Prop Part with Markup Compat
		M3.4a.xps -- Simple Text with dig sig containing 2 dig sig origin parts
		M3.4b.xps -- Simple Text with Dig Sig missing relationship to Dig Sig Origin
		M3.5a.xps -- Simple Text with Dig Sig with no rel from dig sig orig to dig sig part
		M3.6a.xps -- Simple Text with dig sig with no relationship to cert
		M3.7a.xps -- Simple Text with dig sig ref with bad fragment identifier
		M3.8a.xps -- Simple Text with dig sig with 2 Object Elements
		M3.9a.xps -- Simple Text with dig sig with Object containing PCDATA
		M3.9b.xps -- Simple Text with dig sig with Object containing Signature
		M3.9c.xps -- Simple Text with dig sig with Sig Value in Object Element
		M3.10a.xps -- Simple Text with dig sig with fragment ident after Content Type
		M3.10b.xps -- Simple Text with dig sig with fragment ident right after URI
		M3.10c.xps -- Simple Text with dig sig with reference to external part
		M3.11a.xps -- Simple Text with Dig sig with ref query content type with bad content
		M3.11b.xps -- Simple Text with Dig Sig with Ref query with missing Content Type
		M3.12a.xps -- Simple Text with Dig Sig with Content Type not case matching
		M3.12b.xps -- Simple Text with dig sig with mismatched Reference Content Type
		M3.13a.xps -- Simple Text with dig sig Reference with unknown XForm
		M3.14a.xps -- Simple Text with dig sig Relationships XForm without Canon XForm
		M3.15a.xps -- Simple Text with dig sig with Signature Prop with no Sig Time
		M3.16a.xps -- Simple Text with dig sig with no Objects
		M3.17a.xps -- Simple Text with dig sig Signed Info missing Ref
		M3.17b.xps -- Simple Text with dig sig signedinfo with ref to incorrect object
		M3.18a.xps -- Simple Text with dig sig with bad algorithm
		M3.20a.xps -- Simple Text with dig sig Reference with unknown XForm
		M3.21a.xps -- Simple Text with dig sig with Manifest Object with no DigestMethod
		M3.21b.xps -- Simple Text with dig sig with no DigestMethod in Package object
		M3.25a.xps -- Simple Text with dig sig with Time format not compat with w3c
		M3.26a.xps -- Simple Text with dig sig with value element not matching format
		M3.28a.xps -- Simple Text with dig sig relation xform after canon xform
		M3.29a.xps -- Simple Text with dig sig with Relation xform having SourceType with case sensitive mismatch
		M3.31a.xps -- Simple Text with dig sig with DigestValue in Object Ref Descendant mismatch
		M3.31b.xps -- Simple Text with dig sig with DigestValue in Object Ref Descendant too long.
		M3.32a.xps -- Simple Text with dig sig with Bad DigestValue in Signed Info - Too long
		M3.32b.xps -- Simple Text with dig sig with Bad DigestValue in Signed Info – Wrong
		M4.1a.xps -- Simple Text with bad URI reference
		M4.2a.xps -- Simple Text PARTNAME contains '%c'
		M4.3a.xps -- Simple Text with percent encoding
		M6.1a.xps -- Simple Text with mismatch filename length
		M6.2a.xps -- Simple Text with invalid file name length
		M6.4a.xps -- Simple Text with unsupported compression method
		M6.6a.xps -- Simple Text with general purpose bit flag specifying files are encrypted
		M6.7a.xps -- Simple Text with offset of central directory equal to unsigned int max
		M6.8a.xps -- Simple Text with Total number of entries in central directory more than maxint
		M7.2a.xps -- Simple Text DC with xml:lang attrib

	XPS/
		M1.2a.xps -- Package is not a zip archive, it’s plain text instead
		M2.3a.xps -- Missing FixedDocumentSequence part
		M2.3b.xps -- Missing fixedrepresentation relationship
		M2.3c.xps -- Two fixedrepresentation relationships
		M2.4a.xps -- Missing FixedDocument part
		M2.4b.xps -- No <DocumentReference> to a FixedDocument
		M2.5a.xps -- Missing FixedPage part
		M2.5b.xps -- No <PageContent> to FixedPage
		M2.6a.xps -- Missing Font Part
		M2.6b.xps -- Missing FontURI attribute
		M2.10a.xps -- Missing required resource relationship for Font in FixedPage
		M2.13a.xps -- Missing fixedrepresentation relationship
		M2.13b.xps -- Two fixedrepresentation relationships
		M2.14a.xps -- Fixedrepresentation relationship pointing to FixedDocucment
		M2.18a.xps -- Corrupt PNG image
		M2.25a.xps -- Corrupt TIF image
		M2.36a.xps -- FixedPage with 2 thumbnails
		M2.37a.xps -- FixedPage with TIF thumbnail
		M2.59a.xps -- Two PrintTickets related to FixedDocumentSequence
		M2.71a.xps -- DTD in Relationship markup
		M2.71b.xps -- DTD in Content_Types markup
		M2.71c.xps -- DTD in FixedPage markup
		M2.73a.xps -- Xml:id attribute in FixedPage markup
		M2.73b.xps -- Xsi:SchemaLocation attribute on <FixedPage> element
		M2.74a.xps -- <Path> having Fill attrib and <Path.Fill> child element
		M2.75a.xps -- Xml:space attribute on <FixedPage> element
		M2.75b.xps -- Xml:space attribute on <Path> element
		M2.76a.xps -- Invalid value for xml:lang attribute on <FixedPage> element
		M3.2a.xps -- <DocucmentReference> Source attribute pointing to Fixed Page instead of FixedDocucment
		M3.3a.xps -- Multiple <DocumentReference> elements pointing to same FixedDocument
		M3.5a.xps -- <PageContent> Source attrib pointing to FixedDocucment instead of FixedPage
		M3.6a.xps -- Multiple <PageContent> elements pointing to the same FixedPage from difference FixedDocuments
		M3.6b.xps -- Multiple <PageContent> elements pointing to same FixedPage (from the same FixedDocument)
		M4.3a.xps -- <PathGeometry> element with Figures attribute and <Path.Figure> child element
		M5.2a.xps -- <Glyphs> with empty UnicodeString attribute and no Indices attribute
		M5.2b.xps -- <Glyphs> with UnicodeString attribute that contains “{}” and no Indices attribute
		M5.4a.xps -- <Glyphs> with an Indices attribute that contains an invalid Glyph Index for the specified font
		M5.4b.xps -- <Glyphs> that has more entries in the indices attribute than the UnicodeString attribute
		M5.7a.xps -- <Glyphs> that has a UnicodeString attribute that starts with '{'
		M5.15a.xps -- <Glyphs> that has both BidiLevel and isSideways attributes
		M6.2a.xps -- <ImageBrush> with x:Key attribute not in a <ResourceDictionary> 
		M6.3a.xps -- <ImageBrush> that references a font part
		M7.5a.xps -- Remote resource dictionary that references another remote resource dictionary 
		M12.2a.xps -- 2 references to the same FixedDocument
		M12.3b.xps -- FixedPage that is referenced twice from the same FixedDocuments
		M12.5a.xps -- FixedDocumentSequence with two PrintTickets
		M12.7a.xps -- ContentType attribute containing a Parameter (ContentType=”image/jpeg;q=0”)

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=