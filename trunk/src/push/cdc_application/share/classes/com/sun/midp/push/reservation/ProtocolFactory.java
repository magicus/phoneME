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

package com.sun.midp.push.reservation;

import com.sun.j2me.security.AccessControlContext;

/**
 * Abstraction for protocol-specific
 * <code>ReservationDescriptor</code> creation.
 */
public interface ProtocolFactory {
    /**
     * Creates a <code>ReservationDescriptor</code>.
     *
     * @param protocol connection protocol (all lowercase)
     * @param targetAndParams reminder of connection string
     * @param filter filter
     * @param context object to check protocol-specific
     *  permissions (cannot be <code>null</code>)
     *
     * @return <code>ReservationDescriptor</code> (cannot be <code>null</code>)
     *
     * @throws IllegalArgumentException if <code>targetAndParams</code> or
     *  <code>filter</code> strings are invalid
     * @throws SecurityException if protocol specific checks fail
     */
    ReservationDescriptor createDescriptor(
            String protocol, String targetAndParams, String filter,
            AccessControlContext context)
        throws IllegalArgumentException, SecurityException;
}
