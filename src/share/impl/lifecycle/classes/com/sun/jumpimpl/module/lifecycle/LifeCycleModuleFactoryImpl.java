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

package com.sun.jumpimpl.module.lifecycle;

import java.util.Map;
import com.sun.jump.module.lifecycle.JUMPLifeCycleModule;
import com.sun.jump.module.lifecycle.JUMPLifeCycleModuleFactory;

/**
 *
 */
public class LifeCycleModuleFactoryImpl extends JUMPLifeCycleModuleFactory {
    private JUMPLifeCycleModule MODULE;
    
    /** 
     * Creates a new instance of LifeCycleModuleFactoryImpl 
     */
    public LifeCycleModuleFactoryImpl() {
	super();
    }
    
    /** 
     * Get the singleton lifecycle module for this address space
     */
    public synchronized JUMPLifeCycleModule getModule() {
	if (MODULE == null) {
	    MODULE = new LifeCycleModuleImpl();
	}
	return MODULE;
    }
        
    public void load(Map config) {
	getModule(); // Create our singleton
	MODULE.load(config);
    }
    
    public void unload() {
	getModule(); // Create our singleton
	MODULE.unload();
    }
}
