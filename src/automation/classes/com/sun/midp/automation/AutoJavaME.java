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
public abstract class AutoJavaME {

    public final static AutoJavaME getInstance() 
        throws IllegalStateException {

        return AutoJavaMEImpl.getInstanceImpl();
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

    public abstract void injectEvent(AutoEvent event);
    public abstract void injectKeyEvent(AutoKeyState keyState, 
            AutoKeyCode keyCode);
    public abstract void injectKeyEvent(AutoKeyState keyState, char keyChar);

    public abstract void replayEvents(AutoEventsSequence events, 
            int speedDivisor);
    public abstract void replayEvents(AutoEventsSequence events);
}
