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

import com.sun.jump.module.JUMPModule;

/**
 * <code>JUMPPreferencesModule</code> provides the ability to maintain a set of
 * persistent preferences, in the form of key-value pairs. This class can be
 * viewed as encapsulating a namespace of key-value pairs.
 * 
 * <p>An implementation is free to commit the set of preferences to permanent
 * store when it chooses, allowing in-memory caching if necessary. The use of
 * the <code>save()</code> method is recommended to force persistence of this
 * JUMPPreferencesModule.
 * 
 * <p>Preference namespaces retain their contents across runs of the
 * device. So upon initialization, a JUMPPreferencesModule instance of a given
 * type name is expected to re-construct its latest known saved state.
 */
public interface JUMPPreferencesModule extends JUMPModule {
    
    /**
     * @param prefName   the name of the preference to look up
     * @return value of the preference
     */
    public String getPreference(String prefName);

    /**
     * @param prefName   the name of the preference to set
     * @param prefValue  the value to set the preference to
     * @return value of the preference
     */
    public String setPreference(String prefName, String prefValue);

    /**
     * @param prefName   the name of the preference to delete
     */
    public void deletePreference(String prefName);

    /**
     * @return a list of names of preferences.
     */
    public String[] getPreferenceNames();

    /**
     * Force persistence of the namespace. 
     */
    public void save();
}
