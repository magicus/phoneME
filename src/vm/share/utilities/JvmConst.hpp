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

/** \file JvmConst.hpp
 * Constants from the Java Virtual Machine Specification
 *
 * This file defines various constants that are an integral
 * part of the Java Virtual Machine Specification (JVMS).
 *
 * Bibliographic reference:
 *  Java Virtual Machine
 * Specification (Java Series), 2nd Edition, Addison-Wesley,
 * 1999, ISBN 0-201-43294-3.
 */

/* Constants in class files [JVMS pages 96, 113, 115] */
#define JVM_ACC_PUBLIC        0x0001  /* visible to everyone */
#define JVM_ACC_PRIVATE       0x0002  /* visible only to the defining class */
#define JVM_ACC_PROTECTED     0x0004  /* visible to subclasses */
#define JVM_ACC_STATIC        0x0008  /* method or variable is static */
#define JVM_ACC_FINAL         0x0010  /* no further subclassing, overriding */
#define JVM_ACC_SYNCHRONIZED  0x0020  /* wrap method call in monitor lock */
#define JVM_ACC_SUPER         0x0020  /* funky handling of invokespecial */
#define JVM_ACC_VOLATILE      0x0040  /* can cache in registers */
#define JVM_ACC_TRANSIENT     0x0080  /* not persistent */
#define JVM_ACC_NATIVE        0x0100  /* implemented in C */
#define JVM_ACC_INTERFACE     0x0200  /* class is an interface */
#define JVM_ACC_ABSTRACT      0x0400  /* no definition provided */
#define JVM_ACC_STRICT          0x0800  /* strict floating point */

/* Constant pool tag types [JVMS page 103] */
enum {
    JVM_CONSTANT_Utf8 = 1,
    JVM_CONSTANT_Unicode,        /* unused */
    JVM_CONSTANT_Integer,
    JVM_CONSTANT_Float,
    JVM_CONSTANT_Long,      
    JVM_CONSTANT_Double,
    JVM_CONSTANT_Class,
    JVM_CONSTANT_String,
    JVM_CONSTANT_Fieldref,
    JVM_CONSTANT_Methodref,
    JVM_CONSTANT_InterfaceMethodref,
    JVM_CONSTANT_NameAndType
};

/* Used in the newarray instruction [JVMS page 343] */
#define JVM_T_BOOLEAN 4
#define JVM_T_CHAR    5
#define JVM_T_FLOAT   6
#define JVM_T_DOUBLE  7
#define JVM_T_BYTE    8
#define JVM_T_SHORT   9
#define JVM_T_INT    10
#define JVM_T_LONG   11

/* JVM method signatures [JVMS page 101] */
#define JVM_SIGNATURE_ARRAY        '['
#define JVM_SIGNATURE_BYTE        'B'
#define JVM_SIGNATURE_CHAR        'C'
#define JVM_SIGNATURE_CLASS        'L'
#define JVM_SIGNATURE_ENDCLASS            ';'
#define JVM_SIGNATURE_ENUM        'E'
#define JVM_SIGNATURE_FLOAT        'F'
#define JVM_SIGNATURE_DOUBLE            'D'
#define JVM_SIGNATURE_FUNC        '('
#define JVM_SIGNATURE_ENDFUNC            ')'
#define JVM_SIGNATURE_INT        'I'
#define JVM_SIGNATURE_LONG        'J'
#define JVM_SIGNATURE_SHORT        'S'
#define JVM_SIGNATURE_VOID        'V'
#define JVM_SIGNATURE_BOOLEAN            'Z'

/* JVM recognized modifiers [JVMS pages 96, 113, 115] */
#define JVM_RECOGNIZED_CLASS_MODIFIERS (JVM_ACC_PUBLIC | \
                    JVM_ACC_FINAL | \
                    JVM_ACC_SUPER | \
                    JVM_ACC_INTERFACE | \
                    JVM_ACC_ABSTRACT)    
       
#define JVM_RECOGNIZED_FIELD_MODIFIERS (JVM_ACC_PUBLIC | \
                    JVM_ACC_PRIVATE | \
                    JVM_ACC_PROTECTED | \
                    JVM_ACC_STATIC | \
                    JVM_ACC_FINAL | \
                    JVM_ACC_VOLATILE | \
                    JVM_ACC_TRANSIENT)

#define JVM_RECOGNIZED_METHOD_MODIFIERS (JVM_ACC_PUBLIC | \
                     JVM_ACC_PRIVATE | \
                     JVM_ACC_PROTECTED | \
                     JVM_ACC_STATIC | \
                     JVM_ACC_FINAL | \
                     JVM_ACC_SYNCHRONIZED | \
                     JVM_ACC_NATIVE | \
                     JVM_ACC_ABSTRACT | \
                     JVM_ACC_STRICT)

#define JVM_PRINTABLE_CLASS_MODIFIERS (JVM_ACC_PUBLIC | \
                    JVM_ACC_FINAL | \
                    JVM_ACC_INTERFACE | \
                    JVM_ACC_ABSTRACT)    

#define JVM_PRINTABLE_FIELD_MODIFIERS JVM_RECOGNIZED_FIELD_MODIFIERS

#define JVM_PRINTABLE_METHOD_MODIFIERS JVM_RECOGNIZED_METHOD_MODIFIERS
