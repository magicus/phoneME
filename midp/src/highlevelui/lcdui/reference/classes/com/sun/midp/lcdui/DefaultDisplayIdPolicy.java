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

package com.sun.midp.lcdui;

/**
 * Enables AMS to assign displays IDs instead of the local display
 * container.
 */
class DefaultDisplayIdPolicy implements DisplayIdPolicy {
  
    /** ID of the Isolate this instance is created in */
    private final int isolateId;

    /** Last local Display count used to create Display ID */
    private int lastLocalDisplayId;

    /**
     * Initialize the policy.
     *
     * @param theIsolateId Isolate ID to OR with the display IDs
     */
    DefaultDisplayIdPolicy(int theIsolateId) {
        isolateId = theIsolateId;
    }

    /**
     * Create a display ID and set it in the display object.
     *
     * @param display reference to the displays implementation data
     *                the method must call the setDisplayId method
     * @param container container of sibling displays to check for duplicate
     *                  IDs
     */
    public void createDisplayId(DisplayAccess display,
                                DisplayContainer container) {
        int id;
        
        do {
            lastLocalDisplayId++;
	    // [high 8 bits: isolate id][low 24 bits: display id]]
            id = ((isolateId & 0xff)<<24) | (lastLocalDisplayId & 0x00ffffff);
        } while (container.findDisplayById(id) != null);

        display.setDisplayId(id);
    }
}
