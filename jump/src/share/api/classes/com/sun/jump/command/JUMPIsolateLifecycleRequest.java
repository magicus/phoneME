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

/**
 * <code>JUMPIsolateLifecycleRequest</code> defines all the lifecycle 
 * related requests that originate from the <Code>JUMPIsolate</code>
 */
public interface JUMPIsolateLifecycleRequest {
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
     * Application inidcating that it has paused otself.
     * <ol>
     *   <li>args[0] - Isolate Id</li>
     *   <li>args[1] - Application Id</li>
     * </ol>
     * Asynchronous request. No <code>JUMPResponse</code> required.
     */
    public static final String ID_APP_PAUSED = "AppPaused";
    
    /**
     * Application inidcating that it has paused otself.
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
}
