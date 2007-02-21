/*
 *
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

package javax.microedition.lcdui;

import com.sun.midp.midlet.MIDletEventConsumer;
import com.sun.midp.midlet.MIDletPeer;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.main.MIDletControllerEventProducer;

import com.sun.midp.events.EventQueue;
import com.sun.midp.lcdui.EventConstants;

import com.sun.midp.lcdui.DisplayContainer;
import com.sun.midp.lcdui.DisplayAccess;
import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.DisplayEventProducer;
import com.sun.midp.lcdui.RepaintEventProducer;
import com.sun.midp.lcdui.ItemEventConsumer;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;

/**
 * This class has dual functiopnality:
 *
 * First, it implements DisplayEventHandler I/F and thus provides access
 * to display objects (creation, preemption, set/get IDs and other properties).
 *
 * Second, it implements ItemEventConsumer I/F and thus processes
 * LCDUI events that due to different reasons can't be associated with
 * Display instance specific DisplayEventConsumer objects,
 * but need to be processed by isolate-wide handler.
 * TBD: These are subjects for futher investigation to move them
 * to DisplayEventConsumer.
 *
 * In addition, it implements a number of package private methods that work
 * with Display and are called locally by display/DisplayAccessor.
 * TBD: These are subjects for further investination to move them closer
 * to end users: Display & displayAccessor classes.
 *
 */
class DisplayEventHandlerImpl implements DisplayEventHandler,
        ItemEventConsumer {

    /** This class has a different security domain than the MIDlet suite. */
    private static SecurityToken classSecurityToken;

    /** Cached reference to Active Displays Container. */
    private DisplayContainer displayContainer;

    /** Cached reference to the MIDletControllerEventProducer. */
    private MIDletControllerEventProducer midletControllerEventProducer;

    /** The preempting display. */
    private DisplayAccess preemptingDisplay;

    /**
     * True to signal to send a MIDlet destroyed event instead of a
     * end preempt event.
     */
    private boolean destroyPreemptingDisplay;

    /** Package private constructor restrict creation to LCDUI package */
    DisplayEventHandlerImpl() {
        if (classSecurityToken != null) {
            throw new SecurityException("singleton violation");
        }
    }

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     * DisplayEventHandler I/F method.
     *
     * @param token security token for this class.
     */
    public void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }

    /**
     * Initialize Display Event Handler.
     * DisplayEventHandler I/F method.
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
        DisplayContainer theDisplayContainer) {

        token.checkIfPermissionAllowed(Permissions.MIDP);

        midletControllerEventProducer = theMIDletControllerEventProducer;

        displayContainer = theDisplayContainer;

        /*
         * TBD: not a good idea to call static initializer
         * from non-static method ...
         * Maybe to create a separate method:
         * DisplayEventHandler.initDisplayClass(token,...)
         * for these purposes and call it from Suite Loader's main() ?
         * displayEventHandlerImpl I/F miplementor will call
         * Display.initClass() from itsinitDisplayClass() method ?
         */
        Display.initClass(
            theMIDletControllerEventProducer,
            theDisplayEventProducer,
            theRepaintEventProducer,
            theDisplayContainer);

        /** create DisplayEventListener & initialize it */
        new DisplayEventListener(
            theEventQueue,
            theDisplayContainer);
    }

    /**
     * Preempt the current displayable with
     * the given displayable until donePreempting is called.
     * The preemptor should stop preempting when a destroyMIDlet event
     * occurs. The event will have a null MIDlet parameter. To avoid
     * dead locking the event thread his method
     * MUST NOT be called in the event thread.
     * DisplayEventHandler I/F method.
     *
     * @param l object to notify with the destroy MIDlet event.
     * @param d displayable to show the user
     * @param waitForDisplay if true this method will wait if the
     *        screen is being preempted by another thread, however
     *        if this is called in the event dispatch thread this
     *        method will return null regardless of the value
     *        of <code>waitForDisplay</code>
     *
     * @return an preempt token object to pass to donePreempting done if
     * prempt will happen, else null
     *
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public Object preemptDisplay(MIDletEventConsumer l, Displayable d,
                                 boolean waitForDisplay)
            throws InterruptedException {
        Display tempDisplay;
        String title;

        if (d == null) {
            throw new NullPointerException(
                "The displayable can't be null");
        }

        title = d.getTitle();
        if (title == null) {
            throw new NullPointerException(
                "The title of the displayable can't be null");
        }

        if (EventQueue.isDispatchThread()) {
            // Developer programming error
            throw new RuntimeException(
                "Blocking call performed in the event thread");
        }

        /**
         * This sync protects preempt related local fields:
         * preemptingDisplay and destroyPreemptingDisplay.
         */
        synchronized (this) {
            /**
             * ATTENTION !
             * The synchronization below is essential for atomic creation of
             * preempting Displays:
             * "create display Id" and "add the display in the container"
             * (see "new Display(...)") shall be synchronized with
             * check if other displays exist in the container
             * (see "isOneElementInContainer()").
             * This check is needed to distinguish the situation when
             * no midlets exist in current isolate yet.
             *
             * This code is suspicious, in future preemption will definitely
             * be reworked and simplified, but until that this synchronization
             * on an external object is needed.
             */
            synchronized (displayContainer) {
                if (preemptingDisplay != null) {

                    if (!waitForDisplay) {
                        return null;
                    }

                    this.wait();
                }

                tempDisplay = new Display(l);
                tempDisplay.setCurrent(d);
                preemptingDisplay = tempDisplay.accessor;

                if (displayContainer.isOneElementInContainer()) {
                    // No applications, so this preempt is like a new MIDlet.
                    midletControllerEventProducer.sendMIDletCreateNotifyEvent(
                        0, tempDisplay.displayId, MIDletSuite.UNUSED_SUITE_ID,
                            "preempt", title);

                    midletControllerEventProducer.
                        sendDisplayForegroundRequestEvent(
                            tempDisplay.displayId, true);
                    destroyPreemptingDisplay = true;
                } else {
                    midletControllerEventProducer.sendDisplayPreemptStartEvent(
                        tempDisplay.displayId);
                    destroyPreemptingDisplay = false;
                }

                return preemptingDisplay;
            } // end "synchronized (displayContainer)"
        }
    }

    /**
     * Display the displayable that was being displayed before
     * preemptDisplay was called.
     * DisplayEventHandler I/F method.
     *
     * @param preemptToken the token returned from preemptDisplay
     */
    public void donePreempting(Object preemptToken) {
        /**
         * This sync protects preempt related local fields:
         * preemptingDisplay and destroyPreemptingDisplay.
         */
        synchronized (this) {
            if (preemptingDisplay != null &&
                (preemptToken == preemptingDisplay || preemptToken == null)) {

                if (destroyPreemptingDisplay) {
                    // No applications, so this preempt is like a new MIDlet.
                    midletControllerEventProducer.sendMIDletDestroyNotifyEvent(
                        preemptingDisplay.getDisplayId());

                } else {
                    midletControllerEventProducer.sendDisplayPreemptStopEvent(
                        preemptingDisplay.getDisplayId());

                }

                preemptingDisplay = null;

                displayContainer.removeDisplay(preemptingDisplay);

                // A midlet may be waiting to preempt
                this.notify();
            }
        }
    }

    /**
     * Create a display and return its internal access object.
     * DisplayEventHandler I/F method.
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
                                       MIDletPeer midlet) {
        token.checkIfPermissionAllowed(Permissions.MIDP);

        return new Display((MIDletEventConsumer)midlet).accessor;
    }

    /**
     * Get the Image of the trusted icon for this Display.
     * Only callers with the internal MIDP permission can use this method.
     * DisplayEventHandler I/F method.
     *
     * @return an Image of the trusted icon.
     */
    public Image getTrustedMIDletIcon() {
        return Display.getSystemImage("trustedmidlet_icon.png");
    }

    /**
     * Called by event delivery to process an Item state change.
     * ItemEventConsumer I/F method.
     *
     * @param item the Item which has changed state
     */
    public void handleItemStateChangeEvent(Item item) {
        if (item.owner != null) {
            item.owner.uCallItemStateChanged(item);
        }
    }

    /**
     * Called by event delivery to refresh a CustomItem's size information.
     * ItemEventConsumer I/F method.
     *
     * @param ci the custom item whose size information has to be changed
     */
    public void handleItemSizeRefreshEvent(CustomItem ci) {
        ci.customItemLF.uCallSizeRefresh();
    }


/*
 * private methods
 */
    static {
        // Instantiate link with MMAPI video player for repaint hooks
        new MMHelperImpl();
    }
}
