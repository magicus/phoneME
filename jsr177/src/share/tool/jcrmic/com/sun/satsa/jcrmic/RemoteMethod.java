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

package com.sun.satsa.jcrmic;

import java.util.*;
import java.io.*;

import com.sun.satsa.jcrmic.classfile.*;
import com.sun.satsa.jcrmic.utils.IndentingWriter;

/**
 * This class represents remote method.
 */

public class RemoteMethod {

    /**
     * Method name.
     */
    private String name;

    /**
     * Method descriptor.
     */
    private String descriptor;

    /**
     * Exceptions thrown by method.
     */
    private Vector exceptions;

    /**
     * Class loader.
     */
    private Loader loader;

    /**
     * Constructor.
     * @param name method name
     * @param descriptor method descriptor
     * @param loader class loader
     */
    public RemoteMethod(String name, String descriptor, Loader loader) {

        this.name = name;
        this.descriptor = descriptor;
        this.loader = loader;
        exceptions = new Vector();
    }

    /**
     * Add an exception type to the exceptions list.
     * @param ex exception class descriptor
     */
    public void addException(JClass ex) {

        exceptions.add(ex);
    }

    /**
     * Returns the list of exceptions thrown by method.
     * @return the list of exceptions thrown by method
     */
    public Vector getExceptions() {
        return exceptions;
    }

    /**
     * Merge the lists of exceptions thrown by this and specfied remote methods.
     * @param m remote method descriptor
     */
    public void merge(RemoteMethod m) {

        Vector list1 = exceptions;
        Vector list2 = m.getExceptions();
        exceptions = new Vector();
        mergeLists(list1, list2, exceptions);
        mergeLists(list2, list1, exceptions);
    }

    /**
     * Places into the third list all exceptions from the first list that are
     * subclasses of exceptions in the second list.
     * @param list1 exceptions list
     * @param list2 exceptions list
     * @param exceptions exceptions list
     */
    private static void mergeLists(Vector list1, Vector list2,
                                   Vector exceptions) {

        for (int i = 0; i < list1.size(); i++) {

            JClass c1 = (JClass) list1.elementAt(i);

            if (exceptions.contains(c1)) {
                continue;
            }

            for (int j = 0; j < list2.size(); j++) {

                JClass c2 = (JClass) list2.elementAt(j);

                if (c1.isSubclass(c2)) {
                    exceptions.add(c1);
                    break;
                }
            }
        }
    }

    /**
     * Write stub method for this method into the output stream.
     * @param p output stream
     * @throws IOException if I/O error occurs
     */
    public void write(IndentingWriter p) throws IOException {

        Vector v = parseDescriptor(descriptor);
        String param = (String) v.elementAt(v.size() - 1);

        String s = "public " + getType(param) + " " + name + "(";

        for (int i = 0; i < v.size() - 1; i++) {

            s = s + getType((String) v.elementAt(i)) + " param" + (i + 1);

            if (i < v.size() - 2) {
                s = s + ", ";
            }
        }

        s = s + ") ";

        if (exceptions.size() > 0) {

            s = s + "throws ";

            for (int i = 0; i < exceptions.size(); i++) {

                JClass ex = (JClass) exceptions.elementAt(i);
                s = s + ex.getClassName().replace('/', '.');

                if (i < exceptions.size() - 1) {
                    s = s + ", ";
                }
            }
        }

        p.pln("");
        p.plnI(s + " {");
        p.pln("");

        computeUniqueCatchList();

        if (exceptions.size() > 0) {
            p.plnI("try {");
        }


        p.pln("");

        if (!((String) v.elementAt(v.size() - 1)).equals("V")) {
            p.p("Object result = ");
        }

        p.p("ref.invoke(\"" + name + descriptor + "\", ");

        if (v.size() == 1) {
            p.p("null");
        } else {
            p.p("new java.lang.Object[] {");

            for (int i = 0; i < v.size() - 1; i++) {
                p.p(getParameter((String) v.elementAt(i), i));
                if (i < v.size() - 2) {
                    p.p(", ");
                }
            }
            p.p("}");
        }

        p.pln(");");
        s = (String) v.elementAt(v.size() - 1);

        if (!s.equals("V")) {
            p.pln("return " + getReturnFunction(s));
        }

        p.pln("");


        /*
         * If we need to catch any particular exceptions, finally
         * write the catch blocks for them, rethrow any other
         * Exceptions with an UnexpectedException, and end the try
         * block.  
         */
        if (exceptions.size() > 0) {
            for (Enumeration enum = exceptions.elements();
                 enum.hasMoreElements();) {
                JClass def = (JClass) enum.nextElement();
                p.pOlnI("} catch (" + def.getClassName().replace('/', '.') +
                        " e) {");
                p.pln("throw e;");
            }
            p.pOlnI("} catch (java.lang.Exception e) {");
            p.pln("throw new java.rmi.RemoteException" +
                    "(\"undeclared checked exception\", e);");
            p.pOln("}");                // end try/catch block
        }

        p.pOln("}");
    }

    /**
     * Returns parameter initialization expression for stub method.
     * @param param parameter descriptor
     * @param index parameter index
     * @return parameter initialization expression
     */
    private String getParameter(String param, int index) {

        if (param.length() == 1) {

            switch (JCReturnTypes.indexOf(param)) {

                case 0:
                    return "new Boolean(param" + (index + 1) + ")";
                case 1:
                    return "new Byte(param" + (index + 1) + ")";
                case 2:
                    return "new Short(param" + (index + 1) + ")";
                case 3:
                    return "new Integer(param" + (index + 1) + ")";
            }
        }

        return "param" + (index + 1);
    }

    /**
     * Java Card type descriptor string.
     */
    private static final String JCTypes = "ZBSI";
    /**
     * Java Card type descriptor string.
     */
    private static final String JCArrays = "[Z[B[S[I";
    /**
     * Java Card type descriptor string.
     */
    private static final String JCReturnTypes = "ZBSIV";

    /**
     * Returns return type expression for stub method.
     * @param param return type descriptor
     * @return return type expression
     */
    private static String getType(String param) {

        if (param.length() == 1) {

            switch (JCReturnTypes.indexOf(param)) {

                case 0:
                    return "boolean";
                case 1:
                    return "byte";
                case 2:
                    return "short";
                case 3:
                    return "int";
                case 4:
                    return "void";
            }
        }

        if (param.length() == 2) {

            switch (JCArrays.indexOf(param)) {

                case 0:
                    return "boolean[]";
                case 2:
                    return "byte[]";
                case 4:
                    return "short[]";
                case 6:
                    return "int[]";
            }
        }

        return param.substring(1, param.length() - 1).replace('/', '.');
    }

    /**
     * Returns expression for stub method return value.
     * @param param return type descriptor
     * @return expression for stub method return value
     */
    private static String getReturnFunction(String param) {

        if (param.length() == 1) {

            switch (JCReturnTypes.indexOf(param)) {

                case 0:
                    return "((Boolean) result).booleanValue();";
                case 1:
                    return "((Byte) result).byteValue();";
                case 2:
                    return "((Short) result).shortValue();";
                case 3:
                    return "((Integer) result).intValue();";
            }
        }

        if (param.length() == 2) {

            switch (JCArrays.indexOf(param)) {

                case 0:
                    return "(boolean[]) result;";
                case 2:
                    return "(byte[]) result;";
                case 4:
                    return "(short[]) result;";
                case 6:
                    return "(int[]) result;";
            }
        }

        param = param.substring(1, param.length() - 1).replace('/', '.');
        return "(" + param + ") result;";
    }

    /**
     * Compute the exceptions which need to be caught and rethrown in a
     * stub method before wrapping Exceptions in RemoteException.
     */
    private void computeUniqueCatchList() {

        Vector uniqueList = new Vector();

        uniqueList.addElement(loader.getClass("java/lang/RuntimeException"));
        uniqueList.addElement(loader.getClass("java/rmi/RemoteException"));

        JClass defException = loader.getClass("java/lang/Exception");

        /* For each exception declared by the stub method's throws clause: */
        nextException:
        for (int i = 0; i < exceptions.size(); i++) {

            JClass decl = (JClass) exceptions.elementAt(i);

            if (defException.isSubclass(decl)) {
                /*
                 * If java.lang.Exception (or a superclass) was
                 * declared in the throws clause of this stub method,
                 * then we don't have to bother catching anything;
                 * clear the list and return.  
                 */
                uniqueList.removeAllElements();
                break;
            } else if (!decl.isSubclass(defException)) {
                /*
                * Ignore other Throwables that do not extend Exception,
                * since they do not need to be caught anyway.
                */
                continue;
            }
            /*
            * Compare this exception against the current list of
            * exceptions that need to be caught:
            */
            for (int j = 0; j < uniqueList.size(); ) {
                JClass def = (JClass) uniqueList.elementAt(j);
                if (decl.isSubclass(def)) {
                    /*
                    * If a superclass of this exception is already on
                    * the list to catch, then ignore and continue;
                    */
                    continue nextException;
                } else if (def.isSubclass(decl)) {
                    /*
                    * If a subclass of this exception is on the list
                    * to catch, then remove it.
                    */
                    uniqueList.removeElementAt(j);
                } else {
                    j++;        // else continue comparing
                }
            }
            /* This exception is unique: add it to the list to catch. */
            uniqueList.addElement(decl);
        }
        exceptions = uniqueList;
    }

    /**
     * Parses remote method descriptor.
     * @param descriptor remote method descriptor
     * @return vector that contais string descriptor for every parameter
     * and return value.
     */
    public static Vector parseDescriptor(String descriptor) {

        Vector v = new Vector();

        int index = descriptor.indexOf(")");

        String parameters = descriptor.substring(1, index);

        int j = 0;
        while (j < parameters.length()) {

            String c = parameters.substring(j, j + 1);
            j++;

            if (JCTypes.indexOf(c) != -1) {
                v.add(c);
                continue;
            }

            if (j == parameters.length()) {
                return null;
            }

            c = c + parameters.substring(j, j + 1);
            j++;

            if (JCArrays.indexOf(c) != -1) {
                v.add(c);
                continue;
            }

            return null;
        }

        parameters = descriptor.substring(index + 1);

        if ((parameters.length() == 1) &&
                (JCReturnTypes.indexOf(parameters) == -1)) {
            return null;
        }
        if (parameters.length() == 2 &&
                JCArrays.indexOf(parameters) == -1) {
            return null;
        }
        if (parameters.length() > 2 &&
                !(parameters.startsWith("L") &&
                parameters.endsWith(";"))) {
            return null;
        }
        v.add(parameters);

        return v;
    }
}
