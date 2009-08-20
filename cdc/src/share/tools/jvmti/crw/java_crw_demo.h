/*
 * @(#)java_crw_demo.h	1.17 05/11/17
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

#ifndef JAVA_CRW_DEMO_H
#define JAVA_CRW_DEMO_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

    /*
     * CVM defines these in verify_impl.h but they are not the same as the
     * ones used by cldc stack maps
     */
enum {
    CRW_ITEM_Object = 7,		/* Extra info field gives name. */
    CRW_ITEM_NewObject = 8		/* Like object, but uninitialized. */
};

/* This callback is used to notify the caller of a fatal error. */

typedef void (*FatalErrorHandler)(const char*message, const char*file, int line);

/* This callback is used to return the method information for a class.
 *   Since the information was already read here, it was useful to
 *   return it here, with no JVMTI phase restrictions.
 *   If the class file does represent a "class" and it has methods, then
 *   this callback will be called with the class number and pointers to
 *   the array of names, array of signatures, and the count of methods.
 */

typedef void (*MethodNumberRegister)(unsigned, const char**, const char**, int);

/* Class file reader/writer interface. Basic input is a classfile image
 *     and details about what to inject. The output is a new classfile image
 *     that was allocated with malloc(), and should be freed by the caller.
 */

/* Names of external symbols to look for. These are the names that we
 *   try and lookup in the shared library. On Windows 2000, the naming
 *   convention is to prefix a "_" and suffix a "@N" where N is 4 times
 *   the number or arguments supplied.It has 19 args, so 76 = 19*4.
 *   On Windows 2003, Linux, and Solaris, the first name will be
 *   found, on Windows 2000 a second try should find the second name.
 *
 *   WARNING: If You change the JavaCrwDemo typedef, you MUST change
 *            multiple things in this file, including this name.
 */

#define JAVA_CRW_DEMO_SYMBOLS { "java_crw_demo", "_java_crw_demo@76" }

/* Typedef needed for type casting in dynamic access situations. */

typedef void (JNICALL *JavaCrwDemo)(
	 unsigned class_number,
	 const char *name,
	 const unsigned char *file_image,
	 long file_len,
	 int system_class,
	 char* tclass_name,
	 char* tclass_sig,
	 char* call_name,
	 char* call_sig,
	 char* return_name,
	 char* return_sig,
	 char* obj_init_name,
	 char* obj_init_sig,
	 char* newarray_name,
	 char* newarray_sig,
	 unsigned char **pnew_file_image,
	 long *pnew_file_len,
	 FatalErrorHandler fatal_error_handler,
	 MethodNumberRegister mnum_callback
);

/* Function export (should match typedef above) */

JNIEXPORT void JNICALL java_crw_demo(
	 
	 unsigned class_number, /* Caller assigned class number for class */
	 
	 const char *name,      /* Internal class name, e.g. java/lang/Object */
				/*   (Do not use "java.lang.Object" format) */
	 
	 const unsigned char 
	   *file_image,         /* Pointer to classfile image for this class */
	 
	 long file_len, 	/* Length of the classfile in bytes */
	 
	 int system_class,      /* Set to 1 if this is a system class */
				/*   (prevents injections into empty */
				/*   <clinit>, finalize, and <init> methods) */
         
	 char* tclass_name,	/* Class that has methods we will call at */
				/*   the injection sites (tclass) */
         
	 char* tclass_sig,	/* Signature of tclass */
				/*  (Must be "L" + tclass_name + ";") */
         
	 char* call_name,	/* Method name in tclass to call at offset 0 */
				/*   for every method */
         
	 char* call_sig,	/* Signature of this call_name method */
				/*  (Must be "(II)V") */
         
	 char* return_name,	/* Method name in tclass to call at all */
				/*  return opcodes in every method */
         
	 char* return_sig,	/* Signature of this return_name method */
				/*  (Must be "(II)V") */
         
	 char* obj_init_name,	/* Method name in tclass to call first thing */
				/*   when injecting java.lang.Object.<init> */
         
	 char* obj_init_sig,	/* Signature of this obj_init_name method */
				/*  (Must be "(Ljava/lang/Object;)V") */
         
	 char* newarray_name,	/* Method name in tclass to call after every */
				/*   newarray opcode in every method */
         
	 char* newarray_sig,	/* Signature of this method */
				/*  (Must be "(Ljava/lang/Object;II)V") */
	 
	 unsigned char 
	   **pnew_file_image,   /* Returns a pointer to new classfile image */
	 
	 long *pnew_file_len,   /* Returns the length of the new image */
	 
	 FatalErrorHandler 
	   fatal_error_handler, /* Pointer to function to call on any */
			        /*  fatal error. NULL sends error to stderr */
	 
	 MethodNumberRegister 
	   mnum_callback        /* Pointer to function that gets called */
				/*   with all details on methods in this */
				/*   class. NULL means skip this call. */
	   
	   );


/* External to read the class name out of a class file .
 *
 *   WARNING: If You change the typedef, you MUST change
 *            multiple things in this file, including this name.
 */

#define JAVA_CRW_DEMO_CLASSNAME_SYMBOLS \
	 { "java_crw_demo_classname", "_java_crw_demo_classname@12" }

/* Typedef needed for type casting in dynamic access situations. */

typedef char * (JNICALL *JavaCrwDemoClassname)(
	 const unsigned char *file_image, 
	 long file_len, 
	 FatalErrorHandler fatal_error_handler);

JNIEXPORT char * JNICALL java_crw_demo_classname(
         const unsigned char *file_image, 
	 long file_len, 
	 FatalErrorHandler fatal_error_handler);

/* Opcode length initializer, use with something like:
 *   unsigned char opcode_length[JVM_OPC_MAX+1] = JVM_OPCODE_LENGTH_INITIALIZER;
 */
#define JVM_OPCODE_LENGTH_INITIALIZER { \
   1,   /* nop */                       \
   1,   /* aconst_null */               \
   1,   /* iconst_m1 */                 \
   1,   /* iconst_0 */                  \
   1,   /* iconst_1 */                  \
   1,   /* iconst_2 */                  \
   1,   /* iconst_3 */                  \
   1,   /* iconst_4 */                  \
   1,   /* iconst_5 */                  \
   1,   /* lconst_0 */                  \
   1,   /* lconst_1 */                  \
   1,   /* fconst_0 */                  \
   1,   /* fconst_1 */                  \
   1,   /* fconst_2 */                  \
   1,   /* dconst_0 */                  \
   1,   /* dconst_1 */                  \
   2,   /* bipush */                    \
   3,   /* sipush */                    \
   2,   /* ldc */                       \
   3,   /* ldc_w */                     \
   3,   /* ldc2_w */                    \
   2,   /* iload */                     \
   2,   /* lload */                     \
   2,   /* fload */                     \
   2,   /* dload */                     \
   2,   /* aload */                     \
   1,   /* iload_0 */                   \
   1,   /* iload_1 */                   \
   1,   /* iload_2 */                   \
   1,   /* iload_3 */                   \
   1,   /* lload_0 */                   \
   1,   /* lload_1 */                   \
   1,   /* lload_2 */                   \
   1,   /* lload_3 */                   \
   1,   /* fload_0 */                   \
   1,   /* fload_1 */                   \
   1,   /* fload_2 */                   \
   1,   /* fload_3 */                   \
   1,   /* dload_0 */                   \
   1,   /* dload_1 */                   \
   1,   /* dload_2 */                   \
   1,   /* dload_3 */                   \
   1,   /* aload_0 */                   \
   1,   /* aload_1 */                   \
   1,   /* aload_2 */                   \
   1,   /* aload_3 */                   \
   1,   /* iaload */                    \
   1,   /* laload */                    \
   1,   /* faload */                    \
   1,   /* daload */                    \
   1,   /* aaload */                    \
   1,   /* baload */                    \
   1,   /* caload */                    \
   1,   /* saload */                    \
   2,   /* istore */                    \
   2,   /* lstore */                    \
   2,   /* fstore */                    \
   2,   /* dstore */                    \
   2,   /* astore */                    \
   1,   /* istore_0 */                  \
   1,   /* istore_1 */                  \
   1,   /* istore_2 */                  \
   1,   /* istore_3 */                  \
   1,   /* lstore_0 */                  \
   1,   /* lstore_1 */                  \
   1,   /* lstore_2 */                  \
   1,   /* lstore_3 */                  \
   1,   /* fstore_0 */                  \
   1,   /* fstore_1 */                  \
   1,   /* fstore_2 */                  \
   1,   /* fstore_3 */                  \
   1,   /* dstore_0 */                  \
   1,   /* dstore_1 */                  \
   1,   /* dstore_2 */                  \
   1,   /* dstore_3 */                  \
   1,   /* astore_0 */                  \
   1,   /* astore_1 */                  \
   1,   /* astore_2 */                  \
   1,   /* astore_3 */                  \
   1,   /* iastore */                   \
   1,   /* lastore */                   \
   1,   /* fastore */                   \
   1,   /* dastore */                   \
   1,   /* aastore */                   \
   1,   /* bastore */                   \
   1,   /* castore */                   \
   1,   /* sastore */                   \
   1,   /* pop */                       \
   1,   /* pop2 */                      \
   1,   /* dup */                       \
   1,   /* dup_x1 */                    \
   1,   /* dup_x2 */                    \
   1,   /* dup2 */                      \
   1,   /* dup2_x1 */                   \
   1,   /* dup2_x2 */                   \
   1,   /* swap */                      \
   1,   /* iadd */                      \
   1,   /* ladd */                      \
   1,   /* fadd */                      \
   1,   /* dadd */                      \
   1,   /* isub */                      \
   1,   /* lsub */                      \
   1,   /* fsub */                      \
   1,   /* dsub */                      \
   1,   /* imul */                      \
   1,   /* lmul */                      \
   1,   /* fmul */                      \
   1,   /* dmul */                      \
   1,   /* idiv */                      \
   1,   /* ldiv */                      \
   1,   /* fdiv */                      \
   1,   /* ddiv */                      \
   1,   /* irem */                      \
   1,   /* lrem */                      \
   1,   /* frem */                      \
   1,   /* drem */                      \
   1,   /* ineg */                      \
   1,   /* lneg */                      \
   1,   /* fneg */                      \
   1,   /* dneg */                      \
   1,   /* ishl */                      \
   1,   /* lshl */                      \
   1,   /* ishr */                      \
   1,   /* lshr */                      \
   1,   /* iushr */                     \
   1,   /* lushr */                     \
   1,   /* iand */                      \
   1,   /* land */                      \
   1,   /* ior */                       \
   1,   /* lor */                       \
   1,   /* ixor */                      \
   1,   /* lxor */                      \
   3,   /* iinc */                      \
   1,   /* i2l */                       \
   1,   /* i2f */                       \
   1,   /* i2d */                       \
   1,   /* l2i */                       \
   1,   /* l2f */                       \
   1,   /* l2d */                       \
   1,   /* f2i */                       \
   1,   /* f2l */                       \
   1,   /* f2d */                       \
   1,   /* d2i */                       \
   1,   /* d2l */                       \
   1,   /* d2f */                       \
   1,   /* i2b */                       \
   1,   /* i2c */                       \
   1,   /* i2s */                       \
   1,   /* lcmp */                      \
   1,   /* fcmpl */                     \
   1,   /* fcmpg */                     \
   1,   /* dcmpl */                     \
   1,   /* dcmpg */                     \
   3,   /* ifeq */                      \
   3,   /* ifne */                      \
   3,   /* iflt */                      \
   3,   /* ifge */                      \
   3,   /* ifgt */                      \
   3,   /* ifle */                      \
   3,   /* if_icmpeq */                 \
   3,   /* if_icmpne */                 \
   3,   /* if_icmplt */                 \
   3,   /* if_icmpge */                 \
   3,   /* if_icmpgt */                 \
   3,   /* if_icmple */                 \
   3,   /* if_acmpeq */                 \
   3,   /* if_acmpne */                 \
   3,   /* goto */                      \
   3,   /* jsr */                       \
   2,   /* ret */                       \
   99,  /* tableswitch */               \
   99,  /* lookupswitch */              \
   1,   /* ireturn */                   \
   1,   /* lreturn */                   \
   1,   /* freturn */                   \
   1,   /* dreturn */                   \
   1,   /* areturn */                   \
   1,   /* return */                    \
   3,   /* getstatic */                 \
   3,   /* putstatic */                 \
   3,   /* getfield */                  \
   3,   /* putfield */                  \
   3,   /* invokevirtual */             \
   3,   /* invokespecial */             \
   3,   /* invokestatic */              \
   5,   /* invokeinterface */           \
   0,   /* xxxunusedxxx */              \
   3,   /* new */                       \
   2,   /* newarray */                  \
   3,   /* anewarray */                 \
   1,   /* arraylength */               \
   1,   /* athrow */                    \
   3,   /* checkcast */                 \
   3,   /* instanceof */                \
   1,   /* monitorenter */              \
   1,   /* monitorexit */               \
   0,   /* wide */                      \
   4,   /* multianewarray */            \
   3,   /* ifnull */                    \
   3,   /* ifnonnull */                 \
   5,   /* goto_w */                    \
   5    /* jsr_w */                     \
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif

