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

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.module.download.*;
import com.sun.jump.common.JUMPAppModel;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.LineNumberReader;
import java.io.OutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.File;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
import java.util.Vector;

public class MIDPOTADownloadModule 
implements JUMPDownloadModule {

    // From the JUMPModule interface.  
    public void load(Map config) {}
    public void unload() {}

    public static Hashtable statusOTA2MIDP = new Hashtable();
    static String jadMime = "text/vnd.sun.j2me.app-descriptor";

    public JUMPDownloader createDownloader(
        JUMPDownloadDescriptor descriptor)
        throws JUMPDownloadException {

        return new DownloaderImpl(descriptor);
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
        
    public JUMPDownloadDescriptor createDescriptor(String url)
        throws JUMPDownloadException {

        MIDPDownloadDescriptor d = new MIDPDownloadDescriptor(getSchema(), url);

        try
        {

            URL jadURL = new URL(encode(url));

            URLConnection conn = jadURL.openConnection();
            if (jadURL.getProtocol().equals("http") &&
                ((HttpURLConnection)conn).getResponseCode() 
                                != HttpURLConnection.HTTP_OK) {
                throw new JUMPDownloadException("Bad Http response code: "+
                          ((HttpURLConnection)conn).getResponseCode());
            }

            String mimeType = conn.getContentType();

            if (DownloadModuleFactoryImpl.verbose) {
                System.err.println("debug : jad mimetype is " + mimeType);
            }
            
            if (!jadURL.getProtocol().equals("file") && 
                        (mimeType == null || !mimeType.equalsIgnoreCase(jadMime))) {
              throw new JUMPDownloadException("Content type for the JAD URL" +
                                      " is not " + jadMime + "\n" + url);
            }

	    /**
	     * First, write out the .jad as a temp file.
	     * The installer may use the stored jad data to perform
	     * additional content verificiation.
	     */
	    
            InputStream in = conn.getInputStream();
            LineNumberReader pr =
              new LineNumberReader(new InputStreamReader(in));

	    File jadFile = File.createTempFile("midlet", ".jad");
	    PrintWriter outputStream = 
		     new PrintWriter(new FileWriter(jadFile));

            String l;
	    while ((l = pr.readLine()) != null) {
	       outputStream.println(l);
            }

	    pr.close();
	    outputStream.close();

	    // Now, let's read back the data. 
            pr = new LineNumberReader(new FileReader(jadFile));

            Hashtable missed = new Hashtable();

            while (true) {

                String prop = pr.readLine();
                if (prop == null || prop.equals("")) {
                    break;
                }

                int idx = prop.indexOf(':');

                if (idx <= 1) {
                    throw new JUMPDownloadException("Jad file format error, line:\n"+ prop);
                }

                String name = prop.substring(0, idx).trim().toLowerCase();
                String value = prop.substring(idx+1).trim();

                if ("midlet-jar-size".equals(name)) {
                    d.setSize(Integer.parseInt(value));
                } else if ("midlet-jar-url".equals(name)) {
                    d.setObjectURI(value);
                } else if ("midlet-version".equals(name)) {
                  d.setVersion(value);
                } else if ("midlet-install-notify".equals(name)) {
                    d.setInstallNotifyURI(value);
                } else if ("midlet-name".equals(name)) {
                    d.setName(value);
                } else if ("midlet-description".equals(name)) {
                    d.setDescription(value);
                } else if ("midlet-vendor".equals(name)) {
                    d.setVendor(value);
                } else {
                    missed.put(name, value);
                }
            }

	    // Done with parsing.  Close the jad file.  
	    pr.close();

            int no = 1;

            Vector applications = new Vector();
            
            while (true) {

                String key;
                String val = (String)missed.get(key = "midlet-" + no);
                if (val == null) {
                    break;
                }

                int idx1 = val.indexOf(',');
                int idx2 = val.lastIndexOf(',');
                if ((idx1 < 0) || (idx1 == idx2)) {
                   throw new JUMPDownloadException("Invalid midlet reference "+val);
                }

                String classname = val.substring(0, idx1);
                String iconpath = val.substring(idx1+1, idx2);
                String title = val.substring(idx2+1);
              
                // Set the properties for the application
                Properties props = new Properties();                
                props.setProperty("MIDLETApplication_initialClass", classname);
                props.setProperty(JUMPApplication.TITLE_KEY, title);
                props.setProperty(JUMPApplication.ICONPATH_KEY, iconpath);
                props.setProperty(JUMPApplication.APPMODEL_KEY, JUMPAppModel.MIDLET.getName());                
                props.setProperty("JUMPApplication_localJadUrl", jadFile.getCanonicalPath());                

                applications.add(props);
                
                d.setType(JUMPDownloadDescriptor.TYPE_APPLICATION);
                missed.remove(key);

            }
            
            Object appsArray[] = applications.toArray();
            Properties apps[] = new Properties[appsArray.length];
            for (int i = 0; i < appsArray.length; i++) {
                apps[i] = (Properties)appsArray[i];
            }            
            d.setApplications(apps);
            
            d.checkOut();
        }
        catch (SyntaxException e) {
            throw new JUMPDownloadException("ERROR: Descriptor is invalid");
        }
        catch (JUMPDownloadException e) {
            throw e;
        }
        catch (Throwable e) {
            throw new JUMPDownloadException("ERROR: I/O trouble:\n"+e.toString());
        }
        return d;
    }

    public boolean sendNotify(String notifyURL, String statusCode, 
		    String statusMsg) {

        try {
            if ((notifyURL == null)||"".equals(notifyURL)) {
                return false;
            }

            statusCode = (String)statusOTA2MIDP.get(statusCode);

            if ((statusCode == null)||"".equals(statusCode)) {
                return false;
            }

            if (DownloadModuleFactoryImpl.verbose) {
                System.out.println("InstallNotifyURL: " + notifyURL);
            }

            URL url = new URL(notifyURL);
   
            // Open a connection to the install-notiy URL 
            HttpURLConnection huc = (HttpURLConnection) url.openConnection();

            // This operation sends a POST request
            huc.setDoOutput(true);

            if (DownloadModuleFactoryImpl.verbose) {
                System.out.println("StatusCode=" + statusCode);
                System.out.println("StatusMsg=" + statusMsg);
            }

            // Write the status code and message to the URL
            OutputStream os = huc.getOutputStream();
            String content = statusCode + " " + statusMsg;
            byte [] buf = content.getBytes(); 
            os.write(buf);
            os.flush();
            os.close();

            if (huc.getResponseCode() != HttpURLConnection.HTTP_OK) {
                throw new JUMPDownloadException("Bad Http response code: "+
                                        huc.getResponseCode());
            } else {
                if (DownloadModuleFactoryImpl.verbose) {
                    System.out.println("RESPONSE code: " + huc.getResponseCode() +
                                       " " + huc.getResponseMessage());
                }
            }

            huc.disconnect();

        }
        catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    static
    {
        statusOTA2MIDP.put(DownloaderImpl.ST_SUCCESS, DownloaderImpl.ST_SUCCESS);
        statusOTA2MIDP.put(DownloaderImpl.ST_INSUFFICIENTMEMORY, DownloaderImpl.ST_INSUFFICIENTMEMORY);
        statusOTA2MIDP.put(DownloaderImpl.ST_USERCANCELLED, DownloaderImpl.ST_USERCANCELLED);
        statusOTA2MIDP.put(DownloaderImpl.ST_LOSSOFSERVICE, DownloaderImpl.ST_LOSSOFSERVICE);
        statusOTA2MIDP.put(DownloaderImpl.ST_SIZEMISMATCH, DownloaderImpl.ST_SIZEMISMATCH);
        statusOTA2MIDP.put(DownloaderImpl.ST_ATTRIBUTEMISMATCH, DownloaderImpl.ST_ATTRIBUTEMISMATCH);
        statusOTA2MIDP.put(DownloaderImpl.ST_INVALIDDESCRIPTOR, DownloaderImpl.ST_INVALIDDESCRIPTOR);
        statusOTA2MIDP.put(DownloaderImpl.ST_INVALIDDDVERSION, DownloaderImpl.ST_INVALIDDESCRIPTOR);
        statusOTA2MIDP.put(DownloaderImpl.ST_DEVICEABORTED, DownloaderImpl.ST_USERCANCELLED);
        statusOTA2MIDP.put(DownloaderImpl.ST_NONACCEPTABLECONTENT, DownloaderImpl.ST_INVALIDDESCRIPTOR);
        statusOTA2MIDP.put(DownloaderImpl.ST_LOADERERROR, DownloaderImpl.ST_INSUFFICIENTMEMORY);
    }

    public String getSchema() {
        return "midp";
    }
}
