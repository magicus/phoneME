/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/**
 * @version @(#)OTAFactory.java	1.2 05/10/20
 */

package com.sun.appmanager.ota;

/**
 * Abstract class providing a factory-like source for
 * OTA download agents.
 */
public abstract class OTAFactory
{
    /**
     * Return implementation instance.
     * @param impl protocol name, which is used to attempt
     * to find an impl class for the protocol (e.g., "MIDP" or
     * "OMA"). This argument can also denote a fully qualified
     * class name to be instantiated.
     * @return OTA implementation class, or <code>null</code>
     * if no implementation found for this protocol. If class
     * name is specified, the class should have argumentless
     * public constructor.
     */
    public OTA getImpl( String impl )
    {
        return null;
    }
}
