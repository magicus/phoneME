/*
 * @(#)TypeInfo.java	1.11 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */


package com.sun.xlet.ixc;

import java.util.Hashtable;

//
//  This little utility class is used by IxcClassLoader to hold
//  information that's relevant to it about types (particularly
//  primitive types and their wrappers).
//

class TypeInfo {
    String typeDescriptor;	// 'L', 'D', ...
    int localSlots;		// Ints take 1 local slot, doubles & longs 2
    byte loadInstruction;  	// iload, dload, fload, ...
    byte returnInstruction;  	// ireturn, dreturn, ...
    Class primitiveWrapper;	// null for non-primitive
    String valueMethod;		// "intValue(), floatValue(), ..."
    // A Mapping from the java.lang.Class instance for each primitive
    // type to some information that relevant to us about these types:
    private static Hashtable primitiveMap;
    static {
        primitiveMap = new Hashtable(11);
        primitiveMap.put(Boolean.TYPE, new TypeInfo(
                "Z", 1, (byte) 0x15, (byte) 0xac, Boolean.class, "booleanValue"));
        primitiveMap.put(Byte.TYPE, new TypeInfo(
                "B", 1, (byte) 0x15, (byte) 0xac, Byte.class, "byteValue"));
        primitiveMap.put(Character.TYPE, new TypeInfo(
                "C", 1, (byte) 0x15, (byte) 0xac, Character.class, "charValue"));
        primitiveMap.put(Integer.TYPE, new TypeInfo(
                "I", 1, (byte) 0x15, (byte) 0xac, Integer.class, "intValue"));
        primitiveMap.put(Long.TYPE, new TypeInfo(
                "J", 2, (byte) 0x16, (byte) 0xad, Long.class, "longValue"));
        primitiveMap.put(Short.TYPE, new TypeInfo(
                "S", 1, (byte) 0x15, (byte) 0xac, Short.class, "shortValue"));
        primitiveMap.put(Float.TYPE, new TypeInfo(
                "F", 1, (byte) 0x17, (byte) 0xae, Float.class, "floatValue"));
        primitiveMap.put(Double.TYPE, new TypeInfo(
                "D", 2, (byte) 0x18, (byte) 0xaf, Double.class, "doubleValue"));
        primitiveMap.put(Void.TYPE, new TypeInfo(
                "V", -1, (byte) 0xff, (byte) 0xb1, null, null));
    }
    TypeInfo(String typeDescriptor, int localSlots, byte loadInstruction,
        byte returnInstruction, Class primitiveWrapper, 
        String valueMethod) {
        this.typeDescriptor = typeDescriptor;
        this.localSlots = localSlots;
        this.loadInstruction = loadInstruction;
        this.returnInstruction = returnInstruction;
        this.primitiveWrapper = primitiveWrapper;
        this.valueMethod = valueMethod;
    }

    static TypeInfo get(Class type) {
        return (TypeInfo) primitiveMap.get(type);
    }

    static String descriptorFor(Class type) {
        if (type.isPrimitive()) {
            return get(type).typeDescriptor;
        } else if (type.isArray()) {
            return type.getName().replace('.', '/');
        } else {
            return "L" + type.getName().replace('.', '/') + ";";
        }
    }

    static int localSlotsFor(Class type) {
        if (type.isPrimitive()) {
            return get(type).localSlots;
        } else {
            return 1;
        }
    }
}
