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


package com.sun.jumpimpl.module.installer;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.module.installer.JUMPInstallerModule;
import java.util.Map;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * Factory implementation methods for the installer module
 */
public class InstallerFactoryImpl extends JUMPInstallerModuleFactory {
    private JUMPInstallerModule mainInstaller;
    private JUMPInstallerModule xletInstaller;
    private JUMPInstallerModule midletInstaller;
    private Map configMap;
    
    private JUMPInstallerModule getMainInstaller() 
    {
	synchronized(InstallerFactoryImpl.class) {
	    if (mainInstaller == null) {
		mainInstaller = new MAINInstallerImpl();
		mainInstaller.load(configMap);
	    }
	    return mainInstaller;
	}
    }
	
    private JUMPInstallerModule getXletInstaller() 
    {
	synchronized(InstallerFactoryImpl.class) {
	    if (xletInstaller == null) {
		xletInstaller = new XLETInstallerImpl();
		xletInstaller.load(configMap);
	    }
	    return xletInstaller;
	}
    }
	
    private JUMPInstallerModule getMidletInstaller() 
    {
	synchronized(InstallerFactoryImpl.class) {
	    if (midletInstaller == null) {
		midletInstaller = new MIDLETInstallerImpl();
		midletInstaller.load(configMap);
	    }
	    return midletInstaller;
	}
    }
	
    
    /**
     * resource bundle for the installer module
     */
    static protected ResourceBundle bundle = null;
    
    /**
     * load this module with the given properties
     * @param map properties of this module
     */
    public void load(Map map) {
	this.configMap = map;
    }
    /**
     * unload the module
     */
    public void unload() {
    }
    
    /**
     * Returns a <code>JUMPInstallerModule</code> for the app model specified
     * @param appModel the application model for which an appropriate
     *        installer module should be returned.
     * @return installer module object
     */
    public JUMPInstallerModule getModule(JUMPAppModel appModel) {
        
        if (appModel == JUMPAppModel.MAIN) {
            return getMainInstaller();
        }
        
        if (appModel == JUMPAppModel.XLET) {
            return getXletInstaller();
        }
        
        if (appModel == JUMPAppModel.MIDLET) {
            return getMidletInstaller();
        }
        
        throw new IllegalArgumentException("Illegal app model for installer.");
    }
    
    /**
     * Returns a <code>JUMPInstallerModule</code> for the mime type
     * specified.
     *
     * @param mimeType mime type for which an appropriate
     *        installer module should be returned.
     *
     * @return This can return null if no installer module is available
     *
     */
    public JUMPInstallerModule getModule(String mimeType) {
        
        // Note yet implemented
        return null;
    }
    
    /**
     * Get all of the available installer modules
     * @return a list of all registered installers in the system for all types
     * of content.
     */
    public JUMPInstallerModule[] getAllInstallers() {
        
        // We support XLET, MAIN, AND MIDLET
        JUMPInstallerModule modules[] = new JUMPInstallerModule[3];
        
        modules[0] = getMainInstaller();
        modules[1] = getXletInstaller();
        modules[2] = getMidletInstaller();
        
        return modules;
    };
    
    /*
     * For localization support
     */
    static ResourceBundle getResourceBundle() {
        if (bundle == null) {
            bundle = ResourceBundle.getBundle(
                    "com.sun.jumpimpl.module.installer.resources.installer");
        }
        
        return bundle;
    }
    
    static String getString(String key) {
        String value = null;
        
        try {
            value = getResourceBundle().getString(key);
        } catch (MissingResourceException e) {
            System.out.println("Could not find key for " + key);
            e.printStackTrace();
        }
        
        return value;
    }
    
    static int getInt(String key) {
        return Integer.parseInt(getString(key));
    }
}
