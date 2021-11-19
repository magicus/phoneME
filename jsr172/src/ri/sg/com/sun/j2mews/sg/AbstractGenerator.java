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

package com.sun.j2mews.sg;

import java.io.*;
import java.util.*;
import java.text.*;

import javax.xml.namespace.QName;
import com.sun.xml.rpc.processor.model.Model;
import com.sun.xml.rpc.processor.model.AbstractType;
import com.sun.xml.rpc.processor.model.java.*;
import com.sun.xml.rpc.encoding.InternalEncodingConstants;
import com.sun.xml.rpc.wsdl.document.soap.SOAPConstants;
import com.sun.xml.rpc.processor.model.soap.*;
import com.sun.xml.rpc.processor.model.*;
import com.sun.xml.rpc.processor.model.literal.*;
import com.sun.xml.rpc.processor.generator.*;
import com.sun.xml.rpc.processor.*;
import com.sun.xml.rpc.processor.config.Configuration;
import com.sun.xml.rpc.processor.model.Service;
import com.sun.xml.rpc.processor.ProcessorOptions;
import com.sun.xml.rpc.processor.util.GeneratedFileInfo;
import com.sun.xml.rpc.util.localization.*;

import org.netbeans.modules.schema2beansdev.gen.*;

/**
 * We'll generate the Java client proxy for JSR-172.
 *
 */
public abstract class AbstractGenerator {
    static final public String FLOAT_DOUBLE_TO_STRING = "FLOAT_DOUBLE_TO_STRING";
    static final public String SHOW_ALL_CLDC1_0_INFO = "SHOW_ALL_CLDC1_0_INFO";
    static final public String EXPAND_ARGUMENTS = "EXPAND_ARGUMENTS";
    static final public String OPTIMIZE = "OPTIMIZE";
    
    protected com.sun.xml.rpc.processor.util.ProcessorEnvironment env;
    protected Service service;
    protected Port port;

    protected JavaWriter jw;
    protected String fullClassName;
    protected String className;
    protected String packageName;
    protected File sourceDir;

    /**
     * If you're going to call getMessage, then you need to set
     * messageFactory.
     */
    protected LocalizableMessageFactory messageFactory;
    protected Localizer localizer = new Localizer();

    /**
     * Whether or not the formal parameters on the Stub should get expanded.
     * It will be done for each parameter and return type, if there's
     * only 1 of them and it's a JavaStructureType.
     */
    protected boolean expandArguments = false;

    /**
     * Whether to trade readable generated code for more efficient generated
     * code.
     */
    protected boolean optimize = false;

    /**
     * Whether or not to display every info message, or just 1 (similar to
     * javac's -deprecation flag).
     */
    protected boolean showCldc1_0Info = false;

    /**
     * Sets the internal state of fullClassName, className, & packageName.
     * @param name the full class name
     */
    protected void setFullClassName(String name) {
        fullClassName = name;
        int pos = fullClassName.lastIndexOf('.');
        if (pos >= 0) {
            packageName = fullClassName.substring(0, pos);
            className = fullClassName.substring(pos+1, fullClassName.length());
        } else {
            packageName = null;
            className = fullClassName;
        }
    }

    public void setEnvironment(com.sun.xml.rpc.processor.util.ProcessorEnvironment e) {
        env = e;
    }

    public void perform(Model model, Configuration config, Properties options) {
        //env = config.getEnvironment();
        sourceDir = new File(options.getProperty(ProcessorOptions.SOURCE_DIRECTORY_PROPERTY));
        if ("true".equals(options.getProperty(EXPAND_ARGUMENTS)))
            expandArguments = true;
        else
            expandArguments = false;
        if ("true".equals(options.getProperty(SHOW_ALL_CLDC1_0_INFO)))
            showCldc1_0Info = true;
        else
            showCldc1_0Info = false;
        if ("true".equals(options.getProperty(OPTIMIZE)))
            optimize = true;
        else
            optimize = false;

        try {
            for (Iterator it = model.getServices(); it.hasNext(); ) {
                service = (Service) it.next();
                generate(service);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public void generate(Service service) throws IOException {
        for (Iterator it = service.getPorts(); it.hasNext(); ) {
            port = (Port) it.next();
            setFullClassName(getFullClassName());
            generate();
        }
    }

    /**
     * The port and env variables will be set correctly before
     * calling this method.
     */
    protected abstract String getFullClassName();

    /**
     * At this point, the service and port variables are set.
     */
    protected void generate() throws java.io.IOException {
        jw = new JavaWriter();

        jw.comment("This class was generated by 172 StubGenerator.");
        jw.comment("Contents subject to change without notice.");
        jw.comment("@generated");
        jw.cr();
        jw.writePackage(packageName);
        jw.cr();
        writeImports();

        generateClass();

        writeOutGeneratedFile();
    }

    protected void writeOutGeneratedFile() throws IOException {
        File srcFile = getSourceFile();
        log("Generating Java class for: "+srcFile.getName());
        OutputStream out = new FileOutputStream(srcFile);
        jw.writeTo(out);
        out.close();
        GeneratedFileInfo fi = new GeneratedFileInfo();
        fi.setFile(srcFile);
        fi.setType(getSourceFileType());
        env.addGeneratedFile(fi);
    }

    /**
     * The port and env variables will be set correctly before
     * calling this method.
     * This is the File that gets written to.
     */
    protected abstract File getSourceFile();

    /**
     * @see GeneratorConstants
     */
    protected abstract String getSourceFileType();

    /**
     * Here's your chance to import anything.
     */
    protected void writeImports() throws IOException {
    }

    protected abstract void generateClass() throws java.io.IOException;

    /**
     * @return the return type taking into account expandArguments.
     */
    protected JavaType getExpandedReturnType(JavaMethod method) {
        JavaType returnType = method.getReturnType();
        if (returnType == null || "void".equals(javaTypeToString(returnType)))
            return returnType;
        boolean isNillable = false;
        if (expandArguments && !isNillable &&
            returnType instanceof JavaStructureType) {
            JavaStructureType jst = (JavaStructureType) returnType;
            if (jst.getMembersCount() == 1) {
                returnType = ((JavaStructureMember)jst.getMembers().next()).getType();
            }
        }
        return returnType;
    }

    /**
     * @return a List of parameters to this method in the stub, taking
     * into account expandArguments.
     */
    protected List getExpandedParametersList(JavaMethod method) {
        List parameterList = method.getParametersList();
        if (parameterList.size() == 1 && expandArguments) {
            JavaParameter parameter = (JavaParameter) parameterList.iterator().next();
            Parameter p = parameter.getParameter();
            JavaType parameterType = parameter.getType();
            Block parameterBlock = p.getBlock();
            boolean isNillable = parameterBlock.getType().isNillable();
            if (parameterType instanceof JavaStructureType && !isNillable) {
                JavaStructureType jst = (JavaStructureType) parameterType;
                parameterList = new ArrayList(jst.getMembersCount());
                for (Iterator members = jst.getMembers(); members.hasNext(); ) {
                    JavaStructureMember jsm = (JavaStructureMember) members.next();
                    String memberName = jsm.getName();
                    JavaType memberType = jsm.getType();
                    parameter = new JavaParameter(memberName, memberType, p);
                    parameterList.add(parameter);
                }
            }
        }
        return parameterList;
    }


    protected String javaTypeToString(JavaType type) {
        if (type == null)
            return null;
        String result;
        if (type.isHolder())
            result = env.getNames().holderClassName(port, type);
        else
            result = env.getNames().typeClassName(type);
        return result;
    }

    /**
     * Is @param type valid for MIDP?
     * This assumes CLDC 1.1 (where float and double are okay).
     */
    static public boolean isValidType(String type) {
        if (type == null)  // "void" type
            return true;
        type = type.intern();
        if (type == "java.math.BigDecimal" || type == "java.math.BigInteger")
            return false;
        return true;
    }

    /**
     * Try to get a unique and somewhat nice looking name.
     */
    protected String makeVarName(QName name, String prefix, Map usedNames) {
        String result = prefix+name.getLocalPart();
        result = result.replace('.', '_');
        result = result.replace('/', '_');
        result = result.replace(':', '_');
        if (usedNames != null && usedNames.containsKey(result)) {
            String abbrevNamespace = name.getNamespaceURI();
            if (abbrevNamespace != null) {
                int pos = abbrevNamespace.lastIndexOf('/');
                if (pos >= 0)
                    abbrevNamespace = abbrevNamespace.substring(pos+1, abbrevNamespace.length());
                abbrevNamespace = abbrevNamespace.replace('.', '_');
                abbrevNamespace = abbrevNamespace.replace(':', '_');
                abbrevNamespace = abbrevNamespace.replace('/', '_');
                abbrevNamespace = abbrevNamespace.replace('%', '_');
                result = prefix+abbrevNamespace+"_"+name.getLocalPart();
            }
            result = makeUniq(result, usedNames);
        }
        return result;
    }

    protected String instanceOf(JavaType type, String suffix, Map usedNames) {
        String name = type.getName();
        return instanceOf(name, suffix, usedNames);
    }
    
    protected String instanceOf(JavaType type, Map usedNames) {
        return instanceOf(type, null, usedNames);
    }

    protected String instanceOf(QName name, String suffix, Map usedNames) {
        return instanceOf(name.getLocalPart(), suffix, usedNames);
    }
    
    /**
     * Make a variable name valid and unique.
     */
    protected String instanceOf(String name, String suffix, Map usedNames) {
        int pos;
        // Remove everything before the .
        pos = name.lastIndexOf('.');
        if (pos > 0)
            name = name.substring(pos+1, name.length());

        // Get rid of any trailing []
        int arrayDimensions = 0;
        while (name.endsWith("[]")) {
            name = name.substring(0, name.length()-2);
            ++arrayDimensions;
        }
        for (int i = 0; i < arrayDimensions; ++i)
            name += "Array";

        if (JavaUtil.isPrimitiveType(name))
            name = "a_"+name;

        if (suffix != null)
            name += suffix;

        /*
        // make first char lowercase
        name = java.beans.Introspector.decapitalize(name);
        */
        name = env.getNames().validJavaMemberName(name);

        return makeUniq(name, usedNames);
    }

    /**
     * Make a variable name unique.
     * Puts the name into usedNames.
     */
    protected String makeUniq(String name, Map usedNames) {
        if (usedNames.containsKey(name) || env.getNames().isJavaReservedWord(name)) {
            String baseName = name;
            
            for (int count = optimize ? 0 : 2; usedNames.containsKey(name); ++count)
                name = baseName + (optimize ? Integer.toString(count, Character.MAX_RADIX) : ""+count);
        }
        usedNames.put(name, null);
        return name;
    }

    /**
     * This will return a name from @param fullName where everything upto
     * and including the last '.' is removed.
     *   eg: "java.lang.String[]" -> "String[]"
     *       "java.util.ArrayList" -> "ArrayList"
     */
    protected static String baseName(String fullName) {
        int pos = fullName.lastIndexOf('.');
        if (pos == -1)
            return fullName;
        return fullName.substring(pos+1, fullName.length());
    }

    protected void onWarning(Localizable msg) {
        //System.err.println("Warning: "+msg);
        env.warn(msg);
    }

    protected void onInfo(Localizable msg) {
        //System.err.println("Info: "+msg);
        env.info(msg);
    }

    protected void onError(Localizable msg) {
        //System.err.println("Error: "+msg);
        env.error(msg);
    }

    protected Localizable getMessage(String key) {
        return messageFactory.getMessage(key);
    }

    protected Localizable getMessage(String key, String arg) {
        return messageFactory.getMessage(key, new Object[] {arg});
    }

    protected Localizable getMessage(String key, String arg1, String arg2) {
        return messageFactory.getMessage(key, new Object[] {arg1, arg2});
    }

    protected Localizable getMessage(String key, String arg1, String arg2, String arg3) {
        return messageFactory.getMessage(key, new Object[] {arg1, arg2, arg3});
    }

    protected Localizable getMessage(String key, Localizable l) {
        return messageFactory.getMessage(key, new Object[] {l});
    }

    protected Localizable getMessage(String key, Object[] args) {
        return messageFactory.getMessage(key, args);
    }

    protected String localize(Localizable msg) {
        return localizer.localize(msg);
    }

    public void setLocalizer(Localizer l) {
        localizer = l;
    }

    protected void log(String msg) {
        if (env.verbose()) {
            System.out.println("["+env.getNames().stripQualifier(this.getClass().getName())+": "+msg+"]");
        }
    }
}
