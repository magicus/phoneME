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

package com.sun.appmanager;

import com.sun.appmanager.presentation.PresentationMode;
import com.sun.appmanager.apprepository.AppRepository;
import com.sun.appmanager.preferences.Preferences;
import com.sun.appmanager.store.PersistentStore;
import com.sun.appmanager.mtask.Client;
import com.sun.appmanager.ota.OTAFactory;

import java.rmi.registry.Registry;
import java.util.ResourceBundle;

/**
 * 
 * <tt>AppManager</tt> is the superclass that contains handles to each
 * of the AppManager modules. Only one <tt>AppManager</tt> instance
 * can be created. Subsequent instances cause a
 * <tt>RuntimeException</tt>
 *
 */
abstract public class AppManager {

    protected static PresentationMode presentationMode;
    protected static AppRepository    appRepository;
    protected static Preferences      preferences;
    protected static Client           mtaskClient;
    protected static OTAFactory       otaFactory;
    protected static PersistentStore  persistentStore;
    protected static Registry         registry;
    protected static ResourceBundle   appManagerRB;

    private static AppManager instance = null;
    
    /**
     * Create a singleton instance.
     *
     * <p>If more than one instance is created, a
     * <tt>RuntimeException</tt> is thrown.
     */
    public AppManager() {
        if (instance == null) {
            instance = this;
	    appManagerRB =
		ResourceBundle.getBundle( "com.sun.appmanager.resources.AppManagerResources" );
        } else {
            throw new RuntimeException(
	        appManagerRB.getString( "OnlyOneAppManInstance" ) );
        }
    }

    public static PresentationMode getPresentationMode() 
    {
        return instance.presentationMode;
    }
    
    public static AppRepository getAppRepository()
    {
        return instance.appRepository;
    }
    
    public static Preferences getPreferences()
    {
        return instance.preferences;
    }
    
    public static Client getMtaskClient()
    {
        return instance.mtaskClient;
    }

    public static OTAFactory getOTAFactory()
    {
        return instance.otaFactory;
    }
    
    public static PersistentStore getPersistentStore()
    {
        return instance.persistentStore;
    }

    public static ResourceBundle getResourceBundle()
    {
        return instance.appManagerRB;
    }
    
    public static Registry getRegistry()
    {
        return instance.registry;
    }
    
}
