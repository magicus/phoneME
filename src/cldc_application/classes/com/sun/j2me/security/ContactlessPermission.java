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

package com.sun.j2me.security;

import com.sun.midp.security.Permissions;

/**
 * ContactlessPermission access permissions.
 */
public class ContactlessPermission extends Permission {
    
    static public ContactlessPermission DISCOVERY_MANAGER =
        new ContactlessPermission(Permissions.getName(Permissions.CONTACTLESS_DISCOVERY), null);
    static public ContactlessPermission NDEF_TAG_CONNECTION_WRITE =
        new ContactlessPermission(Permissions.getName(Permissions.CONTACTLESS_NDEF_WRITE), null);
    static public ContactlessPermission CONNECTOR_NDEF =
        new ContactlessPermission(Permissions.getName(Permissions.CONTACTLESS_NDEF), null);
    static public ContactlessPermission CONNECTOR_RF =
        new ContactlessPermission(Permissions.getName(Permissions.CONTACTLESS_RF), null);
    static public ContactlessPermission CONNECTOR_SC =
        new ContactlessPermission(Permissions.getName(Permissions.CONTACTLESS_SC), null);
    static public ContactlessPermission CONNECTOR_VTAG =
        new ContactlessPermission(Permissions.getName(Permissions.CONTACTLESS_VTAG), null);

    public ContactlessPermission(String name, String resource) {
        super(name, resource);
    }    
}
