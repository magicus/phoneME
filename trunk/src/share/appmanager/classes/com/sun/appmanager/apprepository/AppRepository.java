/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.apprepository;

import com.sun.appmanager.ota.Descriptor;
import com.sun.appmanager.ota.Destination;

/**
 * Interface for the application manager's repository which contains
 * necessary items like application files, application descriptors, etc.
 */

public interface AppRepository {

    /**
     * Return an array of AppModules representing available applications
     * on the device for the given user.
     * @return AppModule[] array of application modules for each
     *   available application and menu
     */
    AppModule[] getAppList();

    /**
     * Create a Destination object to handle the download of a new application
     * @param descriptor Descriptor object pertaining to the application to install
     * @return New Destination object, null if one cannot be created
     */
    Destination getDestination(Descriptor descriptor);

    /**
     * Install an app bundle containing one or more apps into the repository
     * @param destination Destination object pertaining to the application to install
     * @param menu the menu within the application manager in which this
     * application should be installed in.
     * @return AppModule module object pertaining to the newly installed application, null
     * the application could not be installed
     */
    AppModule[] installApplication(Destination destination, String menu);

    /**
     * Remove an app bundle containing one or more apps from the repository
     * @param module applications to be removed.  These apps must all be
     * part of the same app bundle.
     * @return boolean true if all apps were removed, false otherwise.  This
     * method makes an attempt to remove each application regardless of if
     * any previous apps fail to be removed, but will return a false if any
     * application fails to be removed completely.  Also, all applications specified
     * in module[] must be from the same bundle.  This method will return false
     * immediately if this is not the case.
     *
     */
    boolean removeApplication(AppModule module[]);

}
