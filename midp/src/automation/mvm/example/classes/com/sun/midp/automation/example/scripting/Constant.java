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

package com.sun.midp.automation.example.scripting;

class Constant implements Expression {
    int type;
    Object value;

    static final Constant NULL         = new Constant();
    static final Constant TRUE         = new Constant(true);
    static final Constant FALSE        = new Constant(false);
    static final Constant ZERO         = new Constant(0);
    static final Constant ONE          = new Constant(1);
    static final Constant EMPTY_STRING = new Constant("");

    Constant() {
        this.type = TYPE_UNKNOWN;
        this.value = null;
    }

    Constant(int value) {
        this.type = TYPE_INTEGER;
        this.value = new Integer(value);
    }

    Constant(Integer value) {
        this.type = TYPE_INTEGER;
        this.value = value;
    }

    Constant(boolean value) {
        this.type = TYPE_BOOLEAN;
        this.value = value ? Boolean.TRUE : Boolean.FALSE;
    }

    Constant(double value) {
        this.type = TYPE_DOUBLE;
        this.value = new Double(value);
    }

    Constant(String value) {
        this.type = TYPE_STRING;
        this.value = value;
    }

    Constant(Variable[] value) {
        this.type = TYPE_ARRAY;
        this.value = value;
    }

    Constant(Object value) {
        this.type = TYPE_UNKNOWN;
        this.value = value;
    }

    public Constant evaluate() {
        return this;
    }

    public Variable lvalue() {
        return null;
    }

    int getType() {
        return type;
    }

    Object getValue() {
        return value;
    }

    int asInteger() {
        switch (type) {
        case TYPE_BOOLEAN:
            return ((Boolean)value).booleanValue() ? 1 : 0;

        case TYPE_INTEGER:
            return ((Integer)value).intValue();

        case TYPE_DOUBLE:
            return ((Double)value).intValue();

        case TYPE_STRING:
            return Integer.parseInt((String)value);

        case TYPE_ARRAY:
            return ((Variable[])value).length;

        case TYPE_UNKNOWN:
        default:
            return 0;
        }
    }

    boolean asBoolean() {
        switch (type) {
        case TYPE_BOOLEAN:
            return ((Boolean)value).booleanValue();

        case TYPE_INTEGER:
            return ((Integer)value).intValue() != 0;

        case TYPE_DOUBLE:
            return ((Double)value).doubleValue() != 0.0;

        case TYPE_STRING:
            return ((String)value).length() > 0;

        case TYPE_ARRAY:
            return ((Variable[])value).length > 0;

        case TYPE_UNKNOWN:
        default:
            return false;
        }
    }

    double asDouble() {
        switch (type) {
        case TYPE_BOOLEAN:
            return ((Boolean)value).booleanValue() ? 1.0 : 0.0;

        case TYPE_INTEGER:
            return ((Integer)value).intValue();

        case TYPE_DOUBLE:
            return ((Double)value).doubleValue();

        case TYPE_STRING:
            return Double.parseDouble((String)value);

        case TYPE_ARRAY:
            return ((Variable[])value).length;

        case TYPE_UNKNOWN:
        default:
            return 0.0;
        }
    }

    Variable[] asArray() {
        return (Variable[])value;
    }

    String asString() {
        if (value == null) {
            return "";
        }

        return type == TYPE_ARRAY ? "array" : value.toString();
    }
}
