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
package com.sun.jumpimpl.module.preferences;

import java.util.Map;
import com.sun.jump.module.contentstore.JUMPContentStore;
import com.sun.jump.module.contentstore.JUMPStore;
import com.sun.jump.module.contentstore.JUMPStoreFactory;
import com.sun.jump.module.contentstore.JUMPStoreHandle;

class PreferencesStore extends JUMPContentStore {

    private JUMPStoreHandle storeHandle = null;
    private Map configData; 

    /**
     * Name of directory where preferences will be stored
     */
    protected static String REPOSITORY_PREFERENCES_ROOT_DIRNAME =
        "./preferences";

    /**
     * Returns an instance of the content store for Preferences module to use.
     * @return Instance of JUMPStore
     */
    protected JUMPStore getStore() {
        JUMPStore store = JUMPStoreFactory.getInstance().
            getModule(JUMPStoreFactory.TYPE_FILE);        
        return store;
    }
    
    /**
     * Loads the module.
     *
     * @param config the configuration data required for loading this service.
     */
    public void load(final Map map) {
        this.configData = map;
    }

    /**
     * Unload all the resources the module uses.
     */
    public void unload() {
        closeStore(storeHandle);
        storeHandle = null;
    }


}

