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

package com.sun.mtask.midlet;

import sun.mtask.Listener;
import sun.mtask.AppModelManager;
import com.sun.midp.main.MIDletProxy;
import com.sun.midp.main.MIDletProxyList;

/**
 * Proxy MIDlet Manager. 
 */
public class PMidletManager implements AppModelManager {
    /** Current proxy list. */
    private static MIDletProxyList proxyList = null;
    /** Most recent foreground MIDlet. */ 
    private static MIDletProxy lastForegroundMidlet = null;
    /** Verbose debug output flag. */
    private static boolean verbose = false;

    static {
        PMidletManager pmm = new PMidletManager();
        try {
            Listener.setAppModelManager(pmm);
        } catch (Throwable e) {
	    // The Listener is not created. Do nothing.
        }
    }

    /**
     * Register this app model manager with the system.
     */
    public void register() {
    }

    /**
     * Deactivate myself.
     */
    public void deactivate() {
        if (lastForegroundMidlet == null) {
	    if (proxyList == null) {
	        proxyList = MIDletProxyList.getMIDletProxyList();
            }
            lastForegroundMidlet = proxyList.getForegroundMIDlet();
        }
        proxyList.setForegroundMIDlet(null);
    }

    /**
     * Activate myself.
     */
    public void activate() {
        if (lastForegroundMidlet != null) {
            proxyList.setForegroundMIDlet(lastForegroundMidlet);
            lastForegroundMidlet = null;
        }
    }

    /**
     * Process incoming message for this app model
     * @param s target application type
     * @param messageId message identifier (currently unused)
     * @return <code>true</code> if action was performed
     */
    public boolean processMessage(String s, String messageId) {
        if (!s.startsWith("MIDLET_")) {
            return false;
        }
        s = s.substring(7);
        if (verbose) {
	    System.err.println("MIDLET MESSAGE=\""+s+"\"");
        }
        if (s.startsWith("ACTIVATE")) {
            activate();
        } else if (s.startsWith("DEACTIVATE")) {
            deactivate();
        } else {
            return false;
        }
        return true;
    }
}
