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

import com.sun.j2me.i18n.Resource;
import com.sun.j2me.i18n.ResourceConstants;

/** MIDP dependencies - public API */
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Item;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.CommandListener;

/** MIDP dependencies - internal API */
import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.DisplayEventHandlerFactory;

import com.sun.j2me.security.Token;

/**
 * This class represents simple dialog form.
 */
public class Dialog implements CommandListener {

    /** Answer that indicates that the dialog was cancelled. */
    public static final int CANCELLED = -1;
    /** Answer that indicates successful completion. */
    public static final int CONFIRMED = 1;

    /** Caches the display manager reference. */
    private DisplayEventHandler displayEventHandler;

    /* Command object for "OK" command. */
    private Command okCmd = new Command(Resource
                .getString(ResourceConstants.OK),
                Command.OK, 1);
    
    /* Command object for "Cancel" command. */
    private Command cancelCmd = new Command(Resource
    		    .getString(ResourceConstants.CANCEL),
                Command.CANCEL, 1);

    /** Holds the preempt token so the form can end. */
    private Object preemptToken;

    /** Holds the answer to the security question. */
    private int answer = CANCELLED;

    /** Form object for this dialog. */
    private Form form;

    /**
     * Construct message dialog for informational message or
     * confirmation request.
     * @param title dialog title
     * @param withCancel show Cancel button
     */
    public Dialog(String title, boolean withCancel) {
        form = new Form(title);
        form.addCommand(okCmd);
        if (withCancel) {
            form.addCommand(cancelCmd);
        }
        form.setCommandListener(this);
    }

    /**
     * Adds an Item into the Form.
     * @param item the Item to be added
     */
    void append(Item item) {
        form.append(item);
    }

    /**
     * Waits for the user's answer.
     * @param token security token.
     * @return user's answer
     * @throws InterruptedException if interrupted
     */
    public int waitForAnswer(Token token) throws InterruptedException {
        if (displayEventHandler == null) {
            displayEventHandler = DisplayEventHandlerFactory.getDisplayEventHandler(token.getSecurityToken());
        }
        preemptToken = displayEventHandler.preemptDisplay(form, true);

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
     * Respond to a command issued on form.
     * Sets the user's answer and notifies waitForAnswer and
     * ends the form.
     * @param c command activiated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        synchronized (this) {
            answer = c == okCmd ? CONFIRMED : CANCELLED;
            displayEventHandler.donePreempting(preemptToken);
            notify();
        }
    }
}
