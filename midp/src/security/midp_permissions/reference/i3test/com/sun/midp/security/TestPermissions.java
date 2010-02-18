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

package com.sun.midp.security;

import com.sun.midp.i3test.*;

/**
 * Unit tests for the Permissions class.
 */
public class TestPermissions extends TestCase {

    /**
     */
    void setUp() {
    }

    /**
     */
    void tearDown() {
    }

    /**
     * Runs all tests.
     */
    public void runTests() throws Throwable {
        setUp();
        declare("testSetPermissionGroupRuntime");
        testSetPermissionGroupRuntime();
        declare("testSetPermissionGroupAppSettings");
        testSetPermissionGroupAppSettings();
        declare("testCheckPushInterruptLevel");
        testCheckPushInterruptLevel();
        tearDown();
    }

    /**
     * Tests the method setPermissionGroup for the correct grouping for
     * the runtime dialog.
     */
    void testSetPermissionGroupRuntime() {
        byte [][] temp =
              Permissions.forDomain(Permissions.UNIDENTIFIED_DOMAIN_BINDING);
        int push = temp[Permissions.CUR_LEVELS][Permissions.PUSH];

        assertFalse("default permission level for HTTPS is ONESHOT",
                    Permissions.ONESHOT !=
                    temp[Permissions.CUR_LEVELS][Permissions.HTTPS]);

        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                       Permissions.HTTP,
                                       Permissions.SESSION);

        assertEquals("changed HTTP permissions, HTTPS permissions must also change",
                     Permissions.SESSION,
                     temp[Permissions.CUR_LEVELS][Permissions.HTTPS]);

        assertEquals("but PUSH permissions must not change",
                     push,
                     temp[Permissions.CUR_LEVELS][Permissions.PUSH]);
    }

    /**
     * Tests the method setPermissionGroup used by app settings to ensure
     * that mutally exclusive permission combinations using the Third Party
     * Indentified domain do not occur.
     */
    void testSetPermissionGroupAppSettings() {
        boolean securityExceptionThrown;
        byte [][] temp =
            Permissions.forDomain(Permissions.IDENTIFIED_DOMAIN_BINDING);

        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS], (byte)0,
                                       Permissions.NET_ACCESS_GROUP,
                                       Permissions.BLANKET_GRANTED);
        try {
            securityExceptionThrown = false;
            Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                           (byte)0,
                                           Permissions.MULTIMEDIA_GROUP,
                                           Permissions.BLANKET_GRANTED);
        } catch (SecurityException se) {
            securityExceptionThrown = true;
        }

        assertTrue(securityExceptionThrown);

        try {
            securityExceptionThrown = false;
            Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                           (byte)0,
                                           Permissions.READ_USER_DATA_GROUP,
                                           Permissions.BLANKET_GRANTED);
        } catch (SecurityException se) {
            securityExceptionThrown = true;
        }

        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS], (byte)0,
                                       Permissions.NET_ACCESS_GROUP,
                                       Permissions.SESSION);
        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                       (byte)0,
                                       Permissions.MULTIMEDIA_GROUP,
                                       Permissions.BLANKET_GRANTED);

        try {
            Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                           (byte)0,
                                           Permissions.NET_ACCESS_GROUP,
                                           Permissions.BLANKET_GRANTED);
        } catch (SecurityException se) {
            securityExceptionThrown = true;
        }

        assertTrue(securityExceptionThrown);

        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                       (byte)0,
                                       Permissions.MULTIMEDIA_GROUP,
                                       Permissions.SESSION);
        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                       (byte)0,
                                       Permissions.READ_USER_DATA_GROUP,
                                       Permissions.BLANKET_GRANTED);
        try {
            Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                           (byte)0,
                                           Permissions.NET_ACCESS_GROUP,
                                           Permissions.BLANKET_GRANTED);
        } catch (SecurityException se) {
            securityExceptionThrown = true;
        }

        assertTrue(securityExceptionThrown);

        try {
            securityExceptionThrown = false;
            Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                           Permissions.BLANKET_GRANTED,
                                           Permissions.NET_ACCESS_GROUP,
                                           Permissions.BLANKET_GRANTED);
        } catch (SecurityException se) {
            securityExceptionThrown = true;
        }

        assertTrue(securityExceptionThrown);
    }

    /** Test the checkPushInterruptLevel method. */
    void testCheckPushInterruptLevel() {
        boolean securityExceptionThrown;
        byte [][] temp =
            Permissions.forDomain(Permissions.IDENTIFIED_DOMAIN_BINDING);

        Permissions.setPermissionGroup(temp[Permissions.CUR_LEVELS],
                                       (byte)0,
                                       Permissions.NET_ACCESS_GROUP,
                                       Permissions.BLANKET_GRANTED);

        try {
            securityExceptionThrown = false;
            Permissions.checkPushInterruptLevel(temp[Permissions.CUR_LEVELS],
                                    Permissions.BLANKET_GRANTED);
        } catch (SecurityException se) {
            securityExceptionThrown = true;
        }

        assertTrue(securityExceptionThrown);
    }
}
