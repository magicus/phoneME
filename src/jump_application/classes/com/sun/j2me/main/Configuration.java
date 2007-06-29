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

package com.sun.j2me.main;

import java.util.Properties;
import java.util.Hashtable;

/**
 * Intermediate class for getting internal properties
 */
public class Configuration {

    /** List of all internal properties. */
    private static Properties props = new Properties();

    /** Static native initialization. */
    static {
        initialize();
    }

    /** Don't let anyone instantiate this class. */
    private Configuration() {
    }

    /**
     * Returns internal property value by key.
     *
     * @param key property key.
     * @return property value.
     */
    public static String getProperty(String key) {
        return props.getProperty(key);
    }

    /**
     * Sets internal property value by key.
     *
     * @param key property key.
     * @param value property value.
     */
    public static void setProperty(String key, String value) {
        props.setProperty(key, value);
    }

    /**
     * Gets the implementation property indicated by the specified key or
     * returns the specified default value as an int.
     *
     * @param      key   the name of the implementation property.
     * @param      def   the default value for the property if not
     *                  specified in the configuration files or command
     *                  line over rides.
     *
     * @return     the int value of the implementation property,
     *             or <code>def</code> if there is no property with that key or
     *             the config value is not an int.
     *
     * @exception  NullPointerException if <code>key</code> is
     *             <code>null</code>.
     * @exception  IllegalArgumentException if <code>key</code> is empty.
     */
    public static int getIntProperty(String key, int def) {
        String prop = getProperty(key);
        if (prop == null) {
            return def;
        }

        try {
            int temp = Integer.parseInt(prop);
            return temp;
        } catch (NumberFormatException nfe) {
            // keep the default
        }

        return def;
    }

    /**
     * Performs native initialization necessary for this class.
     */
    private static native void initialize();
}
