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
package com.sun.mmedia;

import com.sun.midp.security.Permissions;
import com.sun.midp.midlet.Scheduler;
import com.sun.midp.midlet.MIDletSuite;

/**
 * A Wrapper class for platform/product specific permission management.
 * This file contains CLDC/MIDP speecific version of the class.
 */
public final class PermissionAccessor {
    
    public static final int PERMISSION_SYSTEM = 0;
    
    public static final int PERMISSION_HTTP_READ = 1;
    public static final int PERMISSION_HTTP_WRITE = 2;
    public static final int PERMISSION_FILE_READ = 3;
    public static final int PERMISSION_FILE_WRITE = 4;
    public static final int PERMISSION_SOCKET_READ = 5;
    public static final int PERMISSION_SOCKET_WRITE = 6;
    
    public static final int PERMISSION_IMAGE_CAPTURE = 7;
    public static final int PERMISSION_VIDEO_CAPTURE = 8;
    public static final int PERMISSION_AUDIO_CAPTURE = 9;
    
    public static final int PERMISSION_VIDEO_RECORDING = 10;
    public static final int PERMISSION_AUDIO_RECORDING = 11;    
       
    public static final int PERMISSION_VIDEO_SNAPSHOT = 12;
    
    public static final int PERMISSION_VIDEO_CAMERA_SHUTTER_FEEDBACK = 13;    
    public static final int PERMISSION_RADIO_TUNER_SET_PRESET = 14;
    
    private static final int mapPermissions [] = {
        /* PERMISSION_SYSTEM                        */ Permissions.MIDP,
                
        /* PERMISSION_HTTP_READ                     */ Permissions.HTTP,
        /* PERMISSION_HTTP_WRITE                    */ Permissions.HTTP,
        /* PERMISSION_FILE_READ                     */ Permissions.FILE_CONNECTION_READ,
        /* PERMISSION_FILE_WRITE                    */ Permissions.FILE_CONNECTION_WRITE,
        /* PERMISSION_SOCKET_READ                   */ Permissions.TCP,
        /* PERMISSION_SOCKET_WRITE                  */ Permissions.TCP,
                
        /* PERMISSION_IMAGE_CAPTURE                 */ Permissions.MM_IMAGE_CAPTURING,
        /* PERMISSION_VIDEO_CAPTURE                 */ Permissions.MM_RECORD,
        /* PERMISSION_AUDIO_CAPTURE                 */ Permissions.MM_RECORD,
                
        /* PERMISSION_VIDEO_RECORDING               */ Permissions.MM_RECORD,
        /* PERMISSION_AUDIO_RECORDING               */ Permissions.MM_RECORD,
                
        /* PERMISSION_VIDEO_SNAPSHOT                */ Permissions.MM_IMAGE_CAPTURING,
                
        //ATTENTION: THE FOLLOWING AMMS PERMISSIONS ARE NOT DEFINED YET
        /* PERMISSION_VIDEO_CAMERA_SHUTTER_FEEDBACK */ //Permissions.AMMS_CAMERA_SHUTTERFEEDBACK,
        /* PERMISSION_RADIO_TUNER_SET_PRESET        */ //Permissions.AMMS_TUNER_SETPRESET
    };
    
    /**
     * Method indended to be called by Players & Controls to check
     * if user application has enough permissions to perform
     * a secured operation ...
     *
     * @param thePermission - one of PERMISSION_* constants that 
     *        define permissions in an product-independent form.
     */
    public static void checkPermissions(int thePermission) throws SecurityException {
        try {
            /* 
             * Map between PermissionAccessor.* permission constants
             * and Permissions.* ...
             * Any incorrect permission constant will result in 
             * ArrayIndexOutOfBoundsException -> 
             * a SecurityException will be thrown !
             */
            int permission = mapPermissions[thePermission];
        
            MIDletSuite midletSuite = Scheduler.getScheduler().getMIDletSuite();
            midletSuite.checkIfPermissionAllowed( permission );
        } catch (SecurityException se) {
            ///*DEBUG:*/ se.printStackTrace();
            throw se;
        } catch (Exception e) {
            ///*DEBUG:*/ e.printStackTrace();
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
    private static final int NEED_NO_PERMISSIONS = 0;
    private static final int FAILED_PERMISSIONS = -1;
            
    private static final int mapLocatorPermissions[] = {
        /* "capture://audio" */ NEED_NO_PERMISSIONS,
        /* "capture://video" */ NEED_NO_PERMISSIONS,
        /* "capture://radio" */ NEED_NO_PERMISSIONS,
        /* "capture://"      */ NEED_NO_PERMISSIONS,
        /* "device://"       */ NEED_NO_PERMISSIONS,
        /* "file://"         */ Permissions.FILE_CONNECTION_READ,
        /* "http://"         */ Permissions.HTTP
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
            
            MIDletSuite midletSuite = Scheduler.getScheduler().getMIDletSuite();
            midletSuite.checkIfPermissionAllowed( permission );
        } catch (SecurityException se) {
            ///*DEBUG:*/ se.printStackTrace();
            throw se;
        } catch (Exception e) {
            ///*DEBUG:*/ e.printStackTrace();
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
