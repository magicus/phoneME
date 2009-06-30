/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.jump.midletsuite;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteCorruptedException;
import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletSuiteInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;

import com.sun.midp.jump.MIDletApplication;
import com.sun.midp.jump.installer.StorageAccessInterface;
import com.sun.jump.common.JUMPContent;

public class MIDletSuiteStorageAccessor 
    implements StorageAccessInterface {

    private MIDletSuiteStorage storage; 
    
    public MIDletSuiteStorageAccessor() {
	this.storage = MIDletSuiteStorage.getMIDletSuiteStorage();
    }
    
    public int[] getInstalledMIDletSuiteIds() {  
	return storage.getListOfSuites();
    }
    
    public JUMPContent[] convertToMIDletApplications(int suiteId) {
	try { 
	    MIDletSuiteInfo suiteInfo = getMIDletSuiteInfo(suiteId);
	    MIDletInfo[] midletInfos  = getMIDletInfos(suiteInfo);
	    
	    JUMPMIDletSuiteInfo currentMIDletSuiteInfo = 
		new JUMPMIDletSuiteInfo(suiteInfo, midletInfos);
	    
	    return currentMIDletSuiteInfo.getMIDletApplications();
	    
	} catch (IOException e) { 
	    System.err.println(e + " thrown while accessing the midlet suite "
			       + suiteId);
	    return new JUMPContent[0];
	} catch (MIDletSuiteLockedException e) { 
	    System.err.println(e + " thrown while accessing the midlet suite "
			       + suiteId);
	    return new JUMPContent[0];
	} catch (MIDletSuiteCorruptedException e) { 
	    System.err.println(e + " thrown while accessing the midlet suite "
			       + suiteId);
	    return new JUMPContent[0];
	}
    }	    
    
    private MIDletSuiteInfo getMIDletSuiteInfo(int id) throws IOException {
	return storage.getMIDletSuiteInfo(id);
    }
    
    private MIDletInfo[] getMIDletInfos(MIDletSuiteInfo suiteInfo) 
	throws MIDletSuiteLockedException, MIDletSuiteCorruptedException {
	MIDletSuiteImpl midletSuite = storage.getMIDletSuite(suiteInfo.suiteId,
							     false);
	
	MIDletInfo[] midletInfos =
	    new MIDletInfo[midletSuite.getNumberOfMIDlets()];
	for (int i = 0; i < midletInfos.length; i++) {
	    midletInfos[i] =
		new MIDletInfo(
			       midletSuite.getProperty("MIDlet-" + (i+1)));
	}
	
	// Need to unlock midletsuite.
	midletSuite.close();
	
	return midletInfos;
    }
    
    public void remove(int id) { 
	try {	    
	    storage.remove(id);
	} catch (MIDletSuiteLockedException e) { 	   
	    new RuntimeException(e);
	}	   
    }
    
    class JUMPMIDletSuiteInfo {
	
        MIDletSuiteInfo suiteInfo; 
        ArrayList midletApplications;
	
        public JUMPMIDletSuiteInfo(MIDletSuiteInfo suiteInfo,
				   MIDletInfo[] midletInfos) {
	    this.suiteInfo = suiteInfo;
	    
	    midletApplications = new ArrayList(midletInfos.length);
	    for (int i = 0; i < midletInfos.length; i++) {              
		String jarfile[] =
		    storage.getMidletSuiteClassPath(suiteInfo.suiteId);
		URL iconURL = null;
		if (jarfile[0] != null & midletInfos[i].icon != null) {
		    String iconURLfile = null;
		    if (midletInfos[i].icon.startsWith("/")) {
			iconURLfile = "jar:file://"
			    + jarfile[0] + "!/"
			    + midletInfos[i].icon.substring(1);
		    } else {
			iconURLfile = "jar:file://"
			    + jarfile[0] + "!/" + midletInfos[i].icon;
		    }
		    try {
			iconURL = new URL(iconURLfile);
		    } catch (MalformedURLException ex) {
			ex.printStackTrace();
		    }
		}
		MIDletApplication app = 
		    new MIDletApplication(midletInfos[i].name,
					  iconURL, suiteInfo.suiteId,
					  midletInfos[i].classname, (i+1)); 
		midletApplications.add(i, app);
	    }
	    
        }

        public MIDletSuiteInfo getMIDletSuiteInfo() {
            return suiteInfo;
        }
	
        public MIDletApplication[] getMIDletApplications() {
            return (MIDletApplication[]) 
		midletApplications.toArray(new MIDletApplication[] {});
        }
	
        public boolean contains(MIDletApplication midletApp) {
            return midletApplications.contains(midletApp);
        }
    }
}
