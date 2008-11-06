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

package com.sun.jump.command;

import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageReader;
import com.sun.jump.message.JUMPOutgoingMessage;

/**
 * <code>JUMPIsolateLifecycleRequest</code> defines all the lifecycle 
 * related requests that originate from the <Code>JUMPIsolate</code>
 */
public class JUMPIsolateLifecycleRequest extends JUMPRequest {
    private int isolateId;
    private int appId;

    /* FIXME: do these state constants belong here, or the lifecycle module? */
    /**
     * Newly created isolate in state ISOLATE_STATE_CREATED
     */
    public static final int ISOLATE_STATE_CREATED = 1;

    /**
     * Initialized isolate in state ISOLATE_STATE_INITIALIZED
     */
    public static final int ISOLATE_STATE_INITIALIZED = 2;

    /**
     * Isolate running at least one application
     */
    public static final int ISOLATE_STATE_RUNNING = 3;
    
    /**
     * Isolate has been destroyed 
     */
    public static final int ISOLATE_STATE_DESTROYED = 4;
    
    /**
     * Application requesting the executive to pause itself.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     *   <li>args[1] - Application Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_APP_REQUEST_PAUSE= "AppRequestPause";
    
    /**
     * Application requesting the executive to resume itself.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     *   <li>args[1] - Application Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_APP_REQUEST_RESUME = "AppRequestResume";
    
    /**
     * Application indicating that it has paused itself.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     *   <li>args[1] - Application Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_APP_PAUSED = "AppPaused";
    
    /**
     * Application indicating that it has paused itself.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     *   <li>args[1] - Application Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_APP_RESUMED = "AppResumed";
    
    /**
     * Isolate Initialized.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_ISOLATE_INITIALIZED = "IsolateInitialized";
    
    /**
     * Death of an Isolate.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_ISOLATE_DESTROYED = "IsolateDestroyed";

    public static final String MESSAGE_TYPE = "mvm/lifecycle";
    
    JUMPIsolateLifecycleRequest() {
	super(MESSAGE_TYPE, null, null);
    }
	
    private JUMPIsolateLifecycleRequest(String id) {
	super(MESSAGE_TYPE, id, null);
    }

    public JUMPIsolateLifecycleRequest(String id,
				       JUMPIsolate isolate,
				       int appId) {
	this(id);
	this.isolateId = isolate.getIsolateId();
	this.appId = appId;
    }

    public JUMPIsolateLifecycleRequest(String id,
				       JUMPIsolate isolate) {
	this(id, isolate, -1);
    }

    public int getIsolateId() {
	return isolateId;
    }
    
    public int getAppId() {
        return appId;
    }
    
    /** 
     * For subclasses to use to initialize any fields
     * using <code>JUMPMessage.get*</code> methods.
     */
    protected void deserializeFrom(JUMPMessageReader message) {
	// First deserialize any shared fields
	super.deserializeFrom(message);
	// And now window request specific fields
	this.isolateId = message.getInt();
	this.appId = message.getInt();
    }
    
    /** 
     * For subclasses to use to put data in a message
     * using <code>JUMPOutgoingMessage.add*</code> methods.
     */
    protected void serializeInto(JUMPOutgoingMessage message) {
	super.serializeInto(message);
	// And now window request specific fields
	message.addInt(this.isolateId);
	message.addInt(this.appId);
    }
    
    public static JUMPCommand fromMessage(JUMPMessage message) {
	return JUMPCommand.fromMessage(message,
				       JUMPIsolateLifecycleRequest.class);
    }

}
