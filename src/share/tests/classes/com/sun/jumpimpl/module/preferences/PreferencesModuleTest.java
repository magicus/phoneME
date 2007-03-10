/*
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

package com.sun.jumpimpl.module.preferences;

import junit.framework.*;

import com.sun.jump.module.contentstore.*;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jumpimpl.process.JUMPModulesConfig;
import com.sun.jump.module.preferences.*;
import com.sun.jumpimpl.module.preferences.*;

public class PreferencesModuleTest extends TestCase {

    JUMPPreferencesModuleFactory prefModule = null;
    JUMPPreferencesModule sysModule = null;
    boolean debugmode = false;

    public PreferencesModuleTest() {        
        initTest();
        String dprop=System.getProperty("debug");
        if ((dprop !=null) && (dprop.equals("true"))){
            debugmode=true;
            System.err.println("Debug Mode Turned ON!");
        }
    }

    private void initTest() {
        // Executive setup for testing
        if (JUMPExecutive.getInstance() == null) {
            JUMPStoreFactory factory = 
                new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();
            factory.load(JUMPModulesConfig.getProperties());
            new com.sun.jumpimpl.module.preferences.PreferencesModuleFactoryImpl();
        }        

        prefModule = JUMPPreferencesModuleFactory.getInstance();

        if (prefModule != null) {
            sysModule = 
                prefModule.getModule
                (JUMPPreferencesModuleFactory.TYPE_SYSTEM_PREFERENCES);
        }
    }

    public void testValidPreferenceType() {
        if (prefModule != null) {
            try {
                JUMPPreferencesModule module = 
                    prefModule.getModule("Invalid-Type");
            } catch (IllegalArgumentException success) {
                assertNotNull(success.getMessage());
            }
        }
    }

    public void testSetGetPreferences() {

        if (sysModule != null) {
            
            printDebug("Starting Set&Get Preferences Test:");

            setPreferences();

            // Get the preferences
            String colorVal = sysModule.getPreference("Color");
            String fontVal = sysModule.getPreference("Font");
            String modeVal = sysModule.getPreference("Mode");

            // check if they were set fine
            assertEquals("Gray", colorVal);
            assertEquals("Sansaref", fontVal);
            assertEquals("fullscreen", modeVal);

            // Debug: print the preferences
            printDebug("Color Value = "+sysModule.getPreference("Color"));
            printDebug("Font Value = "+sysModule.getPreference("Font"));
            printDebug("Mode Value = "+sysModule.getPreference("Mode")); 
        }
    }

    public void testDeletePreferences() {

        if (sysModule != null) {

            printDebug("Starting Delete Preferences Test:");

            setPreferences();
            // delete "Color" preference                   
            sysModule.deletePreference("Color");            
            // check if it got deleted
            String colorVal = sysModule.getPreference("Color");
            assertEquals(null, colorVal);

            printDebug("Deleted Color Preference => "+
                       sysModule.getPreference("Color"));      
        }
        else {
            printDebug("JUMPPreferencesModuleFactory instance NULL");
        }        
    }

    private void setPreferences() {
        // Set some system preferences 
        sysModule.setPreference("Color", "Gray");
        sysModule.setPreference("Font", "Sansaref");
        sysModule.setPreference("Mode", "fullscreen");        
        // save the data
        sysModule.save();
    }   

    private void printDebug(String msg) {
        if (debugmode) {
            System.err.println(msg);
        }
    }
    
}
