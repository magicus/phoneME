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

package com.sun.satsa.acl;

import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.DisplayEventHandlerFactory;
import com.sun.midp.security.SecurityToken;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.midlet.MIDletEventConsumer;
import com.sun.midp.midlet.MIDletPeer;

import javax.microedition.lcdui.*;


/** Implements PIN entry dialog. */
public class PINEntryDialog implements CommandListener, MIDletEventConsumer {

    /** Answer that indicates that the dialog was cancelled. */
    private static final int CANCELLED = -1;
    /** Answer that indicates successful completion. */
    private static final int CONFIRMED = 1;

    /** Caches the display manager reference. */
    private DisplayEventHandler displayEventHandler;

    /** Command object for "OK" command. */
    private Command okCmd = 
	new Command(Resource.getString(ResourceConstants.OK),
		    Command.OK, 1);

    /** Command object for "Cancel" command. */
    private Command cancelCmd =
	new Command(Resource.getString(ResourceConstants.CANCEL),
		    Command.CANCEL, 1);

    /** Holds the preempt token so the form can end. */
    private Object preemptToken;

    /** Holds the answer to the security question. */
    private int answer = CANCELLED;

    /** PIN entry text field. */
    private TextField tf1;
    /** PIN entry text field. */
    private TextField tf2;
    /** Attributes of the 1st PIN. */
    private PINAttributes pin1;
    /** Attributes of the 2nd PIN. */
    private PINAttributes pin2;
    /** 1st PIN data. */
    private byte[] data1;
    /** 2nd PIN data. */
    private byte[] data2;

    /**
     * Construct PIN dialog.
     *
     * @param token security token with the permission to peempt the
     *        foreground display
     * @param action PIN entry operation identifier.
     * @param attr1 1st PIN attributes.
     * @param attr2 2nd PIN attributes.
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public PINEntryDialog(SecurityToken token, int action,
                          PINAttributes attr1, PINAttributes attr2)
            throws InterruptedException {

        String title = null;

        String label1 = attr1.label;
        String label2 = null;
        pin1 = attr1;
        pin2 = null;

        switch (action) {
            case ACLPermissions.CMD_VERIFY: {
                // "PIN verification"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_VERIFY);
                break;
            }
            case ACLPermissions.CMD_CHANGE: {
                // "Change PIN"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_CHANGE);
                // "New value" -> "Enter new PIN"
                label2 = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_ENTERPIN);
                pin2 = attr1;
                break;
            }
            case ACLPermissions.CMD_DISABLE: {
                // "Disable PIN"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_DISABLE);
                break;
            }
            case ACLPermissions.CMD_ENABLE: {
                // "Enable PIN";
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_ENABLE);
                break;
            }
            case ACLPermissions.CMD_UNBLOCK: {
                // "Unblock PIN"
                title = Resource.getString(ResourceConstants.
                    JSR177_PINDIALOG_TITLE_UNBLOCK);
                label1 = attr2.label;
                label2 = attr1.label + " - " + 
                    // "new value"
                    Resource.getString(ResourceConstants.
                        JSR177_PINDIALOG_NEWVALUE);
                pin1 = attr2;
                pin2 = attr1;
                break;
            }
        }

        Form form = new Form(title);

        int flags = TextField.PASSWORD;
        if (pin1.isNumeric()) {
            flags |= TextField.NUMERIC;
        }
        tf1  = new TextField(label1, "", pin1.getMaxLength(), flags);
        form.append(tf1);

        if (pin2 != null) {

            flags = TextField.SENSITIVE | TextField.NON_PREDICTIVE;
            if (pin2.isNumeric()) {
                flags |= TextField.NUMERIC;
            }
            tf2  = new TextField(label2, "", pin2.getMaxLength(), flags);
            form.append(tf2);

        }

        form.addCommand(cancelCmd);
        form.addCommand(okCmd);
        form.setCommandListener(this);
	if (displayEventHandler  == null) {
	    displayEventHandler = DisplayEventHandlerFactory
		.getDisplayEventHandler(token);
	}
        preemptToken = displayEventHandler.preemptDisplay(this, form, true);
    }

    /**
     * Waits for the user's answer.
     * @return user's answer
     */
    public int waitForAnswer() {
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

    /**
     * Sets the user's answer and notifies waitForAnswer and
     * ends the form.
     * @param theAnswer user's answer or CANCEL if system cancelled the
     *        screen
     */
    private void setAnswer(int theAnswer) {
        synchronized (this) {
            answer = theAnswer;
            displayEventHandler.donePreempting(preemptToken);
            notify();
        }

    }

    /**
     * Respond to a command issued on form.
     * @param c command activiated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == okCmd) {
            if (! checkInput()) {
                return;
            }
            setAnswer(CONFIRMED);
        } else {
            setAnswer(CANCELLED);
        }

    }

    /**
     * Verifies that the values entered are acceptable.
     * @return true if the values entered are acceptable.
     */
    private boolean checkInput() {

        if (tf1.getString().trim().equals("")) {
            return false;
        }

        data1 = pin1.transform(tf1.getString());
        if (data1 == null) {
            // PIN can't be formatted, pass empty PIN to update counter
            data1 = new byte[8];
        }

        if (pin2 != null) {
            data2 = pin2.transform(tf2.getString());
            if (data2 == null) {
                tf2.setString("");
                return false;
            }
        } else {
            data2 = null;
        }
        return true;
    }

    /**
     * Start the currently suspended state. This is not apply to
     * this dialog.
     *
     * @param midlet midlet that the event applies to
     */
    public void startMIDlet(MIDletPeer midlet) {}

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

    /**
     * Get the entered values.
     * @return null if PIN entry was cancelled by user, otherwise an array
     * containing PIN value(s).
     */
    public Object[] getPINs() {

        if (answer == CANCELLED) {
            return null;
        }

        if (pin2 == null) {
            return new Object[] {data1};
        }

        return new Object[] {data1, data2};
    }
}
