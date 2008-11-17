/*
 * @(#)FileURLConnection.java	1.58 06/10/10
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

/**
 * Open an file input stream given a URL.
 * @author	James Gosling
 * @author	Steven B. Byrne
 * @version 	1.48, 05/03/00
 */

package sun.net.www.protocol.file;

import java.net.URL;
import java.net.FileNameMap;
import java.io.*;
/* Remove the sorting ability because of cutting
   the following java.text.* dependency.   
java/text/CharacterIterator
java/text/CollationElementIterator
java/text/CollationKey
java/text/CollationRules
java/text/Collator
java/text/CompactByteArray
java/text/CompactIntArray
java/text/CompactShortArray
java/text/EntryPair
java/text/IntHashtable
java/text/MergeCollation
java/text/Normalizer
java/text/PatternEntry
java/text/RBCollationTables
java/text/RBTableBuilder
java/text/RuleBasedCollator
java/text/StringCharacterIterator
import java.text.Collator; 
*/
import java.security.Permission;
import sun.net.*;
import sun.net.www.*;
import java.util.*;
import java.text.SimpleDateFormat;

import sun.security.action.GetPropertyAction;
import sun.security.action.GetIntegerAction;
import sun.security.action.GetBooleanAction;

public class FileURLConnection extends URLConnection {
    
    static String CONTENT_LENGTH = "content-length";
    static String CONTENT_TYPE = "content-type";
    static String TEXT_PLAIN = "text/plain";
    static String LAST_MODIFIED = "last-modified";

    String contentType;
    InputStream is;

    File file;
    String filename;
    boolean isDirectory = false;
    boolean exists = false;
    List files;

    long length = 0;
    long lastModified = 0;

    protected FileURLConnection(URL u, File file) {
	super(u);
	this.file = file;
    }

    /*  
     * Note: the semantics of FileURLConnection object is that the
     * results of the various URLConnection calls, such as
     * getContentType, getInputStream or getContentLength reflect
     * whatever was true when connect was called.  
     */
    public void connect() throws IOException {
	if (!connected) {
            try {
                String decodedPath = ParseUtil.decode(url.getPath());
                file = new File(decodedPath.replace('/', File.separatorChar));
                filename = file.toString();
                isDirectory = file.isDirectory();
                if (isDirectory) {
                    files = (List) Arrays.asList(file.list());
                } else {
                   is = new BufferedInputStream(chainDecorator(new FileInputStream(filename)));
                }
            } catch (IOException e) {
                throw e;
            }
	    connected = true;
	}
    }

    /*
     * To be overridden by subclasses, e.g. Java Plug-in.
     */
    protected InputStream chainDecorator(InputStream s)
    {
	return s;
    }

    private boolean initializedHeaders = false;

    private void initializeHeaders() {
	try {
	    connect();
	    exists = file.exists();
	} catch (IOException e) {
	}
	if (!initializedHeaders || !exists) {
	    length = file.length();
	    lastModified = file.lastModified();

	    if (!isDirectory) {
		FileNameMap map = java.net.URLConnection.getFileNameMap();
		contentType = map.getContentTypeFor(filename);
		if (contentType != null) {
		    properties.add(CONTENT_TYPE, contentType);
		}
		properties.add(CONTENT_LENGTH, String.valueOf(length));

		/*
		 * Format the last-modified field into the preferred 
		 * Internet standard - ie: fixed-length subset of that
		 * defined by RFC 1123
		 */
		if (lastModified != 0) {
		    Date date = new Date(lastModified);
		    SimpleDateFormat fo = 
			new SimpleDateFormat ("EEE, dd MMM yyyy HH:mm:ss 'GMT'", Locale.US);
		    fo.setTimeZone(TimeZone.getTimeZone("GMT"));
		    properties.add(LAST_MODIFIED, fo.format(date));
		}
	    } else {
		properties.add(CONTENT_TYPE, TEXT_PLAIN);
	    }
	    initializedHeaders = true;
	}
    }

    public String getHeaderField(String name) {
	initializeHeaders();
	return super.getHeaderField(name);
    }

    public String getHeaderField(int n) {
	initializeHeaders();
	return super.getHeaderField(n);
    }

    public int getContentLength() {
        initializeHeaders();
        return super.getContentLength();
    }

    public String getHeaderFieldKey(int n) {
	initializeHeaders();
	return super.getHeaderFieldKey(n);
    }

    public MessageHeader getProperties() {
	initializeHeaders();
	return super.getProperties();
    }

    public synchronized InputStream getInputStream()
	throws IOException {

	int iconHeight;
	int iconWidth;

	connect();

	if (is == null) {
	    iconHeight = ((Integer)java.security.AccessController.doPrivileged(
	            new GetIntegerAction("hotjava.file.iconheight",
					 32))).intValue();
	    iconWidth = ((Integer)java.security.AccessController.doPrivileged(
	            new GetIntegerAction("hotjava.file.iconwidth",
					 32))).intValue();
	    if (isDirectory) {
		FileNameMap map = java.net.URLConnection.getFileNameMap();

		StringBuffer buf = new StringBuffer();

		if (files == null) {
		    throw new FileNotFoundException(filename);
		}

                /* Comment this out because of dependency cut. 4/5/2000 
                 * Collections.sort(files, Collator.getInstance()); 
                 */

		buf.append("<title>");
		buf.append((String)java.security.AccessController.doPrivileged(
		        new GetPropertyAction("file.dir.title", 
					      "Directory Listing")));
		buf.append("</title>\n");
		buf.append("<base href=\"file://localhost/");
		buf.append(filename.substring((filename.charAt(0) == '/') ? 1 : 0));
		if (filename.endsWith("/")) {
		    buf.append("\">");
		} else {
		    buf.append("/\">");
		}
		buf.append("<h1>");
		buf.append(filename);
		buf.append("</h1>\n");
		buf.append("<hr>\n");

		Boolean tmp;
		tmp = (Boolean) java.security.AccessController.doPrivileged(
                        new GetBooleanAction("file.hidedotfiles"));
		boolean hideDotFiles = tmp.booleanValue();
		for (int i = 0 ; i < files.size() ; i++) {
                    String fileName = (String)files.get(i);

		    if (hideDotFiles) {
			if (fileName.indexOf('.') == 0) {
			    continue;
			}
		    }
 
		    buf.append("<img align=middle src=\"");
		    if (new File(filename + "/" + fileName).isDirectory()) {
			buf.append(MimeEntry.defaultImagePath +
				   "/directory.gif\" width="+iconWidth+
				   " height="+iconHeight+">\n");
		    } else {
			String imageFileName = MimeEntry.defaultImagePath +
			    "/file.gif";

			if (map instanceof MimeTable) {
			    MimeEntry entry = 
				((MimeTable)map).findByFileName(fileName);
			    if (entry != null) {
				String realImageName = 
				    entry.getImageFileName();
				if (realImageName != null) {
				    imageFileName = realImageName;
				}
			    }
			}
		    
			buf.append(imageFileName);
			buf.append("\" width="+iconWidth+" height="+iconHeight+
				   ">\n");
		    }
		    buf.append("<a href=\"");
		    buf.append(fileName);
		    buf.append("\">");
		    buf.append(fileName);
		    buf.append("</a><br>");
		}
		// Put it into a (default) locale-specific byte-stream.
		is = new ByteArrayInputStream(buf.toString().getBytes());
	    } else {
		throw new FileNotFoundException(filename);
	    }
	}
	return is;
    }

    Permission permission;

    /* since getOutputStream isn't supported, only read permission is
     * relevant 
     */
    public Permission getPermission() throws IOException {
	if (permission == null) {
            String decodedPath = ParseUtil.decode(url.getPath());
	    if (File.separatorChar == '/') {
		permission = new FilePermission(decodedPath, "read");
	    } else {
		permission = new FilePermission(
			decodedPath.replace('/',File.separatorChar), "read");
	    }
	}
	return permission;
    }
}




