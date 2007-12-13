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

import javax.microedition.lcdui.*;

/**
 * Main view of the ODTAgentMIDlet to display information to the user.
 * Each incoming request, will be logged to the screen in a short string.
 *
 * @author Roy Ben Hayun
 */
public class ProgressScreen extends TextBox {
    //
    // Members
    //
    
    private static final int MAX_SIZE = 5096; 
    private static final String INIT_MSG = "Please start the server.";
    private static final String TRUNCATED_MSG = "...";
    
    
    //
    // Members
    //
    
    //TODO: use hh:mm:ss parsed from java.util.Date
    private long startTime = System.currentTimeMillis();
    
    //
    // Life cycle
    //
    
    /**
     * Creates a new instance of ProgressScreen
     */
    public ProgressScreen() {
        super("ODTAgent", "", MAX_SIZE, TextField.UNEDITABLE);
        log(INIT_MSG);
        //TODO: display warning Alert, requesting confirmation to run the agent
        setTicker(new Ticker("ODD-Agent is running. Please exit if you do not need it running"));
    }
    
    //
    // Operations
    //
    
    /**
     * Log message in progress screen
     *
     * @param msg message to the user
     */
    public void log(String msg){
        //TODO: use hh:mm:ss parsed from java.util.Date
        msg = (System.currentTimeMillis() - startTime)  + ": " + msg + "\n";
        if((size() + msg.length()) >= MAX_SIZE){
            clear();
        }
        insert(msg, getString().length());
    }
    
    /**
     * Clean progress screen
     */
    public void clear(){
        log(TRUNCATED_MSG);
        setString("");
    }
}
