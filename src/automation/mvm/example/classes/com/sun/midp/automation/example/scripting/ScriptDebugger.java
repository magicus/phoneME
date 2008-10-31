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

class ScriptDebugger {
    static String OPERATORS[] = {"l", "!", "&&", "||", "+", "-", "*", "/", "%", 
        "<<", ">>", "&", "|", "==", "!=", ">", "<",
        ">=", "<=", "."};
    
    static void traceFlow(Routine routine) {
        traceFlow(routine.entry);
    }

    static void traceFlow(Instruction entry) {
        for (Instruction instr = entry; instr != null; instr = instr.next) {
            printInstruction(instr);
        }
    }

    static void printInstruction(Instruction instr) {
        System.out.print(getCode(instr) + ": ");
        if (instr instanceof Branch) {
            printBranch((Branch)instr);
        } else if (instr instanceof Assignment) {
            printAssignment((Assignment)instr);
        } else if (instr instanceof SubroutineCall) {
            System.out.println("call");
        } else if (instr instanceof ExternalCall) {
            printExternalCall((ExternalCall)instr);
        } else {
            System.out.println("nop");
        }
    }

    static String getCode(Object obj) {
        return obj == null ? "null" : Integer.toHexString(obj.hashCode());
    }

    static void printBranch(Branch instr) {
        System.out.print("branch (");
        printExpression(instr.condition);
        System.out.println(") to " + getCode(instr.branch));
    }

    static void printAssignment(Assignment instr) {
        System.out.print("assignment " + getCode(instr.var) + " := ");
        printExpression(instr.expr);
        System.out.println();
    }

    static void printExternalCall(ExternalCall instr) {
        System.out.println("external call no." + instr.function);
    }

    static void printExpression(Expression expr) {
        if (expr instanceof Constant) {
            printConstant((Constant)expr);
        } else if (expr instanceof Variable) {
            printVariable((Variable)expr);
        } else if (expr instanceof Compound) {
            printCompound((Compound)expr);
        } else {
            System.out.print("expr");
        }
    }

    static void printCompound(Compound expr) {
        System.out.print("(");
        printExpression(expr.operand1);
        System.out.print(" " + OPERATORS[expr.operation] + " ");
        printExpression(expr.operand2);
        System.out.print(")");
    }
    
    static void printConstant(Constant c) {
        System.out.print(c.asString());
    }

    static void printVariable(Variable v) {
        System.out.print("{" + v.value.asString() + "}");
    }
}
