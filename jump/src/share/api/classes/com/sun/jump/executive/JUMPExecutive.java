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

package com.sun.jump.executive;

import com.sun.jump.common.JUMPProcess;
import com.sun.jump.message.JUMPMessagingService;
import java.util.HashMap;
import java.util.Map;

/**
 * <code>JUMPExecutive</code> hosts all the JUMP executive services. 
 * The services are declratively specified 
 * in a file, which allows the executive to be extended with new 
 * services. 
 * 
 * @see com.sun.jump.module.JUMPModule
 */
public abstract class JUMPExecutive
    implements JUMPProcess, JUMPMessagingService {

    private static JUMPExecutive INSTANCE = null;
    
    protected Map moduleFactoryMap = null;
    
    /**
     * Returns the singleton executive instance.
     */ 
    public static JUMPExecutive getInstance() {
	return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPExecutive
     */
    protected  JUMPExecutive() {
        synchronized (JUMPExecutive.class){
            if ( INSTANCE == null ) {
                INSTANCE = this;
            }
        }
        this.moduleFactoryMap = new HashMap();
    }
    
    /**
     * Initialize the executive. This method initializes the list of 
     * modules, by reading the module definitions from a property file. This
     * method can be overriden to load the modules using some other 
     * mechanism, even possibly hardcoding the list of modules.
     */
    protected void initialize() {
    }
    
    /**
     * Gets the User input manager configured with the executive.
     */
    public abstract JUMPUserInputManager getUserInputManager();

    /**
     * Gets the Isolate factory configured with the executive.
     */

    public abstract JUMPIsolateFactory getIsolateFactory();
}
