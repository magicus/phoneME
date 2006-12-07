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

package com.sun.jump.command;

/**
 * <code>JUMPExecutiveWindowRequest</code> defines all the windowing  
 * related requests that originate from the <Code>JUMPExecutive</code>
 */
public interface JUMPExecutiveWindowRequest {
    /**
     * Command to bring the Isolate's window to the foreground.
     * <ol>
     *   <li>args[0] - Window Id</li>
     * </ol>
     * Synchronous request. expects a 
     * {@link com.sun.jump.command.JUMPResponse#Success} or 
     * {@link com.sun.jump.command.JUMPResponse#Failure}
     */
    public static final String ID_FOREGROUND= "Foreground";
    
    /**
     * Command to bring the Isolate's window to the background.
     * <ol>
     *   <li>args[0] - Window Id</li>
     * </ol>
     * Synchronous request. expects a 
     * {@link com.sun.jump.command.JUMPResponse#Success} or 
     * {@link com.sun.jump.command.JUMPResponse#Failure}
     */
    public static final String ID_BACKGROUND = "Background";
}
