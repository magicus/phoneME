/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.utils.mkstubs;

import java.util.*;
import java.io.*;
import java.util.zip.*;
import java.lang.reflect.*;

public class MkStubs {
    private static String[] classes;
    private static Hashtable pkgNames = new Hashtable();
    private static Hashtable typeList = new Hashtable();
    private static Hashtable emptyList = new Hashtable();
    private static String tmpDir = ".";
    private static String agentDir = ".";

    
    private static String translateClassNamePkg(String typeName) {
        String name = translateArray(typeName);
        String pkgName = (String)pkgNames.get(name);
        return (pkgName == null ? "" : pkgName + ".") + translateClassName(name);
    }
    private static String translateInterfaceImplNamePkg(String typeName) {
        String name = translateArray(typeName);
        String pkgName = (String)pkgNames.get(name);
        return (pkgName == null ? "" : pkgName + ".") + translateInterfaceImplName(name);
    }
    private static String translateClassName(String typeName) {
        String name = translateArray(typeName);
        int ind;
        if ((ind = (name.lastIndexOf('.'))) != -1) {
            name = name.substring(ind + 1);
        }
        return name + "";
    }
    private static String translateInterfaceImplName(String typeName) {
        String name = translateArray(typeName);
        int ind;
        if ((ind = (name.lastIndexOf('.'))) != -1) {
            name = name.substring(ind + 1);
        }
        return name + "__Impl";
    }
    private static String translateAgentClass(String typeName) {
        String name = translateArray(typeName);
        int ind;
        if ((ind = (name.lastIndexOf('.'))) != -1) {
            name = name.substring(ind + 1);
        }
        return name + "__AgentClass";
    }
    private static boolean isTranslatedClass(String name) {
        return findTranslatedClass(classes, name) != -1;
    }

    private static int findTranslatedClass(String[] classList, String typeName) {
        if (classList == null) {
            return -1;
        }
        String name = translateArray(decodeType(typeName));
        for (int i = 0; i < classList.length; i++) {
            if (classList[i] != null && name.equals(classList[i])) {
                return i;
            }
        }
        return -1;
    }
    private static String translateArray(String name) {
        int len = name.length();
        while (len > 0 && (name.charAt(len - 1) == '[' || name.charAt(len - 1) == ']')) {
            len--;
        }
        if (len < name.length()) {
            return name.substring(0, len);
        }
        return name;
    }
    
    private static String getBrackets(String name) {
        int len = name.length();
        while (len > 0 && (name.charAt(len - 1) == '[' || name.charAt(len - 1) == ']')) {
            len--;
        }
        if (len < name.length()) {
            return name.substring(len);
        }
        return "";
    }

    public static void main(String[] args)  {
        String pkgName = "proxy";
        Vector clsList = new Vector();
        Vector clsNameList = new Vector();
        boolean tmpDirFlag = false;
        boolean agentDirFlag = false;
        boolean pkgNameFlag = false;
        String[] classPath = new String[] {};
        boolean classPathFlag = false;
        boolean listMissedClasses = false;
        Hashtable missedClasses = new Hashtable();
        boolean doTrace = false;
        boolean inherit = false;
        Hashtable inheritList = new Hashtable();
        boolean empty = false;

        for (int i = 0; i < args.length; i++) {
            Class clazz;
            if (args[i].equals("-d")) {
                tmpDirFlag = true;
                continue;
            }
            if (args[i].equals("-ad")) {
                agentDirFlag = true;
                continue;
            }
            if (args[i].equals("-package")) {
                pkgNameFlag = true;
                continue;
            }
            if (args[i].equals("-jarpath")) {
                classPathFlag = true;
                continue;
            }
            if (args[i].equals("-l")) {
                listMissedClasses = true;
                continue;
            }
            if (args[i].equals("-i")) {
                inherit = true;
                continue;
            }
            if (args[i].equals("-e")) {
                empty = true;
                continue;
            }
            if (args[i].equals("-trace")) {
                doTrace = true;
                continue;
            }
            if (tmpDirFlag) {
                tmpDir = args[i];
                tmpDirFlag = false;
                continue;
            }
            if (agentDirFlag) {
                agentDir = args[i];
                agentDirFlag = false;
                continue;
            }
            if (pkgNameFlag) {
                pkgName = args[i];
                pkgNameFlag = false;
                continue;
            }
            if (classPathFlag) {
                String cpArg = args[i];
                Vector cpList = new Vector();
                int ind;
                String name;
                
                do {
                    ind = cpArg.indexOf(File.pathSeparatorChar);
                    if (ind == -1) {
                        name = cpArg;
                        cpArg = null;
                    } else {
                        name = cpArg.substring(0, ind);
                        cpArg = cpArg.substring(ind + 1);
                    }
                    cpList.addElement(name);
                } while (cpArg != null);
                classPath = new String[cpList.size()];
                cpList.toArray(classPath);
                classPathFlag = false;
                continue;
            }
            if (clsNameList.contains(args[i])) {
                pl("Class '" + args[i] + 
                   "' already provided. Skipped.");
                continue;
            }
            clsNameList.addElement(args[i]);
            pkgNames.put(args[i], pkgName);
            if (inherit) {
                inheritList.put(args[i], "");
                inherit = false;
            }
            if (empty) {
                emptyList.put(args[i], "");
                empty = false;
            }
        }
        
        final String[] fClassPath = classPath;
        ClassLoader classLoader = new ClassLoader() {
            String[] classPath;
            ZipFile[] jarFile;
            boolean initialized = false;
            Object sync = new Object();
            
            private void initClassPath() {
                classPath = fClassPath;
                jarFile = new ZipFile[classPath.length];
                for (int i = 0; i < classPath.length; i++) {
                    try {
                        jarFile[i] = new ZipFile(classPath[i]);
                    } catch (IOException e) {
                        jarFile[i] = null;
                        continue;
                    }
                }
                initialized = true;
            }
            public Class findClass(String name) throws ClassNotFoundException {
                if (!initialized) {
                    synchronized (sync) {
                        if (!initialized) {
                            initClassPath();
                        }
                    }
                }
                InputStream jarStream = null;
                try {
                    String cls = name.replace('.', '/') + ".class";
                    boolean found = false;
                    for (int i = 0; i < classPath.length; i++) {
                        if (jarFile[i] == null) {
                            continue;
                        }
                        ZipEntry en = jarFile[i].getEntry(cls);
                        if (en != null) {
                            found = true;
                            jarStream = jarFile[i].getInputStream(en);
                            break;
                        }
                    }
                    if (!found) {
                        throw new ClassNotFoundException();
                    }
                } catch (IOException e) {
                    throw new ClassNotFoundException();
                }
                
                try {
                    byte[] b = loadClassData(name, jarStream);
                    return defineClass(name, b, 0, b.length);
                } catch (IOException e) {
                    throw new ClassNotFoundException();
                } catch (ClassNotFoundException cnfe) {
                    throw cnfe;
                } finally {
                    try {
                        jarStream.close();
                    } catch (IOException e) {}
                }
            }
            private byte[] loadClassData(String cls, InputStream jarStream) throws ClassNotFoundException, IOException {
                
                int size = 1000;
                byte[] b = new byte[size];
                int bytesread = 0;
                for (;;) {
                    int len = jarStream.read(b, bytesread, b.length - bytesread);
                    if (len == -1) {
                        if (bytesread == 0) {
                            throw new ClassNotFoundException();
                        }
                        if (bytesread != size) {
                            byte[] buf = new byte[bytesread];
                            System.arraycopy(b, 0, buf, 0, bytesread);
                            return buf;
                        }
                        return b;
                    }
                    bytesread += len;
                    if (bytesread == size) {
                        size = 2 * size;
                        byte[] buf = new byte[size];
                        System.arraycopy(b, 0, buf, 0, b.length);
                        b = buf;
                    }
                }
            }
        };
        
        
        if (clsNameList.size() == 0) {
            System.out.println("At least one class name must be provided in the command line");
            System.exit(1);
        }
        
        try {
            for (int i = 0; i < clsNameList.size(); i++) {
                String clsName = (String)clsNameList.elementAt(i);
                try {
                    Class c = Class.forName(clsName, false, classLoader);
                    clsList.addElement(c);
                } catch (ClassNotFoundException cnfe) {
                    //cnfe.printStackTrace();
                    pl("Class '" + clsName + "' not found. Skipped.");
                }
            }
    
            classes = new String[clsList.size()];
            for (int i = 0; i < classes.length; i++) {
                classes[i] = ((Class)clsList.elementAt(i)).getName();
            }
            Hashtable agentPackages = new Hashtable();
            Hashtable proxyPackages = new Hashtable();
            for (int clsIndex = 0; clsIndex < clsList.size(); clsIndex++) {
                Class clazz = (Class)clsList.elementAt(clsIndex);
                String className = clazz.getName();
                String pkg = (String)pkgNames.get(className);
                boolean isIface = clazz.isInterface();
                if (emptyList.containsKey(className)) {
                    JavaFile wr = new JavaFile(tmpDir, translateClassName(className), doTrace);
                    printHeader(wr, pkg);
                    wr.pl("public " + (isIface ? "interface" : "class") + " " + translateClassName(className)+ " {");
                    wr.pl("}");
                    wr.close();
                    continue;
                }
                if (inheritList.containsKey(className)) {
                    if (printAgentClass(clazz, pkg, doTrace, isIface)) {
                        if (isIface) {
                            printProxyClass(clazz, translateInterfaceImplName(className), false, doTrace, true);
                        }
                        printProxyClass(clazz, translateClassName(className), isIface, doTrace, true);
                        agentPackages.put(pkg, "");
                    }
                } else {
                    if (isIface) {
                        printProxyClass(clazz, translateInterfaceImplName(className), false, doTrace, false);
                    }
                    printProxyClass(clazz, translateClassName(className), isIface, doTrace, false);
                }
                proxyPackages.put(pkg, "");
            }
            JavaFile wr;
            for (Enumeration e = agentPackages.keys(); e.hasMoreElements();) {
                String pkg = (String)e.nextElement();
                String prefix = pkg.replaceAll("\\.", "_");
                wr = new JavaFile(agentDir, prefix + "__AgentManager", doTrace);
                printHeader(wr, pkg);
                wr.pl("public class " + prefix + "__AgentManager {");
                wr.pl(1, "private static java.util.Hashtable classList = new java.util.Hashtable();");
                wr.pl(1, "public static void registerClass(Object homeClass, String className) {");
                wr.pl(2, "classList.put(className, homeClass);");
                wr.pl(1, "}");
                wr.pl(1, "public static Object getHomeClass(String className) {");
                wr.pl(2, "Object homeClass = classList.get(className);");
                wr.pl(2, "if (homeClass == null) {");
                wr.pl(3, "try{");
                wr.pl(4, "Class.forName(className + \"__AgentClass\");"); 
                wr.pl(3, "} catch (ClassNotFoundException cnfe) {");
                wr.pl(4, "throw new RuntimeException(cnfe);");
                wr.pl(3, "}");
                wr.pl(2, "}");
                wr.pl(2, "homeClass = classList.get(className);");
                wr.pl(2, "if (homeClass == null) {");
        wr.pld(4,"System.out.println(\"home class not found\");");
                wr.pl(3, "throw new RuntimeException(\"Cannot load agent class \" + className);");
                wr.pl(2, "}");
                wr.pl(2, "return homeClass;");
                wr.pl(1, "}");
                wr.pl("}");
                wr.close();
                wr = new JavaFile(agentDir, prefix + "__AgentInterface", doTrace);
                printHeader(wr, pkg);
                wr.pl("public interface " + prefix + "__AgentInterface {");
                wr.pl(1, "public Object __invoke(int num, Object obj, Object[] a) throws InvocationTargetException;");
                wr.pl(1, "public void __setClassLoader(java.lang.ClassLoader cl);");
                wr.pl("}");
                wr.close();
            }
            for (Enumeration e = proxyPackages.keys(); e.hasMoreElements();) {
                String pkg = (String)e.nextElement();
                String className = pkg.replaceAll("\\.", "_") + "__ProxyBase";
                wr = new JavaFile(tmpDir, className, doTrace);
                printHeader(wr, pkg);
                wr.pl("public class " + className + " {");
                wr.pl(1, "// Internal instance variable");
                wr.pl(1, "private Object __internal = null;");
                wr.pl(1, "// Get internal instance");
                wr.pl(1, "public Object __getInternal() {");
        wr.pld(2,"System.out.println(this.getClass().getName() + \".__getInternal() = \" + (__internal == null ? \"null\" : \"not null\") + \"\");");
                wr.pl(2, "return __internal;");
                wr.pl(1, "}");
                wr.pl(1, "protected void __setInternal(Object obj) {");
                wr.pl(2, "__internal = obj;");
        wr.pld(2,"System.out.println(this.getClass().getName() + \".__setInternal() = \" + (__internal == null ? \"null\" : \"not null\") + \"\");");
        //wr.pld(2, "Thread.dumpStack();");
                wr.pl(1, "}");
                wr.pl(1, "// List of used instances ");
                wr.pl(1, "protected static java.util.Hashtable __objList = new java.util.Hashtable();");
                wr.pl("}");
                wr.close();
            }
        } catch (Exception e) {
            System.out.println("Unexpected exception:");
            e.printStackTrace();
            System.exit(1);
        }
        System.exit(0);
    }
    
    private static boolean printAgentClass(Class clazz, String pkgName, boolean doTrace, boolean isIface) {
        String fullClsName = clazz.getName();
        String pkg;
        int lastDot = fullClsName.lastIndexOf('.');
        if (lastDot != -1) {
            pkg = fullClsName.substring(0, lastDot);
        } else {
            pkg = null;
        }
        String prefix = ((String)pkgNames.get(fullClsName)).replaceAll("\\.", "_");
        String stubClsName = translateAgentClass(fullClsName);
        Field[] fields = null;
        Constructor[] constructors = null;
        Method[] methods = null;
        
        boolean allLinksOk = false;
        try {
            fields = clazz.getDeclaredFields();
            constructors = clazz.getDeclaredConstructors();
            methods = clazz.getDeclaredMethods();
            allLinksOk = true;
        } catch (UnsatisfiedLinkError ule) {
            ule.printStackTrace();
        } catch (NoClassDefFoundError ncdfe) {
            ncdfe.printStackTrace();
        }
        if (!allLinksOk) {
            pl("Linkage error in class " + fullClsName + ". Skipped");
            return false;
        }
        Member mems[] = buildMemberList(methods, constructors, true);

        if (Modifier.isFinal(clazz.getModifiers())) {
            pl(fullClsName + ": is final. Skipped.");
            return false;
        }
        if (constructors != null && constructors.length > 0) {
            int suitableConstructors = 0;
            for (int i = 0; i < constructors.length; i++) {
                if (!Modifier.isPrivate(constructors[i].getModifiers())) {
                    suitableConstructors++;
                }
            }
            if (suitableConstructors == 0) {
                pl(fullClsName + ": has no suitable constructors. Skipped.");
                return false;
            }
        }
        JavaFile wr = new JavaFile(agentDir, stubClsName, doTrace);
        
        printHeader(wr, pkg);
        wr.pl("// Agent class for " + fullClsName);
        
        wr.p("public class " + stubClsName);
        
        wr.pl(" {");
        wr.p(1, "private static class Child ");
        if (isIface) {
            wr.p("implements " + 
                "/*" + Modifier.toString(clazz.getModifiers())+ "*/ " + 
                fullClsName);
        } else {
            wr.p("extends " + 
                "/*" + Modifier.toString(clazz.getModifiers())+ "*/ " + 
                fullClsName);
        }
        wr.pl(" {");
        wr.pl(2, "private Object __external = null;");
        wr.pl(2, "public void __setExternal(Object obj) {");
        wr.pl(3, "__external = obj;");
        wr.pl(2, "}");
        /*for (int i = 0; i < fields.length; i++) {
            String typeName = decodeType(fields[i].getType().getName());
            String mod = Modifier.toString(fields[i].getModifiers());
            String fieldName = fields[i].getName();
            int modif = fields[i].getModifiers();
            if (Modifier.isPrivate(modif) || 
                    !Modifier.isStatic(modif) || 
                    !Modifier.isFinal(modif)) {
                // wr.pl(1, "// " + mod + " " + typeName + " " + fieldName + ";");
                continue;
            }
            wr.pl(2, "public " + typeName + " __getField_" + fieldName + "() {");
            wr.pl(3, "return " + fullClsName + "." + fieldName + ";");
            wr.pl(2, "}");
        } */
        if (!isIface) {
            for (int i = 0; i < mems.length; i++) {
                if (!isGoodMember(mems[i])) {
                    continue;
                }
                if (!(mems[i] instanceof Constructor)) {
                    continue;
                }
                Constructor constructor = (Constructor)mems[i];
                int modif = mems[i].getModifiers();
                wr.pl(2, "// " + constructor.toString());
                Class[] pars = constructor.getParameterTypes();
                Class[] exc = constructor.getExceptionTypes();
                wr.p(2, "public Child(");
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    String typeName = decodeType(pars[j].getName());
                    wr.p(typeName + " ");
                    wr.p("arg" + (int)(j + 1));
                }
                wr.p(")");
                if (exc != null && exc.length > 0) {
                    wr.p(" throws ");
                    for (int j = 0; j < exc.length; j++) {
                        if (j > 0) {
                            wr.p(", ");
                        }
                        String exName = exc[j].getName();
                        wr.p(exName);
                    }
                }
                wr.pl(" {");
                wr.p(3, "super(");
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    wr.p("arg" + (int)(j + 1));
                }
                wr.pl(");");
                wr.pl(2, "}");
            }
        }
        for (int i = 0; i < mems.length; i++) {
            if (!isGoodMember(mems[i])) {
                continue;
            }
            if (!(mems[i] instanceof Method)) {
                continue;
            }
            Method method = (Method)mems[i];
            int modif = mems[i].getModifiers();
            wr.pl(2, "// " + method.toString());
            String methodName = method.getName();
            String retType = decodeType(method.getReturnType().getName());
            Class[] pars = method.getParameterTypes();
            Class[] exc = method.getExceptionTypes();
            if (!isIface) {
                wr.p(2, "public ");
                if (Modifier.isStatic(modif)) {
                    wr.p("static ");
                }
                wr.p(retType + " "); 
                wr.p("__invoke_" + methodName + "(");
                
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    String typeName = decodeType(pars[j].getName());
                    wr.p(typeName + " ");
                    wr.p("arg" + (int)(j + 1));
                }
                wr.p(")");
                
                if (exc != null && exc.length > 0) {
                    wr.p(" throws ");
                    for (int j = 0; j < exc.length; j++) {
                        if (j > 0) {
                            wr.p(", ");
                        }
                        String exName = exc[j].getName();
                        wr.p(exName);
                    }
                }
                wr.pl(" {");
                if (Modifier.isAbstract(modif)) {
                    wr.pl(3, "throw new RuntimeException(\"Abstract method '" + fullClsName + "." + methodName + "' was called\");");
                } else {
                    if (!retType.equals("void")) {
                        wr.p(3, "return ");
                    } else {
                        wr.p(3, "");
                    }
                    if (Modifier.isStatic(modif)) {
                        wr.p(fullClsName + ".");
                    } else {
                        wr.p("super.");
                    }
                    wr.p(methodName + "(");
                    for (int j = 0; j < pars.length; j++) {
                        if (j > 0) {
                            wr.p(", ");
                        }
                        wr.p("arg" + (int)(j + 1));
                    }
                    wr.pl(");");
                }
                wr.pl(2, "}");
            }
            if (!isIface) {
                if (Modifier.isFinal(modif)) {
                    continue;
                }
                if (!Modifier.isPublic(modif) && !Modifier.isProtected(modif)) {
                    continue;
                }
            }
            wr.p(2, Modifier.toString(modif).
                    replaceAll("abstract", "/*abstract*/").
                    replaceAll("native", "/*native*/") + " ");
            wr.p(retType + " " + methodName + "(");
            for (int j = 0; j < pars.length; j++) {
                if (j > 0) {
                    wr.p(", ");
                }
                String typeName = decodeType(pars[j].getName());
                wr.p(typeName + " ");
                wr.p("arg" + (int)(j + 1));
            }
            wr.p(")");
            if (exc != null && exc.length > 0) {
                wr.p(" throws");
                for (int j = 0; j < exc.length; j++) {
                    wr.p(" " + exc[j].getName());
                }
            }
            wr.pl(" {");
            if (!retType.equals("void")) {
                wr.pl(3, "Object result;");
            }
            wr.pl(3, "try {");
            if (!retType.equals("void")) {
                wr.p(4, "result = ");
            } else {
                wr.p(4, "");
            }
            /*if (clazz.isInterface()) {
                wr.p("Home.getMethod" + (int)(i+1) + "().invoke(");
                if (Modifier.isStatic(modif)) {
                    wr.p("null, ");
                } else {
                    wr.p("__external, ");
                }
                wr.p("new Object[] {");
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    printActualArg4(wr, pars[j], "arg" + (int)(j + 1));
                }
                wr.p("}");
                wr.pl(");");
            } else */{
                wr.p("Home.getCallbackMethod().invoke(null, ");
                wr.p("new Object[] {new Integer(" + i + "), ");
                if (Modifier.isStatic(modif)) {
                    wr.p("null, ");
                } else {
                    wr.p("__external, ");
                }
                wr.p("new Object[] {");
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    printActualArg4(wr, pars[j], "arg" + (int)(j + 1));
                }
                wr.p("}}");
                wr.pl(");");
            }
            wr.pl(3, "} catch (IllegalAccessException iae) {");
            wr.pld(4, "iae.printStackTrace();");
            wr.pl(4, "throw new RuntimeException(iae.toString());");
            wr.pl(3, "} catch (InvocationTargetException ite) {");
            wr.pl(4, "Throwable th = ite.getTargetException();");
            for (int j = 0; j < exc.length; j++) {
                wr.pl(4, "if (th instanceof " + exc[j].getName() + ") {");
                wr.pl(5, "throw (" + exc[j].getName() + ")th;");
                wr.pl(4, "}");
            }
            wr.pld(4, "th.printStackTrace();");
            wr.pl(4, "throw new RuntimeException(\"Unreported exception:\" + th.toString());");
            wr.pl(3, "}");
            if (!retType.equals("void")) {
                wr.p(3, "return ");
                printActualArg5(wr, method.getReturnType(), "result");
                wr.pl(";");
            }
            wr.pl(2, "}");
        }
        wr.pl(1, "}");
        wr.pl(1, "private static class Home implements " + pkgName + "." + prefix + "__AgentInterface {");
        /*for (int i = 0; i < fields.length; i++) {
            String typeName = decodeType(fields[i].getType().getName());
            String mod = Modifier.toString(fields[i].getModifiers());
            String fieldName = fields[i].getName();
            int modif = fields[i].getModifiers();
            if (Modifier.isPrivate(modif) || 
                    !Modifier.isStatic(modif) || 
                    !Modifier.isFinal(modif)) {
                // wr.pl(1, "// " + mod + " " + typeName + " " + fieldName + ";");
                continue;
            }
            wr.pl(2, "public static " + typeName + " __getField_" + fieldName + "() {");
            wr.pl(3, "return " + fullClsName + "." + fieldName + ";");
            wr.pl(2, "}");
        }*/
        wr.pl(2, "public Object __invoke(int num, Object obj, Object a[]) throws InvocationTargetException {");
        wr.pl(3, "try {");
        wr.pl(4, "switch (num) {");
        wr.pl(5, "default: ");
        wr.pl(6, "throw new RuntimeException(\"Illegal invocation code \" + num);");
        if (isIface) {
            wr.pl(5, "case -1: {");
            wr.pl(6, "Child ch = new Child();");
            wr.pl(6, "ch.__setExternal(obj);");
wr.pld(5,"System.out.println(\"Trying to call __invoke -1 - " + fullClsName + "\");");
            wr.pl(6, "return ch;");
            wr.pl(5, "}");
        } else {
            for (int i = 0; i < mems.length; i++) {
                if (!isGoodMember(mems[i])) {
                    continue;
                }
                wr.pl(5, "case " + i + ": {");
wr.pld(5,"System.out.println(\"Trying to call __invoke" + i + " " + mems[i].toString() + "\");");
                if (mems[i] instanceof Constructor) {
                    Constructor constructor = (Constructor)mems[i];
                    int modif = mems[i].getModifiers();
                    wr.pl(5, "// " + constructor.toString());
                    Class[] pars = constructor.getParameterTypes();
                    Class[] exc = constructor.getExceptionTypes();
                    wr.p(6, "Child ch = new Child(");
                    for (int j = 0; j < pars.length; j++) {
                        String parType = decodeType(pars[j].getName());
                        if (j > 0) {
                            wr.p(", ");
                        }
                        if (pars[j].isPrimitive()) {
                            wr.p("((" + wrapperName(parType) + ")a[" + j + "])." + parType + "Value()");
                        } else {
                            wr.p("(" + parType + ")a[" + j + "]");
                        }
                    }
                    wr.pl(");");
                    wr.pl(6, "ch.__setExternal(obj);");
                    wr.pl(6, "return ch;");
                } else {
                    Method method = (Method)mems[i];
                    int modif = mems[i].getModifiers();
                    wr.pl(5, "// " + method.toString());
                    String methodName = method.getName();
                    String retType = decodeType(method.getReturnType().getName());
                    Class[] pars = method.getParameterTypes();
                    Class[] exc = method.getExceptionTypes();
                    if (!retType.equals("void")) {
                        wr.pl(6, retType + " result;");
                    }
                    wr.pl(6, "if (obj instanceof Child) {");
                    if (!retType.equals("void")) {
                        wr.p(7, "result = ");
                    } else {
                        wr.p(7, "");
                    }
                    if (Modifier.isStatic(modif)) {
                        wr.p("Child.");
                    } else {
                        wr.p("((Child)obj).");
                    }
                    wr.p("__invoke_" + methodName + "(");
                    for (int j = 0; j < pars.length; j++) {
                        String parType = decodeType(pars[j].getName());
                        if (j > 0) {
                            wr.p(", ");
                        }
                        if (pars[j].isPrimitive()) {
                            wr.p("((" + wrapperName(parType) + ")a[" + j + "])." + parType + "Value()");
                        } else {
                            wr.p("(" + parType + ")a[" + j + "]");
                        }
                    }
                    wr.pl(");");
                    wr.pl(6, "} else { /* !(obj instanceof Child) */");
                    if (!retType.equals("void")) {
                        wr.p(7, "result = ");
                    } else {
                        wr.p(7, "");
                    }
                    if (Modifier.isStatic(modif)) {
                        wr.p(fullClsName + ".");
                    } else {
                        wr.p("((" + fullClsName + ")obj).");
                    }
                    wr.p(methodName + "(");
                    for (int j = 0; j < pars.length; j++) {
                        String parType = decodeType(pars[j].getName());
                        if (j > 0) {
                            wr.p(", ");
                        }
                        if (pars[j].isPrimitive()) {
                            wr.p("((" + wrapperName(parType) + ")a[" + j + "])." + parType + "Value()");
                        } else {
                            wr.p("(" + parType + ")a[" + j + "]");
                        }
                    }
                    wr.pl(");");
                    wr.pl(6, "}");
                    if (!retType.equals("void")) {
                        if (method.getReturnType().isPrimitive()) {
                            wr.pl(6, "return (Object)(new " + wrapperName(retType) + "(result));");
                        } else {
                            wr.pl(6, "return (Object)result;");
                        }
                    } else {
                        wr.pl(6, "return null;");
                    }
                }
                wr.pl(5, "}");
            }
        }
        wr.pl(4, "} /* end of switch */");
        wr.pl(3, "} catch (RuntimeException re) {");
        wr.pl(4, "throw re;");
        wr.pl(3, "} catch (Exception e) {");
        wr.pl(4, "throw new InvocationTargetException(e);");
        wr.pl(3, "}");
        wr.pl(2, "}");
        wr.pl(2, "public void __setClassLoader(java.lang.ClassLoader cl) {");
        wr.pl(3, "classLoader = cl;");
        wr.pl(2, "}");
        /*if (clazz.isInterface()) {
            for (int i = 0; i < mems.length; i++) {
                if (!isGoodMember(mems[i])) {
                    continue;
                }
                if (!(mems[i] instanceof Method)) {
                    continue;
                }
                Method method = (Method)mems[i];
                int modif = mems[i].getModifiers();
                wr.pl(2, "// " + method.toString());
                String methodName = method.getName();
                String retType = decodeType(method.getReturnType().getName());
                Class[] pars = method.getParameterTypes();
                Class[] exc = method.getExceptionTypes();
    
                wr.pl(2, "private static Method method" + (int)(i+1) + " = null;");
                wr.pl(2, "public static Method getMethod" + (int)(i+1) + "() {");
                wr.pl(3, "if (method" + (int)(i+1) + " == null) {");
                wr.pl(4, "try {");
                wr.p(5, "method" + (int)(i+1) + " = getExternalClass().getMethod(\"" + methodName + "\", new Class[] {");
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    wr.p(decodeType(pars[j].getName()) + ".class");
                }
                wr.pl("});");
                wr.pl(4, "} catch (NoSuchMethodException nsme) {");
                wr.pld(5, "nsme.printStackTrace();");
                wr.pl(5, "throw new RuntimeException(nsme.toString());");
                wr.pl(4, "}");
                wr.pl(3, "}");
                wr.pl(3, "return method" + (int)(i+1) + ";");
                wr.pl(2, "}");
            }
        } else */{
            wr.pl(2, "private static Method callbackMethod = null;");
            wr.pl(2, "public static Method getCallbackMethod() {");
            wr.pl(3, "if (callbackMethod == null) {");
                wr.pl(4, "java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {");
                wr.pl(5, "public Object run() {");
            wr.pl(6, "try {");
            wr.pl(7, "callbackMethod = getExternalClass().getMethod(\"__callback\", new Class[] {int.class, java.lang.Object.class, java.lang.Object[].class});");
            wr.pl(6, "} catch (NoSuchMethodException nsme) {");
            wr.pld(7, "nsme.printStackTrace();");
            wr.pl(7, "throw new RuntimeException(nsme.toString());");
            wr.pl(6, "}");
            wr.pl(6, "return null;");
            wr.pl(5, "} // run()");
            wr.pl(4, "}); // doPrivileged()");
            wr.pl(3, "}");
            wr.pl(3, "return callbackMethod;");
            wr.pl(2, "}");
        }
        wr.pl(2, "private static Class clazz = null;");
        wr.pl(2, "private static ClassLoader classLoader = null;");
        wr.pl(2, "public static Class getExternalClass() {");
        wr.pl(3, "if (clazz == null) {");
        wr.pl(4, "try {");
        wr.pl(5, "clazz = Class.forName(\"" + 
            (clazz.isInterface() ? translateInterfaceImplNamePkg(fullClsName) : translateClassNamePkg(fullClsName)) + 
            "\", true, classLoader);");
        wr.pl(4, "} catch (ClassNotFoundException cnfe) {");
        wr.pld(5, "cnfe.printStackTrace();");
        wr.pl(5, "throw new RuntimeException(cnfe.toString() + \" ClassLoader=\"+(classLoader==null?\"null\":classLoader.getClass().getName()),cnfe);");
        wr.pl(4, "}");
        wr.pl(3, "}");
        wr.pl(3, "return clazz;");
        wr.pl(2, "}");
        wr.pl(1, "}");
        wr.pl(1, "private static Home __instance = null;");
        wr.pl(1, "private " + stubClsName + "() {");
        wr.pl(1, "// default empty constructor");
        wr.pl(1, "}");
        wr.pl(1, "static {");
        wr.pld(2,"System.out.println(\"Static initializer: " + stubClsName + "\");");
        wr.pl(2, "__instance = new Home();");
        wr.pl(2, pkgName + "." + prefix + "__AgentManager.registerClass(__instance, \"" + fullClsName + "\");");
        wr.pl(1, "}");
        wr.pl("}");
        wr.close();
        return true;
    }
    
    private static boolean isGoodMember(Member mem) {
        boolean hasDollar = false;
        if (mem instanceof Method) {
            Method met = (Method)mem;
            String methodName = met.getName();
            if (methodName.indexOf('$') != -1) {
                hasDollar = true;
            }
            String retType = decodeType(met.getReturnType().getName());
            if (retType.indexOf('$') != -1) {
                hasDollar = true;
            }
        }
        Class[] pars;
        if (mem instanceof Method) {
            pars = ((Method)mem).getParameterTypes();
        } else
        if (mem instanceof Constructor) {
            pars = ((Constructor)mem).getParameterTypes();
        } else {
            pars = new Class[0];
        }
        
        for (int j = 0; j < pars.length; j++) {
            if (decodeType(pars[j].getName()).indexOf('$') != -1) {
                hasDollar = true;
                break;
            }
        }
        Class[] exc;
        if (mem instanceof Method) {
            exc = ((Method)mem).getExceptionTypes();
        } else
        if (mem instanceof Constructor) {
            exc = ((Constructor)mem).getExceptionTypes();
        } else {
            exc = new Class[0];
        }
        for (int j = 0; j < exc.length; j++) {
            if (exc[j].getName().indexOf('$') != -1) {
                hasDollar = true;
                break;
            }
        }
        return !hasDollar;
    }


    private static Member[] buildMemberList(Method[] methods, Constructor[] constructors, boolean isInherit) {
        Vector methodsAndConsList = new Vector();
        if (methods != null && methods.length > 0) {
            for (int i = 0; i < methods.length; i++) {
                if (Modifier.isPublic(methods[i].getModifiers()) ||
                        (isInherit && !Modifier.isPrivate(methods[i].getModifiers()))) {
                    if (methods[i].getName().equals("finalize")) {
                        Class[] pars = methods[i].getParameterTypes();
                        if (pars == null || pars.length == 0) {
                            continue;
                        }
                    }
                    methodsAndConsList.addElement(methods[i]);
                }
            }
        }
        if (constructors != null && constructors.length > 0) {
            for (int i = 0; constructors != null && i < constructors.length; i++) {
                if (Modifier.isPublic(constructors[i].getModifiers()) ||
                        (isInherit && !Modifier.isPrivate(constructors[i].getModifiers()))) {
                    methodsAndConsList.addElement(constructors[i]);
                }
            }
        }
        Member mems[] = new Member[methodsAndConsList.size()];
        methodsAndConsList.toArray(mems);
        return mems;
    }
    
    private static void printProxyClass(Class clazz, String stubClsName, boolean isIface, boolean doTrace, boolean isInherit) {
        String fullClsName = clazz.getName();
        String pkgName = (String)pkgNames.get(fullClsName);
        boolean initStaticFlag = false;
        boolean isDerivedClass = false;
        boolean isException = false;
        String prefix = pkgName.replaceAll("\\.", "_");

        Class bclass = null;
        Class[] ifaces = null;
        Field[] fields = null;
        Constructor[] constructors = null;
        Method[] methods = null;
        
        boolean allLinksOk = false;
        try {
            bclass = clazz.getSuperclass();
            ifaces = clazz.getInterfaces();
            fields = clazz.getDeclaredFields();
            constructors = clazz.getDeclaredConstructors();
            methods = clazz.getDeclaredMethods();
            allLinksOk = true;
        } catch (UnsatisfiedLinkError ule) {
            ule.printStackTrace();
        } catch (NoClassDefFoundError ncdfe) {
            ncdfe.printStackTrace();
        }
        if (!allLinksOk) {
            pl("Linkage error in class " + fullClsName + ". Skipped");
            return;
        }
        
        Member mems[] = buildMemberList(methods, constructors, isInherit);
        typeList.clear();
        typeList.put(fullClsName, "");
        if (bclass != null) {
            if (isTranslatedClass(bclass.getName())) {
                isDerivedClass = true;
            }
        }
        
        JavaFile wr = new JavaFile(tmpDir, stubClsName, doTrace);
        
        printHeader(wr, pkgName);
        wr.pl("// Proxy class for " + fullClsName);
        
        if (isIface) {
            wr.p("public interface " + stubClsName);
        } else {
            wr.p(Modifier.toString(clazz.getModifiers()).
                replaceAll("abstract", "/* abstract */").
                replaceAll("interface", "") +
                " class " + stubClsName);
        }
        String proxyBase = pkgName.replaceAll("\\.", "_") + "__ProxyBase";
        if (clazz.isInterface() && !isIface) {
            wr.p(" extends " + proxyBase);
            wr.p(" implements " + translateClassNamePkg(clazz.getName()));
        } else {
            if (bclass != null && !bclass.getName().equals("java.lang.Object")) {
                String bname = bclass.getName();
                if (java.lang.Throwable.class.isAssignableFrom(bclass)) {
                    isException = true;
                }
                if (isTranslatedClass(bname)) {
                    bname = translateClassNamePkg(bname);
                }
                wr.p(" extends " + bname);
            } else {
                if (!isIface) {
                    wr.p(" extends " + proxyBase);
                }
            }
            if (ifaces.length > 0) {
                wr.p(" implements ");
                for (int i = 0; i < ifaces.length; i++) {
                    if (i > 0) {
                        wr.p(", ");
                    }
                    String iname = ifaces[i].getName();
                    if (isTranslatedClass(iname)) {
                        iname = translateClassNamePkg(iname);
                    }
                    wr.p(iname);
                }
            }
        }
        wr.pl(" {");
        if (isException) {
            wr.pl(1, "public " + stubClsName + "() {");
            wr.pl(2, "super();");
            wr.pl(1, "}");
            wr.pl(1, "public " + stubClsName + "(String s) {");
            wr.pl(2, "super(s);");
            wr.pl(1, "}");
            wr.pl(1, "public " + stubClsName + "(Throwable t) {");
            wr.pl(2, "super(t);");
            wr.pl(1, "}");
            wr.pl(1, "public " + stubClsName + "(String s, Throwable t) {");
            wr.pl(2, "super(s, t);");
            wr.pl(1, "}");
            wr.pl("}");
            wr.close();
            return;
        }
        
        Hashtable fieldMethods = new Hashtable();
        if (fields != null && fields.length > 0) {
            //wr.pl(1, "// Public fields");
            for (int i = 0; i < fields.length; i++) {
                String typeName = decodeType(fields[i].getType().getName());
                String mod = Modifier.toString(fields[i].getModifiers());
                String fieldName = fields[i].getName();
                int modif = fields[i].getModifiers();
                if (!Modifier.isPublic(modif) || 
                        !Modifier.isStatic(modif) || 
                        !Modifier.isFinal(modif)) {
                    // wr.pl(1, "// " + mod + " " + typeName + " " + fieldName + ";");
                    continue;
                }
                if ((!fields[i].getType().isPrimitive() && !typeName.equals("java.lang.String"))) {
                    int k = findTranslatedClass(classes, typeName);
                    if (k != -1) {
                        typeList.put(typeName, "");
                        typeName = translateClassNamePkg(typeName) + getBrackets(typeName);
                        wr.pl(1, mod + " " + typeName + " " + fieldName + 
                            " = new " + typeName + "(__getMyClass(), \"" + fieldName + "\", null);");
                    } else {
                        wr.pl(1, "// " + mod + " " + typeName + " " + fieldName + ";");
                    }
                    continue;
                } 
                wr.p(1, mod + " " + typeName + " " + fieldName + " = ");
                String getFieldMethod = null;
                try {
                    if (typeName.equals("java.lang.String")) {
                        getFieldMethod = "get";
                        wr.p("\"" + (String)fields[i].get(null) + "\"");
                        getFieldMethod = null;
                    } else
                    if (typeName.equals("int")) {
                        getFieldMethod = "getInt";
                        wr.p("" + fields[i].getInt(null));
                        getFieldMethod = null;
                    } else
                    if (typeName.equals("boolean")) {
                        getFieldMethod = "getBoolean";
                        wr.p("" + fields[i].getBoolean(null));
                        getFieldMethod = null;
                    } else
                    if (typeName.equals("long")) {
                        getFieldMethod = "getLong";
                        wr.p("" + fields[i].getLong(null) + "L");
                        getFieldMethod = null;
                    } else
                    if (typeName.equals("short")) {
                        getFieldMethod = "getShort";
                        wr.p("" + fields[i].getShort(null));
                        getFieldMethod = null;
                    } else
                    if (typeName.equals("char")) {
                        getFieldMethod = "getChar";
                        wr.p("'" + fields[i].getChar(null) + "'");
                        getFieldMethod = null;
                    } else
                    if (typeName.equals("byte")) {
                        getFieldMethod = "getByte";
                        wr.p("(byte)" + fields[i].getByte(null));
                        getFieldMethod = null;
                    } else {
                        wr.p("/* Invalid value */");
                    }
                } catch (Throwable th) {} // ignored
                if (getFieldMethod != null) {
                    initStaticFlag = true;
                    fieldMethods.put(getFieldMethod, typeName);
                    if (isIface) {
                        wr.p(translateInterfaceImplNamePkg(stubClsName) + ".");
                    }
                    if (typeName.equals("java.lang.String")) {
                        wr.p("(String)");
                    }
                    wr.p("__" + getFieldMethod + "Value(\"" + fieldName + "\")");
                    // wr.p("__clazz.getField(\"" + fieldName + "\")." + getFieldMethod + "(null)");
                }
                wr.pl(";");
            }
            
        }
        
        //wr.pl(1, "// Internal stuff");
        if (!isIface) {
            if (mems.length == 0 && !initStaticFlag && !(clazz.isInterface() && !isIface) && !isInherit) {
                wr.pl(1, "private static void __init() {");
                wr.pl(1, "}");
            } else {
                if (!isInherit || clazz.isInterface()) {
                    for (int i = 0; i < mems.length; i++) {
                        if (mems[i] instanceof Constructor) {
                            wr.pl(1, "private static Constructor __cons" + (int)(i + 1) + ";");
                        } else {
                            wr.pl(1, "private static Method __method" + (int)(i + 1) + ";");
                        }
                    }
                }
                wr.pl(1, "private static ClassLoader __cloader;");
                wr.pl(1, "private static boolean __initialized = false;");
                wr.pl(1, "private static Object __sync;");
                wr.pl(1, "// Initialize class");
                wr.pl(1, "private static void __init() {");
wr.pld(4,"System.out.println(\"class " + stubClsName + " - Init\");");
                wr.pl(2, "if (__sync == null) {");
                wr.pl(3, "__sync = new Object();");
                wr.pl(2, "}");
                wr.pl(2, "if (__initialized) {");
                wr.pl(3, "return;");
                wr.pl(2, "}");
                wr.pl(2, "synchronized (__sync) {");
                wr.pl(3, "if (__initialized) {");
                wr.pl(4, "return;");
                wr.pl(3, "}");
wr.pld(4,"System.out.println(\"Init 1\");");
                wr.pl(3, "java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {");
                wr.pl(4, "public Object run() {");
                wr.pl(5, "__cloader = sun.misc.MIDPConfig.getMIDPImplementationClassLoader();");
                wr.pl(5, "if (__cloader == null) {");
                wr.pl(6, "throw new RuntimeException(\"Cannot get ClassLoader\");");
                wr.pl(5, "}");
                wr.pl(5, "__init_classes();");
                if (isInherit) {
                    wr.pl(5, "__init_agent();");
                }
                if (!isInherit || (!isIface && clazz.isInterface())) {
                    if (mems.length > 0) {
                        wr.pl(5, "try {");
                        for (int i = 0; i < mems.length; i++) {
                            if (!isGoodMember(mems[i])) {
                                continue;
                            }
                            if (!(mems[i] instanceof Method)) {
                                continue;
                            }
                            Method met = (Method)mems[i];
    //wr.pld(4,"System.out.println(\"Method __method" + (int)(i + 1)+"\");");
                            wr.p(6, "__method" + (int)(i + 1) + " = __clazz.getDeclaredMethod(\"" + met.getName() + "\", new Class[] {");
                            Class pars[] = met.getParameterTypes();
                            if (pars != null && pars.length > 0) {
                                for (int j = 0; j < pars.length; j++) {
                                    if (j > 0) {
                                        wr.p(", ");
                                    }
                                    String typeName = pars[j].getName();
                                    typeName = decodeType(typeName);
                                    int k = findTranslatedClass(classes, typeName);
                                    if (k != -1) {
                                        typeList.put(typeName, "");
                                        wr.p("__clazz" + (int)(k + 1));
                                    } else {
                                        wr.p(typeName + ".class");
                                    }
                                }
                            }
                            wr.pl("});");
                        }
                        for (int i = 0; i < mems.length; i++) {
                            if (!isGoodMember(mems[i])) {
                                continue;
                            }
                            if (!(mems[i] instanceof Constructor)) {
                                continue;
                            }
                            Constructor cons = (Constructor)mems[i];
    //wr.pld(4,"System.out.println(\"Constructor __cons" + (int)(i + 1)+"\");");
                            wr.p(6, "__cons" + (int)(i + 1) + " = __clazz.getDeclaredConstructor(new Class[] {");
                            Class pars[] = cons.getParameterTypes();
                            if (pars != null && pars.length > 0) {
                                for (int j = 0; j < pars.length; j++) {
                                    if (j > 0) {
                                        wr.p(", ");
                                    }
                                    String typeName = pars[j].getName();
                                    typeName = decodeType(typeName);
                                    int k = findTranslatedClass(classes, typeName);
                                    if (k != -1) {
                                        typeList.put(typeName, "");
                                        wr.p("__clazz" + (int)(k + 1));
                                    } else {
                                        wr.p(typeName + ".class");
                                    }
                                }
                            }
                            wr.pl("});");
                        }
                        wr.pl(5, "} catch (NoSuchMethodException e) {");
                        wr.pld(6, "e.printStackTrace();");
                        wr.pl(6, "throw new RuntimeException(e);");
                        wr.pl(5, "}");
                    }
                }
                wr.pl(4, "return null;");
                wr.pl(4, "}");
                wr.pl(3, "});");
                wr.pl(3, "__initialized = true;");
                wr.pl(2, "}");
                wr.pl(1, "}");
            }

            wr.pl(1, "// getInstance method");
            wr.pl(1, "public static Object __getInstance(Object internal) {");
wr.pld(0,"System.out.println(\"" + stubClsName + ".__getInstance()\");");
            wr.pl(2, "__init();");
            wr.pl(2, "if (internal == null) {");
            wr.pl(3, "return null;");
            wr.pl(2, "}");
            wr.pl(2, "Object saved = __objList.get(internal);");
            wr.pl(2, "if (saved != null) {");
            wr.pl(3, "saved = ((java.lang.ref.WeakReference)saved).get();");
            wr.pl(3, "if (saved != null) {");
wr.pld(0,"System.out.println(\"" + stubClsName + ".__getInstance() - instance found \");");
            wr.pl(4, "return saved;");
            wr.pl(3, "} else {");
wr.pld(0,"System.out.println(\"" + stubClsName + ".__getInstance() - WeakReference freed\");");
            wr.pl(4, "__objList.remove(internal);");
            wr.pl(3, "}");
            wr.pl(2, "}");
            wr.pl(2, "return new "+ stubClsName +"(" + translateClassNamePkg(clazz.getName()) + ".class, null, internal);");
            wr.pl(1, "}");
            wr.pl(1, "// Internal constructor");
            wr.pl(1, "public " + stubClsName + "(final Class cls, final String fieldName, final Object obj) {");
            if (isDerivedClass) {
                wr.pl(2, "super(cls, fieldName, obj);");
            }
wr.pld(0,"System.out.println(\"" + stubClsName + "." + stubClsName + "() - internal constructor, field=\" + (fieldName == null ? \"null\" : \"'\" + fieldName + \"'\"));");
            wr.pl(2, "__init();");
            wr.pl(2, "if (fieldName == null) {");
            wr.pl(3, "__setInternal(obj);");
            wr.pl(2, "} else {");
                wr.pl(3, "java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {");
                wr.pl(4, "public Object run() {");
            wr.pl(5, "try {");
            wr.pl(6, "__setInternal(cls.getDeclaredField(fieldName).get(obj));");
            wr.pl(5, "} catch (NoSuchFieldException nsfe) {");
            wr.pld(6, "nsfe.printStackTrace();");
            wr.pl(6, "throw new RuntimeException(nsfe);");
            wr.pl(5, "} catch (IllegalAccessException iae) {");
            wr.pld(6, "iae.printStackTrace();");
            wr.pl(6, "throw new RuntimeException(iae);");
            wr.pl(5, "}");
            wr.pl(4, "return null;");
            wr.pl(4, "} // run()");
            wr.pl(3, "}); // doPrivileged()");
            wr.pl(2, "}");
            wr.pl(2, "if (__getInternal() != null) {");
            wr.pl(3, "__objList.put(__getInternal(), new java.lang.ref.WeakReference(this));");
            wr.pl(2, "}");
            wr.pl(1, "}");
            wr.pl(1, "// Finalizer");
            wr.pl(1, "protected void finalize() {");
wr.pld(0,"System.out.println(\"" + stubClsName + ".finalize()\");");
            wr.pl(2, "__objList.remove(__getInternal());");
            wr.pl(1, "}");
            boolean defaultConstructorPresent = false;
            for (int i = 0; i < mems.length; i++) {
                if (!isGoodMember(mems[i])) {
                    continue;
                }
                if (!(mems[i] instanceof Constructor)) {
                    continue;
                }
                Constructor con = (Constructor)mems[i];
                Class[] consArgs = con.getParameterTypes();
                if (consArgs == null || consArgs.length == 0) {
                    defaultConstructorPresent = true;
                    break;
                }
            }
            if (!defaultConstructorPresent) {
                wr.pl(1, "// Default constructor (for derived classes)");
                wr.pl(1, "protected " + stubClsName + "() {");
wr.pld(4,"System.out.println(\"" + stubClsName + "." + stubClsName + "() - default constructor\");");
                wr.pl(2, "__init();");
                wr.pl(1, "}");
            }
        }
        for (int i = 0; i < mems.length; i++) {
            if (!isGoodMember(mems[i])) {
                continue;
            }
            boolean isCtor = (mems[i] instanceof Constructor);
            if (!isCtor || !isIface) {
                printMethod(wr, translateClassName(clazz.getName()), mems[i], i, isCtor, isIface, clazz.isInterface(), isInherit);
            }
        }
        if (!isIface && isInherit /* && !clazz.isInterface() */) {
            wr.pl(1, "public static Object __callback(int num, Object obj, Object a[]) throws InvocationTargetException {");
            wr.pl(2, "try {");
            wr.pl(3, "switch (num) {");
            wr.pl(4, "default: ");
            wr.pl(5, "throw new RuntimeException(\"Illegal invocation code \" + num);");
            for (int i = 0; i < mems.length; i++) {
                int modif = mems[i].getModifiers();
                if (!isGoodMember(mems[i])) {
                    continue;
                }
                if (mems[i] instanceof Constructor || Modifier.isStatic(modif)) {
                    continue;
                }
                wr.pl(4, "case " + i + ": {");
                Method method = (Method)mems[i];
                wr.pl(4, "// " + method.toString());
                String methodName = method.getName();
                String retType = decodeType(method.getReturnType().getName());
                Class[] pars = method.getParameterTypes();
                Class[] exc = method.getExceptionTypes();
wr.pld(4,"System.out.println(\"Trying to call __callback" + i + " " + mems[i].toString() + "\");");
                if (!retType.equals("void")) {
                    String rType = retType;
                    if (isTranslatedClass(retType)) {
                        rType = translateClassNamePkg(retType) + getBrackets(retType);
                    }
                    wr.p(5, rType + " result = ");
                } else {
                    wr.p(5, "");
                }
                wr.p("(("+ translateClassNamePkg(fullClsName) + ")obj)." + methodName + "(");
                for (int j = 0; j < pars.length; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    printActualArg2(wr, pars[j], "a[" + j + "]");
                }
                wr.pl(");");
                if (!retType.equals("void")) {
                    wr.p(5, "return ");
                    printActualArg0(wr, method.getReturnType(), "result");
                    wr.pl(";");
                } else {
                    wr.pl(5, "return null;");
                }
                wr.pl(4, "}");
            }
            wr.pl(3, "} /* end of switch */");
            wr.pl(2, "} catch (RuntimeException re) {");
            wr.pl(3, "throw re;");
            wr.pl(2, "} catch (Exception e) {");
            wr.pl(3, "throw new InvocationTargetException(e);");
            wr.pl(2, "}");
            wr.pl(1, "}");
        }
        if (clazz.isInterface() && !isIface) {
            String className = translateClassNamePkg(fullClsName);
            wr.pl(1, "public static Object __getProxyClass(" + className + " instance) {");
wr.pld(4,"System.out.println(\"" + stubClsName + ".__getProxyClass()\");");
            wr.pl(2, "__init();");
            if (!isInherit) {
                wr.pl(2, "final " + className + " fInstance = instance;");
                wr.pl(2, "Object proxy = ");
                printProxyInstance(new JavaFile(wr, 3), clazz, "fInstance");
                wr.pl(";");
                wr.pl(2, "__objList.put(proxy, new java.lang.ref.WeakReference(fInstance));");
            } else {
                wr.pl(2, "Object proxy;");
                wr.pl(2, "try {");
                wr.pl(3, "proxy = __invokeMethod.invoke(__homeClass, new Object[]{new Integer(-1), instance, new Object[0]});");
                wr.pl(2, "} catch (IllegalAccessException iae) {");
                wr.pl(3, "throw new RuntimeException(\"\" + iae.toString());");
                wr.pl(2, "} catch (InvocationTargetException ite) {");
                wr.pl(3, "Throwable th = ite.getTargetException();");
                wr.pl(3, "throw new RuntimeException(\"Unreported exception:\" + th.toString());");
                wr.pl(2, "}");
            }
            wr.pl(2, "return proxy;");
            wr.pl(1, "}");
        }
        if (!isIface && isInherit) {
            wr.pl(1, "private static Object __homeClass = null;");
            wr.pl(1, "private static Method __invokeMethod = null;");
            wr.pl(1, "private static void __init_agent() {");
            wr.pl(2, "Class agentManager;");
            wr.pl(2, "try {");
            wr.pl(3, "agentManager = Class.forName(\"" + pkgName + "." + prefix + "__AgentManager\", true, __cloader);");
            wr.pl(2, "} catch (ClassNotFoundException cnfe) {");
            wr.pld(3, "cnfe.printStackTrace();");
            wr.pl(3, "throw new RuntimeException(cnfe.toString() + \" ClassLoader=\"+(__cloader==null?\"null\":__cloader.getClass().getName()),cnfe);");
            wr.pl(2, "}");
            wr.pl(2, "Method getClassMethod;");
            wr.pl(2, "try {");
            wr.pl(3, "getClassMethod = agentManager.getDeclaredMethod(\"getHomeClass\", new Class[]{java.lang.String.class});");
            wr.pl(2, "} catch (NoSuchMethodException nsme) {");
            wr.pl(3, "throw new RuntimeException(\"Cannot find getHomeClass in " + pkgName + "." + prefix + "__AgentManager\" + nsme.toString());");
            wr.pl(2, "}");
            wr.pl(2, "try {");
            wr.pl(3, "__homeClass = getClassMethod.invoke(null, new Object[]{\"" + fullClsName + "\"});");
            wr.pl(2, "} catch (IllegalAccessException iae) {");
            wr.pl(3, "throw new RuntimeException(\"\" + iae.toString());");
            wr.pl(2, "} catch (RuntimeException re) {");
            wr.pl(3, "throw re;");
            wr.pl(2, "} catch (InvocationTargetException ite) {");
            wr.pl(3, "Throwable th = ite.getTargetException();");
            wr.pl(3, "if (th instanceof RuntimeException) {");
            wr.pl(4, "throw (RuntimeException)th;");
            wr.pl(3, "}");
            wr.pld(3, "th.printStackTrace();");
            wr.pl(3, "throw new RuntimeException(\"Unreported exception: \" + th.toString());");
            wr.pl(2, "}");
            wr.pl(2, "Class homeIface;");
            wr.pl(2, "try {");
            wr.pl(3, "homeIface = Class.forName(\"" + pkgName + "." + prefix + "__AgentInterface\", true, __cloader);");
            wr.pl(2, "} catch (ClassNotFoundException cnfe) {");
            wr.pld(3, "cnfe.printStackTrace();");
            wr.pl(3, "throw new RuntimeException(cnfe.toString() + \" ClassLoader=\"+(__cloader==null?\"null\":__cloader.getClass().getName()),cnfe);");
            wr.pl(2, "}");
            wr.pl(2, "Method setClassLoaderMethod;");
            wr.pl(2, "try {");
            wr.pl(3, "__invokeMethod = homeIface.getDeclaredMethod(\"__invoke\", new Class[]{int.class, java.lang.Object.class, java.lang.Object[].class});");
            wr.pl(3, "setClassLoaderMethod = homeIface.getDeclaredMethod(\"__setClassLoader\", new Class[]{java.lang.ClassLoader.class});");
            wr.pl(2, "} catch (NoSuchMethodException nsme) {");
            wr.pl(3, "throw new RuntimeException(\"Cannot find __invoke or __setClassLoader in " + pkgName + "." + prefix + "__AgentInterface\" + nsme.toString());");
            wr.pl(2, "}");
            wr.pl(2, "try {");
            wr.pl(3, "setClassLoaderMethod.invoke(__homeClass, new Object[]{" + pkgName + "." + stubClsName + ".class.getClassLoader()});");
            wr.pl(2, "} catch (IllegalAccessException iae) {");
            wr.pl(3, "throw new RuntimeException(\"\" + iae.toString());");
            wr.pl(2, "} catch (RuntimeException re) {");
            wr.pl(3, "throw re;");
            wr.pl(2, "} catch (InvocationTargetException ite) {");
            wr.pl(3, "Throwable th = ite.getTargetException();");
            wr.pl(3, "if (th instanceof RuntimeException) {");
            wr.pl(4, "throw (RuntimeException)th;");
            wr.pl(3, "}");
            wr.pld(3, "th.printStackTrace();");
            wr.pl(3, "throw new RuntimeException(\"Unreported exception: \" + th.toString());");
            wr.pl(2, "}");
            wr.pl(1, "}");
        }
        if (!isIface && (mems.length > 0 || initStaticFlag || (clazz.isInterface() && !isIface))) {
            wr.pl(1, "private static Class __clazz;");
            for (int i = 0; i < classes.length; i++) {
                if (typeList.containsKey(classes[i])) {
                    wr.pl(1, "private static Class __clazz" + (int)(i + 1) + ";");
                }
            }
            wr.pl(1, "private static void __init_classes() {");
            wr.pl(2, "try {");
            for (int i = 0; i < classes.length; i++) {
                if (typeList.containsKey(classes[i])) {
                    wr.pl(3, "__clazz" + (int)(i + 1) + " = Class.forName(\"" + classes[i] + "\", true, __cloader);");
wr.pld(4,"System.out.println(\"Finding class " + classes[i] + " (__clazz" + (int)(i + 1)+") = \" + ((__clazz" + (int)(i + 1) + " == null)?\"null\" : \"not null\"));");
                    if (classes[i].equals(fullClsName)) {
                        wr.pl(3, "__clazz = __clazz" + (int)(i + 1) + ";");
wr.pld(4,"System.out.println(\"It's my class!!\");");
                    }
                }
            }
            wr.pl(2, "} catch (ClassNotFoundException e) {");
            wr.pld(3, "e.printStackTrace();");
            wr.pl(3, "throw new RuntimeException(e.toString() + \" ClassLoader=\"+(__cloader==null?\"null\":__cloader.getClass().getName()),e);");
            wr.pl(2, "}");
            wr.pl(1, "}");
            wr.pl(1, "public static Class __getMyClass() {");
            wr.pl(2, "__init();");
            wr.pl(2, "return __clazz;");
            wr.pl(1, "}");
            
        }

        if (!isIface && initStaticFlag) {
            Enumeration e = fieldMethods.keys();
            if (e.hasMoreElements()) {
                do {
                    String getFieldMethod = (String)e.nextElement();
                    String typeName = (String)fieldMethods.get(getFieldMethod);
                    wr.pl(1, "static " + 
                        typeName + " __" + getFieldMethod + "Value(final String fieldName) {");
wr.pld(4,"System.out.println(\"" + stubClsName + ".__" + getFieldMethod + "Value(String fieldName)" + "\");");
                    wr.pl(2, "__init();");
                    if (isPrimitive(typeName)) {
                        wr.pl(3, wrapperName(typeName)+" ret_val = ("+wrapperName(typeName)+")");
                    } else {
                        wr.pl(3, typeName+" ret_val = ("+typeName+")");
                    }
                    wr.p("java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {");
                    wr.pl(4, "public Object run() {");
                    wr.pl(5, "try {");
                    if (isPrimitive(typeName)) {
                        wr.pl(6, "return (Object) new "+wrapperName(typeName)+"(__clazz.getField(fieldName)." + getFieldMethod + "(null));");
                    } else {
                        wr.pl(6, "return (Object)__clazz.getField(fieldName)." + getFieldMethod + "(null);");
                    }
                    wr.pl(5, "} catch (IllegalAccessException iae) {");
                    wr.pld(6, "iae.printStackTrace();");
                    wr.pl(6, "throw new RuntimeException(iae);");
                    wr.pl(5, "} catch (NoSuchFieldException nsfe) {");
                    wr.pld(6, "nsfe.printStackTrace();");
                    wr.pl(6, "throw new RuntimeException(nsfe);");
                    wr.pl(5, "}");
                    wr.pl(4, "} // run()");
                    wr.pl(3, "}); // doPrivileged()");
                    if (isPrimitive(typeName)) {
                        wr.pl(3, "return ret_val."+typeName+"Value();");
                    } else {
                        wr.pl(3, "return ret_val;");
                    }
                    wr.pl(1, "}");
                } while (e.hasMoreElements());
            }
        }
        wr.pl("}");
        wr.close();
    }

    /**
     * Prints string into output stream. Does not issue end-of-line character.
     * @param s string to be printed
     */
    private static void p(String s) {
        System.out.print(s);
    }
    /**
     * Prints string into output stream. Issues end-of-line character.
     * @param s string to be printed
     */
    private static void pl(String s) {
        System.out.println(s);
    }
    
    private static class TypeEncoding {
        public String primitive;
        public String wrapper;
        public char code;
        public String getter;
        public TypeEncoding(String primitive, String wrapper, char code, String getter) {
            this.primitive = primitive;
            this.wrapper = wrapper;
            this.code = code;
            this.getter = getter;
        }
    }
    
    private static TypeEncoding typeEncoding[] = {
        new TypeEncoding ("int", "Integer", 'I', "getInt"),
        new TypeEncoding ("char", "Character", 'C', "getChar"),
        new TypeEncoding ("byte", "Byte", 'B', "getByte"),
        new TypeEncoding ("short", "Short", 'S', "getShort"),
        new TypeEncoding ("long", "Long", 'J', "getLong"),
        new TypeEncoding ("boolean", "Boolean", 'Z', "getBoolean"),
        new TypeEncoding ("float", "Float", 'F', "getFloat"),
        new TypeEncoding ("double", "Double", 'D', "getDouble")
    };
    private static String decodeType(String name) {
        return decodeType(name, false);
    }
    private static String decodeType(String name, boolean force) {
        char ch = name.charAt(0);
        if (!force && ch != '[') {
            return name.replace('$','.');
        }
        if (ch == '[') {
            return decodeType(name.substring(1), true) + "[]";
        }
        if (ch == 'L') {
            return name.substring(1, name.length() - 1).replace('/', '.');
        }
        for (int i = 0; i < typeEncoding.length; i++) {
            if (ch == typeEncoding[i].code) {
                return typeEncoding[i].primitive;
            }
        }
        return "unknown \"" + name + "\"";
    }
    
    private static String wrapperName(String name) {
        for (int i = 0; i < typeEncoding.length; i++) {
            if (name.equals(typeEncoding[i].primitive)) {
                return typeEncoding[i].wrapper;
            }
        }
        return "\"invalid primitive name: '" + name + "'\"";
    }
    
    private static boolean isPrimitive(String typeName) {
        for (int i = 0; i < typeEncoding.length; i++) {
            if (typeName.equals(typeEncoding[i].primitive)) {
                return true;
            }
        }
        return false;
    }
    
    private static String primitiveName(String name) {
        for (int i = 0; i < typeEncoding.length; i++) {
            if (name.equals(typeEncoding[i].wrapper)) {
                return typeEncoding[i].primitive;
            }
        }
        return "\"invalid wrapper name: '" + name + "'\"";
    }
    
    private static String getterName(String name) {
        for (int i = 0; i < typeEncoding.length; i++) {
            if (name.equals(typeEncoding[i].primitive)) {
                return typeEncoding[i].getter;
            }
        }
        return "\"invalid getter name: '" + name + "'\"";
    }
    

    private static void printHandleException(JavaFile wr, String msg) {
        wr.pl(1, "public static Exception handleException(Throwable e) {");
        wr.pl(2, "if (e instanceof Error) {");
        wr.pl(3, "throw (Error)e;");
        wr.pl(2, "}");
        wr.pl(2, "if (e instanceof RuntimeException) {");
        wr.pl(3, "throw (RuntimeException)e;");
        wr.pl(2, "}");
        wr.pl(2, "if (e instanceof IllegalAccessException || " + 
                     "e instanceof ClassNotFoundException || " + 
                     "e instanceof NoSuchMethodException) {");
        wr.pld(3, "e.printStackTrace();");
        wr.pl(3, "throw new RuntimeException(msg + \":\" + e.toString);");
        wr.pl(2, "}");
        wr.pl(2, "if (e instanceof InvocationTargetException) {");
        wr.pl(3, "Throwable th = e.getTargetException();");
        wr.pld(3, "th.printStackTrace();");
        wr.pl(3, "return handleException(th);");
        wr.pl(2, "}");
        wr.pld(3, "e.printStackTrace();");
        wr.pl(2, "return e;");
        wr.pl(1, "}");
    }
    private static void printMethod(JavaFile wr, String className, Member mem, int num, boolean isCtor, 
                                                                                        boolean isIface, 
                                                                                        boolean isClassIface,
                                                                                        boolean isInherit) {
        String retType = null;
        String mod = Modifier.toString(mem.getModifiers()).
                replaceAll("native", "/* native */").replaceAll("abstract", "/* abstract */");
        /*wr.p(1, "");
        if (mod.length() > 0) {
            wr.p(mod + " ");
        } */
        wr.p(1, "public ");
        if (Modifier.isStatic(mem.getModifiers())) {
            wr.p("static ");
        }
        
        if (!isCtor) {
            retType = decodeType(((Method)mem).getReturnType().getName());
            wr.p((isTranslatedClass(retType) ? translateClassNamePkg(retType) + getBrackets(retType) : retType) + " ");
        }
        if (isCtor) {
            wr.p(className);
        } else {
            wr.p(mem.getName());
        }
        wr.p("(");
        Class[] pars;
        if (isCtor) {
            pars = ((Constructor)mem).getParameterTypes();
        } else {
            pars = ((Method)mem).getParameterTypes();
        }
        for (int j = 0; j < pars.length; j++) {
            if (j > 0) {
                wr.p(", ");
            }
            String typeName = decodeType(pars[j].getName());
            if (isTranslatedClass(typeName)) {
                typeName = translateClassNamePkg(typeName) + getBrackets(typeName);
            }
            wr.p(typeName + " ");
            wr.p("arg" + (int)(j + 1));
        }
        wr.p(")");
        Class[] exc;
        if (isCtor) {
            exc = ((Constructor)mem).getExceptionTypes();
        } else {
            exc = ((Method)mem).getExceptionTypes();
        }
        if (exc == null) {
            exc = new Class[0];
        }
        Arrays.sort(exc, new Comparator() {
            public int compare(Object o1, Object o2) {
                Class clazz1 = (Class)o1;
                Class clazz2 = (Class)o2;
                if (clazz1.equals(clazz2)) {
                    return 0;
                }
                if (clazz1.isAssignableFrom(clazz2)) {
                    return 1;
                }
                return -1;
            }
            public boolean equals(Object obj) {
                return false;
            }
        });
        if (exc.length > 0) {
            wr.p(" throws ");
            for (int j = 0; j < exc.length; j++) {
                if (j > 0) {
                    wr.p(", ");
                }
                String exName = exc[j].getName();
                int k;
                for (k = 0; k < classes.length; k++) {
                    if (classes[k].equals(exName)) {
                        break;
                    }
                }
                if (k < classes.length) {
                    exName = translateClassNamePkg(exName);
                }
                wr.p(exName);
            }
        }
        if (isIface) {
            wr.pl(";");
        } else {
            wr.pl(" {");

            if (isCtor || Modifier.isStatic(mem.getModifiers())) {
                wr.pl(2, "__init();");
            }
wr.pld(4,"System.out.println(\"Trying to call " + className + "." + (isCtor?"constructor"+(int)(num + 1):mem.getName())+ "\");");
            wr.pl(2, "try {");
            wr.p(3,"");
            if (isCtor) {
                wr.p("__setInternal(");
                if (isInherit && !isClassIface) {
                    wr.p("__invokeMethod.invoke(__homeClass, new Object[]{new Integer(" + num + "), this, new Object[]{");
                    printActualArgs(new JavaFile(wr, 4), pars);
                    wr.p("}})");
                } else {
                    wr.p("__cons" + (int)(num + 1) + ".newInstance(");
                    wr.p("new Object[] {");
                    printActualArgs(new JavaFile(wr, 4), pars);
                    wr.p("})");
                }
                wr.pl(");");
wr.pld(4,"System.out.println(\"return from " + className + "." + (isCtor?"constructor"+(int)(num + 1):mem.getName())+ "\");");
                wr.pl(3, "if (__getInternal() != null) {");
                wr.pl(4, "__objList.put(__getInternal(), new java.lang.ref.WeakReference(this));");
                wr.pl(3, "}");
            } else {
                if (isTranslatedClass(retType)) {
                    wr.p("Object result = ");
                } else {
                    if (!retType.equals("void")) {
                        String rType = ((Method)mem).getReturnType().isPrimitive() ? 
                            "java.lang." + wrapperName(retType) : retType;
                        wr.p(rType + " result = (" + rType + ")");
                    }
                }
                if (isInherit && !isClassIface) {
                    wr.p("__invokeMethod.invoke(__homeClass, new Object[]{new Integer(" + num + "), ");
                    wr.p(Modifier.isStatic(mem.getModifiers()) ? "null" : "__getInternal()");
                    wr.p(", new Object[]{");
                    printActualArgs(new JavaFile(wr, 4), pars);
                    wr.p("}})");
                } else {
                    wr.p("__method" + (int)(num+1) + ".invoke(");
                    wr.p(Modifier.isStatic(mem.getModifiers()) ? "null" : "__getInternal()");
                    wr.p(", new Object[] {");
                    printActualArgs(new JavaFile(wr, 4), pars);
                    wr.p("})");
                }
                wr.pl(";");
            
wr.pld(4,"System.out.println(\"return from " + className + "." + (isCtor?"constructor"+(int)(num + 1):mem.getName())+ "\");");
                if (emptyList.containsKey(retType)) {
                    wr.pl(3, "return (" + translateClassNamePkg(retType) + getBrackets(retType) + ")result;");
                } else {
                    if (isTranslatedClass(retType)) {
                        String rType = translateClassNamePkg(retType);
                        String arrayBrackets = getBrackets(retType);
                        Class retClass = ((Method)mem).getReturnType();
                        while (retClass.isArray()) {
                            retClass = retClass.getComponentType();
                        }
                        String implClass = (retClass.isInterface() ? translateInterfaceImplNamePkg(retType) : rType);
                        if (arrayBrackets.length() > 0) {
                            int dimensions = arrayBrackets.length() / 2; // must be "[][][]..."
                            wr.pl(3, "if (result == null) {");
                            wr.pl(4, "return (" + rType + arrayBrackets + ")null;");
                            wr.pl(3, "}");
                            wr.pl(3, rType + arrayBrackets + " retArray = new " + 
                                        rType + "[((Object[])result).length]" + arrayBrackets.substring(2) + ";");
                            String indices = "";
                            for (int i = 0; i < dimensions; i++) {
                                String varName = "i" + i;
                                wr.pl(3+i*2, "for (int " + varName + " = 0; " + varName + " < ((Object" + arrayBrackets + ")result)" + indices + ".length; " + varName + "++) {");
                                indices += "[i" + i + "]";
                                wr.pl(4+i*2, "if (((Object" + arrayBrackets + ")result)" + indices + " == null) {");
                                wr.pl(5+i*2, "retArray" + indices + " = null;");
                                wr.pl(4+i*2, "} else {");
                                if (i == dimensions - 1) {
                                    wr.pl(5+i*2, "retArray" + indices + " = (" + rType + ")" + 
                                        implClass + ".__getInstance(((Object" + arrayBrackets + ")result)" + indices + ");");
                                } else {
                                    wr.pl(5+i*2, "retArray" + indices + " = new " + 
                                            rType + "[((Object" + arrayBrackets + ")result)" + indices + ".length]" + arrayBrackets.substring((i + 2) * 2) + ";");
                                }
                            }
                            for (int i = dimensions - 1; i >= 0; --i) {
                                wr.pl(4+i*2, "}");
                                wr.pl(3+i*2, "} // for i" + i);
                            }
                            wr.pl(3, "return (" + rType + arrayBrackets + ")retArray;");
                        } else {
                            wr.pl(3, "return (" + rType + ")" + implClass + ".__getInstance(result);");
                        }
                    } else {
                        if (!retType.equals("void")) {
                            if (((Method)mem).getReturnType().isPrimitive()) {
                                wr.pl(3, "return result." + retType + "Value();");
                            } else {
                                wr.pl(3, "return result;");
                            }
                        }
                    }
                }
            }
            wr.pl(2, "} catch (IllegalAccessException iae) {");
            wr.pld(3, "iae.printStackTrace();");
            wr.pl(3, "throw new RuntimeException(iae);");
            wr.pl(2, "} catch (InvocationTargetException ite) {");
            wr.pl(3, "Throwable target = ite.getTargetException();");
            wr.pld(3, "target.printStackTrace();");
            for (int i = 0; exc != null && i < exc.length; i++) {
                String exName = exc[i].getName();
                int k = findTranslatedClass(classes, exName);
                if (k != -1) {
                    typeList.put(exName, "");
                    wr.pl(3, "if (__clazz" + (int)(k+1) + ".isAssignableFrom(target.getClass())) {");
                    wr.pl(4, "throw new " + translateClassNamePkg(exName) + "(target.toString());");
                    wr.pl(3, "}");
                } else {
                    wr.pl(3, "if (target instanceof " + exName + ") {");
                    wr.pl(4, "throw (" + exName + ")target;");
                    wr.pl(3, "}");
                }
            }
            wr.pl(3, "throw new RuntimeException(\"Unreported exception:\" + target.toString());");
            if (isCtor && !isInherit) {
                wr.pl(2, "} catch (InstantiationException ie) {");
                wr.pld(3, "ie.printStackTrace();");
                wr.pl(3, "throw new RuntimeException(ie);");
            }
            wr.pl(2, "}");
            wr.pl(1, "}");
        }
    }

    
    private static void printProxyInstance(JavaFile wr, Class iface, String instanceName) {
        String ifaceName = iface.getName();
        int k = findTranslatedClass(classes, ifaceName);
        if (k == -1) {
            // impossible
            return;
        }
        typeList.put(ifaceName, "");
        wr.pl(0,"Proxy.newProxyInstance(__cloader, "); 
        wr.pl(1,"new Class[] {__clazz" + (int)(k+1) + "}, ");
        wr.pl(1,"new InvocationHandler() {");
        wr.pl(2,"private " + translateClassNamePkg(ifaceName) + " myInstance = " + instanceName + ";");
        wr.pl(2,"private int hc = " + instanceName + ".hashCode() + 1;");
        wr.pl(2,"public Object invoke(Object p, Method m, Object[] a) throws Throwable {");
        wr.pl(3,"Class[] args = m.getParameterTypes();");
        Method[] mlist = iface.getMethods();
        if (mlist != null && mlist.length > 0) {
            for (int i = 0; i < mlist.length; i++) {
                wr.p(3,"if (m.getName().equals(\"" + mlist[i].getName() + "\")");
                Class mpars[] = mlist[i].getParameterTypes();
                int parCnt = 0;
                if (mpars != null && mpars.length > 0) {
                    parCnt = mpars.length;
                }
                if (parCnt == 0) {
                    wr.p(" && (args == null || args.length == 0)");
                } else {
                    wr.p(" && args.length == " + parCnt);
                    for (int j = 0; j < parCnt; j++) {
                        wr.p(" && args[" + j + "].getName().equals(\"" + mpars[j].getName() + "\")");
                    }
                }
                wr.pl(") {");
wr.pld(4,"System.out.println(\"callback - " + ifaceName + "." + mlist[i].getName() + "\");");
                Class retClass = mlist[i].getReturnType();
                String retType = decodeType(retClass.getName());
                String rType;
                if (retClass.isInterface() && isTranslatedClass(retType)) {
                    //wr.pl(4, "try {");
                }
                if (!retType.equals("void")) {
                    if (isTranslatedClass(retType)) {
                        rType = translateClassNamePkg(retType);
                    } else {
                        rType = retType;
                    }
                    wr.p(5, rType + " result = ");
                } else {
                    wr.p(5, "");
                }
                wr.p("myInstance." + mlist[i].getName() + "(");
                for (int j = 0; j < parCnt; j++) {
                    if (j > 0) {
                        wr.p(", ");
                    }
                    printActualArg2(new JavaFile(wr, 5), mpars[j], "a[" + j + "]");
                }
                wr.pl(");");
wr.pld(4,"System.out.println(\"return from callback - " + ifaceName + "." + mlist[i].getName() + "\");");
                wr.p(5, "return ");
                printActualArg0(new JavaFile(wr, 5), retClass, "result");
                wr.pl(";");
                if (retClass.isInterface() && isTranslatedClass(retType)) {
                    //wr.pl(4, "} catch (NoSuchMethodException nsme) {");
                    //wr.pl(5, "nsme.printStackTrace();");
                    //wr.pl(5, "throw new RuntimeException(nsme);");
                    //wr.pl(4, "}");
                }
                wr.pl(3, "} else");
            }
        }
        wr.pl(3, "if (m.getName().equals(\"hashCode\") && (args == null || args.length == 0)) {");
        wr.pl(4, "return new Integer(myInstance.hashCode());");
        wr.pl(3, "} else");
        wr.pl(3, "if (m.getName().equals(\"equals\") && args.length == 1 && args[0].getName().equals(\"java.lang.Object\")) {");
        wr.pl(4, "boolean result = (a[0] == null ? false : a[0].equals(myInstance));");
        wr.pl(4, "return new Boolean(result);");
        wr.pl(3, "} else {");
        wr.pl(4, "StringBuffer s = new StringBuffer();");
        wr.pl(4, "s.append(m.getName());");
        wr.pl(4, "s.append(\"(\");");
        wr.pl(4, "for (int i = 0; i < args.length; i++) {");
        wr.pl(5, "if (i > 0) {");
        wr.pl(6, "s.append(\", \");");
        wr.pl(5, "}");
        wr.pl(5, "s.append(args[i].getName());");
        wr.pl(4, "}");
        wr.pl(4, "s.append(\")\");");
wr.pld(4,"System.out.println(\"callback - " + ifaceName + " - couldn't find method: \" + s.toString());");
        wr.pl(4, "throw new RuntimeException(\"Cannot find method: \" + s.toString());");
        wr.pl(3, "}");
        wr.pl(2,"}");
        wr.p(1,"})");
    }
    
    // return value from callback
    private static void printActualArg0(JavaFile wr, Class par, String arg) {
        String parClassName = decodeType(par.getName());
        if (parClassName.equals("void")) {
            wr.p("null");
            return;
        }
        if (emptyList.containsKey(parClassName)) {
            wr.p(arg);
            return;
        }
        if (par.isPrimitive()) {
            wr.p("new " + "java.lang." + wrapperName(parClassName) + "(" + arg + ")");
        } else {
            if (isTranslatedClass(parClassName)) {
                wr.p("(" + arg + " == null ? null : (");
                if (par.isInterface()) {
                    String proxyBase = ((String)pkgNames.get(parClassName)).replaceAll("\\.", "_") + "__ProxyBase";
                    String ifaceClassName = translateInterfaceImplNamePkg(parClassName);
                    wr.p("((" + arg + " instanceof " + ifaceClassName + ") ?");
                    wr.p("(((" + ifaceClassName + ")" + arg + ").__getInternal()) : (");
                    wr.p("(" + arg + " instanceof " + proxyBase + ") ? ((" + proxyBase + ")" + arg +").__getInternal() :");
                    wr.p(translateClassNamePkg(parClassName) + 
                        ".class.getDeclaredMethod(\"__getInternal\", new Class[]{}).invoke(null, new Object[]{})");
                    wr.p("))");
                } else {
                    wr.p("((" + translateClassNamePkg(parClassName) + ")" + arg + ").__getInternal()");
                }
                wr.p("))");
            } else {
                wr.p(arg);
            }
        }
    }
    // method param
    private static void printActualArg1(JavaFile wr, Class par, String arg) {
        String parClassName = par.getName();
        if (parClassName.equals("void")) {
            return;
        }
        if (emptyList.containsKey(parClassName)) {
            wr.p(arg);
            return;
        }
        if (par.isPrimitive()) {
            wr.p("new " + "java.lang." + wrapperName(parClassName) + "(" + arg + ")");
        } else {
            if (isTranslatedClass(parClassName)) {
                wr.p("(" + arg + " == null ? null : (");
                if (par.isInterface()) {
                    String className = translateInterfaceImplNamePkg(parClassName);
                    wr.p("((" + arg + " instanceof " + className + ") ?");
                    wr.p("(((" + className + ")" + arg + ").__getInternal()" + ") : (");
                    wr.p(className + ".__getProxyClass(" + arg + ")");
                    wr.p("))");
                } else {
                    wr.p("((" + translateClassNamePkg(parClassName) + ")" + arg + ").__getInternal()");
                }
                wr.p("))");
            } else {
                wr.p(arg);
            }
        }
    }
    // callback method param
    private static void printActualArg2(JavaFile wr, Class par, String arg) {
        String parClassName = par.getName();
        if (parClassName.equals("void")) {
            return;
        }
        if (emptyList.containsKey(parClassName)) {
            wr.p(arg);
            return;
        }
        if (par.isPrimitive()) {
            wr.p("((" + wrapperName(parClassName) + ")" + arg + ")." + parClassName + "Value()");
        } else {
            if (isTranslatedClass(parClassName)) {
                String className;
                if (par.isInterface()) {
                    className = translateInterfaceImplNamePkg(parClassName);
                } else {
                    className = translateClassNamePkg(parClassName);
                }
                wr.p("(" + className + ")" + className + ".__getInstance(" + arg + ")");
            } else {
                String className = decodeType(parClassName);
                wr.p("(" + className + ")" + arg);
            }
        }
    }
    // method param (pass thru)
    private static void printActualArg3(JavaFile wr, Class par, String arg) {
        String parClassName = par.getName();
        if (parClassName.equals("void")) {
            return;
        }
        wr.p(arg);
    }

    // callback from agent method param
    private static void printActualArg4(JavaFile wr, Class par, String arg) {
        String parClassName = par.getName();
        if (parClassName.equals("void")) {
            return;
        }
        if (par.isPrimitive()) {
            wr.p("new " + "java.lang." + wrapperName(parClassName) + "(" + arg + ")");
        } else {
            wr.p(arg);
        }
    }
    
    // return value from callback from agent
    private static void printActualArg5(JavaFile wr, Class par, String arg) {
        String parClassName = par.getName();
        if (parClassName.equals("void")) {
            return;
        }
        if (par.isPrimitive()) {
            wr.p("((" + wrapperName(parClassName) + ")" + arg + ")." + parClassName + "Value()");
        } else {
            String className = decodeType(parClassName);
            wr.p("(" + className + ")" + arg);
        }
    }
    
    private static void printActualArgs(JavaFile wr, Class[] pars) {
        if (pars == null || pars.length == 0) {
            return;
        }
        for (int i = 0; i < pars.length; i++) {
            if (i > 0) {
                wr.p(", ");
            }
            printActualArg1(wr, pars[i], "arg" + (int)(i+1));
        }
    }

    private static void printHeader(JavaFile wr, String pkg) {
        wr.pl("/*");
        wr.pl(" * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.");
        wr.pl(" * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
        wr.pl("*/");
        wr.pl("/*");
        wr.pl(" * This is a generated file. Do not edit it!");
        wr.pl(" */");
        if (pkg != null) {
            wr.pl("package " + pkg + ";");
        }
        wr.pl("import java.lang.reflect.*;");
    }
}

/**
 * Helper class which represents output Java file.
 */
class JavaFile {
    /** Output stream */
    private PrintWriter wr;
    
    /** Initial indent */
    private int ind = 0;
    
    /** Debug flag */
    private boolean debug = false;
    
    /**
     * Constructor.
     * @param dir directory name.
     * @param name class name.
     */
    JavaFile(String dir, String name, boolean debugFlag) {
        this(dir, name);
        debug = debugFlag;
    }
    
    /**
     * Constructor.
     * @param dir directory name.
     * @param name class name.
     */
    JavaFile(String dir, String name) {
        try {
            wr = new PrintWriter(
                 new FileOutputStream(new File(dir + "/" + name + ".java")));
        }
        catch (IOException e) {
            System.out.println("Cannot create file " + name + ".java");
            System.exit(1);
        }
    }
    
    /**
     * Constructor.
     * @param file parent.
     * @param indent initial indent.
     */
    JavaFile(JavaFile file, int indent) {
        ind = file.ind + indent;
        wr = file.wr;
        debug = file.debug;
    }
    
    /**
     * Closes file.
     */
    void close() {
        wr.close();
    }
    /**
     * Prints string into output stream. Do not issues end-of-line character.
     * @param s string to be printed
     */
    void p(String s) {
        wr.print(s);
    }
    
    /**
     * Prints string with indent into output stream. 
     * Do not issue end-of-line character.
     * @param indent how many spaces before string
     * @param s string to be printed
     */
    void p(int indent, String s) {
        writeIndent(indent);
        wr.print(s);
    }
    
    /**
     * Prints string into output stream. Does not issue end-of-line character.
     * @param s string to be printed
     */
    void pl(String s) {
        wr.println(s);
    }
    
    /**
     * Prints string with indent into output stream. 
     * Issues end-of-line character.
     * @param indent how many spaces before string
     * @param s string to be printed
     */
    void pl(int indent, String s) {
        writeIndent(indent);
        wr.println(s);
    }
    
    /**
     * Prints end-of-line character.
     */
    void pl() {
        wr.println();
    }
    
    /**
     * Prints string into output stream. Do not issues end-of-line character.
     * @param s string to be printed
     */
    void pd(String s) {
        if (debug) {
            p(s);
        }
    }
    
    /**
     * Prints string with indent into output stream. 
     * Do not issue end-of-line character.
     * @param indent how many spaces before string
     * @param s string to be printed
     */
    void pd(int indent, String s) {
        if (debug) {
            p(indent, s);
        }
    }
    
    /**
     * Prints string into output stream. Does not issue end-of-line character.
     * @param s string to be printed
     */
    void pld(String s) {
        if (debug) {
            pl(s);
        }
    }
    
    /**
     * Prints string with indent into output stream. 
     * Issues end-of-line character.
     * @param indent how many spaces before string
     * @param s string to be printed
     */
    void pld(int indent, String s) {
        if (debug) {
            pl(indent, s);
        }
    }
    
    /**
     * Prints end-of-line character.
     */
    void pld() {
        if (debug) {
            pl();
        }
    }
    
    /**
     * Prints indent.
     * @param indent the intent.
     */
    private void writeIndent(int indent) {
        indent += ind; // add initial indent
        for (int i = 0; i < indent; i++) {
            wr.print("    ");
        }
    }
}
