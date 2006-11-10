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
 * @version @(#)OMAOTA.java	1.31 05/09/23
 */

package com.sun.appmanager.impl.ota;

import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;
import java.util.Iterator;
import java.util.Vector;
import java.io.OutputStream;

import com.sun.appmanager.ota.Application;
import com.sun.appmanager.ota.Descriptor;
import com.sun.appmanager.ota.OTAException;
import com.sun.appmanager.ota.SyntaxException;

public class OMAOTA extends CDCAmsOTA
{
    static String ddMime = "application/vnd.oma.dd+xml";

    public Descriptor createDescriptor( String url ) throws OTAException
    {

      try
      {
          URL ddURL = new URL( url );
          URLConnection conn = ddURL.openConnection();
          conn.setRequestProperty( "User-Agent", "CDC/FP 1.1 Appmanager" );

          String mimeType = conn.getContentType();

          //System.err.println( "debug : xlet mimetype is " + mimeType );

          if ( ( mimeType == null) || !mimeType.equalsIgnoreCase( ddMime ) )
          {
              throw new OTAException( "Content type for the DD URL"+
                                      "is not "+ ddMime + "\n" + url );
          }

          // load the descriptor
          Parser parser = new Parser();
          parser.parse( ddURL );
          Document dd = parser.getDocument();


          // parse the descriptor into Descriptor class.

          OMADescriptor d = new OMADescriptor( getSchema(), url );

          Iterator i = dd.getIterator();

          while ( i.hasNext() )
          {
              DocumentElement de = (DocumentElement)i.next();

              String ename = de.getName();
              String eval = de.getValue();

              // System.out.println( "ename="+ename+", eval="+eval);

              if ( "type".equals( ename ) )
              {
                  d.setType( eval );
              }
              else if ( "size".equals( ename ) )
              {
                  d.setSize( Integer.parseInt( eval ) );
              }
              else if ( "objectURI".equals( ename ) )
              {
                  d.setObjectURI( eval );
              }
              else if ( "installNotifyURI".equals( ename ) )
              {
                  d.setInstallNotifyURI( eval );
              }
              else if ( "nextURL".equals( ename ) )
              {
                  d.setNextURI( eval );
              }
              else if ( "name".equals( ename ) )
              {
                  d.setName( eval );
              }
              else if ( "ddx:display".equals( ename ) )
              {
                  d.setDisplayName( eval );
              }
              else if ( "description".equals( ename ) )
              {
                  d.setDescription( eval );
              }
              else if ( "vendor".equals( ename ) )
              {
                  d.setVendor( eval );
              }
              else if ( "infoURL".equals( ename ) )
              {
                  d.setInfoURI( eval );
              }
              else if ( "iconURI".equals( ename ) )
              {
                  d.setIconURI( eval );
              }
              else if ( "ddx:object".equals( ename ) )
              {
                  extractObjects( d, de );
              }
              else if ( "ddx:dependencies".equals( ename ) )
              {
                  // Currently no dependency support
              }
              else if ( "ddx:version".equals( ename ) )
              {
                  d.setVersion( eval );
              }
              else if ( "ddx:security".equals(ename) ) {
                  d.setSecurityLevel(eval);
              }
              else
              {
                  System.out.println( "Warning : unknown OMA tag : " +
                                      ename );
              }
          }

          // Check the descriptor for internal consistency.
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

    private void extractObjects( Descriptor d,
                                 DocumentElement de )
        throws Exception
    {

        if ( de == null )
        {
            throw new NullPointerException( "null DocumentElement!" );
        }

        Application ca = null;

        Vector elementVector = de.elements;
        for ( int i = 0 ; i < elementVector.size(); i++ )
        {
            DocumentElement subElement =
                (DocumentElement)elementVector.get( i );

            String name = subElement.getName();
            if ( "ddx:application".equals( name ) )
            {
                d.setAppManagerType( Descriptor.TYPE_APP );
                ca = new MainApplication(
                                subElement.getAttribute( "name" ),
                                subElement.getAttribute( "icon" ),
                                subElement.getAttribute( "classname" ),
                                subElement.getAttribute( "classpath" ) );
                d.addApplication( ca );
            }
            else if ( "ddx:xlet".equals( name ) )
            {
                d.setAppManagerType( Descriptor.TYPE_APP );
                ca = new XletApplication(
                                subElement.getAttribute( "name" ),
                                subElement.getAttribute( "icon" ),
                                subElement.getAttribute( "classname" ),
                                subElement.getAttribute( "classpath" ) );
                d.addApplication( ca );
            }
            else if ( "ddx:daemon".equals( name ) )
            {
                // Currently unsupported
            }
            else if ( "ddx:player".equals( name ) )
            {
                // Currently unsupported
            }
            else if ( "ddx:data".equals( name ) )
            {
                d.setAppManagerType( Descriptor.TYPE_DATA );
                d.addData( subElement.getAttribute( "mimetype" ),
                           subElement.getAttribute( "name" ) );
                ca = null;
            }
            else if ( "ddx:library".equals( name ) )
            {
                String type = subElement.getAttribute( "type" );
                d.setAppManagerType( Descriptor.TYPE_LIB );
                d.addLibrary( type.equalsIgnoreCase( "java" ) );
                ca = null;
            }
            else if ( "ddx:property".equals( name ) )
            {
                if ( ca == null )
                {
                    throw new SyntaxException( "property w/o "+
                                               "application context" );
                }
                ca.addProperty( subElement.getAttribute( "name" ),
                                subElement.getAttribute( "value" ) );
            }
            else if ( "ddx:mime".equals( name ) )
            {
                // This allows us to map a mimetype to a
                // certain application, which will act as a
                // "player." Currently unsupported.
            }
            else if ( "action".equals( name ) )
            {
                // Currently unsupported
            }
            else
            {
                System.out.println( "Warning : unknown object tag " +
                                    name );
                ca = null;
            }
        }
    }

    public boolean sendNotify( String notifyURL, String statusCode,
           String statusMsg )
    {
        try
        {
            if ( (notifyURL == null ) || "".equals( notifyURL ) )
            {
                return false;
            }

            // System.out.println( "InstallNotifyURL: " + notifyURL );

            URL url = new URL( notifyURL );

            // Open a connection to the install-notify URL
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

            huc.disconnect();

        }
        catch ( Exception e )
        {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public String getSchema()
    {
        return "oma";
    }
}
