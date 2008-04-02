/*
 *   
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
 */

package makedep;

import java.io.*;
import java.util.*;
import config.Configurator;

/**
 * Base class for the creation of IDE projects files for building the VM.
 *
 * Currently only Win32/VC6 project files are supported. See Win32IDETool.java
 * for more details.
 */
abstract public class IDETool {
    static File pwd = new File(System.getProperty("user.dir"));

    /** command-line argument */
    String buildspace;
    /** command-line argument */
    String product;
    /** command-line argument */
    String platform;
    /** command-line argument */
    String database;
    /** command-line argument */
    String workspace;

    /** User settings */
    Properties userSettings;

    /** */
    Configurator configurator;

    public String getBuildSpaceArg() {
        return buildspace;
    }
    public String getProductArg() {
        return product;
    }
    public String getPlatformArg() {
        return platform;
    }
    public String getDatabaseArg() {
        return database;
    }
    public String getWorkSpaceArg() {
        return workspace;
    }

    private Vector apiSources;
    private Vector vmSources;
    private Vector jccSources;

    public void parseArguments(String args[]) {
        for (int i=0; i<args.length; ) {
            try {
                int n = parseOneArgument(args, i);
                if (n <= 0) {
                    fatal("unknown argument " + args[i]);
                } else {
                    i += n;
                }
            } catch (ArrayIndexOutOfBoundsException e) {
                fatal("parameter missing for argument " + args[i]);
            }
        }

        if (getWorkSpaceArg().indexOf(' ') >= 0) {
            fatal("workspace name \"" + getWorkSpaceArg() +
                  "\" may not contain space character");
        }
        if (getBuildSpaceArg().indexOf(' ') >= 0) {
            fatal("buildspace name \"" + getBuildSpaceArg() +
                  "\" may not contain space character");
        }

    }

    /**
     * Override this method in a subclass if you want to parse extra arguments.
     *
     * @throw ArrayIndexOutOfBoundsException if expected paramater is missing.
     */
    public int parseOneArgument(String args[], int i)
        throws ArrayIndexOutOfBoundsException
    {
        int starting_i = i;

        String arg = args[i++];
        if (arg.equals("-buildspace")) {
            this.buildspace = args[i++];
        }
        else if (arg.equals("-product")) {
            this.product = args[i++];
        }
        else if (arg.equals("-platform")) {
            this.platform = args[i++];
        }
        else if (arg.equals("-database")) {
            this.database = args[i++];
        }
        else if (arg.equals("-workspace")) {
            this.workspace = args[i++];
        }
        else {
            // unknown argument
            return -1;
        }

        return i - starting_i;
    }

    void initConfigurator() throws Exception {
        configurator = new Configurator();

        String workspace = getWorkSpaceArg();
        String input = workspace + "/src/vm/share/utilities/BuildFlags.hpp";

        configurator.readProductFile(product);
        configurator.readPlatformFile(platform);
        configurator.readInputFile(input);
    }

    public static void fatal(String message) {
        System.out.println("Error: " + message);
        System.exit(1);
    }

    String getOutputFileFullPath(String f) {
        // IMPL_NOTE: allow different name than "win32_i386_ide"
        return buildspace + File.separator +
            "win32_i386_ide" + File.separator + f;
    }

    void makeDirExist(String dirName) {
        dirName = getOutputFileFullPath(dirName);
        File dir = new File(dirName);
        if (dir.exists()) {
            if (!dir.isDirectory()) {
                fatal("not a directory: " + dir);
            }
        } else {
            if (!dir.mkdir()) {
                fatal("failed to create directory: "+ dir);
            }
        }
    }

    public PrintWriter openOutputFile(String fileName) throws IOException {
        fileName = getOutputFileFullPath(fileName);
        File file = new File(fileName);
        File dir = file.getParentFile();
        if (!dir.exists()) {
            dir.mkdir();
        }
        return new PrintWriter(new FileWriter(fileName));
    }

    public abstract String getExecutablePath(String name);

    public String getAbsolutePath(String path) {
        File file = new File(path);
        if (!file.isAbsolute()) {
            path = pwd + File.separator + path;
        }

        while (path.indexOf("..") >= 0) {
            Vector v = splitPath(path);
            removeOneDotDot(v);
            path = joinPath(v);
        }

        return path;
    }

    static Vector splitPath(String path) {
        Vector v = new Vector();

        StringTokenizer st = new StringTokenizer(path, File.separator);
        while (st.hasMoreTokens()) {
            String f = st.nextToken();
            v.addElement(f);
        }
        return v;
    }

    static void removeOneDotDot(Vector v) {
        for (int i=0; i<v.size()-1; i++) {
            if (v.elementAt(i+1).equals("..")) {
                v.removeElementAt(i);
                v.removeElementAt(i);
                return;
            }
        }
    }

    static String joinPath(Vector v) {
        String sep = "";
        StringBuffer sbuf = new StringBuffer();
        for (int i=0; i<v.size(); i++) {
            sbuf.append(sep);
            sbuf.append(v.elementAt(i));
            sep = File.separator;
        }
        return sbuf.toString();
    }

    public Vector getAPISources() {
        if (apiSources == null) {
            apiSources = findAPISources();
        }
        return apiSources;
    }

    Vector findAPISources() {
        Vector v = new Vector();
        String s = File.separator;
        String cldc_ver;
        if (isOptionEnabled("ENABLE_REFLECTION")) {
            cldc_ver = "cldc1.1plus";
        } else if (isOptionEnabled("ENABLE_CLDC_11")) {
            cldc_ver = "cldc1.1";
        } else {
            cldc_ver = "cldc1.0";
        }

        findSources(v, workspace + s + "src" + s + "javaapi" + s + cldc_ver,
                    ".java");
        findSources(v, workspace + s + "src" + s + "javaapi" + s + "share",
                    ".java");
        return v;
    }

    public boolean isDefaultBuildspace() {
        return workspace.equals("..\\..") && buildspace.equals("..");
    }

    public Vector getJccSources() {
        if (jccSources == null) {
            jccSources = findJccSources();
        }
        return jccSources;
    }

    Vector findJccSources() {
        Vector v = new Vector();
        String s = File.separator;
        findSources(v, workspace + s + "src" + s + "tools" + s + "jcc",
                    ".java");
        return v;
    }

    void findSources(Vector v, String path, String extension) {
        File dir = new File(path);
        if (dir.isDirectory()) {
            if (path.endsWith("SCCS")) {
                return;
            }
            if (path.endsWith("CVS")) {
                return;
            }

            String files[] = dir.list();
            for (int i=0; i<files.length; i++) {
                findSources(v, path + File.separator + files[i], extension);
            }
        } else {
            if (extension.equals(".java")) {
                if (!isOptionEnabled("ENABLE_ISOLATES") &&
                    (path.indexOf("isolate") != -1 ||
                     path.endsWith("Reflect.java"))) {
                    return;
                }
                if (!isOptionEnabled("ENABLE_DYNUPDATE") &&
                    path.indexOf("dynupdate") != -1) {
                    return;
                }
                if (!isOptionEnabled("ENABLE_METHOD_TRAPS") &&
                    path.endsWith("MethodTrap.java")) {
                    return;
                }
                if (!isOptionEnabled("ENABLE_JAVA_DEBUGGER") &&
                    path.endsWith("DebuggerInvoke.java")) {
                    return;
                }
            }
            if (path.endsWith(extension)) {
                v.addElement(path);
            }
        }
    }

    /**
     * Sets this.userSettings, which contains the preferences of the user.
     * If the user does not specify a value for a certain build option,
     * a default value is taken. See Configurator.java for details.
     */
    public void setUserSettings(Properties s) {
        userSettings = s;
    }

    public Properties getUserSettings() {
        return userSettings;
    }

    public boolean isOptionEnabled(String name) {
        String value = (String)getUserSettings().get(name);
        if ("default".equals(value)) {
            value = configurator.getDefaultValue(name);
        }
        return "true".equals(value);
    }
}
