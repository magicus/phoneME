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
 *  This class represents a symbolic reference to a class method.
 */

public class JConstantMethodRef extends JConstant {

    /**
     * Class constant pool entry index.
     */
    private int class_index;

    /**
     * Method name and type constant pool entry index.
     */
    private int nameAndType_index;

    /**
     * Class name.
     */
    protected String class_name;

    /**
     * Method name.
     */
    protected String method_name;

    /**
     * Method descriptor.
     */
    protected String descriptor;

    /**
     * Returns class name.
     * @return class name
     */
    public String getClassName() {
        resolve();
        return class_name;
    }

    /**
     * Returns method name.
     * @return method name
     */
    public String getMethodName() {
        resolve();
        return method_name;
    }

    /**
     * Returns method descriptor.
     * @return method descriptor
     */
    public String getDescriptor() {
        resolve();
        return descriptor;
    }

    /**
     * Constructor.
     * @param cp constant pool reference
     */
    public JConstantMethodRef(JConstantPool cp) {
        super(cp);
    }

    /**
     * Resolves the constant pool entry.
     */
    private void resolve() {

        if (! resolved) {

	        class_name = cp.getConstantClass(class_index).getClassName();

            JConstantNameAndType nameAndTypeConstant =
                    cp.getConstantNameAndType(nameAndType_index);

            method_name = nameAndTypeConstant.getName();
            descriptor = nameAndTypeConstant.getDescriptor();

            resolved = true;
        }
    }

    /**
     * Parses constant pool entry.
     * @param dis input stream
     * @throws IOException if I/O exception occurs
     */
    public void parse(DataInputStream dis) throws IOException {
        class_index = dis.readUnsignedShort();
        nameAndType_index = dis.readUnsignedShort();
    }
}
