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

package com.sun.midp.jump;

import com.sun.j2me.security.AccessControlContextAdapter;
import com.sun.j2me.security.AccessController;

/**
 * Initialize the JUMP executive environment for using midp code.
 */
public class JumpInit {
    /*
     * Initialization for the jump executive.
     *
     * Sets SuiteStorage, and performs a routine to 
     * register this caller as a trusted midlet.
     *
     * @param home path to the MIDP home directory.
     */
    public static void init(String midpHome) {
	// First, load the midp natives.   
        try {
            String n = System.getProperty("sun.midp.library.name", "midp");
            System.loadLibrary(n);
        } catch (UnsatisfiedLinkError err) {}


        AccessController.setAccessControlContext(
            new AccessControlContextImpl());

        if (!initMidpStorage(midpHome)) {
           throw new RuntimeException("MIDP suite store initialization failed");
        }
    }
  
    /**
     * Performs native midp suitestorage initialization.
     * This method performs only a subroutine of 
     * CDCInit.initMidpNativeState(midpHome), by skipping lcdui initialization.
     * 
     * @param home path to the MIDP working directory.
     */
    static native boolean initMidpStorage(String midpHome);
}

class AccessControlContextImpl extends AccessControlContextAdapter {
    /** Only this package can create an object of this class. */
    AccessControlContextImpl() {
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     * <p>
     * If the permission check failed because an InterruptedException was
     * thrown, this method will throw a InterruptedSecurityException.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     * @param extraValue string to insert into the question,
     *        can be null if no %3 in the question
     *
     * @param name name of the requested permission
     * 
     * @exception SecurityException if the specified permission
     * is not permitted, based on the current security policy
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkPermissionImpl(String name, String resource,
            String extraValue) throws SecurityException, InterruptedException {
        // This is the jump executive every thing running is trusted
        return;
    }
}

