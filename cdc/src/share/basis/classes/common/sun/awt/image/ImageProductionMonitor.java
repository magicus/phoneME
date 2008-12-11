/*
 * @(#)ImageProductionMonitor.java	1.13 06/10/10
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

package sun.awt.image;

//import sun.misc.VM;
/**
 * This class monitors image production.  
 *
 * Currently it excersizes its control in two ways:
 *
 * 1) It determines whether image production is allowed or not
 *    by checkingn the VM's memory state.  Image production is
 *    disabled when the state reaches a certain level.
 *    By default, image production is disabled when the state 
 *    reaches VM.STATE_YELLOW.
 *
 * 2) It also determines how many image fetcher threads are allowed
 *    to function concurrently.  This number is by default 4.
 *
 **/
class ImageProductionMonitor {
    //    private static final int abortMemState = VM.STATE_YELLOW;
    private static final int abortMemState = 0;
    static final int MAX_FETCHER_THREADS = 4;
    static boolean allowsProduction() {
        //	if (VM.getStateQuick() >= abortMemState) {
        //	    return false;
        //	}
        return true;
    }
}
