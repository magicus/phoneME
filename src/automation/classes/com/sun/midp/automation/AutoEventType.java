/*
 *   
 *
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

package com.sun.midp.automation;

/**
 * Represents event type.
 */
public final class AutoEventType {
    /**
     * Event type: key event
     */
    public static final AutoEventType KEY = 
        new AutoEventType("key");

    /**
     * Event type: pen event
     */
    public static final AutoEventType PEN = 
        new AutoEventType("pen");   

    /**
     * Event type: delay event
     */
    public static final AutoEventType DELAY = 
        new AutoEventType("delay");

    /** Type name */
    private String name;


    /**
     * Gets the type name
     *
     * @return type name as string
     */
    public String getName() {
        return name;
    }

    /**
     * Private constructor to prevent creating class instances
     */
    private AutoEventType(String name) {
        this.name = name;
    }
}
