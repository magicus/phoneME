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

package com.sun.j2me.security;

/**
 * Satsa access permissions.
 */
public class SatsaPermission extends Permission {
    
    static String APDU_OPEN       = "apdu:open";
    static String APDU_SAT_OPEN   = "apdu:satopen";

    static String APDU_CHANNEL0   = "javax.microedition.apdu.channel0#";
    static String APDU_CONNECTION = "javax.microedition.apdu.connection#";

    static public SatsaPermission APDU_CHANNEL0_SAT_OPEN =
        new SatsaPermission(APDU_CHANNEL0 + APDU_SAT_OPEN, null);
    
    static public SatsaPermission APDU_CONNECTION_OPEN =
        new SatsaPermission(APDU_CONNECTION + APDU_OPEN, null);

    static public SatsaPermission JCRMI_CONNECTION = 
        new SatsaPermission("javax.microedition.jcrmi.connection", null);

    static public SatsaPermission SIGN_SERVICE =
        new SatsaPermission("javax.microedition.pki.signservice", null);
    
    public SatsaPermission(String permission, String resource) {
        super(permission, resource);
    }
}
