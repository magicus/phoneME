/*
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
 * FileConnection access permissions.
 */
public class FileConnectionPermission extends Permission {
    static String ACCESS_READ  = "read";
    static String ACCESS_WRITE = "write";

    static String FILE_CONNECTION = "javax.microedition.io.Connector.file.";

    static public FileConnectionPermission READ =
        new FileConnectionPermission(FILE_CONNECTION + ACCESS_READ, null,
            Permissions.FILE_CONNECTION_READ);
    static public FileConnectionPermission WRITE =
        new FileConnectionPermission(FILE_CONNECTION + ACCESS_WRITE, null,
            Permissions.FILE_CONNECTION_WRITE);

    public FileConnectionPermission(String name, String resource) {
        super(name, resource);
    }

    public FileConnectionPermission(String name, String resource, int midpPerm) {
        super(name, resource, midpPerm);
    }
}
