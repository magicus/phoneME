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
 *  Represents MIDlet in launched state.
 */
public interface AutoMIDlet {
    /**
     * Initiates a switch (transition) from current to specified 
     * lifecycle state.
     *
     * @param state state to switch to
     * @param wait if true, wait (block) until transition to the 
     * specified state has been completed
     * @throws IllegalStateException thrown when switching to specified
     * state is invalid
     */
    public void switchTo(AutoMIDletLifeCycleState state, boolean wait) 
        throws IllegalStateException;
    
    /**
     * Initiates a switch (transition) from current to specified 
     * foreground state.
     *
     * @param state state to switch to
     * @param wait if true, wait (block) until transition to the 
     * specified state has been completed
     * @throws IllegalStateException thrown when switching to specified
     * state is invalid
     */
    public void switchTo(AutoMIDletForegroundState state, boolean wait) 
        throws IllegalStateException;



    /**
     * Waits (blocks) until MIDlet reaches specified lifecycle state.
     *
     * @param state state to wait for
     * @throws IlegalStateException thrown if state to wait for is invalid,
     * or if during waiting MIDlet has reached the state where waiting for
     * specified state is no longer valid
     */
    public void waitFor(AutoMIDletLifeCycleState state)
        throws IllegalStateException;
        
    /**
     * Waits (blocks) until MIDlet reaches specified foreground state.
     *
     * @param state state to wait for
     * @throws IlegalStateException thrown if state to wait for is invalid,
     * or if during waiting MIDlet has reached the state where waiting for
     * specified state is no longer valid
     */
    public void waitFor(AutoMIDletForegroundState state)
        throws IllegalStateException;

    
    
    /**
     * Gets current lifecycle state.
     *
     * @return AutoLifeCycleState representing current
     * lifecycle state
     */
    public AutoMIDletLifeCycleState getLifeCycleState();

    
    /**
     * Gets current foreground state.
     *
     * @return AutoForegroundState representing current
     * foreground state
     */
    public AutoMIDletForegroundState getForegroundState();
    

    /**
     * Gets MIDlet's descriptor.
     *
     * @return AutoMIDletDescriptor representing descriptor.
     */
    public AutoMIDletDescriptor getMIDletDescriptor();
}
