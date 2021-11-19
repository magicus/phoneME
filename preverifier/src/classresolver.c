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
 * SUBSYSTEM: class resolver.c.
 * FILE:      classresolver.c
 * OVERVIEW:  Routines for loading and resolving class definitions.
 *            These routines should not be depending upon the interpreter
 *            or the garbage collector.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <oobj.h>
#include <tree.h>
#include <signature.h>
#include <path.h>
#include <sys_api.h>
#include <convert_md.h>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

ClassClass *classJavaLangObject;
ClassClass *classJavaLangClass = 0;
ClassClass *classJavaLangString;
ClassClass *classJavaLangThrowable;
ClassClass *classJavaLangError;
ClassClass *classJavaLangException;
ClassClass *classJavaLangRuntimeException;
ClassClass *classJavaLangThreadDeath;

ClassClass *interfaceJavaLangCloneable;
ClassClass *interfaceJavaIoSerializable;

bool_t inline_jsr_on = TRUE;

static bool_t RunClinit(ClassClass * cb);

static ClassClass *InitializeAndResolveClass(ClassClass *cb, bool_t resolve);
static char *Locked_InitializeClass(ClassClass * cb, char **detail);
static char *Locked_LinkClass(ClassClass * cb, char **detail);
char * LinkClass(ClassClass *cb, char **detail);
static ClassClass *FindLoadedClass(char *name,
                   struct Hjava_lang_ClassLoader *loader);
static ClassClass *Locked_FindArrayClassFromClass(struct execenv *ee, 
                          char *name, 
                          ClassClass *from);
static void InitPrimitiveClasses();

/* An Explanation of Class-related Locks
 *
 * There are two global locks related to class loading:
 *
 * LOADCLASS_LOCK: ensures that only one thread is loading a class at
 * a given time. This eliminates the possibility of two threads loading
 * classes with the same name and same loader.
 *
 * BINCLASS_LOCK: ensures that only one thread is updating the global
 * class table (binclasses). This lock is also grabbed by the GC to ensure
 * that the GC always sees a valid global class table state.
 *
 * In addition, each class may have its own associated locks that are
 * created with monitorEnter(). ResolveClass, for example, need to
 * first grab this lock to ensure that the class is resolved only by
 * one thread, rather than simultaneously by multiple threads.
 */

sys_mon_t *_loadclass_lock;
sys_mon_t *_binclass_lock;

static struct fieldblock **
addslots(struct fieldblock ** fb, ClassClass * cb)
{
    long n = cbFieldsCount(cb);
    struct fieldblock *sfb = cbFields(cb);
    if (cbSuperclass(cb))
    fb = addslots(fb, cbSuperclass(cb));
    while (--n >= 0) {
    *fb++ = sfb;
    sfb++;
    }
    return fb;
}

int
Locked_makeslottable(ClassClass * clb)
{
    ClassClass *sclb;
    int         nslots = 0;

    if (cbSlotTable(clb)) {
    return SYS_OK;
    }
    sclb = clb;
    while (sclb) {
    long        n = cbFieldsCount(sclb);
    struct fieldblock *fb = cbFields(sclb);
    while (--n >= 0) {
        nslots++;
        fb++;
    }
    if (cbSuperclass(sclb) == 0) {
        break;
    }
    sclb = cbSuperclass(sclb);
    }
    cbSlotTableSize(clb) = nslots;
    if (nslots == 0) {
    nslots++;
    }
    cbSlotTable(clb) = (struct fieldblock **)
    sysMalloc(nslots * sizeof(struct fieldblock *));
    if (cbSlotTable(clb) == 0) {
    return SYS_NOMEM;
    }
    addslots(cbSlotTable(clb), clb);
    return SYS_OK;
}

int
makeslottable(ClassClass * clb)
{
    int result;
    LOADCLASS_LOCK();
    result = Locked_makeslottable(clb);
    LOADCLASS_UNLOCK();
    return result;
}

#if 0
static char *
copyclassname(char *src, char *dst)
{
    sysAssert(*src == SIGNATURE_CLASS);
    src++;
    while (*src && *src != SIGNATURE_ENDCLASS)
    *dst++ = *src++;
    *dst = 0;
    return src;
}
#endif

static char *
ResolveFields(ClassClass *cb, unsigned slot)
{
    struct fieldblock *fb;
    int size;

    fb = cbFields(cb);
    for (size = 0; size < (int) cbFieldsCount(cb); size++, fb++) {
    char *signature = fieldsig(fb);
    int size = (((signature[0] == SIGNATURE_LONG || 
              signature[0] == SIGNATURE_DOUBLE)) ? 2 : 1);
    fb->ID = NameAndTypeToHash(fb->name, signature);
    if (fb->access & ACC_STATIC) {
        /* Do nothing.  Handled when the class is loaded. */
    } else {
        fb->u.offset = slot;
        slot += size * sizeof(OBJECT);
    }
#ifdef UNUSED
    if ((fb->access & (ACC_STATIC | ACC_TRANSIENT)) == 0) {
        for (s = fieldname(fb); (c = *s++) != 0;)
        thishash = thishash * 7 + c;
        for (s = fieldsig(fb); (c = *s++) != 0;)
        thishash = thishash * 7 + c;
    }
#endif
    }
    if (slot > 65535) {
        return "java/lang/InternalError";
    }

    cbInstanceSize(cb) = slot;
#ifdef UNUSED
    cbThisHash(cb) = thishash;
    if (cbSuperclass(cb))
    cbTotalHash(cb) = thishash - cbTotalHash(unhand(cbSuperclass(cb)));
    else
        cbTotalHash(cb) = thishash;
    if (cbTotalHash(cb) < N_TYPECODES)
    cbTotalHash(cb) += N_TYPECODES;
#endif
    return NULL;
}

static char *
ResolveMethods(ClassClass *cb)
{
    struct methodblock *mb;
    int size;
    struct methodtable *new_table;
    struct methodblock **super_methods;
    int mslot, super_methods_count;
    void *ptr;
    static unsigned finalizerID = 0;
    
    if (finalizerID == 0)
    finalizerID = NameAndTypeToHash(FINALIZER_METHOD_NAME,
                    FINALIZER_METHOD_SIGNATURE);
    mb = cbMethods(cb);
    for (size = 0; size < (int) cbMethodsCount(cb); size++, mb++) {
    mb->fb.ID = NameAndTypeToHash(mb->fb.name, mb->fb.signature);
    mb->fb.u.offset = 0;
    mb->invoker = 0;
    }

    if (cbIsInterface(cb)) { 
    /* We don't really need to built a method table for interfaces. */
    cbMethodTable(cb) = NULL;
    cbMethodTableSize(cb) = 0;
    mb = cbMethods(cb);
    /* Each method's offset is its index in the interface */
    for (size = 0; size < (int) cbMethodsCount(cb); size++, mb++) {
        mb->fb.u.offset = size;
    }
    return NULL;
    }

    if (cbSuperclass(cb) != NULL) { 
    ClassClass *super = cbSuperclass(cb);
    mslot = cbMethodTableSize(super);
    super_methods = cbMethodTable(super)->methods;
        super_methods_count = cbMethodTableSize(super);
    /* Inherit one's parent's finalizer, if it has one */
    cbFinalizer(cb) = cbFinalizer(cbSuperclass(cb));
    } else { 
    mslot = 1;
    super_methods = NULL;
    super_methods_count = 0;
    cbFinalizer(cb) = NULL;
    }

    mb = cbMethods(cb);
    for (size = 0; size < (int) cbMethodsCount(cb); size++, mb++) {
    unsigned long ID = mb->fb.ID;
    struct methodblock **super_methods_p;
    int count;

    if ((mb->fb.access & ACC_STATIC) || (mb->fb.access & ACC_PRIVATE))
        continue;
    if (strcmp(mb->fb.name, "<init>") == 0)
        continue;

    /* If this item has its own finalizer method, grab it */
    if (mb->fb.ID == finalizerID) {
        if (no_finalizers) {
        panic("finalize methods should not appear");
        }
        cbFinalizer(cb) = mb;
    }

    for (super_methods_p = super_methods, count = super_methods_count;
           count > 0;
           super_methods_p++, count--) {
        if ((*super_methods_p != NULL) && ((*super_methods_p)->fb.ID == ID)) { 
          /* Private methods are not inherited outside of the class. */
            if ((*super_methods_p)->fb.access & ACC_PRIVATE)
          continue;
          /* Package-private methods are not inherited outside of the
         package. */
            if (((*super_methods_p)->fb.access & ACC_PROTECTED) ||
            ((*super_methods_p)->fb.access & ACC_PUBLIC) ||
            IsSameClassPackage((*super_methods_p)->fb.clazz, cb)) {
            mb->fb.u.offset = (*super_methods_p)->fb.u.offset;
            break;
        } 
          /*
        jio_fprintf(stderr, "!!!%s.%s\n", cbName(cb), mb->fb.name);
        */
        }
    }

    if (mb->fb.u.offset == 0) { 
        mb->fb.u.offset = mslot;
        mslot++;
    }
    }

    if (mslot > 65535) {
        return "java/lang/InternalError";
    }

    /*
     *    This should be the only place that method tables are
     *    allocated.  We allocate more than we need here and mask the
     *    resulting pointer because the methodtable pointer field in
     *    object handles is overloaded and sometimes hold array types
     *    and array lengths.  We must ensure that method table pointers
     *    are allocated on an appropriate boundary so we don't lose any
     *    address bits when we mask off the type portion of the pointer.
     */
    ptr = sysMalloc(sizeof(struct methodtable)
               + (mslot - 1)* sizeof(struct methodblock *)
               + FLAG_MASK);
    if (ptr == NULL) {
    CCSet(cb, Error);
    return JAVAPKG "OutOfMemoryError";
    }
    cbMethodTableMem(cb) = ptr;
    new_table = (struct methodtable *)((((long)ptr) + FLAG_MASK) & LENGTH_MASK);
    new_table->classdescriptor = cb;
    memset((char *)new_table->methods, 0, mslot * sizeof(struct methodblock *));
    if (super_methods) 
    memcpy((char *) new_table->methods,
           (char *) super_methods,
           super_methods_count * sizeof(struct methodblock *));
    mb = cbMethods(cb);
    for (size = 0; size < (int) cbMethodsCount(cb); size++, mb++) {
    int offset = (int)mb->fb.u.offset;
    if (offset > 0) { 
        sysAssert(offset < mslot);
        mt_slot(new_table, offset) = mb;
    }
    }
    cbMethodTable(cb) = new_table;
    cbMethodTableSize(cb) = mslot;

    return NULL;
}

static char *
ResolveInterfaces(ClassClass *cb, char **detail)
{
    const int ITEM_SIZE =  sizeof(cbIntfMethodTable(cb)->itable[0]);
    bool_t isInterface = cbIsInterface(cb);
    if (cbImplementsCount(cb) == 0 && !isInterface) {
    /* classes that don't implement their own interfaces can just inherit
     * their parents imethodtable. */
    if (cb == classJavaLangObject) {
        /* We can preinitialize the imethodtable of java.lang.Object */
        static struct imethodtable t = { 0 };
        cbIntfMethodTable(cb) = &t;
    } else { 
        ClassClass *super = cbSuperclass(cb);
        cbIntfMethodTable(cb) = cbIntfMethodTable(super);
    } 
    return NULL;
    } else { 
    cp_item_type *constant_pool = cbConstantPool(cb);
    ClassClass *super = cbSuperclass(cb);
    unsigned char *mallocSpace, *mallocSpaceEnd;
    ClassClass *icb;    /* temporary */
    struct imethodtable *super_itable = cbIntfMethodTable(super);
    struct imethodtable *this_itable = NULL;
    int super_itable_count = super_itable->icount;
    int i, j, k, icount, mcount;
    
    /* Resolve all the interfaces that this class implements, and 
     * make sure that they all really are interfaces 
     *
     * icount will total all the interfaces that this class implements,
     *    (include interfaces implemented by our superclass, and by 
     *    interfaces that we implement.)
     * mcount will total the total amount of space (in sizeof(long)) for 
     *    which we'll have to allocate space for in the offsets table.
     */
    icount = super_itable_count + (isInterface ? 1 : 0);
    mcount = 0;
    for (i = 0; i < (int)(cbImplementsCount(cb)); i++) {
        int interface_index = cbImplements(cb)[i];
        struct imethodtable *sub_itable;

        icb = constant_pool[interface_index].clazz;
        if (!cbIsInterface(icb)) {
            *detail = "Implementing class";
        return JAVAPKG "IncompatibleClassChangeError";
        }
        sub_itable = cbIntfMethodTable(icb);
        icount += sub_itable->icount;
        if (!isInterface) 
        for (j = sub_itable->icount; --j >= 0; ) 
            mcount += cbMethodsCount(sub_itable->itable[j].classdescriptor);
    }

    { 
        int this_itable_size = 
        offsetof(struct imethodtable, itable) + icount * ITEM_SIZE;
        int offsets_size = mcount * sizeof(unsigned long);
        mallocSpace = sysMalloc(this_itable_size + offsets_size);
        if (mallocSpace == NULL) { 
        return JAVAPKG "OutOfMemoryError";
        }
        mallocSpaceEnd = mallocSpace + this_itable_size + offsets_size;
        this_itable = (struct imethodtable *)mallocSpace;
        mallocSpace += this_itable_size;
        sysAssert(mallocSpace <= mallocSpaceEnd);
    }
    
    cbIntfMethodTable(cb) = this_itable;

    /* Start filling in the table. */
    icount = 0;
    if (isInterface) {
        this_itable->itable[icount].classdescriptor = cb;
        this_itable->itable[icount].offsets = NULL;    
        icount++;
    }
    if (super_itable_count > 0) { 
        /* We can copy our superclass's offset table.  The offsets
           will stay the same.  */
        memcpy(&this_itable->itable[icount], 
           &super_itable->itable[0], 
           super_itable_count * ITEM_SIZE);
        icount += super_itable_count;
    }
    for (i = 0; i < (int)(cbImplementsCount(cb)); i++) {
        /* Add the interfaces that we implement, either directly, or
         * because those interfaces implement other interfaces.    */
        int interface_index = cbImplements(cb)[i]; 
        icb = constant_pool[interface_index].clazz;
        memcpy(&this_itable->itable[icount], 
           &cbIntfMethodTable(icb)->itable[0],
           cbIntfMethodTable(icb)->icount * ITEM_SIZE);
        icount += cbIntfMethodTable(icb)->icount;
    }
    sysAssert(!isInterface || super_itable_count == 0);
    /* Delete duplicates from the table.  This is very rare, so it can
     * be quite inefficient.  */
    for (i = (isInterface ? 1 : super_itable_count); i < icount; i++) {
        icb = this_itable->itable[i].classdescriptor;
        for (j = 0; j < i; j++) { 
        if (icb == this_itable->itable[j].classdescriptor) { 
            /* We have an overlap.  Item i is a duplicate.  Delete from
             * the table */
            for (k = i + 1; k < icount; k++) 
            this_itable->itable[k - 1] = this_itable->itable[k];
            icount--;
            i--;     /* Reconsider this entry! */
            break;
        }
        }
    }
    this_itable->icount = icount;
    if (isInterface || cbIsAbstract(cb)) { 
        /* Nothing more to do for interfaces or abstract classes*/
        return NULL;
    }

    /* For each additional interface . . . */
    for (i = super_itable_count; i < icount; i++) { 
        /* The table length is the number of interface methods */
        ClassClass *intfi = this_itable->itable[i].classdescriptor;
        int intfi_count = cbMethodsCount(intfi);
        unsigned long *offsets = (unsigned long *)mallocSpace;
        mallocSpace += intfi_count * sizeof(unsigned long);
        sysAssert(mallocSpace <= mallocSpaceEnd);
        this_itable->itable[i].offsets = offsets;
        /* Look at each interface method */
        for (j = 0; j < intfi_count; j++) { 
        struct methodblock *imb = cbMethods(intfi) + j;
        if ((imb->fb.access & ACC_STATIC) != 0) { 
            sysAssert(!strcmp(imb->fb.name, "<clinit>"));
            offsets[j] = 0; 
        } else {
            /* Find the class method that implements the interface 
             * method */
            unsigned ID = imb->fb.ID;
            struct methodblock **mbt = cbMethodTable(cb)->methods;
            for (k = cbMethodTableSize(cb) - 1; k >= 0; --k) { 
            struct methodblock *mb = mbt[k];
            if (mb != NULL 
                  && mb->fb.ID == ID && IsPublic(mb->fb.access)) {
                offsets[j] = mb->fb.u.offset;
                break;
            }
            }
                    /*
            if (k == -1) { 
            *detail = "Unimplemented interface method";
            return JAVAPKG "IncompatibleClassChangeError";
            }
                    */
        }
        }
    }
    return NULL;
    }
}

char *
InitializeClass(ClassClass * cb, char **detail) 
{ 
    char *result;
    monitorEnter(obj_monitor(cb));
    result = Locked_InitializeClass(cb, detail);
    monitorExit(obj_monitor(cb));
    return result;
}

char *
ResolveClass(ClassClass *cb, char **detail) 
{
    char *result;
    if (CCIs(cb, Resolved))
        return 0;
    result = LinkClass(cb, detail);
    if (result == 0) {
        if (!RunClinit(cb)) {
        result = JAVAPKG "ExceptionInInitializerError";
        *detail = cbName(cb);
    }
    }
    return result;
}

char *
LinkClass(ClassClass *cb, char **detail)
{
    char *result;
    if (CCIs(cb, Linked))
        return 0;
    monitorEnter(obj_monitor(cb));
    result = Locked_LinkClass(cb, detail);
    monitorExit(obj_monitor(cb));
    return result;
}

/*
 * Detect class circularities from InitializeClass()
 */

static void
pushSeen(ExecEnv *ee, struct seenclass *seen)
{
    struct seenclass *prev = ee->seenclasses.next;
    ee->seenclasses.next = seen;
    seen->next = prev;
}

static void
popSeen(ExecEnv *ee, struct seenclass *seen)
{
    if (seen != ee->seenclasses.next) /* paranoia */
    panic("popSeen: corrupt seen class stack");
    ee->seenclasses.next = ee->seenclasses.next->next;
}

static bool_t
checkSeen(ExecEnv *ee, ClassClass *cb)
{
    struct seenclass *seen = ee->seenclasses.next;
    while (seen) {
    if (cb == seen->cb)
        return TRUE;
    seen = seen->next;
    }
    return FALSE;
}

static char *
Locked_InitializeClass(ClassClass * cb, char **detail)
{
    ClassClass *super = 0;
    char *ret = 0;
    bool_t noLoader;
    int i;
    char buff[BUFSIZ];
    char *nativeName = &buff[0];

    if (CCIs(cb, Initialized))
    return NULL;

#ifndef TRIMMED
    if (verbose)
    jio_fprintf(stderr, "[Initializing %s]\n", cbName(cb));
#endif

    noLoader = (cbLoader(cb) == 0);
    if ((strcmp(cbName(cb), CLS_RESLV_INIT_CLASS) == 0) && noLoader) {
    /* Temporarily disable class circularity checks */
    ExecEnv *ee = EE();
    struct seenclass *save = ee->seenclasses.next;
    ee->seenclasses.next = NULL;
    /* Note: don't bother restoring on (fatal) error during bootstrap */

    classJavaLangClass = cb;
    MakeClassSticky(cb);
    classJavaLangString =
        FindStickySystemClass(NULL, JAVAPKG "String", TRUE);
    if (classJavaLangString == 0) {
        *detail = JAVAPKG "String";
        return JAVAPKG "NoClassDefFoundError";
    }
/*    classJavaLangThreadDeath =
        FindStickySystemClass(NULL, JAVAPKG "ThreadDeath", TRUE);
    if (classJavaLangThreadDeath == 0) {
        *detail = JAVAPKG "ThreadDeath";
        return JAVAPKG "NoClassDefFoundError";
    }
    */
    classJavaLangThrowable =
        FindStickySystemClass(NULL, JAVAPKG "Throwable", TRUE);
    if (classJavaLangThrowable == 0) {
        *detail = JAVAPKG "Throwable";
        return JAVAPKG "NoClassDefFoundError";
    }
    /*
    classJavaLangException =
        FindStickySystemClass(NULL, JAVAPKG "Exception", TRUE);
    if (classJavaLangException == 0) {
        *detail = JAVAPKG "Exception";
        return JAVAPKG "NoClassDefFoundError";
    }
    classJavaLangError = 
                FindStickySystemClass(NULL, JAVAPKG "Error", TRUE);
    if (classJavaLangError == 0) {
        *detail = JAVAPKG "Error";
        return JAVAPKG "NoClassDefFoundError";
    }
    classJavaLangRuntimeException = 
        FindStickySystemClass(NULL, JAVAPKG "RuntimeException", TRUE);
    if (classJavaLangRuntimeException == 0) {
        *detail = JAVAPKG "RuntimeException";
        return JAVAPKG "NoClassDefFoundError";
    }
    interfaceJavaLangCloneable =
        FindStickySystemClass(NULL, JAVAPKG "Cloneable", TRUE);
    if (interfaceJavaLangCloneable == 0) {
        *detail = JAVAPKG "Cloneable";
        return JAVAPKG "NoClassDefFoundError";
    }
    interfaceJavaIoSerializable =
        FindStickySystemClass(NULL, "java/io/Serializable", TRUE);
    if (interfaceJavaIoSerializable == 0) {
        *detail = "java/io/Serializable";
        return JAVAPKG "NoClassDefFoundError";
    }
*/
    /* Restore stack for class circularity checks */
    ee->seenclasses.next = save;

    } else if ((strcmp(cbName(cb), CLS_RESLV_INIT_OBJECT) == 0) && noLoader){
    classJavaLangObject = cb;
    MakeClassSticky(classJavaLangObject);
    }

    if (noLoader) { 
    char *name = cbName(cb);
    if (strcmp(name, CLS_RESLV_INIT_REF) == 0) {
        CCSet(cb, SoftRef);
    }
    if (strncmp(name, "java/", 5) || strncmp(name, "sun/", 4)) {
        CCSet(cb, SysLock);
    }
    }
    if (cbSuperclass(cb) == NULL) {
    if (cbSuperName(cb)) {
        /* Check for class definition circularities */
        ExecEnv *ee = EE();
        struct seenclass this[1];
        if (checkSeen(ee, cb)) {
        *detail = cbName(cb);
        CCSet(cb, Error);
        return JAVAPKG "ClassCircularityError";
        }
        this->cb = cb;
        this->next = NULL;
        pushSeen(ee, this);
           
            /* Conversion for Japanese file names */
            utf2native(cbSuperName(cb), nativeName, BUFSIZ);

        super = FindClassFromClass(ee, nativeName, FALSE, cb);

        popSeen(ee, this);
        if (super != NULL) { 
        sysAssert(CCIs(super, Initialized));
        /* Don't allow a class to have a superclass it can't access */
        if (!VerifyClassAccess(cb, super, FALSE)) { 
            super = NULL;
        } 
        }
        if (super != NULL) {
        cbSuperclass(cb) = cbHandle(super);
        if (CCIs(super, SoftRef))
            CCSet(cb, SoftRef);
        } else {
        ret = JAVAPKG "NoClassDefFoundError";
        *detail = cbSuperName(cb);
        cbSuperclass(cb) = NULL;
        CCSet(cb, Error);
        } 
    } else if (cb == classJavaLangObject) {
        cbSuperclass(cb) = 0;        
    } else {
        *detail = cbName(cb);
        return JAVAPKG "ClassFormatException";
    }
    }

    for (i = 0; i < (int)(cbImplementsCount(cb)); i++) {
        /* Be very careful, since not verified yet. . . 
     */
        int interface_index = cbImplements(cb)[i];
    cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table = 
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    ClassClass *icb;
    int nconstants = cbConstantPoolCount(cb);
    int iname_index;
    char *iname;

    if (interface_index <= 0 ||
        interface_index >= nconstants ||
        type_table[interface_index] != CONSTANT_Class ||
        !(iname_index = constant_pool[interface_index].i) ||
        iname_index <= 0 ||
        iname_index >= nconstants ||
        type_table[iname_index] != 
        (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED)) {
        *detail = "Bad interface index";
        return JAVAPKG "ClassFormatError";
    }
        
    iname = constant_pool[iname_index].cp;
    if (iname == NULL || !IsLegalClassname(iname, FALSE)) {
        *detail = "Bad interface name";
        return JAVAPKG "ClassFormatError";
    }

    {
        ExecEnv *ee = EE();
        struct seenclass this[1];
        if (checkSeen(ee, cb)) {
        *detail = cbName(cb);
        CCSet(cb, Error);
        return JAVAPKG "ClassCircularityError";
        }
        this->cb = cb;
        this->next = NULL;
        pushSeen(ee, this);
        icb = FindClassFromClass(ee, iname, FALSE, cb);
        popSeen(ee, this);
        if (icb) {
            constant_pool[interface_index].clazz = icb;
        type_table[interface_index] |= CONSTANT_POOL_ENTRY_RESOLVED;
        } else {
            *detail = iname;
        CCSet(cb, Error);
        return JAVAPKG "NoClassDefFoundError";
        }
    }
    }

    CCSet(cb, Initialized);

    /* Make sure we know what classJavaLangClass is, and that it's method table
     * is filled in.
     */
    if (classJavaLangClass == 0) {
    classJavaLangClass = 
          FindClassFromClass(0, CLS_RESLV_INIT_CLASS, TRUE, cb);
    if (classJavaLangClass == 0) {
        return JAVAPKG "NoClassDefFoundError";
    }
    }
    
    /* The following may not do anything useful if classJavaLangClass hasn't
     * been completely resolved.  But we clean up in ResolveClass 
     */

    cbHandle(cb)->methods = cbMethodTable(classJavaLangClass);
    return ret;
}

static char *
Locked_LinkClass(ClassClass * cb, char **detail)
{
    ClassClass *super = 0;
    unsigned slot = 0;
    char *ret = 0;
    int i;

    if (CCIs(cb, Error)) {
    *detail = cbName(cb);
    return JAVAPKG "NoClassDefFoundError";
    }
   
    sysAssert(CCIs(cb, Initialized));

    if (CCIs(cb, Linked)) {
    return NULL;
    }

    if (cbSuperclass(cb)) {
    /* If this object has a superclass. . . */
    super = cbSuperclass(cb);
    if (!CCIs(super, Linked)) {
        if ((ret = LinkClass(super, detail)) != 0) {
        CCSet(cb, Error);
        return ret;
        }
    }
    sysAssert(CCIs(super, Linked));
    slot = cbInstanceSize(super);
    }

    for (i = 0; i < (int)(cbImplementsCount(cb)); i++) {
        int interface_index = cbImplements(cb)[i];
    cp_item_type *constant_pool = cbConstantPool(cb);
    ClassClass *icb;
    icb = constant_pool[interface_index].clazz;
    if (!CCIs(icb, Linked)) {
        if ((ret = LinkClass(icb, detail)) != 0) {
        CCSet(cb, Error);
        return ret;
        }
    }
    }
    
    sysAssert(!CCIs(cb, Error));
    sysAssert(!CCIs(cb, Linked));

#ifndef TRIMMED
    if (verbose)
        jio_fprintf(stderr, "[Resolving %s]\n", cbName(cb));
#endif
    cbInstanceSize(cb) = -1;
    if ((ret = ResolveFields(cb, slot))) { /* May return "InternalError" */
        *detail = cbName(cb);
    CCSet(cb, Error);
    return ret;
    }

    if ((ret = ResolveMethods(cb))) {
        /* May return "OutOfMemoryError" or "InternalError" */
    *detail = cbName(cb);
    CCSet(cb, Error);
    return ret;
    }

    if ((ret = ResolveInterfaces(cb, detail))) { /* All sorts of errors */
    CCSet(cb, Error);
    return ret;
    }

    InitializeInvoker(cb);

    if ((verifyclasses == VERIFY_ALL) ||
        ((verifyclasses == VERIFY_REMOTE) && (cbLoader(cb) != NULL))) {
        if (!VerifyClass(cb)) {
            *detail = "";
            return JAVAPKG "VerifyError";
        }
    }

    CCSet(cb, Linked);

    /* We need this for bootstrapping.  We can't set the Handle's class block
     * pointer in Object or Class until Class has been resolved.
     */
    if (cb == classJavaLangClass) {
    int         j;
    ClassClass **pcb;
    BINCLASS_LOCK();
    for (j = nbinclasses, pcb = binclasses; --j >= 0; pcb++) {
        cbHandle(*pcb)->methods = cbMethodTable(cb);
    }
    BINCLASS_UNLOCK();
    InitPrimitiveClasses();
    }
    return NULL;
}

static bool_t
RunClinit(ClassClass * cb)
{

    if (CCIs(cb, Resolved))
        return TRUE;

    if ((cbName(cb)[0] != SIGNATURE_ARRAY) && !cbIsPrimitive(cb)) {
    /* Don't need to initialize or verify array or primitive classes */
        return RunStaticInitializers(cb);
    } else if (cbName(cb)[0] == SIGNATURE_ARRAY) {
    ClassClass *inner_cb = 
        cbConstantPool(cb)[CONSTANT_POOL_ARRAY_CLASS_INDEX].clazz;
    if (inner_cb) {
        char *detail = 0;
        char *ret = ResolveClass(inner_cb, &detail);
        if (ret != NULL) {
            if (!exceptionOccurred(EE())) { 
            SignalError(0, ret, detail);
        }
        CCSet(cb, Error);
        return FALSE;
        } else {
            CCSet(cb, Resolved);
        }
    } else {
        CCSet(cb, Resolved);
    }
    } else {
        CCSet(cb, Resolved);
    }
    return TRUE;
}

/* Find an already loaded class with the given name.  If no such class exists
 * then return 0.
 */

#ifdef DEBUG
void
PrintLoadedClasses()
{
    int         j;
    ClassClass **pcb, *cb;
    BINCLASS_LOCK();
    pcb = binclasses;
    for (j = nbinclasses; --j >= 0; pcb++) {
    cb = *pcb;
    jio_fprintf(stdout, "class=%s, 0x%x\n", cbName(cb), cbLoader(cb));
    }
    BINCLASS_UNLOCK();
}
#endif

static ClassClass *
FindLoadedClass(char *name, struct Hjava_lang_ClassLoader *loader)
{
    int left, right, middle, result;
    ClassClass *cb = NULL;

    BINCLASS_LOCK();
    left = 0;
    right = nbinclasses - 1;
    result = 1;
    while (left <= right) {
        middle = (left + right)/2;
    cb = binclasses[middle];
    result = strcmp(name, cbName(cb));
    if (result == 0) {
        if (loader < cbLoader(cb)) {
            result = -1;
        } else if (loader > cbLoader(cb)) {
            result = 1;
        } else {
            result = 0;
        }
    }
    if (result < 0) {
        right = middle - 1;
    } else if (result > 0) {
        left = middle + 1;
    } else {
        break;
    }
    }

    BINCLASS_UNLOCK();
    if (result == 0) {
        return cb;
    }
    return NULL;
}

/* We attempt to resolve it, also, if the "resolve" argument is true.
 * Otherwise, we only perform a minimal initialization on it, such that
 * it points to its superclass, which points to its superclass. . . .
 */

static ClassClass *
InitializeAndResolveClass(ClassClass *cb, bool_t resolve)
{
    char *err, *detail = NULL;
    if ((err = InitializeClass(cb, &detail))) {
        /* Check for underlying error already posted */
    if (!exceptionOccurred(EE()))
        SignalError(0, err, detail);
    return 0;
    }
    if (resolve) {
        err = ResolveClass(cb, &detail);
    if (err) {
        /* Check for underlying error already posted */ 
        if (!exceptionOccurred(EE())) 
            SignalError(0, err, detail);
        return 0;
    }
    }
    return cb;
}

/* Find the class with the given name.  
 * If "resolve" is true, then we completely resolve the class.  Otherwise,
 * we only make sure that it points to its superclass, which points to its
 * superclass, . . . .
 */
ClassClass *(*class_loader_func)(HObject *loader, char *name, long resolve) = 0;

ClassClass *
FindClass(struct execenv *ee, char *name, bool_t resolve)
{
    return FindClassFromClass(ee, name, resolve, 0);
}

/*
 * lock_classes and unlock_classes are exported by the JIT interface,
 * but no longer used anywhere else. It grabs both locks to ensure 
 * backward compatibility with 1.0.2.
 */
void
lock_classes()
{
    LOADCLASS_LOCK();
    BINCLASS_LOCK();
}

void
unlock_classes()
{
    BINCLASS_UNLOCK();
    LOADCLASS_UNLOCK();
}

ClassClass *
FindClassFromClass(struct execenv *ee, char *name, bool_t resolve,
           ClassClass *from)
{
    ClassClass *cb = 0;

    if (name[0] == SIGNATURE_ARRAY) {
    cb = Locked_FindArrayClassFromClass(ee, name, from) ;
    if (cb && !exceptionOccurred(ee ? ee : EE()))
        return InitializeAndResolveClass(cb, resolve);
        return NULL;
    } else if (from != NULL && cbLoader(from)) {
    /* unlock the classes while the class is located by
     * the class loader. The class loader should do this
     * in a synchronized method
     */
    return ClassLoaderFindClass(ee, cbLoader(from), name, resolve);
    } else {
        extern char *current_class_name;
        ClassClass *result = NULL;
        char *old_class_name = current_class_name;

        current_class_name = name;

        LOADCLASS_LOCK();
    cb = FindLoadedClass(name, 0);
    if (cb == 0) {
        /* Check if there was an error; bail if so */
        if (ee == NULL)
        ee = EE();
        if (!exceptionOccurred(ee))
        cb = LoadClassLocally(name);
    }
    LOADCLASS_UNLOCK();
    if (cb && !exceptionOccurred(ee ? ee : EE())) { 
        result = InitializeAndResolveClass(cb, resolve);
        }
        current_class_name = old_class_name;
        return result;
    }
}

/*
 * FindStickySystemClass is a variant of FindClassFromClass that makes the 
 * class sticky (immune to class GC) if the class can be found.  It is only
 * used on system classes, i.e. classes with no class loader, so it's 
 * equivalent to FindClassFromClass with "from" argument 0.  It returns 0
 * if the class can't be found, and like FindClass assumes that the caller
 * will deal with that.  Note that until the class has been made sticky it
 * must be kept somewhere where GC will find it.
 */
ClassClass *
FindStickySystemClass(struct execenv *ee, char *name, bool_t resolve)
{
    ClassClass *cb;
    if ((cb = FindClassFromClass(ee, name, resolve, 0)) != NULL) {
    MakeClassSticky(cb);
    }
    return cb;
}

/* Find the array class with the specified name.  */
static ClassClass *
Locked_FindArrayClassFromClass(struct execenv *ee, char *name, 
                   ClassClass *from) { 
    struct Hjava_lang_ClassLoader *fromLoader = 
            (from  == NULL) ? NULL : cbLoader(from);
    char *name_p;
    int depth, base_type;
    ClassClass *cb;
    struct Hjava_lang_ClassLoader *loader;
    ClassClass *inner_cb;

    sysAssert(name[0] == SIGNATURE_ARRAY);

    if (fromLoader == NULL) { 
    /* quick optimization if we know that we don't need a class loader */
    cb = FindLoadedClass(name, NULL);
    if (cb != NULL) 
        return cb;
    }
        

    /* Strip off all the initial SIGNATURE_ARRAY's to determine the depth,
     * and also what's left.  When we're done, name_p points at the first
     * non-character, and depth is the count of the array depth
     */
    for (name_p = name, depth = 0; *name_p == SIGNATURE_ARRAY; 
     name_p++, depth++);

    /* Look at the character to determine what type of array we have. */
    switch(*name_p++) {
        case SIGNATURE_INT:    base_type = T_INT; break;
    case SIGNATURE_LONG:   base_type = T_LONG; break;
    case SIGNATURE_FLOAT:  base_type = T_FLOAT; break;
    case SIGNATURE_DOUBLE: base_type = T_DOUBLE; break;
    case SIGNATURE_BOOLEAN:base_type = T_BOOLEAN; break; 
    case SIGNATURE_BYTE:   base_type = T_BYTE; break;
    case SIGNATURE_CHAR:   base_type = T_CHAR; break;
    case SIGNATURE_SHORT:  base_type = T_SHORT; break;
    case SIGNATURE_CLASS:  base_type = T_CLASS; break;
        default:           return NULL;    /* bad signature */
    }
    if (base_type == T_CLASS) { 
    char buffer_space[50];
    char *buffer = buffer_space;
    char *endChar = strchr(name_p, SIGNATURE_ENDCLASS);
    int nameLength = endChar - name_p;
    if (endChar == NULL || endChar[1] != '\0')
        return NULL;
    if (nameLength >= sizeof(buffer_space)) /* need bigger buffer */
        buffer = sysMalloc(nameLength + 1);
    memcpy(buffer, name_p, nameLength);
    buffer[nameLength] = '\0';
    inner_cb = FindClassFromClass(ee, buffer, FALSE, from);
    if (buffer != buffer_space) /* free buffer, if we malloc'ed it */
        sysFree(buffer);
    if (inner_cb == NULL) 
        return NULL;
    /* loader should either be fromLoader or NULL. */
    loader = cbLoader(inner_cb);
    } else { 
    if (*name_p != '\0')            /* bad signature */
        return NULL;
    inner_cb = NULL;
    loader = NULL;
    }

    LOADCLASS_LOCK();
    cb = FindLoadedClass(name, loader);
    if (cb == NULL) { 
    /* Create the fake array class corresponding to this.
     */
    cb = createFakeArrayClass(name, base_type, depth, inner_cb, loader);
    } 
    LOADCLASS_UNLOCK();
    return cb;
}

/*
 * Convert a name and type to a hash code
 *
 * We make copies of all strings that go into the hash table
 * in case the class that is the source of the strings being
 * inserted is eventually unloaded.
 */
unsigned NameAndTypeToHash(char *name, char *type)
{
    extern struct StrIDhash *nameTypeHash;

    unsigned name_key;
    unsigned type_key;
    unsigned ret;

    NAMETYPEHASH_LOCK();
    if (((name_key = Str2ID(&nameTypeHash, name, 0, TRUE)) == 0) ||
    ((type_key = Str2ID(&nameTypeHash, type, 0, TRUE)) == 0)) {
    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
    ret = 0;
    } else {
    ret = (name_key << 16) + type_key;
    }
    NAMETYPEHASH_UNLOCK();

    return ret;
}

/*
 * Pseudo-classes to represent primitive Java types.
 */

ClassClass *class_void;
ClassClass *class_boolean;
ClassClass *class_byte;
ClassClass *class_char;
ClassClass *class_short;
ClassClass *class_int;
ClassClass *class_long;
ClassClass *class_float;
ClassClass *class_double;

struct primtype {
    char        *name;
    char        typesig;
    unsigned char    typecode;
    unsigned char    slotsize;
    unsigned char    elementsize;
    ClassClass        **cellp;
} const primitive_types[] = {
    { "void",    SIGNATURE_VOID,       T_VOID,    0, 0,    &class_void },
    { "boolean",SIGNATURE_BOOLEAN, T_BOOLEAN,    4, 1,    &class_boolean },
    { "byte",    SIGNATURE_BYTE,       T_BYTE,    4, 1,    &class_byte },
    { "char",    SIGNATURE_CHAR,       T_CHAR,    4, 2,    &class_char },
    { "short",    SIGNATURE_SHORT,   T_SHORT,    4, 2,    &class_short },
    { "int",    SIGNATURE_INT,       T_INT,    4, 4,    &class_int },
    { "long",    SIGNATURE_LONG,       T_LONG,    8, 8,    &class_long },
    { "float",    SIGNATURE_FLOAT,   T_FLOAT,    4, 4,    &class_float },
    { "double",    SIGNATURE_DOUBLE,  T_DOUBLE,    8, 8,    &class_double }
};

#define    PRIMITIVE_TYPE_COUNT    \
    (sizeof (primitive_types) / sizeof (primitive_types[0]))

#define    GET_PRIMITIVE_CLASS(nm) \
    (class_##nm ? class_##nm : FindPrimitiveClass(#nm))

ClassClass *
FindPrimitiveClass(char *name)
{
    int i;
    ClassClass *cb;
    const struct primtype *p;
    
    for (i = 0; i < PRIMITIVE_TYPE_COUNT; i++) {
    p = &primitive_types[i];
    if (strcmp(name, p->name) == 0) {
        cb = *p->cellp;
        if (cb == NULL) {
        char *err, *detail = NULL;
        cb = createPrimitiveClass(p->name, p->typesig, p->typecode,
            p->slotsize, p->elementsize);
        sysAssert(cb != NULL);
        if ((err = InitializeClass(cb, &detail)) != NULL)
            return NULL; /* IMPL_NOTE */
        if ((err = ResolveClass(cb, &detail)) != NULL)
            return NULL; /* IMPL_NOTE*/
        *p->cellp = cb;
        }
        return cb;
    }
    }
    return NULL;
}

static void
InitPrimitiveClasses()
{
    int i;
    for (i = 0; i < PRIMITIVE_TYPE_COUNT; i++)
    FindPrimitiveClass(primitive_types[i].name);
}
