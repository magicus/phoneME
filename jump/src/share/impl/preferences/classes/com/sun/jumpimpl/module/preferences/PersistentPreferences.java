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

import java.util.Vector;
import java.util.Properties;
import java.util.Enumeration;
import java.io.File;

import com.sun.jump.module.contentstore.JUMPData;
import com.sun.jump.module.contentstore.JUMPNode;
import com.sun.jump.module.contentstore.JUMPStoreHandle;

/**
 * An implementation of preferences with "save state"
 *
 * An instance of PersistentPreferences shadows an optional
 * default set of preferences. It keeps track of changes, 
 * and saves conditionally only of there are changes to the prefs.
 */

class PersistentPreferences {

    private Properties defaultProps = null;
    private Properties props = null;
    private boolean changed = false;
    private String filename = null;
    private File f = null;

    private static boolean debug = false;

    PersistentPreferences(String filename, JUMPStoreHandle storeHandle) {
        this(null, filename, storeHandle);
    }
    
    PersistentPreferences(PersistentPreferences defaultPrefs,
                          String filename, JUMPStoreHandle storeHandle) {       
        if (defaultPrefs != null) {
            this.defaultProps = defaultPrefs.getProps();
            this.props = new Properties(this.defaultProps);
        } else {
            this.props = new Properties();
        }
        this.filename = filename;
        this.changed = false; // No need to save copy if default is there
        initializeProps(storeHandle);
    }

    private void initializeProps(JUMPStoreHandle storeHandle) {
        // If corresponding 'filename' exists, make sure to load it in
        // but keep the defaults in
        try {
            if (storeHandle.getNode(filename) != null) {                
                if (debug) System.err.println("File "+f+" exists, loading");
                load(storeHandle);
            }
            else {
                storeHandle.createDataNode(filename, 
                                           new JUMPData(new Properties()));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    void delete(String key) {
        props.remove(key);
        changed = true;
    }

    Properties getProps() {
        return props;
    }
    
    String[] getNames() {
        return PersistentPreferences.getNames(props.propertyNames()); 
    }
    
    private static String[] getNames(Enumeration e) {
	Vector v = new Vector();
	int i = 0;
	while (e.hasMoreElements()) {
	    String key = (String)e.nextElement();
	    v.addElement(key);
	    i++;
	}
	String[] s = new String[i];
	v.copyInto(s);
	return s;
    }

    String get(String key) {
        return props.getProperty(key);
    }
    
    void set(String key, String value) {
        props.setProperty(key, value);
        changed = true;
    }    

    void save(JUMPStoreHandle storeHandle) {
        // Nothing to do
        if (!changed) return;
        
        JUMPData propertiesData = new JUMPData(props);        
        
        if (debug)System.err.println("Storing Prefs to \""+filename+"\"");
        
        try {
            storeHandle.updateDataNode(filename, propertiesData);  
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    void load(JUMPStoreHandle storeHandle) {
        
        JUMPNode prefsNode = null;
        JUMPData prefsData = null;
        
        try {
            prefsNode = storeHandle.getNode(filename);
            if (prefsNode != null && prefsNode.containsData()) {
                prefsData = ((JUMPNode.Data)prefsNode).getData();
                props = (Properties)prefsData.getValue();
                if (debug) {
                    System.err.println("PREFS LOADED FROM \""+filename+"\"");
                    props.list(System.err);
                }                
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        
    }
    
}
