/*
 * @(#)ansi_java_md.c	1.87 06/10/10
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

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/setjmp.h"
#include "javavm/include/porting/ansi/assert.h"
#include "javavm/include/porting/ansi/string.h"

#include "javavm/include/porting/ansi/ctype.h"

#include "javavm/export/jni.h"
#include "portlibs/ansi_c/java.h"

#ifdef CVM_MTASK
#include "portlibs/posix/mtask.h"
static CVMParsedSubOptions serverOpts;
static int isServer = 0;
#endif

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

const int extra_args = (0
#ifdef ANSI_USE_VFPRINTF_HOOK
			+1
#endif
#ifdef ANSI_USE_ABORT_HOOK
			+1
#endif
#ifdef ANSI_USE_EXIT_HOOK
			+1
#endif
);
#define EXTRA_ARGS extra_args
#define OPT_INDEX(i) ((i) + EXTRA_ARGS)

JavaVMInitArgs*
ANSIargvToJinitArgs(int argc, char** argv,
		    char** appclasspathStr,
		    char** bootclasspathStr)
{
    int i, j, k;
    JavaVMInitArgs* args;
    args = (JavaVMInitArgs*)calloc(1, sizeof(JavaVMInitArgs));
    if (args == NULL) {
	return NULL;
    }
    args->version = JNI_VERSION_1_2;
    args->ignoreUnrecognized = JNI_FALSE;
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
    /* The following is a conservative estimate. We might end up
       allocating more than we need if we encounter -jar, -cp or
       -classpath. */
    args->nOptions = EXTRA_ARGS + argc;
    if (args->nOptions == 0) {
	/* Nothing more to do. Do not bother to malloc(0) */
	args->options = NULL;
	return args;
    }
    args->options =
	(JavaVMOption*) malloc(args->nOptions * sizeof(JavaVMOption));
    if (args->options == NULL) {
	free(args);
	return NULL;
    }
  
    j = 0;
#ifdef ANSI_USE_VFPRINTF_HOOK
    args->options[j].optionString = "vfprintf";
    args->options[j].extraInfo = (void *)vfprintf;
    ++j;
#endif
#ifdef ANSI_USE_ABORT_HOOK
    args->options[j].optionString = "abort";
    args->options[j].extraInfo = (void *)abort;
    ++j;
#endif
#ifdef ANSI_USE_EXIT_HOOK 
    args->options[j].optionString = "exit";
    args->options[j].extraInfo = (void *)exit;
    ++j;
#endif
    (void)j; /* avoid compiler warning if NDEBUG is #defined */
    assert(j == EXTRA_ARGS);
    
    /*
     * Iterate over arguments. Leave the '-' arguments to be parsed
     * later, except for -cp, -classpath, and -jar.
     * Prepend -Xcvm to all the non-'-' arguments.
     */

    /*
     * k tracks argv
     * i tracks where we are in the options array
     */
    for (k = 0, i = 0; k < argc; i++, k++) {
	if (argv[k][0] != '-') {
   	   char* tmpStr;
           if (k > 0) {
	      /* Prepend "-Xjar" if this is following the -jar command */
              if ((strcmp(argv[k-1], "-jar")) == 0) {
                 tmpStr = malloc(sizeof(char) * (strlen(argv[k]) + 7));
                 strcpy(tmpStr, "-Xjar=");
                 strcat(tmpStr, argv[k]);                 
		 /* Extract app classpath for special casing */
		 if (appclasspathStr != NULL) {
		     char* str = (char*)argv[k];
		     *appclasspathStr = str;
		 }
		 /* Overwrite the lone -jar argument */
		 i--;
		 args->nOptions--;
		 
	         args->options[OPT_INDEX(i)].optionString = tmpStr;
	         args->options[OPT_INDEX(i)].extraInfo = tmpStr;
		 /* everything after this is args to main */
		 i++;
		 k++;
		 goto main_args_loop;
	      /* Prepend "-Xcp" if this is following the -cp or -classpath command */
              } else if (((strcmp(argv[k-1], "-cp")) == 0) || 
                         ((strcmp(argv[k-1], "-classpath")) == 0)) {
		 tmpStr = malloc(sizeof(char) * (strlen(argv[k]) + 6));
		 if (tmpStr == NULL) {
		     return NULL;
		 }
                 strcpy(tmpStr, "-Xcp=");
                 strcat(tmpStr, argv[k]);                 
		 /* Extract java.class.path for special casing */
		 if (appclasspathStr != NULL) {
		     char* str = (char*)argv[k];
		     *appclasspathStr = str;
		 }
		 /* Overwrite the lone -cp or -classpath */
		 i--;
		 args->nOptions--;
		 
	         args->options[OPT_INDEX(i)].optionString = tmpStr;
	         args->options[OPT_INDEX(i)].extraInfo = tmpStr;
                 continue;  /* need to go back to the forloop */
              }
           }

	   /* this is the main class and its args */
	   goto main_args_loop;

	   break;
	} else {
            if (strcmp(argv[k], "-XsafeExit") == 0) {
		args->options[OPT_INDEX(i)].optionString = "_safeExit";
		args->options[OPT_INDEX(i)].extraInfo = (void *)safeExit;
	    } else {
		/* This could be -cp, -classpath, or -jar. The
		   arguments for these will be parsed and remapped in
		   the next iteration. */
		args->options[OPT_INDEX(i)].optionString = (char*) argv[k];
		args->options[OPT_INDEX(i)].extraInfo = NULL;
	    }
	    /* The following is the test for a malloc'ed optionString,
	       so don't let it succeed for these cases */
	    assert(args->options[OPT_INDEX(i)].optionString !=
		   args->options[OPT_INDEX(i)].extraInfo);
#ifdef CVM_MTASK
	    if (strncmp(argv[k], "-Xserver", 8) == 0) {
		char* str = argv[k];
		isServer = CVM_TRUE;
		if (str[8] == ':') {
		    CVMinitParsedSubOptions(&serverOpts, str + 9);
		}
	    }
#endif
	    /* Extract java.class.path for special casing */
            if (appclasspathStr != NULL) {
		char* str = (char*)argv[k];
		if (strncmp(str, "-Djava.class.path=", 18) == 0) {
		    *appclasspathStr = str + 18;
		} else if (!strncmp(str, "-Xcp=", 5)) {
		    *appclasspathStr = str + 5;
		} else if (!strncmp(str, "-Xjar=", 6)) {
		    *appclasspathStr = str + 6;
		}
	    }
	    /* Extract boot classpath settings for special casing */
            if (bootclasspathStr != NULL) {
		char* str = (char*)argv[k];
		if (!strncmp(str, "-Xbootclasspath/a:", 18) ||
                    !strncmp(str, "-Xbootclasspath/a=", 18)) {
		    *bootclasspathStr = str + 18;
		} else if (!strncmp(str, "-Xbootclasspath:", 16) ||
                           !strncmp(str, "-Xbootclasspath=", 16)) {
		    *bootclasspathStr = str + 16;
		}
	    }
	}
    }

main_args_loop:
   /* Prepend "-Xcvm" to the rest of the command line options */
   /* They are all to be passed into main */
   while (k < argc) {
	/* Make room for "-Xcvm" and NULL */
	char* tmpStr;
	tmpStr = malloc(sizeof(char) * (strlen(argv[k]) + 6)); 
	if (tmpStr == NULL) {
	    return NULL;
	}
	strcpy(tmpStr, "-Xcvm");
	strcat(tmpStr, argv[k]);
	args->options[OPT_INDEX(i)].optionString = tmpStr;
	args->options[OPT_INDEX(i)].extraInfo = tmpStr;
	i++;
	k++;
   }
    return args;
}

void
ANSIfreeJinitArgs(JavaVMInitArgs* args)
{
    int i;
    
    for (i = 0; i < args->nOptions; i++) {
	char* optionString = args->options[OPT_INDEX(i)].optionString;
	char* extraInfo = args->options[OPT_INDEX(i)].extraInfo;
	/* We marked malloc'ed optionString's by setting extraInfo to be
	   the same as the malloc data. */
	if ((optionString == extraInfo) && (optionString != NULL)) {
	    assert(!strncmp(optionString, "-Xjar=", 6) ||
		   !strncmp(optionString, "-Xcp=", 5) ||
		   !strncmp(optionString, "-Xcvm", 5));
	    free(optionString);
	    args->options[OPT_INDEX(i)].optionString = NULL;
	    args->options[OPT_INDEX(i)].extraInfo = NULL;
	}
    }
    
    free(args->options);
    args->options = NULL; /* This has all been processed by
			    JNI_CreateJavaVM */
    free(args);
}

int
ansiJavaMain(int argc, const char** argv,
    JNI_CreateJavaVM_func *JNI_CreateJavaVMFunc)
{
    JavaVM*         jvm;
    JNIEnv*         env;
    JavaVMInitArgs* args;
    jint result;
    jint jvmVersion;
    jclass cvmClass;
    const char **argsArray = &argv[1];
    int argCount = argc - 1;
#ifdef CVM_MTASK
    ServerState state;
#endif

    in_main = 0;
    exitCode = 0;
    
#ifdef CVM_MTASK
    memset(&state, 0, sizeof(ServerState));
    memset(&serverOpts, 0, sizeof(CVMParsedSubOptions));
#endif

    /* Convert the arguments into a JavaVMInitArgs structure */
    args = ANSIargvToJinitArgs(argCount, (char**)argsArray, NULL, NULL);

    if (args == NULL) {
	fprintf(stderr, "Out of Memory.\n");
	return 1;
    }

    {
	void*   envV;
	result = (*JNI_CreateJavaVMFunc)(&jvm, &envV, args);
	env = envV;
    }

    /*
     * First of all, free the memory that we allocated. The options
     * have already been parsed and copied by JNI_CreateJavaVM
     */
    jvmVersion = args->version;
    ANSIfreeJinitArgs(args);
    args = NULL; /* Make sure no one uses this anymore */
    
    /* Now check to see if we can proceed with the main program */
    if (result != JNI_OK) {
	fprintf(stderr, "Could not create JVM.\n");
#ifdef CVM_MTASK
	CVMdestroyParsedSubOptions(&serverOpts);
#endif
	return 1;
    }
    if (jvmVersion != JNI_VERSION_1_2) {
	exitCode = 2;
	fprintf(stderr, "JVM does not support JNI_VERSION_1_2.\n");
#ifdef CVM_MTASK
	CVMdestroyParsedSubOptions(&serverOpts);
#endif
	goto exitingVM;
    }

    {
	jmethodID		runMainID;
	cvmClass = (*env)->FindClass(env, "sun/misc/CVM");
	if (cvmClass == NULL) {
	    goto uncaughtException;
	}
	runMainID =
	    (*env)->GetStaticMethodID(env, cvmClass,
				      "runMain",
				      "()V");
	if (runMainID == NULL) {
	    goto uncaughtException;
	}

#ifdef CVM_MTASK
	if (isServer) {
	    if (!MTASKserverInitialize(&state, &serverOpts, env, cvmClass)) {
		goto uncaughtException;
	    }
	}
#endif


	/* Now we have created the JVM, and we ready to execute.
	   We are either going to wait for requests as a server,
	   or just execute the main program right away */

#ifdef CVM_MTASK
nextRequest:

	if (isServer) {
	    if (!MTASKnextRequest(&state)) {
		goto checkServerException;
	    }
	}
#endif
	    
	if (setjmp(jmp_env) == 0) {
	    in_main = 1;
	    (*env)->CallStaticVoidMethod(env, cvmClass, runMainID);
	} else {
	    return exitCode;
	}
    }

#ifdef CVM_MTASK
checkServerException:

    /* In the server mode, it's possible to execute a command in the
       body of the server instead of a forked client.
    */
    if (isServer && state.wasSourceCommand) {
	if ((*env)->ExceptionCheck(env)) {
	    (*env)->ExceptionDescribe(env);
	    (*env)->ExceptionClear(env);
	}
	MTASKserverSourceReset(&state);
	/* And go back to the server */
	goto nextRequest;
    }
#endif
    
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

    if (cvmClass != NULL) {
	(*env)->DeleteLocalRef(env, cvmClass);
    }

exitingVM:
    if ((*jvm)->DetachCurrentThread(jvm) != 0) {
	fprintf(stderr, "Could not detach main thread.\n");
	exitCode = 1;
    }

    (*jvm)->DestroyJavaVM(jvm);

    return exitCode;
}
