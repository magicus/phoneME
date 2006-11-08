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

package com.sun.appmanager.impl.preferences;

import java.util.Properties;
import java.util.Hashtable;
import java.util.Vector;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.FileOutputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.FilenameFilter;
import java.util.Enumeration;

import com.sun.appmanager.AppManager;
import com.sun.appmanager.preferences.Preferences;
import com.sun.appmanager.store.PersistentStore;

//
// IMPORTANT: NO MULTIPLE APPLICATION CONCURRENT ACCESS ALLOWED!!!
// AppManager access is completely safe.
//
public class CDCAmsPreferences implements Preferences {
    private static boolean debug = false;

    //
    // An implementation of preferences with "save state"
    //
    // An instance of PersistentPreferences shadows an optional
    // default set of preferences. It is constructed with a file
    // backing store. It keeps track of changes, and saves
    // conditionally only of there are changes to the prefs.
    //
    private class PersistentPreferences {
	private Properties defaultProps = null;
	private Properties props = null;
	private boolean changed = false;
	private String filename = null;
	private File f = null;

	PersistentPreferences(String filename) {
	    this(null, filename);
	}

	PersistentPreferences(PersistentPreferences defaultPrefs,
			      String filename) {
	    if (defaultPrefs != null) {
		this.defaultProps = defaultPrefs.getProps();
		this.props = new Properties(this.defaultProps);
	    } else {
		this.props = new Properties();
	    }
	    this.filename = filename;
	    this.changed = false; // No need to save copy if default is there
	    initializeProps();
	}

	void delete() {
	    if (f.exists()) {
		if (CDCAmsPreferences.debug)
		    System.err.println("File "+f+" exists, deleting");
		f.delete();
	    }
	}

	Properties getProps() {
	    return props;
	}

	String[] getNames() {
	    return CDCAmsPreferences.getNames(props.propertyNames()); 
	}

	String get(String key) {
	    return props.getProperty(key);
	}

	void set(String key, String value) {
	    props.setProperty(key, value);
	    changed = true;
	}

	private void initializeProps() {
	    //
	    // If corresponding 'filename' exists, make sure to load it in
	    // but keep the defaults in
	    //
	    f = new File(filename);
	    if (f.exists()) {
		if (CDCAmsPreferences.debug)
		    System.err.println("File "+f+" exists, loading");
		load();
	    }
	}

	void save() {
	    // Nothing to do
	    if (!changed) return;

	    FileOutputStream fos = null;
	    BufferedOutputStream bos = null;

	    if (CDCAmsPreferences.debug)
		System.err.println("STORE PREFS TO \""+filename+"\"");
	    try {
		fos = new FileOutputStream(filename);
		bos = new BufferedOutputStream(fos);
		props.store(bos, "");
		bos.close();
		if (CDCAmsPreferences.debug)
		    System.err.println("PREFS STORED TO \""+filename+"\"");
		this.changed = false;
	    } catch (IOException e) {
		e.printStackTrace();
		try {
		    if (bos != null) {
			bos.close();
			fos = null;
		    }
		    if (fos != null) {
			fos.close();
		    }
		} catch (IOException e2) {
		    e2.printStackTrace();
		}
	    }
	}

	void load() {
	    FileInputStream fis = null;
	    BufferedInputStream bis = null;

	    if (CDCAmsPreferences.debug)
		System.err.println("LOAD_PREFS FROM \""+filename+"\"");
	    try {
		fis = new FileInputStream(filename);
		bis = new BufferedInputStream(fis);
		props.load(bis);
		bis.close();
		if (CDCAmsPreferences.debug) {
		    System.err.println("PREFS LOADED FROM \""+filename+"\"");
		    props.list(System.err);
		}
		changed = false;
	    } catch (IOException e) {
		e.printStackTrace();
		try {
		    if (bis != null) {
			bis.close();
			fis = null;
		    }
		    if (fis != null) {
			fis.close();
		    }
		} catch (IOException e2) {
		    e2.printStackTrace();
		}
	    }
	}
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

    //
    // A single instance of system-wide preferences
    // and the defaults
    //
    private PersistentPreferences systemPreferences;
    private PersistentPreferences defaultSystemPreferences;

    //
    // This is a Hashtable of user preferences. The key is a user identifier.
    // The value is the PersistentPreferences of that user.
    //
    private Hashtable allUserPreferences = new Hashtable(11);

    //
    // Default user preferences
    //
    private PersistentPreferences defaultUserPreferences;

    //
    // Persistent store
    //
    private PersistentStore ps = null;

    private static String DEFAULT_SYSTEM_PREFERENCES_FILE_NAME =
	"preferences/System/defaultSystemPreferences.props";
    private static String SYSTEM_PREFERENCES_FILE_NAME =
	"preferences/System/systemPreferences.props";

    private static String DEFAULT_USER_PREFERENCES_FILE_NAME =
	"preferences/System/defaultUserPreferences.props";

    private static String USER_PREFERENCES_FILE_NAME_PREFIX =
	"preferences/User/";
    private static String USER_PREFERENCES_FILE_NAME =
	".preferences.props";

    private PersistentPreferences getSystemPreferences() {
	return systemPreferences;
    }

    private class UserPrefsFilenameFilter implements FilenameFilter {
        public boolean accept(File dir, String name) {
            String prefix = new String(USER_PREFERENCES_FILE_NAME_PREFIX);
            prefix = prefix.substring(0, prefix.length()-1);
            if ((dir.getAbsolutePath().equals(ps.absolutePathOf(prefix))) &&
                (name.endsWith(USER_PREFERENCES_FILE_NAME)))
                return true;
            else
                return false;
        }
    }

    private void loadAllUserPreferences() {
        File prefDir = new File(ps.absolutePathOf(USER_PREFERENCES_FILE_NAME_PREFIX));
        String [] fileList = prefDir.list(new UserPrefsFilenameFilter());
        if (fileList != null) {
            for (int i=0; i<fileList.length; i++) {
                String key = fileList[i].substring(0, fileList[i].indexOf("."));
                allUserPreferences.put(key, 
                    new PersistentPreferences(defaultUserPreferences, 
                                              prefDir.getAbsolutePath() + 
                                              File.separatorChar + 
                                              fileList[i]));
            }
        }
    }
  
    private PersistentPreferences getUserPreferences(String userName) {
        return getUserPreferences(userName, false);
    }

    private PersistentPreferences getUserPreferences(String userName, 
                                                     boolean create) {
	PersistentPreferences thisUserPrefs =
	    (PersistentPreferences)allUserPreferences.get(userName);

	//
	// If the create flag is set to true, if we have not seen preferences 
        // for this user before, create the user's preferences
	//
   	if ((thisUserPrefs == null) && create) {
	    String userFile =
		USER_PREFERENCES_FILE_NAME_PREFIX +
		userName +
		USER_PREFERENCES_FILE_NAME;

	    thisUserPrefs =
		new PersistentPreferences(defaultUserPreferences,
		    			  ps.absolutePathOf(userFile));
	    allUserPreferences.put(userName, thisUserPrefs);
        }
	return thisUserPrefs;
    }

    public void setUserPreference(String userName,
				  String prefName,
				  String prefValue)
    {
	PersistentPreferences userPrefs = getUserPreferences(userName, true);
	userPrefs.set(prefName, prefValue);
    }

    public String getUserPreference(String userName,
				    String prefName)
    {
	PersistentPreferences userPrefs = getUserPreferences(userName);
        if (userPrefs != null) {
	    return userPrefs.get(prefName);
        }
        else {
            return defaultUserPreferences.get(prefName);
        }
    }

    public void setSystemPreference(String prefName,
				    String prefValue)
    {
	PersistentPreferences systemPrefs = getSystemPreferences();
	systemPrefs.set(prefName, prefValue);
    }

    public String getSystemPreference(String prefName)
    {
	PersistentPreferences systemPrefs = getSystemPreferences();
	return systemPrefs.get(prefName);
    }

    /**
     * Get names of all system preferences
     */
    public String[] getAllSystemPreferenceNames()
    {
	PersistentPreferences systemPrefs = getSystemPreferences();
	return systemPrefs.getNames();
    }

    
    /**
     * Get names of all user preferences corresponding to 'user'
     */
    public String[] getAllUserPreferenceNames(String user) 
    {
	PersistentPreferences userPrefs = getUserPreferences(user);
        if (userPrefs != null) {
   	    return userPrefs.getNames();
        }
        else {
            return null;
        }
    }
	
    /**
     * Get names of all users 
     */
    public String[] getAllUserNames() 
    {
	return CDCAmsPreferences.getNames(allUserPreferences.keys());
    }




    public void saveUserPreferences(String userName) {
	PersistentPreferences p = getUserPreferences(userName);
        if (p != null) {
  	    p.save();
        }
    }

    public void deleteUserPreferences(String userName) {
	PersistentPreferences p = getUserPreferences(userName);
        if (p != null) {
	    p.delete();
	    allUserPreferences.remove(userName);
        }
    }

    public void saveSystemPreferences() {
	systemPreferences.save();
    }

    private void saveAllUserPreferences() {
	for (Enumeration e = allUserPreferences.keys();
	     e.hasMoreElements(); ) {
	    String userName = (String)e.nextElement();
	    saveUserPreferences(userName);
	}
    }

    public void saveAll() {
	saveAllUserPreferences();
	saveSystemPreferences();
    }

    public CDCAmsPreferences() {
	this.ps = AppManager.getPersistentStore();
	String dsys  = ps.absolutePathOf(DEFAULT_SYSTEM_PREFERENCES_FILE_NAME);
	String users = ps.absolutePathOf(DEFAULT_USER_PREFERENCES_FILE_NAME);
	String sys   = ps.absolutePathOf(SYSTEM_PREFERENCES_FILE_NAME);

	defaultSystemPreferences = new PersistentPreferences(dsys);
	defaultUserPreferences = new PersistentPreferences(users);

	//
	// Set up system preferences
	//
	systemPreferences =
	    new PersistentPreferences(defaultSystemPreferences, sys);
        loadAllUserPreferences();
    }

    public static void main(String[] args) {
	Preferences p = new CDCAmsPreferences();
	if (false) {
	p.setSystemPreference("spref2", "New value 3");
	p.setSystemPreference("spref3", "Brand new value 3");
	}
	p.setUserPreference("user1", "upref2", "test2");
	p.setUserPreference("user2", "upref1", "test1");

	p.saveAll();
	if (CDCAmsPreferences.debug) {
	    System.err.println("spref1="+
			       p.getSystemPreference("spref1"));
	    System.err.println("spref2="+
			       p.getSystemPreference("spref2"));
	    System.err.println("spref3="+
			       p.getSystemPreference("spref2"));
	}
    }
}
