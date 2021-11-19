/*
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

package com.sun.ukit.dom;

import java.io.IOException;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.xml.sax.EntityResolver;
import org.xml.sax.ErrorHandler;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

import com.sun.ukit.xml.Input;
import com.sun.ukit.xml.Pair;
import com.sun.ukit.xml.Parser;

/**
 * XML non-validating DOM parser.
 *
 * @see com.sun.ukit.xml.Parser
 */

/* pkg */ final class ParserDOM
	extends Parser
{
	//		SAX handlers
	/* pkg */ ErrorHandler   mHandErr;   // the error handler
	/* pkg */ EntityResolver mHandEnt;   // the entity resolver

	private XDoc   mDoc;
	private Node   mParent;

	/**
	 * Constructor.
	 */
	/* pkg */ ParserDOM(boolean isNSAware, XDoc doc)
	{
		super();
		mIsNSAware = isNSAware;
		mDoc       = doc;
		mParent    = mDoc;
	}

	/**
	 * Parse an XML document.
	 *
	 * <p>The application can use this method to instruct the XML
	 * reader to begin parsing an XML document from any valid input
	 * source (a character stream, a byte stream, or a URI).</p>
	 *
	 * <p>Applications may not invoke this method while a parse is in
	 * progress (they should create a new parser instead for each
	 * nested XML document).  Once a parse is complete, an
	 * application may reuse the same parser object, possibly with a
	 * different input source.</p>
	 *
	 * <p>This method is synchronous: it will not return until parsing
	 * has ended.  If a client application wants to terminate 
	 * parsing early, it should throw an exception.</p>
	 *
	 * @param is The input source for the top-level of the XML document.
	 * @exception org.xml.sax.SAXException Any SAX exception, possibly
	 *            wrapping another exception.
	 * @exception java.io.IOException An IO exception from the parser,
	 *            possibly from a byte stream or character stream
	 *            supplied by the application.
	 * @see org.xml.sax.InputSource
	 * @see org.xml.sax.EntityResolver
	 * @see org.xml.sax.ErrorHandler 
	 */
	public Document parse(InputSource is)
		throws IOException, SAXException
	{
		if (is == null)
			throw new IllegalArgumentException("");
		//		Set up the document
		mInp = new Input(BUFFSIZE_READER);
		mPh  = PH_BEFORE_DOC;  // before parsing
		try {
			setinp(is);
		} catch(SAXException saxe) {
			throw saxe;
		} catch(IOException ioe) {
			throw ioe;
		} catch(RuntimeException rte) {
			throw rte;
		} catch(Exception e) {
			panic(e.toString());
		}
		return parse();
	}

	/**
	 * Parse the XML document content using specified handlers and an 
	 * input source.
	 *
	 * @exception IOException If any IO errors occur.
	 * @exception SAXException If the underlying parser throws a SAXException 
	 *   while parsing.
	 */
	private Document parse()
		throws SAXException, IOException
	{
		init();
		mDoc._setBuild(true);
		try {
			if (mPh != PH_MISC_DTD)
				mPh = PH_MISC_DTD;  // misc before DTD
			mEvt = EV_NULL;
			//		XML document prolog
			do {
				switch (step()) {
				case EV_ELM:
				case EV_ELMS:
					mPh = PH_DOCELM;
					break;

				case EV_COMM:
				case EV_PI:
				case EV_PENT:
				case EV_UENT:
				case EV_NOT:
					break;

				case EV_DTDS:
					if (mPh >= PH_DTD)
						panic(FAULT);
					mPh = PH_DTD;  // DTD
					break;

				case EV_DTDE:
					if (mPh != PH_DTD)
						panic(FAULT);
					mPh = PH_DTD_MISC;  // misc after DTD
					break;

				default:
					panic(FAULT);
				}
			} while (mPh < PH_DOCELM);  // misc before DTD
			//		XML document starting with document's element
			do {
				switch (mEvt) {
				case EV_ELMS:
					//		Set new parent node
					step();
					break;
				case EV_ELM:
				case EV_ELME:
					//		Restore previous parent node
					mParent = mParent.getParentNode();
					while (mPref.list == mElm) {
						mPref = del(mPref);
					}
					//		Remove the top element tag
					mElm = del(mElm);
					if (mElm == null)
						mPh = PH_DOCELM_MISC;
					else
						step();
					break;

				case EV_TEXT:
				case EV_WSPC:
				case EV_CDAT:
				case EV_COMM:
				case EV_PI:
				case EV_ENT:
					step();
					break;

				default:
					panic(FAULT);
				}
			} while (mPh == PH_DOCELM);
			//		Misc after document's element
			do {
				if (wsskip() == EOS)
					break;

				switch (step()) {
				case EV_COMM:
				case EV_PI:
					break;

				default:
					panic(FAULT);
				}
			} while (mPh == PH_DOCELM_MISC);
			mPh = PH_AFTER_DOC;  // parsing is completed

		} catch(SAXException saxe) {
			throw saxe;
		} catch(IOException ioe) {
			throw ioe;
		} catch(RuntimeException rte) {
			throw rte;
		} catch(Exception e) {
			panic(e.toString());
		} finally {
			mDoc._setBuild(false);
			cleanup();
		}

		return mDoc;
	}

	/**
	 * DTD post-processing.
	 * Process all default attributes and set these attributes on document. 
	 * This method sets all the declared element and attribute names as a 
	 * String and value for all the default attributes. This method also sets 
	 * all the default attributes on the document.
	 */
	protected void dtdpost()
		throws Exception
	{
		super.dtdpost();

		//		Set all default attributes on the document object
		for (Pair elm = mAttL; elm != null; elm = elm.next) {
			elm.name = elm.qname();  // need qName string for DOM
			for (Pair att = elm.list; att != null; att = att.next) {
				if ((att.num & 0x3) == 0x3)
					mDoc._setDefAttr(elm.name, att.qname(), att.value);
			}
		}
	}

	/**
	 * Creates a new element and sets it as current parent.
	 */
	protected void attrs()
		throws Exception
	{
		//		Count default attributes. 
		int defnum = 0;
		for(Pair next = mElm.list; next != null; next = next.next) {
			if ((next.num & 0x3) == 0x3)  // next has default value
				defnum++;
		}

		super.attrs();  // process all element's attributes

		//		Create a new element and set it as current parent.
		XElm elm = new ElmImp(mElm, defnum, mDoc);
		//		Set new parent
		mParent.appendChild(elm);
		mParent = elm;
	}

	/**
	 * Reports document type.
	 *
	 * @param name The name of the entity.
	 * @param pubid The public identifier of the DTD or <code>null</code>.
	 * @param sysid The system identifier of the DTD or <code>null</code>.
	 * @param dtdint The DTD internal subset or <code>null</code>.
	 */
	protected void docType(
		String name, String pubid, String sysid, char[] dtdint)
	{
		mDoc.appendChild(
			mDoc.getImplementation().createDocumentType(name, pubid, sysid));
	}

	/**
	 * Reports a comment.
	 *
	 * @param text The comment text starting from first character.
	 * @param length The number of characters in comment.
	 */
	protected void comm(char[] text, int length)
	{
		if (mPh != PH_DTD)
			mParent.appendChild(mDoc.createComment(new String(text, 0, length)));
	}

	/**
	 * Reports a processing instruction.
	 *
	 * @param target The processing instruction target name.
	 * @param body The processing instruction body text.
	 */
	protected void pi(String target, String body)
		throws SAXException
	{
		if (mPh != PH_DTD)
			mParent.appendChild(mDoc.createProcessingInstruction(target, body));
	}

	/**
	 * Reports new namespace prefix. 
	 * The Namespace prefix (<code>mPref.name</code>) being declared and 
	 * the Namespace URI (<code>mPref.value</code>) the prefix is mapped 
	 * to. An empty string is used for the default element namespace, 
	 * which has no prefix.
	 */
	protected void newPrefix()
		throws SAXException
	{
	}

	/**
	 * Reports skipped entity name.
	 *
	 * @param name The entity name.
	 */
	protected void skippedEnt(String name)
		throws SAXException
	{
		if (mPh != PH_DTD)
			mParent.appendChild(mDoc.createEntityReference(name));
	}

	/**
	 * Returns an <code>InputSource</code> for specified entity or 
	 * <code>null</code>.
	 *
	 * @param name The name of the entity.
	 * @param pubid The public identifier of the entity.
	 * @param sysid The system identifier of the entity.
	 */
	protected InputSource resolveEnt(String name, String pubid, String sysid)
		throws SAXException, IOException
	{
		return (mHandEnt != null)? mHandEnt.resolveEntity(pubid, sysid): null;
	}

	/**
	 * Reports internal parsed entity.
	 *
	 * @param name The entity name.
	 * @param value The entity replacement text.
	 */
	protected void intparsedEntDecl(String name, char[] value)
		throws Exception
	{
		DocTypeImp doctyp = (DocTypeImp)mDoc.getDoctype();
		if (doctyp.entmap._find(name) < 0) {
			EntImp ent = new EntImp(name, mDoc);
			ent._appendChild((XNode)mDoc.createTextNode(new String(value)));
			doctyp.entmap._append(ent);
		}
	}

	/**
	 * Reports external parsed entity.
	 *
	 * @param name The entity name.
	 * @param pubid The entity public identifier, may be null.
	 * @param name The entity system identifier, may be null.
	 */
	protected void extparsedEntDecl(String name, String pubid, String sysid)
		throws Exception
	{
		DocTypeImp doctyp = (DocTypeImp)mDoc.getDoctype();
		if (doctyp.entmap._find(name) < 0)
			doctyp.entmap._append(new EntImp(name, pubid, sysid, null, mDoc));
	}

	/**
	 * Reports notation declaration.
	 *
	 * @param name The notation's name.
	 * @param pubid The notation's public identifier, or null if none was given.
	 * @param sysid The notation's system identifier, or null if none was given.
	 */
	protected void notDecl(String name, String pubid, String sysid)
		throws SAXException
	{
		DocTypeImp doctyp = (DocTypeImp)mDoc.getDoctype();
		if (doctyp.notmap._find(name) < 0)
			doctyp.notmap._append(new NotImp(name, pubid, sysid, mDoc));
	}

	/**
	 * Reports unparsed entity name.
	 *
	 * @param name The unparsed entity's name.
	 * @param pubid The entity's public identifier, or null if none was given.
	 * @param sysid The entity's system identifier.
	 * @param notation The name of the associated notation.
	 */
	protected void unparsedEntDecl(
			String name, String pubid, String sysid, String notation)
		throws SAXException
	{
		DocTypeImp doctyp = (DocTypeImp)mDoc.getDoctype();
		if (doctyp.entmap._find(name) < 0)
			doctyp.entmap._append(new EntImp(name, pubid, sysid, notation, mDoc));
	}

	/**
	 * Notifies the handler about fatal parsing error.
	 *
	 * @param msg The problem description message.
	 */
	protected void panic(String msg)
		throws SAXException
	{
		SAXParseException spe = new SAXParseException(msg, null);
		if (mHandErr != null)
			mHandErr.fatalError(spe);
		throw spe;  // [#1.2] fatal error definition
	}

	/**
	 * Reports characters and empties the parser's buffer.
	 * This method is called only if parser is going to return control to 
	 * the main loop. This means that this method may use parser buffer 
	 * to report white space without copying characters to temporary 
	 * buffer.
	 */
	protected void bflash()
		throws SAXException
	{
		String str = (mBuffIdx >= 0)? new String(mBuff, 0, (mBuffIdx + 1)): "";

		//		Textual data has been read
		switch (mEvt) {
		case EV_TEXT:
			if (mBuffIdx >= 0)
				mParent.appendChild(mDoc.createTextNode(str));
			break;

		case EV_CDAT:
			mParent.appendChild(mDoc.createCDATASection(str));
			break;

		default:
			panic(FAULT);
		}
		mBuffIdx = -1;
	}

	/**
	 * Reports white space characters and empties the parser's buffer.
	 * This method is called only if parser is going to return control to 
	 * the main loop. This means that this method may use parser buffer 
	 * to report white space without copying characters to temporary 
	 * buffer.
	 */
	protected void bflash_ws()
		throws SAXException
	{
		if (mBuffIdx >= 0) {
			if ((mElm.id & FLAG_XMLSPC_PRESERVE) != 0) {
				//		Textual data has been read
				mParent.appendChild(mDoc.createTextNode(
					new String(mBuff, 0, (mBuffIdx + 1))));
			}

			mBuffIdx = -1;
		}
	}
}
