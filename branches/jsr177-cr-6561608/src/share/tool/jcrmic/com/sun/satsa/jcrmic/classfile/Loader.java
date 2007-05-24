/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.satsa.jcrmic.classfile;

import com.sun.satsa.jcrmic.utils.Notifier;

import java.util.*;
import java.io.*;
import java.util.zip.*;

/**
 * This class is a class loader.
 */

public class Loader {

    /**
     * Class path.
     */
    private Vector classPath;

    /**
     * Loaded classes.
     */
    private Hashtable classes;

    /**
     * Errors/warnings notifier.
     */
    private Notifier notifier;

    /**
     * Constructor.
     * @param notifier errors/warnings notifier
     */
    public Loader(Notifier notifier) {

        classPath = new Vector();
        classes = new Hashtable();
        this.notifier = notifier;
    }

    /**
     * Set path for class loading.
     * @param path path for class loading
     * @return true if successfull
     */
    public boolean setClassPath(String path) {

        classPath.clear();

        StringTokenizer parser = new StringTokenizer(path, File.pathSeparator);

        while (parser.hasMoreTokens()) {

            String s = parser.nextToken();

            File f = new File(s);

            try {
                if (f.isDirectory()) {
                    // directory - add path to the vector
                    classPath.addElement(f.getCanonicalPath());
                } else {
                    // file - check that it exists
                    if (!f.exists()) {
                        notifier.error("rmic.file.not.found", s);
                        return false;
                    }

                    ZipFile z = new ZipFile(f);
                    classPath.addElement(z);
                }
            } catch (IOException e) {
                notifier.error("rmic.incorrect.classpath", s);
                return false;
            }
        }

        if (classPath.size() == 0) {
            return false;
        }

        return true;
    }

    /**
     * Loads the class.
     * @param className the class name
     * @return true if successfull
     */
    public boolean loadClass(String className) {

        className = className.replace('.', '/');

        // check to avoid duplicate loading

        if (classes.containsKey(className))
            return true;

        InputStream is = null;

        for (int i = 0; i < classPath.size(); i++) {

            if (classPath.elementAt(i) instanceof String) {

                String path = (String) classPath.elementAt(i);

                path += File.separator +
                        className.replace('/', File.separatorChar) +
                        ".class";

                File f = new File(path);

                if (!f.exists())
                    continue;

                try {
                    is = new FileInputStream(f);
                } catch (FileNotFoundException e) {
                    continue;
                }

            } else {

                ZipFile z = (ZipFile) classPath.elementAt(i);
                ZipEntry ze = z.getEntry(className + ".class");

                if (ze == null)
                    continue;

                try {
                    is = z.getInputStream(ze);
                } catch (IOException e) {
                    notifier.error("rmic.ioerror", z.getName());
                    return false;
                }
            }

            if (is != null)
                break;
        }

        if (is == null) {
            notifier.error("rmic.class.not.found", className);
            return false;
        }

        // class file is found, load it

        DataInputStream dis = new DataInputStream(
                new BufferedInputStream(is));

        JClass cl = new JClass(this);

        try {
            cl.parse(dis);
        } catch (Exception e) {
            notifier.error("rmic.parsing.error", className);
            return false;
        }

        // class is loaded

        classes.put(className, cl);

        // update the class, load all related classes

        if (!cl.update())
            return false;

        return true;
    }

    /**
     * Returns class definition loaded earlier.
     * @param className the class name
     * @return class definition
     */
    public JClass getClass(String className) {

        return (JClass) classes.get(className.replace('.', '/'));
    }
}
