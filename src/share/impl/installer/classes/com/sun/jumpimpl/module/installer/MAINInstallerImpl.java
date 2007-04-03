/*
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
import com.sun.jump.module.installer.JUMPInstallerModule;
import java.net.URL;
import java.util.Vector;

/**
 * MAINInstallerImpl subclasses XLETInstallerImpl because the behavior of both
 * the XLET and Main installers are very similar.  The methods defined below
 * are the minimun amount of methods needed to be overridden to match the
 * current expected behavior of main application installation.
 */
public class MAINInstallerImpl extends XLETInstallerImpl implements JUMPInstallerModule {
    protected final static String DESCRIPTOR_INITIALCLASS_KEY = "mainClass";
    
    /**
     *
     * Create a JUMPApplication object
     * @param bundle Name of application bundle that this application belongs to
     * @param clazz Initial class of application
     * @param classPathURL URL of the classpath of this application
     * @param title The user visible title of this application
     * @param iconPathURL URL to the path of the icon for this application
     * @return application object
     */
    protected JUMPApplication createJUMPApplicationObject(String bundle,
            String clazz, URL classPathURL, String title, URL iconPathURL, int id) {
        return new MAINApplication(contentStoreDir, bundle, clazz, classPathURL, title, iconPathURL, id);
    }
    
    /**
     * Return the app model described by this class.
     * @return the appmodel of the class
     */
    protected JUMPAppModel getInstallerAppModel() {
        return JUMPAppModel.MAIN;
    }
    
    /**
     * Get the key value used to retrive the initialClass value
     * @return string pertaining to key representing initialClass
     */
    protected String getInstallerInitialClassKey() {
        return "mainClass";
    }
    
    /**
     * Get the key value used to specify an application's initial class from Properties object
     * @return initial class key value
     */
    protected String getPropertyInstallerInitialClassKey() {
        return MAINApplication.INITIAL_CLASS_KEY;
    }
    
    /**
     * Get the name of the bundle this application belongs to
     * @param app application object
     * @return bundle Name of application bundle that this application belongs to
     */
    protected String getBundleName(JUMPApplication app) {
        MAINApplication mainApp = (MAINApplication)app;
        return mainApp.getBundle();
    }
    
    /**
     * Retrieve the application object belonging to the bundle
     * @param bundle Name of application bundle that this application belongs to
     * @return application objects within the bundle
     */
    protected JUMPApplication[] getAppsInBundle(String bundle) {
        JUMPApplication[] apps = (JUMPApplication[]) getInstalled();
        Vector appsVector = new Vector();
        for (int i = 0; i < apps.length; i++) {
            MAINApplication mainApp = (MAINApplication)apps[i];
            if (mainApp.getBundle().equals(bundle)) {
                appsVector.add(apps[i]);
            }
        }
        Object[] objs = appsVector.toArray();
        JUMPApplication[] bundleApps = new JUMPApplication[objs.length];
        for (int i = 0; i < objs.length; i++ ) {
            bundleApps[i] = (JUMPApplication)objs[i];
        }
        return bundleApps;
    }
    
    /**
     * Retrieve the classpath value of the appliation
     * @param app the application object
     * @return string value of classpath
     */
    protected String getAppClasspath(JUMPApplication app) {
        if (app == null) {
            return null;
        }
        MAINApplication mainApp = (MAINApplication)app;
        return mainApp.getClasspath().getFile();
    }
}