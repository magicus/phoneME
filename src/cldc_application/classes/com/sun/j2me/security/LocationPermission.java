/*
 *
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

package com.sun.j2me.security;

import com.sun.midp.security.Permissions;

/**
 * Location and landmark store access permissions.
 */
public class LocationPermission extends Permission {

    static public LocationPermission LOCATION = new LocationPermission(
        Permissions.getName(Permissions.LOCATION), null);

    static public LocationPermission ORIENTATION = new LocationPermission(
        Permissions.getName(Permissions.ORIENTATION), null);

    static public LocationPermission LOCATION_PROXIMITY =
        new LocationPermission(Permissions.getName(
            Permissions.LOCATION_PROXIMITY), null);

    static public LocationPermission LANDMARK_STORE_READ =
        new LocationPermission(Permissions.getName(Permissions.LANDMARK_READ),
        null);

    static public LocationPermission LANDMARK_STORE_WRITE = 
        new LocationPermission(Permissions.getName(Permissions.LANDMARK_WRITE),
        null);

    static public LocationPermission LANDMARK_STORE_CATEGORY = 
        new LocationPermission(Permissions.getName(
            Permissions.LANDMARK_CATEGORY), null);

    static public LocationPermission LANDMARK_STORE_MANAGE = 
        new LocationPermission(Permissions.getName(Permissions.LANDMARK_MANAGE),
        null);

    public LocationPermission(String name, String resource) {
        super(name, resource);
    }
    
}
