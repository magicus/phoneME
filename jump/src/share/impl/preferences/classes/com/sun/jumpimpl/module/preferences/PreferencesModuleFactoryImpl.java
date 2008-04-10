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

import com.sun.jump.module.preferences.JUMPPreferencesModuleFactory;
import com.sun.jump.module.preferences.JUMPPreferencesModule;
import java.util.Map;
import java.util.Vector;

public class PreferencesModuleFactoryImpl extends JUMPPreferencesModuleFactory {
    // singleton instances
    private static JUMPPreferencesModule systemPreferencesModule = null;
    private Map configData; 

    /**
     * Returns a <code>JUMPPreferencesModule</code> for the type specified.
     * 
     * @param type the type or namespace of the preferences. 
     * The supported values are
     *        <ul>
     *          <li>{@link #TYPE_SYSTEM_PREFERENCES}</li>
     *          <li>{@link #TYPE_APPLICATION_PREFERENCES}</li>
     *        </ul>
     * @throws java.lang.IllegalArgumentException if the type is not 
     *         not supported by the factory.
     */
    public JUMPPreferencesModule getModule(String type) {
        if (type.equals(JUMPPreferencesModuleFactory.TYPE_SYSTEM_PREFERENCES))
            return getSystemPreferencesModule();
        else throw new IllegalArgumentException("Preferences Type "+type+" not supported; only System preferences type is supported");

    }

   private synchronized JUMPPreferencesModule getSystemPreferencesModule() {
       if (systemPreferencesModule == null) {
           systemPreferencesModule = new SystemPreferencesModuleImpl();
           systemPreferencesModule.load(configData);
       }
       return systemPreferencesModule;
   }

    /**
     * @return a list of all registered preferences in the system for all
     * namespaces.
     */
    public JUMPPreferencesModule[] getAllPreferences() {
        Vector moduleVector = new Vector();
        JUMPPreferencesModule systemPrefs = getSystemPreferencesModule();
        if (systemPrefs != null) moduleVector.add(systemPrefs);
        return (JUMPPreferencesModule[]) moduleVector.toArray(new JUMPPreferencesModule[]{});
    }

    /**
     * load this module with the given properties
     * @param map properties of this module
     */
    public void load(Map config) {
        this.configData = config;
    }
    
    /**
     * unload the module
     */
    public void unload() {
        if (systemPreferencesModule != null) 
            systemPreferencesModule.unload();
        systemPreferencesModule = null;       
    }

}
