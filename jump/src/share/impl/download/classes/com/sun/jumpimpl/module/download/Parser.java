/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.module.download;

import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;

import java.net.URL;
import java.net.URLConnection;

import java.util.Vector;

import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;
import javax.xml.parsers.SAXParserFactory;
import javax.xml.parsers.SAXParser;

public class Parser extends DefaultHandler {

    static SAXParserFactory spf;

    private SAXParser parser;
    private Document document;
    private Vector currentElement;
    private URL url;
    boolean fastForward = false;

    public Document getDocument() {
        return document;
    }

    public void parse(URL url) throws Exception {

        this.url = url;

        try {
            InputStream is = null;

            if ((url.getProtocol()).startsWith("file")) {

                String fileName = url.getFile();
                while (fileName.charAt(1) == '/') {
                    fileName = fileName.substring(1);
                }
                is = (InputStream)(new FileInputStream(fileName));

            } else {

                URLConnection uc = url.openConnection();
                is = uc.getInputStream();

            }
            
            parse(is);

        } catch (Exception e) {
            // e.printStackTrace();
            throw e;
        }

        return;
    }
  
    public void parse(InputStream is) throws Exception {
        this.parser = spf.newSAXParser();
        parser.parse(new InputSource(is), this);
        return;
    }

    
    // Called once at the beginning of a document
    public void startDocument() {
        currentElement = new Vector();
        return;
    }

    public void endDocument() {

        if (currentElement.size() != 0) {
            if (DownloadModuleFactoryImpl.verbose) {
                System.out.println("bogosity! document end with extra elements!");
                System.out.println("currentElement size is " +
                                   currentElement.size());
            }
        }
        return;
    }
  
    /**
     * Receive notification of the start of an element.
     * @param uri - The Namespace URI, or the empty string
     *              if the element has no Namespace URI or
     *              if Namespace processing is not being
     *              performed.
     * @param localName - The local name (without prefix), or the
     *                    empty string if Namespace processing is not
     *                    being performed.
     * @param qName - The qualified name (with prefix), or the empty
     *                string if qualified names are not available.
     * @param atts - The attributes attached to the element. If there
     *               are no attributes, it shall be an empty 
     *               Attributes object.
     * @throws SAXException - Any SAX exception, possibly wrapping
     *                        another exception.
     */
    public void startElement(String uri, String localName,
                             String qName, Attributes attr)
      throws SAXException {
  
        if (DownloadModuleFactoryImpl.verbose) {
            System.out.println("startEl : uri="+uri+", localname="+localName+
                               ", qName="+qName);
        }
        DocumentElement de = null;
        DocumentElement parent = null;

        // The first element we find should be the document
        if (currentElement.size() == 0) {
            de = new DocumentElement(qName, true, null);
            document = new Document(qName, de);
            for ( int i = 0; i < attr.getLength() ; i++ ) {
                if (DownloadModuleFactoryImpl.verbose) {
                    System.out.println( "\tattribute: " +
                                        attr.getQName(i) + ", value: " +
                                        attr.getValue(i));
                }
                de.addAttribute(attr.getQName(i), 
                                attr.getValue(i));
            }

        } else {

            // We're inside the document. Create a new element
            parent = (DocumentElement)currentElement.elementAt( 
                                                 currentElement.size() - 1);
            de = new DocumentElement(qName, false, parent);
            for (int i = 0; i < attr.getLength(); i++) {
                if (DownloadModuleFactoryImpl.verbose) {
                    System.out.println( "\tattribute: " +
                                        attr.getQName(i) + ", value: " +
                                        attr.getValue(i));
                }
                de.addAttribute( attr.getQName(i), 
                                 attr.getValue(i));
            }
        }
        // Add the new element to our vectors
        if (parent != null) {
            parent.addElement(de);
        }
        currentElement.add(de);
        return;
    }

    public void characters( char [] ch, int start,
                            int length ) throws SAXException {
        if (fastForward) {
            return;
        }

        ((DocumentElement)currentElement.elementAt(currentElement.size() - 1)).addCharacters(ch, start, length);

        if (DownloadModuleFactoryImpl.verbose) {
            System.out.println("\tcharacters " +
                               (new String(ch, start, length)).trim());
        }
        return;
    }

    public void endElement(String namespaceURI, String localName,
                           String qName) throws SAXException {
        if (DownloadModuleFactoryImpl.verbose) {
            System.out.println( "endEl : uri=" + namespaceURI + ", localname=" + 
                              localName + ", qName=" + qName );
        }
        currentElement.removeElementAt(currentElement.size() - 1);
        return;    
    }
  

    static {
        try {
            spf = SAXParserFactory.newInstance();
            //spf.setNamespaceAware( true );
            spf.setValidating( false );
        } catch( Exception e ) { 
            e.printStackTrace();
        }
    }
  
}
