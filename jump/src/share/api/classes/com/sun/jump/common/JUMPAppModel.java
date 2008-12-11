/*
 * %W% %E%
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

package com.sun.jump.common;

import java.io.Serializable;

/**
 * <code>JUMPAppModel</code> enumerates the application models supported
 * by JUMP.
 */
public class JUMPAppModel implements Serializable {
    public static final JUMPAppModel XLET   = new JUMPAppModel("xlet");
    public static final JUMPAppModel MIDLET = new JUMPAppModel("midlet");
    public static final JUMPAppModel MAIN   = new JUMPAppModel("main");
    
    private String model;
    
    /**
     * Creates a new instance of JUMPAppModel
     */
    private JUMPAppModel(String model) {
        this.model = model;
    }
    
    public String getName() {
	return this.model;
    }
    
    public static JUMPAppModel fromName(String name) {
	if (name.equals("xlet")) {
	    return XLET;
	} else if (name.equals("midlet")) {
	    return MIDLET;
	} else if (name.equals("main")) {
	    return MAIN;
	} else {
	    return null;
	}
    }
    
    public String toString(){
        return this.model;
    }
}
