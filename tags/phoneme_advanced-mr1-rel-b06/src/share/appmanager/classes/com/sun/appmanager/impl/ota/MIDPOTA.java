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
 * @version @(#)MIDPOTA.java	1.32 05/09/20
 */

package com.sun.appmanager.impl.ota;

import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;
import java.util.Hashtable;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.io.InputStream;
import java.io.OutputStream;

import com.sun.appmanager.ota.Descriptor;
import com.sun.appmanager.ota.OTAException;
import com.sun.appmanager.ota.SyntaxException;

public class MIDPOTA extends CDCAmsOTA
{
    public static Hashtable statusOTA2MIDP = new Hashtable();
    static String jadMime = "text/vnd.sun.j2me.app-descriptor";

    public Descriptor createDescriptor( String url ) throws OTAException
    {
        Descriptor d = new Descriptor( getSchema(), url );

        try
        {

            URL jadURL = new URL( url );

            URLConnection conn = jadURL.openConnection();

            if ( ( ( HttpURLConnection )conn ).getResponseCode() !=
                 HttpURLConnection.HTTP_OK )
            {
                throw new OTAException( "Http response isn't OK "+
                          ( (HttpURLConnection )conn ).getResponseCode() );
            }

            String mimeType = conn.getContentType();

            // System.err.println( "debug : jad mimetype is " + mimeType );
        
            if ( (mimeType == null ) || !mimeType.equalsIgnoreCase( jadMime ) )
            {
              throw new OTAException( "Content type for the JAD URL" +
                                      "is not " + jadMime + "\n" + url );
            }

            InputStream in = conn.getInputStream();
            LineNumberReader pr =
              new LineNumberReader( new InputStreamReader( in ) );

            Hashtable missed = new Hashtable();

            while ( true )
            {

                String prop = pr.readLine();
                if ( prop == null )
                {
                    break;
                }

                int idx = prop.indexOf( ':' );

                if ( idx <= 1)
                {
                    throw new OTAException( "Jad file format error, line:\n"+
                                            prop );
                }

                String name = prop.substring(0, idx ).trim().toLowerCase();
                String value = prop.substring( idx+1).trim();

                if ( "midlet-jar-size".equals( name ) )
                {
                    d.setSize( value );
                }
                else if ( "midlet-jar-url".equals( name ) )
                {
                    d.setObjectURI( value );
                }
                else if ( "midlet-version".equals( name ) )
                {
                  d.setVersion( value );
                }
                else if ( "midlet-install-notify".equals( name ) )
                {
                    d.setInstallNotifyURI( value );
                }
                else if ( "midlet-name".equals( name ) )
                {
                    d.setName( value );
                }
                else if ( "midlet-description".equals( name ) )
                {
                    d.setDescription( value );
                }
                else if ( "midlet-vendor".equals( name ) )
                {
                    d.setVendor( value );
                }
                else
                {
                    missed.put( name, value );
                }
            }

            int no = 1;

            while ( true )
            {
                String key;
                String val = ( String )missed.get( key = "midlet-" + no );
                if ( val == null )
                {
                    break;
                }

                int idx1 = val.indexOf( ',' );
                int idx2 = val.lastIndexOf( ',' );
                if ( (idx1 < 0)||( idx1==idx2) )
                {
                  throw new OTAException( "Invalid midlet reference"+val );
                 }

                MidletApplication app = new MidletApplication(
                                               val.substring(0, idx1),
                                               val.substring( idx1+1, idx2 ),
                                               val.substring( idx2+1) );
                missed.remove( key );
                d.addApplication( app );
            }
            
            d.checkOut();
        }
        catch ( SyntaxException e )
        {
            e.printStackTrace(); 
            throw new OTAException( "Descriptor is invalid" );
        }
        catch ( OTAException e )
        {
            throw e;
        }
        catch ( Throwable e )
        {
            e.printStackTrace();
            throw new OTAException( "I/O trouble:\n"+e.toString() );
        }
        return d;
    }

    public boolean sendNotify( String notifyURL, String statusCode,
           String statusMsg )
    {

        try
        {
            if ( ( notifyURL == null )||"".equals( notifyURL ) )
            {
                return false;
            }

            statusCode = ( String )statusOTA2MIDP.get( statusCode );

            if ( (statusCode == null )||"".equals( statusCode ) )
            {
                return false;
            }

            System.out.println( "InstallNotifyURL: " + notifyURL );

            URL url = new URL( notifyURL );
   
            // Open a connection to the install-notiy URL 
            HttpURLConnection huc = ( HttpURLConnection ) url.openConnection();

            // This operation sends a POST request
            huc.setDoOutput( true );

            // System.out.println( "StatusCode=" + statusCode );
            // System.out.println( "StatusMsg=" + statusMsg );

            // Write the status code and message to the URL
            OutputStream os = huc.getOutputStream();
            String content = statusCode + " " + statusMsg;
            byte [] buf = content.getBytes(); 
            os.write( buf );
            os.flush();
            os.close();

            if ( huc.getResponseCode() != HttpURLConnection.HTTP_OK )
            {
                throw new OTAException( "Http response is not OK "+
                                        huc.getResponseCode() );
            }
            else
            {
                System.out.println( "RESPONSE code: " + huc.getResponseCode() +
                                    " " + huc.getResponseMessage() );
            }

            huc.disconnect();

        }
        catch ( Exception e )
        {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    static
    {
        statusOTA2MIDP.put( ST_SUCCESS, ST_SUCCESS );
        statusOTA2MIDP.put( ST_INSUFFICIENTMEMORY, ST_INSUFFICIENTMEMORY );
        statusOTA2MIDP.put( ST_USERCANCELLED, ST_USERCANCELLED );
        statusOTA2MIDP.put( ST_LOSSOFSERVICE, ST_LOSSOFSERVICE );
        statusOTA2MIDP.put( ST_SIZEMISMATCH, ST_SIZEMISMATCH );
        statusOTA2MIDP.put( ST_ATTRIBUTEMISMATCH, ST_ATTRIBUTEMISMATCH );
        statusOTA2MIDP.put( ST_INVALIDDESCRIPTOR, ST_INVALIDDESCRIPTOR );
        statusOTA2MIDP.put( ST_INVALIDDDVERSION, ST_INVALIDDESCRIPTOR );
        statusOTA2MIDP.put( ST_DEVICEABORTED, ST_USERCANCELLED );
        statusOTA2MIDP.put( ST_NONACCEPTABLECONTENT, ST_INVALIDDESCRIPTOR );
        statusOTA2MIDP.put( ST_LOADERERROR, ST_INSUFFICIENTMEMORY );
    }

    public String getSchema()
    {
        return "midp";
    }
    
}
