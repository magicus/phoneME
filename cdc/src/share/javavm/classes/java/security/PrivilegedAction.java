/*
 * @(#)PrivilegedAction.java	1.12 06/10/10
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

package java.security;


/**
 * A computation to be performed with privileges enabled.  The computation is
 * performed by invoking <code>AccessController.doPrivileged</code> on the
 * <code>PrivilegedAction</code> object.  This interface is used only for
 * computations that do not throw checked exceptions; computations that
 * throw checked exceptions must use <code>PrivilegedExceptionAction</code>
 * instead.
 *
 * @see AccessController
 * @see AccessController#doPrivileged(PrivilegedAction)
 * @see PrivilegedExceptionAction
 */

public interface PrivilegedAction {
    /**
     * Performs the computation.  This method will be called by
     * <code>AccessController.doPrivileged</code> after enabling privileges.
     *
     * @return a class-dependent value that may represent the results of the
     *	       computation. Each class that implements
     *         <code>PrivilegedAction</code>
     *	       should document what (if anything) this value represents.
     * @see AccessController#doPrivileged(PrivilegedAction)
     * @see AccessController#doPrivileged(PrivilegedAction,
     *                                     AccessControlContext)
     */
    Object run();
}
