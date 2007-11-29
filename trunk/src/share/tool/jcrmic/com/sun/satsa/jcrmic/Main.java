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
import com.sun.satsa.jcrmic.utils.*;

/**
 * Main "jcrmic" program.
 */

public class Main {

    /**
     *  The list of interface names specified by user.
     */
    private Vector classNames;

    /**
     * Class loader.
     */
    private Loader loader;

    /**
     * Errors/warnings notifier.
     */
    private Notifier notifier;

    /**
     * Output directory for java and class files.
     */
    private File destDir;

    /**
     * Class path specified by user.
     */
    private String classPath;

    /**
     * 'Compile generated source files' flag.
     */
    private boolean compile = true;

    /**
     * 'Keep generated source files' flag.
     */
    private boolean keepGenerated = false;

    /**
     * Constructor.
     * @param out output stream for messages.
     */
    public Main(OutputStream out) {

        notifier = new Notifier(out);
        classNames = new Vector();
        loader = new Loader(notifier);
    }

    /**
     * Main program.
     * @param argv command line arguments given by user.
     */
    public static void main(String argv[]) {

        Main compiler = new Main(System.out);
        System.exit(compiler.compile(argv) ? 0 : 1);
    }

    /**
     * Run the compiler.
     * @param argv command line arguments given by user.
     * @return true if compilation was successful
     */
    public boolean compile(String argv[]) {

        if (!parseArgs(argv)) {
            usage();
            return false;
        }

        return doCompile();
    }

    /**
     * Parse the arguments for compile.
     * @param argv command line arguments given by user.
     * @return true if command line is parsed without errors
     */
    public boolean parseArgs(String argv[]) {

        // Parse arguments
        for (int i = 0; i < argv.length; i++) {

            if (argv[i].equals("-classpath")) {
                if ((i + 1) < argv.length) {
                    if (!loader.setClassPath(argv[++i]))
                        return false;
                    classPath = argv[i];
                } else {
                    notifier.error("rmic.option.requires.argument",
                            "-classpath");
                    return false;
                }
            } else if (argv[i].equals("-d")) {
                if ((i + 1) < argv.length) {
                    if (destDir != null) {
                        notifier.error("rmic.option.already.seen", "-d");
                        return false;
                    }
                    destDir = new File(argv[++i]);
                    if (!destDir.exists()) {
                        notifier.error("rmic.no.such.directory",
                                destDir.getPath());
                        return false;
                    }
                }
            } else if (argv[i].equals("-nocompile")) {
                compile = false;
            } else if (argv[i].equals("-keep")) {
                keepGenerated = true;
            } else if (argv[i].startsWith("-")) {
                notifier.error("rmic.no.such.option", argv[i]);
                return false;
            } else {
                classNames.addElement(argv[i]);
            }
        }

        if (classNames.size() == 0) {
            return false;
        }

        return true;
    }


    /**
     * Do the compile with the switches and files already supplied.
     * @return true if compilation was successful
     */
    public boolean doCompile() {

        if (!(loader.loadClass("java/rmi/Remote") &&
                loader.loadClass("java/rmi/RemoteException") &&
                loader.loadClass("java/lang/RuntimeException"))) {
            return false;
        }

        // load classes

        for (int i = 0; i < classNames.size(); i++) {
            String name = (String) classNames.elementAt(i);
            if (!loader.loadClass(name)) {
                return false;
            }
        }

        // verify classes

        for (int i = 0; i < classNames.size(); i++) {

            String name = (String) classNames.elementAt(i);

            if (! verify(name)) {
                return false;
            }
        }

        // create stubs

        Vector fileNames = new Vector();

        for (int i = 0; i < classNames.size(); i++) {

            String name = (String) classNames.elementAt(i);

            String filePath = name.replace('.', File.separatorChar);
            String packagePath = "";

            int index = filePath.lastIndexOf(File.separatorChar);

            if (index != -1) {
                packagePath = filePath.substring(0, index);
                filePath = filePath.substring(index + 1);
            }

            filePath = filePath + "_Stub.java";

            File stubFile;

            if (destDir != null) {
                File packageDir = new File(destDir, packagePath);
                /*
                * Make sure that the directory for this package exists.
                * We assume that the caller has verified that the top-
                * level destination directory exists, so we don't have
                * to worry about creating it unintentionally.
                */
                if (!packageDir.exists()) {
                    packageDir.mkdirs();
                }
                stubFile = new File(packageDir, filePath);
            } else {
                /*
                * If a top-level destination directory is not specified
                * (with the "-d" option), we just put the generated files
                * in the current working directory, which was the behavior
                * of rmic in JDK 1.1.  This feels less than ideal, but
                * there is no easy alternative.
                */
                stubFile = new File(System.getProperty("user.dir"), filePath);
            }

            try {
                stubFile.createNewFile();

                fileNames.add(stubFile.getCanonicalPath());

                IndentingWriter out = new IndentingWriter(
                        new OutputStreamWriter(new FileOutputStream(stubFile)));

                writeStub(name, out);

                out.close();

            } catch (IOException e) {
                notifier.error("rmic.ioerror.writing", stubFile.getPath());
                return false;
            }
        }

        if (! compile) {
            return true;
        }

        // compile the stubs

        String command = System.getProperty("JAVAC_PATH");

        if (command == null || ! new File(command).exists()) {
            notifier.error("rmic.compiler.not.found");
            return false;
        }

        command += " -classpath " + classPath;

        if (destDir != null) {
            command += " -d " + destDir.getPath();
        }

        for (int i = 0; i < fileNames.size(); i++) {
            command += " " + (String) fileNames.elementAt(i);
        }

        Process p;
        try {
            p = Runtime.getRuntime().exec(command);
        } catch (IOException e) {
            notifier.error("rmic.compilation.error");
            return false;
        }

        (new StreamReader(p.getInputStream())).start();
        (new StreamReader(p.getErrorStream(), notifier)).start();

        try {
            p.waitFor();
        } catch (InterruptedException e) {
            notifier.error("rmic.compilation.error");
            return false;
        }

        if (! keepGenerated) {
            for (int i = 0; i < fileNames.size(); i++) {
                new File((String) fileNames.elementAt(i)).delete();
            }
        }

        if (p.exitValue() != 0) {
            notifier.error("rmic.compilation.failed");
            return false;
        }

        return true;
    }

    /**
     * Prints usage message.
     */
    public void usage() {
        notifier.error("rmic.usage");
    }

    /**
     * Verify that interface specified in command line complies with
     * JCRMI limitations.
     * @param name interface name
     * @return verification result
     */
    public boolean verify(String name) {

        JClass c = loader.getClass(name);

        if (!c.isInterface()) {
            notifier.error("rmic.cant.make.stubs.for.class", name);
            return false;
        }

        if (!c.isRemote()) {
            notifier.error("rmic.must.implement.remote", name);
            return false;
        }

        // Verify this interface and all interfaces extended by this
        // interface
        Vector remoteInterfaces = new Vector();

        addInterface(c, remoteInterfaces);

        if (!verifyInterfaces(remoteInterfaces)) {
            return false;
        }

        return true;
    }

    /**
     * Verify all the interfaces in vector for compliance with JCRMI
     * limitations.
     * @param interfaces the list of interfaces
     * @return verification result
     */
    private boolean verifyInterfaces(Vector interfaces) {

        for (int i = 0; i < interfaces.size(); i++) {

            if (!verifyInterface((JClass) interfaces.elementAt(i))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify that interface complies with JCRMI limitations.
     * @param cl interface to be verified
     * @return verification result
     */
    private boolean verifyInterface(JClass cl) {

        if (cl.isVerified()) {
            return true;
        }

        cl.setVerified();

        JMethod[] methods = cl.getMethods();

        for (int i = 0; i < methods.length; i++) {

            if (!verifyRemoteMethod(cl.getClassName(), methods[i]))
                return false;
        }

        return true;
    }

    /**
     * The list of exceptions that can be thrown by remote methods.
     */
    private static String[] JCExceptions = {
        "java/lang/Throwable",
        "java/lang/ArithmeticException",
        "java/lang/ArrayIndexOutOfBoundsException",
        "java/lang/ArrayStoreException",
        "java/lang/ClassCastException",
        "java/lang/Exception",
        "java/lang/IndexOutOfBoundsException",
        "java/lang/NegativeArraySizeException",
        "java/lang/NullPointerException",
        "java/lang/RuntimeException",
        "java/lang/SecurityException",
        "java/io/IOException",
        "java/rmi/RemoteException",
        "javacard/framework/APDUException",
        "javacard/framework/CardException",
        "javacard/framework/CardRuntimeException",
        "javacard/framework/ISOException",
        "javacard/framework/PINException",
        "javacard/framework/SystemException",
        "javacard/framework/TransactionException",
        "javacard/framework/UserException",
        "javacard/security/CryptoException",
        "javacard/framework/service/ServiceException"};

    /**
     * Verify that remote method complies with JCRMI limitations.
     * @param class_name the name of class that defines the method
     * @param m the method to be verified
     * @return verification result
     */
    private boolean verifyRemoteMethod(String class_name, JMethod m) {

        // check signature

        String descriptor = m.getMethodDescriptor();

        Vector v = RemoteMethod.parseDescriptor(descriptor);

        boolean ok = (v != null);

        if (ok) {

            String parameter = (String) v.elementAt(v.size() - 1);

            if (parameter.startsWith("L")) {

                parameter = parameter.substring(1, parameter.length() - 1);

                if (!loader.loadClass(parameter)) {
                    return false;
                }

                JClass ret = loader.getClass(parameter);

                ok = ok && ret.isInterface() && ret.isRemote();

                if (ok) {

                    Vector z = new Vector();

                    addInterface(ret, z);

                    if (!verifyInterfaces(z)) {
                        return false;
                    }
                }
            }
        }

        if (!ok) {
            notifier.error("rmic.incorrect.method.signature",
                    class_name, m.getMethodName() + descriptor);
            return false;
        }

        // check that all exceptions are defined in Java Card API
        // and that method throws RemoteException

        JClass jrRemoteException = loader.getClass("java/rmi/RemoteException");
        boolean RemoteThrown = false;

        String[] exceptions = m.getExceptionsThrown();

        if (exceptions != null) {

            for (int i = 0; i < exceptions.length; i++) {

                boolean found = false;

                for (int j = 0; j < JCExceptions.length; j++) {

                    if (exceptions[i].equals(JCExceptions[j])) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    notifier.error("rmic.method.throws.invalid.exception",
                            class_name, m.getMethodName() + descriptor,
                            exceptions[i]);
                    return false;
                }

                JClass ex = loader.getClass(exceptions[i]);

                RemoteThrown = RemoteThrown ||
                               jrRemoteException.isSubclass(ex);
            }

        }

        if (!RemoteThrown) {
            notifier.error("rmic.must.throw.remoteexception",
                    class_name, m.getMethodName() + descriptor);
            return false;
        }

        return true;
    }

    /**
     * Writes the stub for remote interface into the stream.
     * @param className interface name
     * @param p output stream
     * @throws IOException if I/O error occurs
     */
    public void writeStub(String className, IndentingWriter p)
            throws IOException {

        JClass c = loader.getClass(className);

        // find all remote interfaces

        Vector remoteInterfaces = new Vector();
        addInterface(c, remoteInterfaces);

        // find all remote methods

        Hashtable remoteMethods = new Hashtable();

        for (int i = 0; i < remoteInterfaces.size(); i++) {

            JClass cl = (JClass) remoteInterfaces.elementAt(i);
            JMethod[] methods = cl.getMethods();

            for (int j = 0; j < methods.length; j++) {

                String m_name = methods[j].getMethodName();
                String m_descriptor = methods[j].getMethodDescriptor();

                RemoteMethod m = new RemoteMethod(m_name, m_descriptor, loader);

                String[] exceptions = methods[j].getExceptionsThrown();

                if (exceptions != null) {
                    for (int k = 0; k < exceptions.length; k++) {
                        m.addException(loader.getClass(exceptions[k]));
                    }
                }

                String key = m_name + m_descriptor;

                RemoteMethod m_old = (RemoteMethod) remoteMethods.get(key);

                if (m_old == null) {
                    remoteMethods.put(key, m);
                } else {
                    m_old.merge(m);
                }
            }
        }

        p.pln("// Stub class generated by jcrmic, do not edit.");
        p.pln("// Contents subject to change without notice.");
        p.pln();

        String name = c.getClassName().replace('/', '.');
        String pname = "";
        int index = name.lastIndexOf('.');

        if (index != -1) {

            pname = name.substring(0, index);
            name = name.substring(index + 1);
        }

        name = name + "_Stub";

        /*
        * If remote implementation class was in a particular package,
        * declare the stub class to be in the same package.
        */
        if (!pname.equals("")) {
            p.pln("package " + pname + ";");
            p.pln();
        }

        /*
        * Declare the class; implement all remote interfaces.
        */

        String s = "";

        for (int i = 0; i < remoteInterfaces.size(); i++) {

            JClass cl = (JClass) remoteInterfaces.elementAt(i);

            if (cl.isRemote()) {

                if (! s.equals("")) {
                    s = s + ", ";
                }

                s = s + cl.getClassName().replace('/', '.');
            }
        }

        p.plnI("public final class " + name);
        p.pln("extends javax.microedition.jcrmi.RemoteStub");
        p.pln("implements " + s + " {");

        p.pln("");

        p.pln("// constructor");

        p.plnI("public " + name + "() {");
        p.pln("super();");
        p.pOln("}");

        for (Enumeration methods = remoteMethods.elements();
             methods.hasMoreElements();) {

            ((RemoteMethod) methods.nextElement()).write(p);
        }

        p.pOln("}");
    }



    /**
     * Put this interface and all interfaces extended by this interface
     * into vector.
     * @param cl an interface
     * @param v target vector
     */
    public static void addInterface(JClass cl, Vector v) {

        /* !!! debug */
        if (!cl.isInterface()) {
            System.out.println("error - addInterface");
            System.exit(1);
        }

        if (!v.contains(cl)) {

            v.add(cl);

            JClass[] interfaces = cl.getInterfaces();

            for (int i = 0; i < interfaces.length; i++) {
                addInterface(interfaces[i], v);
            }
        }
    }
}
