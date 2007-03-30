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

package com.sun.midp.jump.installer;

import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;

import com.sun.midp.installer.InternalMIDletSuiteImpl;

/**
 * A proxy for InternalMIDletSuiteImpl to bypass the security check.
 * Used by the JUMP exective to bypass NullPointerException 
 * thrown at various ams secuirty check code through MIDletStateHandler like 
 * listed below.  See JumpInit.init() for the code that 
 * sets MIDletStateHandler.java's MIDletSuite field value.
 *
 *    MIDletStateHandler midletStateHandler =
 *          MIDletStateHandler.getMidletStateHandler();
 *    MIDletSuite midletSuite = midletStateHandler.getMIDletSuite();
 *    midletSuite.checkIfPermissionAllowed(Permissions.AMS);
 *
 * This is a temporary fix until the security architecture in ams s replaced 
 * with more CDC centric AccessController syntax.
**/
public class TrustedMIDletSuiteInfo implements MIDletSuite {
  
    MIDletSuite suite;

    public TrustedMIDletSuiteInfo() {
       suite = InternalMIDletSuiteImpl.create("executive", 0);
    }
  
    public void checkIfPermissionAllowed(int permission) {
       return;  // trusted, don't throw SecurityException.
    } 

    /**
     * Get a property of the suite. A property is an attribute from
     * either the application descriptor or JAR Manifest.
     *
     * @param key the name of the property
     * @return A string with the value of the property.
     *    <code>null</code> is returned if no value is available for
     *          the key.
     */
    public String getProperty(String key) {
       return suite.getProperty(key);
    }

    /**
     * Gets push setting for interrupting other MIDlets.
     * Reuses the Permissions.
     *
     * @return push setting for interrupting MIDlets the value
     *        will be permission level from {@link Permissions}
     */
    public byte getPushInterruptSetting() {
       return suite.getPushInterruptSetting();
    }

    /**
     * Gets push options for this suite.
     *
     * @return push options are defined in {@link PushRegistryImpl}
     */
    public int getPushOptions() {
        return suite.getPushOptions();
    }

    /**
     * Gets list of permissions for this suite.
     *
     * @return array of permissions from {@link Permissions}
     */
    public byte[] getPermissions() {
        return suite.getPermissions();
    }

    /**
     * Replace or add a property to the suite for this run only.
     *
     * @param token token with the AMS permission set to allowed,
     *        can be null to use the suite's permission
     * @param key the name of the property
     * @param value the value of the property
     *
     * @exception SecurityException if the caller's token does not have
     *            internal AMS permission
     */
    public void setTempProperty(SecurityToken token, String key, String value) {
        suite.setTempProperty(token, key, value);
    }

    /**
     * Get the name of a MIDlet to display to the user.
     *
     * @param className classname of a MIDlet in the suite
     *
     * @return name to display to the user
     */
    public String getMIDletName(String className) {
        return suite.getMIDletName(className);
    }

    /**
     * Check to see the suite has the ALLOW level for specific permission.
     * This is used for by internal APIs that only provide access to
     * trusted system applications.
     * <p>
     * Only trust this method if the object has been obtained from the
     * Scheduler of the suite.
     *
     * @param permission permission ID from
     *      {@link com.sun.midp.security.Permissions}
     *
     * @exception SecurityException if the suite is not
     *            allowed to perform the specified action.
     */
    //public void checkIfPermissionAllowed(int permission);

    /**
     * Check for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkForPermission(int permission, String resource)
        throws InterruptedException {
        suite.checkForPermission(permission, resource);
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     * @param extraValue string to insert into the question,
     *        can be null if no %3 in the question
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkForPermission(int permission, String resource,
        String extraValue) throws InterruptedException {
        suite.checkForPermission(permission, resource, extraValue);
    }

    /**
     * Get the status of the specified permission.
     * If no API on the device defines the specific permission
     * requested then it must be reported as denied.
     * If the status of the permission is not known because it might
     * require a user interaction then it should be reported as unknown.
     *
     * @param permission to check if denied, allowed, or unknown.
     * @return 0 if the permission is denied; 1 if the permission is allowed;
     *  -1 if the status is unknown
     */
    public int checkPermission(String permission) {
        return suite.checkPermission(permission);
    }

    /**
     * Gets the unique ID of the suite.
     *
     * @return suite ID
     */
    public int getID() {
        return suite.getID();
    }

    /**
     * Ask the user want to interrupt the current MIDlet with
     * a new MIDlet that has received network data.
     *
     * @param connection connection to place in the permission question or
     *        null for alarm
     *
     * @return true if the use wants interrupt the current MIDlet, else false
     */
    public boolean permissionToInterrupt(String connection) {
        return suite.permissionToInterrupt(connection);
    }

    /**
     * Indicates if the named MIDlet is registered in the suite
     * with MIDlet-&lt;n&gt; record in the manifest or
     * application descriptor.
     * @param midletClassName class name of the MIDlet to be checked
     *
     * @return true if the MIDlet is registered
     */
    public boolean isRegistered(String midletClassName) {
        return suite.isRegistered(midletClassName);
    }

    /**
     * Indicates if this suite is trusted.
     * (not to be confused with a domain named "trusted",
     * this is used for extra checks beyond permission checking)
     *
     * @return true if the suite is trusted false if not
     */
    public boolean isTrusted() {
        return suite.isTrusted();
    }


    /**
     * Check whether the suite classes are preverified and
     * the suite content hasn't been changed since installation
     *
     * @return true if no more verification needed, false otherwise
     */
    public boolean isVerified() {
        return suite.isVerified();
    }

    /**
     * Determine if the a MIDlet from this suite can be run. Note that
     * disable suites can still have their settings changed and their
     * install info displayed.
     *
     * @return true if suite is enabled, false otherwise
     */
    public boolean isEnabled() {
        return suite.isEnabled();
    }

    /**
     * Close the opened MIDletSuite
     */
    public void close() {
        suite.close();
    }
}
