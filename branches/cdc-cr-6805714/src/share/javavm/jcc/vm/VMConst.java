/*
 * @(#)VMConst.java	1.10 06/10/10
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
 *
 */

package vm;

/**
 * Constants used by the Java linker which are not exported in any
 * direct way from the Java runtime or compiler.
 * These are constants of the VM implementation.
 * Note: We should separate JVM and CVM versions.
 * For the time being, this is the CVM Version.
 * This does not include class-file-related constants, which are in
 * ClassFileConst, or the opcode values or names, which are in the
 * derived file OpcodeConst.
 */

public interface VMConst
{

    public static int sysLockFlag =        1;
    public static int resolvedFlag =       2;
    public static int errorFlag =          4;
    public static int softRefFlag =      0x8;
    public static int initializedFlag =  0x10;
    public static int linkedFlag =       0x20;
    public static int verifiedFlag =     0x40;
    public static int primitiveFlag =    0x100;
    public static int referencedFlag =   0x200;
    public static int stickyFlag =       0x400;
    public static int scannableFlag =    0x1000;
    public static int hasFinalizerFlag = 0x2000;

    public static int classFlags = 
        sysLockFlag | initializedFlag | linkedFlag | resolvedFlag | stickyFlag;

    // For now, perform no inlining of methods
    public static final int noinlineFlag = 0x1 << 24;

    public static final int OBJ_FLAG_CLASS_ARRAY =  1;
    public static final int OBJ_FLAG_BYTE_ARRAY  =  3;
    public static final int OBJ_FLAG_SHORT_ARRAY =  5;
    public static final int OBJ_FLAG_INT_ARRAY   =  7;
    public static final int OBJ_FLAG_LONG_ARRAY  =  9;
    public static final int OBJ_FLAG_FLOAT_ARRAY =  11;
    public static final int OBJ_FLAG_DOUBLE_ARRAY = 13;
    public static final int OBJ_FLAG_BOOLEAN_ARRAY = 15;
    public static final int OBJ_FLAG_CHAR_ARRAY    = 17;

    /**
     * From src/share/java/include/tree.h
     */
    public static final int ACC_DOUBLEWORD = 0x4000;
    public static final int ACC_REFERENCE  = 0x8000;

    public static final byte ROM_Initializing         = 0x01;
    public static final byte ROM_Initialized          = 0x02;

    public static final byte INVOKE_JAVA_METHOD			   = 0;
    public static final byte INVOKE_SYNCHRONIZED_JAVA_METHOD	   = 1;
    public static final byte INVOKE_NATIVE_METHOD		   = 2;
    public static final byte INVOKE_SYNCHRONIZED_NATIVE_METHOD	   = 3;
    public static final byte INVOKE_JNI_NATIVE_METHOD		   = 4;
    public static final byte INVOKE_JNI_SYNCHRONIZED_NATIVE_METHOD = 5;
    public static final byte INVOKE_LAZY_NATIVE_METHOD		   = 6;
    public static final byte INVOKE_ABSTRACT_METHOD		   = 7;
    public static final byte INVOKE_COMPILED_METHOD		   = 8;
    public static final byte INVOKE_UNSAFE			   = 9;

}
