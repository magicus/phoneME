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

package com.sun.jumpimpl.module.multimedia;

import java.util.Map;
import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;
import com.sun.jump.module.JUMPModule;
import com.sun.jump.os.JUMPOSInterface;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.command.JUMPRequest;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.executive.JUMPIsolateFactory;
import com.sun.jump.module.eventqueue.JUMPEventQueueModuleFactory;
import com.sun.jump.module.eventqueue.JUMPEventQueueModule;
import com.sun.jump.module.eventqueue.JUMPEventHandler;

/**
 * Implementation of Multimedia module.
 * This class performs the following activities:
 * <ul>
 *  <li>starts native file system monitoring;</li>
 *  <li>listens to multimedia events from the platform;</li>
 *  <li>notifies related isolates via JUMP messages.</li>
 * </ul>
 */
public class MultimediaModuleImpl implements JUMPModule, JUMPEventHandler {
    /** Message type for Multimedia events. */
    public static final String MESSAGE_TYPE = "mvm/multimedia";

    /** Message ID for notifications from Multimedia monitor. */
    public static final String MM_EVENT = "mm_event";

    /** Executive process for messaging. */
    private JUMPExecutive thisProcess;

    /** Message handler registration token. */
    Object msgToken;

    /** Isolate manager for getting <code>JUMPIsolateProxy</code> instances. */
    JUMPIsolateFactory isolateFactory;

    /**
     * Loads the Multimedia module. Performs necessary initialization to start
     * monitoring Multimedia events.
     *
     * @param config configuration data (ignored).
     */
    public void load(Map config) {
        // Register this module as event handler for JSR-75.
        JUMPEventQueueModule eventQueue = JUMPEventQueueModuleFactory.getInstance().getModule();
        eventQueue.registerEventHandler(135, this);
        // Get executive instance.
        thisProcess = JUMPExecutive.getInstance();
    }

    /**
     * Unloads the Multimedia module.
     */
    public void unload() {
    }
    /**
     * Handles incoming events from event queue module. All native events
     * related to Multimedia should be delivered here.
     *
     * @param id event identifier.
     * @param data event data.
     */
    public void handleEvent(int id, byte[] data) {
        // Parse event data to get IsolateId
        int isolateId = id;
        if (isolateFactory == null) {
            isolateFactory = thisProcess.getIsolateFactory();
        }

        JUMPIsolateProxy ip=isolateFactory.getIsolate(isolateId);
        // Pre-create message for isolate notifications.
        JUMPRequest eventCmd = new JUMPRequest(MESSAGE_TYPE, MM_EVENT);
        JUMPOutgoingMessage notification = eventCmd.toMessage(thisProcess);

        notification.addByteArray(data);
        try {
            ip.sendMessage(notification);
        } catch (IOException ex) {
            System.out.println("==> Can not send MMEvent: " + ex.getMessage());
        }
    }

}

