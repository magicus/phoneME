/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.tck.xlet;

import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import java.rmi.RemoteException;
import java.rmi.NotBoundException;

import com.sun.xlet.mvmixc.*;

public class CDCAmsXletManagerImpl implements XletManager {
    XletLauncherInterface launcher = null;

    public CDCAmsXletManagerImpl() {
       try {
          IxcRegistry registry = IxcRegistry.getRegistry(new CDCAmsXletContext());
          launcher = (XletLauncherInterface) registry.lookup("cdcams/XletLauncher");
       } catch (NotBoundException nbe) { 
          System.err.println("WARNING: Xlet Launching Object not found in the IxcRegistry");
       } catch (Exception e) { e.printStackTrace(); }
    }
    
    public void init(String[] args) {}
    
    public ManageableXlet loadXlet(String className, 
                                   String location, String[] args) { 
       
       try {
          if (launcher != null) {
             XletLifecycleInterface handler = 
                launcher.launchXlet(className, new String[]{location}, args);
 
             if (handler != null) {
                return new ManageableXletImpl(handler);
             }
          }
       } catch (Exception e) { e.printStackTrace(); }
       
       return null;
    }
}                    

class ManageableXletImpl implements ManageableXlet {

    protected XletLifecycleInterface handler;

    ManageableXletImpl(XletLifecycleInterface handler) {
        this.handler = handler;
    }

    public void initXlet() {
        try {
           handler.postInitXlet();
        } catch (RemoteException e) { e.printStackTrace(); }
    }
    
    public void startXlet() {
        try {
           handler.postStartXlet();
        } catch (RemoteException e) { e.printStackTrace(); }
    }
    
    public void pauseXlet() {
        try {
           handler.postPauseXlet();
        } catch (RemoteException e) { e.printStackTrace(); }
    }
    
    public void destroyXlet(boolean unconditional) {
        try {
           handler.postDestroyXlet(unconditional);
        } catch (RemoteException e) { 
           // This is expected - we're destroying the xlet in the middle of ixc.
        }
    }
    
    public int getState() {
        int state = UNKNOWN;
        try {
           state = handler.getState();
        } catch (RemoteException e) { e.printStackTrace(); }

        return state;
    }    
}


