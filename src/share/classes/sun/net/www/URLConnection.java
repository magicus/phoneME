/*
 * @(#)URLConnection.java	1.36 06/10/10
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

package sun.net.www;

import java.net.URL;
import java.net.ContentHandler;
import java.util.*;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.BufferedInputStream;
import java.net.UnknownServiceException;

/**
 * A class to represent an active connection to an object
 * represented by a URL.
 * @author  James Gosling
 */

abstract public class URLConnection extends java.net.URLConnection {

    /** The URL that it is connected to */

    private String contentType;
    private int contentLength = -1;

    protected MessageHeader properties;

    /** Create a URLConnection object.  These should not be created directly:
	instead they should be created by protocol handers in response to
	URL.openConnection.
	@param	u	The URL that this connects to.
     */
    public URLConnection (URL u) {
	super(u);
	properties = new MessageHeader();
    }

    /** Call this routine to get the property list for this object.
     * Properties (like content-type) that have explicit getXX() methods
     * associated with them should be accessed using those methods.  */
    public MessageHeader getProperties() {
	return properties;
    }

    /** Call this routine to set the property list for this object. */
    public void setProperties(MessageHeader properties) {
	this.properties = properties;
    }

    public void setRequestProperty(String key, String value) {
	if(connected)
	    throw new IllegalAccessError("Already connected");
	if (key == null)
	    throw new NullPointerException ("key cannot be null");
	properties.set(key, value);
    }


    public String getHeaderField(String name) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	return properties == null ? null : properties.findValue(name);
    }

    /**
     * Return the key for the nth header field. Returns null if
     * there are fewer than n fields.  This can be used to iterate
     * through all the headers in the message.
     */
    public String getHeaderFieldKey(int n) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	MessageHeader props = properties;
	return props == null ? null : props.getKey(n);
    }

    /**
     * Return the value for the nth header field. Returns null if
     * there are fewer than n fields.  This can be used in conjunction
     * with getHeaderFieldKey to iterate through all the headers in the message.
     */
    public String getHeaderField(int n) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	MessageHeader props = properties;
	return props == null ? null : props.getValue(n);
    }

    /** Call this routine to get the content-type associated with this
     * object.
     */
    public String getContentType() {
	if (contentType == null)
	    contentType = getHeaderField("content-type");
	if (contentType == null) {
	    String ct = null;
	    try {
		ct = guessContentTypeFromStream(getInputStream());
	    } catch(java.io.IOException e) {
	    }
	    String ce = properties.findValue("content-encoding");
	    if (ct == null) {
		ct = properties.findValue("content-type");

		if (ct == null)
		    if (url.getFile().endsWith("/"))
			ct = "text/html";
		    else
			ct = guessContentTypeFromName(url.getFile());
	    }

	    /*
	     * If the Mime header had a Content-encoding field and its value
	     * was not one of the values that essentially indicate no
	     * encoding, we force the content type to be unknown. This will
	     * cause a save dialog to be presented to the user.  It is not
	     * ideal but is better than what we were previously doing, namely
	     * bringing up an image tool for compressed tar files.
	     */

	    if (ct == null || ce != null &&
		    !(ce.equalsIgnoreCase("7bit")
		      || ce.equalsIgnoreCase("8bit")
		      || ce.equalsIgnoreCase("binary")))
		ct = "content/unknown";
	    contentType = ct;
	}
	return contentType;
    }

    /**
     * Set the content type of this URL to a specific value.
     * @param	type	The content type to use.  One of the
     *			content_* static variables in this
     *			class should be used.
     *			eg. setType(URL.content_html);
     */
    public void setContentType(String type) {
	contentType = type;
    }

    /** Call this routine to get the content-length associated with this
     * object.
     */
    public int getContentLength() {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return -1;
	}
	int l = contentLength;
	if (l < 0) {
	    try {
		l = Integer.parseInt(properties.findValue("content-length"));
		contentLength = l;
	    } catch(Exception e) {
	    }
	}
	return l;
    }

    /** Call this routine to set the content-length associated with this
     * object.
     */
    protected void setContentLength(int length) {
	contentLength = length;
    }

    /**
     * Returns true if the data associated with this URL can be cached.
     */
    public boolean canCache() {
	return url.getFile().indexOf('?') < 0;
    }

    /**
     * Call this to close the connection and flush any remaining data.
     * Overriders must remember to call super.close()
     */
    public void close() {
	url = null;
    }
}
