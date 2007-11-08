/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jump.common;

/**
 * <code>JUMPWindow</code> is an abstraction for a window in the JUMP system.
 * Each application and each isolate can have multiple JUMPWindow's.
 * Each window has a set of states, and a 64-bit ID.
 */
public abstract class JUMPWindow {
    public static final String FOREGROUND_STATE = "foreground";
    public static final String BACKGROUND_STATE = "background";

    /**
     * Returns the state of the window
     */
    public abstract String getState();

    /**
     * Returns an ID for the window
     */
    public abstract int getId();

    /** 
     * Return the isolate that this window runs in
     */
    public abstract JUMPIsolate getIsolate();
}
