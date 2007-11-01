/*
 * %W% %E%
 *
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

package com.sun.jump.module.preferences;

import com.sun.jump.module.JUMPModuleFactory;

/**
 * <code>JUMPPreferencesModuleFactory</code> is a factory for 
 * <code>JUMPPreferencesModule</code> instances. It handles
 * these instances constitute multiple disjoint namespaces of preferences.
 */
public abstract class JUMPPreferencesModuleFactory extends JUMPModuleFactory {

    //
    // Currently we only have system preferences, but in the future,
    //  we can add more namespaces like User / Application preferences.
    //
    public static final String TYPE_SYSTEM_PREFERENCES  = "system";
    
    private static JUMPPreferencesModuleFactory INSTANCE = null;
    
    public static JUMPPreferencesModuleFactory getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPPreferencesModuleFactory
     */
    protected JUMPPreferencesModuleFactory() {
        synchronized (JUMPPreferencesModuleFactory.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns a <code>JUMPPreferencesModule</code> for the type specified.
     * 
     * @param type the type or namespace of the preferences. 
     * The supported values are
     *        <ul>
     *          <li>{@link #TYPE_SYSTEM_PREFERENCES}</li>
     *        </ul>
     * @throws java.lang.IllegalArgumentException if the type is not 
     *         not supported by the factory.
     */
    public abstract JUMPPreferencesModule getModule(String type);

    /**
     * @return a list of all registered preferences in the system for all
     * namespaces.
     */
    public abstract JUMPPreferencesModule[] getAllPreferences();
}
