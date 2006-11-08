/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/**
 * @version @(#)JNLPOTA.java	1.26 05/10/20
 */

package com.sun.appmanager.impl.ota;

import java.io.OutputStream;
import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Vector;

import com.sun.appmanager.ota.Application;
import com.sun.appmanager.ota.Descriptor;
import com.sun.appmanager.ota.OTAException;
import com.sun.appmanager.ota.SyntaxException;

public class JNLPOTA extends CDCAmsOTA {

    public static final String JNLP_MIME_TYPE = "application/x-java-jnlp-file";

    static String iconMime = "image/gif";
    String codebase = "";
    String version = "";

    // these two flags determine the type of content (application/library)
    // and the type of the library (native/java)
    boolean isLib = false;
    boolean isNative = false;

    // this information needs to be gothered from different sections of
    // JNLP descriptor before Application instance can be added to
    // Descriptior
    String appName = null;
    String appCName = null;
    String appIcon = null;
    // since many sections of JNLP descriptor are being mapped into
    // application parameters we need to gather it from the pieces
    Application app = null;
    // here application arguments are being stored if Application object
    // had not been already created. If it had arguments are being added 
    // to an Appilcation instance
    Hashtable appProps = null;

    public Descriptor createDescriptor(String url) throws OTAException {

        try {
            
            URL ddURL = new URL(url);
            URLConnection conn = ddURL.openConnection();

            String mimeType = conn.getContentType();

            System.err.println("debug : xlet mimetype is "+mimeType);
            
            if ((mimeType == null) || !(mimeType.equalsIgnoreCase(JNLP_MIME_TYPE) || mimeType.equalsIgnoreCase("application/xml"))) {
                throw new OTAException("Content type for the DD URL"+
                        "is not "+JNLP_MIME_TYPE+"\n"+url);
            }

            // load the descriptor
            Parser parser = new Parser();
            parser.parse(ddURL);
            Document dd = parser.getDocument();

            // parse the descriptor
            OMADescriptor d = new OMADescriptor("jnlp", url);

            Iterator i = dd.getIterator();

            // The first DocumentElement should be information
            // about the Document itself, including its
            // attributes.
            DocumentElement de = (DocumentElement)i.next();
            String name = de.getName();
            if ( name.equals( "jnlp" ) )
            {
                codebase = de.getAttribute( "codebase" );
                if ( codebase == null )
                {
                    throw new OTAException("jnlp tag must contain codebase attribute");
                }
                version = de.getAttribute( "version" );
                if ( version == null )
                {
                    throw new OTAException("jnlp tag must contain version attribute");
                }                       
                d.setVersion( version );
            }
            else
            {
                System.err.println("debug : top-level tag name is " + name );
                throw new OTAException("Incorrect top-level tag, must be jnlp");
            }

            while ( i.hasNext() )
            {
                de = (DocumentElement)i.next();

                String ename = de.getName();
                String eval = de.getValue();
                
                if ( ename.equalsIgnoreCase( "information" ) )
                {
                    extractInfo( d, de );
                }
                else if ( ename.equalsIgnoreCase( "resources" ) )
                {
                    extractResources( d, de );
                }
                else if ( ename.equalsIgnoreCase( "application-desc" ) )
                {
                    extractApplication( d, de );
                }
                else if ( ename.equalsIgnoreCase( "applet-desc" ) )
                {
                    extractApplet( d, de );
                }
                else if ( ename.equalsIgnoreCase( "component-desc" ) )
                {
                    // treat this descriptor as library
                    isLib = true;
                }
                else
                {
                    System.out.println( "Warning : unknown or unsupported JNLP tag : " + ename );
                }
            }

            if ( !isLib )
            {
                if ( app != null )
                {
                    d.addApplication( app );
                }
            }
            else
            {
                d.addLibrary( !isNative );
            }

            d.checkOut();
            return d;
        }
        catch ( SyntaxException e )
        {
            e.printStackTrace();
            throw new OTAException( "The descriptor file is invalid" );
        }
        catch ( OTAException e )
        {
            throw e;
        }
        catch ( Throwable e )
        {
            e.printStackTrace();
            throw new OTAException( "Unexpected error:"+e.getMessage() );
        }
    }

    private String createHref(String href) {
        if (href == null) {
            return null;
        }
        if (href.indexOf("://") != -1 || codebase == null) {
            return href;
        } else {
            return codebase + href;
        }
    }

    private void extractInfo( OMADescriptor d, 
                              DocumentElement de ) throws Exception {

        if ( de == null ) {
            throw new NullPointerException( "null DocumentElement!" );
        }

        Vector elementVector = de.elements;
        for ( int i = 0 ; i < elementVector.size(); i++ ) {
            DocumentElement subElement =
                (DocumentElement)elementVector.get( i );

            String name = subElement.getName();
            if ( "icon".equalsIgnoreCase( name ) ) {
                String href = subElement.getAttribute( "href" );
                appIcon = createHref( href );
                d.setIconURI( appIcon );
                if ( app != null ) {
                    app.setIconPath( appIcon );
                }
            } else if ( "title".equalsIgnoreCase( name ) ) {
                String t = subElement.getValue();
                d.setDisplayName( t );
                d.setName( t );
                appName = t;
                if ( app != null ) {
                    app.setName( appName );
                }
            } else if ( "vendor".equalsIgnoreCase( name ) ) {
                d.setVendor( subElement.getValue() );
            } else if ( "description".equalsIgnoreCase( name ) ) {
                d.setDescription( subElement.getValue() );
            } else if ( "homepage".equalsIgnoreCase( name ) ) {
                d.setInfoURI( subElement.getValue() );
            }
        }
        return;
    }

    private void extractResources( Descriptor d, 
                                   DocumentElement de ) throws Exception {
        if ( de == null ) {
            throw new NullPointerException( "null DocumentElement" );
        }
        boolean jar_processed = false;
        Vector elementVector = de.elements;
        for ( int i = 0 ; i < elementVector.size(); i++ ) {
            DocumentElement subElement =
                (DocumentElement)elementVector.get( i );

            String name = subElement.getName();
            if ( "property".equalsIgnoreCase( name ) ) {
                // just copy properties into descriptor
                String pname = subElement.getAttribute( "name" );
                String pval = subElement.getAttribute( "value" );
                if ( app != null ) {
                    app.addProperty(  pname, pval);
                } else {
                    if ( appProps == null ) {
                        appProps = new Hashtable();
                    }
                    appProps.put( pname, pval );
                }
            } else if ( "jar".equalsIgnoreCase( name ) ) {
                // We currently do not support multiple jars JNLP provisioning
                if ( jar_processed ) {
                    throw new OTAException(" Multiple jars are not supported yet." );
                }
                // Will this work with our DocumentElement structure?
                extractObject( d, subElement );
            } else if ( "nativelib".equalsIgnoreCase( name ) ) {
                isLib = true;
                isNative = true;
                // Will this work with our DocumentElement structure?
                extractObject( d, subElement );
            } else if ( "extension".equalsIgnoreCase( name ) ) {
                // Currently we do not support dependencies.
                //String ename = subElement.getAttribute( "name" );
                //String v = subElement.getAttribute( "version" );
                //String href = createHref( subElement.getAttribute( "href" ) );
                //d.addDependency( new Dependency( Dependency.DIO_PACKAGE,
                //             new Requirement(ename, v.toString()), href));
                throw new OTAException( "<extension> tag is not supported yet." );
            } else if ( "package".equalsIgnoreCase( name ) ) {
                throw new OTAException("<package> tag is not supported yet.");
            }
        }
        return;
    }

    private void extractObject( Descriptor d,
                                DocumentElement de ) throws Exception
    {
        String href = createHref( de.getAttribute( "href" ) );
        if ( href == null ) {
            throw new OTAException( "href not specified" );
        }
        System.err.println( "debug : new object URI = " + 
                            href + ", old URI = " + d.getObjectURI() );
        d.setObjectURI(href);
        int sz;
        try {
            sz = Integer.parseInt( de.getAttribute( "size") );
        } catch ( Exception e ) {
            throw new OTAException("size must be integer");
        }
        if ( sz < 0 ) {
            throw new OTAException("Negative size");
        }
        d.setSize(sz);
        return;
    }

    private void extractApplication( Descriptor d,
                                     DocumentElement de ) throws Exception
    {
        appCName = de.getAttribute( "main-class" );
        if ( appName == null ) {
            appName = "unnamed";
        }
        app = new MainApplication( appName, appIcon, appCName, null );
        if ( app == null ) {
            System.err.println( "debug : failed attempt to create Application with name="
                                +appName +
                                ", icon=" + appIcon +
                                ", cname=" + appCName );
            throw new OTAException( "Couldn't create an application instance" );
        }
        Vector elementVector = de.elements;
        String appArgs = "";
        for ( int i = 0 ; i < elementVector.size(); i++ ) {
            DocumentElement subElement =
                (DocumentElement)elementVector.get( i );
            String name = subElement.getName();
            if ( "argument".equalsIgnoreCase( name ) ) {
                appArgs += subElement.getValue();
            }
        }
        if ( !appArgs.equals( "" ) ) {
            app.addProperty( "XletContext.ARGS", appArgs );
        }
        if ( appProps != null ) {
            Enumeration pp = appProps.keys();
            while ( pp.hasMoreElements()) {
                String key = (String) pp.nextElement();
                app.addProperty( key, (String) appProps.get( key ) );
            }
        }
        return;
    }

    private void extractApplet( Descriptor d,
                                DocumentElement de ) throws Exception
    {
        appCName = de.getAttribute( "main-class" );
        appName = de.getAttribute( "name" );
        // MainApplication?
        app = new MainApplication( appName, appIcon, appCName, null );

        Vector elementVector = de.elements;
        String pname = "";
        String pval = "";
        for ( int i = 0 ; i < elementVector.size(); i++ ) {
            DocumentElement subElement =
                (DocumentElement)elementVector.get( i );
            String name = subElement.getName();

            if ( "param".equalsIgnoreCase( name ) ) {
                pname = subElement.getAttribute( "name" );
                pval = subElement.getAttribute( "value" );
                app.addProperty( pname, pval );
            } // ignore all tags that do not specify properties
        }
        return;
   }

    public boolean sendNotify(String notifyURL, String statusCode,
           String statusMsg) {

        try {

            if ((notifyURL == null)||"".equals(notifyURL)) {
                return false;
            }

            System.out.println("InstallNotifyURL: " + notifyURL);

            URL url = new URL(notifyURL);
   
            // Open a connection to the install-notiy URL 
            HttpURLConnection huc = (HttpURLConnection) url.openConnection();

            // This operation sends a POST request
            huc.setDoOutput(true);

            // System.out.println("StatusCode=" + statusCode);
            // System.out.println("StatusMsg=" + statusMsg);

            // Write the status code and message to the URL
            OutputStream os = huc.getOutputStream();
            String content = statusCode + " " + statusMsg;
            byte [] buf = content.getBytes(); 
            os.write(buf);
            os.flush();
            os.close();

            if (huc.getResponseCode() != HttpURLConnection.HTTP_OK ) {
                throw new OTAException("Http response is not OK "+
                                       huc.getResponseCode());
            } else {
                System.out.println("RESPONSE code: " + huc.getResponseCode() +
                                   " " + huc.getResponseMessage());
            }

            huc.disconnect();

        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    /**
     * Returns schema implemented by this instance.
     */
    public String getSchema() {
        return "jnlp";
    }
}
