/*
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

package com.sun.midp.lcdui;

import com.sun.midp.security.*;
import com.sun.midp.events.EventQueue;
import javax.microedition.lcdui.Image;

/**
 * Initialize the LCDUI environment.
 */
public class LCDUIEnvironment {


    /**
     * Creates lcdui Display Container. 
     * The rest of the event producers/hanlders/listners
     * are created and initialised in initDisplayEventHandler ().)
     * 
     * @param internalSecurityToken security token
     * @param eventQueue this isolate's eventQueue instance
     * @param isolateId this isolate's id
     */
    public LCDUIEnvironment(SecurityToken internalSecurityToken,
			    EventQueue eventQueue,
			    int isolateId) {
        displayContainer = new DisplayContainer(
            internalSecurityToken, isolateId);
    }

    /**
     * Creates lcdui event producers/handlers/lisneners.
     * 
     * @param internalSecurityToken security token
     * @param eventQueue this isolate's eventQueue instance
     * @param isolateId this isolate's id
     * @param foregroundController this isolate's foreground controller
     */
    public LCDUIEnvironment(SecurityToken internalSecurityToken,
		EventQueue eventQueue, 
		int isolateId,
		ForegroundController foregroundController) {

	this(internalSecurityToken, eventQueue, isolateId);

	initDisplayEventHandler(internalSecurityToken, eventQueue,
				foregroundController);
    }

    /*
     * Creates lcdui event producers/handlers/lisneners.
     * 
     * @param internalSecurityToken security token
     * @param eventQueue this isolate's eventQueue instance
     * @param fgController this isolate's foreground controller
     */
    protected void initDisplayEventHandler(SecurityToken internalSecurityToken,
					   EventQueue eventQueue,
					   ForegroundController fgController) {

        DisplayEventProducer displayEventProducer =
            new DisplayEventProducer(
                eventQueue);

        RepaintEventProducer repaintEventProducer =
            new RepaintEventProducer(
                eventQueue);

        displayEventHandler =
            DisplayEventHandlerFactory.getDisplayEventHandler(
               internalSecurityToken);
        /*
         * Because the display handler is implemented in a javax
         * package it cannot created outside of the package, so
         * we have to get it after the static initializer of display the class
         * has been run and then hook up its objects.
         */
        displayEventHandler.initDisplayEventHandler(
	    displayEventProducer,
            fgController,
            repaintEventProducer,
            displayContainer);

        DisplayEventListener displayEventListener = new DisplayEventListener(
            eventQueue,
            displayContainer);

        /* Bad style of type casting, but DisplayEventHandlerImpl
         * implements both DisplayEventHandler & ItemEventConsumer IFs 
         */
        LCDUIEventListener lcduiEventListener = new LCDUIEventListener(
            internalSecurityToken,
            eventQueue,
            (ItemEventConsumer)displayEventHandler);

	foregroundController = fgController;
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
     * Gets ForegroundController instance. 
     *
     * @return ForegroundController
     */
    public ForegroundController getForegroundController() {
	return foregroundController;
    }

    /** 
     * Called during system shutdown.  
     */
    public void shutDown() {

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

    /** Stores array of active displays for a MIDlet suite isolate. */
    protected DisplayContainer displayContainer;

    /**
     * Provides interface for display preemption, creation and other
     * functionality that can not be publicly added to a javax package.
     */
    protected DisplayEventHandler displayEventHandler;

    /**
     * Provides interface cor foreground switching.
     */
    protected ForegroundController foregroundController;
}