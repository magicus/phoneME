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

package com.sun.midp.lcdui;

import javax.microedition.lcdui.*;
import com.sun.midp.midlet.MIDletEventConsumer;
import com.sun.midp.events.EventQueue;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Display a preempting alert and wait for the user to acknowledge it.
 */
public class SystemAlert extends Alert
    implements CommandListener, MIDletEventConsumer {

    /** Preempt token for displaying errors. */
    private Object preemptToken;

    /** The display event handler for displaying errors. */
    private DisplayEventHandler displayEventHandler;

    /**
     * Construct an <code>SystemAlert</code>.
     *
     * @param displayEventHandler The display event handler for error display
     * @param title The title of the <tt>Alert</tt>
     * @param text The text of the <tt>Alert</tt>
     * @param image An <tt>Image</tt> to display on the <tt>Alert</tt>
     * @param type The <tt>Alert</tt> type
     */
    public SystemAlert(DisplayEventHandler displayEventHandler,
                       String title, String text,
                       Image image, AlertType type) {

        super(title, text, image, type);

        setTimeout(Alert.FOREVER);
        setCommandListener(this);

        this.displayEventHandler = displayEventHandler;

        try {
            preemptToken =
                displayEventHandler.preemptDisplay(this, this, true);
        } catch (Throwable e) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                              "Throwable while preempting Display");
            }
        }
    }

    /** Waits for the user to acknowledge the alert. */
    public void waitForUser() {
        synchronized (this) {
            if (preemptToken == null) {
                return;
            }

            if (EventQueue.isDispatchThread()) {
                // Developer programming error
                throw new RuntimeException(
                    "Blocking call performed in the event thread");
            }

            try {
                wait();
            } catch (Throwable t) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                                  "Throwable while SystemAlert.waitForUser");
                }
            }
        }
    }

    /** Dismiss the alert */
    private synchronized void dismiss() {
        notify(); // wait up waitForUser() thread
        displayEventHandler.donePreempting(preemptToken);
        preemptToken = null;
    }

    /**
     * Respond to a command issued on security question form.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        dismiss();
    }

    /**
     * Pause the current foreground MIDlet and return to the
     * AMS or "selector" to possibly run another MIDlet in the
     * currently active suite.
     * <p>
     * This will end the error alert.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletPauseEvent() {
        dismiss();
    }
 
    /**
     * Start the currently suspended state. This is not apply to
     * the error alert.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletActivateEvent() {}
 
    /**
     * Destroy the MIDlet given midlet.
     * <p>
     * This is will end error alert.
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletDestroyEvent() {
        dismiss();
    }
}
