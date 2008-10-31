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

package com.sun.midp.automation.example;

import com.sun.midp.automation.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.io.*;
import java.io.*;
import java.util.*;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.midletsuite.MIDletSuiteCorruptedException;

/**
 * Player that replays events scenario file (see "scenarios" directory 
 * for example scenarios) for specified MIDlet. The Player has three 
 * arguments:
 *  - URL of MIDlet to install and run
 *  - URL of scenario file
 *  - [optional] Speed divider
 *  For example, assuming that MIDlet and scenario files are stored on 
 *  localhost, you run the player like that:
 *   runMidlet internal com.sun.midp.automation.example.Player 
 *   http://localhost/LWUITDemo.jad http://localhost/LWUITDemo.txt
 */
public class Player extends MIDlet implements Runnable {
    /** If true, print some debug info while running */
    private boolean debug = false;

    /** URL of MIDlet to install and run */
    private String midletURL = null;

    /** URL of scenario to replay */
    private String scenarioURL = null;

    /** Speed adjustment divider */
    private double speedAdjustment;

    /** True, if speed divider has been specified */
    private boolean hasSpeedAdjustment = false;

    /** Our Thread */
    private Thread run = null;

    /** Automation instance */
    private Automation automation = null;

    /** AutoSuiteStorargwe instance for installing/removing MIDlet */
    private AutoSuiteStorage storage = null;

    /** AutoSuiteDescriptor corresponding to MIDlet to run */
    private AutoSuiteDescriptor midletSuite = null;

    /** Running MIDlet */
    private AutoMIDlet midlet = null;

    /** Event sequence created from scenario file */
    private AutoEventSequence scenario = null;    

    public Player() {
        automation = Automation.getInstance();
        storage = automation.getStorage();
    }

    /**
     * Pausing this MIDlet has no special processing.
     */
    public void pauseApp() {
    }

    public void startApp() {
        run = new Thread(this);
        run.setPriority(Thread.MAX_PRIORITY);
        run.start();
    } 

    public void destroyApp(boolean unconditional) {
        try {
            if (midlet != null) {
                midlet.switchTo(AutoMIDletLifeCycleState.DESTROYED, true);
            }
            uninstallMIDlet();
        } catch (Throwable e) {
            reportException(e, false);
        }
    }    

    public void run() {
        midletURL = getAppProperty("arg-0");
        if (midletURL == null) {
            reportException(
                    new RuntimeException("MIDlet URL is not specified"),
                    true);
            return;
        }

        scenarioURL = getAppProperty("arg-1");
        if (scenarioURL == null) {
            reportException(
                    new RuntimeException("Scenario URL is not specified"),
                    true);
            return;
        }

        String sSpeed = getAppProperty("arg-2");
        if (sSpeed != null) {
            hasSpeedAdjustment = true;
            speedAdjustment = Double.parseDouble(sSpeed);
        }

        report("MIDlet URL: " + midletURL);
        report("Scenario URL: " + scenarioURL);
        if (hasSpeedAdjustment) {
            report("Speed adjustment: " + speedAdjustment);
        }

        try {
            installMIDlet();
            downloadScenario();
            startMIDlet();

            // replay scenario
            if (hasSpeedAdjustment) {
                automation.simulateEvents(scenario, speedAdjustment);
            } else {
                automation.simulateEvents(scenario);
            }
        } catch (Throwable e) {
            reportException(e, false);
        }

        destroyApp(true);
        notifyDestroyed();        
    }

    private void reportException(Throwable e, boolean destroy) {
        e.printStackTrace();
        System.out.println("*** " + this.getClass().getName() + 
                " - exception: " + e);

        if (destroy) {
            destroyApp(true);
            notifyDestroyed();
        }
    }

    private void report(String message) {
        if (debug) {
            System.out.println(message);
        }
    }

    private void installMIDlet() 
        throws IOException, 
               MIDletSuiteLockedException, MIDletSuiteCorruptedException {

        midletSuite = storage.installSuite(midletURL);
    }

    private void uninstallMIDlet() 
        throws IOException, MIDletSuiteLockedException {

        if (midletSuite != null) {
            storage.uninstallSuite(midletSuite);
        }
    }

    private void downloadScenario() 
        throws IOException {

        StringBuffer b = new StringBuffer();
        InputStream is = null;
        HttpConnection c = null;
        boolean comment = false;
        
        try {
            long len = 0;
            int ch = 0;
            c = (HttpConnection)Connector.open(scenarioURL);
            is = c.openInputStream();
            len = c.getLength();
            if (len != -1) {
	            // Read exactly Content-Length bytes
   	            for (int i = 0; i < len; i++) {
	                if ((ch = is.read()) != -1) {
                        if (ch == (int)'\n' && comment) {
                            comment = false;
                        }

                        if (ch == (int)'#') {
                            comment = true;
                        }

                        if (!comment) {
	                        b.append((char)ch);
                        }
                    }
                }
            } else {
                // Read until the connection is closed
	            while ((ch = is.read()) != -1) {
                    len = is.available();
                    if (ch == (int)'\n' && comment) {
                        comment = false;
                    }

                    if (ch == (int)'#') {
                        comment = true;
                    }
                    
                    if (!comment) {
	                    b.append((char)ch);
                    }
                }
            }
            
            AutoEventFactory eventFactory = automation.getEventFactory();
            scenario = eventFactory.createFromString(b.toString());
            report("scenario:\n" + scenario.toString());
        } finally {
            if (is != null) {
                is.close();
            }

            if (c != null) {
                c.close();
            }
        }
    }

    private void startMIDlet() {
        // Start the first MIDlet in suite
        Vector midlets = midletSuite.getSuiteMIDlets();
        AutoMIDletDescriptor midletDescr = 
            (AutoMIDletDescriptor)midlets.elementAt(0);
        midlet = midletDescr.start(null);

        // Initial MIDlet state is PAUSED, switch to ACTIVE
        midlet.switchTo(AutoMIDletLifeCycleState.ACTIVE, true);

        // Bring MIDlet to foreground
        midlet.switchTo(AutoMIDletForegroundState.FOREGROUND, true);
    }
}
