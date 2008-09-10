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
 *  Represents our Java ME system.
 */
public abstract class Automation {

    public final static Automation getInstance() 
        throws IllegalStateException {

        return AutomationImpl.getInstanceImpl();
    }

    /**
     * Gets instance of AutoSuiteStorage class.
     *
     * @return instance of AutoSuiteStorage class
     * @throws IllegalStateException if Automation API hasn't been
     * initialized or is not permitted to use
     */    
    public abstract AutoSuiteStorage getStorage() 
        throws IllegalStateException;

    public abstract AutoEventFactory getEventFactory()
        throws IllegalStateException;


    public abstract void simulateEvents(AutoEvent event);

    public abstract void simulateEvents(AutoEventSequence events, 
            double speedAdjustment);

    public abstract void simulateEvents(AutoEventSequence events);


    public abstract void simulateKeyEvent(AutoKeyCode keyCode, 
            AutoKeyState keyState, int delayMsec) 
        throws IllegalArgumentException;

    public abstract void simulateKeyEvent(char keyChar, AutoKeyState keyState, 
            int delayMsec) 
        throws IllegalArgumentException;

    public abstract void simulateKeyClick(AutoKeyCode keyCode, int delayMsec) 
        throws IllegalArgumentException;
    
    public abstract void simulateKeyClick(char keyChar, int delayMsec) 
        throws IllegalArgumentException;

    public abstract void simulatePenEvent(int x, int y, AutoPenState penState, 
            int delayMsec) 
        throws IllegalStateException;

    public abstract void simulatePenClick(int x, int y, int delayMsec);

    public abstract void simulateDelayEvent(int msec) 
        throws IllegalArgumentException;
}
