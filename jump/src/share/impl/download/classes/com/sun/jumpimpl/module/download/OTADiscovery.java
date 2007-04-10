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

public class OTADiscovery 
{
    static final String htmlMime = "text/html";

    public HashMap discover( String url )
    {

        try
        {
            URL netUrl = new URL( url );
            URLConnection conn = netUrl.openConnection();
            conn.connect();

            if (DownloadModuleFactoryImpl.verbose) {
                System.err.println( "debug : discover mimetype is "+
                                    conn.getContentType() );
            }

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

            HashMap h = new HashMap();

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
        return new HashMap();
    }
}
