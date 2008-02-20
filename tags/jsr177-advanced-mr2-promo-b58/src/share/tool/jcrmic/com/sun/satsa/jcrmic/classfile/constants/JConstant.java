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

import java.io.IOException;
import java.io.DataInputStream;

/**
 * This class is the base class for all constants.
 * It also defines a factory method for creating a constant based
 * on the constant type
 */

abstract public class JConstant {

    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_UTF8 = 1;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_INTEGER = 3;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_FLOAT = 4;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_LONG = 5;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_DOUBLE = 6;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_CLASS = 7;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_STRING = 8;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_FIELDREF = 9;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_METHODREF = 10;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_INTERFACE_METHODREF = 11;
    /**
     * Constant pool entry tag.
     */
    public static final int CONSTANT_NAME_AND_TYPE = 12;

    /**
     * Constant pool reference.
     */
    protected JConstantPool cp;

    /**
     * Constructor.
     * @param cp constant pool reference
     */
    public JConstant(JConstantPool cp) {
        this.cp = cp;
    }

    /**
     * 'Resolved' flag.
     */
    protected boolean resolved = false;

    /**
     * Factory method for constant pool entry creation.
     * @param tag constant pool entry type tag
     * @param cp constant pool reference
     * @return constant pool entry object
     */
    public static final JConstant create(int tag, JConstantPool cp) {

        switch (tag) {
            case CONSTANT_CLASS:
                return new JConstantClass(cp);
            case CONSTANT_UTF8:
                return new JConstantUtf8(cp);
            case CONSTANT_FIELDREF:
                return new JConstantFieldRef(cp);
            case CONSTANT_METHODREF:
                return new JConstantMethodRef(cp);
            case CONSTANT_INTERFACE_METHODREF:
                return new JConstantInterfaceMethodRef(cp);
            case CONSTANT_NAME_AND_TYPE:
                return new JConstantNameAndType(cp);
            case CONSTANT_STRING:
                return new JConstantString(cp);
            case CONSTANT_INTEGER:
                return new JConstantInteger(cp);
            case CONSTANT_FLOAT:
                return new JConstantFloat(cp);
            case CONSTANT_LONG:
                return new JConstantLong(cp);
            case CONSTANT_DOUBLE:
                return new JConstantDouble(cp);
            default:
                return null;

        }
    }

    /**
     * Parses constant pool entry.
     * @param dis input stream
     * @throws IOException if I/O exception occurs
     */
    abstract public void parse(DataInputStream dis) throws IOException;
}
