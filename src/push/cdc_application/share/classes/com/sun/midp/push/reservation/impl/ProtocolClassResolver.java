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

/** Generic resolving of classes implementing the protocol. */
public interface ProtocolClassResolver {
    /**
     * Gets name of the class which supports given protocol.
     *
     * @param protocol lower-case name of the protocol (already validated); note
     *  that RFC allows +/-/. characters.
     *
     * @return fully qualified name of the class which supports the protocol or
     *  <code>null</code> if there is no such class; note that the class is not
     *  guaranteed to exist: the only promise is that if there is such a class
     *  it has the given name; the class MUST implement
     *  <code>ProtocolFactory</code> argument and have default ctor.  
     */
    String getClassName(String protocol);
}
