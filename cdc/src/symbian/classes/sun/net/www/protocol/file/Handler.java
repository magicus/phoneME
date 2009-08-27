/*
 * @(#)Handler.java	1.3 06/10/10 
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

package sun.net.www.protocol.file;

import java.net.InetAddress;
import java.net.URLConnection;
import java.net.URL;
import java.net.MalformedURLException;
import java.net.URLStreamHandler;
import java.io.InputStream;
import java.io.IOException;
import sun.net.www.ParseUtil;
import java.io.File;

/**
 * Open an file input stream given a URL.
 * @author	James Gosling
 * @version 	1.41, 99/12/04
 */
public class Handler extends URLStreamHandler {

    private String getHost(URL url) {
	String host = url.getHost();
	if (host == null)
	    host = "";
	return host;
    }

    protected String toExternalForm(URL u) {
        StringBuffer result = new StringBuffer(u.getProtocol());
        result.append(":");
        if (u.getAuthority() != null) {
            result.append("//");
            result.append(u.getAuthority());
        } else {
            result.append("//");
	}
        if (u.getFile() != null) {
            result.append(u.getFile());
        }
        if (u.getRef() != null) {
            result.append("#");
            result.append(u.getRef());
        }
        return result.toString();
    }

    protected void parseURL(URL u, String spec, int start, int limit) {
	/*
	 * Ugly backwards compatibility. Flip any file separator
	 * characters to be forward slashes. This is a nop on Unix
	 * and "fixes" win32 file paths. According to RFC 2396,
	 * only forward slashes may be used to represent hierarchy
	 * separation in a URL but previous releases unfortunately
	 * performed this "fixup" behavior in the file URL parsing code
	 * rather than forcing this to be fixed in the caller of the URL
	 * class where it belongs. Since backslash is an "unwise"
	 * character that would normally be encoded if literally intended
	 * as a non-seperator character the damage of veering away from the
	 * specification is presumably limited.
	 */
	super.parseURL(u, spec.replace(File.separatorChar, '/'), start, limit);
    }

    public synchronized URLConnection openConnection(URL url)
           throws IOException {

        String path;
	String file = url.getFile();
	String host = url.getHost();

        path = ParseUtil.decode(file);
        path = path.replace('/', '\\');
        path = path.replace('|', ':');

	if ((host == null) || host.equals("") ||
		host.equalsIgnoreCase("localhost") ||
		host.equals("~")) {
           return createFileURLConnection(url, new File(path));
	}

	/*
	 * attempt to treat this as a UNC path. See 4180841
	 */
        path = "\\\\" + host + path;
 	File f = new File(path);
	if (f.exists()) {
            return createFileURLConnection(url, f);
	}

	/*
	 * Now attempt an ftp connection.
	 */
	/* Commented out, because CDC/Foundation doesn't support FTP
	URLConnection uc;
	URL newurl;

	try {
	    newurl = new URL("ftp", host, file +
			    (url.getRef() == null ? "":
			    "#" + url.getRef()));
	    uc = newurl.openConnection();
	} catch (IOException e) {
	    uc = null;
	}
	if (uc == null) {
	    throw new IOException("Unable to connect to: " +
					url.toExternalForm());
	}
	return uc;
	*/
	throw new IOException("URL connection with specified hostname " +
			      "is not supported. " + 
			      "Only localhost is supported.");
    }

    /**
     * Template method to be overriden by Java Plug-in. [stanleyh]
     */
    protected URLConnection createFileURLConnection(URL url, File file) {
	return new FileURLConnection(url, file);
    }
}
