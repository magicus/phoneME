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

package com.sun.midp.appmanager;

import javax.microedition.lcdui.*;

import com.sun.midp.configurator.Constants;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.main.MIDletProxy;

/**
 * HeadlessAlert is shown when a user selects a MIDlet to bring to the
 * foreground that has not requested the foreground at least once by
 * call Display.setCurrent(Displayable).
 */
class HeadlessAlert extends Alert implements CommandListener {

    /** The display instance to be used to display error alerts */
    private Display display;

    /** Reference to the previous current displayable. */
    private Displayable previousCurrent;

    /** Command to bring a headless midlet into the background state */
    private Command okCommand;

    /** Command to terminate a headless midlet */
    private Command exitCommand;

    /** Proxy to the headless MIDlet. */
    private MIDletProxy midlet;

    /**
     * Initialize this headless alert.
     *
     * @param theDisplay the Display instance where this alerts
     *        will be shown
     */
    HeadlessAlert(Display theDisplay) {
        super(null,
              Resource.getString(ResourceConstants.LCDUI_DISPLAY_HEADLESS),
              null, AlertType.INFO);

        display = theDisplay;

        setTimeout(Alert.FOREVER);

        okCommand = new Command(Resource.getString(ResourceConstants.OK),
                                Command.OK, 2);
        addCommand(okCommand);

        exitCommand = new Command(Resource.getString(ResourceConstants.EXIT),
                                  Command.EXIT, 1);

        addCommand(exitCommand);

        setCommandListener(this);
    }


    /**
     * Display the Alert.
     *
     * @param proxy proxy to the headless MIDlet
     */
    void show(MIDletProxy proxy) {
        midlet = proxy;

        previousCurrent = display.getCurrent();

        display.setCurrent(this);
    }

    /**
     * Process commands for this alert. "OK" will dismiss the
     * alert with no action. "Exit" will destroy the MIDlet.
     *
     * @param c command the user pressed
     * @param d this displayable(not used)
     */
    public void commandAction(Command c, Displayable d) {
        if (c == exitCommand) {
            midlet.destroyMidlet();
            display.setCurrent(previousCurrent);
        } else if (c == okCommand) {
            display.setCurrent(previousCurrent);
        }
    }
}
