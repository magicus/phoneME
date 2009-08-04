/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui;

import com.sun.j2me.security.AccessController;
import com.sun.midp.security.Permissions;


/**
 * This is the "tunnel" to deliver display orientation configuration
 * to Display class.
 */
public class DisplayOrientationAccess {
    private static Access access;

    public static void setAccess(Access access) {
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);
        DisplayOrientationAccess.access = access;
    }

    public static void setPrimaryDisplayLandscape(
            Object midlet, boolean landscape) {
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);
        access.setPrimaryDisplayLandscape(midlet, landscape);
    }

    public static boolean isPrimaryDisplayLandscape(Object midlet) {
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);
        return access.isPrimaryDisplayLandscape(midlet);
    }

    public static interface Access {
        public void setPrimaryDisplayLandscape(
                Object midlet, boolean landscape);
        public boolean isPrimaryDisplayLandscape(Object midlet);
    }
}
