/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.module.installer;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPApplication;
import java.net.MalformedURLException;
import java.net.URL;

/**
 * This is the representation class for a Main application.
 */
public class MAINApplication extends JUMPApplication {
    
    public static final String CLASSPATH_KEY = "MAINApplication_classpath";
    public static final String BUNDLE_KEY = "MAINApplication_bundle";
    public static final String INITIAL_CLASS_KEY = "MAINApplication_initialClass";
    private String repositoryDir = null;
    
    /**
     * Create an instance of an application.
     *
     * @param bundle The name of the bundle the application belongs to
     * @param clazz The class name of the application
     * @param classpath The path to the application
     * @param title The application's title, can be null
     * @param iconPath The location of the application's icon in, can be null
     */
    public MAINApplication(String repositoryDir, String bundle, String clazz, URL classpath, String title,
            URL iconPath, int id ) {
        
        super(title, iconPath, JUMPAppModel.MAIN, id);
        this.repositoryDir = repositoryDir;
        addProperty(INITIAL_CLASS_KEY, clazz);
        if (classpath != null) {
            addProperty(CLASSPATH_KEY, classpath.getFile());
        }
        addProperty(BUNDLE_KEY, bundle);
    }
    
    /**
     * Get the name of the bundle this application belongs to
     * @return the bundle this application belongs to
     */
    public String getBundle() {
        return getProperty(BUNDLE_KEY);
    }
    /**
     * Set the bundle name of the application.
     * @param bundle the bundle this application belongs to
     */
    public void setBundle(String bundle) {
        addProperty(BUNDLE_KEY, bundle);
    }
    /**
     * Set the classpath value of this application
     * @param classpath the classpath value
     */
    public void setClasspath(URL classpath) {
        if (classpath != null) {
            addProperty(CLASSPATH_KEY, classpath.getFile());
        }
    }
    /**
     * Get the classpath value of this application
     * @return the classpath value
     */
    public URL getClasspath() {
        URL url = null;
        try {
            url = new URL("file", null, getProperty(CLASSPATH_KEY));
        } catch (MalformedURLException ex) {
            ex.printStackTrace();
        }
        return url;
    }
    /**
     * Set the initial class of this application
     * @param initialClass the initial class of this application
     */
    public void setInitialClass(String initialClass) {
        addProperty(INITIAL_CLASS_KEY, initialClass);
    }
}
