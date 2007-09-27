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

package com.sun.jumpimpl.module.lifecycle.remote;

import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.module.JUMPModule;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.common.JUMPApplication;

import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModule;
import com.sun.jump.module.lifecycle.remote.JUMPApplicationLifecycleModuleRemote;
import com.sun.jump.module.lifecycle.remote.JUMPApplicationProxyRemote;

import java.rmi.Remote;
import java.rmi.RemoteException;

/**
 * <code>JUMPApplicationProxyRemote</code> is an remote module
 * to the <code>JUMPApplicationProxy</code> object in the executive.
 * This can be used to control lifecycle operations of an application
 * running in another Isolate.
 */
public class ApplicationProxyRemoteImpl implements JUMPApplicationProxyRemote {

    JUMPApplicationProxy appProxy;

    public ApplicationProxyRemoteImpl(JUMPApplicationProxy appProxy) {
       this.appProxy = appProxy;
    }

    /**
     * Returns the <code>JUMPApplication</code> this proxy is associated with.
     */
    public JUMPApplication getApplication() throws RemoteException {
       return appProxy.getApplication();
    }

    /** 
     * Pauses the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public void pauseApp() throws RemoteException {
       appProxy.pauseApp();
    }

    /** 
     * Resumes the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public void resumeApp() throws RemoteException {
       appProxy.resumeApp();
    }

    /** 
     * Destroys the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public void destroyApp() throws RemoteException {
       appProxy.destroyApp();
    }

    /** 
     * Returns the state of the application associated with this 
     * <code>JUMPApplicationProxy</code>.
     **/
    public int getAppState() throws RemoteException {
       return appProxy.getAppState();
    }
}
