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

package com.sun.midp.main;

import com.sun.j2me.security.*;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.io.j2me.storage.File;

public class CdcAccessControlContext extends AccessControlContextAdapter {

    /** Reference to the current MIDlet suite. */
    private MIDletSuite midletSuite;

    /**
     * Initializes the context for a MIDlet suite.
     *
     * @param suite current MIDlet suite     
     */
    public CdcAccessControlContext(MIDletSuite suite) {
        // Turn on the security manager
        System.setProperty("java.security.policy",
                           File.getConfigRoot(0) + "unidentified.policy");
        System.setSecurityManager(new SecurityManager());
        midletSuite = suite;
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     * <p>
     * If the permission check failed because an InterruptedException was
     * thrown, this method will throw a InterruptedSecurityException.
     *
     * @param permission name of the permission to check for,
     *      must be from JSR spec
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

        java.security.Permission perm = null;
        SecurityManager manager = System.getSecurityManager();

        if (manager == null) {
            // No security.
            System.out.println("*** Running with no security manager ***");
            return;
        }

        if (AccessController.TRUSTED_APP_PERMISSION_NAME.equals(name)) {
            // This is really just a trusted suite check.
            perm = new RuntimePermission("com.sun.j2me.trustedApp");
        } else if ("com.sun.midp.ams".equals(name)) {
            // "lifecycle_management" is from 3GPP MExE spec.
            perm = new RuntimePermission("com.sun.lifecycle_management");
        } else if ("com.sun.midp".equals(name)) {
            // "device core access" is from 3GPP MExE spec.
            perm = new RuntimePermission("com.sun.device_core_access");
        } else {
            // Default to MIDP permission.
            perm = new MidpPermission(name, resource, extraValue);
        }

        manager.checkPermission(perm);
    }
}
