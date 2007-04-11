/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.app;

import com.sun.j2me.security.Permission;

/**
 * Abstraction for application package
 */
public class AppPackage {
    
    /** Static instance. Only one package can be run in a isolate */
    private static AppPackage instance = new AppPackage();
    
    /** Guard from 'new' operator */
    private AppPackage() {
    }
    
    public static AppPackage getInstance() {
        return instance;
    }
    
    public int getId() {
        return 0;
    }

    /** Unused ID */
    public static final int UNUSED_APP_ID = -1;

    /**
     * Returns permission status for the specified permission
     *
     * @param p permission to check
     * @return 1 if allowed; 0 if denied; -1 if status is unknown
     */
    public int checkPermission(Permission p) {
        /* Stub: always allowed */
        return 1;
    }    
    
    /**
     * Checks for specified permission status. Throws an exception
     * if permission is not allowed. May be blocked to ask user
     *
     * @param p a permission to check
     * @exception SecurityException if permission is not allowed
     * @exception InterruptedException if another thread interrupts a calling
     *  thread while asking user
     */
    public void checkForPermission(Permission p) throws InterruptedException {
    }    
    
    /**
     * Throws an exception if a status for the permission is not allowed
     *
     * @param p a permission to check
     * @exception SecurityException if a status for the permission is not allowed
     */
    public void checkIfPermissionAllowed(Permission p) {
        if (checkPermission(p) != 1) {
            throw new SecurityException();
        }
    }    

    /**
     * Gets the name of CA that authorized this suite.
     *
     * @return name of a CA or null if the suite was not signed
     */
    public String getCA() {
        return null;
    }
}
