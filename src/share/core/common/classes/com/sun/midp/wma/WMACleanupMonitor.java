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

package com.sun.midp.wma;

import com.sun.midp.main.MIDletProxy;
import com.sun.midp.main.MIDletProxyList;
import com.sun.midp.main.MIDletProxyListListener;

/**
 * The function of this class is to call a native WMA function,
 * when a MIDletSuite terminates. The native function will
 * perform any cleanup that is required, eg: delete any messages
 * left unread by the MIDletSuite.
 * This class implements the MIDletProxyListener interface and is
 * thus notified whenever a MIDletSuite is installed or removed.
 *<p>
 * Two versions of this file exist: one which is a no-op, is used, when
 * MIDP stack is not built with JSR 205 and the real implementation when
 * MIDP stack <em>is</em> built with JSR205.
 *
 */
public class WMACleanupMonitor implements MIDletProxyListListener {

    /**
     * Native function to delete SMS/CBS/MMS messages left
     * unread by the MIDlet suite that is terminating.
     *
     * @param msid Midlet Suite ID.
     *
     */
    private native void deleteMessages0(int msid);

    /**
     * Prevents instantiation of this class.
     */
    private WMACleanupMonitor() {
    }

    /**
     * Initialization function for the WMA cleanup service.
     * Adds itself as a listener, so as to be notified on
     * suite removal.
     *
     * @param mpl A midlet proxy list, passed down by the
     *            suite loader.
     */
    public static void init(MIDletProxyList mpl) {
        mpl.addListener(new WMACleanupMonitor());
    }

    /**
     * The WMACleanup monitor ignores MIDlet added callbacks.
     * The necessary initialization is done in the Isolate and
     * MIDletState that instantiates the MIDlet.
     * Called when a MIDlet is added to the list and only in the AMS
     * Isolate.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
    }

    /**
     * The WMACleanup monitor ignores MIDlet update callbacks.
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated,
     * see constants above
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
    }

    /**
     * The WMACleanup monitor uses the MIDlet removed callback
     * to delete any messages left unread by the MIDlet suite..
     * Called (in the AMS Isolate) when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        //deleteMessages0(midlet.getSuiteId());
    }

    /**
     * The WMACleanup monitor ignores MIDlet start error callbacks.
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId,
                                 String className, int errorCode,
                                 String errorDetails) {

    }
    /* IMPL NOTE: workaround until the bobcat doesn't use integer midlet suite IDs */
    public void midletStartError(int externalAppId, String suiteId,
                                 String className, int error) {
    }

}

