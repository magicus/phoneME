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

import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventCopyStorage;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Works as a simple (display) manager to test NAMS.
 */
public class NamsManager extends MIDlet implements CommandListener {

    /**
     * The timeout before the running MIDlet is killed.
     */
    private static final int destroyTimeout =
        Configuration.getIntProperty("destroyMIDletTimeout", 5000);
    
    /** stores event copies for native events being tracked */
    static private EventCopyStorage eventCopyStorage = new EventCopyStorage();

    /** array if IDs for dummy midlets */
    private int dummyId[];

    private Command cmdExit;
    private Command cmdHide;
    private Command cmdRefresh;
    private Command cmdStart[];
    private Command cmdActivate[];
    private Command cmdPause[];
    private Command cmdDestroy[];
    private Command cmdI3Framework;

    /** MIDlet's screen */
    private Form form;

    /** reference to MIdlet's Display */
    private Display display;

    /**
     * public constructor
     */
    public NamsManager() {

        int i, j;
        EventQueue eventQueue;

        // appId = 0 is reserved for NamsManager itself
        initNamsManager(MIDletSuiteUtils.getIsolateId());

        dummyId = new int [3];

        cmdExit = new Command("Destroy", Command.EXIT, 0);
        cmdHide = new Command("Hide", Command.SCREEN, 0);
        cmdRefresh = new Command("Refresh MIDlet status", Command.SCREEN, 0);

        cmdStart = new Command[3];
        cmdActivate = new Command[3];
        cmdPause = new Command[3];
        cmdDestroy = new Command[3];

        for (i = 0; i < 3; ++i) {
            dummyId[i] = 0;
            cmdStart[i] = new Command("Start Dummy#" + (i+1),
                    Command.SCREEN, 0);
            cmdActivate[i] = new Command("Activate Dummy#" + (i+1),
                    Command.SCREEN, 0);
            cmdPause[i] = new Command("Pause Dummy#" + (i+1),
                    Command.SCREEN, 0);
            cmdDestroy[i] = new Command("Destroy Dummy#" + (i+1),
                    Command.SCREEN, 0);
        }
        cmdI3Framework = new Command("Start I3 Nams Framework",
                Command.SCREEN, 0);

        form = new Form("NAMS Manager screen");

        form.addCommand(cmdRefresh);
        form.addCommand(cmdExit);
        form.addCommand(cmdHide);
        for (i = 0; i < 3; ++i) {
            form.addCommand(cmdStart[i]);
            form.addCommand(cmdActivate[i]);
            form.addCommand(cmdPause[i]);
            form.addCommand(cmdDestroy[i]);
        }
        form.addCommand(cmdI3Framework);

        form.setCommandListener(this);

        display = Display.getDisplay(this);
        display.setCurrent(form);
    }

    private native void initNamsManager(int isolateId);

    protected void startApp() throws MIDletStateChangeException {
        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "NamsManager MIDlet: entered active state ...");

        display.setCurrent(form);
    };

    protected void pauseApp() {
        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "NamsManager MIDlet: entered paused state ...");

        resumeRequest();
    }

    protected void destroyApp(boolean unconditional)
	throws MIDletStateChangeException {
        /* if (unconditional) */ {
            Logging.report(Logging.WARNING, LogChannels.LC_CORE,
               "NamsManager MIDlet: entered destroyed state ...");
            notifyDestroyed();
            MIDletProxyList list = MIDletProxyList.getMIDletProxyList();
            if (list != null)
                list.shutdown();
            else {
            Logging.report(Logging.WARNING, LogChannels.LC_CORE,
               "NamsManager MIDlet: Proxy List is NULL !!!");
            }
        }
        /* else {
            throw new MIDletStateChangeException(
                    "NAMS Manager: I DO NOT want to be destroyed !");
        } */
    }

    public void commandAction(Command c, Displayable s) {

        if (c == cmdRefresh) {
            refresh();
        } else if (c == cmdExit) {
            try {
                destroyApp(true);
            } catch (MIDletStateChangeException e) {};
        } else if (c == cmdHide) {
            midletSetForeground(findNextForegroundMIDlet(0));
        } else if (c == cmdI3Framework) {
            startInternalMidlet("com.sun.midp.i3test.NamsFramework",
                    /* null, null, null */
                    false, false);
        } else {
            int i;
            for (i = 0; i < 3; ++i) {
                if (c == cmdStart[i]) {
                    dummyId[i] =
                            startInternalMidlet(
                                "com.sun.midp.main.DummyNamsMIDlet" + (i+1),
                                /* null, null, null */
                                false, false);
                } else if (c == cmdActivate[i]) {
                    if (dummyId[i] != 0) {
                        midletResume(dummyId[i]);
                    }
                } else if (c == cmdPause[i]) {
                    if (dummyId[i] != 0) {
                       midletPause(dummyId[i]);
                    }
                } else if (c == cmdDestroy[i]) {
                    if (dummyId[i] != 0) {
                       midletDestroy(dummyId[i]);
                    }
                    dummyId[i] = 0;
                }
            }
        }
    }

    static public void midletCreateStart(
            int suiteId,
            String className,
            /*
            String displayName,
            String arg0,
            String arg1,
            String arg2,
            */
            int appId) {

        if (NamsStorage.getMIDletStateTrack(appId)) {
            eventCopyStorage.putTail(
                    EventTypes.NATIVE_MIDLET_EXECUTE_REQUEST,
                    appId, suiteId, 0, 0,
                    className, null,
                    /* displayName, arg0, arg1, arg2 */
                    null, null, null, null);
        }

        NamsAPIWrapper.midletCreateStart(
                suiteId, className,
                /* displayName, arg0, arg1, arg2, */
                appId);
    }

    static public void midletResume(int appId) {
        if (NamsStorage.getMIDletStateTrack(appId)) {
            eventCopyStorage.putTail(
                    EventTypes.NATIVE_MIDLET_RESUME_REQUEST,
                    appId, 0, 0, 0,
                    null, null, null, null, null, null);
        }

        NamsAPIWrapper.midletResume(appId);
    }

    static public void midletPause(int appId) {
        if (NamsStorage.getMIDletStateTrack(appId)) {
            eventCopyStorage.putTail(
                    EventTypes.NATIVE_MIDLET_PAUSE_REQUEST,
                    appId, 0, 0, 0,
                    null, null, null, null, null, null);
        }

        NamsAPIWrapper.midletPause(appId);
    }

    static public void midletDestroy(int appId) {
        if (NamsStorage.getMIDletStateTrack(appId)) {
            eventCopyStorage.putTail(
                    EventTypes.NATIVE_MIDLET_DESTROY_REQUEST,
                    appId, -1, 0, 0,
                    null, null, null, null, null, null);
        }

        NamsAPIWrapper.midletDestroy(appId, destroyTimeout);
    }

    static public void midletSetForeground(int appId) {
        if (NamsStorage.getDisplayStatusTrack(appId)) {
            eventCopyStorage.putTail(
                    EventTypes.NATIVE_SET_FOREGROUND_REQUEST,
                    appId, 0, 0, 0,
                    null, null, null, null, null, null);
        }

        NamsAPIWrapper.midletSetForeground(appId);
    }

    static public void midletSetBackground() {
        int appId = getForegroundAppId();
        int nextFgAppId = findNextForegroundMIDlet(appId);
        if (nextFgAppId == appId)
            nextFgAppId = 0;

        midletSetForeground(nextFgAppId);
    }

    static int startInternalMidlet(String className,
        /* String arg0, String arg1, String arg2, */
        boolean midlet_state_track, boolean display_status_track) {
        int appId = findNextEmptyMIDlet(getForegroundAppId());

        if (appId == 0) {
            Logging.report(Logging.ERROR, LogChannels.LC_CORE,
               "NamsManager MIDlet: " +
               "Failed to start MIDlet: no IDs available ...");
        } else {
            NamsStorage.setMIDletStateTrack(appId, midlet_state_track);
            NamsStorage.setDisplayStatusTrack(appId, display_status_track);
            midletCreateStart(MIDletSuite.INTERNAL_SUITE_ID, className,
                    /* "Display#" + appId, arg0, arg1, arg2, */
                    appId);
        }
        return appId;
    }

    static native int findNextEmptyMIDlet(int appId);
    static native int findNextForegroundMIDlet(int appId);
    static native int getForegroundAppId();

    private String getMIDletStateString(int state) {
        switch (state) {
            case NamsStorage.NAMS_STATE_NO:
                return "NO";
            case NamsStorage.NAMS_STATE_ACTIVE:
                return "ACTIVE";
            case NamsStorage.NAMS_STATE_PAUSED:
                return "PAUSED";
            case NamsStorage.NAMS_STATE_DESTROYED:
                return "DESTROYED";
            case NamsStorage.NAMS_STATE_ERROR:
                return "ERROR";
            case NamsStorage.NAMS_STATE_RESERVED:
                return "RESERVED";
            default:
                return "INVALID";
        }
    }

    private void refresh() {
        int i;
        int midlet_state;
        boolean display_status;

        form.deleteAll();

        for (i = 0; i < NamsStorage.NAMS_STORAGE_SIZE; ++i) {
            midlet_state = NamsStorage.getMIDletState(i);
            display_status = NamsStorage.getDisplayStatus(i);

            if (midlet_state != NamsStorage.NAMS_STATE_NO || display_status) {

                form.append("#" + i +
                        " state=" + getMIDletStateString(midlet_state) +
                        " display=" + ((display_status) ? "FG" : "BG"));
            }
        }
    }
}

