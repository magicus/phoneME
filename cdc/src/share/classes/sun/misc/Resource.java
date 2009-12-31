/*
 * @(#)Resource.java	1.16 06/10/10
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

package sun.misc;

import java.net.URL;
import java.io.IOException;
import java.io.InputStream;
import java.util.jar.Manifest;
import java.util.jar.Attributes;

/**
 * This class is used to represent a Resource that has been loaded
 * from the class path.
 *
 * @author  David Connelly
 * @version 1.10, 02/02/00
 * @since   JDK1.2
 */
public abstract class Resource {
    /**
     * Returns the name of the Resource.
     */
    public abstract String getName();

    /**
     * Returns the URL of the Resource.
     */
    public abstract URL getURL();

    /**
     * Returns the CodeSource URL for the Resource.
     */
    public abstract URL getCodeSourceURL();

    /**
     * Returns an InputStream for reading the Resource data.
     */
    public abstract InputStream getInputStream() throws IOException;

    /**
     * Returns the length of the Resource data, or -1 if unknown.
     */
    public abstract int getContentLength() throws IOException;

    /**
     * Returns the Resource data as an array of bytes.
     */
    public byte[] getBytes() throws IOException {
	byte[] b;
        // Get stream before content length so that a FileNotFoundException
        // can propagate upwards without being caught too early
	InputStream in = getInputStream();
	int len = getContentLength();
	try {
	    if (len != -1) {
		// Read exactly len bytes from the input stream
		b = new byte[len];
		while (len > 0) {
		    int n = in.read(b, b.length - len, len);
		    if (n == -1) {
			throw new IOException("unexpected EOF");
		    }
		    len -= n;
		}
	    } else {
		// Read until end of stream is reached
		b = new byte[1024];
		int total = 0;
		while ((len = in.read(b, total, b.length - total)) != -1) {
		    total += len;
		    if (total >= b.length) {
			byte[] tmp = new byte[total * 2];
			System.arraycopy(b, 0, tmp, 0, total);
			b = tmp;
		    }
		}
		// Trim array to correct size, if necessary
		if (total != b.length) {
		    byte[] tmp = new byte[total];
		    System.arraycopy(b, 0, tmp, 0, total);
		    b = tmp;
		}
	    }
	} finally {
	    in.close();
	}
	return b;
    }
	
    /**
     * Returns the Manifest for the Resource, or null if none.
     */
    public Manifest getManifest() throws IOException {
	return null;
    }

    /**
     * Returns theCertificates for the Resource, or null if none.
     */
    public java.security.cert.Certificate[] getCertificates() {
	return null;
    }
}
