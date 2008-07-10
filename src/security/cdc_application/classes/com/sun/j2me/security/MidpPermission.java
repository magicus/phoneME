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

import java.security.*;

/** Provides permission handling for MIDP permission. */
public class MidpPermission extends BasicPermission {
    /** Level of the permission. */
    String level;

    /** Resource to be accessed. */
    String resource;

    /** Extra data to show the user. */
    String extraData;

    /**
     * Initialize a MIDP permission from a policy file.
     *
     * @param name name of the permission to check for,
     *      must match the name from JSR spec
     * @param theLevel level of the permission
     */
    public MidpPermission(String name, String theLevel) {
        super(name);
        level = theLevel;
    }

    /**
     * Initialize a MIDP permission when checking a permission.
     *
     * @param name name of the permission to check for,
     *      must match the name from JSR spec
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     * @param extraValue string to insert into the question,
     *        can be null if no %3 in the question
     */
    public MidpPermission(String name, String theResource,
                          String theExtraData) {
        super(name);
        resource = theResource;
        extraData = theExtraData;
    }
}
