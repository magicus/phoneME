/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.preferences;

/**
 * An interface that defines access to preferences.
 *
 * <p><i>Preferences</i> are string based key-value pairs. 
 *
 * <p>There are two kinds of preferences: <i>user</i> preferences, and
 * <i>system</i> preferences. User preferences can be thought of as
 * multiple string-indexed preference <i>namespaces</i>. Preferences
 * for one user are completely disjoint from preferences for another
 * user.  Overlap of a preference name for different user names is therefore
 * not a conflict.  System preferences on the other hand are
 * <i>global</i>; ie they apply to the entire system regardless of the
 * user namespaces.
 *
 * <p>An implementation of Preferences should allow for persistence of
 * user and system preferences.  The <tt>save*</tt> methods below
 * handle persistence explicitly.  
 */
public interface Preferences {
    /**
     * Set preference <tt>prefName</tt> to <tt>prefValue</tt> for user
     * <tt>userName</tt>.  
     */
    void setUserPreference(String userName, String prefName, String prefValue);

    /**
     * Get preference <tt>prefName</tt> for user <tt>userName</tt>.  
     */
    String getUserPreference(String userName, String prefName);

    /**
     * Set system preference <tt>prefName</tt> to <tt>prefValue</tt>
     */
    void setSystemPreference(String prefName, String prefValue);

    /**
     * Get system preference <tt>prefName</tt>
     */
    String getSystemPreference(String prefName);

    /**
     * Get names of all system preferences
     */
    String[] getAllSystemPreferenceNames();

    /**
     * Get names of all user preferences corresponding to 'user'
     */
    String[] getAllUserPreferenceNames(String user);

    /**
     * Get names of all users 
     */
    String[] getAllUserNames();

    /**
     * Make all user preferences and system preferences persistent
     */
    void saveAll();

    /**
     * Save preferences for user <tt>userName</tt>
     */
    void saveUserPreferences(String userName);

    /**
     * Save all preferences for user <tt>userName</tt>
     */
    void deleteUserPreferences(String userName);

    /**
     * Save all system preferences
     */
    void saveSystemPreferences();

}
