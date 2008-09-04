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
 * This class override the MVM display manager so that requests from its
 * parent midlet will not be given to the MVM display manager.
 */
public class NativeDisplayControllerPeer extends MVMDisplayController {
    /**
     * Construct a DisplayController with a reference to the ProxyList.
     *
     * @param theMIDletProxyList reference to the MIDlet proxy list
     */
    public NativeDisplayControllerPeer(MIDletProxyList theMIDletProxyList) {
        // The Java foreground selector is not used by this controller.
        super(theMIDletProxyList, null);
    }

    /**
     * Call to notify that foreground MIDlet is changing and give the
     * display controller a chance to preempt the change.
     * Also the last MIDlet created state will be reset.
     * <p>
     * If the MIDlet to get the foreground is paused, then activate it.
     *
     * @param midlet proxy of the MIDlet to be put in the foreground
     *
     * @return Proxy of the next foreground MIDlet, may be the foreground
     *         MIDlet if the foreground should not change
     */
    MIDletProxy foregroundMidletChanging(MIDletProxy midlet) {
        MIDletProxy nextForeground = super.foregroundMidletChanging(midlet);

        if (nextForeground != null) {
            notifyMidletHasForeground(nextForeground.getExternalAppId());
        }

        return nextForeground;
    }

    /**
     * Handles MIDlet background requests.
     * <p>
     * If the MIDlet is the foreground MIDlet, then forward the request
     * to the native layer to the system can show the native foreground
     * selector to the user.
     *
     * @param midlet The proxy of the MIDlet that was updated
     *
     * @return foreground MIDlet so the foreground will not change at this
     * time
     */
    MIDletProxy backgroundRequest(MIDletProxy midlet) {
        MIDletProxy foreground = midletProxyList.getForegroundMIDlet();

        if (midlet == foreground) {
            /*
             * Normal MVM mode case,
             * Let the user choose the next foreground using the native GUI.
             */
            forwardBackgroundRequest(midlet.getExternalAppId());
        }

        // don't change the foreground
        return foreground;
    }

    /**
     * Notify the native application manager of the MIDlet foreground change.
     *
     * @param externalAppId ID assigned by the external application manager
     */
    static native void notifyMidletHasForeground(int externalAppId);

    /**
     * Forwards MIDlet background requests to the native layer.
     *
     * @param externalAppId ID assigned by the external application manager
     */
    private static native void forwardBackgroundRequest(int externalAppId);
}
