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
                                                                                      
package com.sun.jumpimpl.module.download;

import com.sun.jump.module.download.*;
 
import java.io.InputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.util.Vector;

class DownloaderImpl implements JUMPDownloader {

    JUMPDownloadDescriptor descriptor;
    boolean isCancelled = false;

    // Error codes.
    public static final String ST_SUCCESS = "900";
    public static final String ST_INSUFFICIENTMEMORY = "901";
    public static final String ST_USERCANCELLED = "902";
    public static final String ST_LOSSOFSERVICE = "903";
    public static final String ST_SIZEMISMATCH = "904";
    public static final String ST_ATTRIBUTEMISMATCH = "905";
    public static final String ST_INVALIDDESCRIPTOR = "906";
    public static final String ST_INVALIDDDVERSION = "951";
    public static String ST_DEVICEABORTED = "952";
    public static String ST_NONACCEPTABLECONTENT = "953";
    public static String ST_LOADERERROR = "954";

    JUMPDownloadProgressNotifier report = new JUMPDownloadProgressNotifier();

    public DownloaderImpl( JUMPDownloadDescriptor descriptor ) {
        this.descriptor = descriptor;
    }

    public void setProgressListener(JUMPDownloadProgressListener listener) {
        report.setListener(listener);
    }
                                                                                      
    public void setProgressListener(JUMPDownloadProgressListener listener,
        int updatePercent) {
        report.setListener(listener, updatePercent);
    }
                                                                                      
    public URL start(JUMPDownloadDestination destination)
        throws JUMPDownloadException  {
      
        boolean downloadSucceeded = startDownload(descriptor.getObjectURI(), 
                                       descriptor.getInstallNotifyURI(),
                                       descriptor.getSize(),  
                                       destination);

	String fileExtension = null;

        if (descriptor.getType() == JUMPDownloadDescriptor.TYPE_APPLICATION) {
            fileExtension = ".jar";
        }

        if (downloadSucceeded) {
            try {
                File jarFile = File.createTempFile("content", fileExtension);
                FileOutputStream fos = new FileOutputStream( jarFile );
                fos.write( ((DownloadDestinationImpl)destination).getBuffer() );
                fos.close();

                return jarFile.toURI().toURL();
            } catch ( Exception e ) {
                e.printStackTrace();
                throw new JUMPDownloadException(e.toString());
            }
        }

        return null;
 
    }
    
    private String encode(String url) {
        // Change spaces to %20
        String encodedURL = "";
        for (int i = 0; i < url.length(); i++) {
            char c = url.charAt(i);
            if (c == ' ') {
                encodedURL += "%20";
            } else {
                encodedURL += c;
            }
        }
        return encodedURL;
    }    

    public boolean startDownload( String url, String nfUri,
                                int size, JUMPDownloadDestination store
                                ) throws JUMPDownloadException {
      

        int gran = 0;
        int pct = 0;

        try
        {
            if ( report != null )
            {
                gran = report.getGranularity();
            }

            URL objectUrl = new URL( encode(url) );
    
            if ( report != null )
            {
                report.updatePercent(0);
            }
            
            URLConnection conn = objectUrl.openConnection();
            
            if (objectUrl.getProtocol().equals("http") &&
                ( ( HttpURLConnection )conn ).getResponseCode() !=
                         HttpURLConnection.HTTP_OK )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri, ST_LOADERERROR,
                                "Can't process server response" );
                }
                
                throw new JUMPDownloadException( "Http response is not OK: "+
                             ( (HttpURLConnection )conn ).getResponseCode() );
            }
    
            String mimeType = conn.getContentType();
    
            if (DownloadModuleFactoryImpl.verbose) {
                System.err.println( "debug : download : mimetype is "+mimeType );
            }
    
/**
** mimetype evaluation is currently missing from the public API. 
**
**            try
**            {
**                store.acceptMimeType( mimeType );
**            }
**            catch ( JUMPDownloadException ee )
**            {
**                if ( nfUri != null )
**                {
**                    sendNotify( nfUri,
**                                ST_NONACCEPTABLECONTENT,
**                                "Unknown MIME type" );
**                }
**                if ( report != null )
**                {
**                    report.downloadDone();
**                }
**                throw ee;
**            }
**/
    
            InputStream in = conn.getInputStream();
            store.start( objectUrl, mimeType );
    
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
                throw new JUMPDownloadException( "Could only read " +
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
                throw new JUMPDownloadException( "Read past "+len+" bytes");
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
            if ( ( report != null ) && ( !isCancelled ) )
            {
                if ( nfUri != null )
                {
                    sendNotify( nfUri,
                                ST_LOADERERROR, "I/O error" );
                }
                report.downloadDone();
                throw new JUMPDownloadException( "I/O trouble:\n" +
                                    e.toString() );
            }
        }
    
        if ( (report != null ) && ( isCancelled ) )
        {
            report.downloadDone();
            return false;
        }
        return true;
    }

    public void cancel() {
        isCancelled = true;
        report.notifyCancelled();
    }
                                                                             
    /**
     * Returns the download descriptor that is associated with the
     * download action.
     */
    public JUMPDownloadDescriptor getDescriptor() {
        return descriptor;
    }


    public boolean sendNotify( String uri, String statusCode,
            String statusMsg ) {
        System.err.println("sendNotify() not supported " + statusMsg);

        return false;
    }


    class JUMPDownloadProgressNotifier {
     
        JUMPDownloadProgressListener listener;
        int granularity = 10;
   
        public void setListener(JUMPDownloadProgressListener listener) {
           this.listener = listener;
        }

        public void setListener(JUMPDownloadProgressListener listener, int percent) {
           this.listener = listener;
           granularity = percent;
        }

        public int getGranularity() {
           return granularity;
        }
  
        public void notifyCancelled() {
           if (listener != null)
              listener.downloadStarted();
        }
        
        public void downloadDone() {
           if (listener != null)
              listener.downloadCompleted();
        }
        public void updatePercent(int percent) {
           if (listener != null) {
              if (percent == 0) {
                  listener.downloadStarted();
              } 
              listener.dataDownloaded(percent);
           }
        }
    }
}
