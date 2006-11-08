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
 * @version @(#)CDCAmsOTA.java	1.8 05/10/20
 */

package com.sun.appmanager.impl.ota;

import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;
import java.util.Hashtable;
import java.io.InputStream;

import com.sun.appmanager.ota.Descriptor;
import com.sun.appmanager.ota.Destination;
import com.sun.appmanager.ota.DLIndicator;
import com.sun.appmanager.ota.OTA;
import com.sun.appmanager.ota.OTAException;

public abstract class CDCAmsOTA extends OTA
{
    static final String htmlMime = "text/html";

    public Hashtable discover( String url )
    {

        try
        {
            URL netUrl = new URL( url );
            URLConnection conn = netUrl.openConnection();
            conn.connect();

            //System.err.println( "debug : discover mimetype is "+
            //                    conn.getContentType() );

            if (!htmlMime.equalsIgnoreCase( conn.getContentType() ) )
            {
                throw new Exception( "Content type for the applist " +
                                     
                                     "is not "+ htmlMime );
            }

            String rst = "";
            InputStream in = conn.getInputStream();

            while ( true )
            {
                byte [] data = new byte[16384];
                int rd = in.read( data );
                if ( rd < 0)
                {
                    break;
                }
                rst = rst + new String( data, 0, rd );
            }

            Hashtable h = new Hashtable();

            // parse

            String copy = rst.toLowerCase();
        
            while ( true )
            {

                // require reference in enclosed into quotation marks
                int idx = copy.indexOf( "<a href=\"" );
        
                if ( idx < 0)
                {
                    break;
                }

                try
                {

                    rst = rst.substring( idx+9);

                    idx = rst.indexOf('"');
                    // get the reference

                    String ref = rst.substring(0, idx );

                    rst = rst.substring( idx );

                    idx = rst.indexOf( '>' );
                    rst = rst.substring( idx+1 );

                    // strictly, we should search to the next </a>,
                    // but we only will until first '<'
                    idx = rst.indexOf( '<' );
                    String name = rst.substring( 0, idx );

                    if ( ref.startsWith( "/") ) 
                    {
                        // relative to the hostname
                        ref = netUrl.getProtocol() + "://" +
                            netUrl.getHost() + ":" + netUrl.getPort() + ref;
                    }

                    if ( ref.indexOf(':') < 0 )
                    {
                        // relative to current directory
                        ref = url.substring( 0, url.lastIndexOf( '/' ) + 1 ) + ref;
                    }

                    h.put( ref, name );
                }
                catch ( Exception e )
                {
                    System.err.println( "Parsing got an exception"+e );
                    break;
                }
                copy = rst.toLowerCase();
            }

            return h;
        }
        catch ( Exception e )
        {
            e.printStackTrace();
        }

        // Nothing was found. Return empty.
        return new Hashtable();
    }

    public abstract Descriptor createDescriptor( String url )
        throws OTAException;

    public boolean download( String url, String nfUri,
                                int size, Destination store,
                                DLIndicator report ) throws OTAException
    {

        int gran = 0;
        int pct = 0;

        try
        {
            if ( report != null )
            {
                gran = report.getGranularity();
            }

            URL objectUrl = new URL( url );
    
            if ( report != null )
            {
                report.updatePercent(0);
            }
            
            URLConnection conn = objectUrl.openConnection();
    
            if ( ( ( HttpURLConnection )conn ).getResponseCode() !=
                     HttpURLConnection.HTTP_OK )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri, ST_LOADERERROR,
                                "Can't process server response" );
                }
            
                throw new OTAException( "Http response is not OK: "+
                             ( (HttpURLConnection )conn ).getResponseCode() );
            }
    
            String mimeType = conn.getContentType();
    
            //System.err.println( "debug : download : mimetype is "+mimeType );
    
            try
            {
                store.acceptMimeType( mimeType );
            }
            catch ( OTAException ee )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri,
                                ST_NONACCEPTABLECONTENT,
                                "Unknown MIME type" );
                }
                if ( report != null )
                {
                    report.downloadDone();
                }
                throw ee;
            }
    
            InputStream in = conn.getInputStream();
            store.start( url, mimeType );
    
            int bufferSize = store.getMaxChunkSize();
            if ( bufferSize <= 0)
            {
                bufferSize = 8192;
            }
    
            byte [] data = new byte[ bufferSize ];
            int len = 0;
    
            while ( true )
            {
                int wantread = size - len;
                if ( size <=0 )
                {
                    wantread = bufferSize;
                }
    
                if ( wantread > bufferSize )
                {
                    wantread = bufferSize;
                }
            
                int chunk = store.receive( in, wantread );
                if ( chunk <= 0)
                {
                    break;
                }
                len += chunk;
    
                // Report percentage complete, if appropriate.
                if ( report != null && size != 0 )
                {
                    int current_pct = len * 100 / size;
                    if ( current_pct - pct > gran ) 
                    {
                        pct = current_pct;
                        report.updatePercent( pct );
                    }
                }
            }
                
            if ( (size > 0) && ( len != size ) )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri, ST_SIZEMISMATCH,
                                ( size-len ) + " bytes missing" );
                }
                if ( report != null )
                {
                    report.downloadDone();
                }
                throw new OTAException( "Could only read " +
                                        len + " bytes");
            }
    
            if ( ( size > 0 ) && ( in.read() != -1 ) )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri, ST_SIZEMISMATCH,
                                "Too many bytes" );
                }
                if ( report != null )
                {
                    report.downloadDone();
                }
                throw new OTAException( "Read past "+len+" bytes");
            }

            in.close();
            store.finish();
            in = null;
            if ( report != null )
            {
                report.updatePercent(100);
                try
                {
                    Thread.sleep(300);
                }
                catch ( Exception e )
                {
                    // eat the exception
                }
                report.downloadDone();
            }
    
            store = null;
    
        }
        catch ( Exception e )
        {
            System.out.println( "download exception: " +
                                e.getMessage() );
            e.printStackTrace();
            if ( store != null )
            {
                store.abort();
            }
            if ( ( report != null ) && ( !report.isCancelled() ) )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri,
                                ST_LOADERERROR, "I/O error" );
                }
                report.downloadDone();
                throw new OTAException( "I/O trouble:\n" +
                                    e.toString() );
            }
        }
    
        if ( (report != null ) && ( report.isCancelled() ) )
        {
            report.downloadDone();
            return false;
        }
        return true;
    }

    public boolean download( Descriptor des,
        Destination store, DLIndicator report ) throws OTAException
    {
        return download( des.getObjectURI(), des.getInstallNotifyURI(),
                         des.getSize(), store, report );
    }

    public abstract boolean sendNotify( String notifyURL, String statusCode,
                                        String statusMsg );

    public abstract String getSchema();
}
    
    
