/*
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

package util;

/**
 * A collection of assertion utilities to assert the state of the system
 * throughout the code.
 */
public class Assert
{
    /**
     * Indicates if classloading is allowed in the current state.
     * A non-zero value indicates that classloading is not currently allowed.
     * NOTE: this assertion assumes that JCC is executing as a single-
     * threaded app as is the case currently.  In a multi-threaded app,
     * this variable will need to be converted into a thread local var.
     */
    private static int disallowClassloadingCount = 0;

    /**
     * Declares that classloading is allowed again.  However, classloading is
     * only truly allowed after there is a matching number of allows for
     * each call to disallowClassloading() that preceeded it.
     * NOTE: You should never call allowClassloading() more times then the
     *number of preceeding calls to disallowClassloading().
     */
    public static void allowClassloading() {
        disallowClassloadingCount--;
        assert(disallowClassloadingCount >= 0);
    }
    /**
     * Declares that classloading is disallowed.  May be called repeatedly.
     * Classloading will only be allowed again after there is a matching
     * number of calls to allowClassloading() for each call to
     * disallowClassloading() that preceeded it.
     * NOTE: You should never call allowClassloading() more times then the
     *number of preceeding calls to disallowClassloading().
     */
    public static void disallowClassloading() {
        disallowClassloadingCount++;
    }
    /** Asserts that classloading is currently allowed. */
    public static void assertClassloadingIsAllowed() {
        assert(disallowClassloadingCount == 0);
    }

}
