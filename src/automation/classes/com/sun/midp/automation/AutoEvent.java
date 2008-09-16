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
 * Represents generic event. Serves as base interface for all specific
 * event interfaces.
 */
public interface AutoEvent {
    /**
     * Gets event type.
     *
     * @return AutoEventType representing event type
     */
    public AutoEventType getType();

    /**
     * Gets string representation of event. The format is following:
     * type_name arg1_name: arg1_value, arg2_name: arg2_value, ...
     * where "arg1_name", "arg2_name" and so on are event argument (properties) 
     * names, and "arg1_value", "arg2_value" and so on are argument values.
     * For example:
     *  pen x: 20, y: 100, state: pressed
     * In this example, "pen" is type name, "x" and "y" are argument names, 
     * and "20" and "100" are argument values.
     *
     * @return string representation of event
     */
    public String toString();
}
