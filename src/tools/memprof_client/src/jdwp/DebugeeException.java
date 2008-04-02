/*
 *   
 *
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


/*
 * DebugeeException.java
 *
 * Created on March 7, 2001, 5:35 PM
 */

package com.sun.cldchi.tools.memoryprofiler.jdwp;

/**
 * This exception is raised in case of generic debugger error. The possible
 * causes for this exception are unexpected JDWP reply packet error code,
 * incorrect information inside JDWP packet, network problems etc.
 * The raising of this exception generally means a fatal error of KJDB.
 *
 */
public class DebugeeException extends java.lang.Exception {

    /**
     * Creates new <code>DebugeeException</code> without detail message.
     */
    public DebugeeException() {
    }


    /**
     * Constructs an <code>DebugeeException</code> with the specified detail message.
     *
     * @param msg the detail message.
     */
    public DebugeeException(String msg) {
        super(msg);
    }
}
