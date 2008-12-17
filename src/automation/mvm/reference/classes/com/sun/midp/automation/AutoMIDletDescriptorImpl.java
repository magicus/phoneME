/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.automation;

import com.sun.midp.midletsuite.*;
import com.sun.midp.main.*;

/**
 *  AutoMIDletDescriptor interface implementation.
 */
final class AutoMIDletDescriptorImpl implements AutoMIDletDescriptor {
    /** MIDlet info */
    private MIDletInfo midletInfo;
    
    /** MIDlet's suite */
    private AutoSuiteDescriptorImpl suite;

    /** If MIDlet corresponding to this descriptor is started */
    boolean isStarted;

    /** Lock used when starting MIDlet */
    Object midletStartLock;

    /**
     * Constructor
     *
     * @param suite MIDlet's suite
     * @param midletAttr MIDlet's attributes
     */
    AutoMIDletDescriptorImpl(AutoSuiteDescriptorImpl suite, 
            String midletAttr) {
        this.suite = suite;
        this.midletInfo = new MIDletInfo(midletAttr);
        this.midletStartLock = new Object();
        this.isStarted = false;
    }

    /**
     * Constructor
     *
     * @param suite MIDlet's suite
     * @param midletName MIDlet's name
     * @param midletClassName MIDlet's class name
     */
    AutoMIDletDescriptorImpl(AutoSuiteDescriptorImpl suite, 
            String midletName, String midletClassName) {
        this.suite = suite;
        this.midletInfo = new MIDletInfo(midletName, null, midletClassName);
    }

    
    /**
     * Called when MIDlet corresponding to this suite gets destroyed
     */
    void midletDestroyed() {
        synchronized (midletStartLock) {
            isStarted = false;
        }
    }

    /**
     * AutoMIDletDescriptor implementation
     */
    
    /**
     * Gets MIDlet's suite descriptor.
     *
     * @return AutoMIDletSuiteDescriptor representing MIDlet's suite
     */
    public AutoSuiteDescriptor getSuiteDescriptor() {
        return suite;
    }
    
    /**
     * Gets MIDlet's name.
     *
     * @return MIDlet's name as specified in jad/jar file
     */
    public String getMIDletName() {
        suite.guaranteeSuiteValid("getMIDletName");

        return midletInfo.name;
    }

    /**
     * Gets name of the class implementing MIDlet interface
     * for this MIDlet.
     *  
     * @return name of the class implementing MIDlet interface 
     * as specified in jad/jar file
     */
    public String getMIDletClassName() {
        suite.guaranteeSuiteValid("getMIDletClassName");

        return midletInfo.classname;
    }

    /**
     * Starts this MIDlet.
     *
     * @param args MIDlet's arguments
     * @throws throws RuntimeException if MIDlet couldn't be started
     * @return AutoMIDlet representing started MIDlet
     */
    public AutoMIDlet start(String[] args) {
        suite.guaranteeSuiteValid("start");

        AutoMIDlet midlet = null;
        synchronized (midletStartLock) {
            if (isStarted) {
                throw new RuntimeException("MIDlet is already started");
            }
            
            AutoMIDletStateController stateController = 
                AutoMIDletStateController.getMIDletStateController();

            midlet = stateController.startMIDlet(this, args);
            isStarted = true;
        }

        return midlet;
    }
}
