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

/*
 * @(#)OtaTest.java	1.7 05/10/20
 *
 * A program to contact a JSR-124 implementation server, retrieve a
 * list of downloads available, and then download anything
 * the user has selected. This is a very unsophisticated
 * example which shows how to use the OTA download classes.
 * Command-line args: "-d" for verbosity
 *                     "-i" to be updated by the ota layer
 *                          about status
 */

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.IOException;
import java.lang.Thread;
import java.util.Enumeration;
import java.util.Hashtable;

// CR6327718. We must extend AppManager in order to properly
// initialize the necessary ResourceBundle.
import com.sun.appmanager.AppManager;

import com.sun.appmanager.ota.Descriptor;
import com.sun.appmanager.ota.Destination;
import com.sun.appmanager.ota.DLIndicator;
import com.sun.appmanager.ota.OTA;
import com.sun.appmanager.ota.OTAException;
import com.sun.appmanager.ota.OTAFactory;

import com.sun.appmanager.impl.ota.CDCAmsOTAFactory;

public class OtaTest extends AppManager implements Destination, DLIndicator
{
    private static final String DU_PROP = 
        "com.sun.appmanager.ota.discoveryUrl";
    private static final String DISCOVERY_URL =
        "http://maxfield.sun.com:8000/ri-test/oma";
    private static final String OF_PROP =
        "com.sun.appmanager.ota.outputFile";
    private static final String OUTPUT_FILE = "output";

    private static boolean debug = false;
    private static boolean useIndicator = false;

    private boolean downloadFinished = false;
    private boolean downloadAborted = false;

    OTA omaOTA = null;
    String outputFile = null;
    String discoveryUrl = null;

    OtaTest()
    {
        // trivial constructor
    }

    /*
     * For verbosity, set "-d" on the command line.
     */
    static void trace( String s )
    {
        if ( debug )
        {
            System.out.println( s );
        }
        return;
    }

    void doIt()
    {
        // Determine the discovery URL
        discoveryUrl = System.getProperty( DU_PROP, DISCOVERY_URL );
        trace( "using discovery URL: " + discoveryUrl );

        // Output file
        outputFile = System.getProperty( OF_PROP, OUTPUT_FILE );
        trace( "saving downloaded output to " + outputFile );

	OTAFactory factory = (OTAFactory)(new CDCAmsOTAFactory());
        omaOTA = factory.getImpl( "OMA" );

        // Discover returns a hashtable. Right now we assume it will
        // be keyed by descriptor, and each value will be a textual name
        Hashtable h = omaOTA.discover( discoveryUrl );
        String[] downloads = new String[ h.size() ];
        String[] downloadNames = new String[ h.size() ];
        int i = 0;
        for ( Enumeration e = h.keys(); e.hasMoreElements(); )
        {
            String s = (String)e.nextElement();
            downloads[ i ] = s;
            downloadNames[ i ] = (String)h.get( s );
            trace( "key " + downloads[ i ] +
                   ", value " +
                   downloadNames[ i ] );
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

        // Initiate a download. We've specified ourselves
        // as the handler of the data.
        startDownload( downloads[ chosenDownload ] );

        // Wait for either failure or success
        while ( downloadFinished == false &&
                downloadAborted == false )
        {
            trace( "waiting for download" );
            try
            {
                Thread.sleep( 1 );
            }
            catch ( java.lang.InterruptedException ie )
            {
                // Eat it
            }
        }

        // Some resolution
        if ( ! downloadFinished )
        {
            System.out.println( "ack! download failed!" );
        }
        else
        {
            System.out.println( "download succeeded. save the results" );
            try
            {
                File f = new File( outputFile );
                FileOutputStream fos = new FileOutputStream( f );
                fos.write( buffer );
                fos.close();
            }
            catch ( Exception e )
            {
                e.printStackTrace();
            }
        }
    }

    void startDownload( String uri )
    {
        trace( "creating descriptor for " + uri );
        try
        {
            Descriptor d = omaOTA.createDescriptor( uri );

            buffer = new byte[ d.getSize() ];

            // Trigger the download
            trace( "download returns " +
                   omaOTA.download( d,
                                    this,
                                    ( useIndicator ? this : null ) ) );
        }
        catch ( OTAException o )
        {
            System.out.println( "download failed for " + uri +
                                ": " + o.getMessage() );
            o.printStackTrace();
        }
        return;
    }

    public static void main( String[] args )
    {
        // Parse any args
        for ( int i = 0 ; i < args.length ; i++ )
        {
            if ( "-d".equals( args[ i ] ) )
            {
                debug=true;
            }
            else if ( "-i".equals( args[ i ] ) )
            {
                useIndicator = true;
            }
        }
        (new OtaTest()).doIt();
        return;
    }
    

    // Begin Destination implementation

    byte[] buffer = null;
    int bufferIndex = 0;

    public void acceptMimeType(String mimeType) throws OTAException
    {
        OtaTest.trace( "saying we handle mimetype " + mimeType );
        return;
    }

    public void start( String sourceURL,
                       String mimeType ) throws OTAException, IOException
    {
        OtaTest.trace( "download is about to start from " + sourceURL +
                       ", of type " + mimeType );
        return;
    }

    public int receive(InputStream in, int length ) 
	throws OTAException, IOException
    {
        OtaTest.trace( "receiving data " );
        int numRead = in.read(buffer, bufferIndex, length);
	if (numRead > 0) {
	    bufferIndex += numRead;
	}
	return numRead;
    }

    public void finish() throws OTAException, IOException
    {
        downloadFinished = true;
        OtaTest.trace( "download is finished" );
        return;
    }

    public void abort()
    {
        downloadAborted = true;
        OtaTest.trace( "download aborted" );
        return;
    }

    public int getMaxChunkSize()
    {
        OtaTest.trace( "saying we'll take any chunksize" );
        return 0;
    }

    // End Destination implementation

    // Begin DLIndicator implementation

    public int getGranularity()
    {
        return 10;
    }

    public void updatePercent(int value)
    {
        trace( "DLIndicator update, value: " + value );
        return;
    }

    public boolean isCancelled()
    {
        // What, us cancel the download?
        trace( "OTA is checking for a user cancel" );
        return false;
    }

    Object synchronizationObject = new Object();
    public Object getLockNotifier()
    {
        return synchronizationObject;
    }

    public void downloadDone()
    {
        trace( "OTA tells us the download is done (but maybe not 100%)" );
    }

    // End DLIndicator implementation
}
