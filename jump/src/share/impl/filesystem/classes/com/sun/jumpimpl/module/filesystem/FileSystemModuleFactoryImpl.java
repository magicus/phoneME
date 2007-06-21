/*
 *   
 *
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

package com.sun.jumpimpl.module.filesystem;

import com.sun.jump.module.JUMPModuleFactory;
import com.sun.jump.module.JUMPModule;
import java.util.Map;

/**
 * <code>FileSystemModuleFactoryImpl</code> is a factory for 
 * <code>FileSystemModuleImpl</code>.
 */
public class FileSystemModuleFactoryImpl extends JUMPModuleFactory {
    /** The only instance of FileSystem module. */
    private static JUMPModule MODULE = null;

    /**
     * Load the module factory. 
     *
     * @param config configuration for the factory and the modules created
     *        by the factory.
     */
    public void load(Map config) {
        try {
            Object o = Class.forName("com.sun.jumpimpl.module.filesystem.FileSystemModuleImpl")
                .newInstance();
            MODULE = (JUMPModule)o;
            MODULE.load(config);
        } catch (ClassNotFoundException e) {
            /* The corresponding module may be not present - ignore silently. */
        } catch (InstantiationException e) {
            throw new RuntimeException("FileSystem module initialization failed.");
        } catch (IllegalAccessException e) {
            throw new RuntimeException("FileSystem module initialization failed.");
        }
    }

    /**
     * Unload the factory.
     */
    public void unload() {
        if (MODULE != null) {
            MODULE.unload();
        }
    }
}
