/*
 * @(#)XletContextImpl.java	1.27 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */

package com.sun.xlet;

// Provides graphic representations for xlets.
import java.awt.Container;

// Standard classes used by xlets.
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.XletStateChangeException;

public class XletContextImpl implements XletContext {

    // The xlet class name.
    String mainClass;  

    // Optional runtime arguments passed to this xlet from the command line.
    String[] args; 

    // An instance of XletManager. It manages the xlet that
    // this XletContext belongs to.
    XletManager manager;  

    // An AWT container. The xlet can draw on it.
    Container container = null;  

    public XletContextImpl(String xletName, String[] args, XletManager manager) {
        this.mainClass = xletName;
        this.args = args;
        this.manager = manager;
    }

    // Returns the xlet properties, which were passed to the xlet with the 
    // -args option on the command line.
    public Object getXletProperty(String key) {
        if (key == null) {
            throw new NullPointerException("Key is null");
        } else if (key.equals("")) {
            throw new IllegalArgumentException("Key is an empty String");
        } else if (key.equals(ARGS)) {
            if (args == null) { 
               return new String[]{};
            } else {
               return args;
            }
        } 
        return null;
    }

    // Returns the AWT container that the xlet is using.
    public Container getContainer() {
        if (container == null) {
            container = manager.getContainer();
        }
        return container;
    }

    // Request from the xlet to make itself active. startXlet is called on a
    // different thread than the one used to call resumeRequest.
    public void resumeRequest() {
        // Spawn a different thread.
        new Thread(manager.threadGroup,
            new Runnable() {
                public void run() {
                    manager.handleRequest(XletState.ACTIVE);
                }
            }, "ResumeRequest Thread").start();
        //manager.startXlet(manager);    
    }

    // Notification from the xlet that it is in the paused state.
    public void notifyPaused() {
        manager.setState(XletState.PAUSED);
    }

    // Notification from the xlet that it is in the destroyed state.
    public void notifyDestroyed() {
        manager.setState(XletState.DESTROYED);
    }

    /**
     * Returns the base class loader of the Xlet.  This class loader
     * will be dedicated exclusively to the xlet.  The xlet's classes are
     * all loaded by this classloader or by a classloader that has this
     * classloader as its ancestor.
     */
    public java.lang.ClassLoader getClassLoader() {
        return XletManager.getXletClassLoader((XletContext)this);
    }

}
