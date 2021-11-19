/*
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

/*=========================================================================
 * SYSTEM:    Verifier
 * SUBSYSTEM: class loader.
 * FILE:      classloader.c
 * OVERVIEW:  Routines for loading and resolving class definitions.
 *            These routines should not be depending upon the interpreter
 *            or the garbage collector.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <setjmp.h>

#include <jar.h>
#include <oobj.h>
#include <path.h>
#include <tree.h>
#include <signature.h>
#include <convert_md.h> 

#include <sys_api.h>

#ifdef UNIX
#include <unistd.h>
#endif

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

char *stat_source(ClassClass *cb, struct stat *s, char *pathbuf, int maxlen);

extern ClassClass *allocClassClass();

extern bool_t JARfile;

extern char * zipFileName;

extern zip_t * getZipEntry(char *zipFile, int len);

extern bool_t findJARDirectories(zip_t *entry, struct stat *statbuf);

extern JAR_DataStreamPtr loadJARfile(zip_t *entry, const char* filename);

/*=========================================================================
 * FUNCTION:      AddBinClass 
 * OVERVIEW:      Used by createInternalClass1() and createFakeArrayClass()
 *                to add a class in the class table.
 * INTERFACE:
 *   parameters:  ClassClass: cb
 *
 *   returns:     nothing 
 *=======================================================================*/
void
AddBinClass(ClassClass * cb)
{
    register int left, right, middle, result, i;
    char *name = cbName(cb);
    struct Hjava_lang_ClassLoader *loader = cbLoader(cb);

    BINCLASS_LOCK();
    left = 0;
    right = nbinclasses - 1;
    result = 1;
    while (left <= right) {
        ClassClass *cb1;
        middle = (left+right)/2;
        cb1 = binclasses[middle];
        result = strcmp(name, cbName(cb1));
        if (result == 0) {
            if (loader < cbLoader(cb1)) {
                result = -1;
            } else if (loader > cbLoader(cb1)) {
                result = 1;
            } else {
                result = 0;
            }
        }
        if (result < 0) {
            right = middle-1;
        } else if (result > 0) {
            left = middle+1;
        } else {
            break;
        }
    }
    if (result != 0) {
        if (nbinclasses >= sizebinclasses) {
            if (binclasses == 0)
                binclasses = (ClassClass **)
                sysMalloc(sizeof(ClassClass *) * (sizebinclasses = 50));
            else
                binclasses = (ClassClass **)
                sysRealloc(binclasses, sizeof(ClassClass *)
                    * (sizebinclasses = nbinclasses * 2));
        }
        if (binclasses == 0)
            goto unlock;
        right++;
        for (i = nbinclasses; i > right; i--) {
            binclasses[i] = binclasses[i-1];
        }
        binclasses[right] = cb;
        nbinclasses++;
    }

unlock:
    BINCLASS_UNLOCK();
}

/*=========================================================================
 * FUNCTION:      DelBinClass 
 * OVERVIEW:      Intended for allowing deletion of classes from the class 
 *                table.  
 * 
 * INTERFACE:
 *   parameters:  ClassClass: cb
 *
 *   returns:     nothing 
 *=======================================================================*/
void
DelBinClass(ClassClass * cb)
{
    register int i, j;
    BINCLASS_LOCK();
    for (i = nbinclasses; --i >= 0; )
        if (binclasses[i] == cb) {
            nbinclasses--;
            for (j = i; j < nbinclasses; j++) {
                binclasses[j] = binclasses[j+1];
            }
            break;
        }
    BINCLASS_UNLOCK();
}

/*=========================================================================
 * FUNCTION:      MakeClassSticky
 * OVERVIEW:      Used to lock certain system classes into memory during
 *                initialization. 
 * 
 * INTERFACE:
 *   parameters:  ClassClass: cb
 *
 *   returns:     nothing 
 *=======================================================================*/
void
MakeClassSticky(ClassClass *cb)
{
    /* monitorEnter(obj_monitor(cb));   */
    CCSet(cb, Sticky);
    /* monitorExit(obj_monitor(cb));    */
}

/*=========================================================================
 * FUNCTION:      LoadClassFromFile
 * OVERVIEW:      Loads a .class file normally from disk or a null if it fails.
 *                When the interpreter requests for a file, it is actually
 *                looking for a classblock structure to be created, and the
 *                only way it can get one of those is by loading a compiled
 *                class. 
 *                OpenCode() tries to open a .class file first. If it fails 
 *                to open the file, it returns a non-zero status. If it 
 *                returns a valid file descriptor, this usually means that 
 *                this is a valid .class file.
 *                It then invokes createInternalClass() to actually create the
 *                internal representation of the class. 
 * 
 * INTERFACE:
 *   parameters:  char*: file name
 *                char*: directory
 *                char*: class name
 *
 *   returns:     nothing 
 *=======================================================================*/
static ClassClass *
LoadClassFromFile(char *fn, char *dir, char *class_name)
{
    extern int OpenCode(char *, char *, char *, struct stat*);
    struct stat st;
    ClassClass *cb = 0;
    int codefd = -1;
    unsigned char *external_class;
    char *detail;

    codefd = OpenCode(fn, NULL, dir, &st);

    if (codefd < 0)     /* open failed */
        return 0;

    /* Snarf the file into memory. */
    external_class = (unsigned char *)sysMalloc(st.st_size);
    if (external_class == 0)
        goto failed;
    if (sysRead(codefd, external_class, st.st_size) != st.st_size)
        goto failed;
    sysClose(codefd);
    codefd = -1;

    /* Create the internal class */
    cb = allocClassClass();
    if (cb == NULL ||
        !createInternalClass(external_class, external_class + st.st_size,
                 cb, NULL, class_name, &detail)) {
        sysFree(external_class);
        goto failed;
    }
    sysFree(external_class);

    if (verbose)
        jio_fprintf(stderr, "[Loaded %s]\n", fn);

    return cb;
failed:
    if (codefd >= 0)
        sysClose(codefd);
    if (cb != 0)
        FreeClass(cb);
    return 0;
}

/*=========================================================================
 * FUNCTION:      LoadClassFromZip
 * TYPE:          load class from a JAR or Zip file
 * OVERVIEW:      Called by LoadClassLocally for loading classes from a Zip.
 *
 *                This function loads a .class file normally from a Zip or 
 *                JAR file.
 *                It returns the class when it succeeds or NULL if it fails.
 *
 * INTERFACE:
 *   parameters:  zip_t: zip file entry
 *                char*: class file name to search for loading
 *   returns:     Pointer to ClassClass when it succeeds, or NULL if it fails.
 *=======================================================================*/
static ClassClass *
LoadClassFromZip(zip_t *zipEntry, char *class_name)
{
    ClassClass *cb = 0;
    JAR_DataStreamPtr jdstream = NULL;
    unsigned char *external_class;
    int data_length;
    char *detail;

    jdstream = loadJARfile (zipEntry, class_name);

    if (jdstream == NULL)
        goto failed;

    external_class = jdstream->data;
    data_length = jdstream->dataLen;

    /* Create the internal class */
    cb = allocClassClass();
    if (cb == NULL ||
        !createInternalClass(external_class, external_class + data_length,
                 cb, NULL, class_name, &detail)) {
        goto failed;
    }
    if (jdstream != NULL) {
        freeBytes(jdstream);
    }

    if (verbose) {
        jio_fprintf(stderr, "[Loaded %s] from [%s]\n", class_name,
                                                       zipEntry->name);
    }
    return cb;
failed:
    if (cb != 0)
        FreeClass(cb);
    if (jdstream != NULL) {
        freeBytes(jdstream);
    }
    return 0;
}

/*=========================================================================
 * FUNCTION:      LoadClassLocally
 * OVERVIEW:      Find a class file that is somewhere local, and not from a
 *                class loader.
 *                It still needs to be searched using the classpath.
 *
 * INTERFACE:
 *   parameters:  char* : class file name 
 *   returns:     Pointer to ClassClass when it succeeds, or NULL if it fails.
 *=======================================================================*/
ClassClass *LoadClassLocally(char *name)
{
    ClassClass *cb = 0;
    cpe_t **cpp;

    if (name[0] == DIR_SEPARATOR || name[0] == SIGNATURE_ARRAY)
        return 0;

    for (cpp = sysGetClassPath(); cpp && *cpp != 0; cpp++) {
        cpe_t *cpe = *cpp;
        char *path;

        if (cpe->type == CPE_DIR) {
            path = (char *)sysMalloc(strlen(cpe->u.dir)
                + sizeof(LOCAL_DIR_SEPARATOR)
                + strlen(name)
                + strlen(JAVAOBJEXT)
                + 2);  /* 2 is for the . and the \0 */
            if (sprintf(path, 
                 "%s%c%s." JAVAOBJEXT, cpe->u.dir,
                 LOCAL_DIR_SEPARATOR, name) == -1) {
                sysFree(path);
                return 0;
            }
            if ((cb = LoadClassFromFile(sysNativePath(path), 
                                        cpe->u.dir, name))) {
                sysFree(path);
                return cb;
            }
        } else if (cpe->type == CPE_ZIP) {
            if (JAR_DEBUG && verbose)
                jio_fprintf(stderr, "Loading classes from a ZIP file... \n");
            if ((cb = LoadClassFromZip(cpe->u.zip, name))) {
                return cb;
            }
        }

    }
    return cb;
}

/*=========================================================================
 * FUNCTION:      stat_source
 * OVERVIEW:      Unused by the Verifier, but it is too late to remove it now. 
 *
 * INTERFACE:
 *   parameters:  ClassClass *: cb
 *                struct stat*: s 
 *                char*: pathbuf
 *                int: maxlen 
 *   returns:     char *, or NULL if it fails.
 *=======================================================================*/
char *
stat_source(ClassClass *cb, struct stat *s, char *pathbuf, int maxlen)
{
#define NAMEBUFLEN 255
    char nm[NAMEBUFLEN];
    char *p, *q, *lp;
    cpe_t **cpp;

    /* don't bother searching if absolute */
    /* IMPL_NOTE: only here for compatibility */
    if (sysIsAbsolute(cbSourceName(cb))) {
        if (sysStat(cbSourceName(cb), s) == 0) {
            if (jio_snprintf(pathbuf, maxlen, "%s", cbSourceName(cb)) == -1) {
                return 0;
            }
            return pathbuf;
        } else {
            return 0;
        }
    }

    /* parse the package name */
    p = cbName(cb);
    if (strlen(p) > NAMEBUFLEN - 1) {
        return 0;
    }
    for (q = lp = nm ; *p ; p++) {
        if (*p == DIR_SEPARATOR) {
            *q++ = LOCAL_DIR_SEPARATOR;
            lp = q;
        } else {
            *q++ = *p;
        }
    }

    /* append the source file name */
    p = cbSourceName(cb);
    if (strlen(p) + (lp - nm) > NAMEBUFLEN - 1) {
        return 0;
    }
    for (; *p ; p++) {
        *lp++ = (*p == DIR_SEPARATOR ? LOCAL_DIR_SEPARATOR : *p);
    }
    *lp = '\0';

    /* search the class path */
    for (cpp = sysGetClassPath() ; cpp && *cpp != 0 ; cpp++) {
        cpe_t *cpe = *cpp;
        if (cpe->type == CPE_DIR) {
            if (jio_snprintf(pathbuf, maxlen, "%s%c%s",
                             cpe->u.dir, LOCAL_DIR_SEPARATOR, nm) == -1) {
                return 0;
            }
            if (sysStat(pathbuf, s) == 0) {
                return pathbuf;
            }
        }
    }
    return 0;
}

/*=========================================================================
 * Globals and externs for Internal Class representation 
 *=======================================================================*/
struct CICmallocs {
    struct CICmallocs *next;
    void * alignment_padding;

/* Whatever follows will be 8-byte aligned, if the structure itself is
   8-byte aligned. */
};

typedef struct CICmallocs CICmallocs;

struct CICcontext {
    unsigned char *ptr;
    unsigned char *end_ptr;
    ClassClass *cb;
    jmp_buf jump_buffer;
    char **detail;

    int pass;         /* two passes, 1 or 2 */
    int malloc_size;  /* space needed for everything other than <clinit> */
    int clinit_size;  /* space needed for the <clinit> method */
    int in_clinit;    /* indicates whether we are loading <clinit> method */

    struct {
        CICmallocs *mallocs; /* list of memory blocks used in the first pass */

        void * alignment_padding;
                             /* Whatever follows will be 8-byte aligned */
    } pass1;

    struct {
        char *malloc_buffer; /* used to hold everything other than <clinit> */
        char *malloc_ptr;    /* current point of allocation */

        char *clinit_buffer; /* used to hold the <clinit> method */
        char *clinit_ptr;    /* current point of allocation */
    } pass2;

};

typedef struct CICcontext CICcontext;

static char *getAsciz(CICcontext *, bool_t);
static char *getAscizFromClass(CICcontext *, int i);

static unsigned char get1byte(CICcontext *);
static unsigned short get2bytes(CICcontext *);
static unsigned long get4bytes(CICcontext *);
static void getNbytes(CICcontext *, int count, char *buffer);
static void *allocNBytes(CICcontext *, int size);
static void freeBuffers(CICcontext *);

static void LoadConstantPool(CICcontext *);
static void verifyUTF8String(CICcontext *context, char* bytes,
                             unsigned short length);

static void ReadInCode(CICcontext *, struct methodblock *);
static void ReadLineTable(CICcontext *, struct methodblock *mb);
static void ReadExceptions(CICcontext *, struct methodblock *);
static void ReadLocalVars(CICcontext *, struct methodblock *mb);

static void
createInternalClass0(CICcontext *context, ClassClass *cb,
             struct Hjava_lang_ClassLoader *loader, char *name);

bool_t
createInternalClass1(unsigned char *ptr, unsigned char *end_ptr,
             ClassClass *cb, struct Hjava_lang_ClassLoader *loader,
             char *name, char **detail);

/*=========================================================================
 * FUNCTION:      createInternalClass 
 * OVERVIEW:      Invoked by LoadClassFromFile() or LoadClassFromZip() to 
 *                create an internal class. See createInternalClass1() for
 *                details. 
 * 
 * INTERFACE:
 *   parameters:  unsigned char *: ptr
 *                unsigned char *: end_ptr
 *                ClassClass *: cb
 *                struct Hjava_lang_ClassLoader *: loader
 *                char *: name
 *                char **: detail 
 *
 *   returns:     bool_t
 *=======================================================================*/
bool_t
createInternalClass(unsigned char *ptr, unsigned char *end_ptr,
            ClassClass *cb, struct Hjava_lang_ClassLoader *loader,
            char *name, char **detail)
{
    bool_t res;
    res = createInternalClass1(ptr, end_ptr, cb, loader, name, detail);

    return res;
}

/*=========================================================================
 * FUNCTION:      JAVA_ERROR 
 * OVERVIEW:      Verifier error processing function. 
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: context
 *                char *      : name 
 *
 *   returns:     nothing 
 *=======================================================================*/
void JAVA_ERROR(CICcontext *context, char *name) {
    printCurrentClassName();
    *(context->detail) = name;
    EE()->class_loading_msg = name;
    fprintf(stderr, "Class loading error: %s\n", name);
    exit(1);
}

/*=========================================================================
 * FUNCTION:      createInternalClass1 
 * OVERVIEW:      Auxiliary function for creating an internal class.
 *                Invoked by createInternalClass().
 * 
 *                Creates an internal class file from the indicated data.  
 *                It should be in a buffer for which the first byte is at 
 *                *ptr and the last byte is just before *end_ptr.
 *                The class's classloader is indicated by the classloader 
 *                argument.
 *
 *                We go through the buffer twice. In the first pass, we 
 *                determine the amount of storage needed by the class. 
 *                We then allocate a single chunk of memory, free the 
 *                temporary storage, and load from the buffer for the second
 *                time.
 *
 *                Since all storage needed by the class initialization method 
 *                <clinit> can be freed after the class is loaded, we count 
 *                the <clinit> space needs separately and store the <clinit> 
 *                method in a separate chunk of memory.
 * 
 * INTERFACE:
 *   parameters:  unsigned char *: ptr
 *                unsigned char *: end_ptr
 *                ClassClass *: cb
 *                struct Hjava_lang_ClassLoader *: loader
 *                char *: name
 *                char **: detail 
 *
 *   returns:     bool_t
 *=======================================================================*/
bool_t
createInternalClass1(unsigned char *ptr, unsigned char *end_ptr,
             ClassClass *cb, struct Hjava_lang_ClassLoader *loader,
             char *name, char **detail)
{
    struct CICcontext context_block;
    struct CICcontext *context = &context_block;

    /* Set up the context */
    context->ptr = ptr;
    context->end_ptr = end_ptr;
    context->cb = cb;
    context->detail = detail;

    /* initialize the remaining fields of the context block */
    context->pass = 0;
    context->malloc_size = 0;
    context->clinit_size = 0;
    context->in_clinit = 0;
    context->pass1.mallocs = 0;

    context->pass2.malloc_buffer = NULL;
    context->pass2.malloc_ptr = NULL;
    context->pass2.clinit_buffer = NULL;
    context->pass2.clinit_ptr = NULL;

    if (setjmp(context->jump_buffer)) {

        /* We've gotten an error of some sort 
         * See comments below about zeroing these 
         * two fields before freeing the temporary 
         * buffer.
         */

        cbConstantPool(cb) = NULL;
        cbFields(cb) = NULL;

        /* Zero out the method so that freeClass will
         * not try to free the clinit method.
         */

        cbMethodsCount(cb) = 0;
        freeBuffers(context);
        return FALSE;
    }

    /* The first pass allows us to uncover any class format 
     * errors and find out the size of the buffer needed. 
     */

    context->pass = 1;
    createInternalClass0(context, cb, loader, name);

    /* We must set the following two fields to zero before we free
     * the temporary buffers, because markClassClass may scan a
     * partially constructed class block in the second pass.
     * If these two fields are set to zero, markClassClass will
     * not scan the constant pool and field blocks, which may
     * point to freed memory.
     */

    cbConstantPool(cb) = NULL;
    cbFields(cb) = NULL;

    /* Zero out the method so that freeClass will not try
     * to free the clinit method.
     */

    cbMethodsCount(cb) = 0;
    freeBuffers(context);

    context->ptr = ptr;   /* rewind the raw class data */

    if (context->malloc_size > 0) {
        context->pass2.malloc_buffer = 
                   (char *)sysCalloc(1, context->malloc_size);

        if (context->pass2.malloc_buffer == 0)
            JAVA_ERROR(context, "out of memory");
    }

    if (context->clinit_size > 0) {
        context->pass2.clinit_buffer = 
                   (char *)sysCalloc(1, context->clinit_size * sizeof(char));

        if (context->pass2.clinit_buffer == 0) {
            sysFree(context->pass2.malloc_buffer);
            JAVA_ERROR(context, "out of memory");
        }
    }

    context->pass2.malloc_ptr = context->pass2.malloc_buffer;
    context->pass2.clinit_ptr = context->pass2.clinit_buffer;

    /* The second pass accomplishes the real task. */

    context->pass = 2;
    createInternalClass0(context, cb, loader, name);

    /* Valid class - let's put it in the class table. */
    AddBinClass(cb);

    return TRUE;
}

/*=========================================================================
 * FUNCTION:      createInternalClass0 
 * OVERVIEW:      Auxiliary function invoked by createInternalClass1() during
 *                the second pass to actually load the internal class file
 *                structures such as the constant pool, method blocks, field
 *                blocks and so on.
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: ptr
 *                ClassClass *: cb
 *                struct Hjava_lang_ClassLoader *: loader
 *                char *: name
 *
 *   returns:     nothing 
 *=======================================================================*/
static void
createInternalClass0(CICcontext *context, ClassClass *cb,
             struct Hjava_lang_ClassLoader *loader, char *name)
{
    int i, j, len;
    char buff[BUFSIZ];
    char *UTFname = &buff[0];
    union cp_item_type *constant_pool;
    unsigned char *type_table;
    int attribute_count;
    unsigned fields_count;
    struct methodblock *mb;
    struct fieldblock *fb;
    struct Classjava_lang_Class *ucb = unhand(cb);
    bool_t have_inner_classes;

    if (get4bytes(context) != JAVA_CLASSFILE_MAGIC)
        JAVA_ERROR(context, "Bad magic number");

    ucb->minor_version = get2bytes(context);
    ucb->major_version = get2bytes(context);
    ucb->loader = loader;
 
    /* Ignore version # so that the preverifier can work with JDK 1.4 onwards */
    /*
    if (ucb->major_version != JAVA_VERSION)
        JAVA_ERROR(context, "Bad major version number");
    */

    LoadConstantPool(context);
    constant_pool = ucb->constantpool;
    type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;

    ucb->access = get2bytes(context) & ACC_WRITTEN_FLAGS;

    /* Get the name of the class */
    i = get2bytes(context); /* index in constant pool of class */
    ucb->name = getAscizFromClass(context, i);

    /* Conversion for Japanese filenames */
    len = native2utf8(name, UTFname, BUFSIZ);

    if (name != NULL && strcmp(ucb->name, UTFname) != 0)
        JAVA_ERROR(context, "Wrong name");
    constant_pool[i].clazz = cb;
    CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);

    if (loader) {
        /* We don't trust a classloader to do the right thing. . . */
        ClassClass **pcb, **end_pcb;
        char *name = ucb->name;
        if (name == NULL || !IsLegalClassname(name, FALSE)) {
            JAVA_ERROR(context, "Bad name");
        }
        BINCLASS_LOCK();
        for (pcb = binclasses, end_pcb = pcb + nbinclasses;
                               pcb < end_pcb; pcb++) {
            ClassClass *cb = *pcb;
            if ((cbLoader(cb) == loader) && (strcmp(name, cbName(cb)) == 0))
                break;
        }
        BINCLASS_UNLOCK();
        if (pcb < end_pcb)
            /* There's already a class with the same name and loader */
            JAVA_ERROR(context, "Duplicate name");
    }

    /* Get the super class name. */
    i = get2bytes(context); /* index in constant pool of class */
    if (i > 0) {
        ucb->super_name = getAscizFromClass(context, i);
        if (!IsLegalClassname(ucb->super_name, FALSE)) {
            JAVA_ERROR(context, "Bad superclass name");
        }
    }

    i = ucb->implements_count = get2bytes(context);
    if (i > 0) {
        int j;
        ucb->implements = allocNBytes(context, i * sizeof(short));
        for (j = 0; j < i; j++) {
            ucb->implements[j] = get2bytes(context);
        }
    }

    fields_count = ucb->fields_count = get2bytes(context);
    if (fields_count > 0)
        ucb->fields = (struct fieldblock *)
          allocNBytes(context, ucb->fields_count * sizeof(struct fieldblock));
    for (i = fields_count, fb = ucb->fields; --i >= 0; fb++) {
        bool_t has_constant_value = FALSE;
        fieldclass(fb) = cb;
        fb->access = get2bytes(context) & ACC_WRITTEN_FLAGS;
        fb->name = getAsciz(context, FALSE);
        fb->signature = getAsciz(context, FALSE);
        attribute_count = get2bytes(context);
        for (j = 0; j < (int)attribute_count; j++) {
            char *attr_name = getAsciz(context, FALSE);
            int length = get4bytes(context);
            if (strcmp(attr_name, "ConstantValue") == 0) {
                if (fb->access & ACC_STATIC) {
                    if (length != 2) {
                        JAVA_ERROR(context, "Wrong size for VALUE attribute");
                    } 
                    if (has_constant_value) { 
                        JAVA_ERROR(context,
                                   "Duplicate ConstantValue attribute");
                    }
                    has_constant_value = TRUE;
                    fb->access |= ACC_VALKNOWN;
                    /* we'll change this below */
                    fb->u.offset = get2bytes(context);
                } else {
                    getNbytes(context, length, NULL);
                }
            } else if (strcmp(attr_name, "Deprecated") == 0) {
                if (length > 0) {
                    JAVA_ERROR(context, "Bad deprecated size");
                }
                fb->deprecated = TRUE;
            } else if (strcmp(attr_name, "Synthetic") == 0) {
                if (length > 0) {
                    JAVA_ERROR(context, "Bad synthetic attribute size");
                }
                fb->synthetic = TRUE;
            } else {
                getNbytes(context, length, NULL);
            }
        }
        /*
        if (fb->access & ACC_STATIC) {
            InitializeStaticVar(fb, context);
        }
        */
    }

    if ((ucb->methods_count = get2bytes(context)) > 0)
        ucb->methods = (struct methodblock *)
          allocNBytes(context, ucb->methods_count * sizeof(struct methodblock));

    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) {
        bool_t have_exceptions = FALSE;
        fieldclass(&mb->fb) = cb;
        mb->fb.access = get2bytes(context) & ACC_WRITTEN_FLAGS;
        mb->fb.name = getAsciz(context, FALSE);
        mb->fb.signature = getAsciz(context, FALSE);

        if (strcmp(mb->fb.name, "<clinit>") == 0 &&
            strcmp(mb->fb.signature, "()V") == 0)
            context->in_clinit = TRUE;

        mb->args_size = Signature2ArgsSize(mb->fb.signature)
                    + ((mb->fb.access & ACC_STATIC) ? 0 : 1);
        if (mb->args_size > 255)
            JAVA_ERROR(context, "Too many arguments");

        attribute_count = get2bytes(context);
        for (j = 0; j < attribute_count; j++) {
            char *attr_name = getAsciz(context, FALSE);
            if (strcmp(attr_name, "Code") == 0) {
                if  (((mb->fb.access & (ACC_NATIVE | ACC_ABSTRACT))==0) 
                      || (strcmp(mb->fb.name, "<clinit>") == 0)) {
                    ReadInCode(context, mb);
                } else { 
                    JAVA_ERROR(context, "Abstract and native methods cannot "
                                        "have code");
                }
            } else if (strcmp(attr_name, "Exceptions") == 0) {
                if (have_exceptions) { 
                    JAVA_ERROR(context, "Multiple Exceptions attribute");
                } else { 
                    ReadExceptions(context, mb);
                    have_exceptions = TRUE;
                }
            } else {
                int length = get4bytes(context);
                if (strcmp(attr_name, "Deprecated") == 0) {
                    if (length > 0) {
                        JAVA_ERROR(context, "Bad deprecated size");
                    }
                    mb->fb.deprecated = TRUE;
                } else if (strcmp(attr_name, "Synthetic") == 0) {
                    if (length > 0) {
                        JAVA_ERROR(context, "Bad synthetic attribute size");
                    }
                    mb->fb.synthetic = TRUE;
                } else {
                    getNbytes(context, length, NULL);
                }
            }
        }
        context->in_clinit = FALSE;
    }

    /* See if there are class attributes */
    attribute_count = get2bytes(context);
    have_inner_classes = FALSE;
    for (j = 0; j < attribute_count; j++) {
        char *attr_name = getAsciz(context, FALSE);
        int length = get4bytes(context);
        if (strcmp(attr_name, "SourceFile") == 0) {
            if (length != 2) {
                JAVA_ERROR(context, "Wrong size for VALUE attribute");
            }
            ucb->source_name = getAsciz(context, FALSE);
        } else if (strcmp(attr_name, "AbsoluteSourcePath") == 0) {
            if (length == 2) {
                ucb->absolute_source_name = getAsciz(context, FALSE);
            } else
            getNbytes(context, length, NULL);
        } else if (strcmp(attr_name, "TimeStamp") == 0) {
            if (length == 8) {
                ucb->hasTimeStamp = TRUE;
                ucb->timestamp.high = get4bytes(context);
                ucb->timestamp.low = get4bytes(context);
            } else {
                getNbytes(context, length, NULL);
            }
        } else if (strcmp(attr_name, "Deprecated") == 0) {
            if (length > 0) {
                JAVA_ERROR(context, "Bad deprecated size");
            }
            ucb->deprecated = TRUE;
        } else if (strcmp(attr_name, "Synthetic") == 0) {
            if (length > 0) {
                JAVA_ERROR(context, "Bad synthetic attribute size");
            }
            ucb->synthetic = TRUE;
        } else if (strcmp(attr_name, "InnerClasses") == 0) {
            int count = get2bytes(context);
            struct innerClasses *thisInnerClass = (struct innerClasses *)
                 allocNBytes(context, count * sizeof(struct innerClasses));
            struct innerClasses *lastInnerClass = thisInnerClass + count;

            if (count * 8 + 2 != length) {
                JAVA_ERROR(context, "Bad length of InnerClasses attribute");
            }
            
            if (have_inner_classes) {
                JAVA_ERROR(context, "Duplicate InnerClasses attribute");
            }
            have_inner_classes = TRUE;

            ucb->inner_classes_count = count;
            ucb->inner_classes = thisInnerClass;

            for ( ; thisInnerClass < lastInnerClass; thisInnerClass++) {
                thisInnerClass->inner_class = get2bytes(context);
                thisInnerClass->outer_class = get2bytes(context);
                thisInnerClass->inner_name = getAsciz(context, TRUE);
                thisInnerClass->access = get2bytes(context);
                if (thisInnerClass->inner_class != 0) {
                    /* Make sure that inner_class really belongs to a CLASS */
                    getAscizFromClass(context, thisInnerClass->inner_class);
                }
                if (thisInnerClass->outer_class != 0) {
                    /* Make sure that outer_class really belongs to a CLASS */
                    getAscizFromClass(context, thisInnerClass->outer_class);
                }
            }
        } else {
            getNbytes(context, length, NULL);
        }
    }

    /* Make sure that we have read all of the bytes */
    if (context->end_ptr != context->ptr) { 
        JAVA_ERROR(context, "Junk at end of class file");
    }

}

/*=========================================================================
 * FUNCTION:      createFakeArrayClass
 * OVERVIEW:      Invoked by Locked_FindArrayClassFromClass() for creating
 *                a fake array class that has the specified fields.
 * 
 * INTERFACE:
 *   parameters:  char *: name 
 *                int : base type 
 *                int : depth  (array dimension)
 *                ClassClass * : base type if T_CLASS 
 *                struct Hjava_lang_ClassLoader *: class loader
 *
 *   returns:     ClassClass *
 *=======================================================================*/
ClassClass *
createFakeArrayClass(char *name,   /* name */
             int base_type,        /* base_type */
             int depth,            /* array dimension */
             ClassClass *inner_cb, /* base type if T_CLASS */
             struct Hjava_lang_ClassLoader *loader)
{
    ClassClass *cb = allocClassClass();
    Classjava_lang_Class *ucb = unhand(cb);
    cp_item_type *constant_pool =
    sysCalloc(CONSTANT_POOL_ARRAY_LENGTH,
          (sizeof(cp_item_type) + sizeof(unsigned char)));
    unsigned char *type_table = (unsigned char *)
        (constant_pool + CONSTANT_POOL_ARRAY_LENGTH);
    sysAssert(name[0] == SIGNATURE_ARRAY);
    ucb->major_version = JAVA_VERSION;
    ucb->minor_version = JAVA_MINOR_VERSION;
    ucb->name = strdup(name);
    ucb->super_name = JAVAPKG "Object";
    ucb->constantpool = constant_pool;
    ucb->constantpool_count = CONSTANT_POOL_ARRAY_LENGTH;
    ucb->loader = loader;

    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type = type_table;
    constant_pool[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i = depth;
    constant_pool[CONSTANT_POOL_ARRAY_TYPE_INDEX].i = base_type;
    type_table[CONSTANT_POOL_ARRAY_DEPTH_INDEX] =
    CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;
    type_table[CONSTANT_POOL_ARRAY_TYPE_INDEX] =
    CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;

    if (base_type == T_CLASS) {
        /* Initialize the appropriate fields of the constant pool */
        constant_pool[CONSTANT_POOL_ARRAY_CLASS_INDEX].clazz = inner_cb;
        type_table[CONSTANT_POOL_ARRAY_CLASS_INDEX] =
                      CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED;
        /* The class is public iff its base class is public */
        ucb->access = ACC_FINAL | ACC_ABSTRACT | 
                      (cbAccess(inner_cb) & ACC_PUBLIC);
    } else {
        /* Set the class field to something innocuous */
        type_table[CONSTANT_POOL_ARRAY_CLASS_INDEX] =
                      CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;
        ucb->access = ACC_FINAL | ACC_ABSTRACT | ACC_PUBLIC;
    }
    AddBinClass(cb);
    return cb;
}

/*=========================================================================
 * FUNCTION:      createPrimitiveClass
 * OVERVIEW:      Creates a class block to represent a primitive type.
 *                NOTE: this is not added to the built-in class table, so
 *                      it should only be called once per primitive type.
 *                      See FindPrimitiveClass().    
 *               
 * INTERFACE:
 *   parameters:  char *: name 
 *                char  : sig 
 *                unsigned char: typecode 
 *                unsigned char: slotsize 
 *                unsigned char: elementsize
 *
 *   returns:     ClassClass *
 *=======================================================================*/
ClassClass *
createPrimitiveClass(char *name, char sig, unsigned char typecode,
    unsigned char slotsize, unsigned char elementsize)
{
    ClassClass *cb = allocClassClass();
    Classjava_lang_Class *ucb = unhand(cb);

    ucb->major_version = JAVA_VERSION;
    ucb->minor_version = JAVA_MINOR_VERSION;
    ucb->name = strdup(name);
    ucb->super_name = JAVAPKG "Object";
    ucb->constantpool = NULL;
    ucb->constantpool_count = 0;
    ucb->loader = NULL;
    ucb->access = ACC_FINAL | ACC_ABSTRACT | ACC_PUBLIC;

    CCSet(cb, Primitive);
    cbTypeSig(cb) = sig;
    cbTypeCode(cb) = typecode;
    cbSlotSize(cb) = slotsize;
    cbElementSize(cb) = elementsize;
    MakeClassSticky(cb);

    return cb;
}

/*=========================================================================
 * FUNCTION:      LoadConstantPool
 * OVERVIEW:      Loads the constant pool given a pointer to the internal
 *                class file.  
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: ptr
 *
 *   returns:     nothing 
 *=======================================================================*/
static void LoadConstantPool(CICcontext *context)
{
    ClassClass *cb = context->cb;
    int nconstants = get2bytes(context);
    cp_item_type *constant_pool;
    unsigned char *type_table;
    int i;
    Java8 t1;

    if (nconstants > 16384) {
        JAVA_ERROR(context, "Preverifier only "
                   "handles constant pool size up to 16K");
    }

    t1.x[0] = 0; /* shut off warning */
    if (nconstants < CONSTANT_POOL_UNUSED_INDEX) {
        JAVA_ERROR(context, "Illegal constant pool size");
    }

    constant_pool = (cp_item_type *)
        allocNBytes(context, nconstants * sizeof(cp_item_type));
    type_table = allocNBytes(context, nconstants * sizeof(char));

    for (i = CONSTANT_POOL_UNUSED_INDEX; i < nconstants; i++) {
        int type = get1byte(context);
        CONSTANT_POOL_TYPE_TABLE_PUT(type_table, i, type);
        switch (type) {
            case CONSTANT_Utf8: {
                int length = get2bytes(context);
                char *result = allocNBytes(context, length + 1);
                getNbytes(context, length, result);
                result[length] = '\0';
                constant_pool[i].cp = result;
                CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
                verifyUTF8String(context, result, length);
                break;
            }

            case CONSTANT_Class:
            case CONSTANT_String:
                constant_pool[i].i = get2bytes(context);
                break;

            case CONSTANT_Fieldref:
            case CONSTANT_Methodref:
            case CONSTANT_InterfaceMethodref:
            case CONSTANT_NameAndType:
                constant_pool[i].i = get4bytes(context);
                break;

            case CONSTANT_Float:
                if (no_floating_point) {
                    panic("floating-point constants should not appear");
                } else {
                   constant_pool[i].i = get4bytes(context);
                   CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
                   break;
                }
       
            case CONSTANT_Integer:
                constant_pool[i].i = get4bytes(context);
                CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
                break;

            case CONSTANT_Double:
            case CONSTANT_Long: {
                if (type == CONSTANT_Double) {
                   if (no_floating_point) {
                      panic("floating-point constants should not appear");
                   }
                }
                /* We ignore endian problems, and just load the two
                 * values.  The verifier never actually cares what
                 * the actual value is.
                 */
                constant_pool[i].i = get4bytes(context);
                CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
                i++;
                if (i >= nconstants) {
                    JAVA_ERROR(context, "illegal constant pool entry");
                }
                /* Indicate that the next object in the constant pool cannot
                 * be accessed independently.    
                 */
                constant_pool[i].i = get4bytes(context);
                CONSTANT_POOL_TYPE_TABLE_PUT(type_table, i, 0);
                CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
                break;
            }

            default:
                JAVA_ERROR(context, "Illegal constant pool type");
        } 
    }
    /* It is important to only set these after everything is setup,
       so that the GC sees a consistent state.*/
    cbConstantPool(cb) = constant_pool;
    cbConstantPoolCount(cb) = nconstants;
    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type = type_table;
}

static void
verifyUTF8String(CICcontext *context, char* bytes, unsigned short length)
{
    unsigned int i;
    for (i=0; i<length; i++) {
        unsigned int c = ((unsigned char *)bytes)[i];
        if (c == 0) /* no embedded zeros */
            goto failed;
        if (c < 128)
            continue;
        switch (c >> 4) {
        default:
            break;

        case 0x8: case 0x9: case 0xA: case 0xB: case 0xF:
            goto failed;

        case 0xC: case 0xD:
            /* 110xxxxx  10xxxxxx */
            i++;
            if (i >= length)
                goto failed;
            if ((bytes[i] & 0xC0) == 0x80)
                break;
            goto failed;

        case 0xE:
            /* 1110xxxx 10xxxxxx 10xxxxxx */
            i += 2;
            if (i >= length)
                goto failed;
            if (((bytes[i-1] & 0xC0) == 0x80) &&
                ((bytes[i] & 0xC0) == 0x80))
                break;
            goto failed;
        } /* end of switch */
    }
    return;

 failed:
    JAVA_ERROR(context, "Bad utf string");
}

/*=========================================================================
 * FUNCTION:      ReadInCode 
 * OVERVIEW:      Reads the code attributes given a pointer to the internal 
 *                class file and a pointer to the method block structure.
 *                This includes line number and local variable tables.
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: ptr
 *                struct methodblock *: mb
 *
 *   returns:     nothing 
 *=======================================================================*/
static void ReadInCode(CICcontext *context, struct methodblock *mb)
{
    int attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr + attribute_length;
    int attribute_count;
    int code_length;
    int i;

    if (cbMinorVersion(context->cb) == JAVA_VERSION &&
        cbMinorVersion(context->cb) <= 2) {
        mb->maxstack = get1byte(context);
        mb->nlocals = get1byte(context);
        code_length = mb->code_length = get2bytes(context);
    } else {
        mb->maxstack = get2bytes(context);
        mb->nlocals = get2bytes(context);
        code_length = mb->code_length = get4bytes(context);
    }
    if (mb->nlocals < mb->args_size)
        JAVA_ERROR(context, "Arguments can't fit into locals");

    if (code_length > 65535) {
        JAVA_ERROR(context, "Byte code size exceeds 65535 bytes");
    }

    mb->code = allocNBytes(context, code_length);

    getNbytes(context, code_length, (char *)mb->code);
    if ((mb->exception_table_length = get2bytes(context)) > 0) {
        unsigned exception_table_size = mb->exception_table_length
                                      * sizeof(struct CatchFrame);
        mb->exception_table = allocNBytes(context, exception_table_size);
        for (i = 0; i < (int)mb->exception_table_length; i++) {
            mb->exception_table[i].start_pc = get2bytes(context);
            mb->exception_table[i].end_pc = get2bytes(context);
            mb->exception_table[i].handler_pc = get2bytes(context);
            mb->exception_table[i].catchType = get2bytes(context);
            mb->exception_table[i].compiled_CatchFrame = NULL;
        }
    }
    attribute_count = get2bytes(context);
    for (i = 0; i < attribute_count; i++) {
        char *name = getAsciz(context, FALSE);
        if (strcmp(name, "LineNumberTable") == 0) {
            ReadLineTable(context, mb);
        } else if (strcmp(name, "LocalVariableTable") == 0) {
            ReadLocalVars(context, mb);
        } else {
            int length = get4bytes(context);
            getNbytes(context, length, NULL);
        }
    }
    if (context->ptr != end_ptr)
        JAVA_ERROR(context, "Code segment was wrong length");
}

/*=========================================================================
 * FUNCTION:      ReadLineTable
 * OVERVIEW:      Reads the line number table given a pointer to the internal 
 *                class file and a pointer to the method block structure.
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: ptr
 *                struct methodblock *: mb
 *
 *   returns:     nothing 
 *=======================================================================*/
static void ReadLineTable(CICcontext *context, struct methodblock *mb)
{
    int attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr  + attribute_length;
    int i;
    if ((mb->line_number_table_length = get2bytes(context)) > 0) {
        struct lineno *ln =
               allocNBytes(context, mb->line_number_table_length *
                           sizeof(struct lineno));
        mb->line_number_table = ln;
        for (i = mb->line_number_table_length; --i >= 0; ln++) {
            ln->pc = get2bytes(context);
            ln->line_number = get2bytes(context);
        }
    }
    if (context->ptr != end_ptr)
        JAVA_ERROR(context, "Line number table was wrong length?");
}

/*=========================================================================
 * FUNCTION:      ReadLocalVars
 * OVERVIEW:      Reads the localvar table given a pointer to the internal 
 *                class file and a pointer to the method block structure.
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: ptr
 *                struct methodblock *: mb
 *
 *   returns:     nothing 
 *=======================================================================*/
static void ReadLocalVars(CICcontext *context, struct methodblock *mb)
{
    int attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr  + attribute_length;
    int i;
    if ((mb->localvar_table_length = get2bytes(context)) > 0) {
        struct localvar *lv =
               allocNBytes(context, mb->localvar_table_length *
                           sizeof(struct localvar));
        mb->localvar_table = lv;
        for (i = mb->localvar_table_length; --i >= 0; lv++) {
            lv->pc0 = get2bytes(context);
            lv->length = get2bytes(context);
            lv->name = getAsciz(context, FALSE);
            lv->signature = getAsciz(context, FALSE);
            lv->slot = get2bytes(context);
        }
    }
    if (context->ptr != end_ptr)
        JAVA_ERROR(context, "Local variables table was wrong length?");
}

/*=========================================================================
 * FUNCTION:      ReadExceptions
 * OVERVIEW:      Reads the Exception attribute given a pointer to the internal 
 *                class file and a pointer to the method block structure.
 * 
 * INTERFACE:
 *   parameters:  CICcontext *: ptr
 *                struct methodblock *: mb
 *
 *   returns:     nothing 
 *=======================================================================*/
static void
ReadExceptions(CICcontext *context, struct methodblock *mb)
{
    int attribute_length = get4bytes(context);
    unsigned char *end_ptr = context->ptr + attribute_length;
    unsigned short nexceptions = get2bytes(context);

    if ((mb->nexceptions = nexceptions) > 0) {
        unsigned short *ep, *exceptions =
            allocNBytes(context, nexceptions * sizeof (unsigned short));
        mb->exceptions = ep = exceptions;
        while (nexceptions-- > 0) {
            *ep++ = get2bytes(context);
        }
    }
    if (context->ptr != end_ptr)
        JAVA_ERROR(context, "Exceptions attribute has wrong length");
}

/*=========================================================================
 * FUNCTION:      Signature2ArgsSize 
 * OVERVIEW:      Returns the size of arguments given a pointer to a method
 *                signature.
 *                
 * INTERFACE:
 *   parameters:  char*: method_signature 
 *
 *   returns:     unsigned
 *=======================================================================*/
unsigned Signature2ArgsSize(char *method_signature)
{
    char *p;
    int args_size = 0;
    for (p = method_signature; *p != SIGNATURE_ENDFUNC; p++) {
        switch (*p) {
            case SIGNATURE_FLOAT:
                if (no_floating_point) {
                    panic("floating-point arguments should not appear");
                } else {
                    args_size += 1;
                    break;
                }

            case SIGNATURE_BOOLEAN:
            case SIGNATURE_BYTE:
            case SIGNATURE_CHAR:
            case SIGNATURE_SHORT:
            case SIGNATURE_INT:
                args_size += 1;
                break;

            case SIGNATURE_CLASS:
                args_size += 1;
                while (*p != SIGNATURE_ENDCLASS) p++;
                break;

            case SIGNATURE_ARRAY:
                args_size += 1;
                while ((*p == SIGNATURE_ARRAY)) p++;
                    /* If an array of classes, skip over class name, too. */
                    if (*p == SIGNATURE_CLASS) {
                        while (*p != SIGNATURE_ENDCLASS)
                            p++;
                    }
                break;

            case SIGNATURE_DOUBLE:
                if (no_floating_point) {
                    panic("floating-point arguments should not appear");
                } else {
                    args_size += 2;
                    break;
                }

            case SIGNATURE_LONG:
                args_size += 2;
                break;

            case SIGNATURE_FUNC:  /* ignore initial (, if given */
                break;

            default:              /* Indicates an error. */
                return 0;
        }
    }
    return args_size;
}

/*=========================================================================
 * FUNCTION:      free_clinit_memory 
 * OVERVIEW:      Frees clinit memory.   
 *               
 * INTERFACE:
 *   parameters:  struct methodblock *: mb
 *
 *   returns:     nothing 
 *=======================================================================*/
void free_clinit_memory(struct methodblock *mb)
{
    /* This function is somewhat a wokraround. 
     * sysFree may be called on the wrong memory block if
     * the exception attribute comes before the code attribute.
     */
    /* If there is no exceptions attribute, or if both have already
     * been freed. 
     */
    if (mb->exceptions == NULL) {
        if (mb->code) {
            sysFree(mb->code);
            mb->code = NULL;
        }
        return;
    }

    /* If both attributes exist, free the one at the lower address */
    if ((char *)mb->code < (char *)mb->exceptions)
        sysFree(mb->code);
    else
        sysFree(mb->exceptions);

    mb->code = NULL;
    mb->exceptions = NULL;
}

/*=========================================================================
 * FUNCTION:      FreeClass 
 * OVERVIEW:      Frees class.   
 *               
 * INTERFACE:
 *   parameters:  ClassClass *: cb
 *
 *   returns:     nothing 
 *=======================================================================*/
void FreeClass(ClassClass *cb)
{
    int i;
    struct methodblock *mb;

    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) {
        if (strcmp(mb->fb.name, "<clinit>") == 0 &&
            strcmp(mb->fb.signature, "()V") == 0 &&
            mb->code_length /* not external */ )
            free_clinit_memory(mb);
    }

    sysFree(cbConstantPool(cb));

    sysFree(cbMethodTableMem(cb));
    sysFree(cbSlotTable(cb));

    /* Interface method tables can be shared between child and 
     * super classes.
     */
    if (cbImplementsCount(cb) != 0 || cbIsInterface(cb))
        sysFree(cbIntfMethodTable(cb));
}

/*=========================================================================
 * FUNCTION:      get1byte
 * OVERVIEW:      Gets one byte from the class file.   
 *               
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *
 *   returns:     value read or 0 if an error occurred.
 *=======================================================================*/
static unsigned char get1byte(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 1) {
        JAVA_ERROR(context, "Truncated class file");
        return 0;
    } else {
        unsigned char *ptr = context->ptr;
        unsigned char value = ptr[0];
        (context->ptr) += 1;
        return value;
    }
}

/*=========================================================================
 * FUNCTION:      get2bytes
 * OVERVIEW:      Gets two bytes from the class file.
 *               
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *
 *   returns:     value read or 0 if an error occurred.
 *=======================================================================*/
static unsigned short get2bytes(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 2) {
        JAVA_ERROR(context, "Truncated class file");
        return 0;
    } else {
        unsigned short value = (ptr[0] << 8) + ptr[1];
        (context->ptr) += 2;
        return value;
    }
}

/*=========================================================================
 * FUNCTION:      get4bytes
 * OVERVIEW:      Gets four bytes from the class file.   
 *               
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *
 *   returns:     value read or 0 if an error occurred.
 *=======================================================================*/
static unsigned long get4bytes(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 4) {
        JAVA_ERROR(context, "Truncated class file");
        return 0;
    } else {
        unsigned long value = (ptr[0] << 24) + (ptr[1] << 16) +
                                 (ptr[2] << 8) + ptr[3];
        (context->ptr) += 4;
        return value;
    }
}

/*=========================================================================
 * FUNCTION:      getNbytes
 * OVERVIEW:      Gets N bytes from the class file specified by the count 
 *                parameter. If buffer is not null, it will also copy the
 *                count number of bytes read into the buffer as well. 
 *                Note that this function seems to be always invoked with 
 *                a NULL argument for the buffer, except when it loads the
 *                UTF8 entry from the constant pool and when the code
 *                attribute is loaded.   
 *               
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *                int: count
 *                char *: buffer
 *
 *   returns:     nothing 
 *=======================================================================*/
static void getNbytes(CICcontext *context, int count, char *buffer)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < count)
        JAVA_ERROR(context, "Truncated class file");
    if (buffer != NULL)
        memcpy(buffer, ptr, count);
    (context->ptr) += count;
}

/*=========================================================================
 * FUNCTION:      getAsciz
 * OVERVIEW:      Reads the next two bytes and uses this value to look up for
 *                the corresponding constant pool entry. 
 *                Returns null if the value is 0 and zeroOkay flag is set.
 *               
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *                bool_t : zeroOkay
 *
 *   returns:     char * or NULL 
 *=======================================================================*/
static char *getAsciz(CICcontext *context, bool_t zeroOkay)
{
    ClassClass *cb = context->cb;
    union cp_item_type *constant_pool = cbConstantPool(cb);
    int nconstants = cbConstantPoolCount(cb);
    unsigned char *type_table =
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;

    int value = get2bytes(context);
    if (value == 0 && zeroOkay) {
        return NULL;
    } else if ((value == 0) || (value >= nconstants) ||
        type_table[value] != (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED))
        JAVA_ERROR(context, "Illegal constant pool index");
    return constant_pool[value].cp;
}

/*=========================================================================
 * FUNCTION:      getAscizFromClass
 * OVERVIEW:      Given the constant pool index, returns the name of the class
 *                which corresponds to this constant pool entry.  
 *               
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *                int: value 
 *
 *   returns:     char * or NULL 
 *=======================================================================*/
static char *getAscizFromClass(CICcontext *context, int value)
{
    ClassClass *cb = context->cb;
    union cp_item_type *constant_pool = cbConstantPool(cb);
    int nconstants = cbConstantPoolCount(cb);
    unsigned char *type_table =
    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    if ((value > 0) && (value < nconstants)) {
        if (type_table[value] == CONSTANT_Class) {
            value = constant_pool[value].i;
            if ((value <= 0) || (value >= nconstants) ||
                (type_table[value] != (CONSTANT_Utf8 |
                       CONSTANT_POOL_ENTRY_RESOLVED)))
                JAVA_ERROR(context, "Illegal constant pool index");
            return constant_pool[value].cp;
        } else if (type_table[value] == (CONSTANT_Class |
                                 CONSTANT_POOL_ENTRY_RESOLVED)) {
            ClassClass *cb = constant_pool[value].clazz;
            return cbName(cb);
        } else {
            JAVA_ERROR(context, "Illegal constant pool index");
        }
    } else {
        JAVA_ERROR(context, "Illegal constant pool index");
    }
    return NULL; /* not reached */
}

/* In order to avoid possible alignment errors, round up all sizes to 
 * multiples of eight.
 */
#define ROUNDUP_SIZE(s) while ((s) % 8 != 0) (s)++

/*=========================================================================
 * FUNCTION:      allocNBytes 
 * OVERVIEW:      Memory allocation function for internal class file 
 *                structures. 
 *                It calculates the number of allocations needed for the
 *                two passes, and the allocations required for the clinit
 *                method.
 *              
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *                int : size
 *
 *   returns:     void * 
 *=======================================================================*/
static void *allocNBytes(CICcontext *context, int size)
{
    void *result;
    if (context->pass == 1) {
        /* The first pass
         * A more sophisticated scheme could reduce the number of mallocs.
         */
        CICmallocs *mallocs =
           (CICmallocs *)sysCalloc(1, sizeof(CICmallocs) + size);
        if (mallocs == 0)
            JAVA_ERROR(context, "out of memory");
        result = (void *)(mallocs + 1);
        mallocs->next = context->pass1.mallocs;
        ROUNDUP_SIZE(size);
        if (context->in_clinit)
            context->clinit_size += size;
        else
            context->malloc_size += size;
        context->pass1.mallocs = mallocs;
    } else {
        /* The second pass */

        ROUNDUP_SIZE(size);

#define ALLOC_BLOCK(ptr,buf,sizelimit) \
        result = (ptr);                \
        (ptr) += (size);               \
        sysAssert((ptr) <= (buf) + (sizelimit))

        if (context->in_clinit) {
            /* Make sure that this clinit pointer is not null */
            sysAssert(context->pass2.clinit_ptr != NULL);

            ALLOC_BLOCK(context->pass2.clinit_ptr,
            context->pass2.clinit_buffer,
            context->clinit_size*sizeof(char));

        } else {

            /* Make sure that this malloc pointer is not null */
            sysAssert(context->pass2.malloc_ptr != NULL);

            ALLOC_BLOCK(context->pass2.malloc_ptr,
            context->pass2.malloc_buffer,
            context->malloc_size);
        }
    }
    return result;
}

/*=========================================================================
 * FUNCTION:      freeBuffers
 * OVERVIEW:      Frees buffers allocated by allocNBytes().      
 *              
 * INTERFACE:
 *   parameters:  CICcontext *: context 
 *
 *   returns:     nothing 
 *=======================================================================*/
static void freeBuffers(CICcontext * context)
{
    if (context->pass == 1) {
        CICmallocs *mallocs = context->pass1.mallocs;

        while (mallocs) {
            CICmallocs *tmp = mallocs;
            mallocs = mallocs->next;
            if (tmp != NULL) {
                sysFree(tmp);
            }
        }
        context->pass1.mallocs = 0;
    } else { /* context->pass = 2 */

/* Note: this code is here just for historical reasons. Actually, these
 * buffers cannot really be freed here since the data in them is still
 * pointed to by the class buffer (cb) and we are not yet done loading
 * the class. If the class loading fails, FreeClass() will free the method
 * blocks and the clinit memory.
 */
        if (context->pass2.malloc_buffer != NULL) {
            sysFree(context->pass2.malloc_buffer);
            /* Reset only if buffer was freed */
            context->pass2.malloc_buffer = 0;
        }

        if (context->pass2.clinit_buffer != NULL) {
            sysFree(context->pass2.clinit_buffer);
            /* Initialize only if buffer was freed */
            context->pass2.clinit_buffer = 0;
        }
    }
}

/*=========================================================================
 * FUNCTION:      GetClassConstantClassName
 * OVERVIEW:      Returns class name corresponding to the constant pool entry
 *                for the given cpIndex. This is desirable in cases when we
 *                may simply be interested in the name of the class, but may
 *                not necessarily want to resolve the class reference if it
 *                isn't already. 
 *               
 * INTERFACE:
 *   parameters:  cp_item_type *: constant pool 
 *                int : index 
 *
 *   returns:     char *: class name
 *=======================================================================*/
char *GetClassConstantClassName(cp_item_type *constant_pool, int index)
{
    unsigned char *type_table =
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    switch(type_table[index]) {
        case CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED: {
            ClassClass *cb = constant_pool[index].clazz;
            return cbName(cb);
        }

        case CONSTANT_Class: {
            int name_index = constant_pool[index].i;
            return constant_pool[name_index].cp;
        }

        default:
            return (char *)0;
    }
}

