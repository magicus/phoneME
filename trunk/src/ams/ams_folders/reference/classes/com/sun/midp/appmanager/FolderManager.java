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

import java.util.Vector;

import javax.microedition.lcdui.Image;

import com.sun.midp.*;
import javax.microedition.lcdui.*;
import com.sun.midp.installer.GraphicalInstaller;

/**
 * Provides functionality to manage folders.
 */
class FolderManager {
    /** Folder ID that is never used. */
    public static final int UNUSED_FOLDER_ID  = -1;

    /**
     * ID of the folder where user applications
     * will be installed by default.
     */
    public static final int DEFAULT_FOLDER_ID = 1;

    private static Vector foldersVector = new Vector(5);

    /**
     * Initialization.
     */
    static {
        loadFoldersInfo0();
    }

    /**
     * Returns the number of folders present in the system
     *
     * @return number of folders present in the system
     */
    public static int getFolderCount() {
        return foldersVector.size();
    }

    /**
     * Returns a vector of folders present in the system
     *
     * @return vector of <code>Folder</code> objects
     */
    public static Vector getFolders() {
        return foldersVector;
    }

    /**
     * Returns an ID of the folder where user applications
     * are installed by default. 
     *
     * @return ID of the folder where user applications are installed by default
     */
    public static int getDefaultFolderId() {
        return DEFAULT_FOLDER_ID;
    }

    /**
     * Returns a folder with the given ID
     *
     * @param folderId ID of the folder to find
     *
     * @return <code>Folder</code> having the given ID or null if not found
     */
    public static Folder getFolderById(int folderId) {
        Folder f = null;

        for (int i = 0; i < foldersVector.size(); i++) {
            Folder tmp = (Folder)foldersVector.elementAt(i);
            if (tmp.getId() == folderId) {
                f = tmp;
                break;
            }
        }

        return f;
    }

    /**
     * Creates a new folder.
     *
     * @param f folder to create
     */
    public static void createFolder(Folder f) {
        /*
         * IMPL_NOTE: our current implementation doesn't support
         *            creation of user folders.
         */
    }

    /**
     * Removes a folder.
     *
     * @param f folder to delete
     */
    public static void deleteFolder(Folder f) {
        /*
         * IMPL_NOTE: our current implementation doesn't support
         *            user folders.
         */
    }

    /**
     * Loads information about folders present in the system.
     */
    private static void loadFoldersInfo0() {

	Image icon;

        icon = GraphicalInstaller.getImageFromInternalStorage("Folder");

        /* IMPL_NOTE: temporary hardcoded, should be moved to an xml */
        foldersVector.addElement(new Folder(0, -1, "Preinstalled apps", icon));
        foldersVector.addElement(new Folder(DEFAULT_FOLDER_ID,
                                            -1, "Other apps", icon));
    }
}
