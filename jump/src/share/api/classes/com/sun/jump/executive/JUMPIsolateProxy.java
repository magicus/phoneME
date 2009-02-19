/*
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
import com.sun.jump.common.JUMPWindow;
import com.sun.jump.message.JUMPMessageSender;

/**
 * <code>JUMPIsolateProxy</code> encapsulates the state of the running isolate
 * within the executive.
 * Operations performed on the <code>JUMPIsolateProxy</code>
 * gets sent to the isolate instance (For example :- In the case of
 * process based isolate,
 * to <code>com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess</code>
 * <p>
 * It implements  the {@link com.sun.jump.message.JUMPMessageSender}
 * interface so that messages can be sent to the <code>JUMPIsolate</code>
 * that this class is proxying for.
 * <p>
 * The <Code>JUMPLifecycleModule</code> is responsible for creation and
 * management of <Code>JUMPIsolateProxy</code> objects.
 */
public interface JUMPIsolateProxy extends JUMPIsolate, JUMPMessageSender {
    
    /**
     * Start the application specified. The method returns
     * after the application has started successfully. If for some reason
     * the application cannot be started exceptions are thrown.
     *
     * @return JUMPApplicationProxy the proxy to the started application on the Isolate.
     */
    public JUMPApplicationProxy startApp(JUMPApplication app, String[] args);
    
    /**
     * Get all application proxies
     */
    public JUMPApplicationProxy[] getApps();
    
    /**
     * Get all <code>JUMPWindow</code>-s running in the <code>JUMPIsolateProxy</code>.
     */
    public JUMPWindow[] getWindows();
}
