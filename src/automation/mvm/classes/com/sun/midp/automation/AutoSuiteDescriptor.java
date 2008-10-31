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

import java.util.*;
import java.io.*;

/**
 *  Represents MIDlets suite.
 */
public abstract class AutoSuiteDescriptor {
    /**
     * Gets name of the suite.
     *
     * @return name of the suite as specified in jar/jad
     */
    public abstract String getSuiteName()
        throws IllegalStateException;

    /**
     * Gets MIDlet which should be started by default for this
     * suite, if any.
     *
     * @return AutoMIDletDescriptor representing default
     * MIDlet
     */
    public abstract AutoMIDletDescriptor getInitialMIDlet()
        throws IllegalStateException;
   
    /**
     * Gets suite's MIDlets.
     *
     * @return vector of AutoMIDletDescriptor objects representing 
     * suite's MIDlets, 
     * null if there is no default MIDlet for this suite
     */
    public abstract Vector getSuiteMIDlets() 
        throws IllegalStateException;

    /**
     * Starts this suite's initial MIDlet.
     *
     * @param args MIDlet's arguments
     * @return AutoMIDlet representing started MIDlet
     * @throws RuntimeException if MIDlet couldn't be started
     */
    abstract public AutoMIDlet start(String[] args);

    /**
     * Gets descriptor of internal suite
     *
     * @param className suite's MIDlet class name
     * @return internal suite descriptor
     */
    public final static AutoSuiteDescriptor getInternalSuite(String className)
        throws IOException {
        return AutoSuiteDescriptorImpl.getInstanceByClassName(className);
    }
}
