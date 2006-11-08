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

/**
 * This interface defines a way for an application to introduce xlets
 * into the system. The further control of an xlet is performed via 
 * the ManageableXlet interface.
 * 
 * Licensees need to supply the name of the implementation class
 * and its arguments to the AgentXlet. The format is
 * "-xletManager <class name> [arguments]".
 * The no-argument constructor will be called to instantiate 
 * the specified class, immediately followed by a call to 
 * init(String[] args) to initialize the instance with
 * the specified arguments (if any).
 */
public interface XletManager {

    /**
     * Request the Xlet manager to load an xlet.
     * The xlet will be in the loaded state once this method returns
     * successfully.
     * 
     * @param className  class name of the xlet
     * @param location  location of the xlet classes
     * @param args       initialization arguments for the xlet
     * @return  an instance of ManageableXlet, which can be used to
     *          trigger xlet state changes, or null if the constructor
     *          of the Xlet did not return successfully.
     *
     * @throws ClassNotFoundException if className cannot be found
     *           in the given location
     * 
     */     
    public ManageableXlet loadXlet(String className, String location, String[] args)
        throws ClassNotFoundException; 
        
    /**
     * Initialize the Xlet manager. This method will be called
     * immediately after the no-argument constructor is called.
     *
     * @param args  Arguments used in initializing the Xlet manager.
     */     
    public void init(String[] args);
}
