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

package com.sun.j2me.content;

import com.sun.j2me.app.AppPackage;
import com.sun.j2me.app.AppModel;

/**
 * The class implements application attributes and permitions resolving.
 */
public class AppProxy {

    /**
     * The package abstraction for the current application.
     */
    private static final AppPackage appPackage = AppPackage.getInstance();

    /**
     * The current application Moidel.
     * The supported values are:
     * <ul>
     *  <li> @link com.sun.j2me.app.AppModel.MIDLET
     *  <li> @link com.sun.j2me.app.AppModel.XLET
     * </ul>
     */
    private static final int appModel = AppModel.getAppModel();

    /**
     * The apllication class identifier.
     * TODO: Desired to implement more reliable method for valid application identification.
     */
    private static final Class appClass;
    static {
        try {
            appClass = Class.forName(
                appModel == AppModel.MIDLET?
                "javax.microedition.midlet.MIDlet":
                "javax.microedition.xlet.Xlet");
        } catch (ClassNotFoundException cnfe) {
            throw new RuntimeException("Not knowm Application Model");
        }
    }

    /**
     * Tests if given class name represents valid application.
     *
     * @param classname tested class name.
     *
     * @return new created instance.
     *
     * @exception ClassNotFoundException if no class with given name
     * @exception IllegalArgumentException if given class name does not
     * represent valid application.
     */
    static AppProxy forClass(String classname) throws ClassNotFoundException  {
        if (!appClass.isAssignableFrom(Class.forName(classname))) {
            throw new IllegalArgumentException("The class is not valid application");
        }

        return new AppProxy(classname);
    }

    /** Properies enumeration. */
    static final int VENDOR_NAME_PROP = 1;
    static final int PACKAGE_NAME_PROP = 2;

    /** Default values for undefined properties */
    static final String INTERNAL_VENDOR_NAME = "internal";
    static final String SYSTEM_PACKAGE_NAME = "system";

    /** Permissions enumeration */
    static final int REGISTRY_PERMISSION = 1; // for regirster/unregister functions
    static final int LAUNCH_PERMISSION = 2;   // for application invocation request

    /** The current AppProxy classname */
    private final String classname;

    /**
     * Internal constructor.
     * @param classname
     */
    private AppProxy(String classname) {
        this.classname = classname;
    }

    /**
     * Tests if this application is properly installed.
     * @return result of the testing.
     */
    boolean isRegistered() {

        /* TODO: implement method. */

        return true;
    }

    /**
     * Retrieves installation property for the current application.
     *
     * @param property property enum:
     *      @link VENDOR_NAME_PROP or
     *      @link PACKAGE_NAME_PROP
     *
     * @return property value -- NOT null.
     */
    String getProperty(int property) {

        /* TODO: implement method */

        return property == VENDOR_NAME_PROP?
                INTERNAL_VENDOR_NAME : SYSTEM_PACKAGE_NAME;
    }

    /**
     * Generates default ID for the current application.
     *
     * <p>The ID uniquely identifies the application which contains the
     * content handler.
     * The application ID is assigned when the application is installed.
     * <p>
     * All installed applications have vendor and name;
     *
     * @return the ID; MUST NOT be <code>null</code>
     */
    String getDefaultID() {
        StringBuffer sb = new StringBuffer();
        char dash = '-';
        char undsc = '_';

        sb.append(getProperty(VENDOR_NAME_PROP).replace(' ', undsc))
          .append(dash)
          .append(getProperty(PACKAGE_NAME_PROP).replace(' ', undsc))
          .append(dash)
          .append(classname.replace('.', undsc));

        return sb.toString();
    }

    /**
     * Checks if the current application has trusted authority for
     * access restriction resolving.
     *
     * @return test result.
     */
    boolean isTrusted() {

        /** TODO: implement method */

        return true;
    }

    /**
     * Checks if the application is permitted to given activity.
     * @param permission request for specific activity permission.
     * @exception SecurityException if specific activity is not permitted.
     */
    void checkPermission(int permission) throws SecurityException {

        /** TODO: implement the method */

    }

    /**
     * Class name getter.
     * @return calssname.
     */
    String getClassname() {
        return classname;
    }
}
