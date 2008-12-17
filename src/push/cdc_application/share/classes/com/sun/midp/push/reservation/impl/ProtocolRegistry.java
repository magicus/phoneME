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

package com.sun.midp.push.reservation.impl;

import com.sun.midp.push.reservation.ProtocolFactory;


/**
 * Registry mapping protocol to instances of <code>ProtocolRegistry</code>.
 * <p>
 * Implementations of this interface should be just simple maps and
 * shouldn't be aware of protocol semantics: all the checks must
 * be performed higher in the call chain. 
 * </p>
 */
public interface ProtocolRegistry {
    /**
     * Fetches an instance of <code>ProtocolRegistry</code>.
     *
     * @param protocol protocol to fetch <code>ProtocolRegistry</code> for
     * (cannot be <code>null</code>); MUST be lower case
     *
     * @return an instance of <code>ProtocolRegistry</code> or <code>null</code>
     *  if there is no mapping 
     */
    ProtocolFactory get(String protocol);
}
