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

package com.sun.midp.automation;

import com.sun.midp.main.*;
import com.sun.midp.events.*;

/**
 * Class controlling foreground.
 */
final class AutoForegroundStateController
    implements AutoDisplayControllerListener {

    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /** List of AutoMIDletInfo for MIDlet's we are interested in */
    private AutoMIDletInfoList midletsInfo;

    /** Display controller */
    private AutoDisplayController displayController;

    /** Events producer */
    private MIDletControllerEventProducer midletControllerEventProducer;

    /** The one and only AutoMIDletStateController instance */
    private static AutoForegroundStateController stateController = null;

    /**
     * Private constructor to prevent direct creation of instances.
     */
    private AutoForegroundStateController() {
        midletProxyList = MIDletProxyList.getMIDletProxyList();

        displayController = new AutoDisplayController(midletProxyList);
        displayController.setListener(this);
        midletProxyList.setDisplayController(displayController);

        midletControllerEventProducer =
            AutomationInitializer.getMIDletControllerEventProducer();

        midletsInfo = AutoMIDletInfoList.getMIDletInfoList();
    }


    /**
     * Gets AutoMIDletStateController instance.
     *
     * @return AutoMIDletStateController instance
     */
    synchronized static AutoForegroundStateController
        getForegroundStateController() {

        if (stateController == null) {
            stateController = new AutoForegroundStateController();
        }

        return stateController;
    }


    /**
     * Initiates switching MIDlet to specified state.
     *
     * @param midlet AutoMIDletImpl instance representing MIDlet to switch
     * @param state state to switch to
     */
    void switchTo(AutoMIDletImpl midlet, AutoMIDletForegroundState state) {
        MIDletProxy midletProxy = midletsInfo.findMIDletProxy(midlet);
        if (midletProxy != null) {
            if (state == AutoMIDletForegroundState.FOREGROUND) {
                int suiteID = midletProxy.getSuiteId();
                String className = midletProxy.getClassName();

                midletControllerEventProducer.
                    sendSetForegroundByNameRequestEvent(suiteID, className);
            }
        }
    }



    /**
     * AutoDisplayControllerListener interface implementation.
     */

    /**
     * Called when foreground MIDlet is about to be changed.
     *
     * @param oldForeground MIDlet currently in foreground
     * @param newForeground MIDlet getting foreground
     */
    public void foregroundMIDletChanged(MIDletProxy oldForeground,
            MIDletProxy newForeground) {

        if (oldForeground != null) {
            AutoMIDletImpl midlet = midletsInfo.findMIDlet(oldForeground);
            if (midlet != null) {
                midlet.stateChanged(AutoMIDletForegroundState.BACKGROUND);
            }
        }

        if (newForeground != null) {
            AutoMIDletImpl midlet = midletsInfo.findMIDlet(newForeground);
            if (midlet != null) {
                midlet.stateChanged(AutoMIDletForegroundState.FOREGROUND);
            }
        }
    }



    /**
     * AutoForegroundRequestEventConsumer interface implementation.
     */

    /**
     * Processes AUTO_FOREGROUND_REQUEST event
     *
     * @param midletSuiteID MIDlet's suite ID
     * @param midletClassName MIDlet's class name
     */
    public void handleForegroundRequestEvent(int midletSuiteID,
            String midletClassName) {

        MIDletProxy midletProxy = midletProxyList.findMIDletProxy(
                midletSuiteID, midletClassName);

        if (midletProxy != null) {
            midletProxyList.setForegroundMIDlet(midletProxy);
        }
    }
}
