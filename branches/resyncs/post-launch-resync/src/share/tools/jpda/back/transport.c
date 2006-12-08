/*
 * @(#)transport.c	1.34 06/10/10
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
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "transport.h"
#include "util.h"
#include "debugLoop.h"
#include "debugInit.h"
#include "sys.h"
#include "JDWP.h"

static struct Transport *transport;
static const char *handshakeString = "JDWP-Handshake";
static JVMDI_RawMonitor listenerLock;
static JVMDI_RawMonitor sendLock;
/* The following pieces of memory are allocated using jdwpAlloc and
   should be freed using jdwpFree. See the documentation for
   parsePath, below. */
static char **bootLibraryPaths = NULL;
static int numBootLibraryPaths = 0;
static char **javaLibraryPaths = NULL;
static int numJavaLibraryPaths = 0;
static char *fileSeparator = NULL;
static int fileSeparatorLength = 0;

/*                  
 * data structure used for passing transport info from thread to thread
 */
typedef struct TransportInfo {
    char *name;
    struct Transport *transport;
    void *handle;
    char *address;
} TransportInfo;

#ifdef JDWP_ALLOC_TRACE
static struct TransportCallback callback = {jdwpAllocReal, jdwpFreeReal};
#else
static struct TransportCallback callback = {jdwpAlloc, jdwpFree};
#endif

/* Get a property from java.lang.System with the given name (assumed
   UTF-8 encoded) and return the associated string, or NULL if the
   property is not defined (or if an error occurred) */
static jstring
getSystemProperty(JNIEnv *env,
		  const char *propertyName)
{
    /* This routine is called only during system initialization and
       only twice. For this reason we don't worry about "caching"
       methodIDs and the like. */
    jclass systemClass;
    jmethodID getPropertyID;
    jobject retval;
    jstring propertyString;
    
    systemClass = (*env)->FindClass(env, "java/lang/System");
    if (systemClass == NULL) {
	return NULL;
    }
    getPropertyID = (*env)->GetStaticMethodID(env, systemClass,
					      "getProperty",
					      "(Ljava/lang/String;)Ljava/lang/String;");
    if (getPropertyID == NULL) {
	return NULL;
    }
    propertyString = (*env)->NewStringUTF(env, propertyName);
    if (propertyString == NULL) {
	return NULL;
    }
    retval = (*env)->CallStaticObjectMethod(env, systemClass,
					    getPropertyID, propertyString);
    return (jstring) retval;
}

/* The storage returned from this routine (the char ** and the char *'s
   it contains) is all allocated using jdwpAlloc and should be freed
   when done using jdwpFree. */
static char **
parsePath(JNIEnv *env, jstring jpath, int *numEntries)
{
    /* Get the path separator system property */
    jstring jpathSeparator;
    const char *path;
    const char *p;
    const char *np;
    const char *pathSeparator;
    int num = 0;
    int pathSeparatorLength;
    char **pathEntries;
    int i;

    if (jpath == NULL) {
	*numEntries = 0;
	return NULL;
    }

    jpathSeparator = getSystemProperty(env, "path.separator");
    if (jpathSeparator == NULL) {
	*numEntries = 0;
	return NULL;
    }

    path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (path == NULL) {
	*numEntries = 0;
	return NULL;
    }

    pathSeparator = (*env)->GetStringUTFChars(env, jpathSeparator, NULL);
    if (path == NULL) {
	(*env)->ReleaseStringUTFChars(env, jpath, path);
	*numEntries = 0;
	return NULL;
    }

    pathSeparatorLength = strlen(pathSeparator);

    /* Now count how many instances of the path separator are in the
       path */
    p = path;
    while ((p != NULL) && (*p != '\0')) {
	p = strstr(p, pathSeparator);
	if (p != NULL) {
	    num++;
	    p += pathSeparatorLength;
	}
    }
    /* Allocate enough space for all of the path entries */
    pathEntries = jdwpAlloc((num + 1) * sizeof(char *));
    /* Chop path into component parts */
    p = path;
    /* Loop invariant: p points to start of current path entry */
    /* Note that this loop handles all except the last entry. */
    for (i = 0; i < num; i++) {
	char *entry;
	int entryLen;
	np = strstr(p, pathSeparator);
	if (np == NULL) {
	    /* Should not happen */
	    (*env)->ReleaseStringUTFChars(env, jpath, path);
	    (*env)->ReleaseStringUTFChars(env, jpathSeparator, pathSeparator);
	    *numEntries = 0;
	    return NULL;
	}
	entryLen = np - p;
	if (entryLen == 0) {
	    /* As in the Java code, an empty entry will be considered
               to indicate the current directory */
	    entry = (char *) jdwpAlloc(2 * sizeof(char));
	    entry[0] = '.';
	    entry[1] = 0;
	} else {
	    /* Leave room for NULL terminator */
	    entry = (char *) jdwpAlloc(1 + entryLen);
	    strncpy(entry, p, entryLen);
	    entry[entryLen] = 0;
	}
	pathEntries[i] = entry;
	p = np + pathSeparatorLength;
    }
    /* Now get the last entry */
    i = strlen(p);
    if (i == 0) {
	char *entry;
	/* As in the Java code, an empty entry will be considered
	   to indicate the current directory */
	entry = (char *) jdwpAlloc(2 * sizeof(char));
	entry[0] = '.';
	entry[1] = 0;
	pathEntries[num] = entry;
    } else {
	char *entry;
	/* Leave room for NULL terminator */
	entry = (char *) jdwpAlloc(1 + i);
	strncpy(entry, p, i);
	entry[i] = 0;
	pathEntries[num] = entry;
    }
    *numEntries = num + 1;
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    (*env)->ReleaseStringUTFChars(env, jpathSeparator, pathSeparator);
    return pathEntries;
}

/*
 * This routine is called when the initial event is generated and we
 * have a valid JNI environment in which to make calls back into the
 * interpreter. It gets the system properties sun.boot.library.path
 * and java.library.path and parses them into their components
 * according to the system property path.separator. Note that this is
 * very similar to the code in the 1.2 version of ClassLoader.java,
 * except that this is written in C using the JNI and is not as
 * "Unicode-correct" since it assumes a UTF-8 platform encoding.
 */
static void
parseLibraryPaths(JNIEnv *env)
{
    /* Since this is called once, we're not going to worry about
       "caching" methodIDs and related objects. */
    jstring jtmpString;

    bootLibraryPaths = parsePath(env,
				 getSystemProperty(env,
						   "sun.boot.library.path"),
				 &numBootLibraryPaths);
    javaLibraryPaths = parsePath(env,
				 getSystemProperty(env,
						   "java.library.path"),
				 &numJavaLibraryPaths);
    /* Also get the file.separator property for later */
    jtmpString = getSystemProperty(env, "file.separator");
    /* (jtmpString == NULL) should not happen */
    if (jtmpString != NULL) {
	const char *stringChars =
	    (*env)->GetStringUTFChars(env, jtmpString, NULL);
	if (stringChars == NULL) {
	    return;
	}
	fileSeparatorLength = strlen(stringChars);
	fileSeparator = jdwpAlloc((fileSeparatorLength + 1) * sizeof(char));
	strcpy(fileSeparator, stringChars);
	(*env)->ReleaseStringUTFChars(env, jtmpString, stringChars);
    }
}

/* Helper routine. Returns 1 if string ends with given suffix, 0 if
   not. */
static int
stringEndsWith(const char *str, const char *suffix)
{
    int len;
    int suffixLen;

    if ((str == NULL) || (suffix == NULL)) {
	return 0;
    }
    
    len = strlen(str);
    suffixLen = strlen(suffix);
    if (suffixLen > len) {
	return 0;
    }
    if (strcmp(str + len - suffixLen, suffix) == 0) {
	return 1;
    }
    return 0;
}

static void *
searchPathsForTransport(char *name, char **paths, int numPaths)
{
    int entryCnt;
    char errmsg[MAXPATHLEN + 100];
    void *handle;

    for (entryCnt = 0; entryCnt < numPaths; entryCnt++) {
	char *entry = paths[entryCnt];
	char *fullPathName;
	int entryLen;
	int nameLen;
	/* If the path entry ends with the file separator, then we
           just concatenate the path entry with the library name;
           otherwise, we have to insert the file separator in
           between. */
	entryLen = strlen(entry);
	nameLen = strlen(name);
	if (stringEndsWith(entry, fileSeparator)) {
	    fullPathName = jdwpAlloc(entryLen + nameLen + 1);
	    strcpy(fullPathName, entry);
	    strcat(fullPathName, name);
	} else {
	    fullPathName = jdwpAlloc(entryLen + fileSeparatorLength +
				     nameLen + 1);
	    strcpy(fullPathName, entry);
	    strcat(fullPathName, fileSeparator);
	    strcat(fullPathName, name);
	}
	/* NOTE that dbgsysLoadLibrary is now required to fail
           "cleanly" if the module was not found */
	handle = dbgsysLoadLibrary(fullPathName, errmsg, sizeof(errmsg));
	jdwpFree(fullPathName);
	if (handle != NULL) {
	    return handle;
	}
    }
    return NULL;
}

/*
 * loadTransport() is adapted from loadJVMHelperLib() in 
 * JDK 1.2 javai.c v1.61
 */
static jint
loadTransport(char *name, TransportInfo *infoPtr)
{
    JDWP_OnLoad_t JDWP_OnLoad = NULL;
    const char *onLoadSymbols[] = JDWP_ONLOAD_SYMBOLS;
    char fn2[MAXPATHLEN];
    char errmsg[MAXPATHLEN + 100];
    void *handle = infoPtr->handle;
    int i;

    if (handle == NULL) {
        /* First look along the path used by the native dlopen/LoadLibrary
         * functions.
         */
        dbgsysBuildLibName(fn2, sizeof(fn2), "", name);
        handle = dbgsysLoadLibrary(fn2, errmsg, sizeof(errmsg));
	if (handle == NULL) {
	    /* Get the system properties sun.boot.library.path and
	       java.library.path and search them just like ClassLoader.java
	       does. We take the first appropriately-named library we find. */

	    handle = searchPathsForTransport(fn2, bootLibraryPaths,
					     numBootLibraryPaths);
	}
	if (handle == NULL) {
	    return JDWP_ERROR(TRANSPORT_LOAD);
	}
	infoPtr->handle = handle;
    }

    for (i = 0; i < sizeof(onLoadSymbols) / sizeof(char *); i++) {
	JDWP_OnLoad = (JDWP_OnLoad_t)dbgsysFindLibraryEntry(handle, onLoadSymbols[i]);
	if (JDWP_OnLoad) {
	    break;
	}
    }
    if (JDWP_OnLoad) {
	JNIEnv *env = getEnv();
	jint res;
	JavaVM *jvm;
	(*env)->GetJavaVM(env, &jvm);
	res = (*JDWP_OnLoad)(jvm, &infoPtr->transport, &callback, NULL);
	if (res < 0) {
	    return JDWP_ERROR(TRANSPORT_INIT);
	}
	return JDWP_ERROR(NONE);
    } else {
	return JDWP_ERROR(TRANSPORT_LOAD);
    }
}

static jint 
handshake(Transport *transport)
{
    jbyte b;
    int i,len;

    len = strlen(handshakeString);

    for (i=0; i<len; i++) {
        jint rc = (*transport->receiveByte)(&b);
        if (rc != JDWP_ERROR(NONE))
            return rc;

        if (b != (jbyte)handshakeString[i])
            return JDWP_ERROR(TRANSPORT_INIT);
    }

    for (i=0; i<len; i++) {
        jint rc = (*transport->sendByte)((jbyte)handshakeString[i]);
        if (rc != JDWP_ERROR(NONE)) {
            return JDWP_ERROR(TRANSPORT_INIT);
        }
    }

    return JDWP_ERROR(NONE);
}

static void 
connectionInitiated(struct Transport *t)
{
    jint isValid = JNI_FALSE;
    jint error = JDWP_ERROR(NONE);

    /*
     * Don't allow a connection until initialization is complete
     */
    debugInit_waitInitComplete();

    debugMonitorEnter(listenerLock);

    /* Are we the first transport to get a connection? */

    if (transport == NULL) {
        error = handshake(t);
        if (error == JDWP_ERROR(NONE)) {
            transport = t;
            isValid = JNI_TRUE;
            debugMonitorNotifyAll(listenerLock);
            /*
             * %comment gordonh021
             */
        } 
    } else {
        /*
         * %comment gordonh022
         */
        JDI_ASSERT(JNI_FALSE);
    }


    debugMonitorExit(listenerLock);

    if (!isValid) {
        t->close();
        return;
    }

    debugLoop_run();
}

static void
acceptThread(void *ptr) 
{
    TransportInfo *info = ptr;
    struct Transport *transport = info->transport;
    jint error;
    error = (*transport->accept)();
    if (error != JDWP_ERROR(NONE)) {
        /* %comment gordonh023 */
	/* suppress error if we closed the socket */
	if (errno != JDWP_ERROR(VM_DEAD)) {
	    fprintf(stderr, "%s transport error; accept failed, rc = %d\n",
                 info->name, error);
	}
    }

    (*transport->stopListening)();

    connectionInitiated(transport);
}

static void
attachThread(void *ptr) 
{
    connectionInitiated(ptr);
}



void 
transport_initialize(JNIEnv *env)
{
    transport = NULL;
    listenerLock = debugMonitorCreate("JDWP Transport Listener Monitor");
    sendLock = debugMonitorCreate("JDWP Transport Send Monitor");
    /* Initialize the library paths for when we call loadTransport
       later, as long as we have a valid JNI environment */
    parseLibraryPaths(env);
}

void 
transport_reset()
{
    transport = NULL;
}

static jint
launch(char *command, char *name, char *address) 
{
    jint error;
    char *commandLine = jdwpAlloc(strlen(command) +
                                 strlen(name) +
                                 strlen(address) + 3);
    if (commandLine == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    strcpy(commandLine, command);
    strcat(commandLine, " ");
    strcat(commandLine, name);
    strcat(commandLine, " ");
    strcat(commandLine, address);

    error = dbgsysExec(commandLine);
    jdwpFree(commandLine);
    if (error != SYS_OK) {
        return JDWP_ERROR(TRANSPORT_INIT);
    } else {
        return JDWP_ERROR(NONE);
    }
}

jint 
transport_startTransport(jboolean isServer, char *name, char *address,
			 void **cookiePtr)
{
    TransportInfo *info;
    struct Transport *transport;
    char threadName[MAXPATHLEN + 100];
    jint err = JDWP_ERROR(NONE);

    if (*cookiePtr == NULL) {
        info = jdwpAlloc(sizeof(*info));
        if (info == NULL) {
            return JVMDI_ERROR_OUT_OF_MEMORY;
        }
	info->name = NULL; 
	info->address = NULL; 
	info->transport = NULL; 
	info->handle = NULL; 
	*cookiePtr = info;
    } else {
	info = (TransportInfo *)*cookiePtr;
    }

    if (info->transport == NULL) {
	if (isServer) {
	    if (info->name == NULL) {
		info->name = jdwpStrdup(name);
		if (info->name == NULL) {
		    err = JVMDI_ERROR_OUT_OF_MEMORY;
		    goto handleError;
		}
	    }
	    if (address != NULL && info->address == NULL) {
		info->address = jdwpStrdup(address);
		if (info->address == NULL) {
		    err = JVMDI_ERROR_OUT_OF_MEMORY;
		    goto handleError;
		}
	    }
	}
	err = loadTransport(name, info);
	if (err != JDWP_ERROR(NONE)) {
	    goto handleError;
	}
    }
    transport = info->transport;

    if (isServer) {
        char *retAddress;
        char *launchCommand;

        retAddress = address;
        err = (*transport->listen)(&retAddress);
        if (err != JDWP_ERROR(NONE)) {
           goto handleError;
        }

        strcpy(threadName, "JDWP Transport Listener: ");
        strcat(threadName, name);
        err = spawnNewThread(acceptThread, info, threadName);
        if (err != JDWP_ERROR(NONE)) {
            goto handleError;
        }

        launchCommand = debugInit_launchOnInit();
        if (launchCommand != NULL) {
            err = launch(launchCommand, name, retAddress);
            if (err != JDWP_ERROR(NONE)) {
                goto handleError;
            }
        }

        if ((address == NULL) || (strcmp(address, retAddress) != 0)) {
            if (launchCommand == NULL) {
                fprintf(stdout, "Listening for transport %s at address: %s\n",
                        name, retAddress);
                fflush(stdout);
            }
            jdwpFree(retAddress);
        }
        return JDWP_ERROR(NONE);
        
handleError:
        return err;
    } else {
        /*
         * Note that we don't attempt to do a launch here. Launching
         * is currently supported only in server mode.
         */

        /*
         * If we're connecting to another process, there shouldn't be
         * any concurrent listens, so its ok if we block here in this
         * thread, waiting for the attach to finish.
         */
         err = (*transport->attach)(address);
         if (err != JDWP_ERROR(NONE)) {
            return err;
         }

         /*
          * Start the transport loop in a separate thread
          */
         strcpy(threadName, "JDWP Transport Listener: ");
         strcat(threadName, name);
         return spawnNewThread(attachThread, transport, threadName);
    }
}

void 
transport_stopTransport(void *cookie)
{
    TransportInfo *info = (TransportInfo *)cookie;
    Transport *transport = info->transport;
    if (transport != NULL) {
	(*transport->stopListening)();
	(*transport->close)();
    }
}

void 
transport_unloadTransport(void *cookie)
{
    TransportInfo *info = (TransportInfo *)cookie;
    if (info->handle != NULL) {
	dbgsysUnloadLibrary(info->handle);
    }
}

void 
transport_close() 
{
    (*transport->close)();
}

jint 
transport_sendPacket(Packet *packet)
{
    jint retVal = JDWP_ERROR(NONE);

    /*
     * If the VM is suspended on debugger initialization, we wait 
     * for a connection before continuing. This ensures that all
     * events are delivered to the debugger. (We might as well do this
     * this since the VM won't continue until a remote debugger attaches
     * and resumes it.) If not suspending on initialization, we must
     * just drop any packets (i.e. events) so that the VM can continue
     * to run. The debugger may not attach until much later.
     */
    if ((transport == NULL) && debugInit_suspendOnInit()) {
        debugMonitorEnter(listenerLock);
        while (transport == NULL) {
            debugMonitorWait(listenerLock);
        }
        debugMonitorExit(listenerLock);
    }

    if (transport != NULL) {
        debugMonitorEnter(sendLock);
        retVal = (*transport->sendPacket)(packet);
        debugMonitorExit(sendLock);
    } /* else, bit bucket */

    return retVal;
}

jint 
transport_receivePacket(Packet *packet) {
    return (*transport->receivePacket)(packet);
}

void 
transport_lock() 
{
    debugMonitorEnter(listenerLock);
    debugMonitorEnter(sendLock);
}

void 
transport_unlock() 
{
    debugMonitorExit(sendLock);
    debugMonitorExit(listenerLock);
}
