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

import java.io.InputStream;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;

class Scope {
    Hashtable identifiers;
    Scope outer;

    Scope(Scope outer) {
        this.outer = outer;
        this.identifiers = new Hashtable();
    }
}

class ScriptParser {
    private InputStream input;
    private int lineNumber;
    private int nextChar, nextNextChar;
    private Scope scope;

    private final static int DEFINITION_VAR     = 0;
    private final static int DEFINITION_CONST   = 1;
    private final static int DEFINITION_STATIC = 2;

    static Script parse(InputStream input, ScriptExtension plugin) {
        ScriptParser parser = new ScriptParser(input);
        if (plugin != null) {
            ExternalCall.plugin = plugin;
            plugin.register(parser);
        }
        return parser.readScript();
    }

    void registerExternalVariable(String name, Variable var) {
        registerIdentifier(name, var);
    }

    void registerExternalConstant(String name, Constant c) {
        registerIdentifier(name, c);
    }

    void registerExternalFunction(String name, int function) {
        registerIdentifier(name, new Integer(function));
    }

    void registerExternalFunctions(String[] names) {
        if (names != null) {
            for (int i = 0; i < names.length; i++) {
                registerExternalFunction(names[i], i);
            }
        }
    }

    ScriptParser(InputStream input) {
        this.input = input;
        lineNumber = 1;
        nextChar = nextNextChar = 0;
        scope = new Scope(null);
        registerIdentifier(ArrayFactory.CONSTRUCTOR_NAME, new ArrayFactory());
    }

    void emitError(String message) {
        throw new RuntimeException(message + " at line " + lineNumber);
    }

    void emitWarning(String message) {
        System.out.println("Warning: " + message + " at line " + lineNumber);
    }

    Script readScript() {
        Script script = new Script();
        registerIdentifier(Variable.RETURN_NAME, script.locals[0]);
        script.entry = readSequence();
        skipSpaces();
        if (nextChar != -1) {
            emitWarning("Symbols beyond end of script");
        }
        return script;
    }

    Instruction readSequence() {
        Instruction first = readInstruction();
        Instruction current = first;
        while (current != null) {
            current = tie(current, readInstruction());
        }
        return first;
    }

    Instruction readInstruction() {
        String name;
        boolean extern = false;
        
        while (true) {
            skipSpaces();
            if (nextChar == '}' || nextChar == -1) {
                return null;
            }
            if (nextChar == ';') {
                readChar();
                return new Instruction();
            }
            if (nextChar == '{') {
                readChar();
                Instruction instr = readSequence();
                requireChar('}');
                return instr;
            }

            name = readIdentifier();
            if (name.equals("var")) {
                Instruction instr = readVariables(DEFINITION_VAR, extern);
                if (instr != null) {
                    return instr;
                }
                continue;
            } else if (name.equals("const")) {
                readVariables(DEFINITION_CONST, extern);
                continue;
            } else if (name.equals("static")) {
                readVariables(DEFINITION_STATIC, extern);
                continue;
            } else if (name.equals("function")) {
                readFunction(extern);
                continue;
            } else if (name.equals("extern")) {
                extern = true;
                continue;
            } else if (name.equals("goto")) {
                return readGoto();
            } else if (name.equals("return")) {
                return readReturn();
            } else if (name.equals("if")) {
                return readIf();
            } else if (name.equals("while")) {
                return readWhile();
            } else if (name.equals("do")) {
                return readDo();
            } else if (name.equals("for")) {
                return readFor();
            }

            skipSpaces();
            if (nextChar == ':') {
                readChar();
                return setLabel(name);
            }
            if (nextChar == '(') {
                return readCall(name, true);
            }

            return readAssignment(name, true);
        }
    }

    Instruction readVariables(int deftype, boolean extern) {
        Instruction instr = null;
        do {
            if (instr == null) {
                instr = readVariable(deftype, extern);
            } else {
                tie(instr, readVariable(deftype, extern));
            }
            skipSpaces();
        } while (nextChar != ';' && readChar() == ',');
        requireChar(';');
        return instr;
    }

    Instruction readVariable(int deftype, boolean extern) {
        String name = readIdentifier();
        Expression init;        
        skipSpaces();
        if (nextChar == '=') {
            readChar();
            init = readExpression();
        } else {
            init = Constant.NULL;
        }

        if (extern && findIdentifier(name, true) != null) {
                return null;
        }

        switch (deftype) {
        case DEFINITION_VAR:
            Variable var = new Variable();
            registerIdentifier(name, var);
            return init == Constant.NULL ? null : new Assignment(var, init);
        case DEFINITION_CONST:
            if (!(init instanceof Constant)) {
                emitError("Constant expression needed");
            }
            registerIdentifier(name, init);
            break;
        case DEFINITION_STATIC:
            Constant c = init.evaluate();
            if (c == Constant.NULL) {
                c = new Constant();
            }
            registerIdentifier(name, new Variable(c));
            break;
        }
        return null;
    }

    void readFunction(boolean extern) {
        String name = readIdentifier();
        Object id = findIdentifier(name, false);
        Routine subroutine;

        if (id instanceof Integer) {
            if (!extern) {
                emitWarning("External function with the same name (" + 
                        name + ") registred");
            }
            subroutine = new Routine();
        } else if (id instanceof Routine) {
            subroutine = (Routine)id;
            if (subroutine.entry != null && !isNop(subroutine.entry)) {
                emitWarning("Redifinition of function " + name);
            }
        } else {
            registerIdentifier(name, subroutine = new Routine());
        }

        scope = new Scope(scope);
        Vector locals = new Vector();
        Variable var;

        requireChar('(');
        skipSpaces();
        if (nextChar != ')') {
            do {
                registerIdentifier(readIdentifier(), var = new Variable());
                locals.addElement(var);
                skipSpaces();
            } while (nextChar != ')' && readChar() == ',');
        }
        requireChar(')');
        registerIdentifier(Variable.RETURN_NAME, var = new Variable());

        subroutine.entry = readInstruction();
        subroutine.args = locals.size();
        locals.addElement(var);

        for (Enumeration e = scope.identifiers.elements(); e.hasMoreElements(); ) {
            id = e.nextElement();
            if (id instanceof Variable && ((Variable)id).value == Constant.NULL) {
                locals.addElement(id);
            }
        }
        
        subroutine.locals = new Variable[locals.size()];
        locals.copyInto(subroutine.locals);
        scope = scope.outer;
    }

    Instruction setLabel(String name) {
        Instruction target = readInstruction();
        if (target == null) {
            emitError("Cannot set label here");
        }
        Object id = findIdentifier(name, false);
        if (id == null || !(id instanceof Instruction)) {
            registerIdentifier(name, target);
            return target;
        }
        Instruction dummy = (Instruction)id;
        if (!isNop(dummy)) {
            emitError("Label already set");
        }
        dummy.next = target;
        return dummy;
    }

    Call readCall(String name, boolean statement) {
        Expression params[] = readParams();
        if (statement) {
            requireChar(';');
        }
        Object id = findIdentifier(name, true);
        if (id instanceof Integer) {
            return new ExternalCall(((Integer)id).intValue(), params);
        } else if (id instanceof Routine) {
            return new SubroutineCall((Routine)id, params);
        } else {
            emitError("Unknown function " + name);
            return null;
        }
    }

    Expression[] readParams() {
        requireChar('(');
        skipSpaces();
        if (nextChar == ')') {
            readChar();
            return null;
        }
        
        Vector parameters = new Vector();
        do {
            parameters.addElement(readExpression());
            skipSpaces();
        } while (nextChar != ')' && readChar() == ',');
        requireChar(')');

        Expression params[] = new Expression[parameters.size()];
        parameters.copyInto(params);
        return params;
    }

    Instruction readAssignment(String name, boolean statement) {
        Expression var = readLvalue(name);
        if (var instanceof Constant) {
            emitError("Assignment to a constant");
        }
        Expression expr;
        if (nextChar == '=') {
            readChar();
            expr = readExpression();
            if (statement) {
                requireChar(';');
            }
            return new Assignment(var, expr);
        }

        int operation;
        boolean unary = true;
        if ((operation = testOperator('+', '+', Compound.OPERATOR_ADD)) == 0)
        if ((operation = testOperator('-', '-', Compound.OPERATOR_SUB)) == 0) {
            unary = false;
            if ((operation = testOperator('+', '=', Compound.OPERATOR_ADD)) == 0)
            if ((operation = testOperator('-', '=', Compound.OPERATOR_SUB)) == 0)
            if ((operation = testOperator('*', '=', Compound.OPERATOR_MUL)) == 0)
            if ((operation = testOperator('/', '=', Compound.OPERATOR_DIV)) == 0)
            if ((operation = testOperator('%', '=', Compound.OPERATOR_REM)) == 0)
            if ((operation = testOperator('&', '=', Compound.OPERATOR_BITWISE_AND)) == 0)
            if ((operation = testOperator('|', '=', Compound.OPERATOR_BITWISE_OR)) == 0) {
                if ((operation = testOperator('<', '<', Compound.OPERATOR_SHL)) == 0)
                if ((operation = testOperator('>', '>', Compound.OPERATOR_SHR)) == 0) {
                    emitError("Illegal instruction");
                }
                requireChar('=');
            }
        }

        expr = unary ? Constant.ONE : readExpression();
        if (statement) {
            requireChar(';');
        }
        return new Assignment(var, buildExpression(operation, var, expr));
    }

    Instruction readGoto() {
        String name = readIdentifier();
        Instruction target = (Instruction)findIdentifier(name, false);
        if (target == null) {
            registerIdentifier(name, target = new Instruction());
        }
        requireChar(';');
        return new Branch(Constant.TRUE, target);
    }

    Instruction readReturn() {
        skipSpaces();
        if (nextChar == ';') {
            readChar();
            return new Branch(Constant.TRUE, null);
        }
        Variable var = (Variable)findIdentifier(Variable.RETURN_NAME, false);
        if (var == null) {
            emitError("Routine is not expected to return a value");
        }
        Instruction instr = new Assignment(var, readExpression());
        instr.next = new Branch(Constant.TRUE, null);
        requireChar(';');
        return instr;
    }

    Instruction readIf() {
        requireChar('(');
        Expression expr = readExpression();
        requireChar(')');
        Branch fork = new Branch(expr, readInstruction());
        Instruction elseBranch = null;
        skipSpaces();
        if (nextChar == 'e' && nextNextChar == 'l') {
            if (!readIdentifier().equals("else")) {
                emitError("Confused 'else' clause");
            }
            elseBranch = readInstruction();
        }
        if (fork.branch instanceof Branch && ((Branch)fork.branch).condition == Constant.TRUE) {
            fork.branch = ((Branch)fork.branch).branch;
            tie(fork, elseBranch);
        } else {
            Instruction dummy = tie(fork.branch, new Instruction());
            tie(fork, elseBranch);
            tie(fork, dummy);
        }
        return fork;
    }

    Instruction readWhile() {
        requireChar('(');
        Expression expr = readExpression();
        requireChar(')');
        Branch fork = new Branch(expr, readInstruction());
        return tie(fork.branch, fork);
    }

    Instruction readDo() {
        Instruction loop = readInstruction();
        if (!readIdentifier().equals("while")) {
            emitError("'while' expected");
        }
        requireChar('(');
        tie(loop, new Branch(readExpression(), loop));
        requireChar(')');
        requireChar(';');
        return loop;
    }

    Instruction readFor() {
        requireChar('(');
        Instruction init = readInstruction();
        skipSpaces();
        Expression expr = nextChar == ';' ? Constant.TRUE : readExpression();
        requireChar(';');
        skipSpaces();
        Instruction update = nextChar == ')' ? 
            null : readAssignment(readIdentifier(), false);
        requireChar(')');
        Instruction body = readInstruction();
        tie(body, update);
        tie(init, tie(body, new Branch(expr, body)));
        return init;
    }

    Expression readLvalue(String name) {
        Expression var = (Expression)findIdentifier(name, true);
        if (var == null) {
            emitWarning("Undeclared variable " + name);
            registerIdentifier(name, var = new Variable());
        }
        while (nextChar == '[') {
            readChar();
            var = new Compound(
                    Compound.OPERATOR_GET_ELEMENT, var, readExpression());
            requireChar(']');
            skipSpaces();
        }
        if (nextChar == '.') {
            readChar();
            if (!readIdentifier().equals("length")) {
                emitError("'.length' expected");
            }
            return new Compound(Compound.OPERATOR_GET_LENGTH, var, null);
        }
        return var;
    }

    Expression readExpression() {
        return readExpression(Compound.PRIORITY_MIN);
    }

    Expression readExpression(int priority) {
        if (priority >= Compound.PRIORITY_MAX) {
            return readTerm();
        }

        Expression operand = readExpression(priority + 1);
        int operation;
        while (true) {
            skipSpaces();
            switch (priority) {
            case Compound.PRIORITY_CONDITIONAL:
                if (nextChar == '?') {
                    readChar();
                    Expression positive = readExpression(priority);
                    requireChar(':');
                    return new Conditional(operand, positive, 
                            readExpression(priority));
                }
                return operand;
            case Compound.PRIORITY_OR:
                if ((operation = testOperator('|', '|', Compound.OPERATOR_OR))  != 0) break;
                return operand;
            case Compound.PRIORITY_AND:
                if ((operation = testOperator('&', '&', Compound.OPERATOR_AND)) != 0) break;
                return operand;
            case Compound.PRIORITY_COMPARE:
                if ((operation = testOperator('=', '=', Compound.OPERATOR_EQ))  != 0) break;
                if ((operation = testOperator('!', '=', Compound.OPERATOR_NEQ)) != 0) break;
                if ((operation = testOperator('>', '=', Compound.OPERATOR_GTE)) != 0) break;
                if ((operation = testOperator('<', '=', Compound.OPERATOR_LTE)) != 0) break;
                if ((operation = testOperator('>', '\0', Compound.OPERATOR_GT)) != 0) break;
                if ((operation = testOperator('<', '\0', Compound.OPERATOR_LT)) != 0) break;
                return operand;
            case Compound.PRIORITY_ADD:
                if ((operation = testOperator('+', '\0', Compound.OPERATOR_ADD)) != 0) break;
                if ((operation = testOperator('-', '\0', Compound.OPERATOR_SUB)) != 0) break;
                return operand;
            case Compound.PRIORITY_MUL:
                if ((operation = testOperator('*', '\0', Compound.OPERATOR_MUL)) != 0) break;
                if ((operation = testOperator('/', '\0', Compound.OPERATOR_DIV)) != 0) break;
                if ((operation = testOperator('%', '\0', Compound.OPERATOR_REM)) != 0) break;
                return operand;
            case Compound.PRIORITY_BITWISE:
                if ((operation = testOperator('&', '\0', Compound.OPERATOR_BITWISE_AND)) != 0) break;
                if ((operation = testOperator('|', '\0', Compound.OPERATOR_BITWISE_OR)) != 0) break;
                return operand;
            case Compound.PRIORITY_SHIFT:
                if ((operation = testOperator('<', '<', Compound.OPERATOR_SHL)) != 0) break;
                if ((operation = testOperator('>', '>', Compound.OPERATOR_SHR)) != 0) break;
                return operand;
            default:
                return operand;
            }
            operand = buildExpression(operation, operand, readExpression(priority + 1));
        }
    }
    
    Expression readTerm() {
        skipSpaces();
        if (nextChar == '!') {
            readChar();
            return buildExpression(Compound.OPERATOR_NOT, readTerm(), null);
        } else if (nextChar == '(') {
            readChar();
            Expression expr = readExpression();
            requireChar(')');
            return expr instanceof TypeCast ? 
                new TypeCast(((TypeCast)expr).type, readTerm()) : expr;
        } else if (nextChar == '"') {
            return readString();
        } else if (nextChar >= '0' && nextChar <= '9') {
            return readNumber();
        } else if (nextChar == '+' || nextChar == '-') {
            return nextNextChar >= '0' && nextNextChar <= '9' ?
                         readNumber() :
                         Constant.ZERO;
        } else {
            String name = readIdentifier();
            if (name.equals("true")) {
                return Constant.TRUE;
            } else if (name.equals("false")) {
                return Constant.FALSE;
            } else if (name.equals("null")) {
                return Constant.NULL;
            } else if (name.equals("int")) {
                return TypeCast.CAST_INTEGER;
            } else if (name.equals("boolean")) {
                return TypeCast.CAST_BOOLEAN;
            } else if (name.equals("double")) {
                return TypeCast.CAST_DOUBLE;
            } else if (name.equals("String")) {
                return TypeCast.CAST_STRING;
            } else if (name.equals("new")) {
                name = readIdentifier();
            }
            skipSpaces();
            if (nextChar == '(') {
                return readCall(name, false);
            }
            return readLvalue(name);
        }
    }
    
    String readIdentifier() {
        skipSpaces();
        if (!(nextChar >= 'A' && nextChar <= 'Z' ||
                    nextChar >= 'a' && nextChar <= 'z' || nextChar == '_')) {
            emitError("Identifier expected");
        }
        StringBuffer sb = new StringBuffer();
        sb.append(readChar());
        while (nextChar >= 'A' && nextChar <= 'Z' ||
                     nextChar >= 'a' && nextChar <= 'z' || nextChar == '_' ||
                     nextChar >= '0' && nextChar <= '9') {
            sb.append(readChar());
        }
        return new String(sb);
    }

    Constant readNumber() {
        boolean isDouble = false;
        int radix = 10;
        if (nextChar == '0') {
            if (nextNextChar == 'x') {
                radix = 16;
                readChar();
                readChar();
            } else if (nextNextChar >= '0' && nextNextChar <= '7') {
                radix = 8;
                readChar();
            }
        }

        StringBuffer sb = new StringBuffer();
        boolean goodChar;
        do {
            sb.append(readChar());
            if (nextChar >= '0' && nextChar <= '9') {
                goodChar = true;
            } else if (radix == 16 && (nextChar >= 'A' && nextChar <= 'F' 
                        || nextChar >= 'a' && nextChar <= 'f')) {
                goodChar = true;
            } else if (nextChar == '.' && !isDouble && radix == 10) {
                goodChar = true;
                isDouble = true;
            } else if ((nextChar == 'E' || nextChar == 'e') 
                    && !isDouble && radix == 10) {
                goodChar = true;
                isDouble = true;
                if (nextNextChar == '+' || nextNextChar == '-') {
                    sb.append(readChar());
                }
            } else {
                goodChar = false;
            }
        } while (goodChar);

        String s = new String(sb);
        if (isDouble) {
            return new Constant(Double.parseDouble(s));
        }
        int value = Integer.parseInt(s, radix);
        if (value == 0) {
            return Constant.ZERO;
        } else if (value == 1) {
            return Constant.ONE;
        } else {
            return new Constant(value);
        }
    }

    Constant readString() {
        StringBuffer sb = new StringBuffer();
        do {
            readChar();
            while (nextChar != '"') {
                if (nextChar == '\n' || nextChar == -1) {
                    emitError("Unterminated string constant");
                }
                if (nextChar == '\\') {
                    readChar();
                    switch (nextChar) {
                    case 'n':
                        sb.append('\n');
                        break;
                    case 't':
                        sb.append('\t');
                        break;
                    case '0':
                        sb.append((char)readNumber().asInteger());
                        break;
                    case -1:
                        continue;
                    default:
                        sb.append((char)nextChar);
                    }
                    readChar();
                } else {
                    sb.append(readChar());
                }
            }
            readChar();
            skipSpaces();
        } while (nextChar == '"');

        return sb.length() == 0 ? 
            Constant.EMPTY_STRING : new Constant(new String(sb));
    }

    void requireChar(char c) {
        skipSpaces();
        if (readChar() != c) {
            emitError("'" + c + "' expected");
        }
    }
    
    void skipSpaces() {
        do {
            while (nextChar >= 0 && nextChar <= ' ') {
                readChar();
            }
        } while (nextChar == '/' && skipComments());
    }

    boolean skipComments() {
        switch (nextNextChar) {
        case '/':
            readChar();
            readChar();
            while (nextChar != -1 && readChar() != '\n');
            return true;

        case '*':
            readChar();
            readChar();
            while (nextChar != -1 && !(readChar() == '*' && readChar() == '/'));
            return true;

        default:
            return false;
        }
    }

    char readChar() {
        char thisChar = (char)nextChar;
        nextChar = nextNextChar;
        try {
            nextNextChar = input.read();
        } catch (Exception e) {
            nextNextChar = -1;
        }

        if (thisChar == '\n') {
            lineNumber++;
        }
        return thisChar;
    }

    int testOperator(char char1, char char2, int operation) {
        if (char1 != nextChar) {
            return 0;
        }
        if (char2 == 0 && ((char1 != '|' && char1 != '&') 
                    || nextNextChar != char1)) {
            readChar();
            return operation;
        }
        if (char2 != nextNextChar) {
            return 0;
        }
        readChar();
        readChar();
        return operation;
    }

    Expression buildExpression(int operation, Expression operand1, 
            Expression operand2) {
        Expression compound = new Compound(operation, operand1, operand2);
        return operand1 instanceof Constant && operand2 instanceof Constant ?
                     compound.evaluate() :
                     compound;
    }

    Instruction tie(Instruction dst, Instruction src) {
        if (dst == null) {
            emitWarning(
              "Suspicious control structure, use ';' for empty instruction");
        } else {
            while (dst.next != null) {
                dst = dst.next;
            }
            dst.next = src;
        }
        return src;
    }

    boolean isNop(Instruction instr) {
        return instr.getClass() == Instruction.class && instr.next == null;
    }

    void registerIdentifier(String name, Object identifier) {
        if (scope.identifiers.put(name, identifier) != null) {
            emitError("Duplicate identifier " + name);
        }
    }

    Object findIdentifier(String name, boolean allScopes) {
        Scope sc = scope;
        Object id;
        do {
            if ((id = sc.identifiers.get(name)) != null) {
                return id;
            }
            sc = sc.outer;
        } while (allScopes && sc != null);
        return null;
    }
}
