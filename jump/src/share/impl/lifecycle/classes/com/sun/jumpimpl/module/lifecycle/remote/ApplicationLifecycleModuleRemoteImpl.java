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

import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModuleFactory;
import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModule;

import java.rmi.Remote;
import java.rmi.RemoteException;

/**
 * <code>JUMPApplicationLifecycleModuleRemote</code> is an remote module
 * to the <code>JUMPApplicationLifecycleModule</code> module in the executive,
 * that performs application lifecycle operations.
 */
public class ApplicationLifecycleModuleRemoteImpl 
    implements JUMPApplicationLifecycleModuleRemote {

    JUMPApplicationLifecycleModule lifecycleModule;

    public ApplicationLifecycleModuleRemoteImpl(
		    JUMPApplicationLifecycleModule lifecycleModule) {

       this.lifecycleModule = lifecycleModule;

       try {
          JUMPServiceRegistryModule registry = 
		  JUMPServiceRegistryModuleFactory.getInstance().getModule();

          registry.registerService(
               JUMPApplicationLifecycleModuleRemote.class.getName(),
	       this);
       } catch (java.rmi.RemoteException e) { // shouldn't happen
          e.printStackTrace();
       } catch (java.rmi.AlreadyBoundException e) { // 
          System.err.println(JUMPApplicationLifecycleModuleRemote.class.getName() + " already bound " + e);
       }
    } 

    /**
     * Launch or return a running application for an installed
     * application, according to the lifecycle policy for this
     * lifecycle module.
     */
    public JUMPApplicationProxyRemote launchApplication(JUMPApplication app,
            String args[]) throws RemoteException {

       JUMPApplicationProxy proxy = lifecycleModule.launchApplication(app, args);

       if (proxy == null) {
           System.err.println("Launching failed, " + app);
           return null;
       } 
	     
       return new ApplicationProxyRemoteImpl(proxy);
    }
}
