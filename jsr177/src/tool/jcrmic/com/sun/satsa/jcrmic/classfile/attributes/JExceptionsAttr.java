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
 *  This class represents Exceptions attribute in the method_info structure.
 *  The Exception attribute indicates which checked exceptions a method
 *  may throw
 */

public class JExceptionsAttr extends JAttribute {

    /**
     * Indexes of exception class names.
     */
    private int[] index_table;

    /**
     * Exception class names.
     */
    private String[] exceptions;

    /**
     * Returns exception class names.
     * @return exception class names
     */
    public String[] getExceptions() {
        return exceptions;
    }

    /**
     * Constructor.
     * @param cp constant pool reference.
     */
    public JExceptionsAttr(JConstantPool cp) {
        super(cp);
    }

    /**
     * Parses the attribute definition.
     * @param input input stream
     * @throws IOException if I/O exception occurs
     */
    public void parse(DataInputStream input) throws IOException {

        int length = input.readInt();
        index_table = new int[input.readUnsignedShort()];
        for (int i = 0; i < index_table.length; i++) {
            index_table[i] = input.readUnsignedShort();
        }
    }

    /**
     * Resolves the attribute.
     */
    public void resolve() {
        exceptions = new String[index_table.length];
        for (int i = 0; i < index_table.length; i++) {
            exceptions[i] = cp.getConstantClass(index_table[i]).getClassName();
        }
    }
}
