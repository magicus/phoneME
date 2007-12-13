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
 * View of the user settings (acts as a view for the AgentSettings data class )
 * The user can change the settings from this screen.
 *
 * @author Roy Ben Hayun
 */
public class SettingsScreen extends Form implements CommandListener{
    
    //
    // Constants
    //
    
    /**
     * Displayable title
     */
    private static final String TITLE = "ODD-Agent settings";
    
    /**
     * OK label
     */
    private static final String OK_LABEL = "OK";
    
    /**
     * Back command label
     */
    private static final String BACK_LABEL= "Back";

    /**
     * Silent installation choice label
     */
    private static final String SILENT_INSTALLATION = "Silent installation";
    
    /**
     * Pin required choice label
     */
    private static final String PIN_REQUIRED = "Request pin authentication";

    /**
     * Signed suites only choice label
     */
    private static final String SIGNED_ONLY = "Allow signed suites only";
    
    
    //
    // Members
    //
    
    private ChoiceGroup choices;
    
    /**
     * MIDlet agent
     */
    private final ODTAgentMIDlet agent;
    
    /**
     * Agent settings
     */
    private final AgentSettings settings;
    
    //
    // Life cycle
    //
    
    /**
     * Creates a new instance of SettingsScreen
     */
    public SettingsScreen(ODTAgentMIDlet agent, AgentSettings settings) {
        super(TITLE);
        this.agent = agent;
        this.settings = settings;
        
        //init check-boxes
        choices = new ChoiceGroup(TITLE, ChoiceGroup.MULTIPLE);
        choices.append(SILENT_INSTALLATION, null);
        choices.append(PIN_REQUIRED, null);
        choices.append(SIGNED_ONLY, null);
        append(choices);
        
        //init commands
        addCommand(new Command(OK_LABEL, Command.OK, 0));
        addCommand(new Command(BACK_LABEL, Command.BACK, 1));
        setCommandListener(this);
    }
    
    
    //
    // Operations
    //
    
    /**
     * Set the selected choices according to the agent settings
     */
    void initChoices(){
        boolean[] flags = new boolean[3];
        flags[0] = settings.silentInstallation;
        flags[1] = settings.pinRequired;
        flags[2] = settings.signedOnly;
        choices.setSelectedFlags(flags);
    }
    
    
    //
    // CommandListener implementation
    //
    
    
    /**
     * Handle command events
     *
     * @param command command event source
     * @param displayable displayable
     */
    public void commandAction(Command command, Displayable displayable) {
        if(command.getCommandType() == Command.OK){
            //update settings
            boolean[] flags = new boolean[3];
            choices.getSelectedFlags(flags);
            //TODO: if not all flags are 'true', display Alert to warn the user that the recommended option is true for all
            settings.silentInstallation = flags[0];
            settings.pinRequired = flags[1];
            settings.signedOnly = flags[2];
        }
        
        //back to progress screen        
        agent.displayProgressScreen();
    }
    
    
}