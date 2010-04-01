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

import java.util.*;

/**
 * This class controls which MIDlet's display is in the foreground.
 * Running only in the AMS Isolate (0) the controller consulted by the MIDlet
 * proxy list for any foreground when various state changes occur in a MIDlet.
 * The display controller defers the display decision to the user by choosing
 * the foreground selector to be the next foreground.
 * <p>
 * From the user perspective when the last MIDlet the user launched sets its
 * current displayable for the first time, that MIDlet should automatically
 * get the foreground (see the midletCreated and foregroundRequest methods).
 * <p>
 * A MIDlet that is paused or destroyed is treated as if it has requested the
 * background as described above.
 */
public class MVMDisplayController extends DisplayController {
    /** Foreground Selector MIDlet. */
    private MIDletProxy foregroundSelector;

    /**
     * Construct a DisplayController with a reference to the ProxyList.
     *
     * @param theMIDletProxyList reference to the MIDlet proxy list
     * @param theForegroundSelector the proxy of foreground selector
     */
    public MVMDisplayController(MIDletProxyList theMIDletProxyList,
                                MIDletProxy theForegroundSelector) {
        super(theMIDletProxyList);

        foregroundSelector = theForegroundSelector;

        /*
         * Ensure the foreground selector will get the foreground
         * when requested.
         */
        lastMidletCreated = foregroundSelector;
    }

    /**
     * Handles MIDlet foreground requests.
     * <p>
     * If proxy being updated belongs last MIDlet created in the proxy list,
     * then put the MIDlet in the foreground.
     * <p>
     * Otherwise, the request will not be granted. Foreground will not change.
     *
     * @param midlet The proxy of the MIDlet that was updated
     *
     * @return Proxy of the next foreground MIDlet, may be the foreground
     *         MIDlet if the foreground should not change
     */
    MIDletProxy foregroundRequest(MIDletProxy midlet) {
        /*
         * When the last MIDlet started wants the foreground automatically
         * put in the foreground this time only.
         */
        return (midlet == lastMidletCreated) ? midlet
                    : midletProxyList.getForegroundMIDlet();
    }

    /**
     * Handles MIDlet background requests.
     * <p>
     * If the MIDlet is the foreground MIDlet, then bring the foreground
     * selector to the foreground.
     *
     * @param midlet The proxy of the MIDlet that was updated
     *
     * @return Proxy of the next foreground MIDlet, may be the foreground
     *         MIDlet if the foreground should not change
     */
    MIDletProxy backgroundRequest(MIDletProxy midlet) {
        MIDletProxy foreground = midletProxyList.getForegroundMIDlet();

        if (midlet != foreground) {
            // not in the foreground, so don't change the foreground
            return foreground;
        }

        /*
         * Normal MVM mode case,
         * Let the user choose the next foreground.
         */
        return getForegroundSelector();
    }

    /**
     * Preempt an Isolate's displays.
     *
     * @param preempting proxy of the preempting MIDlet to be put in the
     *  foreground when a preempted MIDlet gets the foreground
     *
     * @return Proxy of the next foreground MIDlet, may be the foreground
     *         MIDlet if the foreground should not change
     * 
     */
    MIDletProxy startPreempting(MIDletProxy preempting) {
        Enumeration midlets;
        MIDletProxy preempted;
        MIDletProxy foreground;

        /*
         * Preempt all of the MIDlets in the same Isolate as the preempting
         * proxy.
         */
        midlets = midletProxyList.getMIDlets();
        while (midlets.hasMoreElements()) {
            MIDletProxy current = (MIDletProxy)midlets.nextElement();

            if (current.getIsolateId() != preempting.getIsolateId()) {
                continue;
            }

            preempted = midletProxyList.getForegroundMIDlet();
            if (preempted == null) {
                preempted = current;
            }
            preempting.setPreemptedMidlet(preempted);
            current.setPreemptingDisplay(preempting);
            midletProxyList.notifyListenersOfProxyUpdate(current, 
                MIDletProxyListListener.PREEMPTING_DISPLAY);
        }

        foreground = midletProxyList.getForegroundMIDlet();
        if (foreground == null || preempting.wasNotActive) {
            return preempting;
        }

        if (foreground.getIsolateId() == preempting.getIsolateId()) {
            preempting.setPreemptedMidlet(foreground);
            return preempting;
        }

        return foreground;
    }

    /**
     * Called to process a select foreground event.
     * Returns the foreground selector MIDlet in the foreground.
     *
     * @param onlyFromLaunchedList true if midlet should
     *        be selected from the list of already launched midlets,
     *        if false then possibility to launch midlet is needed.
     * @return Proxy of the next foreground MIDlet, may be the foreground
     *         MIDlet if the foreground should not change
     */
    MIDletProxy selectForeground(boolean onlyFromLaunchedList) {

        notifyListenersOfSelectForeground(onlyFromLaunchedList);
        return getForegroundSelector();
    }

    /**
     * Returns the foreground selector MIDlet.
     *
     * @return Proxy of the foreground selector MIDlet
     */
    private MIDletProxy getForegroundSelector() {
        return foregroundSelector;
    }
}
