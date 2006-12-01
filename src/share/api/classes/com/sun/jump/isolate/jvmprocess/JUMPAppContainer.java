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

package com.sun.jump.isolate.jvmprocess;

import com.sun.jump.common.JUMPWindow;
import com.sun.jump.messagequeue.JUMPMessage;
import com.sun.jump.messagequeue.JUMPMessageHandler;


/**
 * <code>JUMPAppContainer</code> defines the application container. 
 * The container is responsible for launching the the application class
 * and hosts the application and handles all the JUMP Isolate related 
 * functionality transparently to the application. It also receives 
 * and processes application model agnostic messages.
 * 
 * <p>
 * This class is extended by AppModel specific containers.
 */
public abstract class JUMPAppContainer implements JUMPMessageHandler {
    
    /**
     * Creates a new instance of JUMPAppContainer
     */
    protected  JUMPAppContainer() {
    }
    
    /**
     * Start the application specific by the class name.
     */
    public abstract int startApp(String className, String[] args);
    
    public abstract void pauseApp(int appId);
    
    public abstract void resumeApp(int appId);
    
    public abstract void destroyApp(int appId, boolean force);
    
    public abstract JUMPWindow getAppWindows(int appId);
    
    public void handleMessage(JUMPMessage message) {
        // call the methods by unpacking the message contents
    }
}
