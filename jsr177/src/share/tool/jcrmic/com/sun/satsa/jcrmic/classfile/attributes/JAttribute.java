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

package com.sun.satsa.jcrmic.classfile.attributes;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintWriter;

import com.sun.satsa.jcrmic.classfile.constants.JConstantPool;

/**
 * This class is the base class for all Java class file attributes.
 * It also contain a factory method to create an attribute object
 * based on the attribute type.
 */

public class JAttribute {

    /**
     * Constant pool reference.
     */
    protected JConstantPool cp;

    /**
     * Constructor.
     * @param cp constant pool reference
     */
    JAttribute(JConstantPool cp) {
        this.cp = cp;
    }

    /**
     * Factory method for attribute object creation.
     * @param cp constant pool reference
     * @param index constant pool index for attribute name
     * @return attribute object
     */
    public static final JAttribute create(JConstantPool cp, int index) {

        String name = cp.getConstantUtf8(index).getString();
        if (name.equals("Exceptions"))
            return new JExceptionsAttr(cp);
        return new JAttribute(cp);
    }

    /**
     * Resolves the attribute.
     */
    public void resolve() {
    }

    /**
     * Parses the attribute definition.
     * @param dis input stream
     * @throws IOException if I/O exception occurs
     */
    public void parse(DataInputStream dis) throws IOException {
        int length = dis.readInt();
        dis.skipBytes(length);
    }
}
