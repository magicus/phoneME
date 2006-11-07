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
package javax.microedition.media.protocol;

import javax.microedition.media.Controllable;

import javax.microedition.media.Control;

import java.io.IOException;

/**
 * This class is defined by the JSR-135 specification
 * <em>Mobile Media API,
 * Version 1.2.</em>
 */
// JAVADOC COMMENT ELIDED
abstract public class DataSource implements Controllable {
    private String sourceLocator;
    
    // JAVADOC COMMENT ELIDED
    public DataSource(String locator) {
	sourceLocator = locator;
    }

    // JAVADOC COMMENT ELIDED
    public String getLocator() {
	return sourceLocator;
    }
    
    // JAVADOC COMMENT ELIDED
    public abstract String getContentType();
    
    // JAVADOC COMMENT ELIDED
    public abstract void connect() throws IOException; 

    // JAVADOC COMMENT ELIDED
    public abstract void disconnect();

    // JAVADOC COMMENT ELIDED
    public abstract void start() throws IOException;

    // JAVADOC COMMENT ELIDED
    public abstract void stop() throws IOException;

    // JAVADOC COMMENT ELIDED
    public abstract SourceStream[] getStreams();

    
    // These two methods are declared here (Controllable interface)
    // because of a bug in the VM when code is compiled with JDK 1.4
    // Need to declare these abstract

    public abstract Control [] getControls();

    public abstract Control getControl(String controlType);
    
}
