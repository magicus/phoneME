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
 *  Represents MIDlet in non-started state.
 */
public interface AutoMIDletDescriptor {
    /**
     * Gets MIDlet's suite descriptor.
     *
     * @return AutoMIDletSuiteDescriptor representing MIDlet's suite
     */
    public AutoSuiteDescriptor getSuiteDescriptor();
    
    /**
     * Gets MIDlet's name.
     *
     * @return MIDlet's name as specified in jad/jar file
     */
    public String getMIDletName();

    /**
     * Gets name of the class implementing MIDlet interface
     * for this MIDlet.
     *  
     * @return name of the class implementing MIDlet interface 
     * as specified in jad/jar file
     */
    public String getMIDletClassName();

    /**
     * Starts this MIDlet.
     *
     * @param args MIDlet's arguments
     * @return AutoMIDlet representing started MIDlet
     * @throws RuntimeException if MIDlet couldn't be started
     */
    public AutoMIDlet start(String[] args);
}

