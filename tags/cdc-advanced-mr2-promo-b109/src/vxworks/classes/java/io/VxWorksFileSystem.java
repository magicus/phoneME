/*
 * @(#)VxWorksFileSystem.java	1.9 06/10/10
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

package java.io;

import java.security.AccessController;
import sun.security.action.GetPropertyAction;


class VxWorksFileSystem extends FileSystem {

    private final char slash;
    private final char semicolon;

    public VxWorksFileSystem() {
        slash = 
	    ((String) AccessController.doPrivileged(
              new GetPropertyAction("file.separator"))).charAt(0);
	semicolon = 
	    ((String) AccessController.doPrivileged(
              new GetPropertyAction("path.separator"))).charAt(0);
    }


    /* -- Normalization and construction -- */

    public char getSeparator() {
	return slash;
    }

    public char getPathSeparator() {
	return semicolon;
    }

    /* A normal VxWorks pathname contains no duplicate slashes and does not end
       with a slash.  It may be the empty string. */

    /* Normalize the given pathname, whose length is len, starting at the given
       offset; everything before this offset is already normal. */
    private String normalize(String pathname, int len, int off) {
	if (len == 0) return pathname;
	int n = len;
	while ((n > 0) && (pathname.charAt(n - 1) == '/')) n--;
	if (n == 0) return "/";
	StringBuffer sb = new StringBuffer(pathname.length());
	if (off > 0) sb.append(pathname.substring(0, off));
	char prevChar = 0;
	for (int i = off; i < n; i++) {
	    char c = pathname.charAt(i);
	    if ((prevChar == '/') && (c == '/')) continue;
	    sb.append(c);
	    prevChar = c;
	}
	return sb.toString();
    }

    /* Check that the given pathname is normal.  If not, invoke the real
       normalizer on the part of the pathname that requires normalization.
       This way we iterate through the whole pathname string only once. */
    public String normalize(String pathname) {
	int n = pathname.length();
	if (n > 0 && pathname.charAt(0) == slash) {
	    // Remove the leading slash inserted by File.toURL(). 
	    // See bugid 4288740
	    String sub = pathname.substring(1);
	    if (isAbsolute(sub)) {
		String s = normalize(sub, n - 1, 0);
		return s;
	    }
	}
	char prevChar = 0;
	for (int i = 0; i < n; i++) {
	    char c = pathname.charAt(i);
	    if ((prevChar == '/') && (c == '/'))
		return normalize(pathname, n, i - 1);
	    prevChar = c;
	}
	if (prevChar == '/') return normalize(pathname, n, n - 1);
	return pathname;
    }

    public int prefixLength(String pathname) {
	int n = pathname.length();
	if (n == 0) return 0;
	// Support <hostname>:<path> pathnames
	int prefixEnd =  pathname.indexOf(':');
	int i = prefixEnd + 1;
	if (i < n && pathname.charAt(i) == slash) {
	    return i + 1;
	} else {
	    return i;
	}
    }

    public String resolve(String parent, String child) {
	if (child.equals("")) return parent;
	if (child.charAt(0) == '/') {
	    if (parent.equals("/")) return child;
	    return parent + child;
	}
	if (parent.equals("/")) return parent + child;
	return parent + '/' + child;
    }

    public String getDefaultParent() {
	return "/";
    }


    /* -- Path operations -- */

    private boolean isAbsolute(String pathname) {
	int pl = prefixLength(pathname);
        return pl > 0 ? (pathname.charAt(pl - 1) == slash) : false;
    }

    public boolean isAbsolute(File f) {
	return isAbsolute(f.getPath());
    }

    public String resolve(File f) {
	if (isAbsolute(f)) return f.getPath();
	return resolve(System.getProperty("user.dir"), f.getPath());
    }

    public native String canonicalize(String path) throws IOException;


    /* -- Attribute accessors -- */

    public native int getBooleanAttributes0(File f);

    public int getBooleanAttributes(File f) {
	int rv = getBooleanAttributes0(f);
	String name = f.getName();
	boolean hidden = (name.length() > 0) && (name.charAt(0) == '.');
	return rv | (hidden ? BA_HIDDEN : 0);
    }

    public native boolean checkAccess(File f, boolean write);
    public native long getLastModifiedTime(File f);
    public native long getLength(File f);


    /* -- File operations -- */

    public native boolean createFileExclusively(String path)
	throws IOException;
    public native boolean delete(File f);
    public synchronized native boolean deleteOnExit(File f);
    public native String[] list(File f);
    public native boolean createDirectory(File f);
    public native boolean rename(File f1, File f2);
    public native boolean setLastModifiedTime(File f, long time);
    public native boolean setReadOnly(File f);


    /* -- Filesystem interface -- */

    public File[] listRoots() {
	try {
	    SecurityManager security = System.getSecurityManager();
	    if (security != null) {
		security.checkRead("/");
	    }
	    return new File[] { new File("/") };
	} catch (SecurityException x) {
	    return new File[0];
	}
    }


    /* -- Basic infrastructure -- */

    public int compare(File f1, File f2) {
	return f1.getPath().compareTo(f2.getPath());
    }

    public int hashCode(File f) {
	return f.getPath().hashCode() ^ 1234321;
    }

    
    private static native void initIDs();

    static {
	initIDs();
    }

}
