/*
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
 */

/*
 * @(#)MidletAMS.java	1.4	06/10/10
 */
package com.sun.javax.microedition.midlet;
import sun.misc.MIDPImplementationClassLoader;
import sun.misc.MemberFilter;

/*
 * A simple interface for use by AMSmain in accessing AMS.
 */
public interface MidletAMS {


    /*
     * Should be run once only on the first instance of one of
     * these. Would more logically be static and called
     * using reflection. In which case it would NOT be part
     * of an interface.
     */
    public boolean
    setupSharedState(
	MIDPImplementationClassLoader me,
	MemberFilter mf) throws SecurityException;


    /*
     * Per instance initialization.
     */
    public boolean
    initializeSuite(String midletSuitePath);

    /*
     * Do the work.
     * Return when the suite goes away.
     */
    public void runSuite();
}
