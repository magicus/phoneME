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

import com.sun.jump.module.preferences.JUMPPreferencesModule;
import com.sun.jump.module.contentstore.JUMPStoreHandle;


public class SystemPreferencesModuleImpl extends PreferencesStore implements JUMPPreferencesModule {

    /**
     * Name of directory where preferences will be stored
     */
    private static String REPOSITORY_SYSTEM_PREFERENCES_DIRNAME =
	"./preferences/System";

    /*
     * Name of the file that will store the preferences
     */
    private static String SYSTEM_PREFERENCES_FILE_NAME =
	"./preferences/System/systemPreferences.props";

    /*
     * Name of the file that will store default preferences
     */
    private static String DEFAULT_SYSTEM_PREFERENCES_FILE_NAME =
	"./preferences/System/defaultSystemPreferences.props";    

    //
    // A single instance of system-wide preferences
    // and the defaults
    //
    private PersistentPreferences systemPreferences;
    private PersistentPreferences defaultSystemPreferences;
    private JUMPStoreHandle storeHandle = null;

    public SystemPreferencesModuleImpl() {

        // Get a handle to content store 
        storeHandle = openStore(true);

        try {
            // create preferences root dir if it doesn't already exist.
            if (storeHandle.
                getNode(REPOSITORY_PREFERENCES_ROOT_DIRNAME) == null) {
                storeHandle.createNode(REPOSITORY_PREFERENCES_ROOT_DIRNAME);
            }
            
            if (storeHandle.
                getNode(REPOSITORY_SYSTEM_PREFERENCES_DIRNAME) == null) {
                storeHandle.createNode(REPOSITORY_SYSTEM_PREFERENCES_DIRNAME);
            }

            // Default SystemPreferences
            defaultSystemPreferences = 
                new PersistentPreferences(DEFAULT_SYSTEM_PREFERENCES_FILE_NAME,
                                              storeHandle);
           
            // SystemPreferences
            systemPreferences = 
                new PersistentPreferences(defaultSystemPreferences,
                                          SYSTEM_PREFERENCES_FILE_NAME, 
                                          storeHandle);                        

        } catch (Exception ex) {
            ex.printStackTrace();
        }

        closeStore(storeHandle);
    }

    /**
     * @param prefName   the name of the preference to look up
     * @return value of the preference
     */
    public String getPreference(String prefName) {
        return systemPreferences.get(prefName);
    }

    /**
     * @param prefName   the name of the preference to set
     * @param prefValue  the value to set the preference to
     * @return value of the preference
     */
    public String setPreference(String prefName, String prefValue) {
	systemPreferences.set(prefName, prefValue);
        return prefValue;
    }

    /**
     * @param prefName   the name of the preference to delete
     */
    public void deletePreference(String prefName) {
        systemPreferences.delete(prefName);
    }

    /**
     * @return a list of names of preferences.
     */
    public String[] getPreferenceNames() {
	return systemPreferences.getNames();
    }

    /**
     * Force persistence of the namespace. 
     */
    public void save() {
        storeHandle = openStore(true);
        systemPreferences.save(storeHandle);
        closeStore(storeHandle);
    }    

}

