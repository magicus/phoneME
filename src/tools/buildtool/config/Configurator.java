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

package config;

import java.io.*;
import java.util.*;
import util.*;

/**
 * The Configurator class deals with the creation of jvmconfig.h,
 * which controls the build options of the VM (e.g.,
 * ENABLE_JAVA_DEBUGGER, ENABLE_CLDC_11, etc).
 *
 * See src/vm/share/utilities/BuildFlags.hpp about how the content of
 * jvmconfig.h is specified.
 */
public class Configurator {
    Vector flags;
    Hashtable platform;
    Hashtable product;

    public Configurator() {

    }

    /**
     * Read input from platform file, which usually have contents like:
     * os_family    = linux
     * arch         = i386
     * carch        = thumb2
     * iarch        = c   
     * os_arch      = linux_i386
     * compiler     = gcc
     * cpu_variant  = 
     */
    public void readPlatformFile(String infile) throws Exception {
        platform = new Hashtable();

        readConfigFile(infile, platform);
    }

    public void readProductFile(String infile) throws Exception {
        product = new Hashtable();

        readConfigFile(infile, product);
    }

    private void readConfigFile(String infile, Hashtable properties) 
      throws Exception {
        FileInputStream in = new FileInputStream(infile);
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        String line;

        while ((line = reader.readLine()) != null) {
            StringTokenizer st = new StringTokenizer(line, "=");
            String name  = null;
            String value = null;

            try {
                name = st.nextToken();
                value = st.nextToken();
            } catch (Throwable t) {;}

            if (name == null) {
                continue;
            }
            name = name.trim();
            if (name.equals("")) {
                continue;
            }
            if (value == null) {
                value = "";
            } else {
                value = value.trim();
            }

            properties.put(name, value);
        }
    }

    /**
     * Read input from infile (usually BuildFlags.hpp)
     */
    public void readInputFile(String infile) throws Exception {
        FileInputStream in = new FileInputStream(infile);
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));

        String line;
        boolean started = false;
        EnableFlag currentFlag = null;
        flags = new Vector();

        while ((line = reader.readLine()) != null) {
            if (line.indexOf("{{ ENABLE_FLAGS_BEGIN") >= 0) {
                started = true;
            } else if (line.indexOf("ENABLE_FLAGS_END }}") >= 0) {
                break;
            } else {
                EnableFlag newFlag = tryStartFlag(line);
                if (newFlag == null) {
                    if (currentFlag != null) {
                        addComments(currentFlag, line);
                    }
                } else {
                    flags.addElement(newFlag);
                    currentFlag = newFlag;
                }
            }
        }
    }

    /**
     * Print a warning if an unknown ENABLE_XXX flag is specified in the
     * env. This is probably a typo by the user.
     */
    void checkSpuriousFlags(Hashtable env) {
        boolean warned = false;
        Vector names = getFlagNames();
        for (Enumeration e = env.keys(); e.hasMoreElements() ;) {
            String key = (String)e.nextElement();
            if (key.startsWith("ENABLE_") && !key.endsWith("__BY")
                && !key.equals("ENABLE_MAP_FILE")
                && !names.contains(key)) {
                if (!warned) {
                    System.out.println("*********");
                    warned = true;
                }
                System.out.println("Warning: unknown flag " + key);
            }
        }

        if (warned) {
            System.out.println("*********");
        }
    }

    /**
     * Write the output file. The env hashtable allows the user to specify
     * alternative values for the configuration flags.
     */
    public void write(String outfile, Hashtable env, Vector extra) 
        throws Exception
    {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintWriter writer = new PrintWriter(new OutputStreamWriter(baos));
        byte data[];

        checkSpuriousFlags(env);
        writeConfigProlog(writer);
        writePlatformFlags(writer);
        writeEnableFlags(writer, env);
        writeOptionDump(writer);
        writeExtraFlags(writer, extra);
        writeConfigEpilog(writer);
        writer.flush();
        data = baos.toByteArray();
        writer.close();

        writeFileIfNecessary(data, outfile);
    }

    public void writeFileIfNecessary(byte data[], String outfile)
        throws Exception
    {
        boolean necessary = true;

        File f = new File(outfile);
        if (f.exists()) {
            FileInputStream in = new FileInputStream(outfile);
            byte savedData[] = new byte[in.available()];
            if (in.read(savedData) == savedData.length) {
                necessary = false;
                for (int i=0; i<savedData.length; i++) {
                    if (data[i] != savedData[i]) {
                        necessary = true;
                        break;
                    }
                }
            }
            in.close();
        }

        if (necessary) {
            FileOutputStream out = new FileOutputStream(outfile);
            out.write(data);
            out.close();
        } else {
            System.out.println("jvmconfig.h has not changed");
        }
    }

    public Vector getFlagNames() {
        Vector v = new Vector();
        for (int i=0; i<flags.size(); i++) {
            EnableFlag flag = (EnableFlag)flags.elementAt(i);
            v.addElement(flag.name);
        }
        return v;
    }

    public String getDefaultValue(String name) {
        Vector v = new Vector();
        for (int i=0; i<flags.size(); i++) {
            EnableFlag flag = (EnableFlag)flags.elementAt(i);
            if (flag.name.equals(name)) {
                if (flag.debug.def != flag.product.def) {
                    throw new Error("Debug and product default values aren't "+
                                    "same");
                }
                switch (flag.debug.def) {
                case ALWAYS_DISABLE:
                case DISABLE:
                    return "false";
                default:
                    return "true";
                }
            }
        }
        throw new Error("Option \"" + name + "\" not found");
    }

    public String getProductName() {
      return (String)product.get(PRODUCT_NAME_KEY);
    }

    public String getReleaseVersion() {
      return (String)product.get(RELEASE_VERSION_KEY);
    }

    static final String PRODUCT_NAME_KEY    = "PRODUCT_NAME";
    static final String RELEASE_VERSION_KEY = "RELEASE_VERSION";

    static final int ALWAYS_DISABLE = 1;
    static final int DISABLE        = 2;
    static final int ENABLE         = 3;
    static final int ALWAYS_ENABLE  = 4;

    EnableFlag tryStartFlag(String line) {
        StringTokenizer st = new StringTokenizer(line, " \t\n\r");
        int tokenNumber = 0;
        String name = null;
        int debugDefault = -1000;
        int productDefault = -1000;
        String comments = "";

        while (st.hasMoreTokens()) {
            String token = st.nextToken();
            switch (tokenNumber) {
            case 0:
                if (token.equals("//")) {
                    continue;
                }
                else if (token.startsWith("ENABLE_")) {
                    name = token;
                    tokenNumber ++;
                    continue;
                } else {
                    return null;
                }

            case 1:
                if (token.length() != 3) {
                    return null;
                }
                if (token.charAt(1) != ',') {
                    return null;
                }
                if ((debugDefault = getDefault(token.charAt(0))) < 0) {
                    return null;
                }
                if ((productDefault = getDefault(token.charAt(2))) < 0) {
                    return null;
                }
                tokenNumber ++;
                break;

            default:
                if (token.indexOf("=====") < 0) {
                    comments = comments + " " + token;
                }
            }
        }

        if (tokenNumber <= 1) {
            return null;
        }

        EnableFlag flag = new EnableFlag();
        flag.name = name;
        flag.debug.def = debugDefault;
        flag.product.def = productDefault;
        flag.comments = comments;

        return flag;
    }
 
    void addComments(EnableFlag flag, String line) {
        StringTokenizer st = new StringTokenizer(line, " \t\n\r");
        String comments = "";

        while (st.hasMoreTokens()) {
            String token = st.nextToken();
            if (token.equals("//") && comments.equals("")) {
                continue;
            } else if (token.indexOf("=====") >= 0) {
                continue;
            } else {
                comments = comments + " " + token;
            }
        }
        if (comments.length() > 0) {
            flag.comments += " " + comments;
        }
    }

    int getDefault(char c) {
        switch (c) {
        case '-': return ALWAYS_DISABLE;
        case '0': return DISABLE;
        case '1': return ENABLE;
        case '+': return ALWAYS_ENABLE;
        default:  return -1;
        }
    }

    void writeConfigProlog(PrintWriter writer) throws Exception {
        InputStream in =
            Configurator.class.getResourceAsStream("config_prolog.txt");
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        String line;

        while ((line = reader.readLine()) != null) {
            writer.println(line);
        }
        in.close();
    }

    void writePlatformFlags(PrintWriter writer)
        throws Exception
    {
        String arch     = (String)platform.get("arch");
        String iarch    = (String)platform.get("iarch");
        String carch    = (String)platform.get("carch");
        String compiler = (String)platform.get("compiler");

        if (iarch == null || iarch.equals("")) {
            iarch = arch;
        }
        if (carch == null || carch.equals("")) {
            carch = arch;
        }

        writer.println();
        writer.println("// BEGIN: Information derived from \"platform\" file");

        if (carch.equals("arm") || iarch.equals("arm")) {
            writer.println("#ifndef ARM");
            writer.println("#define ARM 1");
            writer.println("#endif");
        }
        else if (carch.equals("thumb2") || iarch.equals("thumb2")) {
            writer.println("#ifndef ARM");
            writer.println("#define ARM 1");
            writer.println("#endif");
        }
        else if (carch.equals("sh") || iarch.equals("sh")) {
            writer.println("#ifndef HITACHI_SH");
            writer.println("#define HITACHI_SH1");
            writer.println("#endif");
        }

        writer.println("#define INTERPRETER_ARCH_NAME \"" + iarch + "\"");
        writer.println("// END: Information derived from \"platform\" file");
        writer.println();
    }

    void writeEnableFlags(PrintWriter writer, Hashtable env)
        throws Exception
    {
        for (int i=0; i<flags.size(); i++) {
            EnableFlag flag = (EnableFlag)flags.elementAt(i);
            writeFlag(writer, env, flag);
        }

    }

    public void writeExtraFlags(PrintWriter writer, Vector extra) 
        throws Exception
    {
        for (int i=0; i<extra.size(); i++) {
            String flag = (String)extra.elementAt(i);
            flag = flag.replace('=', ' ');
            writer.println();
            writer.println("/* Extra flag for this build configuration */");
            writer.println("#define " + flag);
        }
    }

    void writeConfigEpilog(PrintWriter writer) throws Exception {
        writer.println();
        writer.println("#endif /* _JVM_CONFIG_H_ */");
        writer.println();
    }

    void writeFlag(PrintWriter writer, Hashtable env, EnableFlag flag) {
        writer.println();
        writer.println("/" + "*\n * " + flag.name);
        writer.print(" *      ");
 
        int linepos = 0;
        StringTokenizer st = new StringTokenizer(flag.comments);
        while (st.hasMoreTokens()) {
            String token = st.nextToken();
            if (linepos > 0 && token.length() + linepos > 60) {
                writer.println();
                writer.print(" *      ");
                linepos = 0;
            }
            writer.print(" ");
            writer.print(token);
            linepos += 1 + token.length();
        }
        writer.println();
        writer.println(" *" + "/");
        writer.println();
 
        getValue(env, flag.name, flag.debug);
        getValue(env, flag.name, flag.product);

        if (flag.debug.value == flag.product.value &&
            flag.debug.source.equals(flag.product.source)) {
            writeValue(writer, flag.name, flag.debug);
        } else {
            writer.println("#ifndef PRODUCT");
            writeValue(writer, flag.name, flag.debug);
            writer.println("#else");
            writeValue(writer, flag.name, flag.product);
            writer.println("#endif");
       }
    }

    void writeValue(PrintWriter writer, String name, FlagValue value) {
        int i;
        if (name.trim().equals("ENABLE_PCSL")) {
            writer.println("#ifndef " + name);
        }
        writer.print("#define " + name + " ");
        for (i=name.length() + 1; i<30; i++) {
            writer.print(" ");
        }
        writer.println((value.value ? "1" : "0") +
                       "  /* " + value.source + " */");
        if (name.trim().equals("ENABLE_PCSL")) {
            writer.println("#endif");
        }
    }

    void getValue(Hashtable env, String name, FlagValue value) {
        String envValue  = (String)env.get(name);
        String envSource = (String)env.get(name + "__BY");
        boolean envDefault = false;

        if (envValue != null) {
            if (envValue.equals("true")) {
                envDefault = true;
            } else if (envValue.equals("false")) {
                envDefault = false;
            } else {
                System.out.println("Bad value \"" + envValue + 
                                   "\" for env variable \"" + name +
                                   "\"");
                System.out.println("Must be \"true\" or \"false\".\n");
                System.exit(0);
                return;
            }
        }

        switch (value.def) {
        case ALWAYS_DISABLE:
            value.value = false;
            value.source = "Always disabled";
            break;
        case ALWAYS_ENABLE:
            value.value = true;
            value.source = "Always enabled";
            break;
        default:
            if (envValue == null) {
                value.value = (value.def == ENABLE);
                value.source = "VM default: BuildFlags.hpp";
            } else {
                value.value = envDefault;
                if ("idetool".equals(envSource)) {
                    value.source = "User setting in idetool";
                } else if (envSource != null) {
                    if (!envSource.startsWith("configurator override:")) {
                        value.source = "Platform default: " + envSource;
                    } else {
                        value.source = envSource;
                    }
                } else {
                    value.source = "User environment variable";
                }
            }
        }
    }

    /**
     * Writes the names and values of the flags into a table, so that
     * it's easy for C code to dump the values of the flags (for debugging 
     * purposes).
     */
    void writeOptionDump(PrintWriter writer) {
        writer.println();
        writer.println("/*");
        writer.println(" * The following table is for dumping the flags");
        writer.println(" * for debugging purposes.");
        writer.println(" */");
        writer.println();
        writer.println("#ifndef PRODUCT");
        writeDump(writer, flags, true);
        writer.println("#else");
        writeDump(writer, flags, false);
        writer.println("#endif");
    }

    void writeDump(PrintWriter writer, Vector flags, boolean isDebug) {
        writer.println("#define ENABLE_FLAG_VALUES { \\");
        for (int i=0; i<flags.size(); i++) {
            EnableFlag flag = (EnableFlag)flags.elementAt(i);
            FlagValue value;
            if (isDebug) {
                value = flag.debug;
            } else {
                value = flag.product;
            }
            writer.print("\t\"" + flag.name + "\", ");
            for (int j=flag.name.length(); j<30; j++) {
                writer.print(" ");
            }
            if (value.value) {
                writer.print("\"enabled\",");
            } else {
                writer.print("\"-\",");
            }
            writer.println(" \\");
        }
        writer.println("}");
    }
}

class EnableFlag {
    public String name;
    public String comments;
    public FlagValue debug;
    public FlagValue product;

    public EnableFlag() {
        debug = new FlagValue();
        product = new FlagValue();
    }
}

class FlagValue {
    public int def;
    public String source;
    public boolean value;
}
