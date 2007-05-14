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

package com.sun.jumpimpl.module.lifecycle;

import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.executive.JUMPIsolateFactory;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModule;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.executive.JUMPApplicationProxy;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

public class OneLiveOnlyApplicationLifecycleModule
        implements JUMPApplicationLifecycleModule {
    
    private final HashMap mapAppToAppProxy = new HashMap();
    
    //
    // This is called on first use -- extract any configuration
    // information here.
    // Examples could be -- maximum number of instances allowed, etc.
    //
    public void load(Map config) {
    }
    
    public void unload() {
        mapAppToAppProxy.clear();
    }
    
    /*
     * Launch or return a running application for an installed
     * application, according to the lifecycle policy for this
     * lifecycle module.
     */
    public JUMPApplicationProxy launchApplication(JUMPApplication app, String args[]) {
        // This implementation allows for one live instance only.
        // Either create it, or return existing instance.
        synchronized(this) {
            
            if (app == null) {
                return null;
            }
            
            // launch or return existing proxy
            // Maintain map of
            // JUMPApplication to existing running
            // JUMPApplicationProxy.
            //
            JUMPApplicationProxy proxy =
                    (JUMPApplicationProxy)mapAppToAppProxy.get(app);
            
            if (proxy == null) {
                // Get a handle to the isolate manager.
                JUMPIsolateFactory ism = JUMPExecutive.getInstance().getIsolateFactory();
                
                // Create isolate, which returns JUMPIsolateProxy
                JUMPIsolateProxy ip = ism.newIsolate(app.getAppType());
                
                // Create JUMPApplicationProxy by calling startApp()
                // on the JUMPIsolateProxy
                proxy = ip.startApp(app, args);
                
                // ...
                // And put returned app proxy in map
                mapAppToAppProxy.put(app, proxy);
            }
            
            return proxy;
        }
    }
    
    /**
     * Returns any running instances of a given JUMPApplication.
     * Can be found by iterating over mapAppToAppProxy.
     */
    public JUMPApplicationProxy[] getApplications(JUMPApplication app) {
        JUMPApplicationProxy proxy[] = new JUMPApplicationProxy[1];
        proxy[0] = (JUMPApplicationProxy)mapAppToAppProxy.get(app);
        if (proxy[0] == null) {
            return null;
        } else {
            return proxy;
        }
    }
    
    
    /**
     * Returns any running application instances for this lifecycle module
     * Can be found by iterating over mapAppToAppProxy.
     */
    public JUMPApplicationProxy[] getApplications() {
        Collection v =  mapAppToAppProxy.values();
        if (v == null) {
            return null;
        } else {
            return (JUMPApplicationProxy[])v.toArray(new JUMPApplicationProxy[]{});
        }
    }
}

