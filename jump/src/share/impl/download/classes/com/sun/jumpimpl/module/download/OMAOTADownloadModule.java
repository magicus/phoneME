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

import com.sun.jump.module.download.*;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;

import java.net.URL;
import java.net.URLConnection;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
import java.util.Vector;
import java.io.OutputStream;

public class OMAOTADownloadModule //extends GenericDownloadModuleImpl 
    implements JUMPDownloadModule {

    // From the JUMPModule interface.  Do initialization and uninitialization here.
    public void load(Map config) {}
    public void unload() {}

    public JUMPDownloader createDownloader(
        JUMPDownloadDescriptor descriptor)
        throws JUMPDownloadException { 
 
        return new DownloaderImpl(descriptor);        
    }

    static String ddMime = "application/vnd.oma.dd+xml";

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

      try {

          URL ddURL = new URL(encode(url));
          URLConnection conn = ddURL.openConnection();
          conn.setRequestProperty("User-Agent", "CDC/FP 1.1 Appmanager");

          String mimeType = conn.getContentType();

          if (DownloadModuleFactoryImpl.verbose) {
              System.err.println("debug : xlet mimetype is " + mimeType);
          }

          if (!ddURL.getProtocol().equals("file") &&
                  (mimeType == null || !mimeType.equalsIgnoreCase(ddMime))) {
              throw new JUMPDownloadException("Content type for the DD URL"+
                                      " is not "+ ddMime + "\n" + url);
          }

          // load the descriptor
          Parser parser = new Parser();
          parser.parse( ddURL );
          Document dd = parser.getDocument();


          // parse the descriptor into Descriptor class.

          OMADownloadDescriptor d = new OMADownloadDescriptor(getSchema(), url);

          Iterator i = dd.getIterator();

          while (i.hasNext()) {
              DocumentElement de = (DocumentElement)i.next();

              String ename = de.getName();
              String eval = de.getValue();

              if (DownloadModuleFactoryImpl.verbose) {
                  System.out.println("ename="+ename+", eval="+eval);
              }

              if ("type".equals(ename)) {
                  d.setType(eval);
              } else if ("size".equals(ename)) {
                  d.setSize(Integer.parseInt(eval));
              } else if ("objectURI".equals(ename)) {
                  d.setObjectURI(eval);
              } else if ("installNotifyURI".equals(ename)) {
                  d.setInstallNotifyURI(eval);
              } else if ("nextURL".equals(ename)) {
                  d.setNextURI(eval);
              } else if ("name".equals(ename)) {
                  d.setName(eval);
              } else if ("ddx:display".equals(ename)) {
                  d.setDisplayName(eval);
              } else if ("description".equals(ename)) {
                  d.setDescription(eval);
              } else if ("vendor".equals(ename)) {
                  d.setVendor(eval);
              } else if ("infoURL".equals(ename)) {
                  d.setInfoURI(eval);
              } else if ("iconURI".equals(ename)) {
                  d.setIconURI(eval);
              } else if ("ddx:object".equals(ename)) {
                  extractObjects(d, de);
              } else if ("ddx:dependencies".equals(ename)) {
                  // Currently no dependency support
              } else if ("ddx:version".equals(ename)) {
                  d.setVersion(eval);
              } else if ("ddx:security".equals(ename)) { 
		  d.setSecurityLevel(eval);
              } else {
                  if (DownloadModuleFactoryImpl.verbose) {
                      System.out.println("Warning : unknown OMA tag : " +
                                          ename);
                  }
              }
          }

          // Check the descriptor for internal consistency.
          d.checkOut();

          return d;

      } catch (SyntaxException e) {
          throw new JUMPDownloadException("The descriptor file is invalid");
      } catch (JUMPDownloadException e) {
          throw e;
      } catch (Throwable e) {
          throw new JUMPDownloadException( "Unexpected error:"+e.getMessage() );
      }
    }

    private void extractObjects(OMADownloadDescriptor d,
                                DocumentElement de)
        throws Exception {

        if (de == null) {
            throw new NullPointerException("null DocumentElement!");
        }

        JUMPApplication ca = null;

        Vector elementVector = de.elements;
        Vector applications = new Vector();
        
        for (int i = 0 ; i < elementVector.size(); i++) {
            DocumentElement subElement =
                (DocumentElement)elementVector.get(i);

            String name = subElement.getName();
            if ("ddx:application".equals(name)) {

                d.setType(JUMPDownloadDescriptor.TYPE_APPLICATION);

                String iconpath  = subElement.getAttribute("icon"); 

                // Set the propreties of this application
                Properties props = new Properties();                
                props.setProperty("MAINApplication_initialClass", subElement.getAttribute( "classname" ));
                props.setProperty(JUMPApplication.TITLE_KEY, subElement.getAttribute( "name" ));
                props.setProperty(JUMPApplication.ICONPATH_KEY, iconpath);
                props.setProperty(JUMPApplication.APPMODEL_KEY, JUMPAppModel.MAIN.getName());
                applications.add(props);               

            } else if ("ddx:xlet".equals(name)) {

                d.setType(JUMPDownloadDescriptor.TYPE_APPLICATION);

                String iconpath  = subElement.getAttribute("icon"); 

                // Set the properites of this application
                Properties props = new Properties();                
                props.setProperty("XLETApplication_initialClass", subElement.getAttribute( "classname" ));
                props.setProperty(JUMPApplication.TITLE_KEY, subElement.getAttribute( "name" ));
                props.setProperty(JUMPApplication.ICONPATH_KEY, iconpath);
                props.setProperty(JUMPApplication.APPMODEL_KEY, JUMPAppModel.XLET.getName()); 
                applications.add(props);                

            } else if ("ddx:daemon".equals(name)) {

                // Currently unsupported
		
            } else if ("ddx:player".equals(name)) {

                // Currently unsupported
		
            } else if ("ddx:data".equals(name)) {

                d.setType(JUMPDownloadDescriptor.TYPE_DATA);
                //d.setData( subElement.getAttribute( "mimetype" ),
                //           subElement.getAttribute( "name" ) );
                ca = null;

            } else if ("ddx:library".equals(name)) {

                String type = subElement.getAttribute("type");
                d.setType(JUMPDownloadDescriptor.TYPE_LIBRARY);
                //d.addLibrary( type.equalsIgnoreCase( "java" ) );
                ca = null;

            } else if ("ddx:property".equals(name)) {

                if (ca == null) {
                    throw new SyntaxException("property w/o "+
                                              "application context");
                }
                ca.addProperty(subElement.getAttribute("name"),
                               subElement.getAttribute("value"));

            } else if ("ddx:mime".equals(name)) {
                // This allows us to map a mimetype to a
                // certain application, which will act as a
                // "player." Currently unsupported.
		
            } else if ("action".equals(name)) {

                // Currently unsupported
		
            } else {
                if (DownloadModuleFactoryImpl.verbose) {
                    System.out.println("Warning : unknown object tag " +
                                        name);
                }
                ca = null;
            }
        }
        
        Object appsArray[] = applications.toArray();
        Properties apps[] = new Properties[appsArray.length];
        for (int i = 0; i < appsArray.length; i++) {
            apps[i] = (Properties)appsArray[i];
        }
        d.setApplications(apps);
    }

    public boolean sendNotify(String notifyURL, String statusCode,
           String statusMsg) {

        try {
            if ((notifyURL == null) || "".equals(notifyURL)) {
                return false;
            }
 
            if (DownloadModuleFactoryImpl.verbose) {
                System.out.println("InstallNotifyURL: " + notifyURL);
            }

            URL url = new URL(notifyURL);

            // Open a connection to the install-notify URL
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
                throw new JUMPDownloadException("Http response is not OK "+
                                        huc.getResponseCode());
            }

            huc.disconnect();

        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public String getSchema() {
        return "oma";
    }
}

