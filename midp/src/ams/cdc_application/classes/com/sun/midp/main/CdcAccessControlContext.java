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

import com.sun.j2me.security.AccessControlContextAdapter;
import com.sun.j2me.security.AccessController;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.security.Permissions;

public class CdcAccessControlContext extends AccessControlContextAdapter {

    /** Reference to the current MIDlet suite. */
    private MIDletSuite midletSuite;

    /**
     * Initializes the context for a MIDlet suite.
     *
     * @param suite current MIDlet suite     
     */
    public CdcAccessControlContext(MIDletSuite suite) {
        midletSuite = suite;
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     * <p>
     * If the permission check failed because an InterruptedException was
     * thrown, this method will throw a InterruptedSecurityException.
     *
     * @param name name of the requested permission
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     * @param extraValue string to insert into the question,
     *        can be null if no %3 in the question
     * 
     * @exception SecurityException if the specified permission
     * is not permitted, based on the current security policy
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkPermissionImpl(String name, String resource,
            String extraValue) throws SecurityException, InterruptedException {
        int permissionId;

        if (AccessController.TRUSTED_APP_PERMISSION_NAME.equals(name)) {
            // This is really just a trusted suite check.
            if (midletSuite.isTrusted()) {
                return;
            }

            throw new SecurityException("suite not trusted");
        }

        if ("com.sun.midp.ams".equals(name) || "com.sun.midp".equals(name)) {
            // These permission checks cannot block
            midletSuite.checkIfPermissionAllowed(name);
        } else {
            midletSuite.checkForPermission(name, resource, extraValue);
        }
    }
}
