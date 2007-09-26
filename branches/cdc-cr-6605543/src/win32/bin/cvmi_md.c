/*
 * @(#)cvmi_md.c	1.8 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

#include "javavm/include/defs.h"
#include "javavm/export/jni.h"
#include "portlibs/win32/java.h"

/* %comment d001 */
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"

static jmp_buf jmp_env;
static int in_main = 0;
static int exitCode = 0;

static void
safeExit(int status)
{
    if (in_main) {
	exitCode = status;
	longjmp(jmp_env, 1);
    } else {
	fprintf(stderr, "Unexpected VM exit(%d)\n", status);
    }
}

int
ansiJavaMain(int argc, const char** argv)
{
    JavaVM*         jvm = 0;
    JNIEnv*         env = 0;
    JavaVMInitArgs  args;
    int i;
    int j = 0;
    const int extra_args = (2
#ifdef ANSI_USE_EXIT_HOOK
                            +1
#endif
        );
    args.version = JNI_VERSION_1_2;
    args.ignoreUnrecognized = JNI_FALSE;
    /* Allocate JVM arguments and put argv in there.
       These are parsed up in Java by the CVM class.

       argv contains the incoming options to the VM, like -Xms20m, as
       well as the program name and its arguments. We're going to try
       to do as much command line parsing as possible in Java. Here we
       just parcel up argv so that the arguments we pass into
       JNI_CreateJavaVM are spec-compliant. Namely, we prepend the
       program name and its arguments by "-Xcvm". These are recognized
       by CVM.parseCommandLineOptions and used to build up the list of
       arguments for the main() method. Note that it is also valid to
       call JNI_CreateJavaVM without the special -Xcvm arguments (as a
       user embedding the VM in another application would) and do all
       command line parsing and constructing of arguments for main()
       in C.

       %comment kbr001

       %comment kbr002
    */
#define EXTRA_ARGS extra_args
#define ARGV_OFFSET 1
#define OPT_INDEX(i) ((i) + EXTRA_ARGS - ARGV_OFFSET)
    args.nOptions = EXTRA_ARGS + argc - 1;
    args.options =
	(JavaVMOption*) malloc(args.nOptions * sizeof(JavaVMOption));
  
    args.options[j].optionString = "vfprintf";
    args.options[j].extraInfo = (void *)vfprintf;
    ++j;
    args.options[j].optionString = "abort";
#ifndef WINCE
    args.options[j].extraInfo = (void *)abort;
#else
    args.options[j].extraInfo = (void *)ExitThread;
#endif
    ++j;
#ifdef ANSI_USE_EXIT_HOOK 
    args.options[j].optionString = "exit";
    args.options[j].extraInfo = (void *)exit;
    ++j;
#endif
    assert(j == EXTRA_ARGS);
    
    for (i = ARGV_OFFSET; i < argc; i++) {
	if (argv[i][0] != '-') {
	    /* Prepend "-Xcvm" to the rest of the command line options */
	    while (i < argc) {
		char* tmpStr;
		/* Make room for "-Xcvm" and NULL */
		tmpStr = malloc(sizeof(char) *
				    (strlen(argv[i]) + 6));
		strcpy(tmpStr, "-Xcvm");
		strcat(tmpStr, argv[i]);
		args.options[OPT_INDEX(i)].optionString = tmpStr;
		args.options[OPT_INDEX(i)].extraInfo = NULL;
		i++;
	    }
	    break;
	} else {
            if (strcmp(argv[i], "-XsafeExit") == 0) {
		args.options[OPT_INDEX(i)].optionString = "_safeExit";
		args.options[OPT_INDEX(i)].extraInfo = (void *)safeExit;
	    } else {
		args.options[OPT_INDEX(i)].optionString = (char*) argv[i];
		args.options[OPT_INDEX(i)].extraInfo = NULL;
	    }
	}
    }

    in_main = 0;
    exitCode = 0;

    if (JNI_CreateJavaVM(&jvm, (void **)&env, &args) == JNI_OK &&
	args.version == JNI_VERSION_1_2)
    {
	jclass			cvmClass;
	jmethodID		getParseStatusID;
	jmethodID		getMainClassNameID;
	jmethodID		getMainArgumentsID;
	jobject			mainClassName;
	const char*		mainClassChars;
	jobject			mainArgs;
	volatile jclass		mainClass = NULL;
	volatile jmethodID	mainMethodID;
	jint            	parseStatus;

	/*
	 * First of all, free the memory that we allocated. The options
	 * have already been parsed and copied by JNI_CreateJavaVM
	 */
	for (i = ARGV_OFFSET; i < argc; i++) {
	    if (argv[i][0] != '-') {
		/* Prepend "-Xcvm" to the rest of the command line options */
		while (i < argc) {
		    CVMassert(args.options[OPT_INDEX(i)].optionString !=
			      NULL);
		    CVMassert(args.options[OPT_INDEX(i)].extraInfo == NULL);

		    free(args.options[OPT_INDEX(i)].optionString);
		    args.options[OPT_INDEX(i)].optionString = NULL;
		    i++;
		}
		break;
	    }
	}
	free(args.options);
	args.options = NULL; /* This has all been processed by
				JNI_CreateJavaVM */
	cvmClass = CVMcbJavaInstance(CVMsystemClass(sun_misc_CVM));
	getParseStatusID =
	    (*env)->GetStaticMethodID(env, cvmClass,
				      "getParseStatus",
				      "()I");
	getMainClassNameID =
	    (*env)->GetStaticMethodID(env, cvmClass,
				      "getMainClassName",
				      "()Ljava/lang/String;");
	getMainArgumentsID =
	    (*env)->GetStaticMethodID(env, cvmClass,
				      "getMainArguments",
				      "()[Ljava/lang/String;");

	parseStatus =
	    (*env)->CallStaticIntMethod(env, cvmClass, getParseStatusID);
	if ((*env)->ExceptionCheck(env)) {
	    goto uncaughtException;
	}
	if (parseStatus == sun_misc_CVM_ARG_PARSE_EXITVM) {
	    goto exitingVM;
	} else {
	    /* The error case must have been filtered out */
	    CVMassert(parseStatus == sun_misc_CVM_ARG_PARSE_OK);
	}
	mainClassName = (*env)->CallStaticObjectMethod(env, cvmClass,
						       getMainClassNameID);
	if ((*env)->ExceptionCheck(env) || (mainClassName == NULL)) {
	    goto uncaughtException;
	}
	mainArgs = (*env)->CallStaticObjectMethod(env, cvmClass,
						  getMainArgumentsID);
	if (mainArgs == NULL) {
	    goto uncaughtException;
	}
	
	mainClassChars = (*env)->GetStringUTFChars(env, mainClassName, NULL);
	if ((*env)->ExceptionCheck(env) || (mainClassChars == NULL)) {
	    (*env)->DeleteLocalRef(env, mainClassName);
	    goto uncaughtException;
	}
	
	(*env)->DeleteLocalRef(env, mainClassName);
	mainClass = (*env)->FindClass(env, mainClassChars);
	(*env)->ReleaseStringUTFChars(env, mainClassName, mainClassChars);

	if (mainClass == NULL) { /* exception already thrown */
	    goto uncaughtException;
	}

	mainMethodID =
	    (*env)->GetStaticMethodID(env, mainClass,
				      "main", "([Ljava/lang/String;)V");
	if (mainMethodID == NULL) {
	    goto uncaughtException;
	}

	/*
	 * The vm spec says that the main method must be public, although
	 * the JDK does not check for this.
	 */
	if (!CVMmbIs(mainMethodID, PUBLIC)) {
	    CVMthrowIllegalAccessException(CVMjniEnv2ExecEnv(env),
					   "%M is not public", mainMethodID);
	    goto uncaughtException;
	}

	if (setjmp(jmp_env) == 0) {
	    in_main = 1;
	    (*env)->CallStaticVoidMethod(env, mainClass, mainMethodID,
					 mainArgs);
	} else {
	    return exitCode;
	}

    uncaughtException:
	/* Print stack traces of exceptions which propagate out of
	   main(). Should we call ThreadGroup.uncaughtException()
	   here? JDK1.2 doesn't. */
	{
	    if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
                exitCode = 1;
	    }
	}

	(*env)->DeleteLocalRef(env, mainClass);

    exitingVM:
	if ((*jvm)->DetachCurrentThread(jvm) != 0) {
	    fprintf(stderr, "Could not detach main thread.\n");
            exitCode = 1;
	}

	(*jvm)->DestroyJavaVM(jvm);
    } else {
	fprintf(stderr, "Could not create JVM.\n");
        exitCode = 1;
    }

    return exitCode;
}
