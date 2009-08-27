/*
 * @(#)classload.c	1.94 06/10/10
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

/*
 * Contains functions that deal with loading a class.
 */

#ifdef CVM_CLASSLOADING

#include "javavm/include/interpreter.h"
#include "javavm/include/classes.h"
#ifdef CVM_DUAL_STACK
#include "javavm/include/dualstack_impl.h"
#endif
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/globals.h"
#include "javavm/include/preloader.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/directmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/path.h"
#include "native/java/util/zip/zip_util.h"
#include "native/common/jni_util.h"

#ifdef __cplusplus
#define this XthisX
#endif

static CVMClassBlock*
CVMclassLoadClass0(CVMExecEnv* ee, CVMClassLoaderICell* loader, 
		   const char* classname, 
		   CVMClassTypeID classTypeID);

/*
 * Link a class' hierarchy by resolving symbolic links to its superclass
 * and interfaces. All classes in the hierarcy are guaranteed to be loaded
 * before calling.
 */
CVMBool
CVMclassLinkSuperClasses(CVMExecEnv* ee, CVMClassBlock* cb)
{
    CVMClassTypeID superTypeID = CVMcbSuperclassTypeID(cb);
    CVMClassBlock* superCb;
    CVMClassLoaderICell* loader = CVMcbClassLoader(cb);
    int i;
    CVMtraceClassLoading(("CL: Linking superclasses of %C\n", cb));

    CVMassert(!CVMcbCheckRuntimeFlag(cb, SUPERCLASS_LOADED));
   
    /* Get the superclass. It should already be loaded. */
    superCb = CVMclassLookupClassWithoutLoading(ee, superTypeID, loader);

    CVMassert(superCb != NULL);
    CVMassert(CVMcbCheckRuntimeFlag(superCb, SUPERCLASS_LOADED));

    /* Don't allow a class to have a superclass it can't access */
    if (!CVMverifyClassAccess(ee, cb, superCb, CVM_FALSE)) {
	CVMthrowIllegalAccessError(
	     ee, "cannot access superclass %C from class %C", superCb, cb);
	goto error; /* exception already thrown */
    }

    /* 
     * Link the superclass in. This overwrites the CVMcbSuperclassTypeID() 
     * field, and we don't want to do this if there is an error, but we have
     * to do it now for the following code to work. Because of this, we
     * already saved the typeid and will restore it if there is an error.
     */
    CVMcbSuperclass(cb) = superCb;

    /* 
     * Make sure the interfaces are loaded and link them in. We only
     * look at the interfaces the class declares it implements.
     * Inherited interfaces will automatically be loaded recursively and
     * the inteface table slots will be filled in later by CVMclassLink. 
     * Note that CVMcbInterfaceCount() isn't even setup at this point.
     */
    for (i = 0; i < CVMcbImplementsCount(cb); i++) {
	CVMClassTypeID interfaceTypeID = CVMcbInterfaceTypeID(cb, i);
	CVMClassBlock* interfaceCb; 

	/* Get the interfaceCb. It should already be loaded. */
	interfaceCb = 
	    CVMclassLookupClassWithoutLoading(ee, interfaceTypeID, loader);
	
	CVMassert(interfaceCb != NULL);
	CVMassert(CVMcbCheckRuntimeFlag(interfaceCb, SUPERCLASS_LOADED));

	/* Make sure the class is really an interface. */
	if (!CVMcbIs(interfaceCb, INTERFACE)) {
	    CVMthrowIncompatibleClassChangeError(
                ee, "class %C is not an interface", interfaceCb);
	    goto error; /* exception already thrown */
	}
	
	/* Don't allow a class to have an interface it can't access */
	if (!CVMverifyClassAccess(ee, cb, interfaceCb, CVM_FALSE)) {
	    CVMthrowIllegalAccessError(
                ee, "cannot access interface %C from class %C",
		interfaceCb, cb);
	    goto error; /* exception already thrown */
	}
	
	/* If the following invariant isn't true, then someone has
	 * mucked with CVMInterfaceTable in a potentially bad way.
	 * 
	 * Take possible padding of interface table into account.
	 */
	CVMassert((CVMUint8*)&CVMcbInterfaceTypeID(cb, i) == 
		  (CVMUint8*)&CVMcbInterfacecb(cb, i) 
		  + CVMoffsetof(CVMInterfaceTable, intfInfoX.interfaceTypeIDX));
	CVMcbInterfacecb(cb, i) = interfaceCb;
    }

    /*
     * java.lang.ref.Reference is already ROMized and has its
     * REFERENCE bit set. Same for all its ROMized subclasses. So the
     * bootstrapping is easy.
     */
    if (CVMcbIs(superCb, REFERENCE)) {
	CVMcbSetAccessFlag(cb, REFERENCE);
    }

    CVMassert(!CVMcbCheckRuntimeFlag(cb, SUPERCLASS_LOADED));
    CVMcbSetRuntimeFlag(cb, ee, SUPERCLASS_LOADED);
    return CVM_TRUE;

 error:
    CVMcbSetErrorFlag(ee, cb);
    CVMcbSuperclassTypeID(cb) = superTypeID;
    return CVM_FALSE;
}

/*
 * Load a class using the specified class loader.
 */

CVMClassBlock*
CVMclassLoadClass(CVMExecEnv* ee, CVMClassLoaderICell* loader, 
		  const char* classname,
		  CVMClassTypeID classTypeID)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMClassBlock* cb;

    CVMtraceClassLoading(("CL: trying to load class \"%s\" using "
			  "0x%x class loader\n", classname, loader));

    CVMassert(!CVMexceptionOccurred(ee));

    /* We need a JNI local frame so we can make method calls using JNI. */
    if (CVMjniPushLocalFrame(env, 4) != JNI_OK) {
	return NULL; /* excpeption already thrown */
    }

    cb = CVMclassLoadClass0(ee, loader, classname, classTypeID);

    CVMjniPopLocalFrame(env, NULL);


    {
	CVMBool isSameClassLoader = CVM_TRUE;
	if (cb != NULL) {
	    CVMID_icellSameObjectNullCheck(ee, loader, CVMcbClassLoader(cb), 
					   isSameClassLoader);
	}
	if (!isSameClassLoader) {
	    CVMtraceClassLoading(("CL: class \"%s\" already loaded by "
				  "0x%x class loader.\n",
				  classname, CVMcbClassLoader(cb)));
	} else {
	    CVMtraceClassLoading(("CL: class \"%s\"%s loaded by "
				  "0x%x class loader.\n", classname,
				  cb == NULL ? " not" : "", loader));
	}
    }
    return cb;
}

/* Helper function for CVMclassLoadClass */
static CVMClassBlock*
CVMclassLoadClass0(CVMExecEnv* ee, CVMClassLoaderICell* loader, 
		   const char* classname, 
		   CVMClassTypeID classTypeID)
{
    CVMClassBlock* cb = NULL;
    jclass         result;
    JNIEnv*        env = CVMexecEnv2JniEnv(ee);
    jstring        classString;
    char           dot_name_buf[256];
    char*          dot_name;
    int i;

    CVMassert(!CVMexceptionOccurred(ee));

    /* 
     * Allocate a buffer for the tranlated class name if the stack buffer
     * isn't big enough.
     */
    if (strlen(classname) >= sizeof(dot_name_buf)) {
        dot_name = (char *)malloc(strlen(classname) + 1);
	if (dot_name == NULL) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return NULL;
	}
    } else {
        dot_name = dot_name_buf;
    }

    /* Copy the class name, translating slashes to dots in the process. */
    for (i = 0; classname[i]; i++) {
        if (classname[i] == '/') {
	    dot_name[i] = '.';
        } else {
	    dot_name[i] = classname[i];
	}
    }
    dot_name[i] = 0;

    /* Convert the class name to a String object. */
    classString = (*env)->NewStringUTF(env, dot_name);
    if (classString == 0) {
        goto end;
    }

    /* Call loader.loadClass() */
    if (loader != NULL) {
	result = (*env)->CallObjectMethod(env,
            loader, 
	    CVMglobals.java_lang_ClassLoader_loadClass,
	    classString);
    } else {
	result = (*env)->CallStaticObjectMethod(env,
            CVMcbJavaInstance(CVMsystemClass(java_lang_ClassLoader)),
       	    CVMglobals.java_lang_ClassLoader_loadBootstrapClass,
            classString);
    }

    if (CVMexceptionOccurred(ee)) {
	cb = NULL;
	/* Exception already thrown, so we can't trust the return value.
	   Let exception stand */
	goto end;
    }
    
    if (result == NULL || CVMID_icellIsNull(result)) {
	cb = NULL;
    } else {
	cb = CVMgcSafeClassRef2ClassBlock(ee, result);
    }

    if (!CVMexceptionOccurred(ee) && cb == NULL) {
	/* Loader returned NULL without raising an exception. */
	CVMthrowClassNotFoundException(ee, "%s", classname);
	goto end;
    }
    
    /* 
     * We do not trust that the ClassLoader.loadClass method actually
     * returns the right class.
     */
    if (cb != NULL && classTypeID != CVMcbClassName(cb)) {
        CVMthrowNoClassDefFoundError(ee, 
				     "Bad class name (expect: %!C, get: %!C)",
				     classTypeID, CVMcbClassName(cb));
	cb = NULL;
    }

 end:
    if (dot_name != dot_name_buf) {
        free(dot_name);
    }
    return cb;
}

/*
 * Class path element, which is either a directory or zip file.
 */

typedef enum {
    CVM_CPE_DIR, CVM_CPE_ZIP, CVM_CPE_INVALID
} CVMClassPathEntryType;

struct CVMClassPathEntry {
    CVMClassPathEntryType type;
    jzfile *zip;
    char *path;
    /* The cached CodeSource object of this path component */
    CVMObjectICell* codeSource;
    /* The cached protection domain of this path component */
    CVMObjectICell* protectionDomain;   
    /* The cached URL object of this path component */
    CVMObjectICell* url;   
    /* The cached JarFile object of this path component */
    CVMObjectICell* jfile;
    /* The class path entry is a signed jar file */
    CVMBool isSigned;
    /* Has the Java side of this been initialized */
    CVMBool javaInitialized;
};

/*
 * Found item on a classpath
 */
typedef struct ClassInfo {
    CVMUint8*           classData;
    CVMUint32           fileSize32;
    const char*         dirname;
    const char*         entryname;
    CVMBool             fromZip;
    CVMClassPathEntry*  pathComponent;
    CVMBool             classFound;
} ClassInfo;

static void
initInfo(ClassInfo* info)
{
    info->classData     = NULL;
    info->fileSize32    = 0;
    info->dirname       = NULL;
    info->entryname     = NULL;
    info->fromZip       = CVM_FALSE;
    info->pathComponent = NULL;
    info->classFound    = CVM_FALSE;
}

static void
searchPathAndFindClassInfo(CVMExecEnv* ee, const char* classname, 
			   char* filename, CVMClassPath* path, 
			   ClassInfo* info);


CVMClassICell*
CVMclassLoadBootClass(CVMExecEnv* ee, const char* classname)
{
    ClassInfo info;
    CVMClassICell* classRoot;
    char * filename = (char *)malloc(CVM_PATH_MAXLEN);
    CVMClassPath* path = &CVMglobals.bootClassPath;

    if (filename == NULL) {
        CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    }

    initInfo(&info);
    
    searchPathAndFindClassInfo(ee, classname, filename, path, &info);

    if (info.classFound) {
	CVMassert(info.classData != NULL);
	
	/* We found this class in one of the path components */
	/* create the CVMClassBlock */
	classRoot =
	    CVMclassCreateInternalClass(ee, info.classData, info.fileSize32,
					NULL, classname, info.dirname,
					CVM_FALSE);
	free(info.classData);
	if (classRoot == NULL) {
	    goto done;
	}
	CVMtraceClassLoading(("CL: class \"%s\" "
			      "loaded from path component \"%s\".\n",
			      classname, info.dirname));
    } else {
	CVMassert(info.classData == NULL);
	classRoot = NULL;
	/*
	 * This returns NULL on class not found,
	 * and leaves it to a wrapper to throw an exception if
	 * appropriate.
	 */
    }

 done:
    free(filename);
    
    return classRoot;
}

#ifdef CVM_CLASSLIB_JCOV
/* Purpose: Loads a classfile from the specified path without converting it
            into a classblock. */
CVMUint8 *
CVMclassLoadSystemClassFileFromPath(CVMExecEnv *ee, const char *classname,
                                    CVMClassPath *path,
                                    CVMUint32 *classfileSize)
{
    ClassInfo info;
    char *filename = malloc(CVM_PATH_MAXLEN);

    if (filename == NULL) {
        CVMthrowOutOfMemoryError(ee, NULL);
        return NULL;
    }

    initInfo(&info);
    searchPathAndFindClassInfo(ee, classname, filename, path, &info);
    if (info.classFound) {
        CVMassert(info.classData != NULL);
        *classfileSize = info.fileSize32;
    } else {
        CVMassert(info.classData == NULL);
        *classfileSize = 0;
        if (!CVMexceptionOccurred(ee)) {
            CVMthrowClassNotFoundException(ee, "%s", classname);
        }
    }
    free(filename);
    return info.classData;
}

/* Purpose: Releases a previously loaded classfile. */
void
CVMclassReleaseSystemClassFile(CVMExecEnv *ee, CVMUint8 *classfile)
{
    free(classfile);
}
#endif

/*
 * CVMclassLoadFromFile: Creates and returns the cb for the specified
 * class file.
 *
 *   -filename:  full path name of the class file
 *   -classname: name of the class (including package part)
 *
 * Returns NULL if the file cannot be opened or if there are any errors
 * while trying to access the class file. An exception is thrown for
 * all errors other than failing to open the class file.
 */
static void 
getClassInfoFromFile(CVMExecEnv* ee,
		     const char* filename,
		     const char* dirname,
		     ClassInfo* info)
{
    CVMInt32 codefd;
    CVMInt64 filesize64;
    CVMInt64 filesize64Truncated;
    CVMInt32 filesize32;
    CVMUint8* externalClass = NULL;

    /* open the file */
    codefd = CVMioOpen(filename, O_RDONLY, 0);
    if (codefd <= 0) {
	/* If the file isn't found, then we don't throw an exception
	 * because the caller may still want to search in other directories.
	 */
	info->classFound = CVM_FALSE;
	goto finish;
    }
    info->dirname    = dirname;
    info->entryname  = filename;
    info->fromZip    = CVM_FALSE;

    /* get the file size */
    if (CVMioFileSizeFD(codefd, &filesize64) != 0) {
	CVMthrowInternalError(ee, "could not get class file size");
	goto finish;
    }
    filesize32 = CVMlong2Int(filesize64);
    filesize64Truncated = CVMint2Long(filesize32);
    if (CVMlongCompare(filesize64Truncated, filesize64)) {
	CVMthrowIOException(ee, "%s", filename);
	goto finish; /* file is too big */
    }

    /* allocate space for the file */
    externalClass = (CVMUint8 *)malloc(filesize32);
    if (externalClass == NULL) {
	CVMthrowOutOfMemoryError(ee, "loading class file");
	goto finish;
    }

    /* read in the file */
    if (CVMioRead(codefd, externalClass, filesize32) != filesize32) {
	CVMthrowIOException(ee, "%s", filename);
	goto finish;
    }
    
    /*
     * Close the file now, or else we may end up with a bunch of open
     * files because CVMclassCreateInternalClass() is recursive.
     * (NOTE: Actually it is no longer recursive, but it is still
     *  best to close the file now.)
     */
    if (codefd > 0) {
	CVMioClose(codefd);
	codefd = -1; /* so we don't try to close it again below. */
    }

    info->classData  = externalClass;
    info->fileSize32 = filesize32;
    info->classFound = CVM_TRUE;
    
    return;
    
 finish:
    if (codefd > 0) {
	CVMioClose(codefd);
    }
    if (externalClass != NULL) {
	free(externalClass);
    }
}

/*
 * CVMclassLoadFromZipFile: Creates and returns the cb for the specified
 * class file in the specified zip file..
 *
 *   -filename:  full path name of the class file
 *   -classname: name of the class (including package part). The buffer will
 *               get clobbered during the call to CVMclassLoadFromZipFile(),
 *   -zipfile:   the zip file.
 *
 * Returns NULL if the file cannot be opened or if there are any errors
 * while trying to access the class file. An exception is thrown for
 * all errors other than failing to open the class file.
 */

static void 
getClassInfoFromZipFile(CVMExecEnv* ee,
			char* filename,
			jzfile* zipfile,
			ClassInfo* info)
{
    jzentry* zipEntry;
    CVMUint8* externalClass = NULL;
    jint fileSize;
    jint fileNameLen;

    /* See if the class file is in this zip file. */
    zipEntry = ZIP_FindEntry(zipfile, filename, &fileSize, &fileNameLen);
    if (zipEntry == NULL) {
	info->classFound = CVM_FALSE;
	goto finish;
    }
    
    info->fileSize32 = fileSize;
    info->entryname  = filename;
    info->dirname    = zipfile->name;
    info->fromZip    = CVM_TRUE;

    /* Allocate space for the class file. */
    externalClass = (CVMUint8 *)malloc(fileSize);
    if (externalClass == NULL) {
	CVMthrowOutOfMemoryError(ee, "loading class file");
	goto finish;
    }

    /* 
     * Read in the class file. Note that filename will get overwritten
     * with itself. ZIP_ReadEntry requires a buffer to store the file name
     * in even though we already know the file name.
     */
    if (!ZIP_ReadEntry(zipfile, zipEntry, externalClass, filename)) {
	goto finish;
    }

    info->classData  = externalClass;
    info->classFound = CVM_TRUE;

    return;

 finish:
    if (externalClass != NULL) {
	free(externalClass);
    }
}

static void
searchPathAndFindClassInfo(CVMExecEnv* ee, const char* classname,
			   char* filename, CVMClassPath* path, 
			   ClassInfo* info)
{
    /* 
     * Iterate over each class path entry, trying to load the class file
     * from a directory or zip file in the class path.
     */
    CVMUint32 idx;

    for (idx = 0; idx < path->numEntries; idx++) {
	CVMClassPathEntry* currEntry = &path->entries[idx];

	if (currEntry->type == CVM_CPE_DIR) {
	    /* Look for class file in specified classpath directory. */
	    const char* dirname = currEntry->path;

	    if (strlen(dirname) + 1 + strlen(classname) + 1 +
		strlen(CVM_PATH_CLASSFILEEXT) > CVM_PATH_MAXLEN) {
		CVMtraceClassLoading(("CL: filename is too long: "
				      "%s%c%s.%s\n", 
				      dirname, CVM_PATH_LOCAL_DIR_SEPARATOR,
				      classname, CVM_PATH_CLASSFILEEXT));
		continue;
	    }
	
	    sprintf(filename, "%s%c%s.%s", dirname, 
			CVM_PATH_LOCAL_DIR_SEPARATOR,
			classname, CVM_PATH_CLASSFILEEXT);
	    info->pathComponent = currEntry;
	    getClassInfoFromFile(ee, CVMioNativePath(filename),
				 dirname, info);
	} else if (currEntry->type == CVM_CPE_ZIP) {
	    /* Look for class file in specified bootclasspath zip file. */

	    if (strlen(classname) + 1 +
		strlen(CVM_PATH_CLASSFILEEXT) > CVM_PATH_MAXLEN) {
		CVMtraceClassLoading(("CL: filename is too long: "
				      "%s.%s\n", classname,
				      CVM_PATH_CLASSFILEEXT));
		continue;
	    }
	    sprintf(filename, "%s.%s", classname, CVM_PATH_CLASSFILEEXT);
	    info->pathComponent = currEntry;
	    getClassInfoFromZipFile(ee, filename, currEntry->zip, info);
	} else if (currEntry->type == CVM_CPE_INVALID) {
	    continue;
	} else {
	    CVMassert(CVM_FALSE);
	}

	if (info->classFound || CVMexceptionOccurred(ee)) {
	    /* We are done searching */
	    /* There was an exception or we found the data */
	    return;
	}
    }
}

/*
 * Takes class name, converts it to a C utf name, and replaces '.' with '/'
 */
static char*
CVMclassProcessNameForLoading(JNIEnv* env, jstring name)
{
    char *utfName;

    CVMassert(name != NULL);
    {
        int len = (*env)->GetStringUTFLength(env, name);
	int unicode_len = (*env)->GetStringLength(env, name);
	utfName = (char *)malloc(len + 1);
	if (utfName == NULL) {
	    JNU_ThrowOutOfMemoryError(env, NULL);
	    return NULL;
	}
    	(*env)->GetStringUTFRegion(env, name, 0, unicode_len, utfName);
	VerifyFixClassname(utfName);
    }
    return utfName;
}

static jclass
defineClassLocal(CVMExecEnv* ee, const char* clname,
		 CVMObjectICell* appLoader, ClassInfo* info)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMObjectICell* resultCell;
    CVMObjectICell* pdCell;
    jobject csObject;
    jobject appclObject;
    jobject clnameObject;
    jclass clClass;
    jmethodID mid;

    csObject = info->pathComponent->codeSource;
    appclObject = CVMsystemClassLoader(ee);
    CVMassert(appclObject != NULL);
    CVMassert(!CVMID_icellIsNull(appclObject));

    clClass = CVMcbJavaInstance(CVMsystemClass(java_lang_ClassLoader));
    mid = CVMglobals.java_lang_ClassLoader_checkCerts;
    clnameObject = (*env)->NewStringUTF(env, clname);
    if (CVMexceptionOccurred(ee) || clnameObject == NULL) {
        return NULL;
    }

    (*env)->CallNonvirtualVoidMethod(env, appclObject, clClass,
				     mid, clnameObject, csObject); 
    if (CVMexceptionOccurred(ee)) {
        return NULL;
    }

    pdCell = info->pathComponent->protectionDomain;
    
    resultCell = CVMdefineClass(ee, clname, appLoader,
				info->classData, info->fileSize32, pdCell,
				CVM_FALSE);

    return resultCell;
}

static CVMBool 
fillInProtectionDomain(JNIEnv* env, CVMClassPathEntry* entry);

static CVMBool 
fillInJavaSide(JNIEnv* env, CVMClassPathEntry* entry);

static CVMBool 
initPathJavaSide(JNIEnv* env, ClassInfo* info);

JNIEXPORT jobject JNICALL
CVMclassFindContainer(JNIEnv *env, jobject this, jstring name) {
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    const char* clPlatformName;
    char* clPlatformSlashName;
    char* clUtfName;
    char* filename;
    char *p;
    
    CVMClassPath* appPath = &CVMglobals.appClassPath;
    ClassInfo info;

    jobject retObject = NULL;
    
    if (name == NULL) {
        CVMthrowClassNotFoundException(ee, "(null)");
	return NULL;
    }

    /*
     * Prevent circularity accidents
     */
    CVMassert(appPath->initialized);
    
    filename = (char *)malloc(CVM_PATH_MAXLEN);    
    if (filename == NULL) {
        CVMthrowOutOfMemoryError(ee, 0);
        return 0;
    }

    /* Get the class name's utf C string */
    clUtfName = CVMclassProcessNameForLoading(env, name);
    if (clUtfName == NULL) {
	CVMassert(CVMlocalExceptionOccurred(ee));
        goto end1;
    }

    /* Get the class name's i18n C string */
    clPlatformName = JNU_GetStringPlatformChars(env, name, NULL);
    if (clPlatformName == NULL) {
	CVMassert(CVMlocalExceptionOccurred(ee));
        goto end2;
    }
    /* Translate dots to slashes. */
    if (strchr(clPlatformName, '.') != NULL) {
        clPlatformSlashName = strdup(clPlatformName);
        if (clPlatformSlashName == NULL) {
            CVMthrowClassNotFoundException(ee, 0);
            goto end3;
        }
        p = clPlatformSlashName;
        while (*p != '\0') {
            if (*p == '.') {
                *p++ = '/';
            } else {
                p++;
            }
        }
    } else {
        clPlatformSlashName = (char *)clPlatformName;
    }

    initInfo(&info);

    searchPathAndFindClassInfo(ee, clPlatformSlashName,
                               filename, appPath, &info);
    if (info.classFound) {
	jobject jfile;
	jclass ccClass;
	jstring entryname;
	jobject url;
	jclass retClass;
	jmethodID mid;
	
	CVMassert(info.classData != NULL);
    
#ifdef CVM_LVM /* %begin lvm */
	/* In LVM-enabled mode, execution can get to this point without
	 * initialization of system class loader of this LVM.
	 * Get it initialized before invoking initPathJavaSide(). */
	if (CVMclassGetSystemClassLoader(ee) == NULL) {
	    return CVM_FALSE; /* exception already thrown */
	}
#endif /* %end lvm */
        /*
         * Initialize the java side informantion for the class path
         * component.
         */
        if (!initPathJavaSide(env, &info)) {
            if (!CVMexceptionOccurred(ee)) {
		CVMthrowOutOfMemoryError(ee,
		    "Couldn't initialize path component.");
	    }
            goto end;
        }
	
	/*
	 * Take the short-cut jfile and url from the path component
	 */
	jfile = info.pathComponent->jfile;
	url   = info.pathComponent->url;
	/*
	 * Now start computing
	 */

	/*
	 * Get the entryname for the ZIP case
	 */
	if (info.fromZip) {
	    entryname = (*env)->NewStringUTF(env, info.entryname);
            if (CVMexceptionOccurred(ee)) {
                goto end;
            }
	} else {
	    entryname = NULL;
	}
	
        /*
         * Finally, ship it all back to the caller, nicely packaged
         * in a Launcher.ClassContainer
         */
        ccClass =  CVMcbJavaInstance(
                       CVMsystemClass(sun_misc_Launcher_ClassContainer));
        if (!info.pathComponent->isSigned) {
            /*
             * If the class path entry is not signed,
             * define the class from the data we read.
             */
	    retClass = defineClassLocal(ee, clUtfName, this, &info);
            if (retClass == NULL) {
                goto end;
            }
            mid = CVMglobals.sun_misc_Launcher_ClassContainer_init_withClass;
            retObject = (*env)->NewObject(env, ccClass, mid,
                                          url, retClass, entryname, jfile);
        } else {
            mid = CVMglobals.sun_misc_Launcher_ClassContainer_init;
            retObject = (*env)->NewObject(env, ccClass, mid,
                                          url, entryname, jfile);
        }
    } else {
	retObject = NULL;
	/* Preserve pending exception */
	if (!CVMlocalExceptionOccurred(ee)) {
	    CVMthrowClassNotFoundException(ee, "%s", clUtfName);
	}
    }
 
 end:
    if (info.classFound) {
        free(info.classData);
    }
    if (clPlatformSlashName != clPlatformName) {
        free(clPlatformSlashName);
    }
 end3:
    JNU_ReleaseStringPlatformChars(env, name, clPlatformName);
 end2:
    free(clUtfName);
 end1:
    free(filename);

    return retObject;
}


/*
 * Initialize a class path data structure based on the value of
 * CVMglobals classpath pathString. 
 */

CVMBool
CVMclassBootClassPathInit(JNIEnv *env)
{
    if (CVMglobals.bootClassPath.pathString == NULL) {
        CVMglobals.bootClassPath.initialized = CVM_TRUE;
        return CVM_TRUE;
    } else {
        return CVMclassPathInit(env, &CVMglobals.bootClassPath, NULL,
                                CVM_FALSE, CVM_FALSE);
    }
}

CVMBool
CVMclassClassPathInit(JNIEnv *env)
{
    return CVMclassPathInit(env, &CVMglobals.appClassPath, NULL,
			    CVM_TRUE, CVM_TRUE);
}

#ifdef CVM_MTASK
/*
 * Append 'classPath' onto the existing classpath and bootClassPath
 * onto the existing boot classpath.
 */
CVMBool
CVMclassClassPathAppend(JNIEnv *env, char* classPath, char* bootClassPath)
{
    jmethodID getPropertiesID;
    jmethodID updateAppCLID;
    jmethodID putID;

    jobject props; /* System properties */
    
    if (!CVMclassPathInit(env, &CVMglobals.appClassPath, 
			  classPath, CVM_TRUE, CVM_TRUE)) {
	return CVM_FALSE;
    }

    if (!CVMclassPathInit(env, &CVMglobals.bootClassPath, 
			  bootClassPath, CVM_TRUE, CVM_FALSE)) {
	return CVM_FALSE;
    }

    updateAppCLID = 
	(*env)->GetStaticMethodID(env, 
				  CVMcbJavaInstance(CVMsystemClass(sun_misc_Launcher)), 
			    "updateLauncher",
			    "()V");

    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionDescribe(env);
	return CVM_FALSE;
    }

    getPropertiesID = 
	(*env)->GetStaticMethodID(env, 
			    CVMcbJavaInstance(CVMsystemClass(java_lang_System)), 
			    "getProperties",
			    "()Ljava/util/Properties;");

    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionDescribe(env);
	return CVM_FALSE;
    }

    props = (*env)->CallStaticObjectMethod(env,
                CVMcbJavaInstance(CVMsystemClass(java_lang_System)),
                getPropertiesID);

    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionDescribe(env);
	return CVM_FALSE;
    }
    
    putID = 
	(*env)->GetMethodID(env,
			    (*env)->GetObjectClass(env, props),
			    "setProperty",
			    "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");

    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionDescribe(env);
	return CVM_FALSE;
    }

    if (!CVMputPropForPlatformCString(env, putID, props, "java.class.path",
				      CVMglobals.appClassPath.pathString)) {
	if ((*env)->ExceptionOccurred(env)) {
	    (*env)->ExceptionDescribe(env);
	}
        return CVM_FALSE;
    }

    if (!CVMputPropForPlatformCString(env, putID, props, "sun.boot.class.path",
				      CVMglobals.bootClassPath.pathString)) {
	if ((*env)->ExceptionOccurred(env)) {
	    (*env)->ExceptionDescribe(env);
	}
        return CVM_FALSE;
    }

    /* Now tell the Launcher to update our singleton app class loader */
    (*env)->CallStaticVoidMethod(env,
                CVMcbJavaInstance(CVMsystemClass(sun_misc_Launcher)),
                updateAppCLID);

    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionDescribe(env);
	return CVM_FALSE;
    }
    return CVM_TRUE;
}
#endif

/*
 * We have set up a class component.
 * Create the Java objects that matter for this path component
 */
static CVMBool 
fillInJavaSide(JNIEnv* env, CVMClassPathEntry* entry)
{
    jobject fileObject = NULL;
    jobject fileString;
    jclass fileClass;
    jclass launcherClass;
    jmethodID mid;
    jobject urlObject = NULL;
    jobject jfileObject = NULL;

    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    
    /*
     * new File(codebase)
     */
    fileString = (*env)->NewStringUTF(env, entry->path);
    if (CVMexceptionOccurred(ee)) {
        return CVM_FALSE;
    }
    fileClass = CVMcbJavaInstance(CVMsystemClass(java_io_File));
    mid = CVMglobals.java_io_File_init;
    if (CVMclassInit(ee, CVMsystemClass(java_io_File))) {
        fileObject = (*env)->NewObject(env, fileClass, mid, fileString);
    }
    if (CVMexceptionOccurred(ee) || fileObject == NULL) {
        return CVM_FALSE;
    }
    /*
     * Now map this file object to a URL using Launcher.getFileURL()
     */
    launcherClass = CVMcbJavaInstance(CVMsystemClass(sun_misc_Launcher));
    mid = CVMglobals.sun_misc_Launcher_getFileURL;
    if (CVMclassInit(ee, CVMsystemClass(sun_misc_Launcher))) {
        urlObject = (*env)->CallStaticObjectMethod(env, launcherClass,
                                                   mid, fileObject);
    }
    if (CVMexceptionOccurred(ee)) {
        return CVM_FALSE;
    }
    CVMID_icellAssign(ee, entry->url, urlObject);
	    
    /*
     * Now if it is a zip, create a JarFile out of this File object.
     */
    if (entry->type == CVM_CPE_ZIP) {
	fileClass = CVMcbJavaInstance(CVMsystemClass(java_util_jar_JarFile));
	mid = CVMglobals.java_util_jar_JarFile_init;
        if (CVMclassInit(ee, CVMsystemClass(java_util_jar_JarFile))) {
            jfileObject = (*env)->NewObject(env, fileClass, mid, fileObject);
        }
        if (CVMexceptionOccurred(ee) || jfileObject == NULL) {
            return CVM_FALSE;
        }
	CVMID_icellAssign(ee, entry->jfile, jfileObject);
    }

    return CVM_TRUE;
}

/*
 * Obtain the system class loader (initialize the cache if not yet done)
 * NOTE: Having no synchronization around checking the cache is 
 * harmless; we may just end up allocating an extra global root (which will 
 * be freed on VM exit), or assigning the same copy to the cache twice.
 */
CVMClassLoaderICell* 
CVMclassGetSystemClassLoader(CVMExecEnv* ee)
{
    /* 
     * We call CVMsystemClassLoader(ee) to obtain the system
     * class loader. We'll cache it in the CVMglobals so we only have
     * to do this once.
     */
    if (CVMsystemClassLoader(ee) == NULL) {
	CVMsystemClassLoader(ee) = CVMID_getGlobalRoot(ee);
	if (CVMsystemClassLoader(ee) == NULL) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return NULL; /* out of memory */
	}
    }
    if (CVMID_icellIsNull(CVMsystemClassLoader(ee))) {
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	const CVMClassBlock* loaderCb =
	    CVMsystemClass(java_lang_ClassLoader);
	CVMMethodTypeID methodID = CVMtypeidLookupMethodIDFromNameAndSig(
	    ee, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	CVMMethodBlock* mb = 
	    CVMclassGetStaticMethodBlock(loaderCb, methodID);
	jobject systemClassLoader;
#ifdef CVM_DEBUG_ASSERTS
        CVMBool sameClassLoader;
#endif
        
	CVMassert(mb != NULL);
	systemClassLoader = 
	    (CVMClassLoaderICell*)(*env)->CallStaticObjectMethod(
		env, CVMcbJavaInstance(loaderCb), mb);
	if (CVMexceptionOccurred(ee)) {
	    return NULL;
	}
        /* When sun.misc.Launcher creates the appClassLoader, it
         * also sets the systemClassLoader in CVMglobals. */
#ifdef CVM_DEBUG_ASSERTS
        CVMID_icellSameObjectNullCheck(ee, CVMsystemClassLoader(ee),
                              systemClassLoader, sameClassLoader);
        CVMassert(sameClassLoader);
#endif
	if (systemClassLoader != NULL) {
	    CVMjniDeleteLocalRef(env, systemClassLoader);
	}
    }
    return CVMsystemClassLoader(ee);
}

/*
 * Set the system classloader.
 */
void
CVMclassSetSystemClassLoader(CVMExecEnv* ee, jobject loader)
{
    CVMassert(loader != NULL);
    if (CVMID_icellIsNull(CVMsystemClassLoader(ee))) {
	  CVMID_icellAssign(ee, CVMsystemClassLoader(ee), loader);
    }
}

/*
 * We have set up a class component.
 * Create the Java objects that matter for this path component
 */
static CVMBool 
fillInProtectionDomain(JNIEnv* env, CVMClassPathEntry* entry)
{
    jclass csClass;
    jclass sclClass;
    jmethodID mid;
    jobject pdObject;
    jobject csObject;
    jobject appclObject;

    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    
    /*
     * The default ProtectionDomain creation
     * 1) create cs = new CodeSource(url, null)
     * 2) set pd = SecureClassLoader.getProtectionDomain(cs)
     */
    csClass = CVMcbJavaInstance(CVMsystemClass(java_security_CodeSource));
    mid = CVMglobals.java_security_CodeSource_init;
    csObject = (*env)->NewObject(env, csClass, mid, entry->url, NULL);
    if (CVMexceptionOccurred(ee) || csObject == NULL) {
        return CVM_FALSE;
    }
    CVMID_icellAssign(ee, entry->codeSource, csObject);
    
    appclObject = CVMsystemClassLoader(ee);
    CVMassert(appclObject != NULL);
    CVMassert(!CVMID_icellIsNull(appclObject));

    sclClass = CVMcbJavaInstance(CVMsystemClass(java_security_SecureClassLoader));
    mid = CVMglobals.java_security_SecureClassLoader_getProtectionDomain;
    pdObject = (*env)->CallNonvirtualObjectMethod(env, appclObject, sclClass, mid, csObject);
    if (CVMexceptionOccurred(ee)) {
        return CVM_FALSE;
    }
    CVMID_icellAssign(ee, entry->protectionDomain, pdObject);

    return CVM_TRUE;
}

static CVMBool 
initPathJavaSide(JNIEnv* env, ClassInfo* info) {
    if (!info->pathComponent->javaInitialized) {
        CVMBool ok;
        jobject appclObject = CVMsystemClassLoader(CVMjniEnv2ExecEnv(env));

        /*
         * This can only be called from the AppClassLoader (the
         * system classloader). So the CVMsystemClassLoader
         * should not contain null.
         */
        CVMassert(!CVMID_icellIsNull(appclObject));
        if ((*env)->MonitorEnter(env, appclObject) == JNI_OK) {
            if (info->pathComponent->javaInitialized) {
                ok = (*env)->MonitorExit(env, appclObject);
                CVMassert(ok == JNI_OK);
                return CVM_TRUE;
            }
            if (!fillInJavaSide(env, info->pathComponent)) {
                (*env)->MonitorExit(env, appclObject);
                return CVM_FALSE;
            }
            /*
             * We only fill in ProtectionDomain if
             * the current path entry is not signed.
             */
            if (!info->pathComponent->isSigned) {
                if (!fillInProtectionDomain(env, info->pathComponent)) {
                    (*env)->MonitorExit(env, appclObject);
                    return CVM_FALSE;
                }
            }
            info->pathComponent->javaInitialized = CVM_TRUE;
            ok = (*env)->MonitorExit(env, appclObject);
            CVMassert(ok == JNI_OK);
            return CVM_TRUE;
        } else {
            return CVM_FALSE;
        }
    }
    return CVM_TRUE;
}

static CVMUint32
getNumPathComponents(char* pathStr)
{
    char* pathSepPtr;
    CVMUint32 numEntries;

    numEntries = 0;
    pathSepPtr = pathStr;
    while (pathSepPtr != NULL) {
	numEntries++;
	pathSepPtr = strstr(pathSepPtr, CVM_PATH_CLASSPATH_SEPARATOR);
	if (pathSepPtr != NULL) {
	    pathSepPtr += strlen(CVM_PATH_CLASSPATH_SEPARATOR);
	}
    }
    return numEntries;
}

/* Initialize class path. If additionalPathString is supplied, add this to
   what we already know about this classpath */
/* Note: was static, now used by JVMTI code */
CVMBool
CVMclassPathInit(JNIEnv* env, CVMClassPath* classPath, 
		 char* additionalPathString,
		 CVMBool doNotFailWhenPathNotFound, CVMBool initJavaSide)
{
    int idx; /* classpath component index */
    char* pathStr;

    classPath->initialized = CVM_TRUE;

    /* 
     * CVMglobals.bootClassPath.pathString and 
     * CVMglobals.apptClassPath.pathString were already initialized 
     * during CVM global states initialization.
     */
     if ((classPath->pathString == NULL) &&
	 (additionalPathString  == NULL)) {
         /* 
          * The global pathString is not initialized for this classPath
          * Trust the caller to know 
          */
         return doNotFailWhenPathNotFound;
     }
    
    /*
     * Count the number of entries in the class path string and allocate
     * a large enough entries array for it.
     */
    {
	if (classPath->numEntries == 0) {
	    CVMUint32 numEntries;
	    /* This is the first initialization of this path */
	    CVMassert(classPath->entries == NULL);
	    pathStr = classPath->pathString;
	    if (pathStr == NULL) {
		/* Allow the additional path string to be the first
		   initialization of this class path */
		pathStr = additionalPathString;
		classPath->pathString = strdup(pathStr);
		if (classPath->pathString == NULL) {
		    return CVM_FALSE;
		}
	    }
	    /* Initialize one or the other */
	    CVMassert(pathStr != NULL);

	    numEntries = getNumPathComponents(pathStr);
    
	    classPath->numEntries = numEntries;
	    /* allocate space for all of the classpath entries */
	    classPath->entries = (CVMClassPathEntry*)
		malloc(classPath->numEntries * sizeof(CVMClassPathEntry));
	    if (classPath->entries == NULL) {
		free(classPath->pathString);
		classPath->pathString = NULL;
		return CVM_FALSE;
	    }
	    /* Start at the top */
	    idx = 0;
	} else {
	    /* We are appending ... */
	    CVMUint32 numEntries;
	    CVMUint32 origNumEntries;	 
	    CVMClassPathEntry* origEntries;
	    char* newPathString;

	    pathStr = additionalPathString;
	    /* We are appending. Check if there is anything to append */
	    if ((pathStr == NULL) || (*pathStr == '\0')) {
		/* Nothing to do */
		return doNotFailWhenPathNotFound;
	    }
	    
	    numEntries = getNumPathComponents(pathStr);

	    origNumEntries = classPath->numEntries;
	    origEntries = classPath->entries;
    
	    /* We will be appending... */
	    idx = origNumEntries;
	    
	    /* ... this many components */
	    classPath->numEntries += numEntries;

	    /* allocate space for all of the classpath entries */
	    classPath->entries = (CVMClassPathEntry*)
		malloc(classPath->numEntries * sizeof(CVMClassPathEntry));
	    if (classPath->entries == NULL) {
		return CVM_FALSE;
	    }

	    /* And copy those already initialized */
	    memmove(classPath->entries, origEntries, 
		    origNumEntries * sizeof(CVMClassPathEntry));
	    free(origEntries);
	    /* Finally, make up the resulting pathString */
	    newPathString = 
		(char*)malloc(strlen(classPath->pathString) + 
			      strlen(additionalPathString) +
			      strlen(CVM_PATH_CLASSPATH_SEPARATOR) +
			      1);
	    *newPathString = '\0';
	    strcat(newPathString, classPath->pathString);
	    strcat(newPathString, CVM_PATH_CLASSPATH_SEPARATOR);
	    strcat(newPathString, additionalPathString);
	    classPath->pathString = newPathString;
	}
    }
    
    /* initialize the classpath entries */
    {
        /* 
         * Duplicate the string pointed by classPath->pathString, so
         * it won't get corrupted during the process.
         */
        char* firstPath = strdup(pathStr);
	char* currPath = firstPath; 
	char* nextPath;
        CVMBool hasExtInfo = CVM_FALSE;
	while (currPath != NULL) {
	    CVMInt32 fileType;
	    CVMClassPathEntry* currEntry = &classPath->entries[idx];

	    currEntry->javaInitialized = CVM_FALSE;
	    
	    if (initJavaSide) {
		CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
                currEntry->codeSource = CVMID_getGlobalRoot(ee);
                if (currEntry->codeSource == NULL) {
                    free(firstPath);
                    return CVM_FALSE;
                }
		currEntry->protectionDomain = CVMID_getGlobalRoot(ee);
		if (currEntry->protectionDomain == NULL) {
                    free(firstPath);
		    return CVM_FALSE;
		}
		currEntry->url = CVMID_getGlobalRoot(ee);
		if (currEntry->url == NULL) {
                    free(firstPath);
		    return CVM_FALSE;
		}
		currEntry->jfile = CVMID_getGlobalRoot(ee);
		if (currEntry->jfile == NULL) {
                    free(firstPath);
		    return CVM_FALSE;
		}
	    } else {
                currEntry->codeSource = NULL;
		currEntry->protectionDomain = NULL;
		currEntry->url = NULL;
		currEntry->jfile = NULL;
	    }
   
            currEntry->isSigned = CVM_FALSE;
	    
	    /* 
	     * Find the start of the next class path item and null terminate
	     * the current class path item.
	     */
	    nextPath = strstr(currPath, CVM_PATH_CLASSPATH_SEPARATOR);
	    if (nextPath != NULL) {
		*nextPath = '\0';
		nextPath += strlen(CVM_PATH_CLASSPATH_SEPARATOR);
	    }

	    /* The empty string is the same as the current directory. */
	    if (*currPath == '\0') {
		currPath = CVM_PATH_CURDIR;
	    }

	    /* Find out what the file type of the class path item is. */
	    fileType = CVMioFileType(currPath);
	    currEntry->path = NULL; /* make error cleanup easier */
	    if (fileType < 0) {
		/* File or directory does not exist */
		CVMdebugPrintf(("\"%s\" is not a valid "
				"classpath zip file or directory.\n",
				currPath));
		currEntry->type = CVM_CPE_INVALID;
	    } else if (fileType == CVM_IO_FILETYPE_DIRECTORY) {
		/* it's a directory */
		CVMtraceClassLoading(("CL: Added directory \"%s\" to the "
				      "classpath\n", currPath));
		currEntry->type = CVM_CPE_DIR;
		currEntry->path = strdup(currPath);
	    } else if (fileType == CVM_IO_FILETYPE_REGULAR) {
		/* it's a zip file */
                char canonicalPath[CVM_PATH_MAXLEN];
		char* msg = NULL;
		jzfile* zip;

		if (CVMcanonicalize(currPath, canonicalPath, CVM_PATH_MAXLEN)
		    < 0) {
		    CVMdebugPrintf(("Bad classpath name: \"%s\"\n",
				    currPath));
                    free(firstPath);
		    return CVM_FALSE;
		}
 
		zip = ZIP_Open(canonicalPath, &msg);
		if (zip == NULL) {
		    CVMdebugPrintf(("Opening classpath zip file "
				    "\"%s\" failed.\n", currPath));
		    currEntry->type = CVM_CPE_INVALID;
		} else {
		    CVMtraceClassLoading(("CL: Added zip file \"%s\" to the "
					  "classpath\n", currPath));
		    currEntry->type = CVM_CPE_ZIP;
                    /* canonical form *is* needed (6669683) */
		    currEntry->path = strdup(canonicalPath);
		    currEntry->zip = zip;
                    /* For classpath entries only */
                    if (initJavaSide) {

/* Java SE Doesn't support this ZIP function
   FIXME - need to determine where jar file signing occurs in SE and
   make sure that wearen't disabling some security check with this change.
*/
#ifndef JAVASE
                        if (ZIP_IsSigned(currEntry->zip)) {
                            currEntry->isSigned = CVM_TRUE;
                        }
#endif
                        if (!hasExtInfo) {
/* Java SE Doesn't support this ZIP function.
   FIXME - see comment above
*/
#ifndef JAVASE
                            if (ZIP_HasExtInfo(currEntry->zip)) {
                                CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
                                jclass appclClass; 
                                jmethodID mid;
                                appclClass = CVMcbJavaInstance(CVMsystemClass(
                                    sun_misc_Launcher_AppClassLoader));
                                mid = CVMglobals.
                                    sun_misc_Launcher_AppClassLoader_setExtInfo;
                                if (CVMclassInit(ee, CVMsystemClass(
                                        sun_misc_Launcher_AppClassLoader))) {
                                    (*env)->CallStaticVoidMethod(
                                        env, appclClass, mid);
                                }
                                if ((*env)->ExceptionOccurred(env)) {
                                    free(firstPath);
                                    return CVM_FALSE;
                                }
                                hasExtInfo = CVM_TRUE; 
                            }
#endif
                        }
                    }
		}
	    } else {
		/* Neither a file nor a directory, so just ignore it */
		CVMdebugPrintf(("\"%s\" is not a valid "
				"classpath zip file or directory.\n",
				currPath));
		currEntry->type = CVM_CPE_INVALID;
	    }

	    /* on to the next class path item. */
	    idx++;
	    currPath = nextPath;
	}
        free(firstPath);
    }

    CVMassert(idx == classPath->numEntries);
    return CVM_TRUE;
}

static void
freePathComponents(CVMExecEnv* ee, CVMClassPath* path)
{
    int idx;
    for (idx = 0; idx < path->numEntries; idx++) {
	CVMClassPathEntry* currEntry = &path->entries[idx];
	
	if (currEntry->type == CVM_CPE_ZIP) {
	    jzfile* zip = currEntry->zip;
	    CVMtraceClassLoading(("CL: Closing boot class path entry %s\n",
				  currEntry->path));
	    ZIP_Close(zip);
	}
	if (currEntry->path != NULL) {
	    free(currEntry->path);
	}
        if (currEntry->codeSource != NULL) {
            CVMID_freeGlobalRoot(ee, currEntry->codeSource);
            currEntry->codeSource = NULL;
        }
	if (currEntry->protectionDomain != NULL) {
	    CVMID_freeGlobalRoot(ee, currEntry->protectionDomain);
	    currEntry->protectionDomain = NULL;
	}
        if (currEntry->url != NULL) {
            CVMID_freeGlobalRoot(ee, currEntry->url);
            currEntry->url = NULL;
        }
        if (currEntry->jfile != NULL) {
            CVMID_freeGlobalRoot(ee, currEntry->jfile);
            currEntry->jfile = NULL;
        }
    }
}

static void
classPathDestroy(CVMExecEnv* ee, CVMClassPath* path) 
{
    /* 
     * Note the pathString is freed in 
     * CVMdestroyVMGlobalState()
     */

    if (path->entries != NULL) {
	freePathComponents(ee, path);
	free(path->entries);
    }
}

void
CVMclassBootClassPathDestroy(CVMExecEnv* ee) 
{
    classPathDestroy(ee, &CVMglobals.bootClassPath);
}

void
CVMclassClassPathDestroy(CVMExecEnv* ee) 
{
    classPathDestroy(ee, &CVMglobals.appClassPath);
}

#ifdef CVM_DUAL_STACK
/* 
 * Check if the classloader is one of the MIDP dual-stack classloaders.
 */
#include "generated/offsets/sun_misc_MIDletClassLoader.h"

CVMBool
CVMclassloaderIsMIDPClassLoader(CVMExecEnv *ee,
                                CVMClassLoaderICell* loaderICell,
                                CVMBool checkImplClassLoader)
{
    if (loaderICell != NULL) {
        CVMClassBlock* loaderCB = CVMobjectGetClass(
                                  CVMID_icellDirect(ee, loaderICell));
        CVMClassTypeID loaderID = CVMcbClassName(loaderCB);

        if (loaderID == CVMglobals.midletClassLoaderTid) {
            if (checkImplClassLoader) {
                return CVM_TRUE;
            } else {
                /* Make sure this is a real MIDletClassLoader with
                   filtering enabled. */
                CVMBool enableFilter;
                CVMD_fieldReadInt(
                    CVMID_icellDirect(ee, loaderICell),
                    CVMoffsetOfsun_misc_MIDletClassLoader_enableFilter,
                    enableFilter);
                return enableFilter;
            }
        } else if (checkImplClassLoader) {
            if (loaderID == CVMglobals.midpImplClassLoaderTid) {
	        return CVM_TRUE;
	    }
            return CVM_FALSE;
        }
    }
    return CVM_FALSE;
}
#endif

#endif /* CVM_CLASSLOADING */
