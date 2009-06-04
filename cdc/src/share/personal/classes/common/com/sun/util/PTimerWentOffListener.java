/*
 * @(#)PTimerWentOffListener.java	1.14 06/10/10
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
 *
 */

package com.sun.util;
    
/**
 * The PTimerWentOffListener is an interface which must be implemented
 * for an object to be informed of PTimerWentOffEvents.
 * <h3>Compatibility</h3>
 * The PTimerWentOffEventListener is supported in PersonalJava implementations.
 * It is deprecated in Personal Profile and not supported in J2SE, or J2ME
 * foundation profiles.
 *
 * @deprecated
 * @since: PersonalJava1.0
 */
public interface PTimerWentOffListener {
    /**
     * Called when a PTimerWentOff event occurs.
     * @param e the event specifying which timer and which timer specification
     * went off.
     */
    void timerWentOff(PTimerWentOffEvent e);
}
