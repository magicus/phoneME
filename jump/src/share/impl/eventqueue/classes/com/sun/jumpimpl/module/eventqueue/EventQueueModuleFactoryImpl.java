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

package com.sun.jumpimpl.module.eventqueue;

import com.sun.jump.module.eventqueue.JUMPEventQueueModuleFactory;
import com.sun.jump.module.eventqueue.JUMPEventQueueModule;
import java.util.Map;

/**
 * <code>EventQueueModuleFactoryImpl</code> is a factory for
 * <code>EventQueueModuleImpl</code>.
 */
public class EventQueueModuleFactoryImpl extends JUMPEventQueueModuleFactory {
    /** The only instance of event queue module. */
    private static JUMPEventQueueModule MODULE = null;

    /**
     * Returns a <code>JUMPEventQueueModule</code> instance.
     */
    public JUMPEventQueueModule getModule() {
        if (MODULE == null) {
            MODULE = new EventQueueModuleImpl();
        }
        return MODULE;
    }

    /**
     * Loads the event queue factory and module.
     *
     * @param config properties of this module factory.
     */
    public void load(Map config) {
        getModule().load(config);
    }

    /**
     * Unloads the event queue factory.
     */
    public void unload() {
        if (MODULE != null) {
            MODULE.unload();
        }
    }
}
