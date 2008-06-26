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
package com.sun.midp.appmanager;

import javax.microedition.lcdui.*;

/**
 * Selector provides a simple user interface to select MIDlets to run.
 * It extracts the list of MIDlets from the attributes in the
 * descriptor file and presents them to the user using the MIDlet-&lt;n&gt;
 * name and icon if any. When the user selects a MIDlet an instance
 * of the class indicated by MIDlet-&lt;n&gt; classname is created.
 */
final class MIDletSelector {
    /**
     * Create and initialize a new Selector MIDlet.
     * The Display is retrieved and the list of MIDlets read
     * from the descriptor file.
     *
     * @param theSuiteInfo information needed to display a list of MIDlets
     * @param theDisplay the Display
     * @param theParentDisplayable the parent's displayable
     * @param theManager the parent application manager
     */
    MIDletSelector(RunningMIDletSuiteInfo theSuiteInfo, Display theDisplay,
                   Displayable theParentDisplayable,
                   ApplicationManager theManager) throws Throwable {

    }

}



