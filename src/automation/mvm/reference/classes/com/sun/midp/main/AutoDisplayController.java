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

package com.sun.midp.main;


/**
 * Automation API display controller
 */
public class AutoDisplayController extends DisplayController {
    
    /** Foreground state changes listener */
    AutoDisplayControllerListener listener;

    /**
     * Construct a DisplayController with a reference to the ProxyList.
     *
     * @param midletProxyList reference to the MIDlet proxy list
     */
    public AutoDisplayController(MIDletProxyList midletProxyList) {
        super(midletProxyList);
        listener = null;
    }

    /**
     * Call to notify display controller that foreground MIDlet 
     * has changed. 
     *
     * @param oldForeground proxy of the old foreground MIDlet 
     * @param newForeground proxy of the new foreground MIDlet 
     *
     */    
    void foregroundMidletChanged(MIDletProxy oldForeground, 
            MIDletProxy newForeground) {
        
        notifyListenersOfForegroundChange(oldForeground, newForeground);
    }


    /**
     * Sets a listener for foreground state changes.
     *
     * @param listener listener
     */
    public void setListener(AutoDisplayControllerListener listener) {
        this.listener = listener;
    }


    /**
     * Notifies listener about foreground change.
     *
     * @param oldForeground MIDlet currently in foreground
     * @param newForeground MIDlet getting foreground
     */
    private void notifyListenersOfForegroundChange(MIDletProxy oldForeground,
            MIDletProxy newForeground) {

        if (listener != null) {
            listener.foregroundMIDletChanged(oldForeground, newForeground);
        }
    }
}
