/*
 *  
 *
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

package com.sun.midp.content;

import com.sun.midp.installer.InstallState;
import com.sun.midp.installer.Installer;
import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.midlet.MIDletSuite;

/**
 * AppProxy interface to a not-yet installed application bundle.
 * Used by the RegistryImpl to parse and extract content handler
 * registrations.
 */
class AppBundleProxy extends AppProxy {
    /** The installer with access to the archive. */
    private Installer installer;

    /** The InstallState. */
    private InstallState state;

    /** The authority for this bundle. */
    private final String authority;

    /**
     * Construct an AppBundleProxy to draft from a
     * yet to be installed package.
     *
     * @param installer the installer
     * @param msuite the MIDletSuite being constructed by the installer
     * @param state the installer state
     * @param authority for the installer
     * @exception ClassNotFoundException if the <code>classname</code>
     * is not present
     * @exception IllegalArgumentException if classname is not
     *  a valid application
     */
    AppBundleProxy(Installer installer, 
		      InstallState state, 
		      MIDletSuite msuite,
		      String authority)
	throws ClassNotFoundException
    {
	super(msuite, null, null);
	this.installer = installer;
	this.state = state;
	this.authority = authority;
    }


    /**
     * Gets the AppProxy for an application class in the current bundle.
     * @param classname the name of the application class
     * @return the AppProxy for classname; <code>null</code> if not
     * a valid application (MIDlet)
     * @exception ClassNotFoundException if the <code>classname</code>
     *   is not present
     * @exception IllegalArgumentException if classname is 
     *   not a valid application
     */
    AppProxy forClass(String classname) throws ClassNotFoundException
    {

        AppProxy curr = null;
	synchronized (mutex) {
	    // Check if class already has a AppProxy
            curr = (AppBundleProxy)appmap.get(classname);
            if (curr == null) {
		// Create a new instance and check if it is a valid app
		curr = new AppBundleProxy(installer, state,
					     msuite, authority);
		curr.classname = classname;
		curr.appmap = appmap;
		// Throws ClassNotFoundException or IllegalArgumentException
		curr.verifyApplication(classname);
		curr.initAppInfo();
		appmap.put(classname, curr);
		if (LOG_INFO) {
		    logInfo("AppProxy created: " + this);
		}
	    }
	}
	return curr;
    }

    /**
     * Verify that the classname is a valid application.
     * Overridden to just check if the appropriate file is
     * in the jar.
     * @param classname the application class
     *
     * @exception ClassNotFoundException is thrown if the class cannot be found
     * @exception IllegalArgumentException if the classname is null or empty
     */
    protected void verifyApplication(String classname)
	throws ClassNotFoundException
    {
	try {
	    installer.verifyMIDlet(classname);
	} catch (InvalidJadException ije) {
	    if (ije.getReason() == InvalidJadException.INVALID_VALUE) {
		throw new IllegalArgumentException();
	    } else {
		throw new ClassNotFoundException(classname);
	    }
	}
    }

    /**
     * Gets the Trusted authority that authenticated this application.
     * <p>
     * For MIDP, this is the CA of the signer.
     * @return the authority.
     */
    String getAuthority() {
	return authority;
    }
}
