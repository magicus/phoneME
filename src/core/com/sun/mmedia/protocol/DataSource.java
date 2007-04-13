/*
 * 
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

package com.sun.mmedia.protocol;

import javax.microedition.media.Controllable;
import javax.microedition.media.Control;
import java.io.IOException;

abstract public class DataSource implements Controllable {
    private String sourceLocator;
    
    public DataSource(String locator) {
	sourceLocator = locator;
    }

    public String getLocator() {
	return sourceLocator;
    }
    
    public abstract String getContentType();
    
    public abstract void connect() throws IOException; 

    public abstract void disconnect();

    public abstract void start() throws IOException;

    public abstract void stop() throws IOException;

    public abstract SourceStream[] getStreams();

    
    // These two methods are declared here (Controllable interface)
    // because of a feature in KVM/VM when code is compiled with JDK 1.4
    // Need to declare these abstract

    public abstract Control [] getControls();

    public abstract Control getControl(String controlType);
    
}
