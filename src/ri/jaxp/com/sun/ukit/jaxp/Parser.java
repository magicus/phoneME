/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */


package com.sun.ukit.jaxp;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import java.io.InputStream;
import java.io.Reader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.Attributes;
import org.xml.sax.Locator;
import org.xml.sax.InputSource;
import org.xml.sax.SAXParseException;
import org.xml.sax.SAXException;

import com.sun.ukit.jaxp.DTD.ExternalID;

import javax.xml.parsers.SAXParser;

class QName {
	final static String prefix( char[] qname ){
		return (qname[0] == 0)? "" : new String( qname, 1, qname[0] - 1 );
	}
	
	final static String local( char[] qname ){
		return new String( qname, 1 + qname[0], qname.length - (1 + qname[0]) );
	}
	
	final static String qname( char[] qname ){
		return new String( qname, 1, qname.length - 1 );
	}
}

/**
 * XML non-validating parser.
 *
 * This non-validating parser conforms to <a href="http://www.w3.org/TR/REC-xml"
 * >Extensible Markup Language (XML) 1.0</a> and <a href="http://www.w3.org/TR/REC-xml-names"
 * >"Namespaces in XML"</a> specifications. 
 * The API used by the parser is <a href="http://www.jcp.org/en/jsr/detail?id=172"
 * >JSR-172</a> subset of <a href="http://java.sun.com/xml/jaxp/index.html">JAXP</a> 
 * and <a href="http://www.saxproject.org/">SAX2</a>.
 *
 * @see org.xml.sax.helpers.DefaultHandler
 */

public final class Parser extends SAXParser implements Locator
{
	final static java.io.PrintStream DEBUG_OUT = System.out; 
	static final boolean STRICT = true; 
	
	public final static String	FAULT	= "";

	private final static int  BUFFSIZE_READER	= 512;
	private final static int  BUFFSIZE_PARSER	= 128;

	/** The end of stream character. */
	public final static char	EOS		= 0xffff;

	private DefaultHandler	mHand;		// a document handler

	private boolean			mIsSAlone;	// xml decl standalone flag
	final private boolean	mIsNSAware;	// if true - to report QName

	private short			mSt;		// global state of the parser
	// mSt values:
	// - 0 : the beginning of the document
	// - 1 : misc before DTD
	// - 2 : DTD
	// - 3 : misc after DTD
	// - 4 : document's element
	// - 5 : misc after document's element
	private final short stateStartOfTheDocument = 0;
	private final short stateMiscBeforeDTD = 1;
	private final short stateInsideDTD = 2;
	private final short stateMiscAfterDTD = 3;
	private final short stateDocument = 4;
	private final short stateMiscAfterDocument = 5;

	private char			mESt;		// built-in entity recognizer state
	// mESt values:
	//   0x100   : the initial state
	//   > 0x100 : unrecognized name
	//   < 0x100 : replacement character

	final private DTD		dtd = new DTD();
	final private Namespace.Stack	namespaceStack = new Namespace.Stack();
	final private Element.Stack		elementStack = new Element.Stack();
	final private Attrs		attributes;	// attributes of the current element

	private Input			mInp;		// stack of entities
	private Input			mDoc;		// document entity

	private char[]			mChars;		// reading buffer
	private int				mChLen;		// current capacity
	private int				mChIdx;		// index to the next char
 
	private char[]			mBuff;		// parser buffer
	private int				mBuffIdx;	// index of the last char

	private final DTD.ExternalID tempExternalID = new DTD.ExternalID();


	final static String XMLNSSTR = "xmlns";

	/**
	 * ASCII character type array.
	 *
	 * This array maps an ASCII (7 bit) character to the character type.<br /> 
	 * Possible character type values are:<br /> 
	 * - ' ' for any kind of white space character;<br /> 
	 * - 'a' for any lower case alphabetical character value;<br /> 
	 * - 'A' for any upper case alphabetical character value;<br /> 
	 * - 'd' for any decimal digit character value;<br /> 
	 * - 'z' for any character less then ' ' except '\t', '\n', '\r';<br /> 
	 * An ASCII (7 bit) character which does not fall in any category listed 
	 * above is mapped to it self.
	 */
	private static final byte asctyp[];

	/**
	 * NMTOKEN character type array.
	 *
	 * This array maps an ASCII (7 bit) character to the character type.<br /> 
	 * Possible character type values are:<br /> 
	 * - 0 for underscore ('_') or any lower and upper case alphabetical character value;<br /> 
	 * - 1 for colon (':') character;<br /> 
	 * - 2 for dash ('-') and dot ('.') or any decimal digit character value;<br /> 
	 * - 3 for any kind of white space character<br /> 
	 * An ASCII (7 bit) character which does not fall in any category listed 
	 * above is mapped to 0xff.
	 */
	private static final byte nmttyp[];

	/**
	 * Static constructor.
	 *
	 * Sets up the ASCII character type array which is used by 
	 * {@link #asctyp asctyp} method and NMTOKEN character type array.
	 */
	static {
		short i	= 0;

		asctyp	= new byte[0x80];
		while (i < ' ')
			asctyp[i++]	= (byte)'z';
		asctyp['\t']	= (byte)' ';
		asctyp['\r']	= (byte)' ';
		asctyp['\n']	= (byte)' ';
		while (i < '0')
			asctyp[i]	= (byte)i++;
		while (i <= '9')
			asctyp[i++]	= (byte)'d';
		while (i < 'A')
			asctyp[i]	= (byte)i++;
		while (i <= 'Z')
			asctyp[i++]	= (byte)'A';
		while (i < 'a')
			asctyp[i]	= (byte)i++;
		while (i <= 'z')
			asctyp[i++]	= (byte)'a';
		while (i < 0x80)
			asctyp[i]	= (byte)i++;

		nmttyp	= new byte[0x80];
		for (i = 0; i < '0'; i++)
			nmttyp[i]   = (byte)0xff;
		while (i <= '9')
			nmttyp[i++] = (byte)2;	// digits
		while (i < 'A')
			nmttyp[i++] = (byte)0xff;
		// skipped upper case alphabetical character are already 0
		for (i = '['; i < 'a'; i++)
			nmttyp[i]   = (byte)0xff;
		// skipped lower case alphabetical character are already 0
		for (i = '{'; i < 0x80; i++)
			nmttyp[i]   = (byte)0xff;
		nmttyp['_']  = 0;
		nmttyp[':']  = 1;
		nmttyp['.']  = 2;
		nmttyp['-']  = 2;
		nmttyp[' ']  = 3;
		nmttyp['\t'] = 3;
		nmttyp['\r'] = 3;
		nmttyp['\n'] = 3;
	}
	
	final static void ASSERT( boolean v ){
		if( !v ) throw new IllegalArgumentException();
	}

	/**
	 * Constructor.
	 */
	public Parser(boolean nsaware)
	{
		super();
		mIsNSAware = nsaware;
		attributes = mIsNSAware? (Attrs)new Attrs.NSAware() : 
						(Attrs)new Attrs.NotNSAware(); 

		//		Initialize the parser
		mBuff	= new char[BUFFSIZE_PARSER];
	}

	/**
	 * Return the public identifier for the current document event.
	 *
	 * <p>The return value is the public identifier of the document
	 * entity or of the external parsed entity in which the markup
	 * triggering the event appears.</p>
	 *
	 * @return A string containing the public identifier, or
	 *	null if none is available.
	 *
	 * @see #getSystemId
	 */
	public String getPublicId()
	{
		return (mInp != null)? mInp.pubid: null;
	}

	/**
	 * Return the system identifier for the current document event.
	 *
	 * <p>The return value is the system identifier of the document
	 * entity or of the external parsed entity in which the markup
	 * triggering the event appears.</p>
	 *
	 * <p>If the system identifier is a URL, the parser must resolve it
	 * fully before passing it to the application.</p>
	 *
	 * @return A string containing the system identifier, or null
	 *	if none is available.
	 *
	 * @see #getPublicId
	 */
	public String getSystemId()
	{
		return (mInp != null)? mInp.sysid: null;
	}

	/**
	 * Return the line number where the current document event ends.
	 *
	 * @return Always returns -1 indicating the line number is not 
	 *	available.
	 *
	 * @see #getColumnNumber
	 */
	public int getLineNumber()
	{
		return -1;
	}

	/**
	 * Return the column number where the current document event ends.
	 *
	 * @return Always returns -1 indicating the column number is not 
	 *	available.
	 *
	 * @see #getLineNumber
	 */
	public int getColumnNumber()
	{
		return -1;
	}

	/**
	 * Indicates whether or not this parser is configured to
	 * understand namespaces.
	 *
	 * @return true if this parser is configured to
	 *         understand namespaces; false otherwise.
	 */
	public boolean isNamespaceAware()
	{
		return mIsNSAware;
	}

	/**
	 * Indicates whether or not this parser is configured to validate
	 * XML documents.
	 *
	 * @return true if this parser is configured to validate XML
	 *          documents; false otherwise.
	 */
	public boolean isValidating()
	{
		return false;
	}

	/**
	 * Parse the content of the given {@link java.io.InputStream}
	 * instance as XML using the specified
	 * {@link org.xml.sax.helpers.DefaultHandler}.
	 *
	 * @param src InputStream containing the content to be parsed.
	 * @param handler The SAX DefaultHandler to use.
	 * @exception IOException If any IO errors occur.
	 * @exception IllegalArgumentException If the given InputStream or handler is null.
	 * @exception SAXException If the underlying parser throws a
	 * SAXException while parsing.
	 * @see org.xml.sax.helpers.DefaultHandler
	 */
	public void parse(InputStream src, DefaultHandler handler)
		throws SAXException, IOException
	{
		if ((src == null) || (handler == null))
			throw new IllegalArgumentException("");
		parse(new InputSource(src), handler);
	}

	/**
	 * Parse the content given {@link org.xml.sax.InputSource}
	 * as XML using the specified
	 * {@link org.xml.sax.helpers.DefaultHandler}.
	 *
	 * @param is The InputSource containing the content to be parsed.
	 * @param handler The SAX DefaultHandler to use.
	 * @exception IOException If any IO errors occur.
	 * @exception IllegalArgumentException If the InputSource or handler is null.
	 * @exception SAXException If the underlying parser throws a
	 * SAXException while parsing.
	 * @see org.xml.sax.helpers.DefaultHandler
	 */
	public void parse(InputSource is, final DefaultHandler handler)
		throws SAXException, IOException
	{
		if ((is == null) || (handler == null))
			throw new IllegalArgumentException("");
		// Set up the handler
		mHand = handler;
		if( DEBUG_OUT != null ){
			mHand = new DefaultHandler(){
				public void characters(char[] ch, int start, int length)
						throws SAXException {
					handler.characters(ch, start, length);
				}
				public void endDocument() throws SAXException {
					handler.endDocument();
				}
				public void ignorableWhitespace(char[] ch, int start, int length)
						throws SAXException {
					handler.ignorableWhitespace(ch, start, length);
				}
				public void notationDecl(String name, String publicId,
						String systemId) throws SAXException {
					handler.notationDecl(name, publicId, systemId);
				}
				public void processingInstruction(String target, String data)
						throws SAXException {
					handler.processingInstruction(target, data);
				}
				public InputSource resolveEntity(String publicId,
						String systemId) throws SAXException {
					return handler.resolveEntity(publicId, systemId);
				}
				public void setDocumentLocator(Locator locator) {
					handler.setDocumentLocator(locator);
				}
				public void skippedEntity(String name) throws SAXException {
					handler.skippedEntity(name);
				}
				public void startDocument() throws SAXException {
					handler.startDocument();
				}
				public void unparsedEntityDecl(String name, String publicId,
								String systemId, String notationName) throws SAXException {
					handler.unparsedEntityDecl(name, publicId, systemId, notationName);
				}
				public void endElement(String uri, String localName, String name)
						throws SAXException {
					DEBUG_OUT.println( "endElement( '" + uri + "', '" + localName + "', '" + name + "' )" );
					handler.endElement(uri, localName, name);
				}
				public void endPrefixMapping(String prefix) throws SAXException {
					DEBUG_OUT.println( "endPrefixMapping( '" + prefix + "' )" );
					handler.endPrefixMapping(prefix);
				}
				public void error(SAXParseException e) throws SAXException {
					e.printStackTrace();
					handler.error(e);
				}
				public void fatalError(SAXParseException e) throws SAXException {
					e.printStackTrace();
					handler.fatalError(e);
				}
				public void startElement(String uri, String localName,
						String name, Attributes attributes) throws SAXException {
					DEBUG_OUT.println( "startElement( '" + uri + "', '" + localName + "', '" + name + "' )" );
					for(int i = 0; i < attributes.getLength(); i++){
						DEBUG_OUT.println( "\t[" + i + "]: '" + attributes.getURI(i) + "', '" +
									attributes.getLocalName(i) + "', '" + attributes.getQName(i) + "' " +
									attributes.getType(i) + " \"" + attributes.getValue(i) + "\"");
					}
					handler.startElement(uri, localName, name, attributes);
				}
				public void startPrefixMapping(String prefix, String uri)
						throws SAXException {
					DEBUG_OUT.println( "startPrefixMapping( '" + prefix + "', '" + uri + "' )" );
					handler.startPrefixMapping(prefix, uri);
				}
				public void warning(SAXParseException e) throws SAXException {
					e.printStackTrace();
					handler.warning(e);
				}
			};
		}
		// Set up the document
		mInp = new Input(BUFFSIZE_READER);
		setinp(is);
		parse();
	}

	/**
	 * Parse the XML document content using the specified
	 * {@link org.xml.sax.helpers.DefaultHandler}.
	 *
	 * @exception IOException If any IO errors occur.
	 * @exception SAXException If the underlying parser throws a
	 * SAXException while parsing.
	 * @see org.xml.sax.helpers.DefaultHandler
	 */
	private void parse() throws SAXException, IOException {
		try {
			//		Initialize the parser
			mDoc = mInp;			// current input is document entity
			mChars = mInp.chars;	// use document entity buffer
			elementStack.clean();
			namespaceStack.clean();
			//		Parse an xml document
			char ch;
			// mHand.setDocumentLocator(this);
			mHand.startDocument();
			mSt	= stateMiscBeforeDTD;
			while ((ch = next()) != EOS) {
				switch (chtyp(ch)) {
				case '<':
					ch	= next();
					switch (ch) {
					case '?':
						pi();
						break;

					case '!':
						ch	= next();
						back();
						if (ch == '-')
							comm();
						else
							dtd();
						break;

					default:			// must be the first char of an xml name 
						if (mSt == stateMiscAfterDocument)	// misc after document's element
							panic(FAULT);
						//		Document's element.
						back();
						mSt	= stateDocument;		// document's element
						parseDocumentElement();
						mSt	= stateMiscAfterDocument;		// misc after document's element
						break;
					}
					break;

				case ' ':
					//		Skip white spaces
					break;

				default:
					panic(FAULT);
				}
			}
			if (mSt != stateMiscAfterDocument)	// misc after document's element
				panic(FAULT);
		} catch( RuntimeException t ) {
			t.printStackTrace();
			throw t;
		} finally {
			mHand.endDocument();
			while (mInp != null)
				pop();
			if ((mDoc != null) && (mDoc.src != null)) {
				try { mDoc.src.close(); } catch (IOException ioe) {}
			}
			dtd.clean();
			mDoc = null;
			mHand = null;
			mSt = stateStartOfTheDocument;
		}
	}

	/**
	 * Parses the document type declaration.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtd() throws SAXException, IOException
	{
		char	ch;
		// read 'DOCTYPE'
		if (!((mSt == stateStartOfTheDocument || mSt == stateMiscBeforeDTD) && 
				"DOCTYPE".equals(name(false))))
			panic(FAULT);
		dtd.clean();
		mSt	= stateInsideDTD;	// DTD
		for (short st = 0; st >= 0;) {
			ch	= next();
			switch (st) {
			case 0:		// read the document type name
				if (chtyp(ch) != ' ') {
					back();
					dtd.name = name(mIsNSAware);
					wsskip();
					st		= 1;	// read 'PUPLIC' or 'SYSTEM'
				}
				break;

			case 1:		// read 'PUPLIC' or 'SYSTEM'
				switch (chtyp(ch)) {
				case 'A':
					back();
					dtd.externalID = new DTD.ExternalID(); 
					pubsys(' ', dtd.externalID);
					st	= 2;	// skip spaces before internal subset
					break;

				case '[':
					back();
					st	= 2;	// skip spaces before internal subset
					break;

				case '>':
					back();
					st	= 3;	// skip spaces after internal subset
					break;

				default:
					panic(FAULT);
				}
				break;

			case 2:		// skip spaces before internal subset
				switch (chtyp(ch)) {
				case '[':
					//		Process internal subset
					dtdsub();
					st	= 3;	// skip spaces after internal subset
					break;

				case '>':
					//		There is no internal subset
					back();
					st	= 3;	// skip spaces after internal subset
					break;

				case ' ':
					// skip white spaces
					break;

				default:
					panic(FAULT);
				}
				break;

			case 3:		// skip spaces after internal subset
				switch (chtyp(ch)) {
				case '>':
					if (dtd.externalID != ExternalID.EMPTY) {
						//		Report the DTD external subset
						InputSource is = mHand.resolveEntity(dtd.externalID.pubidLiteral, 
													dtd.externalID.systemLiteral);
						if (is != null) {
							if (mIsSAlone == false) {
								//		Set the end of DTD external subset char
								back();
								setch(']');
								//		Set the DTD external subset InputSource
								push(new Input(BUFFSIZE_READER));
								setinp(is);
								mInp.pubid = dtd.externalID.pubidLiteral;
								mInp.sysid = dtd.externalID.systemLiteral;
								//		Parse the DTD external subset
								dtdsub();
							} else {
								//		Unresolved DTD external subset
								mHand.skippedEntity("[dtd]");
								//		Release reader and stream
								if (is.getCharacterStream() != null) {
									try {
										is.getCharacterStream().close();
									} catch (IOException ioe) {
									}
								}
								if (is.getByteStream() != null) {
									try {
										is.getByteStream().close();
									} catch (IOException ioe) {
									}
								}
							}
						} else {
							//		Unresolved DTD external subset
							mHand.skippedEntity("[dtd]");
						}
					}
					st	= -1;	// end of DTD
					break;

				case ' ':
					// skip white spaces
					break;

				default:
					panic(FAULT);
				}
				break;

			default:
				panic(FAULT);
			}
		}
		mSt	= stateMiscAfterDTD;	// misc after DTD
		
		if( DEBUG_OUT != null ){
			DEBUG_OUT.println( "DTD.attLists:" );
			for( Enumeration e = dtd.attLists.keys(); e.hasMoreElements();){
				DEBUG_OUT.println( "\t" + e.nextElement() );
			}
		}
	}

	/**
	 * Parses the document type declaration subset.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtdsub()
		throws SAXException, IOException
	{
		char ch;
		for (short st = 0; st >= 0;) {
			ch	= next();
			switch (st) {
			case 0:		// skip white spaces before a declaration
				switch (chtyp(ch)) {
				case '<':
					ch	= next();
					switch (ch) {
					case '?':
						pi();
						break;

					case '!':
						ch	= next();
						back();
						if (ch == '-') {
							comm();
							break;
						}
						// markup or entity declaration
						bntok();
						switch (bkeyword()) {
						case 'n': // ENTITY
							dtdent();
							break;

						case 'a':
							dtdattl();		// parse attributes declaration
							break;

						case 'e':
							dtdelm();		// parse element declaration
							break;

						case 'o':
							dtdnot();		// parse notation declaration
							break;

						default:
							panic(FAULT);	// unsupported markup declaration
						}
						st	= 1;		// read the end of declaration
						break;

					default:
						panic(FAULT);
						break;
					}
					break;

				case '%':
					//		A parameter entity reference
					pent(' ');
					break;

				case ']':
					//		End of DTD subset
					st	= -1;
					break;

				case ' ':
					//		Skip white spaces
					break;

				case 'Z':
					//		End of stream
					if (next() != ']')
						panic(FAULT);
					st	= -1;
					break;

				default:
					panic(FAULT);
				}
				break;

			case 1:		// read the end of declaration
				switch (ch) {
				case '>':		// there is no notation 
					st	= 0;	// skip white spaces before a declaration
					break;

				case ' ':
				case '\n':
				case '\r':
				case '\t':
					//		Skip white spaces
					break;

				default:
					panic(FAULT);
					break;
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Parses an entity declaration.
	 * This method fills the general (<code>mEnt</code>) and parameter 
	 * (<code>mPEnt</code>) entity look up table.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtdent()
		throws SAXException, IOException
	{
		String	entityName = null;
		char[]	val = null;
		Input   inp = null;
		char	ch;
		for (short st = 0; st >= 0;) {
			ch	= next();
			switch (st) {
			case 0:		// skip white spaces before entity name
				switch (chtyp(ch)) {
				case ' ':
					//		Skip white spaces
					break;

				case '%':
					//		Parameter entity or parameter entity declaration.
					ch = next();
					back();
					if (chtyp(ch) == ' ') {
						//		Parameter entity declaration.
						wsskip();
						entityName = name(false);
						switch (chtyp(wsskip())) {
						case 'A':
							//		Read the external identifier
							pubsys(' ', tempExternalID);
							if (wsskip() == '>') {
								//		External parsed entity
								if (dtd.parameterEntities == null || 
										!dtd.parameterEntities.containsKey(entityName)) {	// [#4.2]
									inp       = new Input();
									inp.pubid = tempExternalID.pubidLiteral;
									inp.sysid = tempExternalID.systemLiteral;
									if(dtd.parameterEntities == null)
										dtd.parameterEntities = new Hashtable();
									dtd.parameterEntities.put(entityName, inp);
								}
							} else {
								panic(FAULT);
							}
							st	= -1;	// the end of declaration
							break;

						case '\"':
						case '\'':
							//		Read the parameter entity value
							bqstr('d');
							//		Add the entity to the entity look up table
							if (dtd.parameterEntities == null || 
									!dtd.parameterEntities.containsKey(entityName)) {	// [#4.2]
								//		Create the parameter entity value
								val	= new char[mBuffIdx + 1];
								System.arraycopy(mBuff, 1, val, 1, val.length - 1);
								//		Add surrounding spaces [#4.4.8]
								val[0] = ' ';
								
								inp       = new Input(val);
								inp.pubid = mInp.pubid;
								inp.sysid = mInp.sysid;
								if(dtd.parameterEntities == null)
									dtd.parameterEntities = new Hashtable();
								dtd.parameterEntities.put(entityName, inp);
							}
							st	= -1;	// the end of declaration
							break;

						default:
							panic(FAULT);
						}
					} else {
						//		Parameter entity reference.
						pent(' ');
					}
					break;

				default:
					back();
					entityName = name(false);
					st	= 1;	// read entity declaration value
					break;
				}
				break;

			case 1:		// read general entity declaration value
				switch (chtyp(ch)) {
				case '\"':	// internal entity
				case '\'':
					back();
					bqstr('d');	// read a string into the buffer
					if (dtd.generalEntities == null || 
							!dtd.generalEntities.containsKey(entityName) /*[#4.2]*/) {
						//		Create general entity value
						val	= new char[mBuffIdx];
						System.arraycopy(mBuff, 1, val, 0, val.length);
						//		Add the entity to the entity look up table
						inp       = new Input(val);
						inp.pubid = mInp.pubid;
						inp.sysid = mInp.sysid;
						if( dtd.generalEntities == null )
							dtd.generalEntities = new Hashtable();
						dtd.generalEntities.put(entityName, inp);
					}
					st	= -1;	// the end of declaration
					break;

				case 'A':	// external entity
					back();
					pubsys(' ', tempExternalID);
					switch (wsskip()) {
					case '>':	// external parsed entity
						if (dtd.generalEntities == null || 
								!dtd.generalEntities.containsKey(entityName)) {	// [#4.2]
							inp       = new Input();
							inp.pubid = tempExternalID.pubidLiteral;
							inp.sysid = tempExternalID.systemLiteral;
							if( dtd.generalEntities == null )
								dtd.generalEntities = new Hashtable();
							dtd.generalEntities.put(entityName, inp);
						}
						break;

					case 'N':	// external general unparsed entity
						if ("NDATA".equals(name(false))) {
							wsskip();
							mHand.unparsedEntityDecl( entityName, 
										tempExternalID.pubidLiteral, tempExternalID.systemLiteral, 
										name(false) );
							break;
						}
						// no break here!
					default:
						panic(FAULT);
						break;
					}
					st	= -1;	// the end of declaration
					break;

				case ' ':
					//		Skip white spaces
					break;

				default:
					panic(FAULT);
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}
	
	/**
	 * Parses (consumes) a 'choise' or 'seq' declaration without starting '('. 
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private DTD.ChoiceOrSeq parseChoiseOrSeq() throws SAXException, IOException {
//		[49] choice	::= '(' S? cp ( S? '|' S? cp )+ S? ')'
//		[50] seq	::= '(' S? cp ( S? ',' S? cp )* S? ')'
		DTD.ChoiceOrSeq result = null;
		boolean done = false;
		DTD.Cp cp = parseCp();
		char ch = wsskip();
		switch( ch ){
			case ')': 
				result = new DTD.Seq();
				done = true;
				break;
			case '|':
				result = new DTD.Choice();
				break;
			case ',': 
				result = new DTD.Seq();
				break;
			default:
				panic(FAULT);
		}
		next(); // skip acceptable character 
		result.add( cp );
		final char delimiter = ch; 
		while( !done ){
			result.add( parseCp() );
			switch( ch = wsskip() ){
				case ')': 
					done = true;
					break;
				case '|': case ',': 
					if( delimiter != ch )
						panic(FAULT);
					break;
				default:
					panic(FAULT);
			}
			next(); // skip acceptable character
		}
		result.invertChildrenOrder();
		return result;
	}

	/**
	 * Parses a 'cp' declaration. + leading white spaces
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private DTD.Cp parseCp() throws SAXException, IOException
//	[48] cp ::= (Name | choice | seq) ('?' | '*' | '+')?
//	http://www.w3.org/TR/REC-xml-names/#ns-using[18] cp	::= (QName | choice | seq) ('?' | '*' | '+')? 
	{
		DTD.Cp result = null;
		char ch = wsskip();
		if( ch == '(' ){ // choice | seq
			next();
			result = parseChoiseOrSeq();
		} else { // Name
			result = new DTD.CpName( name(mIsNSAware) ); 
		}
		// ('?' | '*' | '+')?
		switch( (ch = next()) ){
			case '?': case '*': case '+':
				result.modifier = ch;
				break;
			default:
				back();
		}
		return result;
	}

	/**
	 * Parses an element declaration. 
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtdelm() throws SAXException, IOException
//		[45]   	elementdecl	   ::=   	'<!ELEMENT' S  Name  S  contentspec  S? '>'
//		[46]   	contentspec	   ::=   	'EMPTY' | 'ANY' | Mixed | children
//		[47]   	children	   ::=   	(choice | seq) ('?' | '*' | '+')?
//		[48]   	cp	   ::=   	(Name | choice | seq) ('?' | '*' | '+')?
//		[49]   	choice	   ::=   	'(' S? cp ( S? '|' S? cp )+ S? ')'
//		[50]   	seq	   ::=   	'(' S? cp ( S? ',' S? cp )* S? ')'
//		[51]   	Mixed	   ::=   	'(' S? '#PCDATA' (S? '|' S? Name)* S? ')*'
//										| '(' S? '#PCDATA' S? ')'
	{
		wsskip();
		String elementName = name(mIsNSAware);
		// parse contentspec (it is used only in elementdecl)
		char ch = wsskip();
		switch( ch ){
			case '(': // <Mixed> or <children>
				next(); // skip '('
				ch = wsskip();
				if( ch == '#' ){ // Mixed
					next(); // skip '#'
					bntok();
					if( bkeyword() != 'p' )
						panic(FAULT); // "#PCDATA expected"
					DTD.Mixed mixed = new DTD.Mixed(); 
					int namesCount = 0;
					do {
						if( wsskip() != '|' ) break;
						next(); // skip '|'
						wsskip();
						mixed.add( name(mIsNSAware) );
						namesCount++;
					} while(true);
					if( next() != ')' )
						panic(FAULT); // ')' or ')*' expected
					if( next() != '*' ){
						if( namesCount > 0 )
							panic(FAULT); // '*' expected as a part of ')*'
						back();
					} else mixed.finishWithAsterisk  = true;
					mixed.names = (NamesList)LinkedList.invert(mixed.names);
					dtd.elements.put(elementName, mixed);
				} else { // (choice | seq) ('?' | '*' | '+')?
					DTD.Cp choiseOrSeq = parseChoiseOrSeq();
					// ('?' | '*' | '+')?
					switch( (ch = next()) ){
						case '?': case '*': case '+':
							choiseOrSeq.modifier = ch;
							break;
						default:
							back();
					}

					dtd.elements.put( elementName, choiseOrSeq );
				}
				break;
				
			default: // EMPTY or ANY
				bntok();
				switch( bkeyword() ){
					case 'E':
						dtd.elements.put(elementName, DTD.EMPTY);
						break;
					case 'A':
						dtd.elements.put(elementName, DTD.ANY);
						break;
					default:
						panic(FAULT);
				}
		}
		// S? '>'
		if( wsskip() != '>' )
			panic(FAULT);
		// do not consume '>' character
	}

	/**
	 * Parses an attribute list declaration.
	 *
	 * This method parses the declaration up to the closing angle 
	 * bracket.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtdattl() throws SAXException, IOException
	{
		char elmqn[] = null;
		Hashtable/*<String, AttDef>*/ attList = null;
		
		char ch;
		for (short st = 0; st >= 0;) {
			ch = next();
			switch (st) {
			case 0:		// skip spaces, read the element name
				switch (chtyp(ch)) {
				case ' ':
					break;

				case 'a':
				case 'A':
				case '_':
				case 'X':
				case ':':
					back();
					//		Get the element from the list or add a new one.
					elmqn = qname(mIsNSAware);
					attList = dtd.getAttList( QName.qname(elmqn) ); 
					if( DEBUG_OUT != null ){
						DEBUG_OUT.println( "attl '" + QName.qname(elmqn) + "'" );
					}
					st = 1;		// read an attribute declarations
					break;

				case '%':
					pent(' ');
					break;

				default:
					panic(FAULT);
				}
				break;

			case 1:		// read an attribute declaration
				switch (chtyp(ch)) {
				case 'a':
				case 'A':
				case '_':
				case 'X':
				case ':':
					back();
					dtdatt(attList);
					if (wsskip() == '>')
						st = -1;
					break;

				case ' ':
					break;

				case '%':
					pent(' ');
					break;

				default:
					panic(FAULT);
				}
				break;

			default:
				panic(FAULT);
			}
		}
		// check if the element has explicit namespace specification
		DTD.AttDef attdef = null;
		String prefix = QName.prefix(elmqn);
		if( "".equals( prefix ) ){
			attdef = (DTD.AttDef)attList.get(XMLNSSTR);
		} else {
			attdef = (DTD.AttDef)attList.get(XMLNSSTR + ":" + prefix);
		}
		if( attdef != null && attdef.defaultDeclType == DTD.AttDef.ddtFIXED ){
			dtd.addAttList(attdef.defaultDeclValue, QName.local(elmqn), attList);
		}
	}

	/**
	 * dummy element for duplicated attribute definitions
	 */
	private static final DTD.AttDef dummyAttDef = new DTD.AttDef();
	
	/**
	 * Parses an attribute declaration.
	 *
	 * The attribute uses the following fields of Pair object:
	 * chars - characters of qualified name
	 * id    - the type identifier of the attribute
	 * list  - a pair which holds the default value (chars field)
	 *
	 * @param elm An object which represents all defined attributes on an element.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtdatt(Hashtable/*<String, AttDef>*/ attList) 
							throws SAXException, IOException
	{
		char attqn[] = null;
		DTD.AttDef attdef = null;
		
		char ch;
		for (short st = 0; st >= 0;) {
			ch = next();
			switch (st) {
			case 0:		// the attribute name
				switch (chtyp(ch)) {
				case 'a':
				case 'A':
				case '_':
				case 'X':
				case ':':
					back();
					//		Get the attribute from the list or add a new one.
					attqn = qname(mIsNSAware);
					String qname = QName.qname(attqn);
					attdef = (DTD.AttDef)attList.get( qname );
					if (attdef == null) {
						//		New attribute declaration
						attdef = new DTD.AttDef();
						attdef.localName = QName.local(attqn);
						attList.put( qname, attdef );
					} else {
						//		Do not override the attribute declaration [#3.3]
						attdef = dummyAttDef;
						attdef.clean();
					}
					wsskip();
					st = 1;
					break;

				case '%':
					pent(' ');
					break;

				case ' ':
					break;

				default:
					panic(FAULT);
				}
				break;

			case 1:		// the attribute type
				switch (chtyp(ch)) {
				case '(': 
					attdef.attType = DTD.AttDef.typeNMTOKEN; // see org.xml.sax.Attributes.getType()
					st = 2;			// read the first element of the list
					break;

				case '%':
					pent(' ');
					break;

				case ' ':
					break;

				default:
					back();
					bntok();		// read type id
					attdef.attType = new String(mBuff, 1, mBuffIdx);
					
					switch (bkeyword()) {
					case 'o':		// NOTATION
						if (chtyp(next()) != ' ' && wsskip() != '(')
							panic(FAULT);
						ch = next();
						st = 2;		// read the first element of the list
						break;

					case 'i':		// ID
					case 'r':		// IDREF
					case 'R':		// IDREFS
					case 'n':		// ENTITY
					case 'N':		// ENTITIES
					case 't':		// NMTOKEN
					case 'T':		// NMTOKENS
					case 'c':		// CDATA
						wsskip();
						st = 4;		// read default declaration
						break;

					default:
						panic(FAULT);
					}
					break;
				}
				break;

			case 2:		// read the first element of the list
				// [58] NotationType ::= 'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'
				// [59] Enumeration ::= '(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')'
				switch (chtyp(ch)) {
				case 'a':
				case 'A':
				case 'd':
				case '.':
				case ':':
				case '-':
				case '_':
				case 'X':
					back();
					if( attdef.attType == DTD.AttDef.typeNMTOKEN ) {
						// enumeration type
						bntok();
					} else { // NOTATION
						mBuffIdx = -1;
						bname(false);
					}
					String value = new String(mBuff, 1, mBuffIdx);
					attdef.enumeratedTypeValues = new Hashtable();
					attdef.enumeratedTypeValues.put( value, value );
					wsskip();
					st = 3;		// read next element of the list
					break;

				case '%':
					pent(' ');
					break;

				case ' ':
					break;

				default:
					panic(FAULT);
				}
				break;

			case 3:		// read next element of the list
				switch (ch) {
				case ')':
					wsskip();
					st = 4;		// read default declaration
					break;

				case '|':
					wsskip();
					if( attdef.attType == DTD.AttDef.typeNMTOKEN ) {
						// enumeration type
						bntok();
					} else { // NOTATION
						mBuffIdx = -1;
						bname(false);
					}
					String value = new String(mBuff, 1, mBuffIdx);
					if( attdef.enumeratedTypeValues.put( value, value ) != null )
						panic(FAULT); // Validity constraint: No Duplicate Tokens
					wsskip();
					break;

				case '%':
					pent(' ');
					break;

				default:
					panic(FAULT);
				}
				break;

			case 4:		// read default declaration
				switch (ch) {
				case '#':
					bntok();
					switch (bkeyword()) {
					case 'F':	// FIXED
						attdef.defaultDeclType = DTD.AttDef.ddtFIXED;
						wsskip();
						st = 5;	// read the default value
						break;

					case 'Q':	// REQUIRED
						attdef.defaultDeclType = DTD.AttDef.ddtREQUIRED;
						st = -1;
						break;
						
					case 'I':	// IMPLIED
						attdef.defaultDeclType = DTD.AttDef.ddtIMPLIED;
						st = -1;
						break;

					default:
						panic(FAULT);
					}
					break;

				case '\"':
				case '\'':
					back();
					st = 5;			// read the default value
					break;

				case ' ':
				case '\n':
				case '\r':
				case '\t':
					break;

				case '%':
					pent(' ');
					break;

				default:
					panic(FAULT);
				}
				break;

			case 5:		// read the default value
				switch (ch) {
				case '\"':
				case '\'':
					back();
					bqstr('d');	// the value in the mBuff now
					attdef.defaultDeclValue = new String(mBuff, 1, mBuffIdx);
					// here we should check defaultDeclValue validity
					
					st = -1;
					break;

				default:
					panic(FAULT);
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Parses a notation declaration.
	 *
	 * This method parses the declaration up to the closing angle 
	 * bracket.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void dtdnot()
		throws SAXException, IOException
// [82]	NotationDecl   ::=   	'<!NOTATION' S  Name  S (ExternalID | PublicID) S? '>'
// [83]	PublicID	   ::=   	'PUBLIC' S PubidLiteral
	{
		wsskip();
		String name = name(false);
		wsskip();
		DTD.ExternalID externalID = new DTD.ExternalID(); 
		pubsys('N', externalID);
		if( dtd.notations.containsKey(name) )
			panic(FAULT); // http://www.w3.org/TR/REC-xml/#UniqueNotationName
		dtd.notations.put(name, externalID);
		mHand.notationDecl(name, externalID.pubidLiteral, externalID.systemLiteral);
	}
	
	private void parseDocumentElement() throws SAXException, IOException {
		processTag(); // read document element
		short st = 0;
		char ch;
		while( elementStack.size() > 0 ) {
			mBuffIdx = -1;
			while( st >= 0 ) {
				ch = (mChIdx < mChLen)? mChars[mChIdx++]: next();
				switch (st) {
				case 0:		// read the end of the element tag
					switch (ch) {
					case '>':
						//		Report the element
						reportStartElement();
						st = 2;
						break;
					case '/':
						st = 1;
						break;
					default:
						panic(FAULT);
					}
					break;
				case 1:		// read the end of the empty element
					switch (ch) {
					case '>':
						//		Report the element
						reportStartElement();
						st = -1;
						break;
					default:
						panic(FAULT);
					}
					break;

				case 2:		// skip white space between tags
					switch (ch) {
					case ' ':
					case '\t':
					case '\n':
						bappend(ch);
						break;

					case '\r':		// EOL processing [#2.11]
						if (next() != '\n')
							back();
						bappend('\n');
						break;

					case '<':
						// Need revisit: With additional info from DTD and xml:space attr [#2.10]
						// the following call can be supported:
						// mHand.ignorableWhitespace(mBuff, 0, (mBuffIdx + 1));
						bflash();

					default:
						back();
						st = 3;
					}
					break;

				case 3:		// read the text content of the element
					switch (ch) {
					case '&':
						ent('x');
						break;

					case '<':
						bflash();
						st = 2;
						switch (next()) {
						case '/':	// the end of the element content
							//		Check element's open/close tags balance
							mBuffIdx = -1;
							bname(mIsNSAware);
							char[] elementQName = elementStack.top().elementName;
							if (elementQName.length != (mBuffIdx + 1))
								panic(FAULT);
							for (int i = 0; i < elementQName.length; i++) {
								if (elementQName[i] != mBuff[i])
									panic(FAULT);
							}
							//		Skip white spaces before '>'
							if (wsskip() != '>')
								panic(FAULT);
							ch = next(); // consume '>'
							st = -1;
							break;

						case '!':	// a comment or a CDATA
							ch = next();
							back();
							switch (ch) {
							case '-':	// must be a comment
								comm();
								break;

							case '[':	// must be a CDATA section
								cdat();
								break;

							default:
								panic(FAULT);
							}
							break;

						case '?':	// processing instruction
							pi();
							break;

						default:	// must be the first char of an xml name 
							back();
							processTag();
							st = 0;
						}
						mBuffIdx = -1;
						break;

					case '\r':		// EOL processing [#2.11]
						if (next() != '\n')
							back();
						bappend('\n');
						break;

					case EOS:
						panic(FAULT);

					case ' ':		// characters not supported by bappend()
					case '\"':
					case '\'':
					case '\n':
					case '\t':
					case '%':
						bappend(ch);
						break;

					default:
						bappend();
					}
					break;

				default:
					panic(FAULT);
				}
			}
			processTagClosing();
			st = 2;
		}
	}

	private void reportStartElement() throws SAXException {
		Element element = elementStack.top();
		if (mIsNSAware) {
			mHand.startElement( element.namespace.URI, QName.local(element.elementName),
						"", attributes);
		} else {
			mHand.startElement( "", "",
						QName.qname(element.elementName), attributes);
		}
	}

	/**
	 * Registers namespace with prefix. 
	 * 
	 * @param spaceName namespace prefix. Can be empty string. 
	 * @param URI namespace URI
	 * @throws SAXException 
	 */
	final private void pushNamespace( String spaceName, String URI ) throws SAXException {
		Element element = elementStack.top();
		if( namespaceStack.pushUnique( element.registeredNamespacesNumber, spaceName, URI ) ){
			element.registeredNamespacesNumber++;
			mHand.startPrefixMapping( spaceName, URI );
		}
	}
	
	private Element processTag() throws SAXException, IOException {
		// Read an element name and put it on top of the element stack
		elementStack.push( qname(mIsNSAware) );
		Element element = elementStack.top(); 
		// Read attributes till the end of the element tag
		parseAttributes();
		// process attributes: set attributes types, normalize attributes values
		// (http://www.w3.org/TR/REC-xml-names/#ns-using) 
		// Note that DTD-based validation is not namespace-aware in the following sense: 
		// a DTD constrains the elements and attributes that may appear in a document by 
		// their uninterpreted names, not by (namespace name, local name) pairs. To validate 
		// a document that uses namespaces against a DTD, the same prefixes must be used in 
		// the DTD as in the instance. A DTD may however indirectly constrain the namespaces 
		// used in a valid document by providing #FIXED values for attributes that declare
		// namespaces.
		Hashtable l = (Hashtable)dtd.attLists.get(QName.qname(element.elementName));
		if( l != null ){
			setAttrTypes( l );
			addDefaultAttributes( l );
		}
		if( mIsNSAware ){
			// resolve element namespace
			String errMsg = element.resolveNamespace( namespaceStack );
			if( errMsg != null )
				panic(errMsg);
			
			Hashtable ll = dtd.findAttList(element.namespace.URI, 
										QName.local(element.elementName));
			if( ll != null && ll != l ){
				setAttrTypes( ll );
				addDefaultAttributes( ll );
			}
		}
		
		// resolve attributes namespaces
		if( mIsNSAware ){
			for( int idx = attributes.getLength(); idx-- > 0; ){
				String errMsg = ((Attrs.NSAware)attributes).resolveNamespace(idx, namespaceStack);
				if( errMsg != null )
					panic(errMsg);
			}
		}
		
		// all attributes without type already have "CDATA" type
		
		// check attributes duplication
		if( attributes.hasDuplications() )
			panic(FAULT);
		
		/* org/xml/sax/ContentHandler.html#startElement(java.lang.String,%20java.lang.String,%20java.lang.String,%20org.xml.sax.Attributes)
		 * 
		 * The attribute list will contain attributes used for Namespace declarations 
		 * (xmlns* attributes) only if the http://xml.org/sax/features/namespace-prefixes  
		 * property is true (it is false by default, and support for a true value is 
		 * optional).
		 */
		if( mIsNSAware ){
			// NOTE: The namespace declaration should not be reported as an element attribute.
			for( int idx = attributes.getLength(); idx-- > 0; ){
				String qname = attributes.getQName(idx);
				if( XMLNSSTR.equals(qname) || qname.startsWith(XMLNSSTR + ':') ){
					attributes.remove(idx);
				}
			}
		}
		return element;
	}

	private void processTagClosing() throws SAXException {
		Element element = elementStack.pop(); 
		// Report the end of element
		if (mIsNSAware){
			mHand.endElement(element.namespace.URI, 
						QName.local(element.elementName), "");
			// Restore the top of the prefix stack
			if( DEBUG_OUT != null ) 
				DEBUG_OUT.println( QName.qname(element.elementName) + ".registeredNamespacesNumber = " + element.registeredNamespacesNumber );
			while( element.registeredNamespacesNumber-- > 0 ) {
				Namespace ns = namespaceStack.pop();
				mHand.endPrefixMapping(ns.id);
				ns.free();
			}
		} else mHand.endElement("", "", QName.qname(element.elementName));
		// Remove the top element tag
		element.free();
	}

	private void setAttrTypes(Hashtable attList) {
		for( int idx = attributes.getLength(); idx-- > 0; ){
			String qname = attributes.getQName(idx);
			DTD.AttDef def = (DTD.AttDef)attList.get( qname );
			if( def != null ){
				// sets type and convert value if type is 'CDATA'
				attributes.setType( idx, def.attType );
			}
		}
	}

	private void addDefaultAttributes(Hashtable attList) throws SAXException {
		if( DEBUG_OUT != null )
			DEBUG_OUT.println( "addDefaultAttributes" ); 
		for( Enumeration e = attList.keys(); e.hasMoreElements();){
			String qname = (String)e.nextElement();
			DTD.AttDef def = (DTD.AttDef)attList.get(qname);
			if( DEBUG_OUT != null ){
				DEBUG_OUT.println( "\tdefatt '" + qname + "' " + 
						def.attType + " " + def.defaultDeclType + 
						" \"" + def.defaultDeclValue + "\"" );
			}
			switch( def.defaultDeclType ){
				case DTD.AttDef.ddtIMPLIED: // #IMPLIED
					break;
				case DTD.AttDef.ddtREQUIRED: // #REQUIRED
					// our parser is not a validating parser
					// so we don't check presence of the attribute 
					break;
				default: // #FIXED & <default>
					if( attributes.getIndex(qname) == -1 ){
						// add attribute with the default value
						int idx = addAttribute(qname, def.defaultDeclValue);
						attributes.setType( idx, def.attType ); // sets type and convert value if type is 'CDATA'
					}
			}
		}
	}
	
	private int addAttribute(String attributeQName, String attrValue) throws SAXException {
		
		if( mIsNSAware ){
			String namespacePrefix = null;
			if( XMLNSSTR.equals(attributeQName) ){
				namespacePrefix = "";
			} else if( attributeQName.startsWith(XMLNSSTR + ':') ){
				namespacePrefix = attributeQName.substring(XMLNSSTR.length() + 1);
			}
			if( namespacePrefix != null ){
				/* here we register namespaces that are specified in DTD for the element */
				pushNamespace( namespacePrefix, attrValue );
			}
		}
		// save all attributes (xmlns too)
		return attributes.add( attributeQName, attrValue /*CDATA normalized*/ );
	}

	private void parseAttributes() throws SAXException, IOException {
		attributes.clean();
		boolean done = false;
		while( !done ){
			switch( wsskip() ){
				case '/': case '>':
					done = true;
					break;
					
				default:
					// Read the attribute name and value
					char[] attrName = qname(mIsNSAware);
					if( wsskip() != '=' )
						panic(FAULT);
					next(); // consume '='
					bqstr('c');	// read value with CDATA normalization.
					String attrValue = new String(mBuff, 1, mBuffIdx);
					addAttribute(QName.qname(attrName), attrValue);
			}
		}
		// all attributes have been read
	}

	/**
	 * Parses a comment.
	 *
	 * @exception org.xml.SAXException 
	 * @exception java.io.IOException 
	 */
	private void comm()
		throws SAXException, IOException
	{
		if (mSt == stateStartOfTheDocument)
			mSt	= stateMiscBeforeDTD;	// misc before DTD
		char	ch;
		for (short st = 0; st >= 0;) {
			ch = (mChIdx < mChLen)? mChars[mChIdx++]: next();
			switch (st) {
			case 0:		// first '-' of the comment open
				if (ch != '-')
					panic(FAULT);
				st = 1;
				break;

			case 1:		// secind '-' of the comment open
				if (ch != '-')
					panic(FAULT);
				st = 2;
				break;

			case 2:		// skip the comment body
				switch (ch) {
				case '-':
					st = 3;
					break;

				case EOS:
					panic(FAULT);

				default:
				}
				break;

			case 3:		// second '-' of the comment close
				st	= (ch == '-')? (short)4: (short)2;
				break;

			case 4:		// '>' of the comment close
				if (ch != '>')
					panic(FAULT);
				st = -1;
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Parses a processing instruction.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void pi()
		throws SAXException, IOException
	{
		char	ch;
		String	str	= null;
		mBuffIdx = -1;
		for (short st = 0; st >= 0;) {
			ch	= next();
			switch (st) {
			case 0:		// read the PI target name
				switch (chtyp(ch)) {
				case 'a':
				case 'A':
				case '_':
				case ':':
				case 'X':
					back();
					str	= name(false);
					//		PI target name may not be empty string [#2.6]
					//		PI target name 'XML' is reserved [#2.6]
					if ((str.length() == 0) || ("xml".equals(str.toLowerCase())))
						panic(FAULT);
					//		This is processing instruction
					if (mSt == stateStartOfTheDocument)	// the begining of the document
						mSt	= stateMiscBeforeDTD;	// misc before DTD
					wsskip();		// skip spaces after the PI target name
					st	= 1;		// accumulate the PI body
					mBuffIdx = -1;
					break;

				case 'Z':		// EOS
					panic(FAULT);
					break;

				default:
					panic(FAULT);
				}
				break;

			case 1:		// accumulate the PI body
				switch (ch) {
				case '?':
					st	= 2;	// end of the PI body
					break;

				case EOS:
					panic(FAULT);
					break;

				default:
					bappend(ch);
					break;
				}
				break;

			case 2:		// end of the PI body
				switch (ch) {
				case '>':
					//		PI has been read.
					mHand.processingInstruction(
						str, new String(mBuff, 0, mBuffIdx + 1));
					st	= -1;
					break;

				case '?':
					bappend('?');
					break;

				case EOS:
					panic(FAULT);
					break;

				default:
					bappend('?');
					bappend(ch);
					st	= 1;	// accumulate the PI body
					break;
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Parses a character data.
	 *
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void cdat()
		throws SAXException, IOException
	{
		char	ch;
		mBuffIdx = -1;
		for (short st = 0; st >= 0;) {
			ch	= next();
			switch (st) {
			case 0:		// the first '[' of the CDATA open
				if (ch == '[')
					st	= 1;
				else
					panic(FAULT);
				break;

			case 1:		// read "CDATA"
				if (chtyp(ch) == 'A') {
					bappend(ch);
				} else {
					if ("CDATA".equals(
							new String(mBuff, 0, mBuffIdx + 1)) != true)
						panic(FAULT);
					back();
					st	= 2;
				}
				break;

			case 2:		// the second '[' of the CDATA open
				if (ch != '[')
					panic(FAULT);
				mBuffIdx = -1;
				st	= 3;
				break;

			case 3:		// read data before the first ']'
				switch (ch) {
				case ']':
					st	= 4;
					break;

				case EOS:
					panic(FAULT);

				default:
					bappend(ch);
				}
				break;

			case 4:		// read the second ']' or continue to read the data
				if (ch != ']') {
					bappend(']');
					bappend(ch);
					st	= 3;
				} else {
					st	= 5;
				}
				break;

			case 5:		// read '>' or continue to read the data
				switch (ch) {
				case ']':
					bappend(']');
					break;

				case '>':
					bflash();
					st	= -1;
					break;

				default:
					bappend(']');
					bappend(']');
					bappend(ch);
					st	= 3;
					break;
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Reads a xml name.
	 *
	 * The xml name must conform "Namespaces in XML" specification. Therefore 
	 * the ':' character is not allowed in the name. This method should be 
	 * used for PI and entity names which may not have a namespace according 
	 * to the specification mentioned above.
	 *
	 * @param ns The true value turns namespace conformance on.
	 * @return The name has been read.
	 * @exception SAXException When incorrect character appear in the name.
	 * @exception IOException 
	 */
	private String name(boolean ns)
		throws SAXException, IOException
	{
		mBuffIdx = -1;
		bname(ns);
		return new String(mBuff, 1, mBuffIdx);
	}

	/**
	 * Reads a qualified xml name.
	 *
	 * The characters of a qualified name is an array of characters. The 
	 * first (chars[0]) character is the index of the colon character which 
	 * separates the prefix from the local name. If the index is zero, the 
	 * name does not contain separator or the parser works in the namespace 
	 * unaware mode. The length of qualified name is the length of the array 
	 * minus one. 
	 *
	 * @param ns The true value turns namespace conformance on.
	 * @return The characters of a qualified name.
	 * @exception SAXException  When incorrect character appear in the name.
	 * @exception IOException 
	 */
	private char[] qname(boolean ns)
		throws SAXException, IOException
	{
		mBuffIdx = -1;
		bname(ns);
		char chars[] = new char[mBuffIdx + 1];
		System.arraycopy(mBuff, 0, chars, 0, mBuffIdx + 1);
		return chars;
	}

	/**
	 * Reads the public or/and system identifiers.
	 *
	 * @param flag The 'N' allows public id be without system id.
	 * @return The public or/and system identifiers pair.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void pubsys( char flag, DTD.ExternalID eid ) throws SAXException, IOException
	{
		eid.pubidLiteral = eid.systemLiteral = null;
		String str	= name(false);
		if ("PUBLIC".equals(str)) {
			bqstr('i');		// non-CDATA normalization [#4.2.2]
			eid.pubidLiteral = new String(mBuff, 1, mBuffIdx);
			switch (wsskip()) {
			case '\"':
			case '\'':
				bqstr(' ');
				eid.systemLiteral = new String(mBuff, 1, mBuffIdx);
				break;

			default:
				if (flag != 'N')	// [#4.7]
					panic(FAULT);
			}
		} else if ("SYSTEM".equals(str)) {
			bqstr(' ');
			eid.systemLiteral = new String(mBuff, 1, mBuffIdx);
		} else panic(FAULT);
	}

	/**
	 * Reads an attribute value.
	 *
	 * The grammar which this method can read is:<br /> 
	 * <code>eqstr := S &quot;=&quot; qstr</code><br /> 
	 * <code>qstr  := S (&quot;'&quot; string &quot;'&quot;) | 
	 *	('&quot;' string '&quot;')</code><br /> 
	 * This method resolves entities inside a string unless the parser 
	 * parses DTD.
	 *
	 * @param flag The '=' character forces the method 
	 *	to accept the '=' character before quoted string.
	 * @return The name has been read.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private String eqstr(char flag)
		throws SAXException, IOException
	{
		if (flag == '=') {
			wsskip();
			if (next() != '=')
				panic(FAULT);
		}
		bqstr('-');
		return new String(mBuff, 1, mBuffIdx);
	}

	/*
	// #2.2
	// [2] Char	::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
	final private int[] legalCharRangeStart = new int[] {0x9, 0xA, 0xD, 0x20, 0xE000, 0x10000};
	final private int[] legalCharRangeEnd = new int[] {0x9, 0xA, 0xD, 0xD7FF, 0xFFFD, 0x10FFFF};
	*/
	/**
	 * Checks and convert codePoint (came from a character reference) into UTF-16 characters
	 * @param codePoint 
	 * @return value as character
	 * @throws SAXException
	 */
	private void codePointToBuffer( int codePoint, char flag ) throws SAXException
	{
		/*
		// find index in legalCharRangeStart of value that is less than 'value' (binary search)
		int head = 0, tail = legalCharRangeStart.length;
		while( head < tail ){
			int pos = (head + tail) / 2;
			if( legalCharRangeStart[pos] <= value ) head = pos + 1;
			else tail = pos;
		}
		if( !(head > 0 && value <= legalCharRangeEnd[head - 1]) )
			panic(FAULT); // "invalid character reference"
		*/
		if( codePoint < 0 )
			panic(FAULT); // impossible (superfluous check)
	    if (codePoint < 0x10000) {
	        if (0xd7FF < codePoint && codePoint < 0xe000)
	            panic(FAULT); // invalid code point
	        char ch = (char)codePoint;
	        // code is copied from its original place (ent(int flag))
			if (ch == ' ' || mInp.next != null) 
				bappend(ch, flag);
			else bappend(ch);
	    } else {
	        if (codePoint > 0x10FFFF)
	            panic(FAULT); // invalid code point
	    	
	        int offset = codePoint - 0x10000;
	        bappend((char)((offset >> 10) + 0xd800));
	        bappend((char)((offset & 0x3ff) + 0xdc00));
	    }
	}

	/**
	 * Resoves an entity.
	 *
	 * This method resolves built-in and character entity references. It is 
	 * also reports external entities to the application.
	 *
	 * @param flag The 'x' character forces the method to report a skipped entity;
	 *	'i' character - indicates non-CDATA normalization.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void ent(char flag)
		throws SAXException, IOException
	{
		char	ch;
		int		idx	= mBuffIdx + 1;
		Input	inp = null;
		String	str	= null;
		mESt = 0x100;	// reset the built-in entity recognizer
		bappend('&');
		for (short st = 0; st >= 0;) {
			ch = (mChIdx < mChLen)? mChars[mChIdx++]: next();
			switch (st) {
			case 0:		// the first character of the entity name
			case 1:		// read built-in entity name
				switch (chtyp(ch)) {
				case 'd':
				case '.':
				case '-':
					if (st != 1)
						panic(FAULT);
				case 'a':
				case 'A':
				case '_':
				case 'X':
					bappend(ch);
					eappend(ch);
					st	= 1;
					break;

				case ':':
					if (mIsNSAware != false)
						panic(FAULT);
					bappend(ch);
					eappend(ch);
					st	= 1;
					break;

				case ';':
					if (mESt < 0x100) {
						//		The entity is a built-in entity
						mBuffIdx = idx - 1;
						bappend(mESt);
						st	= -1;
						break;
					} else if (mSt == stateInsideDTD) {
						//		In DTD entity declaration has to resolve character 
						//		entities and include "as is" others. [#4.4.7]
						bappend(';');
						st	= -1;
						break;
					}
					//		Convert an entity name to a string
					str	= new String(mBuff, idx + 1, mBuffIdx - idx);
					inp	= (dtd.generalEntities == null)? null : (Input)dtd.generalEntities.get(str);
					//		Restore the buffer offset
					mBuffIdx = idx - 1;
					if (inp != null) {
						if (inp.chars == null) {
							//		External entity
							InputSource is = mHand.resolveEntity(inp.pubid, inp.sysid);
							if (is != null) {
								push(new Input(BUFFSIZE_READER));
								setinp(is);
								mInp.pubid = inp.pubid;
								mInp.sysid = inp.sysid;
							} else {
								//		Unresolved external entity
								bflash();
								if (flag != 'x')
									panic(FAULT);	// unknown entity within markup
								mHand.skippedEntity(str);
							}
						} else {
							//		Internal entity
							push(inp);
						}
					} else {
						//		Unknown or general unparsed entity
						bflash();
						if (flag != 'x')
							panic(FAULT);	// unknown entity within markup
						mHand.skippedEntity(str);
					}
					st	= -1;
					break;

				case '#':
					if (st != 0)
						panic(FAULT);
					st	= 2;
					break;

				default:
					panic(FAULT);
				}
				break;

			case 2:		// read character entity
				switch (chtyp(ch)) {
				case 'd':
					bappend(ch);
					break;

				case ';':
					//		Convert the character entity to a character
					try {
						int i = Integer.parseInt(
							new String(mBuff, idx + 1, mBuffIdx - idx), 10);
						// Restore the buffer offset
						mBuffIdx = idx - 1;
						codePointToBuffer(i, flag);
					} catch (NumberFormatException nfe) {
						panic(FAULT);
					}
					st	= -1;
					break;

				case 'a':
					//		If the entity buffer is empty and ch == 'x'
					if ((mBuffIdx == idx) && (ch == 'x')) {
						st	= 3;
						break;
					}
				default:
					panic(FAULT);
				}
				break;

			case 3:		// read hex character entity
				switch (chtyp(ch)) {
				case 'A':
				case 'a':
				case 'd':
					bappend(ch);
					break;

				case ';':
					//		Convert the character entity to a character
					try {
						int i = Integer.parseInt(
							new String(mBuff, idx + 1, mBuffIdx - idx), 16);
						// Restore the buffer offset
						mBuffIdx = idx - 1;
						codePointToBuffer(i, flag);
					} catch (NumberFormatException nfe) {
						panic(FAULT);
					}
					st	= -1;
					break;

				default:
					panic(FAULT);
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Resoves a parameter entity.
	 *
	 * This method resolves a parameter entity references. It is also reports 
	 * external entities to the application.
	 *
	 * @param flag The '-' instruct the method to do not set up surrounding 
	 *	spaces [#4.4.8].
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void pent(char flag)
		throws SAXException, IOException
	{
		int		idx	= mBuffIdx + 1;
		Input	inp = null;
		String	str	= null;
		bappend('%');
		if (mSt != stateInsideDTD)		// the DTD internal subset
			return;			// Not Recognized [#4.4.1]
		//		Read entity name
		bname(false);
		str = new String(mBuff, idx + 2, mBuffIdx - idx - 1);
		if (next() != ';')
			panic(FAULT);
		inp	= (dtd.parameterEntities == null)? null : (Input)dtd.parameterEntities.get(str);
		//		Restore the buffer offset
		mBuffIdx = idx - 1;
		if (inp != null) {
			if (inp.chars == null) {
				//		External parameter entity
				InputSource is = mHand.resolveEntity(inp.pubid, inp.sysid);
				if (is != null) {
					if (flag != '-')
						bappend(' ');	// tail space
					push(new Input(BUFFSIZE_READER));
					// Need revisit: there is no leading space! [#4.4.8]
					setinp(is);
					mInp.pubid = inp.pubid;
					mInp.sysid = inp.sysid;
				} else {
					//		Unresolved external parameter entity
					mHand.skippedEntity("%" + str);
				}
			} else {
				//		Internal parameter entity
				if (flag == '-') {
					//		No surrounding spaces
					inp.chIdx = 1;
				} else {
					//		Insert surrounding spaces
					bappend(' ');	// tail space
					inp.chIdx = 0;
				}
				push(inp);
			}
		} else {
			//		Unknown parameter entity
			mHand.skippedEntity("%" + str);
		}
	}

	/**
	 * Skips xml white space characters.
	 *
	 * This method skips white space characters (' ', '\t', '\n', '\r') and 
	 * looks ahead not white space character. 
	 *
	 * @return The first not white space look ahead character.
	 * @exception SAXException When End Of Stream character typed.
	 * @exception IOException 
	 */
	private char wsskip()
		throws SAXException, IOException
	{
		char	ch;
		char	type;
		while (true) {
			//		Read next character
			ch = (mChIdx < mChLen)? mChars[mChIdx++]: next();
			type = (char)0;	// [X]
			if (ch < 0x80) {
				type = (char)nmttyp[ch];
			} else if (ch == EOS) {
				panic(FAULT);
			}
			if (type != 3) {	// [ \t\n\r]
				mChIdx--;	// back();
				return ch;
			}
		}
	}

	/**
	 * Notifies the handler about fatal parsing error.
	 *
	 * @param msg The problem description message.
	 */
	private void panic(String msg) throws SAXException
	{
		final boolean debug_mode = false;
		if( debug_mode ){
			msg += "\n";
			try {
				char ch;
				for( int i = 0; i < 30 && (ch=next()) != EOS; i++) msg += ch;
			} catch(Exception x){}
		}
		SAXParseException spe = new SAXParseException(msg, this);
		mHand.fatalError(spe);
		throw spe;	// [#1.2] fatal error definition
	}

	/**
	 * Reads a qualified xml name. 
	 *
	 * This is low level routine which leaves a qName in the buffer.
	 * The characters of a qualified name is an array of characters. The 
	 * first (chars[0]) character is the index of the colon character which 
	 * separates the prefix from the local name. If the index is zero, the 
	 * name does not contain separator or the parser works in the namespace 
	 * unaware mode. The length of qualified name is the length of the array 
	 * minus one. 
	 *
	 * @param ns The true value turns namespace conformance on.
	 * @exception SAXException  When incorrect character appear in the name.
	 * @exception IOException 
	 */
	private void bname(boolean ns) throws SAXException, IOException
	{
		char	ch;
		char	type;
		mBuffIdx++;		// allocate a char for colon offset
		int		bqname	= mBuffIdx;
		int		bcolon	= bqname;
		int		bchidx	= bqname + 1;
		int		bstart	= bchidx;
		int		cstart	= mChIdx;
		short	st		= (short)(ns? 0: 2);
		while (true) {
			//		Read next character
			if (mChIdx >= mChLen) {
				bcopy(cstart, bstart);
				next();
				mChIdx--;	// back();
				cstart = mChIdx;
				bstart = bchidx;
			}
			ch = mChars[mChIdx++];
			if (ch == EOS)
				panic(FAULT);
			type = (char)0;	// [X]
			if (ch < 0x80) {
				type = (char)nmttyp[ch];
			}
			//		Parse QName
			switch (st) {
			case 0:		// read the first char of the prefix
			case 2:		// read the first char of the suffix
				switch (type) {
				case 0:	// [aA_X]
					bchidx++;	// append char to the buffer
					st++;		// (st == 0)? 1: 3;
					break;

				case 1:	// [:]
					mChIdx--;	// back();
					st++;		// (st == 0)? 1: 3;
					break;

				default:
					panic(FAULT);
				}
				break;

			case 1:		// read the prefix
			case 3:		// read the suffix
				switch (type) {
				case 0:	// [aA_X]
				case 2: // [.-d]
					bchidx++;	// append char to the buffer
					break;

				case 1:	// [:]
					bchidx++;	// append char to the buffer
					if( ns ) {
						if (bcolon != bqname)
							panic(FAULT);	// it must be only one colon
						bcolon = bchidx - 1;
						if (st == 1)
							st = 2;
					}
					break;

				default:
					mChIdx--;	// back();
					bcopy(cstart, bstart);
					mBuff[bqname] = (char)(bcolon - bqname);
					if( DEBUG_OUT != null )
						DEBUG_OUT.println( "bname '" + new String(mBuff, 1, mBuffIdx) + "'" );
					return;
				}
				break;

			default:
				panic(FAULT);
			}
		}
	}

	/**
	 * Reads a nmtoken. 
	 *
	 * This is low level routine which leaves a nmtoken in the buffer.
	 *
	 * @exception SAXException  When incorrect character appear in the name.
	 * @exception IOException 
	 */
	private void bntok() throws SAXException, IOException
	{
		char	ch;
		mBuffIdx = -1;
		bappend((char)0);	// default offset to the colon char
		while (true) {
			ch	= next();
			switch (chtyp(ch)) {
			case 'a':
			case 'A':
			case 'd':
			case '.':
			case ':':
			case '-':
			case '_':
			case 'X':
				bappend(ch);
				break;

			default:
				back();
				if( mBuffIdx == 0 ) // zero length nmtoken
					panic(FAULT);
				if( DEBUG_OUT != null )
					DEBUG_OUT.println( "bntok '" + new String(mBuff, 1, mBuffIdx) + "'" );
				return;
			}
		}
	}

	/**
	 * Recognizes a keyword. 
	 *
	 * This is low level routine which recognizes one of keywords in the buffer.
	 * Keyword     Id
	 *  ID       - i
	 *  IDREF    - r
	 *  IDREFS   - R
	 *  ENTITY   - n
	 *  ENTITIES - N
	 *  NMTOKEN  - t
	 *  NMTOKENS - T
	 *  ELEMENT  - e
	 *  ATTLIST  - a
	 *  NOTATION - o
	 *  CDATA    - c
	 *  REQUIRED - Q
	 *  IMPLIED  - I
	 *  FIXED    - F
	 *  EMPTY	 - E
	 *  ANY		 - A
	 *  PCDATA	 - p
	 *
	 * @return an id of a keyword or '?'.
	 */
	private char bkeyword()
	{
		String str = new String(mBuff, 1, mBuffIdx);
		switch (str.length()) {
		case 2:	// ID
			return ("ID".equals(str) == true)? 'i': '?';

		case 3: // ANY
			return "ANY".equals(str)? 'A': '?';
			
		case 5:	// IDREF, CDATA, FIXED, EMPTY
			switch (mBuff[1]) {
			case 'I':
				return ("IDREF".equals(str) == true)? 'r': '?';
			case 'C':
				return ("CDATA".equals(str) == true)? 'c': '?';
			case 'F':
				return ("FIXED".equals(str) == true)? 'F': '?';
			case 'E':
				return "EMPTY".equals(str)? 'E': '?';
			default:
				break;
			}
			break;

		case 6:	// IDREFS, ENTITY, PCDATA
			switch (mBuff[1]) {
			case 'I':
				return ("IDREFS".equals(str) == true)? 'R': '?';
			case 'E':
				return ("ENTITY".equals(str) == true)? 'n': '?';
			case 'P':
				return "PCDATA".equals(str)? 'p': '?';
			default:
				break;
			}
			break;

		case 7:	// NMTOKEN, IMPLIED, ATTLIST, ELEMENT
			switch (mBuff[1]) {
			case 'I':
				return ("IMPLIED".equals(str) == true)? 'I': '?';
			case 'N':
				return ("NMTOKEN".equals(str) == true)? 't': '?';
			case 'A':
				return ("ATTLIST".equals(str) == true)? 'a': '?';
			case 'E':
				return ("ELEMENT".equals(str) == true)? 'e': '?';
			default:
				break;
			}
			break;

		case 8:	// ENTITIES, NMTOKENS, NOTATION, REQUIRED
			switch (mBuff[2]) {
			case 'N':
				return ("ENTITIES".equals(str) == true)? 'N': '?';
			case 'M':
				return ("NMTOKENS".equals(str) == true)? 'T': '?';
			case 'O':
				return ("NOTATION".equals(str) == true)? 'o': '?';
			case 'E':
				return ("REQUIRED".equals(str) == true)? 'Q': '?';
			default:
				break;
			}
			break;

		default:
			break;
		}
		return '?';
	}

	/**
	 * Reads a single or double quotted string into the buffer.
	 *
	 * This method resolves entities inside a string unless the parser 
	 * parses DTD.
	 *
	 * @param flag 'c' - CDATA, 'i' - non CDATA, ' ' - no normalization; 
	 *	'-' - not an attribute value; 'd' - in DTD context.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private void bqstr(char flag) throws SAXException, IOException
	{
		Input inp = mInp;	// remember the original input
		mBuffIdx  = -1;
		bappend((char)0);	// default offset to the colon char
		char ch;
		wsskip();
		for (short st = 0; st >= 0;) {
			ch = (mChIdx < mChLen)? mChars[mChIdx++]: next();
			switch (st) {
			case 0:		// read a single or double quote
				switch (ch) {
				case '\'':
					st = 2;	// read a single quoted string
					break;

				case '\"':
					st = 3;	// read a double quoted string
					break;

				default:
					panic(FAULT);
				}
				break;

			case 2:		// read a single quoted string
			case 3:		// read a double quoted string
				switch (ch) {
				case '\'':
					if ((st == 2) && (mInp == inp))
						st = -1;
					else
						bappend(ch);
					break;

				case '\"':
					if ((st == 3) && (mInp == inp))
						st = -1;
					else
						bappend(ch);
					break;

				case '&':
					if (flag != 'd')
						ent(flag);
					else
						bappend(ch);
					break;

				case '%':
					if (flag == 'd')
						pent('-');
					else
						bappend(ch);
					break;

				case '<':
					if ((flag == '-') || (flag == 'd'))
						bappend(ch);
					else
						panic(FAULT);
					break;

				case EOS:		// EOS before single/double quote
					panic(FAULT);

				case '\r':		// EOL processing [#2.11 & #3.3.3]
					if (flag != ' ' && mInp.next == null) {
						if (next() != '\n')
							back();
						ch = '\n';
					}
				case ' ':
				case '\n':
				case '\t':
					bappend(ch, flag);
					break;

				default:
					bappend();
				}
				break;

			default:
				panic(FAULT);
			}
		}
		//		There is maximum one space at the end of the string in
		//		i-mode (non CDATA normalization) and it has to be removed.
		if ((flag == 'i') && (mBuff[mBuffIdx] == ' '))
			mBuffIdx -= 1;
		
		if( DEBUG_OUT != null )
			DEBUG_OUT.println( "bqstr '" + new String(mBuff, 1, mBuffIdx) + "'" );
	}

	/**
	 * Reports characters and empties the parser's buffer.
	 */
	private void bflash() throws SAXException {
		if (mBuffIdx >= 0) {
			//		Textual data has been read
			mHand.characters(mBuff, 0, mBuffIdx + 1);
			mBuffIdx = -1;
		}
	}

	/**
	 * Appends a characters to parser's buffer starting with the last 
	 * read character and until one of special characters.
	 */
	private void bappend() throws SAXException, IOException {
		char ch;

		back();
		while (true) {
			ch = (mChIdx < mChLen)? mChars[mChIdx++]: next();
			switch (ch) {
			case ' ':
			case '\"':
			case '\'':
			case '\n':
			case '\r':
			case '\t':
			case '<':
			case '%':
			case '&':
			case EOS:
				back();
				return;

			default:
				mBuffIdx++;
				if (mBuffIdx < mBuff.length) {
					mBuff[mBuffIdx] = ch;
				} else {
					mBuffIdx--;
					bappend(ch);
				}
				break;
			}
		}
	}

	/**
	 * Appends a character to parser's buffer with normalization.
	 *
	 * @param ch The character to append to the buffer.
	 * @param mode The normalization mode.
	 */
	private void bappend(char ch, char mode)
	{
		//		This implements attribute value normalization as 
		//		described in the XML specification [#3.3.3].
		switch (mode) {
		case 'i':	// non CDATA normalization
			switch (ch) {
			case ' ':
			case '\n':
			case '\r':
			case '\t':
				if ((mBuffIdx > 0) && (mBuff[mBuffIdx] != ' '))
					bappend(' ');
				return;

			default:
				break;
			}
			break;

		case 'c':	// CDATA normalization
			switch (ch) {
			case '\n':
			case '\r':
			case '\t':
				ch = ' ';
				break;

			default:
				break;
			}
			break;

		default:	// no normalization
			break;
		}
		mBuffIdx++;
		if (mBuffIdx < mBuff.length) {
			mBuff[mBuffIdx] = ch;
		} else {
			mBuffIdx--;
			bappend(ch);
		}
	}

	/**
	 * Appends a character to parser's buffer.
	 *
	 * @param ch The character to append to the buffer.
	 */
	private void bappend(char ch)
	{
		try {
			mBuff[++mBuffIdx] = ch;
		} catch (Exception exp) {
			//		Double the buffer size
			char buff[] = new char[mBuff.length << 1];
			System.arraycopy(mBuff, 0, buff, 0, mBuff.length);
			mBuff			= buff;
			mBuff[mBuffIdx]	= ch;
		}
	}

	/**
	 * Appends (mChIdx - cidx) characters from character buffer (mChars) to 
	 * parser's buffer (mBuff).
	 *
	 * @param cidx The character buffer (mChars) start index.
	 * @param bidx The parser buffer (mBuff) start index.
	 */
	private void bcopy(int cidx, int bidx)
	{
		int length = mChIdx - cidx;
		if ((bidx + length + 1) >= mBuff.length) {
			//		Expand the buffer
			char buff[]	= new char[mBuff.length + length];
			System.arraycopy(mBuff, 0, buff, 0, mBuff.length);
			mBuff		= buff;
		}
		System.arraycopy(mChars, cidx, mBuff, bidx, length);
		mBuffIdx += length;
	}

	/**
	 * Recognizes the built-in entities <i>lt</i>, <i>gt</i>, <i>amp</i>, 
	 * <i>apos</i>, <i>quot</i>. 
	 * The initial state is 0x100. Any state belowe 0x100 is a built-in 
	 * entity replacement character. 
	 *
	 * @param ch the next character of an entity name.
	 */
	private void eappend(char ch)
	{
		switch (mESt) {
		case 0x100:	// "l" or "g" or "a" or "q"
			switch (ch) {
			case 'l':	mESt	= 0x101; break;
			case 'g':	mESt	= 0x102; break;
			case 'a':	mESt	= 0x103; break;
			case 'q':	mESt	= 0x107; break;
			default:	mESt	= 0x200; break;
			}
			break;
		case 0x101:	// "lt"
			mESt = (ch == 't')? '<': (char)0x200;
			break;
		case 0x102:	// "gt"
			mESt = (ch == 't')? '>': (char)0x200;
			break;
		case 0x103:	// "am" or "ap"
			switch (ch) {
			case 'm':	mESt	= 0x104; break;
			case 'p':	mESt	= 0x105; break;
			default:	mESt	= 0x200; break;
			}
			break;
		case 0x104:	// "amp"
			mESt = (ch == 'p')? '&': (char)0x200;
			break;
		case 0x105:	// "apo"
			mESt = (ch == 'o')? (char)0x106: (char)0x200;
			break;
		case 0x106:	// "apos"
			mESt = (ch == 's')? '\'': (char)0x200;
			break;
		case 0x107:	// "qu"
			mESt = (ch == 'u')? (char)0x108: (char)0x200;
			break;
		case 0x108:	// "quo"
			mESt = (ch == 'o')? (char)0x109: (char)0x200;
			break;
		case 0x109:	// "quot"
			mESt = (ch == 't')? '\"': (char)0x200;
			break;
		case '<':	// "lt"
		case '>':	// "gt"
		case '&':	// "amp"
		case '\'':	// "apos"
		case '\"':	// "quot"
			mESt	= 0x200;
		default:
			break;
		}
	}

	/**
	 * Sets up a new input source on the top of the input stack.
	 * Note, the first byte returned by the entity's byte stream has to be the 
	 * first byte in the entity. However, the parser does not expect the byte 
	 * order mask in both cases when encoding is provided by the input source.
	 *
	 * @param is A new input source to set up.
	 * @exception IOException If any IO errors occur.
	 * @exception SAXException If the input source cannot be read.
	 */
	private void setinp(InputSource is)
		throws SAXException, IOException
	{
		Reader reader = null;
		mChIdx   = 0;
		mChLen   = 0;
		mChars   = mInp.chars;
		mInp.src = null;
		if (mSt == stateStartOfTheDocument)
			mIsSAlone	= false;	// default [#2.9]
		if (is.getCharacterStream() != null) {
			//		Ignore encoding in the xml text decl. 
			reader = is.getCharacterStream();
			xml(reader);
		} else if (is.getByteStream() != null) {
			String expenc;
			if (is.getEncoding() != null) {
				//		Ignore encoding in the xml text decl.
				expenc = is.getEncoding().toUpperCase();
				if (expenc.equals("UTF-16"))
					reader = bom(is.getByteStream(), 'U');	// UTF-16 [#4.3.3]
				else
					reader = enc(expenc, is.getByteStream());
				xml(reader);
			} else {
				//		Get encoding from BOM or the xml text decl.
				reader = bom(is.getByteStream(), ' ');
				if (reader == null) {
					//		Encoding is defined by the xml text decl.
					reader = enc("UTF-8", is.getByteStream());
					expenc = xml(reader);
					if (expenc.startsWith("UTF-16"))
						panic(FAULT);	// UTF-16 must have BOM [#4.3.3]
					reader = enc(expenc, is.getByteStream());
				} else {
					//		Encoding is defined by the BOM.
					xml(reader);
				}
			}
		} else {
			//		There is no support for public/system identifiers.
			panic(FAULT);
		}
		mInp.src   = reader;
		mInp.pubid = is.getPublicId();
		mInp.sysid = is.getSystemId();
	}

	/**
	 * Determines the entity encoding.
	 *
	 * This method gets encoding from Byte Order Mask [#4.3.3] if any. 
	 * Note, the first byte returned by the entity's byte stream has 
	 * to be the first byte in the entity. Also, there is no support 
	 * for UCS-4.
	 *
	 * @param is A byte stream of the entity.
	 * @param hint An encoding hint, character U means UTF-16.
	 * @return a reader constructed from the BOM or UTF-8 by default.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private Reader bom(InputStream is, char hint)
		throws SAXException, IOException
	{
		int val = is.read();
		switch (val) {
		case 0xef:		// UTF-8
			if (hint == 'U')	// must be UTF-16
				panic(FAULT);
			if (is.read() != 0xbb) 
				panic(FAULT);
			if (is.read() != 0xbf) 
				panic(FAULT);
			return new ReaderUTF8(is);

		case 0xfe:		// UTF-16, big-endian
			if (is.read() != 0xff) 
				panic(FAULT);
			return new ReaderUTF16(is, 'b');

		case 0xff:		// UTF-16, little-endian
			if (is.read() != 0xfe) 
				panic(FAULT);
			return new ReaderUTF16(is, 'l');

		case -1:
			mChars[mChIdx++] = EOS;
			return new ReaderUTF8(is);

		default:
			if (hint == 'U')	// must be UTF-16
				panic(FAULT);
			//		Read the rest of UTF-8 character
			switch (val & 0xf0) {
			case 0xc0:
			case 0xd0:
				mChars[mChIdx++] = (char)(((val & 0x1f) << 6) | (is.read() & 0x3f));
				break;

			case 0xe0:
				mChars[mChIdx++] = (char)(((val & 0x0f) << 12) | 
					((is.read() & 0x3f) << 6) | (is.read() & 0x3f));
				break;

			case 0xf0:	// UCS-4 character
				throw new UnsupportedEncodingException();

			default:
				mChars[mChIdx++] = (char)val;
				break;
			}
			return null;
		}
	}

	/**
	 * Parses the xml text declaration.
	 *
	 * This method gets encoding from the xml text declaration [#4.3.1] if any. 
	 * The method assumes the buffer (mChars) is big enough to accomodate whole 
	 * xml text declaration.
	 *
	 * @param reader is entity reader.
	 * @return The xml text declaration encoding or default UTF-8 encoding.
	 * @exception SAXException 
	 * @exception IOException 
	 */
	private String xml(Reader reader)
		throws SAXException, IOException
	{
		String	str	= null;
		String	enc	= "UTF-8";
		char	ch;
		int		val;
		short	st;
		//		Read the xml text declaration into the buffer
		if (mChIdx != 0) {
			//		The bom method have read ONE char into the buffer. 
			st = (short)((mChars[0] == '<')? 1: -1);
		} else {
			st = 0;
		}
		while (st >= 0 && mChIdx < mChars.length) {
			ch = ((val = reader.read()) >= 0)? (char)val: EOS;
			mChars[mChIdx++] = ch;
			switch (st) {
			case 0:		// read '<' of xml declaration
				switch (ch) {
				case '<':
					st = 1;
					break;

				case 0xfeff:	// the byte order mask
					ch = ((val = reader.read()) >= 0)? (char)val: EOS;
					mChars[mChIdx - 1] = ch;
					st = (short)((ch == '<')? 1: -1);
					break;

				default:
					st = -1;
					break;
				}
				break;

			case 1:		// read '?' of xml declaration [#4.3.1]
				st = (short)((ch == '?')? 2: -1);
				break;

			case 2:		// read 'x' of xml declaration [#4.3.1]
				st = (short)((ch == 'x')? 3: -1);
				break;

			case 3:		// read 'm' of xml declaration [#4.3.1]
				st = (short)((ch == 'm')? 4: -1);
				break;

			case 4:		// read 'l' of xml declaration [#4.3.1]
				st = (short)((ch == 'l')? 5: -1);
				break;

			case 5:		// read white space after 'xml'
				switch (ch) {
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					st = 6;
					break;

				default:
					st = -1;
					break;
				}
				break;

			case 6:		// read content of xml declaration
				switch (ch) {
				case '?':
					st = 7;
					break;

				case EOS:
					st = -2;
					break;

				default:
					break;
				}
				break;

			case 7:		// read '>' after '?' of xml declaration
				switch (ch) {
				case '>':
				case EOS:
					st = -2;
					break;

				default:
					st = 6;
					break;
				}
				break;

			default:
				panic(FAULT);
				break;
			}
		}
		mChLen = mChIdx;
		mChIdx = 0;
		//		If there is no xml text declaration, the encoding is default.
		if (st == -1) {
			return enc;
		}
		mChIdx = 5;		// the first white space after "<?xml"
		//		Parse the xml text declaration
		for (st = 0; st >= 0;) {
			ch = next();
			switch (st) {
			case 0:		// skip spaces after the xml declaration name
				if (chtyp(ch) != ' ') {
					back();
					st = 1;
				}
				break;

			case 1:		// read xml declaration version
			case 2:		// read xml declaration encoding or standalone
			case 3:		// read xml declaration standalone
				switch (chtyp(ch)) {
				case 'a':
				case 'A':
				case '_':
					back();
					str	= name(false).toLowerCase();
					if ("version".equals(str) == true) {
						if (st != 1)
							panic(FAULT);
						if ("1.0".equals(eqstr('=')) != true)
							panic(FAULT);
						st = 2;
					} else if ("encoding".equals(str) == true) {
						if (st != 2)
							panic(FAULT);
						enc = eqstr('=').toUpperCase();
						st  = 3;
					} else if ("standalone".equals(str) == true) {
						if ((st == 1) || (mSt != stateStartOfTheDocument))	// [#4.3.1]
							panic(FAULT);
						str = eqstr('=').toLowerCase();
						//		Check the 'standalone' value and use it 
						if (str.equals("yes") == true) {
							mIsSAlone = true;
						} else if (str.equals("no") == true) {
							mIsSAlone = false;
						} else {
							panic(FAULT);
						}
						st  = 4;
					} else {
						panic(FAULT);
					}
					break;

				case ' ':
					break;

				case '?':
					if (st == 1)
						panic(FAULT);
					back();
					st = 4;
					break;

				default:
					panic(FAULT);
				}
				break;

			case 4:		// end of xml declaration
				switch (chtyp(ch)) {
				case '?':
					if (next() != '>')
						panic(FAULT);
					if (mSt == stateStartOfTheDocument)		// the begining of the document
						mSt	= stateMiscBeforeDTD;		// misc before DTD
					st = -1;
					break;

				case ' ':
					break;

				default:
					panic(FAULT);
				}
				break;

			default:
				panic(FAULT);
			}
		}
		return enc;
	}

	/**
	 * Sets up the document reader.
	 *
	 * @param name an encoding name.
	 * @param is the document byte input stream.
	 * @return a reader constructed from encoding name and input stream.
	 * @exception UnsupportedEncodingException 
	 */
	private Reader enc(String name, InputStream is)
		throws java.io.UnsupportedEncodingException
	{
		//		DO NOT CLOSE current reader if any! 
		if (name.equals("UTF-8"))
			return new ReaderUTF8(is);
		else if (name.equals("UTF-16LE"))
			return new ReaderUTF16(is, 'l');
		else if (name.equals("UTF-16BE"))
			return new ReaderUTF16(is, 'b');
		else
			return new InputStreamReader(is, name);
	}

	/**
	 * Sets up current input on the top of the input stack.
	 *
	 * @param inp A new input to set up.
	 */
	private void push(Input inp)
	{
		mInp.chLen	= mChLen;
		mInp.chIdx	= mChIdx;
		inp.next	= mInp;
		mInp		= inp;
		mChars		= inp.chars;
		mChLen		= inp.chLen;
		mChIdx		= inp.chIdx;
	}

	/**
	 * Restores previous input on the top of the input stack.
	 */
	private void pop()
	{
		if (mInp.src != null) {
			try { mInp.src.close(); } catch (IOException ioe) {}
			mInp.src = null;
		}
		mInp	= mInp.next;
		if (mInp != null) {
			mChars	= mInp.chars;
			mChLen	= mInp.chLen;
			mChIdx	= mInp.chIdx;
		} else {
			mChars	= null;
			mChLen	= 0;
			mChIdx	= 0;
		}
	}

	/**
	 * Maps a character to it's type.
	 *
	 * Possible character type values are:<br /> 
	 * - ' ' for any kind of white space character;<br /> 
	 * - 'a' for any lower case alphabetical character value;<br /> 
	 * - 'A' for any upper case alphabetical character value;<br /> 
	 * - 'd' for any decimal digit character value;<br /> 
	 * - 'z' for any character less then ' ' except 
	 * '\t', '\n', '\r';<br /> 
	 * - 'X' for any not ASCII character;<br /> 
	 * - 'Z' for EOS character.<br /> 
	 * An ASCII (7 bit) character which does not fall in any category listed 
	 * above is mapped to it self. 
	 *
	 * @param ch The character to map.
	 * @return The type of character.
	 */
	private char chtyp(char ch)
	{
		if (ch < 0x80)
			return (char)asctyp[ch];
		return (ch != EOS)? 'X': 'Z';
	}

	/**
	 * Retrives the next character in the document.
	 *
	 * @return The next character in the document.
	 */
	private char next()
		throws java.io.IOException
	{
		if (mChIdx >= mChLen) {
			if (mInp.src == null) {
				pop();		// remove internal entity
				return next();
			}
			//		Read new portion of the document characters
			int Num = mInp.src.read(mChars, 0, mChars.length);
			if (Num < 0) {
				if (mInp != mDoc) {
					pop();	// restore the previous input
					return next();
				}
				mChars[0] = EOS;
				mChLen    = 1;
			} else mChLen = Num;
			mChIdx = 0;
		}
		return mChars[mChIdx++];
	}

	/**
	 * Puts back the last read character.
	 *
	 * This method <strong>MUST NOT</strong> be called more then once after 
	 * each call of {@link #next next} method.
	 */
	private void back()
		throws SAXException
	{
		if(mChIdx <= 0)
			panic(FAULT); // internal error
		mChIdx--;
	}

	/**
	 * Sets the current character.
	 *
	 * @param ch The character to set.
	 */
	private void setch(char ch)
	{
		mChars[mChIdx] = ch;
	}
}

class DTD {
	String name; // doctype name
	ExternalID externalID = ExternalID.EMPTY;
	Hashtable/*<Name, Contentspec>*/ elements = new Hashtable(); 
	Hashtable/*<elemQName, Hashtable<attQName, AttDef>>*/ attLists = new Hashtable();
	Hashtable/*<URI, Hashtable<localName, ref to attList element>>*/ attListsRef = new Hashtable();
	Hashtable parameterEntities = new Hashtable();
	Hashtable generalEntities = new Hashtable();
	Hashtable/*<Name, ExternalID>*/ notations = new Hashtable();
	
	static class ExternalID {
		public String pubidLiteral, systemLiteral; // ExternalID?
		// if pubidLiteral == null && systemLiteral == null => ExternalID is absent
		// if pubidLiteral == null => ExteralID :: 'SYSTEM' SystemLiteral
		// else ExteralID :: 'PUBLIC' PubidLiteral [SystemLiteral]

		public static final ExternalID EMPTY = new ExternalID();
	}
	
	static interface Contentspec {
		final int typeEMPTY = 0;
		final int typeANY = 1;
		final int typeMixed = 2;
		final int typeChildrenName = 3;
		final int typeChildrenChoice = 4;
		final int typeChildrenSeq = 5;
		int type();
	}
	
	static class Mixed implements Contentspec {
		public int type() { return typeMixed; }
		public void add(String name) {
			// add to list head
			names = new NamesList( name, names );
		}
		boolean finishWithAsterisk = false;
		NamesList names = null;
	}
	
	static abstract class Cp implements Contentspec {
		char modifier = '\0'; // valid values are '\0', '?', '+', '*'
	}
	static class CpName extends Cp {
		CpName( String name ) { this.name = name; }
		public int type() { return typeChildrenName; }
		void add(Cp cp) { /* internal error */ };
		String name;
	}
	static class CpList extends LinkedList {
		Cp cp;
		
		CpList( Cp cp, CpList next ){
			this.cp = cp;
			this.next = next;
		}
	}
	static abstract class ChoiceOrSeq extends Cp {
		void add(Cp cp) {
			// add to the list head
			list = new CpList(cp, list);
		};
		void invertChildrenOrder(){
			list = (CpList)LinkedList.invert(list);
		}
		CpList list = null;
	}
	static class Choice extends ChoiceOrSeq {
		public int type() { return typeChildrenChoice; }
	}
	static class Seq extends ChoiceOrSeq {
		public int type() { return typeChildrenSeq; }
	}
	
	static class AttDef {
		String localName;	// attribute local name
		String attType;		// attribute type see [54] AttType
		Hashtable/*<String, String>*/ enumeratedTypeValues;	// values list values for [57] EnumeratedType 
		char defaultDeclType = ddtDEFAULT;	// [60]
		String defaultDeclValue;
		
		final public static String typeNMTOKEN = "NMTOKEN";
		final public static char ddtIMPLIED = 'I';  
		final public static char ddtREQUIRED = 'R';
		final public static char ddtFIXED = 'F';
		final public static char ddtDEFAULT = 'D';
		
		public void clean() {
			localName = null;
			attType = null;
			defaultDeclType = ddtDEFAULT;
			if(enumeratedTypeValues != null) enumeratedTypeValues.clear();
			defaultDeclValue = null;
		}
	}
	
	final static Contentspec EMPTY = new Contentspec() {
		public int type() { return typeEMPTY; }
	};
	final static Contentspec ANY = new Contentspec() {
		public int type() { return typeANY; }
	};
	
	public void clean() {
		name = null;
		externalID = ExternalID.EMPTY;
		elements.clear(); 
		attLists.clear();
		attListsRef.clear();
		parameterEntities.clear();
		generalEntities.clear();
		notations.clear();
	}

	Hashtable getAttList(String elementQName) {
		Hashtable attList = (Hashtable)attLists.get(elementQName);
		if( attList == null ){
			attList = new Hashtable();
			attLists.put(elementQName, attList);
		}
		return attList;
	}

	void addAttList(String URI, String localName, Hashtable attList) {
		Hashtable ns2elm = (Hashtable)attListsRef.get( URI );
		if( ns2elm == null ){
			ns2elm = new Hashtable();
			attListsRef.put( URI, ns2elm );
		}
		// what should we do if ns2elm.containsKey(QName.local(elmqn))?  
		ns2elm.put(localName, attList);
	}
	
	Hashtable findAttList(String namespaceURI, String elementLocalName) {
		Hashtable ns2elm = (Hashtable)attListsRef.get(namespaceURI);
		if( ns2elm != null )
			return (Hashtable)ns2elm.get(elementLocalName);
		return null;
	}
}

/**
 * base class for any linked list
 */

class LinkedList {
	protected LinkedList next = null;
	
	static public LinkedList invert( LinkedList l ){
		LinkedList newHead = null, next;
		while( l != null ){
			next = l.next;
			l.next = newHead;
			newHead = l;
			l = next;
		}
		return newHead;
	}
}

class NamesList extends LinkedList {
	String name;
	
	NamesList( String name, NamesList next ){
		this.name = name;
		this.next = next;
	}
}

/**
 * This class represents namespace.  
 */

class Namespace extends LinkedList {
	String id = null, URI = null;
	
	final public Namespace init(String id, String URI){
		this.id = id;
		this.URI = URI;
		return this;
	}

	/**
	 * stack for namespaces 
	 */
	static class Stack {
		private Vector/*<Namespace>*/ stack = new Vector();
		
		public void clean() {
			while( stack.size() > 0 ) pop().free();

			// Default namespace
			pushUnique(0, "", "");
			// XML namespace
			pushUnique(0, "xml", "http://www.w3.org/XML/1998/namespace");
			// xmlns namespace
			pushUnique(0, Parser.XMLNSSTR, "http://www.w3.org/2000/xmlns/");
		}

		public boolean pushUnique( int topElemsToCheckCount, String id, String URI ){
			if( Parser.STRICT ) Parser.ASSERT( stack.size() >= topElemsToCheckCount );
			for( int idx = stack.size(); topElemsToCheckCount-- > 0 ;){
				if( ((Namespace)stack.elementAt( --idx )).id.equals(id) )
					return false;
			}
			stack.addElement(Namespace.allocate().init( id, URI ));
			return true;
		}

		public Namespace find(String id) {
			if( Parser.DEBUG_OUT != null )
				Parser.DEBUG_OUT.println( "Namespace.Stack.find '" + id + "'");
			for( int idx = stack.size(); idx-- > 0 ;){
				Namespace ns = (Namespace)stack.elementAt(idx);
				if( ns.id.length() == id.length() && id.equals(ns.id) )
					return ns;
			}
			return null;
		}
		
		public Namespace pop() {
			if( Parser.STRICT ) Parser.ASSERT( stack.size() > 0 );
			Namespace ns = (Namespace)stack.elementAt(stack.size() - 1);
			stack.removeElementAt(stack.size() - 1);
			return ns;
		}
	}
	
	// allocate / free support
	
	static public Namespace allocate(){
		if(freeObjects != null){
			Namespace obj = freeObjects;
			freeObjects = (Namespace)freeObjects.next;
			return obj;
		}
		return new Namespace();
	}
	
	public void free(){
		next = freeObjects;
		freeObjects = this;
	}
	
	private Namespace(){}	
	static private Namespace freeObjects = null;
}

class Element extends LinkedList {
	int registeredNamespacesNumber;
	Namespace namespace; // element namespace
	char[] elementName; // as read by qname()
	
	public Element init( char[] qname ){
		registeredNamespacesNumber = 0;
		namespace = null;
		elementName = qname;
		return this;
	}
	
	public String /*error message*/ resolveNamespace(Namespace.Stack nsStack) {
		// mIsNSAware is true
		namespace = nsStack.find( QName.prefix( elementName ) );
		if( namespace == null )
			return Parser.FAULT;
		return null;
	}

	/**
	 * stack for elements 
	 */
	static class Stack {
		private Vector/*<Element>*/ stack = new Vector();
		
		public void clean() {
			while( stack.size() > 0 ) pop().free();
		}
		public int size() {
			return stack.size();
		}
		public Element top(){
			if( Parser.STRICT ) Parser.ASSERT( stack.size() > 0 );
			return (Element)stack.elementAt(stack.size() - 1);
		}
		public void push( char[] qname ){
			stack.addElement(Element.allocate().init( qname ));
		}
		public Element pop(){
			Element element = top(); 
			stack.removeElementAt(stack.size() - 1);
			return element;
		}
	}
	
	// allocate / free support
	
	static public Element allocate(){
		if(freeObjects != null){
			Element obj = freeObjects;
			freeObjects = (Element)freeObjects.next;
			return obj;
		}
		return new Element();
	}
	
	public void free(){
		next = freeObjects;
		freeObjects = this;
	}
	
	private Element(){}	
	static private Element freeObjects = null;
}