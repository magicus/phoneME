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

import java.io.File;
import java.lang.reflect.Method;
import java.util.Vector;
import java.util.HashMap;

import sun.misc.MIDPConfig;

/**
 * Factory implementation methods for the installer module
 */
public class InstallerFactoryImpl extends JUMPInstallerModuleFactory {

    private JUMPInstallerModule mainInstaller;
    private JUMPInstallerModule xletInstaller;
    private JUMPInstallerModule midletInstaller;
    private Map configMap = new HashMap();
    
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
	
    static boolean midletInstallerInitialized = false;
    private JUMPInstallerModule getMidletInstaller() 
    {
	synchronized(InstallerFactoryImpl.class) {

	    if (midletInstaller == null && !midletInstallerInitialized) {

              /** 
              * The MIDLET installer is part of the MIDP library, which
              * is only accessible through the MIDPImplementationClassLoader.
              * Hence the installer needs to be loaded by the midp implementation
              * classloader via reflection. Otherwise, the full midp-cdc-jump
              * rommization fails as well as runtime requiring midp classes to
              * be in the same path as jump classes. 
              **/   

               try {

                  ClassLoader midpImplementationClassLoader = 
   		      MIDPConfig.getMIDPImplementationClassLoader();
   
   	          if (midpImplementationClassLoader == null) {
   
	             /**		   
	              *	Noone had a need to load midp classes yet.
		      *	Create the classloader using the
		      *	default location in which sun.misc.MIDPLauncher
		      *	normally uses, unless jump.midp.classes.zip is set.
		      **/
 
                     //String midpFile = (String) configMap.get("jump.midp.classes.zip");

                     midpImplementationClassLoader =
   	                MIDPConfig.newMIDPImplementationClassLoader(null);
   	          }   
                  
   	          midletInstaller = (JUMPInstallerModule) 
   	             loadAndInstantiate(midpImplementationClassLoader,
   			          "com.sun.midp.jump.installer.MIDLETInstallerImpl"); 	
                  if (midletInstaller != null) {
		      midletInstaller.load(configMap);
                  }

               } catch (ClassNotFoundException e) {
                  // MIDPConfig or MIDLETInstallerImpl not found. 
		  // Not a dual stack platform.
                  e.printStackTrace();
               } catch (Exception e) { 
                  // Other unexpected error? Let's report it.
                  e.printStackTrace();
               }
               midletInstallerInitialized = true;
	    }
	    
	    return midletInstaller;
	}
    }


    private static Object loadAndInstantiate(ClassLoader loader,
                       String className) throws ClassNotFoundException {

	try {     
           Class clazz = Class.forName(className, true, loader);
           return clazz.newInstance();
        } catch (InstantiationException e) { 
	   e.printStackTrace(); 
        } catch (IllegalAccessException e) { 
 	   e.printStackTrace(); 
	}

	return null;
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
            JUMPInstallerModule module = getMidletInstaller();
            if (module == null)
               throw new IllegalArgumentException("Can't find MIDLET installer");

            return module;
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
        Vector vector = new Vector();
        JUMPInstallerModule mainModule = getMainInstaller();
        if (mainModule != null) {
            vector.add(mainModule);
        }
        JUMPInstallerModule xletModule = getXletInstaller();
        if (xletModule != null) {
            vector.add(xletModule);
        }
        JUMPInstallerModule midletModule = getMidletInstaller();
        if (midletModule != null) {
            vector.add(midletModule);
        }
        return (JUMPInstallerModule[]) vector.toArray(new JUMPInstallerModule[]{});
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
