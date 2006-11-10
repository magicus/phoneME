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

package com.sun.appmanager.impl.client;

import java.io.File;

import sun.mtask.Listener;

import com.sun.appmanager.client.Client;
import com.sun.xlet.mvmixc.MvmIxcRegistryImpl;

public class CDCAmsClient extends Client {

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));

    // 
    // If cdcams.home is already set, leave it.
    // Otherwise, set it to ${java.home}
    //
    private void setupCDCAmsProperties() {

	String cdcamsHome = System.getProperty("cdcams.home");
        if (cdcamsHome == null) {
	    cdcamsHome = System.getProperty("java.home");
	    System.setProperty("cdcams.home", cdcamsHome);
	}

        if (verbose) {
	    System.err.println("cdcams.home = "+
			   System.getProperty("cdcams.home"));
        }

	String repositoryDir = System.getProperty("cdcams.repository");
        if (repositoryDir == null) {
            System.setProperty("cdcams.repository", cdcamsHome + 
                 File.separator + "repository");
        }
    }

    private void setupSecurity() {
	//
	// Before installing the security manager, set the right policy
	//
	
	//
	// Set default policy if one is not already set
	//
	if (System.getProperty("java.security.policy") == null) {
	    String cdcamsHome = System.getProperty("cdcams.home"); 
	    if (cdcamsHome == null) {
		throw new RuntimeException("Need cdcams.home");
	    }
	    String policy = cdcamsHome + File.separator + "lib" + 
                            File.separator + "security" + File.separator + 
                            "appmanager.security.constrained";
	    System.setProperty("java.security.policy", policy);
	}
	
        if (verbose) {
	    System.err.println("SECURITY POLICY = "+
			   System.getProperty("java.security.policy"));
        }
	    
        // Install SecurityManager
	System.setSecurityManager(new SecurityManager());
    }

    public CDCAmsClient() {
	super();
	setupCDCAmsProperties();
	setupSecurity();
	this.clientId = Integer.toString(Listener.getMtaskClientId());
    }

}
