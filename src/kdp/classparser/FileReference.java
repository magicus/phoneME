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

package kdp.classparser;

import java.io.File;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/** Abstraction of a reference to a file or an element of a JAR file
 *
 */

public abstract class FileReference {

    private static class BaseFileReference extends FileReference {
        private File file;

        BaseFileReference(File file) {
            this.file = file;
        }

        public InputStream getInputStream() throws IOException {
            return new FileInputStream(file);
        }

        public String toString() {
            return file.toString();
        }

        public boolean equals(Object o) {
            if (o == null || ! (o instanceof BaseFileReference)) {
                return false;
            } else {
                BaseFileReference fr = (BaseFileReference) o;
                return fr.file.equals(file);
            }
        }

        public boolean exists() {
            return file.exists();
        }

        public String[] list(FilenameFilter filter) {
            return file.list(filter);
        }
    }

    private static class JarFileReference extends FileReference {
        private JarFile jarFile;
        private String element;

        JarFileReference(JarFile jarFile, String element) {
            this.jarFile = jarFile;
            this.element = element.replace(File.separatorChar, '/');
        }

        JarFileReference(File file, String element) throws IOException {
            this(JarManager.openJar(file), element);
        }

        public InputStream getInputStream() throws IOException {
            return jarFile.getInputStream(jarFile.getEntry(element));
        }
        
        public String toString() {
            return jarFile.getName() + ":" + element;
        }

        public boolean equals(Object o) {
            if (o == null || ! (o instanceof JarFileReference)) {
                return false;
            } else {
                JarFileReference fr = (JarFileReference) o;
                return fr.jarFile.getName().equals(jarFile.getName())
                    && fr.element.equals(element);
            }
        }

        public boolean exists() {
            return jarFile.getEntry(element) != null;
        }

        public String[] list(FilenameFilter filter) {
            List files = new ArrayList();
            for (Enumeration e = jarFile.entries(); e.hasMoreElements();) {
                JarEntry je = (JarEntry) e.nextElement();
                String name = je.getName();
                if (name.startsWith(element)) {
                    name = name.substring(element.length());
                    if (name.startsWith("/")) {
                        name = name.substring(1);
                    }
                    int i = name.indexOf("/");
                    if (i != -1) {
                        name = name.substring(0, i);
                    }
                    if (filter.accept(new File(element), name)) {
                        files.add(name);
                    }
                }
            }
            return (String[]) files.toArray(new String[files.size()]);
        }
    }

    private static class NonExistentFileReference extends FileReference {
        private String description;
        NonExistentFileReference(String description) {
            this.description = description;
        }
        public boolean exists() { return false; }
        public String toString() { return description; }
        public InputStream getInputStream() throws IOException {
            throw new FileNotFoundException(description);
        }
        public String[] list(FilenameFilter _) { return new String[] { }; }
    }

    private static class JarManager {
        static Map jars = new TreeMap();
        
        static JarFile openJar(File file) throws IOException {
            JarFile jarFile = (JarFile) jars.get(file);
            if (jarFile == null) {
                jarFile = new JarFile(file);
                jars.put(file, jarFile);
            }
            return jarFile;
        }

    }

    private FileReference() { }
    
    public static FileReference create(File file) {
        return new BaseFileReference(file);
    }
    
    public static FileReference create(JarFile jarFile, String element) {
        return new JarFileReference(jarFile, element);
    }

    public static FileReference create(String base, String filename) {
        File baseFile = new File(base);
        if (baseFile.isDirectory()) {
            return new BaseFileReference(new File(baseFile, filename));
        } else {
            try {
                return new JarFileReference(baseFile, filename);
            } catch (IOException e) {
                System.err.println("Couldn't open JAR file " + baseFile);
                return new NonExistentFileReference(baseFile + ":" + filename);
            }
        }
    }

    public abstract InputStream getInputStream() throws IOException;

    public abstract String[] list(FilenameFilter filter);
    
    public abstract boolean exists();

} // FileReference
