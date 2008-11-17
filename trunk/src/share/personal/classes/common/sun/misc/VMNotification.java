/*
 * @(#)VMNotification.java	1.14 06/10/10
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

package sun.misc;

/**
 * The <code>sun.misc.VMNotification</code> interface is implemented by classes
 * wishing to receive events when the available memory changes state.
 * <h3>Compatibilty</h3>
 * Some PersonalJava implementations support the <code>sun.misc.VM</code> class.
 * In Personal Profile, the sun.misc package is optional.
 */

public interface VMNotification {
    /**
     * The sun.misc.VMNotification interface is implemented by
     * classes wishing to receive events when the available memory
     * changes state.
     * This method is invoked when the available memory in the system
     * changes state. The listener should take appropriate action to
     * free memory when the available memory is in the YELLOW or RED states.
     *
     * @param oldState The old memory state, one of VM.STATE_GREEN,
     *			VM.STATE_YELLOW or VM.STATE_RED
     * @param newState The new memory state
     *
     * @see sun.misc.VM#registerVMNotification
     */
    void newAllocState(int oldState, int newState, boolean notUsed);
}
