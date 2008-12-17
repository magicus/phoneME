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


class Instruction {
    Instruction next;

    Instruction execute() {
        return next;
    }
}

class Assignment extends Instruction {
    Expression var;
    Expression expr;

    Assignment(Expression var, Expression expr) {
        this.var = var;
        this.expr = expr;
    }

    Instruction execute() {
        var.lvalue().assign(expr.evaluate());
        return next;
    }
}

class Branch extends Instruction {
    Expression condition;
    Instruction branch;

    Branch(Expression condition, Instruction branch) {
        this.condition = condition;
        this.branch = branch;
    }

    Instruction execute() {
        return condition.evaluate().asBoolean() ? branch : next;
    }
}

abstract class Call extends Instruction implements Expression {
    Expression params[];
    Constant retvalue;

    Call(Expression[] params) {
        this.params = params;
        this.retvalue = Constant.NULL;
    }

    Constant[] evaluateParams() {
        if (params == null) {
            return null;
        }
        Constant values[] = new Constant[params.length];
        for (int i = 0; i < params.length; i++) {
            values[i] = params[i].evaluate();
        }
        return values;
    }

    public Constant evaluate() {
        execute();
        return retvalue;
    }

    public Variable lvalue() {
        return null;
    }
}

class SubroutineCall extends Call {
    Routine subroutine;

    SubroutineCall(Routine subroutine, Expression[] params) {
        super(params);
        this.subroutine = subroutine;
    }
    
    Instruction execute() {
        if (subroutine != null) {
            retvalue = subroutine.invoke(evaluateParams());
        }
        return next;
    }
}

class ExternalCall extends Call {
    int function;
    static ScriptExtension plugin = null;

    ExternalCall(int function, Expression[] params) {
        super(params);
        this.function = function;
    }
    
    Instruction execute() {
        if (plugin != null) {
            retvalue = plugin.invoke(function, evaluateParams());
        }
        return next;
    }
}
