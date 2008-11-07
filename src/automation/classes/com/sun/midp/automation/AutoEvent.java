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
     * <br>&nbsp;&nbsp;
     * <i>type_name arg1_name: arg1_value, arg2_name: arg2_value, ...</i>
     * <br>
     * where <i>arg1_name</i>, <i>arg2_name</i> and so on are event argument 
     * (properties) names, and <i>arg1_value</i>, <i>arg2_value</i> and so on
     * are argument values.
     * <br>
     * For example:
     * <br>&nbsp;&nbsp;
     * <b>pen x: 20, y: 100, state: pressed</b>
     * <br>
     * In this example, <b>pen</b> is type name, <b>x</b> and <b>y</b> are 
     * argument names, and <b>20</b> and <b>100</b> are argument values.
     *
     * @return string representation of event
     */
    public String toString();
}
