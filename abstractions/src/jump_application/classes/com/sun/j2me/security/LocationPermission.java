/*
 *
 *
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

package com.sun.j2me.security;

public class LocationPermission extends Permission {

    static String ACCESS_READ      = "read";
    static String ACCESS_WRITE     = "write";
    static String ACCESS_CATEGORY  = "category";
    static String ACCESS_MANAGE    = "manage";
    static String ACCESS_ANY       = "any";
    static String ACCESS_PROXIMITY = "proximity";

    static String LANDMARK_STORE   = "javax.microedition.location.LandmarkStore";
    static String LOCATION_NAME    = "javax.microedition.location";
    static String ORIENTATION_NAME = "javax.microedition.location.Orientation";
    
    static public LocationPermission LANDMARK_STORE_READ = 
            new LocationPermission(LANDMARK_STORE, ACCESS_READ);

    static public LocationPermission LANDMARK_STORE_WRITE = 
            new LocationPermission(LANDMARK_STORE, ACCESS_WRITE);

    static public LocationPermission LANDMARK_STORE_CATEGORY = 
            new LocationPermission(LANDMARK_STORE, ACCESS_CATEGORY);

    static public LocationPermission LANDMARK_STORE_MANAGE = 
            new LocationPermission(LANDMARK_STORE, ACCESS_MANAGE);

    static public LocationPermission LOCATION = 
            new LocationPermission(LOCATION_NAME, ACCESS_ANY);

    static public LocationPermission LOCATION_PROXIMITY = 
            new LocationPermission(LOCATION_NAME, ACCESS_PROXIMITY);

    static public LocationPermission ORIENTATION = 
            new LocationPermission(ORIENTATION_NAME, ACCESS_ANY);

    public LocationPermission(String name, String resource) {
        super(name, resource);
    }
    
}
