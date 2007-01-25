/*
 *  Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation. 
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt). 
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA 
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions. 
 */
package com.sun.mmedia;

import javax.microedition.media.PlayerPermission;
import javax.microedition.io.*;
import java.io.*;

/**
 * A Wrapper class for platform/product specific permission management.
 * This file contains CDC speecific version of the class.
 */
public final class PermissionAccessor {
    
    public static final int PERMISSION_SYSTEM = 0;
    
    public static final int PERMISSION_HTTP_READ = 1;
    public static final int PERMISSION_HTTP_WRITE = 2;
    public static final int PERMISSION_FILE_READ = 3;
    public static final int PERMISSION_FILE_WRITE = 4;
    public static final int PERMISSION_SOCKET_READ = 5;
    public static final int PERMISSION_SOCKET_WRITE = 6;
    
    public static final int PERMISSION_SNAPSHOT = 7;
    public static final int PERMISSION_RECORDING = 8;
    
    private static final String mapPermissions [] = {
        /* PERMISSION_SYSTEM                        */ null,
                
        /* PERMISSION_HTTP_READ                     */ null,
        /* PERMISSION_HTTP_WRITE                    */ null,
        /* PERMISSION_FILE_READ                     */ null,
        /* PERMISSION_FILE_WRITE                    */ null,
        /* PERMISSION_SOCKET_READ                   */ null,
        /* PERMISSION_SOCKET_WRITE                  */ null,
                
        /* PERMISSION_SNAPSHOT                      */ "snapshot",
        /* PERMISSION_RECORDING                     */ "record",
    };
    
    /**
     * Method indended to be called by Players & Controls to check
     * if user application has enough permissions to perform
     * a secured operation ...
     *
     *
     * @param locator - Locator name.
     * @param thePermission - one of PERMISSION_* constants that 
     *        define permissions in an product-independent form.
     */
    public static void checkPermissions(String locator, int thePermission) throws SecurityException, InterruptedException {
        try {
            /* 
             * Map between PermissionAccessor.* permission constants
             * and Permissions.* ...
             * Any incorrect permission constant will result in 
             * ArrayIndexOutOfBoundsException -> 
             * a SecurityException will be thrown !
             */
            String permission = mapPermissions[thePermission];
        
            SecurityManager sm = System.getSecurityManager();
            if (sm != null)
                sm.checkPermission(new PlayerPermission(locator, permission));
        } catch (SecurityException se) {
            throw se;
        } catch (Exception e) {
            throw new SecurityException(
                "Failed to check user permission");
        }
    }
    
    private static final String locatorTypes[] = {
        "capture://audio",
        "capture://video",
        "capture://radio",
        "capture://",
        "device://",
        "file://",
        "http://"
    };
    
    // inidicates that corresponding locator type needs no special permissions.
    private static final int NEED_NO_PERMISSIONS = -2;
    private static final int FAILED_PERMISSIONS  = -1;
            
    private static final int mapLocatorPermissions[] = {
        /* "capture://audio" */ NEED_NO_PERMISSIONS,
        /* "capture://video" */ NEED_NO_PERMISSIONS,
        /* "capture://radio" */ NEED_NO_PERMISSIONS,
        /* "capture://"      */ NEED_NO_PERMISSIONS,
        /* "device://"       */ NEED_NO_PERMISSIONS,
        /* "file://"         */ PERMISSION_FILE_READ,
        /* "http://"         */ PERMISSION_HTTP_READ
    };
    
    /**
     * Method indended to be called by Manager.createDataSource(locator)
     * and checks if user application has enough permissions to use given type
     * of locators to play media contents.
     *
     * @param locator - the URL to be used as media source for player
     */
    public static void checkLocatorPermissions(String locator) throws SecurityException {
        int permission = FAILED_PERMISSIONS;
        try {
            /* 
             * Find Locator type, and map this type to permission.
             * Any incorrect locator will result in 
             * ArrayIndexOutOfBoundsException or NullPointerException -> 
             * a SecurityException will be thrown !
             */
            String locStr = locator.toLowerCase();
            for (int i = 0; i < locatorTypes.length; ++i) {
                if (locStr.startsWith(locatorTypes[i])) {
                    permission = mapLocatorPermissions[i];
                    if (permission == NEED_NO_PERMISSIONS)
                        return; 
                    break;
                }
            }
            
            SecurityManager sm = System.getSecurityManager();
            if (sm != null) {
                switch (permission) {
                    case PERMISSION_FILE_READ:
	                sm.checkPermission(new FilePermission(locator, "read"));
			break;
                    default:
                    case NEED_NO_PERMISSIONS:
			break;
                }
            }
        } catch (SecurityException se) {
            throw se;
        } catch (Exception e) {
            throw new SecurityException(
                "Failed to check locator permission");
        }
    }

    /**
     * Method indended to be called by Manager.createPlayer(DataSource)
     * and checks if user application has enough permissions to playback 
     * media of a given content-type using given type
     * of locators.
     *
     * @param locator - the URL to be used as media source for player, 
     *        can be null if DataSOurce has been created not from locator
     * @param contentType - content-type boolean of the media
     */
    public static void checkContentTypePermissions(
            String locator, String contentType) throws SecurityException {
        /*
         * THIS IS A STUB
         */
    }
}