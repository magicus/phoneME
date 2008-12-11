/*
 * @(#)SymbianFileSystem.java	1.4 06/10/10
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


class SymbianFileSystem extends FileSystem {

    private final char slash;
    private final char altSlash;
    private final char semicolon;

    public SymbianFileSystem() {
	slash = ((String) AccessController.doPrivileged(
              new GetPropertyAction("file.separator"))).charAt(0);
	semicolon = ((String) AccessController.doPrivileged(
              new GetPropertyAction("path.separator"))).charAt(0);
	altSlash = (this.slash == '\\') ? '/' : '\\';
    }

    private boolean isSlash(char c) {
	return (c == '\\') || (c == '/');
    }

    private boolean isLetter(char c) {
	return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'));
    }

    private String slashify(String p) {
	if ((p.length() > 0) && (p.charAt(0) != slash)) return slash + p;
	else return p;
    }


    /* -- Normalization and construction -- */

    public char getSeparator() {
	return slash;
    }

    public char getPathSeparator() {
	return semicolon;
    }

    /* A normal Win32 pathname contains no duplicate slashes, except possibly
       for a UNC prefix, and does not end with a slash.  It may be the empty
       string.  Normalized Win32 pathnames have the convenient property that
       the length of the prefix almost uniquely identifies the type of the path
       and whether it is absolute or relative:

           0  relative to both drive and directory
	   1  drive-relative (begins with '\\')
	   2  absolute UNC (if first char is '\\'),
	        else directory-relative (has form "z:foo")
	   3  absolute local pathname (begins with "z:\\")
     */

    private int normalizePrefix(String path, int len, StringBuffer sb) {
	int src = 0;
	while ((src < len) && isSlash(path.charAt(src))) src++;
	char c;
	if ((len - src >= 2)
	    && isLetter(c = path.charAt(src))
	    && path.charAt(src + 1) == ':') {
	    /* Remove leading slashes if followed by drive specifier.
	       This is necessary to support file URLs containing drive
	       specifiers (e.g., "file://c:/path").  As a side effect,
	       "/c:/path" can be used as an alternative to "c:/path". */
	    sb.append(c);
	    sb.append(':');
	    src += 2;
	} else {
	    src = 0;
	    if ((len >= 2)
		&& isSlash(path.charAt(0))
		&& isSlash(path.charAt(1))) {
		/* UNC pathname: Retain first slash; leave src pointed at
		   second slash so that further slashes will be collapsed
		   into the second slash.  The result will be a pathname
		   beginning with "\\\\" followed (most likely) by a host
		   name. */
		src = 1;
		sb.append(slash);
	    }
	}
	return src;
    }

    /* Normalize the given pathname, whose length is len, starting at the given
       offset; everything before this offset is already normal. */
    private String normalize(String path, int len, int off) {
	if (len == 0) return path;
	if (off < 3) off = 0;	/* Avoid fencepost cases with UNC pathnames */
	int src;
	char slash = this.slash;
	StringBuffer sb = new StringBuffer(len);

	if (off == 0) {
	    /* Complete normalization, including prefix */
	    src = normalizePrefix(path, len, sb);
	} else {
	    /* Partial normalization */
	    src = off;
	    sb.append(path.substring(0, off));
	}

	/* Remove redundant slashes from the remainder of the path, forcing all
	   slashes into the preferred slash */
	while (src < len) {
	    char c = path.charAt(src++);
	    if (isSlash(c)) {
		while ((src < len) && isSlash(path.charAt(src))) src++;
		if (src == len) {
		    /* Check for trailing separator */
		    int sn = sb.length();
		    if ((sn == 2) && (sb.charAt(1) == ':')) {
			/* "z:\\" */
			sb.append(slash);
			break;
		    }
		    if (sn == 0) {
			/* "\\" */
			sb.append(slash);
			break;
		    }
		    if ((sn == 1) && (isSlash(sb.charAt(0)))) {
			/* "\\\\" is not collapsed to "\\" because "\\\\" marks
		           the beginning of a UNC pathname.  Even though it is
		           not, by itself, a valid UNC pathname, we leave it as
		           is in order to be consistent with the win32 APIs,
		           which treat this case as an invalid UNC pathname
		           rather than as an alias for the root directory of
		           the current drive. */
			sb.append(slash);
			break;
		    }
		    /* Path does not denote a root directory, so do not append
		       trailing slash */
		    break;
		} else {
		    sb.append(slash);
		}
	    } else {
		sb.append(c);
	    }
	}

	String rv = sb.toString();
	return rv;
    }

    /* Check that the given pathname is normal.  If not, invoke the real
       normalizer on the part of the pathname that requires normalization.
       This way we iterate through the whole pathname string only once. */
    public String normalize(String path) {
	int n = path.length();
	char slash = this.slash;
	char altSlash = this.altSlash;
	char prev = 0;
	for (int i = 0; i < n; i++) {
	    char c = path.charAt(i);
	    if (c == altSlash)
		return normalize(path, n, (prev == slash) ? i - 1 : i);
	    if ((c == slash) && (prev == slash) && (i > 1))
		return normalize(path, n, i - 1);
	    if ((c == ':') && (i > 1))
		return normalize(path, n, 0);
	    prev = c;
	}
	if (prev == slash) return normalize(path, n, n - 1);
	return path;
    }

    public int prefixLength(String path) {
	char slash = this.slash;
	int n = path.length();
	if (n == 0) return 0;
	char c0 = path.charAt(0);
	char c1 = (n > 1) ? path.charAt(1) : 0;
	if (c0 == slash) {
	    if (c1 == slash) return 2;	/* Absolute UNC pathname "\\\\foo" */
	    return 1;			/* Drive-relative "\\foo" */
	}
	if (isLetter(c0) && (c1 == ':')) {
	    if ((n > 2) && (path.charAt(2) == slash))
		return 3;		/* Absolute local pathname "z:\\foo" */
	    return 2;			/* Directory-relative "z:foo" */
	}
	return 0;			/* Completely relative */
    }

    public String resolve(String parent, String child) {
	char slash = this.slash;
	int pn = parent.length();
	if (pn == 0) return child;
	int cn = child.length();
	if (cn == 0) return parent;
	String c = child;
	if ((cn > 1) && (c.charAt(0) == slash)) {
	    if (c.charAt(1) == slash) {
		/* Drop prefix when child is a UNC pathname */
		c = c.substring(2);
	    } else {
		/* Drop prefix when child is drive-relative */
		c = c.substring(1);
	    }
	}
	String p = parent;
	if (p.charAt(pn - 1) == slash) p = p.substring(0, pn - 1);
	return p + slashify(c);
    }

    public String getDefaultParent() {
	return ("" + slash);
    }

    public String fromURIPath(String path) {
	String p = path;
	if ((p.length() > 2) && (p.charAt(2) == ':')) {
	    // "/c:/foo" --> "c:/foo"
	    p = p.substring(1);
	    // "c:/foo/" --> "c:/foo", but "c:/" --> "c:/"
	    if ((p.length() > 3) && p.endsWith("/"))
		p = p.substring(0, p.length() - 1);
	} else if ((p.length() > 1) && p.endsWith("/")) {
	    // "/foo/" --> "/foo"
	    p = p.substring(0, p.length() - 1);
	}
	return p;
    }



    /* -- Path operations -- */

    public boolean isAbsolute(File f) {
	int pl = f.getPrefixLength();
	return (((pl == 2) && (f.getPath().charAt(0) == slash))
		|| (pl == 3));
    }

    native String getDriveDirectory(int drive);

    private static String[] driveDirCache = new String[26];

    private static int driveIndex(char d) {
	if ((d >= 'a') && (d <= 'z')) return d - 'a';
	if ((d >= 'A') && (d <= 'Z')) return d - 'A';
	return -1;
    }

    private String getDriveDirectory(char drive) {
	int i = driveIndex(drive);
	if (i < 0) return null;
	String s = driveDirCache[i];
	if (s != null) return s;
	s = getDriveDirectory(i + 1);
	driveDirCache[i] = s;
	return s;
    }

    private String getUserPath() {
	/* For both compatibility and security,
	   we must look this up every time */
	return normalize(System.getProperty("user.dir"));
    }

    private String getDrive(String path) {
	int pl = prefixLength(path);
	return (pl == 3) ? path.substring(0, 2) : null;
    }

    public String resolve(File f) {
	String path = f.getPath();
	int pl = f.getPrefixLength();
	if ((pl == 2) && (path.charAt(0) == slash))
	    return path;			/* UNC */
	if (pl == 3)
	    return path;			/* Absolute local */
	if (pl == 0)
	    return getUserPath() + slashify(path); /* Completely relative */
	if (pl == 1) {				/* Drive-relative */
	    String up = getUserPath();
	    String ud = getDrive(up);
	    if (ud != null) return ud + path;
	    return up + path;			/* User dir is a UNC path */
	}
	if (pl == 2) {				/* Directory-relative */
	    String up = getUserPath();
	    String ud = getDrive(up);
	    if ((ud != null) && path.startsWith(ud))
		return up + slashify(path.substring(2));
	    char drive = path.charAt(0);
	    String dir = getDriveDirectory(drive);
	    String np;
	    if (dir != null) {
		/* When resolving a directory-relative path that refers to a
		   drive other than the current drive, insist that the caller
		   have read permission on the result */
		String p = drive + (':' + dir + slashify(path.substring(2)));
		SecurityManager security = System.getSecurityManager();
		try {
		    if (security != null) security.checkRead(p);
		} catch (SecurityException x) {
		    /* Don't disclose the drive's directory in the exception */
		    throw new SecurityException("Cannot resolve path " + path);
		}
		return p;
	    }
	    return drive + ":" + slashify(path.substring(2)); /* fake it */
	}
	throw new InternalError("Unresolvable path: " + path);
    }

    // Caches for canonicalization results to improve startup performance.
    // The first cache handles repeated canonicalizations of the same path
    // name. The prefix cache handles repeated canonicalizations within the
    // same directory, and must not create results differing from the true
    // canonicalization algorithm in canonicalize_md.c. For this reason the
    // prefix cache is conservative and is not used for complex path names.
    private ExpiringCache cache       = new ExpiringCache();
    private ExpiringCache prefixCache = new ExpiringCache();

    public String canonicalize(String path) throws IOException {
        if (!useCanonCaches) {
            return canonicalize0(path);
        } else {
            String res = cache.get(path);
            if (res == null) {
		String dir = null;
		String resDir = null;
                if (useCanonPrefixCache) {
                    dir = parentOrNull(path);
                    if (dir != null) {
                        resDir = prefixCache.get(dir);
                        if (resDir != null) {
                            // Hit only in prefix cache; full path is canonical
                            String filename = path.substring(1 + dir.length());
                            res = resDir + File.separatorChar + filename;
                            cache.put(dir + File.separatorChar + filename, res);
                        }
                    }
                }
                if (res == null) {
                    res = canonicalize0(path);
                    cache.put(path, res);
                    if (useCanonPrefixCache && dir != null) {
                        resDir = parentOrNull(res);
                        if (resDir != null) {
                            File f = new File(res);
                            if (f.exists() && !f.isDirectory()) {
                                prefixCache.put(dir, resDir);
                            }
                        }
                    }
                }
            }
            assert canonicalize0(path).equalsIgnoreCase(res);
            return res;
        }
    }
    protected native String canonicalize0(String path)
                                                throws IOException;
    // Best-effort attempt to get parent of this path; used for
    // optimization of filename canonicalization. This must return null for
    // any cases where the code in canonicalize_md.c would throw an
    // exception or otherwise deal with non-simple pathnames like handling
    // of "." and "..". It may conservatively return null in other
    // situations as well. Returning null will cause the underlying
    // (expensive) canonicalization routine to be called.
    static String parentOrNull(String path) {
        if (path == null) return null;
        char sep = File.separatorChar;
        char altSep = '/';
        int last = path.length() - 1;
        int idx = last;
        int adjacentDots = 0;
        int nonDotCount = 0;
        while (idx > 0) {
            char c = path.charAt(idx);
            if (c == '.') {
                if (++adjacentDots >= 2) {
                    // Punt on pathnames containing . and ..
                    return null;
                }
                if (nonDotCount == 0) {
                    // Punt on pathnames ending in a .
                    return null;
                }
            } else if (c == sep) {
                if (adjacentDots == 1 && nonDotCount == 0) {
                    // Punt on pathnames containing . and ..
                    return null;
                }
                if (idx == 0 ||
                    idx >= last - 1 ||
                    path.charAt(idx - 1) == sep ||
                    path.charAt(idx - 1) == altSep) {
                    // Punt on pathnames containing adjacent slashes
                    // toward the end
                    return null;
                }
                return path.substring(0, idx);
            } else if (c == altSep) {
                // Punt on pathnames containing both backward and
                // forward slashes
                return null;
            } else if (c == '*' || c == '?') {
                // Punt on pathnames containing wildcards
                return null;
            } else {
                ++nonDotCount;
                adjacentDots = 0;
            }
            --idx;
        }
        return null;
    }


    /* -- Attribute accessors -- */

    public native int getBooleanAttributes(File f);
    public native boolean checkAccess(File f, boolean write);
    public native long getLastModifiedTime(File f);
    public native long getLength(File f);


    /* -- File operations -- */

    public native boolean createFileExclusively(String path)
	throws IOException;
    public boolean delete(File f) {
        // Keep canonicalization caches in sync after file deletion
        // and renaming operations. Could be more clever than this
        // (i.e., only remove/update affected entries) but probably
        // not worth it since these entries expire after 30 seconds
        // anyway.
        cache.clear();
        prefixCache.clear();
        return delete0(f);
    }
    private native boolean delete0(File f);
    public synchronized native boolean deleteOnExit(File f);
    public native String[] list(File f);
    public native boolean createDirectory(File f);
    public boolean rename(File f1, File f2) {
        // Keep canonicalization caches in sync after file deletion
        // and renaming operations. Could be more clever than this
        // (i.e., only remove/update affected entries) but probably
        // not worth it since these entries expire after 30 seconds
        // anyway.
        cache.clear();
        prefixCache.clear();
        return rename0(f1, f2);
    }
    private native boolean rename0(File f1, File f2);
    public native boolean setLastModifiedTime(File f, long time);
    public native boolean setReadOnly(File f);


    /* -- Filesystem interface -- */

    private boolean access(String path) {
	try {
	    SecurityManager security = System.getSecurityManager();
	    if (security != null) security.checkRead(path);
	    return true;
	} catch (SecurityException x) {
	    return false;
	}
    }

    private static native int listRoots0();

    public File[] listRoots() {
	int ds = listRoots0();
	int n = 0;
	for (int i = 0; i < 26; i++) {
	    if (((ds >> i) & 1) != 0) {
		if (!access((char)('A' + i) + ":" + slash))
		    ds &= ~(1 << i);
		else
		    n++;
	    }
	}
	File[] fs = new File[n];
	int j = 0;
	char slash = this.slash;
	for (int i = 0; i < 26; i++) {
	    if (((ds >> i) & 1) != 0)
		fs[j++] = new File((char)('A' + i) + ":" + slash);
	}
	return fs;
    }


    /* -- Basic infrastructure -- */

    public int compare(File f1, File f2) {
	return f1.getPath().compareToIgnoreCase(f2.getPath());
    }

    public int hashCode(File f) {
	/* Could make this more efficient: String.hashCodeIgnoreCase */
	return f.getPath().toLowerCase().hashCode() ^ 1234321;
    }


    private static native void initIDs();

    static {
	initIDs();
    }

}
