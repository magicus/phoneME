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


// Test to check the download module.
// Discovery is done in the command-line mode.  The test lists the available
// apps from the server to the console, and downloads the app chosen by the user.

// Execute runTest() with either ota/midp or ota/oma param to test out download
// for each protocol.

import java.io.*;
import java.net.*;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
 
import com.sun.jump.module.download.*;

import com.sun.jumpimpl.module.download.OTADiscovery;
import com.sun.jumpimpl.module.download.DownloadDestinationImpl;

public class QuickTest {

    private static final String DU_PROP =
        "com.sun.jumpimpl.module.download.discoveryUrl";
    private static final String OF_PROP =
        "com.sun.jumpimpl.module.download.outputFile";
    private static String discovery_url = null;

    private static final String omaSubDirectory = "oma";
    private static final String midpSubDirectory = "jam";

    private static boolean debug = false;
    private static boolean useIndicator = false;

    private boolean downloadFinished = false;
    private boolean downloadAborted = false;

    String outputFile = null;
    String discoveryUrl = null;

    byte[] buffer = null;
    int bufferIndex = 0;

    public static void main(String[] args) {
       if (args == null || args.length < 1) {  
           System.err.println("Warning: no server URL as an argument");
           System.err.println("You can also set a system property " + DU_PROP + " to point to the server URL");
       } else {
           discovery_url = args[0];
       }
       new QuickTest();
    }

    public QuickTest() {
       setupTest();
       runTest(JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA);
       //runTest(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
    }

    void setupTest() { 

       // This one line should be called by the executive in real impl
       new com.sun.jumpimpl.module.download.DownloadModuleFactoryImpl();

       // Determine the discovery URL
       discoveryUrl = System.getProperty( DU_PROP, discovery_url );
       System.out.println( "Using discovery URL: " + discoveryUrl );
    }


    void runTest(String protocol) { 

       HashMap applist;

       System.out.println("Starting the test run for protocol " + protocol);

       if (protocol == JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA ||
           protocol == JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA ) {
          applist = new OTADiscovery().discover(discoveryUrl);
       } else {
          System.out.println("Unknown protocol, aborting");
          return;
       }

       String[] downloads = new String[ applist.size() ];
       String[] downloadNames = new String[ applist.size() ];
       int i = 0;
       for ( Iterator e = applist.keySet().iterator(); e.hasNext(); )
       {
           String s = (String)e.next();
           downloads[ i ] = s;
           downloadNames[ i ] = (String)applist.get( s );
           //System.out.println( "key " + downloads[ i ] +
           //       ", value " +
           //       downloadNames[ i ] );
           i++;
       }

       // Show what is available and read input for a choice.
       System.out.println( "download choices: " );
       for ( i = 0; i < downloadNames.length ; i++ )
       {
           System.out.println( "(" + i + "): " + downloadNames[ i ] );
       }

       int chosenDownload = -1;

       while ( true )
       {
           System.out.print( "Enter choice (-1 to exit) [-1]: " );
           BufferedReader in =
               new BufferedReader( new InputStreamReader( System.in ) );
           String answer;

           try
           {
               answer = in.readLine();
           }
           catch ( java.io.IOException ioe )
           {
               continue;
           }

           if ( "".equals( answer ) )
           {
               break;
           }

           try
           {
               chosenDownload = Integer.parseInt( answer );
               break;
           }
           catch ( Exception e )
           {
               // bad input
           }
       } 

        // If no valid choice, quit
        if ( chosenDownload < 0 )
        {
            System.exit( 0 );
        }

        System.out.println( chosenDownload );

        // Initiate a download. We've specified ourselves
        // as the handler of the data.
        startDownload( downloads[ chosenDownload ], protocol);

        // Wait for either failure or success
        while ( downloadFinished == false &&
                downloadAborted == false )
        {
            System.out.println( "waiting for download" );
            try
            {
                Thread.sleep( 100 );
            }
            catch ( java.lang.InterruptedException ie )
            {
                // Eat it
            }
        }

        // Some resolution
        if ( ! downloadFinished )
        {
            System.out.println( "Download failed!" );
        }
        else
        {
            System.out.println( "download succeeded. save the results" );
        }
    }

    void startDownload( String uri, String protocol )
    {
       downloadFinished = false;
       downloadAborted = false;
  
       System.out.println( "creating descriptor for " + uri );

       JUMPDownloadModule module = 
          JUMPDownloadModuleFactory.getInstance().getModule(protocol);

        try
        {

            JUMPDownloadDescriptor d = module.createDescriptor( uri );

            JUMPDownloader downloader = module.createDownloader(d);

            JUMPDownloadDestination destination = new DownloadDestinationImpl(d);

            // Trigger the download
            System.out.println( "download returns " +
                   downloader.start( destination ));

            downloadFinished = true;
        }
        catch ( JUMPDownloadException o )
        {
            System.out.println( "download failed for " + uri +
                                ": " + o.getMessage() );
            o.printStackTrace();
            downloadAborted = true;
        }
        catch ( Exception o )
        {
            System.out.println( "download failed for " + uri + 
                                ": " + o.getMessage() );
            o.printStackTrace();
            downloadAborted = true;
        }
        return;
    }
}
