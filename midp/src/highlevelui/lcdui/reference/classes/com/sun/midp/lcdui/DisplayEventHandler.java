/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Image;

import com.sun.midp.events.EventQueue;

import com.sun.midp.midlet.MIDletEventConsumer;
import com.sun.midp.midlet.MIDletPeer;
import com.sun.midp.main.MIDletControllerEventProducer;

import com.sun.midp.security.SecurityToken;

/**
 * This class works around the fact that public classes can not
 * be added to a javax package by an implementation.
 */
public interface DisplayEventHandler {
    
    /**
     * Preempt the current displayable with
     * the given displayable until donePreempting is called.
     * The preemptor should stop preempting when a destroyMIDlet event occurs.
     * The event will have a null MIDlet parameter.
     *
     * @param l object to notify with the destroy MIDlet event.
     * @param d displayable to show the user
     * @param waitForDisplay if true this method will wait if the
     *        screen is being preempted by another thread.
     *
     * @return an preempt token object to pass to donePreempting done if prempt
     *  will happen, else null
     *
     * @exception SecurityException if the caller does not have permission
     *   the internal MIDP permission.
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public Object preemptDisplay(MIDletEventConsumer l,
        Displayable d, boolean waitForDisplay) throws InterruptedException;

    /**
     * Display the displayable that was being displayed before
     * preemptDisplay was called.
     *
     * @param preemptToken the token returned from preemptDisplay
     */
    public void donePreempting(Object preemptToken);

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public void initSecurityToken(SecurityToken token);

    /**
     * Initialize Display Event Handler
     *
     * @param token security token for initilaization
     * @param theEventQueue the event queue
     * @param theDisplayEventProducer producer for display events
     * @param theMIDletControllerEventProducer producer for midlet events
     * @param theRepaintEventProducer producer for repaint events events
     * @param theDisplayContainer container for display objects
     */
    public void initDisplayEventHandler(
        SecurityToken token,
        EventQueue theEventQueue,
        DisplayEventProducer theDisplayEventProducer,
        MIDletControllerEventProducer theMIDletControllerEventProducer,
        RepaintEventProducer theRepaintEventProducer,
        DisplayContainer theDisplayContainer);
    
    /**
     * Get the Image of the trusted icon for this Display.
     * Only callers with the internal AMS permission can use this method.
     *
     * @return an Image of the trusted icon.
     *
     * @exception SecurityException if the suite calling does not have the
     * the AMS permission
     */
    public Image getTrustedMIDletIcon();

    /**
     * Create a display and return its internal access object.
     *
     * @param token security token for the calling class
     * @param midlet peer of the MIDlet that will own this display
     *
     * @return new display's access object
     *
     * @exception SecurityException if the caller does not have permission
     *   the internal MIDP permission.
     */
    public DisplayAccess createDisplay(SecurityToken token,
                                       MIDletPeer midlet);
}
