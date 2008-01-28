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

package com.sun.satsa.jcrmic.classfile.constants;

import java.io.DataInputStream;
import java.io.IOException;

/**
 *  This class represents a symbolic reference to a class or interface.
 */

public class JConstantClass extends JConstant {

    /**
     * Index of class name constant pool entry.
     */
    private int name_index;

    /**
     * Class name
     */
    private String name;

    /**
     * Returns class name.
     * @return class name
     */
    public String getClassName() {

        resolve();
        return name;
    }

    /**
     * Constructor.
     * @param cp constant pool reference
     */
    public JConstantClass(JConstantPool cp) {
        super(cp);
    }

    /**
     * Resolves the constant pool entry.
     */
    private void resolve() {

        if (! resolved) {
	        name = cp.getConstantUtf8(name_index).getString();
    	    resolved = true;
        }
    }

    /**
     * Parses constant pool entry.
     * @param dis input stream
     * @throws IOException if I/O exception occurs
     */
    public void parse(DataInputStream dis) throws IOException {
        name_index = dis.readUnsignedShort();
    }
}
