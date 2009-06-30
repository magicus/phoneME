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

package com.sun.midp.i3test;

import com.sun.midp.main.NamsNotifier;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Base class for I3 test cases in case of NAMS build.
 * Shall be used instead of TestCase.
 */
public abstract class NamsTestCase extends TestCase {

    /** contains list of sync objects, accessible to derived classes */
    protected static NamsNotifier namsNotifier = null;
    
    /**
     * Constructs a TestCase.  Since TestCase is an abstract class, this 
     * constructor is called only at the time a TestCase subclass is being 
     * constructed.
     */
    public NamsTestCase() {
        Logging.report(Logging.WARNING, LogChannels.LC_CORE,
           "DEBUG: NamsTestCase: constructor ... " + 
            this.getClass().getName());
            
        
        if (namsNotifier == null)
            namsNotifier = new NamsNotifier();
    }
}
