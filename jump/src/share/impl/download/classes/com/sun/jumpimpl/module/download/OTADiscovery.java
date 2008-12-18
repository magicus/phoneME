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

import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;
import java.util.HashMap;
import java.io.InputStream;

import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;

public class OTADiscovery {

    static final String htmlMime = "text/html";
    static final String xmlMime = "text/xml";

    public HashMap discover(String url) {

        try {

            URL netUrl = new URL(url);
            URLConnection conn = netUrl.openConnection();
            conn.connect();

            if (DownloadModuleFactoryImpl.verbose) {
                System.err.println("debug : discover mimetype is "+
                                   conn.getContentType());
            }

            String contentType = conn.getContentType().toLowerCase();
            // Apache Web server adds encoding type to this field. So we have to look if it contains text/html...
            if (contentType.indexOf(htmlMime.toLowerCase()) != -1)
                return doHTMLDiscovery(url, netUrl, conn);
            // ... or text/xml
            else if (contentType.indexOf(xmlMime.toLowerCase()) != -1)
                return doXMLDiscovery(url, netUrl, conn);
            else
                throw new Exception("Content type for the applist " +
                        "is neither "+ htmlMime +" nor "+ xmlMime);
        } catch (Exception e) {
            e.printStackTrace();
        }
        // Nothing was found. Return empty.
        return new HashMap();
    }

    private static HashMap doHTMLDiscovery(String url, URL netUrl, URLConnection conn) {
        try {
            String rst = "";
            InputStream in = conn.getInputStream();
            
            while (true) {

                byte [] data = new byte[16384];
                int rd = in.read(data);
                if (rd < 0) {
                    break;
                }
                rst = rst + new String(data, 0, rd);
            }

            HashMap h = new HashMap();

            // parse

            String copy = rst.toLowerCase();
        
            while (true) {

                // require reference in enclosed into quotation marks
                int idx = copy.indexOf("<a href=\"");
        
                if (idx < 0) {
                    break;
                }

                try {

                    rst = rst.substring(idx+9);

                    idx = rst.indexOf('"');
                    // get the reference

                    String ref = rst.substring(0, idx);

                    rst = rst.substring(idx);

                    idx = rst.indexOf('>');
                    rst = rst.substring(idx+1);

                    // strictly, we should search to the next </a>,
                    // but we only will until first '<'
                    idx = rst.indexOf('<');
                    String name = rst.substring(0, idx);
                    
                    // let's use one helper function to get full URI
                    ref = ServerConfXMLHandler.getFullURI(netUrl, url, ref);

                    h.put(ref, name);

                } catch (Exception e) {
                    System.err.println("Parsing got an exception"+e);
                    break;
                }
                copy = rst.toLowerCase();
            }

            return h;
            
        } catch (java.io.IOException e) {
            System.err.println("Cannot read html content "+e);
        }
        return new HashMap();
    }


    private static HashMap doXMLDiscovery(String url, URL netUrl, URLConnection conn) {
        try {
            InputStream in = conn.getInputStream();

            SAXParserFactory factory = SAXParserFactory.newInstance();
            SAXParser parser = factory.newSAXParser();
            
            ServerConfXMLHandler handler = new ServerConfXMLHandler(url, netUrl);
            parser.parse(in, handler);

            return handler.getMap();
        } catch (org.xml.sax.SAXException saxExc) {
            System.err.println( "Cannot parse xml content "+saxExc );
        } catch (javax.xml.parsers.ParserConfigurationException parserExc) {
            System.err.println( "Cannot create an xml parser "+parserExc );
        } catch (java.io.IOException ioExc) {
            System.err.println( "Cannot read xml content "+ioExc );
        }
        return new HashMap();
    }
}

class ServerConfXMLHandler extends DefaultHandler {

    private HashMap hmap;
    private String state = "";
    private String path = "";
    
    private String targetURI = "";
    private String targetName = "";
    
    private String chars = "";
    private String url = "";
    private URL netUrl;

    public ServerConfXMLHandler(String url, URL netUrl) {
        this.url = url;
        this.netUrl = netUrl;
        hmap = new HashMap();
    }
    
    public HashMap getMap() {
        return hmap;
    }
    
    public void startElement (String uri, String localName,
                              String qName, Attributes attributes)
	throws SAXException {

        // building xml-path of the current tag
        path = path.concat("/").concat(qName.toLowerCase());
    }
    
    public void endElement (String uri, String localName, String qName)
        throws SAXException {
        // if we have got both the name and the uri of the target, put it into the map...
        if (!"".equals(targetName) && !"".equals(targetURI)) {
            hmap.put(targetURI, targetName);
            // ... and clean it
            targetName = "";
            targetURI = "";
        }
        // if we get reached a name tag, save the name of the target
        if ("/servercontent/dd:media/product/mediaobject/meta/name".equals(path)) {
            targetName = chars.trim();
        }
        // if we get reached a server tag, save the the uri of the target
        if ("/servercontent/dd:media/product/mediaobject/objecturi/server".equals(path)) {
            String ref = targetURI.concat(chars.trim());
            targetURI = getFullURI(netUrl, url, ref);
        }
        chars = "";
        
        // go back with the path
	path = path.substring(0, path.lastIndexOf("/"));
    }

    public void characters (char ch[], int start, int length)
	throws SAXException {
        chars = chars + new String(ch, start, length);
    }

    /**
     * Returns an absolute URI of the target object based on descriptor request URL.
     * @param netUrl the URL of descriptor request.
     * @param url the string representation of descriptor request.
     * @param ref target object reference.
     * @return a full URI of the target object
     */
    public static String getFullURI(URL netUrl, String url, String ref) {
        if (ref.startsWith( "/")) {
            // relative to the hostname
            String base = netUrl.getProtocol() + "://" + netUrl.getHost();
            if (netUrl.getPort() < 0)
                ref = base + ref;
            else
                ref = base + ":" + netUrl.getPort() + ref;
        }
        
        if (ref.indexOf(':') < 0) {
            // relative to current directory
            ref = url.substring(0, url.lastIndexOf('/') + 1) + ref;
        }
        return ref;
    }
}
