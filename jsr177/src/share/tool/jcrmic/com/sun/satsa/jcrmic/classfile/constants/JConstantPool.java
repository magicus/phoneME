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

import java.io.*;

/**
 *  This class represents constant pool.
 *  It defines a set of get and add methods to search or add a constant.
 */

public class JConstantPool {

    /**
     * Constant ppol entries.
     */
    private JConstant[] constants;

    /**
     * Constructor.
     * @param size constant pool size.
     */
    public JConstantPool(int size) {
        constants = new JConstant[size];
    }

    /**
     * Returns constant pool entry.
     * @param index entry index
     * @return the entry
     */
    public JConstant getAt(int index) {

        return constants[index];
    }

    /**
     * Returns constant.
     * @param index entry index
     * @return the entry
     */
    public JConstantUtf8 getConstantUtf8(int index) {
        JConstant constant = getAt(index);

        if (constant instanceof JConstantUtf8)
            return (JConstantUtf8) constant;
        else
            return null;
    }

    /**
     * Returns constant.
     * @param index entry index
     * @return the entry
     */
    public JConstantClass getConstantClass(int index) {
        JConstant constant = getAt(index);

        if (constant instanceof JConstantClass)
            return (JConstantClass) constant;
        else
            return null;
    }

    /**
     * Returns constant.
     * @param index entry index
     * @return the entry
     */
    public JConstantNameAndType getConstantNameAndType(int index) {
        JConstant constant = getAt(index);

        if (constant instanceof JConstantNameAndType)
            return (JConstantNameAndType) constant;
        else
            return null;
    }

    /**
     * Parses the constant pool.
     * @param dis input stream
     * @throws IOException if I/O exception occurs
     */
    public void parse(DataInputStream dis) throws IOException {
        // note:  constants start at index 1

        for (int i = 1; i < constants.length; i++) {
            int tag = dis.readUnsignedByte();

            constants[i] = JConstant.create(tag, this);
            constants[i].parse(dis);


            // If a Constant_Long_info or Constant_Double_info
            // structure is the item in the constant pool table
            // at index n, then the next index n+1 must be
            // considered invalid and must not be used
            if ((tag == JConstant.CONSTANT_DOUBLE) ||
                    (tag == JConstant.CONSTANT_LONG)) {
                i++;
            }
        }
    }
}
