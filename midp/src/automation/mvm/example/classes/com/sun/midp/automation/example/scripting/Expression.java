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

interface Expression {
    static final int TYPE_UNKNOWN = 0;
    static final int TYPE_INTEGER = 1;
    static final int TYPE_BOOLEAN = 2;
    static final int TYPE_DOUBLE = 3;
    static final int TYPE_STRING = 4;
    static final int TYPE_ARRAY   = 1024;

    Constant evaluate();
    Variable lvalue();
}

class Compound implements Expression {
    int operation;
    Expression operand1;
    Expression operand2;

    static final int OPERATOR_GET_LENGTH    = 0;  // x.length
    static final int OPERATOR_NOT           = 1;  // !x
    static final int OPERATOR_AND           = 2;  // x && y
    static final int OPERATOR_OR            = 3;  // x || y
    static final int OPERATOR_ADD           = 4;  // x + y
    static final int OPERATOR_SUB           = 5;  // x - y
    static final int OPERATOR_MUL           = 6;  // x * y
    static final int OPERATOR_DIV           = 7;  // x / y
    static final int OPERATOR_REM           = 8;  // x % y
    static final int OPERATOR_SHL           = 9;  // x << y
    static final int OPERATOR_SHR           = 10; // x >> y
    static final int OPERATOR_BITWISE_AND   = 11; // x & y
    static final int OPERATOR_BITWISE_OR    = 12; // x | y
    static final int OPERATOR_EQ            = 13; // x == y
    static final int OPERATOR_NEQ           = 14; // x != y
    static final int OPERATOR_GT            = 15; // x > y
    static final int OPERATOR_LT            = 16; // x < y
    static final int OPERATOR_GTE           = 17; // x >= y
    static final int OPERATOR_LTE           = 18; // x <= y
    static final int OPERATOR_GET_ELEMENT   = 19; // x[y]

    static final int LAST_UNARY_OPERATOR    = OPERATOR_NOT;

    static final int PRIORITY_CONDITIONAL   = 0;
    static final int PRIORITY_OR            = 1;
    static final int PRIORITY_AND           = 2;
    static final int PRIORITY_COMPARE       = 3;
    static final int PRIORITY_ADD           = 4;
    static final int PRIORITY_MUL           = 5;
    static final int PRIORITY_BITWISE       = 6;
    static final int PRIORITY_SHIFT         = 7;
    static final int PRIORITY_MAX           = 8;
    static final int PRIORITY_MIN           = 0;

    Compound(int operation, Expression operand1, Expression operand2) {
        this.operation = operation;
        this.operand1 = operand1;
        this.operand2 = operand2;
    }

    public Constant evaluate() {
        Constant c1 = operand1.evaluate();
        Constant c2 = operation > LAST_UNARY_OPERATOR 
            ? operand2.evaluate() : null;

        switch (operation) {
        case OPERATOR_GET_LENGTH:
            if (c1.type == TYPE_ARRAY) {
                return new Constant(c1.asArray().length);
            }
            if (c1.type == TYPE_STRING) {
                return new Constant(c1.asString().length());
            }
            return Constant.ZERO;

        case OPERATOR_NOT:
            return new Constant(!c1.asBoolean());

        case OPERATOR_AND:
            return new Constant(c1.asBoolean() && c2.asBoolean());

        case OPERATOR_OR:
            return new Constant(c1.asBoolean() || c2.asBoolean());

        case OPERATOR_ADD:
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(c1.asString() + c2.asString());
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() + c2.asDouble());
            }
            return new Constant(c1.asInteger() + c2.asInteger());

        case OPERATOR_SUB:
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() - c2.asDouble());
            }
            return new Constant(c1.asInteger() - c2.asInteger());

        case OPERATOR_MUL:
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() * c2.asDouble());
            }
            return new Constant(c1.asInteger() * c2.asInteger());

        case OPERATOR_DIV:
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() / c2.asDouble());
            }
            return new Constant(c1.asInteger() / c2.asInteger());

        case OPERATOR_REM:
            return new Constant(c1.asInteger() % c2.asInteger());

        case OPERATOR_SHL:
            return new Constant(c1.asInteger() << c2.asInteger());

        case OPERATOR_SHR:
            return new Constant(c1.asInteger() >> c2.asInteger());

        case OPERATOR_BITWISE_AND:
            return new Constant(c1.asInteger() & c2.asInteger());

        case OPERATOR_BITWISE_OR:
            return new Constant(c1.asInteger() | c2.asInteger());

        case OPERATOR_EQ:
            if (c1.type == TYPE_UNKNOWN || c2.type == TYPE_UNKNOWN) {
                return new Constant(c1.type == c2.type && c1.value == c2.value);
            }
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(c1.asString().equals(c2.asString()));
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() == c2.asDouble());
            }
            if (c1.type == TYPE_BOOLEAN || c2.type == TYPE_BOOLEAN) {
                return new Constant(c1.asBoolean() == c2.asBoolean());
            }
            return new Constant(c1.asInteger() == c2.asInteger());

        case OPERATOR_NEQ:
            if (c1.type == TYPE_UNKNOWN || c2.type == TYPE_UNKNOWN) {
                return new Constant(c1.type != c2.type || c1.value != c2.value);
            }
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(!c1.asString().equals(c2.asString()));
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() != c2.asDouble());
            }
            if (c1.type == TYPE_BOOLEAN || c2.type == TYPE_BOOLEAN) {
                return new Constant(c1.asBoolean() != c2.asBoolean());
            }
            return new Constant(c1.asInteger() != c2.asInteger());

        case OPERATOR_GT:
            if (c1.type == TYPE_UNKNOWN || c2.type == TYPE_UNKNOWN) {
                return Constant.FALSE;
            }
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(c1.asString().compareTo(c2.asString()) > 0);
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() > c2.asDouble());
            }
            return new Constant(c1.asInteger() > c2.asInteger());

        case OPERATOR_LT:
            if (c1.type == TYPE_UNKNOWN || c2.type == TYPE_UNKNOWN) {
                return Constant.FALSE;
            }
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(c1.asString().compareTo(c2.asString()) < 0);
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() < c2.asDouble());
            }
            return new Constant(c1.asInteger() < c2.asInteger());

        case OPERATOR_GTE:
            if (c1.type == TYPE_UNKNOWN || c2.type == TYPE_UNKNOWN) {
                return new Constant(c1.type == c2.type && c1.value == c2.value);
            }
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(
                        c1.asString().compareTo(c2.asString()) >= 0);
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() >= c2.asDouble());
            }
            return new Constant(c1.asInteger() >= c2.asInteger());

        case OPERATOR_LTE:
            if (c1.type == TYPE_UNKNOWN || c2.type == TYPE_UNKNOWN) {
                return new Constant(c1.type == c2.type && c1.value == c2.value);
            }
            if (c1.type == TYPE_STRING || c2.type == TYPE_STRING) {
                return new Constant(
                        c1.asString().compareTo(c2.asString()) <= 0);
            }
            if (c1.type == TYPE_DOUBLE || c2.type == TYPE_DOUBLE) {
                return new Constant(c1.asDouble() <= c2.asDouble());
            }
            return new Constant(c1.asInteger() <= c2.asInteger());

        case OPERATOR_GET_ELEMENT:
            return (c1.asArray())[c2.asInteger()].value;

        default:
            return Constant.NULL;
        }
    }

    public Variable lvalue() {
        return operation == OPERATOR_GET_ELEMENT ?
            (operand1.evaluate().asArray())[operand2.evaluate().asInteger()] :
                null;
    }
}

class Conditional implements Expression {
    Expression condition;
    Expression positive;
    Expression negative;

    Conditional(Expression condition, Expression positive, 
            Expression negative) {

        this.condition = condition;
        this.positive = positive;
        this.negative = negative;
    }
    
    public Constant evaluate() {
        return condition.evaluate().asBoolean() ? 
            positive.evaluate() : negative.evaluate();
    }

    public Variable lvalue() {
        return condition.evaluate().asBoolean() ? 
            positive.lvalue() : negative.lvalue();
    }
}

class TypeCast implements Expression {
    int type;
    Expression expr;

    static final TypeCast CAST_INTEGER = new TypeCast(TYPE_INTEGER, null);
    static final TypeCast CAST_BOOLEAN = new TypeCast(TYPE_BOOLEAN, null);
    static final TypeCast CAST_DOUBLE   = new TypeCast(TYPE_DOUBLE, null);
    static final TypeCast CAST_STRING   = new TypeCast(TYPE_STRING, null);
    
    TypeCast(int type, Expression expr) {
        this.type = type;
        this.expr = expr;
    }

    public Constant evaluate() {
        Constant c = expr.evaluate();
        switch (type) {
        case TYPE_INTEGER:
            return new Constant(c.asInteger());
        case TYPE_BOOLEAN:
            return new Constant(c.asBoolean());
        case TYPE_DOUBLE:
            return new Constant(c.asDouble());
        case TYPE_STRING:
            return new Constant(c.asString());
        default:
            return c;
        }
    }
    
    public Variable lvalue() {
        return expr.lvalue();
    }
}
