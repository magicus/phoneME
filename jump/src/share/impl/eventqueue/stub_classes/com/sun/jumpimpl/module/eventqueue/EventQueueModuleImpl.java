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

import java.util.Map;
import com.sun.jump.module.eventqueue.JUMPEventQueueModule;
import com.sun.jump.module.eventqueue.JUMPEventHandler;

/**
 * Empty implementation of <code>JUMPEventQueueModule</code>.
 */
public class EventQueueModuleImpl implements JUMPEventQueueModule {
    /**
     * Loads the event queue module and starts to receive native events.
     *
     * @param config properties of this module factory.
     */
    public void load(Map config) { }

    /**
     * Unloads the event queue module.
     */
    public void unload() { }

    /**
     * Registers a handler for events of the given type.
     *
     * @param type event type, which equals to the number of JSR that
     *        will handle events.
     * @param handler a <code>JUMPEventHandler</code> instance that
     *        will deal with all events of this type.
     */
    public void registerEventHandler(int type, JUMPEventHandler handler) { }
}
