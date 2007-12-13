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

package com.sun.midp.odd;

import com.sun.midp.odd.remoting.DummyUEIProxy;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

/**
 * Handler for screen commands  (see GoF Mediator pattern)
 *
 * @author Roy Ben Hayun
 */
public class ODTMediator implements CommandListener {
    //
    // Members
    //
    
    //Commands
    private final Command exitCommand = new Command("Exit", Command.EXIT, 0);
    private final Command startCommand = new Command("Start server", Command.SCREEN, 1);
    private final Command stopCommand = new Command("Stop server", Command.SCREEN, 2);
    private final Command clearCommand = new Command("Clear screen", Command.SCREEN, 3);
    private final Command settingsCommand = new Command("Settings", Command.SCREEN, 4);
    
    /**
     * Progress screen
     */
    private final ProgressScreen progressScreen;
    
    /**
     * Engine
     */
    private final ODTEngine engine;
    
    /**
     * MIDlet
     */
    private final ODTAgentMIDlet agent;
    
    
    //
    // Life cycle
    //
    
    /**
     * Creates a new instance of CommandsMediator
     */
    public ODTMediator(ProgressScreen ps, ODTEngine eng, ODTAgentMIDlet agent) {
        progressScreen = ps;
        engine = eng;
        this.agent = agent;
        progressScreen.addCommand(exitCommand);
        progressScreen.addCommand(startCommand);
        progressScreen.addCommand(stopCommand);
        progressScreen.addCommand(clearCommand);
        progressScreen.addCommand(settingsCommand);
        progressScreen.setCommandListener(this);
    }
    
    //
    // CommandListener implementation
    //
    
    public void commandAction(Command command, Displayable displayable) {
        if(command == exitCommand){
            //TODO: shutdown running session, and exit
        } else if(command == startCommand){
            if(engine.isServerRunning()){
                progressScreen.log("Server already started.");
            } else{
                engine.startAcceptingConnections();
                progressScreen.log("Server started.");
                progressScreen.log("waiting for connection...");
                //TODO: Jan Sterba - remove this line. it is for prototyping only
            }
            DummyUEIProxy.runSequence(engine);
        } else if(command == stopCommand){
            engine.stopAcceptingConnections();
            progressScreen.log("Server stopped");
        } else if(command == clearCommand){
            progressScreen.clear();
        } else if(command == settingsCommand){
            agent.displaySettingsScreen();
        } else if(command == exitCommand){
            agent.exit();
        }
    }
    
    
    
}
