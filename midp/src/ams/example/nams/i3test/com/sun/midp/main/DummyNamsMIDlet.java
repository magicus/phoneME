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

package com.sun.midp.main;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;
import javax.microedition.lcdui.Display;

import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Displayable;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Base class for several Dummy MIDlets used in NAMS I3 testing
 */
public class DummyNamsMIDlet extends MIDlet implements CommandListener {
    
    protected String name; // = "Dummy Nams MIDlet";
    private Command cmdMgr;
    private Command cmdHide;
    private Command cmdPause;
    private Command cmdDestroy;
    
    /** main midlet screen */
    private Form form;
    
    /**
     * public constructor
     *
     * @param dummyName name of the MIDlet (i.e.name of its its screen)
     *
     */
    public DummyNamsMIDlet(String dummyName) {
        int i;

        name = dummyName;
        
        cmdDestroy = new Command("Destroy", Command.EXIT, 0);
        cmdPause = new Command("Pause", Command.STOP, 0);
        cmdHide = new Command("Hide", Command.SCREEN, 0);
        cmdMgr = new Command("Show Manager", Command.SCREEN, 0);
        
        form = new Form(name + " screen");
        
        form.addCommand(cmdMgr);
        form.addCommand(cmdHide);
        form.addCommand(cmdPause);
        form.addCommand(cmdDestroy);

        form.setCommandListener(this);

        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "DEBUG: " + name + ": created in isolate #" + 
           MIDletSuiteUtils.getIsolateId());
    }
    
    protected void startApp() throws MIDletStateChangeException {
        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "DEBUG: " + name + ": entered active state ...");
        
        Display.getDisplay(this).setCurrent(form);
    };

    protected void pauseApp() {
        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "DEBUG: " + name + ": entered paused state ...");
        notifyPaused();
    }

    protected void destroyApp(boolean unconditional)
	throws MIDletStateChangeException {
        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "DEBUG: " + name + ": entered destroyed state ...");
        notifyDestroyed();
    }

    public void commandAction(Command c, Displayable s) {

        if (c == cmdDestroy) {
            /*
            Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                "DEBUG: " + name + ": user requested to destroy ...");
             */
            try {
                destroyApp(true);
            } catch (MIDletStateChangeException e) {};
        } else if (c == cmdPause) {
            /*
            Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                "DEBUG: " + name + ": user requested to pause ...");
             */
            pauseApp();
        } else if (c == cmdHide) {
            /*
            Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                "DEBUG: " + name + ": user requested to go to background ...");
             */
            NamsManager.midletSetBackground();
        } else if (c == cmdMgr) {
            /*
            Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                "DEBUG: " + name + ": user requested to show NamsManager ...");
             */
            NamsManager.midletSetForeground(0);
        }
    }
}
