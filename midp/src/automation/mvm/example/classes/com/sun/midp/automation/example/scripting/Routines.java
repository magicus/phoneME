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

class Routine {
    Instruction entry;
    Variable locals[];
    int args;

    Routine() {}

    Constant invoke(Constant[] params) {
        Constant localValues[] = storeLocals();
        setLocals(params, args);

        Instruction instr = entry;
        while (instr != null) {
            instr = instr.execute();
        }

        Constant retvalue = locals[args].value;
        setLocals(localValues, localValues.length);
        return retvalue;
    }

    Constant[] storeLocals() {
        Constant localValues[] = new Constant[locals.length];
        for (int i = 0; i < locals.length; i++) {
            localValues[i] = locals[i].value;
        }
        return localValues;
    }

    void setLocals(Constant[] params, int count) {
        int paramCount = params == null ? 0 : params.length;
        for (int i = 0; i < count; i++) {
            locals[i].value = i < paramCount ? params[i] : Constant.NULL;
        }
    }
}

class ArrayFactory extends Routine {
    static final String CONSTRUCTOR_NAME = "Array";
    
    ArrayFactory() {}

    Constant invoke(Constant[] params) {
        if (params == null || params.length == 0) {
            return Constant.NULL;
        }
        Variable elements[];
        if (params.length == 1) {
            elements = new Variable[params[0].asInteger()];
            for (int i = 0; i < elements.length; i++) {
                elements[i] = new Variable();
            }
        } else {
            elements = new Variable[params.length];
            for (int i = 0; i < elements.length; i++) {
                elements[i] = new Variable(params[i]);
            }
        }
        return new Constant(elements);
    }
}
