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

package runtime;

import components.*;
import vm.*;
import jcc.Util;
import jcc.Const;
import jcc.EVMConst;
import util.*;
import java.util.*;
import java.io.OutputStream;

/*
 * Generate natives table for VM.
 */

public class CLDC_HI_NativesWriter 
    implements CoreImageWriter, Const, EVMConst
{
    /*
     * Natives in the VM can be invoked in two ways -- as a "native" or as an 
     * "entry". Most methods are of the "native" type, unless specified in the
     * following table.
     *
     * Note [1] that a method can be both "native" and "entry". Example:
     *          java.lang.System.arraycopy().
     * Note [2] a "entry" method doesn't necessarily have the ACC_NATIVE
     *          flag -- the VM may try to execute a simple version of the
     *          method in "entry" code. If that fails, it will revert back
     *          to the Java implementation. Example: java.lang.String.compareTo
     */
    static String SYSTEM       = "java/lang/System";
    static String STRING       = "java/lang/String";
    static String MATH         = "java/lang/Math";
    static String VECTOR       = "java/util/Vector";
    static String STRINGBUFFER = "java/lang/StringBuffer";
    static String INTEGER      = "java/lang/Integer";
    static String JVM          = "com/sun/cldchi/jvm/JVM";

    static String[][] use_entries = {
        // class     method  signature(null=any)  entry_name
        {SYSTEM, "arraycopy",   null,   "native_system_arraycopy_entry",
                                        "also-native"},
        {STRING, "charAt",      null,   "native_string_charAt_entry"},
        {STRING, "<init>",     "(Ljava/lang/StringBuffer;)V",   
                                        "native_string_init_entry"},
        {STRING, "equals",      null,   "native_string_equals_entry"},
        {STRING, "indexOf",    "(Ljava/lang/String;I)I", 
                                        "native_string_indexof_string_entry"},
        {STRING, "indexOf",    "(Ljava/lang/String;)I",  
                                        "native_string_indexof0_string_entry"},
        {STRING, "indexOf",    "(II)I", "native_string_indexof_entry"},
        {STRING, "indexOf",    "(I)I",  "native_string_indexof0_entry"},
        {STRING, "startsWith", "(Ljava/lang/String;I)Z", 
                                        "native_string_startsWith_entry"},
        {STRING, "startsWith", "(Ljava/lang/String;)Z",
                                        "native_string_startsWith0_entry"},
        {STRING, "endsWith",    null,   "native_string_endsWith_entry"},
        {STRING, "substring", "(I)Ljava/lang/String;", 
                                        "native_string_substringI_entry"},
        {STRING, "substring", "(II)Ljava/lang/String;", 
                                        "native_string_substringII_entry"},
        {STRING, "valueOf",   "(I)Ljava/lang/String;",
                                        "native_integer_toString_entry"},
        {VECTOR, "elementAt",  "(I)Ljava/lang/Object;",   
                                        "native_vector_elementAt_entry"},
        {VECTOR, "addElement", "(Ljava/lang/Object;)V",   
                                        "native_vector_addElement_entry"},
        {STRINGBUFFER, "append",  "(C)Ljava/lang/StringBuffer;",   
                                        "native_stringbuffer_append_entry"},
        {INTEGER, "toString", "(I)Ljava/lang/String;",
                                        "native_integer_toString_entry"},
        {MATH,   "sin",        "(D)D",  "native_math_sin_entry"},
        {MATH,   "cos",        "(D)D",  "native_math_cos_entry"},
        {MATH,   "tan",        "(D)D",  "native_math_tan_entry"},
        {MATH,   "sqrt",       "(D)D",  "native_math_sqrt_entry"},
        {MATH,   "ceil",       "(D)D",  "native_math_ceil_entry"},
        {MATH,   "floor",      "(D)D",  "native_math_floor_entry"},
        {MATH,   "asin",       "(D)D",  "native_math_asin_entry"},
        {MATH,   "acos",       "(D)D",  "native_math_acos_entry"},
        {MATH,   "atan",       "(D)D",  "native_math_atan_entry"},
        {MATH,   "atan2",      "(DD)D", "native_math_atan2_entry"},
        {JVM, "unchecked_byte_arraycopy", null,  
                               "native_jvm_unchecked_byte_arraycopy_entry"},
        {JVM, "unchecked_char_arraycopy", null,  
                               "native_jvm_unchecked_char_arraycopy_entry"},
        {JVM, "unchecked_int_arraycopy",  null,  
                               "native_jvm_unchecked_int_arraycopy_entry"},
        {JVM, "unchecked_long_arraycopy",  null,  
                               "native_jvm_unchecked_long_arraycopy_entry"},
        {JVM, "unchecked_obj_arraycopy",  null,  
                               "native_jvm_unchecked_obj_arraycopy_entry"}
    };

    /* INSTANCE DATA */
    protected  String    outputFileName;
    protected  Exception failureMode = null; // only interesting on failure

    CCodeWriter out;

    protected     vm.VMClassFactory     classMaker = new vm.EVMClassFactory();

    boolean formatError;

    public CLDC_HI_NativesWriter( ){ 
    }

    public void init( boolean classDebug, ClassnameFilterList nativeTypes, 
                      boolean verbose, int maxSegmentSize ){
    }

    public boolean setAttribute( String attribute ){
        return false; 
    }


    public boolean open( String filename ){
        if ( out != null ) { 
            close();
        }
        outputFileName = filename;
        if ( filename == null){
            out = new CCodeWriter(System.out);
        } else {
            try {
                OutputStream os = new java.io.FileOutputStream( filename );
                out = new CCodeWriter(os);
            } catch ( java.io.IOException e ){
                failureMode = e;
                return false;
            }
        }
        return true;
    }


    public void close(){
        if (out != null) { 
            out.close();
            outputFileName = null;
            out = null;
        }
    }


    public boolean writeClasses( ConstantPool consts ) {
        return writeClasses(consts, null);
    }

    public void printError( java.io.PrintStream o ){
        if ( failureMode != null ){
            failureMode.printStackTrace( o );
        } else {
            if ( out != null && out.checkError() )
                o.println(outputFileName+": Output write error");
        }
    }

    public boolean writeClasses( ConstantPool consts, 
                                 ConstantPool sharedconsts ){
        ClassClass classes[] = ClassClass.getClassVector( classMaker );
        ClassClass.setTypes();
        
        // write out some constant pool stuff here,
        writeProlog();
        
        try {
        writeAllNativeTables(classes);
        } catch (RuntimeException e) { 
            out.flush();
            System.out.println(e);
            e.printStackTrace(System.out);
            formatError = true;
        }
    writeEpilog();
        return (!formatError) && (! out.checkError());
    }
    
    int checkEntry(EVMClass cc, MethodInfo mi) {
        for (int i=0; i<use_entries.length; i++) {
            if (cc.ci.className.equals(use_entries[i][0]) &&
                mi.name.string.equals(use_entries[i][1])) {
                if (use_entries[i][2] == null) {
                    return i;
                }
                if (mi.type.string.equals(use_entries[i][2])) {
                    return i;
                }
            }
        }
        return -1;
    }

    boolean isNativeFunc(EVMClass cc, MethodInfo mi) {
        int entry;
        entry = checkEntry(cc, mi);
        if (entry < 0 || use_entries[entry].length > 4) {
            return true;
        } else {
            return false;
        }
    }

    boolean isEntryFunc(EVMClass cc, MethodInfo mi) {
        int entry;
        entry = checkEntry(cc, mi);
        if (entry >= 0) {
            return true;
        } else {
            return false;
        }
    }

    boolean isNativeOrEntryFunc(EVMClass cc, MethodInfo mi) {
        if ((mi.access & Const.ACC_NATIVE) != 0) {
            return true;
        } else {
            return isEntryFunc(cc, mi);
        }
    }

    void sortClasses(ClassClass classes[]) {
        ArrayList list = new ArrayList();
        for (int i=0; i<classes.length; i++) {
            list.add(classes[i]);
        }
        Collections.sort(list, new Comparator() {
            public int compare(Object o1, Object o2) {
                ClassClass c1 = (ClassClass)o1;
                ClassClass c2 = (ClassClass)o2;

                return c1.ci.className.compareTo(c2.ci.className);
            }           
        });
        for (int i=0; i<list.size(); i++) {
            classes[i] = (ClassClass)list.get(i);
        }
    }

    void sortMethods(EVMMethodInfo methods[]) {
        ArrayList list = new ArrayList();
        for (int i=0; i<methods.length; i++) {
            list.add(methods[i]);
        }
        Collections.sort(list, new Comparator() {
            public int compare(Object o1, Object o2) {
                MethodInfo m1 = ((EVMMethodInfo)o1).method;
                MethodInfo m2 = ((EVMMethodInfo)o2).method;

                int n = m1.name.string.compareTo(m2.name.string);
                if (n != 0) {
                    return n;
                } else {
                    return m1.type.string.compareTo(m2.type.string);
                }
            }           
        });
        for (int i=0; i<list.size(); i++) {
            methods[i] = (EVMMethodInfo)list.get(i);
        }
    }

    protected void writeAllNativeTables(ClassClass classes[]) { 
        sortClasses(classes);
        Vector nativeClasses = new Vector();

        // (1) Write all the function declarations
        for (int i = 0; i < classes.length; i++){
        boolean classHasNatives = false;
            EVMClass cc = (EVMClass)classes[i];
            if (cc.isPrimitiveClass() || cc.isArrayClass()) { 
                continue;
            }
            EVMMethodInfo  m[] = cc.methods;
            sortMethods(m);
            int nmethod = (m == null) ? 0 : m.length;
            for (int j = 0; j < nmethod; j++) {
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
                if (!isNativeOrEntryFunc(cc, mi)) { 
                    continue;
                }
                if (!classHasNatives) { 
                    classHasNatives = true;
                    nativeClasses.addElement(cc);
                }
                int entry;
                entry = checkEntry(cc, mi);
                if (entry < 0 || use_entries[entry].length > 4) {
                    // This is a "native"
                    out.println("extern \"C\" " + mi.getJNIReturnType() + " " +
                                mi.getNativeName(true) + "();");
                }
                if (entry >= 0) {
                    // This is an "entry" (could also be a "native")
                    out.println("extern \"C\" void " + 
                                use_entries[entry][3] + "();");
                }
            }
        }
        out.println();
        out.println();

        // (2) Write all the "_natives[]" and "_entries[]" tables for
        //     individual classes

        for (Enumeration e = nativeClasses.elements(); e.hasMoreElements();){
            EVMClass cc = (EVMClass)e.nextElement();
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = (m == null) ? 0 : m.length;
            int num_native_methods = 0;
            int num_entry_methods = 0;

            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
                if (!isNativeOrEntryFunc(cc, mi)) { 
                    continue;
                }
                if (isNativeFunc(cc, mi)) {
                    num_native_methods ++;
                }
                if (isEntryFunc(cc, mi)) {
                    num_entry_methods ++;
                }
            }

            // Write the "_natives[]" struct
            if (num_native_methods > 0) {
                out.println("static const JvmNativeFunction " + 
                       cc.getNativeName() + "_natives[] = " + "{");
                for (int j = 0; j < nmethod; j++) {
                    EVMMethodInfo meth = m[j];
                    MethodInfo mi = meth.method;
                    if (!isNativeOrEntryFunc(cc, mi)) { 
                        continue;
                    }
                    if (isNativeFunc(cc, mi)) {
                        out.print(pad("  JVM_NATIVE(\"" + mi.name.string + "\",", 30));
                        out.print(pad("\"" + mi.type.string + "\", ", 25));
                        out.println(mi.getNativeName(true) + "),");
                    }
                }
                out.println("  {(char*)0, (char*)0, (void*)0}");
                out.println("};");
                out.println();
            }

            // Write the "_entries[]" struct
            if (num_entry_methods > 0) {
                out.println("static const JvmNativeFunction " + 
                       cc.getNativeName() + "_entries[] = " + "{");
                for (int j = 0; j < nmethod; j++) {
                    EVMMethodInfo meth = m[j];
                    MethodInfo mi = meth.method;
                    if (!isNativeOrEntryFunc(cc, mi)) { 
                        continue;
                    }
                    int entry;
                    if ((entry = checkEntry(cc, mi)) >= 0) {
                        out.print("  JVM_ENTRY(\"" + mi.name.string + "\"," 
                                  + spaces(20 - mi.name.string.length()));
                        out.print("\"" + mi.type.string + "\", ");
                        out.println(use_entries[entry][3] + "),");
                    }
                }
                out.println("  {(char*)0, (char*)0, (void*)0}");
                out.println("};");
                out.println();
            }
        }

        // (3) Write the top-level table

        out.println("const JvmNativesTable jvm_natives_table[] = {");

        for (Enumeration e = nativeClasses.elements(); e.hasMoreElements();){
            EVMClass cc = (EVMClass)e.nextElement();
            String className = cc.ci.className;
            out.println("  JVM_TABLE(\"" + className + "\",");
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = (m == null) ? 0 : m.length;

            int num_native_methods = 0;
            int num_entry_methods = 0;

            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
                if (!isNativeOrEntryFunc(cc, mi)) { 
                    continue;
                }
                if (isNativeFunc(cc, mi)) {
                    num_native_methods ++;
                }
                if (isEntryFunc(cc, mi)) {
                    num_entry_methods ++;
                }
            }
            
            if (num_native_methods > 0) {
                out.println(spaces(40) + cc.getNativeName() + "_natives,");
            } else {
                out.println(spaces(40) + "(JvmNativeFunction*)0,");
            }
            if (num_entry_methods > 0) {
                out.println(spaces(40) + cc.getNativeName() + "_entries),");
            } else {
                out.println(spaces(40) + "(JvmNativeFunction*)0),");
            }
        }
        out.println("  JVM_TABLE((char*)0, (JvmNativeFunction*)0, " +
                    "(JvmNativeFunction*)0)");
        out.println("};");


        // (4) Write the jvm_native_execution_top-level table

        out.println("const JvmExecutionEntry jvm_api_entries[] = {");
        for (Enumeration e = nativeClasses.elements(); e.hasMoreElements();){
            EVMClass cc = (EVMClass)e.nextElement();
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = (m == null) ? 0 : m.length;

            for (int j = 0; j < nmethod; j++) {
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
                int index;
                if ((index = checkEntry(cc, mi)) >= 0) {
                    String entryName = use_entries[index][3];
                    out.println("{(unsigned char*)&" + entryName + ",");
                    out.println("(char*)\"" + entryName + "\"},");
                }
            }
        }
        out.println("{(unsigned char*)0, (char*)0}};");
    }

    String pad(String str, int width) {
        int len = str.length();
        if (len < width) {
            return str + spaces(width - len);
        } else {
            return str;
        }
    }

    protected void writeProlog(){
        out.println("/* This is a generated file.  Do not modify.");
        out.println(" * Generated on " + new java.util.Date());
        out.println(" */");
        out.println();
        out.println("/*");
        out.println(" *");
        out.println(" * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.");
        out.println(" * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
        out.println(" * ");
        out.println(" * This program is free software; you can redistribute it and/or");
        out.println(" * modify it under the terms of the GNU General Public License version");
        out.println(" * 2 only, as published by the Free Software Foundation.");
        out.println(" * ");
        out.println(" * This program is distributed in the hope that it will be useful, but");
        out.println(" * WITHOUT ANY WARRANTY; without even the implied warranty of");
        out.println(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
        out.println(" * General Public License version 2 for more details (a copy is");
        out.println(" * included at /legal/license.txt).");
        out.println(" * ");
        out.println(" * You should have received a copy of the GNU General Public License");
        out.println(" * version 2 along with this work; if not, write to the Free Software");
        out.println(" * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
        out.println(" * 02110-1301 USA");
        out.println(" * ");
        out.println(" * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
        out.println(" * Clara, CA 95054 or visit www.sun.com if you need additional");
        out.println(" * information or have any questions.");
        out.println(" */");
        out.println();
        out.println("#include \"jvmconfig.h\"");
        out.println("#if !defined(ROMIZING) || !defined(PRODUCT) || \\");
        out.println("    ENABLE_TTY_TRACE");
        out.println("#include \"NativesTable.hpp\"");
	out.println("#include \"kni.h\"");	
    out.println();
    out.println();
    }
    
    protected void writeEpilog() {
    out.println();
    out.println("#endif");
    }

    public void printSpaceStats(java.io.PrintStream stream) {}

    private String spaces(int length) { 
    if (length <= 1) { 
        return " ";
    } if (length <= 10) { 
        return "          ".substring(0, length);
    } else { 
        return spaces(10) + spaces(length - 10);
    }
    }

    private boolean isOverloadedNative(MethodInfo mi) { 
        ClassInfo ci = mi.parent;
    int nmethods = ci.methods.length;
    for (int j = 0; j < nmethods; j ++ ){
        MethodInfo m = ci.methods[j];
        if (    (m != mi) 
                 && ((m.access&Const.ACC_NATIVE) != 0 )
                 && (m.name.equals(mi.name))) { 
                return true;
            }
    } 
        return false;
    }

}
