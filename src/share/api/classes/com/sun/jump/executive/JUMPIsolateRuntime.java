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

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.messagequeue.JUMPMessage;
import com.sun.jump.messagequeue.JUMPOutgoingQueue;
import com.sun.jump.messagequeue.JUMPTimedOutException;
import java.io.IOException;

/**
 * <code>JUMPIsolateRuntime</code> encapsulates the state of the running isolate
 * within the executive. 
 * Operations performed on the <code>JUMPIsolateRuntime</code>
 * gets sent to the isolate instance (For example :- In the case of 
 * process based isolate, 
 * to <code>com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess</code>
 * <p>
 * The <Code>JUMPLifecycleModule</code> is responsible for creation and 
 * management of <Code>JUMPIsolateRuntime</code> objects.
 */
public abstract class JUMPIsolateRuntime implements JUMPIsolate {
    /**
     * Returns the state of the Isolate.
     */
    public abstract int getState();
    
    /**
     * Start the application specified. The method returns
     * after the application has started successfully. If for some reason
     * the application cannot be started exceptions are thrown.
     *
     * @return a unique ID identifying the application on the Isolate.
     */
    public abstract int startApp(JUMPApplication app, String[] args);
    
    public abstract void pauseApp(int appId);
    
    public abstract void resumeApp(int appId);
    
    public abstract void destroyApp(int appId);
    
    /**
     * Send a message to the Isolate instance.
     * The method is responsible for serializing multiple messages send to
     * the isolate instance.
     */
    public synchronized void sendMessage(JUMPMessage message) 
        throws IOException {
        JUMPExecutive sysExecutive = JUMPExecutive.getInstance();
        JUMPOutgoingQueue outbox = sysExecutive.getOutgoingQueue();
        outbox.sendMessage(this, message);
    }
    
    /**
     * Send a message to the Isolate instance and wait for a response 
     * message.
     * The method is responsible for serializing multiple messages send to
     * the isolate instance.
     *
     * @param message the message to be sent
     * @param timeout the time in milliseconds to wait, if there was no
     *        response.
     */
    public synchronized JUMPMessage sendMessage(JUMPMessage message, long timeout)
        throws JUMPTimedOutException, IOException {
        return null; 
    }
}
