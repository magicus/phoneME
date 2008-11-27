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

package com.sun.midp.util;

import com.sun.midp.midlet.MIDletStateHandler;

import javax.microedition.lcdui.Display;
import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;


/**
 * A MIDlet for use by i3tests.  This MIDlet includes utility
 * functions that start the midlet, activate it, and bring its display into
 * the foreground. Invoke this from a test class with the statement
 *
 * LcduiTestMIDlet.invoke();
 *
 * and clean it up with the statement
 *
 * LcduiTestMIDlet.cleanup();
 */
public class LcduiTestMIDlet extends MIDlet {

    static LcduiTestMIDlet midlet;
    static Display display;

    static Object lock;
    static boolean canvasPainted;
    static LcduiTestCanvas cv;

    /**
     * Starts the test MIDlet and waits for it to be created, activated, and 
     * for its Display to be put into the foreground and painted. Returns true
     * if this all worked; returns false if the operation timed out. Passes on 
     * any exceptions that occurred in starting up the MIDlet.
     */
    public static boolean invoke() throws Throwable {

        midlet = null;
        display = null;
        cv = new LcduiTestCanvas();

        MIDletStateHandler.getMidletStateHandler().startMIDlet(
            "com.sun.midp.util.LcduiTestMIDlet",
            "LCDUI Test MIDlet");

        return cv.awaitPaint();
    }

    /**
     * Destroys the test MIDlet and waits for the AMS to finish
     * its processing.
     */
    public static void cleanup() {
        midlet.notifyDestroyed();

        try {
            midlet.destroyApp(true);
        } catch (MIDletStateChangeException ignore) { }

        // Wait for the AMS to finish processing.
        // IMPL NOTE: there should be a better way than to sleep.
        try {
            Thread.sleep(1000);
        } catch (InterruptedException ignore) { }
    }

    /**
     * Gets the MIDlet instance.  This is non-null only between calls
     * to invoke() and cleanup().
     */
    public static MIDlet getMIDlet() {
        return midlet;
    }

    /**
     * Gets the Display object for the test MIDlet.  This is non-null only 
     * between calls to invoke() and cleanup().
     */
    public static Display getDisplay() {
        return display;
    }

    /**
     * Should be called only by the MIDlet runtime.
     */
    public LcduiTestMIDlet() {
        midlet = this;
        display = Display.getDisplay(this);
    }

    /**
     * Should be called only by the MIDlet runtime.
     */
    public void startApp() throws MIDletStateChangeException {
        display.setCurrent(cv);
    }
   
    /**
     * Should be called only by the MIDlet runtime.
     */
    public void pauseApp() {
    }

    /**
     * Should be called only by the MIDlet runtime.
     */
    public void destroyApp(boolean unconditional)
            throws MIDletStateChangeException {
        midlet = null;
        display = null;
    }
}
