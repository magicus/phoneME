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

package com.sun.midp.jump;

import java.lang.reflect.Field; 
import java.security.AccessController;
import java.security.PrivilegedAction;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.jump.installer.TrustedMIDletSuiteInfo;

/**
 * Initialize the JUMP executive environment for using midp code.
 */
public class JumpInit {

    public static void init() {

	// First, load the midp natives.   
        try {
            String n = System.getProperty("sun.midp.library.name", "midp");
            // DEBUG: System.err.println("Loading DLL \"" + n + "\" ...");
            System.loadLibrary(n);
        } catch (UnsatisfiedLinkError err) {}

	/** 
         * Path to MIDP working directory. 
         * Default is the property "sun.midp.home.path", the fallback is user.dir.
        */
        String userdir = System.getProperty("user.dir", ".");
        String home = System.getProperty("sun.midp.home.path", userdir);
        String profile = System.getProperty("microedition.profiles");
	if (profile == null) 
            System.setProperty("microedition.profiles", "MIDP-2.1");

        if (!initMidpNativeStates(home)) {
           throw new RuntimeException("MIDP native initialization failed");
        }

	/** 
	 * Making the runtime believe that this is a trusted midlet suite.
	 * This is needed because in midp's code, security check is often done
	 * by checking the executing midlet suite's security token.
	 * See com.sun.midp.jump.installer.TrustedMIDletSuiteInfo for more
	 * information.
	 **/
	
        final MIDletStateHandler handler = MIDletStateHandler.getMidletStateHandler();
        MIDletSuite suite = handler.getMIDletSuite();
        if (suite == null) {
           AccessController.doPrivileged(new PrivilegedAction() {
              public Object run() {
      
                try {
                    Class clazz = MIDletStateHandler.class;
                    Field midletSuiteField = clazz.getDeclaredField("midletSuite");

                    midletSuiteField.setAccessible(true);
      
                    midletSuiteField.set(handler, new TrustedMIDletSuiteInfo());

                } catch (Exception e) { e.printStackTrace(); }
                    return null;
               }
            });
        }
     }
  
    /**
     * Performs native subsystem initialization.
     * @param home path to the MIDP working directory.
     */
    static native boolean initMidpNativeStates(String home);
}
