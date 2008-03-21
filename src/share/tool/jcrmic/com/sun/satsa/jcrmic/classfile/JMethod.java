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

package com.sun.satsa.jcrmic.classfile;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.Modifier;

import com.sun.satsa.jcrmic.classfile.constants.JConstantPool;
import com.sun.satsa.jcrmic.classfile.attributes.*;

/**
 *  This class represents a method.
 */
public class JMethod {

    /**
     * Method name.
     */
    private String method_name;

    /**
     * Method descriptor.
     */
    private String descriptor;

    /**
     * Method access flags.
     */
    private int access_flags;

    /**
     * Constant pool reference.
     */
    private JConstantPool constant_pool;

    /**
     * The names of exceptions declared in the throw clause
     */
    private String[] exceptionsThrown;

    /**
     * Constructor.
     * @param constant_pool constant pool reference
     */
    public JMethod(JConstantPool constant_pool) {
        this.constant_pool = constant_pool;
    }

    /**
     * Returns the list of names of exceptions declared in the throw clause.
     * @return the list of names of exceptions declared in the throw clause
     */
    public String[] getExceptionsThrown() {
        return exceptionsThrown;
    }

    /**
     * Returns method name.
     * @return method name
     */
    public String getMethodName() {
        return method_name;
    }

    /**
     * Returns method descriptor.
     * @return method descriptor
     */
    public String getMethodDescriptor() {
        return descriptor;
    }

    /**
     * Parse and resolve Java method.
     * @param dis input sream
     * @throws IOException if I/O error occurs
     */
    public void parse(DataInputStream dis) throws IOException {

        access_flags = dis.readUnsignedShort();
        int name_index = dis.readUnsignedShort();
        int descriptor_index = dis.readUnsignedShort();
        int attribute_count = dis.readUnsignedShort();
        for (int i = 0; i < attribute_count; i++) {
            int index = dis.readUnsignedShort();
            JAttribute attribute = JAttribute.create(constant_pool, index);
            attribute.parse(dis);
            if (attribute instanceof JExceptionsAttr) {
                attribute.resolve();
                exceptionsThrown = ((JExceptionsAttr)
                                        attribute).getExceptions();
            }
        }

        method_name = constant_pool.getConstantUtf8(name_index).getString();
        descriptor = constant_pool.getConstantUtf8(
                                        descriptor_index).getString();
    }
}
