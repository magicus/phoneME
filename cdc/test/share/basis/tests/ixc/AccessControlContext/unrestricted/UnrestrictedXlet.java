/*
 * @(#)UnrestrictedXlet.java	1.3 06/10/10
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.MalformedURLException;
import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.registry.Registry;

import javax.microedition.xlet.Xlet;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;



public class UnrestrictedXlet implements Xlet {
    public void initXlet(XletContext xletContext) {
        System.out.println("UnrestrictedXlet xletcontext = " + xletContext);
        URL[] urls = new URL[1];

        // Get the unrestricted registry
        IxcRegistry unrestrictedRegistry = IxcRegistry.getRegistry(xletContext);

        // Create URLs for RestrictedURLClassLoader.  Note: 
	// RestrictedURLClassLoader has restricted permissions.
        // See the policy file for more information.
	try {
	    File f = new File("classes.restricted");
	    urls[0] = f.toURL();
	} catch (MalformedURLException ex) {
	    ex.printStackTrace();
	    System.out.println("Bug -- aborting.");
	    System.exit(1);
	}

	System.out.println("Restricted xlet will be read from " + urls[0]);

        // Create RestrictedURLClassLoader
	URLClassLoader cl = URLClassLoader.newInstance(urls);

        // Create Restricted XletContext.  This is the XletContext which 
	// has more restrictive permissions.
        RestrictedXletContext xc = new RestrictedXletContext(cl);

        // Find the xlet class, instantiate it, and initialize it
        try {
            Class xletCl = cl.loadClass("RestrictedXlet");
            Object xletObj = xletCl.newInstance();
	    Xlet xlet = (Xlet) xletObj;
	    xlet.initXlet(xc);
        } catch(Exception e) {
            System.out.println("UnrestrictedXlet.initXlet Exception thrown creating xlet.");
            e.printStackTrace(); 
	    System.out.println("Bug -- aborting.");
	    System.exit(1);
        }

        // Get the restricted xlet's registry.  We know it's been bound
	// already, because we called initXlet synchronously.

	Registry remoteRegistry = null;
	try {
	    Remote r = unrestrictedRegistry.lookup("/restricted/Registry");
	    remoteRegistry = (Registry) r;
	} catch (Exception ex) {
	    ex.printStackTrace();
	    System.out.println("Bug -- aborting.");
	    System.exit(1);
	}

	System.out.println("Got the registry:  " + remoteRegistry);

	boolean passed = false;
	try {
	    remoteRegistry.bind("NotAllowed", remoteRegistry);
	} catch (SecurityException ex) {
	    ex.printStackTrace();
	    System.out.println("TEST PASSED!  Got SecurityException.");
	    passed = true;
	} catch (Exception ex) {
	    ex.printStackTrace();
	    System.out.println("Bug -- aborting.");
	    System.exit(1);
	}
	if (!passed) {
	    System.out.println("Test failed.  No SecurityException.");
	}
	System.exit(0);
    }

    public void destroyXlet(boolean unconditional) {
    }

    public void pauseXlet() {
    }

    public void startXlet() {
    }
}
