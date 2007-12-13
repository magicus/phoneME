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

/**
 * Data class encapsulating the user settings.
 * The SettingsScreen is the view.
 *
 * @author Roy Ben Hayun
 */
public class AgentSettings {
    
    
    //
    // Members
    //

    /**
     * Indicates if installation should be silent, with no prompts
     */
    boolean silentInstallation = true;

    /**
     * Indicates if pin authentication is required, when connection received
     */
    boolean pinRequired = true;
    
    /**
     * Indicates if only signed suites are allowed to be installed
     */
    boolean signedOnly = true;

    //
    // Life cycle
    //
    
    /**
     * C'tor
     */
    AgentSettings(){
        try{
            load();
        }
        catch(Exception e){
            e.printStackTrace();
        }
    }
    
    //
    // Operations
    //
    
    
    /**
     *Load settings from persistant storage 
     */
    void load() throws Exception{
        //TODO: Alexey Z - load from persistant storage e..g., file or RMS
        //if loading fails, use default values
    }
    
    /**
     *Save settings to persistant storage 
     */
    void save() throws Exception{
        //TODO: Alexey Z - save to persistant storage e..g., file or RMS
    }
    
}
