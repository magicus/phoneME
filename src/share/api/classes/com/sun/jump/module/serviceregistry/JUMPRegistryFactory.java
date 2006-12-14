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

package com.sun.jump.module.serviceregistry;

import com.sun.jump.module.JUMPModuleFactory;

/**
 * <code>JUMPRegistryFactory</code> is a factory for <code>JUMPServiceRegistry</code>.
 */
public abstract class JUMPRegistryFactory extends JUMPModuleFactory {
    
    private static JUMPRegistryFactory INSTANCE = null;
    
    public static JUMPRegistryFactory getInstance() {
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPRegistryFactory
     */
    protected JUMPRegistryFactory() {
        synchronized (JUMPRegistryFactory.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
    }
    
    /**
     * Returns the <code>JUMPServiceRegistry</code> for the ClassLoader passed in.
     * 
     * @param loader the classloader to use for the service registry
     *        when loading new stub objects.
     * @throws java.lang.NullPointerException if the ClassLoader is null.
     */
    public abstract JUMPServiceRegistry getModule(ClassLoader loader);
}
