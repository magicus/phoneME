/*
 * @(#)JarURLConnection.java	1.28 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

package sun.net.www.protocol.jar;

import java.io.InputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.BufferedInputStream;
import java.net.URL;
import java.net.URLConnection;
import java.net.MalformedURLException;
import java.net.UnknownServiceException;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.security.Permission;

/**
 * @author Benjamin Renaud
 * @since JDK1.2 
 */
public class JarURLConnection extends java.net.JarURLConnection {

    private static final boolean debug = false;

    /* the Jar file factory. It handles both retrieval and caching. 
     */
    private static JarFileFactory factory = new JarFileFactory();

    /* the url for the Jar file */
    private URL jarFileURL;

    /* the url connection for the JAR file */
    private URLConnection jarFileURLConnection;

    /* the entry name, if any */
    private String entryName;

    /* the JarEntry */
    private JarEntry jarEntry;

    /* the jar file corresponding to this connection */
    private JarFile jarFile;

    /* the content type for this connection */
    private String contentType;

    public JarURLConnection(URL url, Handler handler) 
    throws MalformedURLException, IOException {
	super(url);

	jarFileURL = getJarFileURL();
	jarFileURLConnection = jarFileURL.openConnection();
	entryName = getEntryName();
    }

    public JarFile getJarFile() throws IOException {
	connect();
	return jarFile;
    }

    public JarEntry getJarEntry() throws IOException {
	connect();
	return jarEntry;
    }
    
    public Permission getPermission() throws IOException {
	return jarFileURLConnection.getPermission();
    }

    public void connect() throws IOException {
	if (!connected) {
	    /* the factory call will do the security checks */
	    jarFile = factory.get(jarFileURL, getUseCaches());
	    
	    /* we also ask the factory the permission that was required
	     * to get the jarFile, and set it as our permission.
	     */
	    jarFileURLConnection = factory.getConnection(jarFile);
	    
	    if ((entryName != null)) {
	        jarEntry = (JarEntry)jarFile.getEntry(entryName);
		if (jarEntry == null) {
		    throw new FileNotFoundException("JAR entry " + entryName + 
						    " not found in " + 
						    jarFile.getName());
		}
	    }
	    connected = true;
	}	    
    }

    public InputStream getInputStream() throws IOException {
	connect();

	InputStream result = null;

	if (entryName == null) {
	    throw new IOException("no entry name specified");
	} else {
	    if (jarEntry == null) {
		throw new FileNotFoundException("JAR entry " + entryName + 
						" not found in " + 
						jarFile.getName());
	    }
	    result = jarFile.getInputStream(jarEntry);
	}
	return result;
    }

    public int getContentLength() {
	int result = -1;
	try {
	    connect();
	    if (jarEntry == null) {
		/* if the URL referes to an archive */
		result = jarFileURLConnection.getContentLength();
	    } else {
		/* if the URL referes to an archive entry */
		result = (int)getJarEntry().getSize();
	    }
	} catch (IOException e) {
	}
	return result;
    }

    public Object getContent() throws IOException {
	Object result = null;
	
	connect();
	if (entryName == null) {
	    result = jarFile;
	} else {
	    result = super.getContent();
	}
	return result;
    }

    public String getContentType() {
	if (contentType == null) {
	    if (entryName == null) {
		contentType = "x-java/jar";
	    } else {
		try {
		    connect();
		    InputStream in = jarFile.getInputStream(jarEntry);
		    contentType = guessContentTypeFromStream(
					new BufferedInputStream(in));
		    in.close();
		} catch (IOException e) {
		    // don't do anything
		}
	    }
	    if (contentType == null) {
		contentType = guessContentTypeFromName(entryName);
	    }
	    if (contentType == null) {
		contentType = "content/unknown";
	    }
	}
	return contentType;
    }

    public String getHeaderField(String name) {
	return jarFileURLConnection.getHeaderField(name);
    }

    /**
     * Sets the general request property. 
     *
     * @param   key     the keyword by which the request is known
     *                  (e.g., "<code>accept</code>").
     * @param   value   the value associated with it.
     */
    public void setRequestProperty(String key, String value) {
	jarFileURLConnection.setRequestProperty(key, value);
    }

    /**
     * Returns the value of the named general request property for this
     * connection.
     *
     * @return  the value of the named general request property for this
     *           connection.
     */
    public String getRequestProperty(String key) {
	return jarFileURLConnection.getRequestProperty(key);
    }

    /**
     * Set the value of the <code>allowUserInteraction</code> field of 
     * this <code>URLConnection</code>. 
     *
     * @param   allowuserinteraction   the new value.
     * @see     java.net.URLConnection#allowUserInteraction
     */
    public void setAllowUserInteraction(boolean allowuserinteraction) {
	jarFileURLConnection.setAllowUserInteraction(allowuserinteraction);
    }

    /**
     * Returns the value of the <code>allowUserInteraction</code> field for
     * this object.
     *
     * @return  the value of the <code>allowUserInteraction</code> field for
     *          this object.
     * @see     java.net.URLConnection#allowUserInteraction
     */
    public boolean getAllowUserInteraction() {
	return jarFileURLConnection.getAllowUserInteraction();
    }
    
    /* 
     * cache control 
     */

    /**
     * Sets the value of the <code>useCaches</code> field of this 
     * <code>URLConnection</code> to the specified value. 
     * <p>
     * Some protocols do caching of documents.  Occasionally, it is important
     * to be able to "tunnel through" and ignore the caches (e.g., the
     * "reload" button in a browser).  If the UseCaches flag on a connection
     * is true, the connection is allowed to use whatever caches it can.
     *  If false, caches are to be ignored.
     *  The default value comes from DefaultUseCaches, which defaults to
     * true.
     *
     * @see     java.net.URLConnection#useCaches
     */
    public void setUseCaches(boolean usecaches) {
	jarFileURLConnection.setUseCaches(usecaches);
    }

    /**
     * Returns the value of this <code>URLConnection</code>'s
     * <code>useCaches</code> field.
     *
     * @return  the value of this <code>URLConnection</code>'s
     *          <code>useCaches</code> field.
     * @see     java.net.URLConnection#useCaches
     */
    public boolean getUseCaches() {
	return jarFileURLConnection.getUseCaches();
    }

    /**
     * Sets the value of the <code>ifModifiedSince</code> field of 
     * this <code>URLConnection</code> to the specified value.
     *
     * @param   value   the new value.
     * @see     java.net.URLConnection#ifModifiedSince
     */
    public void setIfModifiedSince(long ifmodifiedsince) {
	jarFileURLConnection.setIfModifiedSince(ifmodifiedsince);
    }

   /**
     * Sets the default value of the <code>useCaches</code> field to the 
     * specified value. 
     *
     * @param   defaultusecaches   the new value.
     * @see     java.net.URLConnection#useCaches
     */
    public void setDefaultUseCaches(boolean defaultusecaches) {
	jarFileURLConnection.setDefaultUseCaches(defaultusecaches);
    }

   /**
     * Returns the default value of a <code>URLConnection</code>'s
     * <code>useCaches</code> flag.
     * <p>
     * Ths default is "sticky", being a part of the static state of all
     * URLConnections.  This flag applies to the next, and all following
     * URLConnections that are created.
     *
     * @return  the default value of a <code>URLConnection</code>'s
     *          <code>useCaches</code> flag.
     * @see     java.net.URLConnection#useCaches
     */
    public boolean getDefaultUseCaches() {
	return jarFileURLConnection.getDefaultUseCaches();
    }
}
