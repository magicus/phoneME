/*
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

package com.sun.midp.jump.installer;

import java.io.IOException;
import com.sun.jump.common.JUMPContent;

/**
 * Abstracts out the operation against the midlet repository class,
 * <code>com.sun.midp.midletsuite.MIDletSuiteStorage</code>, 
 * needed from the jump code.
 **/

public interface StorageAccessInterface {

    /**
     * Gets the list of midlet suite IDs installed in the repository
     * currently.
     *
     * @return The list of installed midlet suites.
     */
    public int[] getInstalledMIDletSuiteIds();   
    
    /**
     * Takes the midlet suite ID and converts it to the 
     * <code>JUMPContent</code> instances, where each midlet in the suite is 
     * a separate <code>JUMPContent</code>.
     *
     * @param The midlet suite id 
     * @return The list of <code>JUMPContent</code>, a zero length array if 
     * the midlet suite is not in the repository.
     */
    public JUMPContent[] convertToMIDletApplications(int suiteId); 
    
    /**
     * Removes the midlet suite from the repository.  Does nothing
     * if the midlet suite is not installed.
     * 
     * @param The midlet suite id to delete.
     */
    public void remove(int suiteId); 
}
