/*
 * %W% %E%
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

package com.sun.jump.module.contentstore;

import com.sun.jump.module.JUMPModuleFactory;

/**
 * <code>JUMPStoreFactory</code> is a factory for <code>JUMPStore</code>.
 */
public abstract class JUMPStoreFactory extends JUMPModuleFactory {
    public static final String TYPE_FILE = "file";
    
    private static JUMPStoreFactory INSTANCE = null;
    
    public static JUMPStoreFactory getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPStoreFactory
     */
    protected JUMPStoreFactory() {
        synchronized (JUMPStoreFactory.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns the <code>JUMPStore</code> for the store type passed.
     * 
     * @param storeType the mechanism used to implement the store. The 
     *        supported types are
     *        <ul>
     *          <li>{@link #TYPE_FILE}</li>
     *        </ul>
     * @throws java.lang.IllegalArgumentException if the store type is not 
     *         not supported by the factory.
     */
    public abstract JUMPStore getModule(String storeType);
}
