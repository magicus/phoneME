/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui;

import com.sun.midp.orientation.OrientationHandler;
import com.sun.midp.orientation.OrientationListener;
import com.sun.midp.orientation.OrientationFactory;
import com.sun.midp.security.*;
import com.sun.midp.events.EventQueue;
import com.sun.midp.main.Configuration;
import javax.microedition.lcdui.Image;

/**
 * Initialize the LCDUI environment.
 */
public class LCDUIEnvironment {

    /** Stores array of active displays for a MIDlet suite isolate. */
    private DisplayContainer displayContainer;

    /** Orientation handler instance. */
    private OrientationHandler orientHandler;

    /** Orientation listener instance. */
    private OrientationListener orientationListener;

    /**
     * Provides interface for display preemption, creation and other
     * functionality that can not be publicly added to a javax package.
     */
    private DisplayEventHandler displayEventHandler;

    /**
     * Creates lcdui event producers/handlers/listeners.
     * 
     * @param internalSecurityToken
     * @param eventQueue
     * @param isolateId
     * @param foregroundController
     */
    public LCDUIEnvironment(SecurityToken internalSecurityToken,
                            EventQueue eventQueue, 
                            int isolateId,
                            ForegroundController foregroundController) {
        this(internalSecurityToken, eventQueue,
             new DefaultDisplayIdPolicy(isolateId), foregroundController);
    }

    /**
     * Creates lcdui event producers/handlers/listeners.
     * 
     * @param internalSecurityToken
     * @param eventQueue
     * @param idPolicy
     * @param foregroundController
     */
    public LCDUIEnvironment(SecurityToken internalSecurityToken,
                            EventQueue eventQueue, 
                            DisplayIdPolicy idPolicy,
                            ForegroundController foregroundController) {

        displayEventHandler =
            DisplayEventHandlerFactory.getDisplayEventHandler(
               internalSecurityToken);

        DisplayEventProducer displayEventProducer =
            new DisplayEventProducer(
                eventQueue);

        RepaintEventProducer repaintEventProducer =
            new RepaintEventProducer(
                eventQueue);

        displayContainer = new DisplayContainer(internalSecurityToken,
                                                idPolicy);

        DisplayDeviceContainer displayDeviceContainer =
            new DisplayDeviceContainer();

        /*
         * Because the display handler is implemented in a javax
         * package it cannot created outside of the package, so
         * we have to get it after the static initializer of display the class
         * has been run and then hook up its objects.
         */
        displayEventHandler.initDisplayEventHandler(
            displayEventProducer,
            foregroundController,
            repaintEventProducer,
            displayContainer,
            displayDeviceContainer);

        // Set a listener in the event queue for display events
        new DisplayEventListener(
            eventQueue,
            displayContainer,
            displayDeviceContainer);

        /*
         * Set a listener in the event queue for LCDUI events
         *
         * Bad style of type casting, but DisplayEventHandlerImpl
         * implements both DisplayEventHandler & ItemEventConsumer IFs 
         */
        new LCDUIEventListener(
            internalSecurityToken,
            eventQueue,
            (ItemEventConsumer)displayEventHandler);

        // Set a listener in the event queue for foreground events
        new ForegroundEventListener(eventQueue, displayContainer);

        // Initialize a handler to process rotation events
        String orientClassName = Configuration.getProperty("com.sun.midp.orientClassName");
        if (orientClassName != null && orientClassName.length() > 0) {
            orientHandler = OrientationFactory.createOrientHandler(orientClassName);
            if (orientHandler != null) {
                orientationListener = new OrientationListenerImpl();
                orientHandler.addListener(orientationListener);
            }
        }
    }

    /**
     * Gets DisplayContainer instance. 
     *
     * @return DisplayContainer
     */
    public DisplayContainer getDisplayContainer() {
        return displayContainer;
    }

    /** 
     * Called during system shutdown.  
     */
    public void shutDown() {

        if (orientHandler != null && orientationListener != null) {
            orientHandler.removeListener(orientationListener);
        }
        // shutdown any preempting
        displayEventHandler.donePreempting(null);
    }

    /**
     * Sets the trusted state based on the passed in boolean.
     *
     * @param isTrusted if true state is set to trusted.
     */
    public void setTrustedState(boolean isTrusted) {
        displayEventHandler.setTrustedState(isTrusted);
    }

    /** This is nested inside LCDUIEnvironment so that it can see the private fields. */
    private class OrientationListenerImpl implements OrientationListener {
    
        /**
         * Calls when orientation is changed. 
         *
         * @param orientation the orientation state
         */
        public void orientationChanged(int orientation) {
            DisplayAccess da = displayContainer.findForegroundDisplay();
            if (da != null) {
                da.getDisplayEventConsumer().handleRotationEvent(orientation);
            }
        }
    }
}
