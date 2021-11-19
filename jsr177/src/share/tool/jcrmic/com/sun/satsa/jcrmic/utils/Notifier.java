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

package com.sun.satsa.jcrmic.utils;

import java.util.*;
import java.io.*;

/**
 *  This class is responsible for reporting any messages (error, warning,
 *  progress, banner and status messages) during the compilation.
 */
public class Notifier {

    /**
     * The stream where error messages are printed.
     */
    OutputStream out;

    /**
     * Constructor.
     * @param out output stream
     */
    public Notifier(OutputStream out) {

        this.out = out;
    }

    /**
     * Output a message.
     * @param msg the message
     */
    public void output(String msg) {

        if (this.out instanceof PrintStream) {
            ((PrintStream) out).println(msg);
        } else {
            OutputStreamWriter osw = new OutputStreamWriter(this.out);
            PrintWriter pw = new PrintWriter(osw, true);
            pw.println(msg);
            try {
                osw.flush();
            } catch (IOException phooey) {
            }
        }
    }

    /**
     * Prints error message.
     * @param msg the message
     */
    public void error(String msg) {
        output(getText(msg));
    }

    /**
     * Prints error message.
     * @param msg the message
     * @param arg1 argument
     */
    public void error(String msg, String arg1) {
        output(getText(msg, arg1));
    }

    /**
     * Prints error message.
     * @param msg the message
     * @param arg1 argument
     * @param arg2 argument
     */
    public void error(String msg, String arg1, String arg2) {
        output(getText(msg, arg1, arg2));
    }

    /**
     * Prints error message.
     * @param msg the message
     * @param arg1 argument
     * @param arg2 argument
     * @param arg3 argument
     */
    public void error(String msg, String arg1, String arg2, String arg3) {
        output(getText(msg, arg1, arg2, arg3));
    }

    /**
     * Return the string value of a named resource in the rmic.properties
     * resource bundle.  If the resource is not found, null is returned.
     * @param key resource name
     * @return the value
     */
    public static String getString(String key) {

        if (!resourcesInitialized) {
            initResources();
        }

        try {
            return resources.getString(key);
        } catch (MissingResourceException ignore) {
        }
        return null;
    }

    /**
     * Flag that indicates that resoyrces were initialized.
     */
    private static boolean resourcesInitialized = false;

    /**
     * Resource bundle.
     */
    private static ResourceBundle resources;

    /**
     * Initializes resource bundle.
     */
    private static void initResources() {

        try {
            resources = ResourceBundle.getBundle("com/sun/satsa/jcrmic/jcrmic");
            resourcesInitialized = true;
        } catch (MissingResourceException e) {
            throw new Error("fatal: missing resource bundle: " +
                    e.getClassName());
        }
    }

    /**
     * Returns the string for given key.
     * @param key the key
     * @return the string
     */
    public static String getText(String key) {

        String message = getString(key);

        if (message == null) {
            message = "no text found: \"" + key + "\"";
        }

        return message;
    }

    /**
     * Returns the message.
     * @param key the key
     * @param num parameter
     * @return the message
     */
    public static String getText(String key, int num) {
        return getText(key, Integer.toString(num), null, null);
    }

    /**
     * Returns the message.
     * @param key the key
     * @param arg0 parameter
     * @return the message
     */
    public static String getText(String key, String arg0) {
        return getText(key, arg0, null, null);
    }

    /**
     * Returns the message.
     * @param key the key
     * @param arg0 parameter
     * @param arg1 parameter
     * @return the message
     */
    public static String getText(String key, String arg0, String arg1) {
        return getText(key, arg0, arg1, null);
    }

    /**
     * Returns the message.
     * @param key the key
     * @param arg0 parameter
     * @param arg1 parameter
     * @param arg2 parameter
     * @return the message
     */
    public static String getText(String key,
                                 String arg0, String arg1, String arg2) {

        String format = getString(key);
        if (format == null) {
            format = "no text found: " +
                    "key = \"{0}\", arguments = \"{1}\", \"{2}\", \"{3}\"";
        }

        String[] args = new String[3];
        args[0] = (arg0 != null ? arg0.toString() : "null");
        args[1] = (arg1 != null ? arg1.toString() : "null");
        args[2] = (arg2 != null ? arg2.toString() : "null");

        return java.text.MessageFormat.format(format, args);
    }
}
