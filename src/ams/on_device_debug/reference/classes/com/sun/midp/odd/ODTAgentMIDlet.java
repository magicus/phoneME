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

/*
 * ODTAgent.java
 *
 * Created on 3 דצמבר 2007, 15:46
 */
package com.sun.midp.odd;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

/**
 * On-Device-Debugging agent MIDlet.
 * ODTAgentMIDlet is an extension of the AMS, which is responsible of
 * 1. Handle incoming requests from the PC side 
 * 2. Manage debug sessions lifecycle (install, run, uninstall)
 * 
 * @author  Roy Ben Hayun
 * @version
 */
//TODO: Alexey Z - integrate ODTAgentMIDlet with JAMS (e.g., implement required interfaces)
public class ODTAgentMIDlet extends MIDlet {
    //
    // Members
    //
    
    /**
     * View to display session progress and feedback to user
     */
    private final ProgressScreen progressScreen;
    
    /**
     * Screen to modify user settings (see AgentSettings data type)
     */
    private final SettingsScreen settingsScreen;
    
    /**
     * ODD engine
     */
    private final ODTEngine engine;
    
    //
    // MIDlet life cycle
    //
    
    /**
     * Create new instance of ODTAgentMIDlet
     */
    public ODTAgentMIDlet(){
        progressScreen = new ProgressScreen();
        AgentSettings settings = new AgentSettings();
        settingsScreen = new SettingsScreen(this, settings);
        engine = new ODTEngine(progressScreen, settings);
        ODTMediator mediator = new ODTMediator(progressScreen, engine, this);
    }
    
    public void startApp() {
        //TODO: Alexey Z, Jan Sterba - ensure resources utilization handled (e.g., connections)
        displayProgressScreen();
    }
    
    public void pauseApp() {
        //TODO: Alexey Z, Jan Sterba - ensure resources utilization handled (e.g., connections)
    }
    
    public void destroyApp(boolean unconditional) {
        exit();
    }
    
    //
    // Operations
    //
    
    /**
     * Display settings screen
     */
    void displaySettingsScreen(){
        settingsScreen.initChoices();
        Display.getDisplay(this).setCurrent(settingsScreen);
    }
    
    /**
     * Display progress screen
     */
    void displayProgressScreen(){
        Display.getDisplay(this).setCurrent(progressScreen);        
    }
    
    /**
     * Exit application.
     * Perform shutdown.
     *
     */
    void exit(){
        //TODO: Alexey Z, Jan Sterba - ensure proper shutdown
        engine.shutdown();
        super.notifyDestroyed();
        
    }
}
