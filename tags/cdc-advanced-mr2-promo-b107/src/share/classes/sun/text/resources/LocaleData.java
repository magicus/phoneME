/*
 * 
 * @(#)LocaleData.java	1.36 06/10/10
 * 
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

/*
 * (C) Copyright Taligent, Inc. 1996, 1997 - All Rights Reserved
 * (C) Copyright IBM Corp. 1996 - 1998 - All Rights Reserved
 *
 * The original version of this source code and documentation
 * is copyrighted and owned by Taligent, Inc., a wholly-owned
 * subsidiary of IBM. These materials are provided under terms
 * of a License Agreement between Taligent and Sun. This technology
 * is protected by multiple US and International patents.
 *
 * This notice and attribution to Taligent may not be removed.
 * Taligent is a registered trademark of Taligent, Inc.
 *
 */

package sun.text.resources;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URL;
import java.net.URLClassLoader;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.Hashtable;
import java.util.Vector;
import java.util.zip.ZipEntry;
import java.io.File;
import java.util.zip.ZipInputStream;
import sun.misc.Launcher;

/**
 * Provides information about and access to resource bundles in the
 * sun.text.resources package.
 * This class now exists only to allow a way to get the list of available resources.  Even
 * this will be changing in the future.
 *
 * @author Asmus Freytag
 * @author Mark Davis
 * @version 1.34 01/23/03
 */

public class LocaleData {
    /**
     * Returns a list of the installed locales.
     * @param key A resource tag.  Currently, this parameter is ignored.  The obvious
     * intent, however,  is for getAvailableLocales() to return a list of only those
     * locales that contain a resource with the specified resource tag.
     *
     * <p>Before we implement this function this way, however, some thought should be
     * given to whether this is really the right thing to do.  Because of the lookup
     * algorithm, a NumberFormat, for example, is "installed" for all locales.  But if
     * we're trying to put up a list of NumberFormats to choose from, we may want to see
     * only a list of those locales that uniquely define a NumberFormat rather than
     * inheriting one from another locale.  Thus, if fr and fr_CA uniquely define
     * NumberFormat data, but fr_BE doesn't, the user wouldn't see "French (Belgium)" in
     * the list and would go for "French (default)" instead.  Of course, this means
     * "English (United States)" would not be in the list, since it is the default locale.
     * This might be okay, but might be confusing to some users.
     *
     * <p>In addition, the other functions that call getAvailableLocales() don't currently
     * all pass the right thing for "key," meaning that all of these functions should be
     * looked at before anything is done to this function.
     *
     * <p>We recommend that someone take some careful consideration of these issues before
     * modifying this function to pay attention to the "key" parameter.  --rtg 1/26/98
     */
    public static Locale[] getAvailableLocales(String key) {
        // creating the locale list is expensive, so be careful to do it
        // only once
        if (localeList == null) {
            synchronized(LocaleData.class) {
                if (localeList == null) {
                    localeList = createLocaleList();
                }
            }
        }

        Locale[] temp = new Locale[localeList.length];
        System.arraycopy(localeList, 0, temp, 0, localeList.length);
        return temp;
    }
    
    /**
     * Gets a sun.text.resources.LocaleElements resource bundle, using privileges
     * to allow accessing a sun.* package.
     */
    public static ResourceBundle getLocaleElements(Locale locale) {
        return getBundle("sun.text.resources.LocaleElements", locale);
    }
    
    /**
     * Gets a sun.text.resources.DateFormatZoneData resource bundle, using privileges
     * to allow accessing a sun.* package.
     */
    public static ResourceBundle getDateFormatZoneData(Locale locale) {
        return getBundle("sun.text.resources.DateFormatZoneData", locale);
    }
    
    private static ResourceBundle getBundle(final String baseName, final Locale locale) {
         return (ResourceBundle) AccessController.doPrivileged(new PrivilegedAction() {
            public Object run() {
                return ResourceBundle.getBundle(baseName, locale);
            }
        });
    }
    

    // ========== privates ==========

    private static Vector classPathSegments = new Vector();
    private static Locale[] localeList;
    private static final String PACKAGE = "sun.text.resources";
    private static final String PREFIX = "LocaleElements_";
    private static final char ZIPSEPARATOR = '/';

    private static Locale[] createLocaleList() {
        Locale[] locales;
        String classPath = 
	    (String) java.security.AccessController.doPrivileged(
             new sun.security.action.GetPropertyAction("sun.boot.class.path"));
        String s = 
	    (String) java.security.AccessController.doPrivileged(
             new sun.security.action.GetPropertyAction("java.class.path"));

	// Search combined system and application class path
	if (s != null && s.length() != 0) {
            if (classPath == null) {
                classPath = s;
            } else {
                classPath += File.pathSeparator + s;
            }
	}
        while (classPath != null && classPath.length() != 0) {
            int i = classPath.lastIndexOf(java.io.File.pathSeparatorChar);
            String dir = classPath.substring(i + 1);
            if (i == -1) {
                classPath = null;
            } else {
                classPath = classPath.substring(0, i);
            }
            classPathSegments.insertElementAt(dir, 0);
        }
        
        // add extensions from the extension class loader
        ClassLoader appLoader = Launcher.getLauncher().getClassLoader();
        URLClassLoader extLoader = (URLClassLoader) appLoader.getParent();
        if (extLoader != null) {
          URL[] urls = extLoader.getURLs();
          for (int i = 0; i < urls.length; i++) {
            classPathSegments.insertElementAt(urls[i].getPath(), 0);
          }
        }

        String[] classList = (String[])
	    java.security.AccessController.doPrivileged(
				    new java.security.PrivilegedAction() {
		public Object run() {
		    return getClassList(PACKAGE, PREFIX);
		}
	    });

        /* 
         * When all library classes are romized, no cdc.jar or foundation.jar
         * exists, so we would fail to create the correct locale data by just
         * searching through sun.boot.class.path and java.class.path.
         * To solve the problem, we use a generated java class, 
         * sun.misc.DefaultLocaleList. This class is generated at build time.
         * The generated class contains a list of default locale information 
         * gathered by parsing build-time class (romized classes) list.
         */
        int plen = PREFIX.length();
        localeList = new Locale[classList.length + 
                                sun.misc.DefaultLocaleList.list.length];
        /* 1. search sun.boot.class.path and java.class.path. */
        for (int i = 0; i < classList.length; i++) {
            localeList[i] = getLocale(classList[i].substring(plen));
        }

        /* 2. add locale information generated from romized classes if 
	 *    there is any 
         */
	for (int j = 0; j < sun.misc.DefaultLocaleList.list.length; j++) {
	    localeList[classList.length + j] = 
	        getLocale(sun.misc.DefaultLocaleList.list[j]);
	}
	return (localeList);
    }

    private static Locale getLocale(String s) {
        String lang = "";
        String region = "";
        String var = "";
        int p1 = s.indexOf('_');
	int p2 = 0;

        if (p1 == -1) {
            lang = s;
        } else {
            lang = s.substring(0, p1);
            p2 = s.indexOf('_', p1 + 1);
            if (p2 == -1) {
                region = s.substring(p1 + 1);
            } else {
                region = s.substring(p1 + 1, p2);
                if (p2 < s.length()) {
                    var = s.substring(p2 + 1);
                }
            }
        }

        return new Locale(lang, region, var);
    }

    /**
     * Walk through CLASSPATH and find class list from a package.
     * The class names start with prefix string
     * @param package name, class name prefix
     * @return class list in an array of String
     */
    private static String[] getClassList(String pkgName, String prefix) {
        Vector listBuffer = new Vector();
        String packagePath = pkgName.replace('.', File.separatorChar)
            + File.separatorChar;
        String zipPackagePath = pkgName.replace('.', ZIPSEPARATOR)
            + ZIPSEPARATOR;
        for (int i = 0; i < classPathSegments.size(); i++){
            String onePath = (String) classPathSegments.elementAt(i);
            File f = new File(onePath);
            if (!f.exists())
                continue;
            if (f.isFile())
                scanFile(f, zipPackagePath, listBuffer, prefix);
            else if (f.isDirectory()) {
                String fullPath;
                if (onePath.endsWith(File.separator))
                    fullPath = onePath + packagePath;
                else
                    fullPath = onePath + File.separatorChar + packagePath;
                File dir = new File(fullPath);
                if (dir.exists() && dir.isDirectory())
                    scanDir(dir, listBuffer, prefix);
            }
        }
        String[] classNames = new String[listBuffer.size()];
        listBuffer.copyInto(classNames);
        return classNames;
    }

    private static void addClass (String className, Vector listBuffer, String prefix) {
        if (className != null && className.startsWith(prefix)
                    && !listBuffer.contains(className))
            listBuffer.addElement(className);
    }

    private static String midString(String str, String pre, String suf) {
        String midStr;
        if (str.startsWith(pre) && str.endsWith(suf))
            midStr = str.substring(pre.length(), str.length() - suf.length());
        else
            midStr = null;
        return midStr;
    }

    private static void scanDir(File dir, Vector listBuffer, String prefix) {
        String[] fileList = dir.list();
        for (int i = 0; i < fileList.length; i++) {
            addClass(midString(fileList[i], "", ".class"), listBuffer, prefix);
        }
    }

    private static void scanFile(File f, String packagePath, Vector listBuffer,
                String prefix) {
        try {
            ZipInputStream zipFile = new ZipInputStream(new FileInputStream(f));
            boolean gotThere = false;
            ZipEntry entry;
            while ((entry = zipFile.getNextEntry()) != null) {
                String eName = entry.getName();
                if (eName.startsWith(packagePath)) {
                    gotThere = true;
                    if (eName.endsWith(".class")) {
                        addClass(midString(eName, packagePath, ".class"),
                                listBuffer, prefix);
                    }
                } else {
                    if (gotThere)    // Found the package, now we are leaving
                        break;
                }
            }
        } catch (FileNotFoundException e) {
            System.out.println("file not found:" + e);
        } catch (IOException e) {
            System.out.println("file IO Exception:" + e);
        } catch (Exception e) {
            System.out.println("Exception:" + e);
        }
    }
}
