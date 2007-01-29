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

package com.sun.j2me.dialog;

//import com.sun.midp.midlet.MIDletEventConsumer;
//import com.sun.midp.lcdui.DisplayEventHandler;
//import com.sun.midp.lcdui.DisplayEventHandlerFactory;
//import com.sun.midp.i18n.Resource;
//import com.sun.midp.i18n.ResourceConstants;
//import com.sun.midp.security.SecurityToken;
//
//import com.sun.midp.midlet.MIDletPeer;
//
//import javax.microedition.lcdui.*;

/**
 * This class represents simple dialog form.
 */
public class Dialog /*implements CommandListener, MIDletEventConsumer */{

    /** Answer that indicates that the dialog was cancelled. */
    public static final int CANCELLED = -1;
    /** Answer that indicates successful completion. */
    public static final int CONFIRMED = 1;

    /** Caches the display manager reference. */
//    private DisplayEventHandler displayEventHandler;

//    /** Command object for "OK" command. */
//    private Command okCmd = new Command(Resource
//					.getString(ResourceConstants.OK),
//					Command.OK, 1);
//
//    /** Command object for "Cancel" command. */
//    private Command cancelCmd = 
//	new Command(Resource
//		    .getString(ResourceConstants.CANCEL),
//		    Command.CANCEL, 1);
    
    /** Holds the preempt token so the form can end. */
    private Object preemptToken;

    /** Holds the answer to the security question. */
    private int answer = CANCELLED;

    /** Form object for this dialog. */
//    private Form form;

    /**
     * Construct message dialog for informational message or
     * confirmation request.
     * @param title dialog title
     * @param withCancel show Cancel button
     */
    public Dialog(String title, boolean withCancel) {

//        form = new Form(title);
//        form.addCommand(okCmd);
        if (withCancel) {
//            form.addCommand(cancelCmd);
        }
//        form.setCommandListener(this);
    }

//    /**
//     * Adds an Item into the Form.
//     * @param item the Item to be added
//     */
//    void append(Item item) {
//        form.append(item);
//    }

    /**
     * Waits for the user's answer.
     * @param token security token.
     * @return user's answer
     * @throws InterruptedException if interrupted
     */
    public int waitForAnswer() 
	throws InterruptedException {
//	if (displayEventHandler == null) {
//	    displayEventHandler  = DisplayEventHandlerFactory
//		.getDisplayEventHandler(token);
//	}
//        preemptToken = displayEventHandler.preemptDisplay(this, form, true);

        synchronized (this) {
            if (preemptToken == null) {
                return CANCELLED;
            }

            try {
                wait();
            } catch (Throwable t) {
                return CANCELLED;
            }

            return answer;
        }
    }

//    /**
//     * Respond to a command issued on form.
//     * Sets the user's answer and notifies waitForAnswer and
//     * ends the form.
//     * @param c command activiated by the user
//     * @param s the Displayable the command was on.
//     */
//    public void commandAction(Command c, Displayable s) {
//        synchronized (this) {
//            answer = c == okCmd ? CONFIRMED : CANCELLED;
//            displayEventHandler.donePreempting(preemptToken);
//            notify();
//        }
//    }

//    /**
//     * Start the currently suspended state. This is not apply to
//     * this dialog.
//     *
//     * @param midlet midlet that the event applies to
//     */
//    public void startMIDlet(MIDletPeer midlet) {}

    /**
     * Pause the current foreground MIDlet and return to the
     * AMS or "selector" to possibly run another MIDlet in the
     * currently active suite.
     * <p>
     * This is not apply to this dialog.
     *
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletPauseEvent() {}

    /**
     * Destroy the MIDlet given midlet.
     * <p>
     * This is not apply to this dialog.
     *
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletDestroyEvent() {}

    /**
     * Activate a MIDlet.
     *
     * MIDletEventConsumer I/F method.
     */
    public void handleMIDletActivateEvent() {
	/* RFC - not needed for Dialog interaction. */
    }
}
