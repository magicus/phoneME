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
/*
 * Java object header format
 */

#ifndef _OOBJ_H_
#define _OOBJ_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <typedefs.h>
#include <signature.h>

#define JAVA_CLASSFILE_MAGIC              0xCafeBabe

#define JAVASRCEXT "java"
#define JAVASRCEXTLEN 4
#define JAVAOBJEXT "class"
#define JAVAOBJEXTLEN 5

#define JAVA_VERSION     45
#define JAVA_MINOR_VERSION 3
#define ARRAYHEADER     long alloclen

/* The size (in bytes) of a statically allocated area that the
 * verifier uses internally in various string operations.
 */
#ifndef STRINGBUFFERSIZE
#define STRINGBUFFERSIZE 512
#endif

#define MAXOPTIONS 2048   /* maximum # of parameters */

#define MAXPATHLEN 255

/* maximum size allowed for package name of a class file */
#define MAXPACKAGENAME 1024  

#define HandleTo(T) typedef struct H##T { Class##T *obj; struct methodtable *methods;} H##T

typedef long OBJECT;
typedef OBJECT Classjava_lang_Object;
typedef OBJECT ClassObject;
HandleTo(java_lang_Object);
typedef Hjava_lang_Object JHandle;
typedef Hjava_lang_Object HObject;

typedef unsigned short unicode;

extern unicode    *str2unicode(char *, unicode *, long);
extern char    *int642CString(int64_t number, char *buf, int buflen);

#define ALIGN(n) (((n)+3)&~3)
#define UCALIGN(n) ((unsigned char *)ALIGN((int)(n)))

struct Hjava_lang_Class;    /* forward reference for some compilers */
struct Classjava_lang_Class;    /* forward reference for some compilers */

typedef struct Classjava_lang_Class Classjava_lang_Class;

HandleTo(java_lang_Class);
typedef struct Hjava_lang_Class ClassClass;

struct fieldblock {
    ClassClass *clazz;
    char *signature;
    char *name;
    unsigned long ID;
    unsigned short access;
    union {
    unsigned long offset;    /* info of data */    
    OBJECT static_value;
    void *static_address;
    } u;
    unsigned deprecated:1;
    unsigned synthetic:1;
};

#define fieldname(fb)    ((fb)->name)
#define fieldsig(fb)     ((fb)->signature)
#define fieldIsArray(fb) (fieldsig(fb)[0] == SIGNATURE_ARRAY)
#define fieldIsClass(fb) (fieldsig(fb)[0] == SIGNATURE_CLASS)
#define    fieldclass(fb)   ((fb)->clazz)

struct execenv;

#define VERIFIER_TOOL

/* Includes/eliminates a large amount of optional debugging code
 * (such as class and stack frame printing operations) in/from
 * the system.  By eliminating debug code, the system will be smaller.
 * Inclusion of debugging code increases the size of the system but
 * does not introduce any performance overhead unless calls to the
 * debugging routines are added to the source code or various tracing
 * options are turned on.
 */

#ifndef INCLUDEDEBUGCODE
#define INCLUDEDEBUGCODE 0
#endif

#define DEBUG_VERIFIER 1
#define DEBUG_READFROMFILE 0 

enum {
    ITEM_Bogus,
    ITEM_Void,            /* only as a function return value */
    ITEM_Integer,
    ITEM_Float,
    ITEM_Double,
    ITEM_Double_2,        /* 2nd word of double in register */
    ITEM_Long,
    ITEM_Long_2,        /* 2nd word of long in register */
    ITEM_InitObject,        /* "this" is init method, before call 
                    to super() */
    ITEM_Object,        /* Extra info field gives name. */
    ITEM_NewObject,        /* Like object, but uninitialized. */
    ITEM_ReturnAddress,        /* Extra info gives instr # of start pc */
    /* The following three are only used within array types. 
     * Normally, we use ITEM_Integer, instead. */
    ITEM_Boolean,
    ITEM_Byte,
    ITEM_Short,
    ITEM_Char
};

/* These are the constants that appear in class files. */
enum {
    CF_ITEM_Bogus,
    CF_ITEM_Integer,
    CF_ITEM_Float,
    CF_ITEM_Double,
    CF_ITEM_Long,
    CF_ITEM_Null,
    CF_ITEM_InitObject,

    CF_ITEM_Object,
    CF_ITEM_NewObject,
};

struct map_entry {
    unsigned char type;
    long info;
};

struct stack_map {
    int offset;
    int nlocals;
    struct map_entry *locals;
    int nstacks;
    struct map_entry *stacks;
};

struct methodblock {
    struct fieldblock fb;
    unsigned char       *code;    /* the code */
    struct CatchFrame   *exception_table;
    struct lineno       *line_number_table;
    struct localvar     *localvar_table;

    unsigned long        code_length;
    unsigned long        exception_table_length;
    unsigned long        line_number_table_length;
    unsigned long        localvar_table_length;

    bool_t  (*invoker)
      (JHandle *o, struct methodblock *mb, int args_size, struct execenv *ee);
    unsigned short       args_size;    /* total size of all arguments */
    unsigned short       maxstack;    /* maximum stack usage */
    unsigned short       nlocals;    /* maximum number of locals */
    /* 2 spare bytes here */
    void                *CompiledCode; /* it's type is machine dependent */
    void                *CompiledCodeInfo; /* it's type is machine dependent */
    long                 CompiledCodeFlags; /* machine dependent bits */
    unsigned long        inlining;      /* possible inlining of code */
    unsigned short       nexceptions;   /* number of checked exceptions */
    unsigned short       *exceptions;    /* constant pool indices */
    char                 *mp_marks;
    int                  n_stack_maps;
    struct stack_map     *stack_maps;
};

struct HIOstream;

struct methodtable {
    ClassClass *classdescriptor;
    struct methodblock *methods[1];
};

struct imethodtable { 
    int icount;            /* number of interfaces to follow */
    struct { 
    ClassClass *classdescriptor;
    unsigned long *offsets;    /* info of data */    
    } itable[1];
};

typedef struct {
    char body[1];
} ArrayOfByte;
typedef ArrayOfByte ClassArrayOfByte;
HandleTo(ArrayOfByte);

typedef struct {
    unicode body[1];
} ArrayOfChar;
typedef ArrayOfChar ClassArrayOfChar;
HandleTo(ArrayOfChar);

typedef struct {
    signed short body[1];
} ArrayOfShort;
typedef ArrayOfShort ClassArrayOfShort;
HandleTo(ArrayOfShort);

typedef struct {
    long        body[1];
} ArrayOfInt;
typedef ArrayOfInt ClassArrayOfInt;
HandleTo(ArrayOfInt);

typedef struct {
    int64_t        body[1];
} ArrayOfLong;
typedef ArrayOfLong ClassArrayOfLong;
HandleTo(ArrayOfLong);

typedef struct {
    float       body[1];
} ArrayOfFloat;
typedef ArrayOfFloat ClassArrayOfFloat;
HandleTo(ArrayOfFloat);

typedef struct {
    double       body[1];
} ArrayOfDouble;
typedef ArrayOfDouble ClassArrayOfDouble;
HandleTo(ArrayOfDouble);

typedef struct {
    JHandle *(body[1]);
} ArrayOfArray;
typedef ArrayOfArray ClassArrayOfArray;
HandleTo(ArrayOfArray);

typedef struct {
    HObject *(body[1]);
} ArrayOfObject;
typedef ArrayOfObject ClassArrayOfObject;
HandleTo(ArrayOfObject);

typedef struct Hjava_lang_String HString;

typedef struct {
    HString  *(body[1]);
} ArrayOfString;
typedef ArrayOfString ClassArrayOfString;
HandleTo(ArrayOfString);

/* Note: any handles in this structure must also have explicit
   code in the ScanClasses() routine of the garbage collector
   to mark the handle. */
struct Classjava_lang_Class {
    /* Things following here are saved in the .class file */
    unsigned short         major_version;
    unsigned short         minor_version;
    char                    *name;
    char                    *super_name;
    char                    *source_name;
    ClassClass              *superclass;
    ClassClass              *HandleToSelf;
    struct Hjava_lang_ClassLoader *loader;
    struct methodblock        *finalizer;

    union cp_item_type      *constantpool;
    struct methodblock      *methods;
    struct fieldblock       *fields;
    short                   *implements;

    struct methodtable      *methodtable;
    struct methodtable        *methodtable_mem;
    struct fieldblock      **slottable;

    HString            *classname;

    union {
    struct {
        unsigned long    thishash;      /* unused */
        unsigned long    totalhash;      /* unused */
    } cbhash;
    struct {
        unsigned char    typecode;      /* VM typecode */
        char        typesig;      /* signature constant */
        unsigned char    slotsize;      /* (bytes) in slot */
        unsigned char    elementsize;      /* (bytes) in array */
        unsigned long    xxspare;
    } cbtypeinfo;
    } hashinfo;

    unsigned short           constantpool_count;  /* number of items in pool */
    unsigned short           methods_count;       /* number of methods */
    unsigned short           fields_count;        /* number of fields */
    unsigned short           implements_count;    /* number of protocols */

    unsigned short           methodtable_size;    /* the size of method table */
    unsigned short           slottbl_size;        /* size of slottable */
    unsigned short           instance_size;       /* (bytes) of an instance */

    unsigned short access;           /* how this class can be accesses */
    unsigned short flags;         /* see the CCF_* macros */
    struct HArrayOfObject   *signers;
    struct   imethodtable   *imethodtable;

    void                    *init_thread; /* EE of initializing thread */    

    ClassClass              *last_subclass_of;
    void            *reserved_for_jit;
    char                    *absolute_source_name;
    struct { int high, low; } timestamp;
    int                     n_new_class_entries;
    char                    **new_class_entries;
    int                     n_new_utf8_entries;
    char                    **new_utf8_entries;
    int                has_stack_maps;

    int                     inner_classes_count;
    struct innerClasses    *inner_classes;

    unsigned deprecated:1;
    unsigned synthetic:1;
    unsigned hasTimeStamp:1;
};

struct innerClasses { 
    int inner_class;
    int outer_class;
    char *inner_name;
    int access;
};

extern void FreeClass(ClassClass *cb);
extern void MakeClassSticky(ClassClass *cb);

#define cbAccess(cb)          ((unhand(cb))->access)
#define cbClassname(cb)       ((unhand(cb))->classname)
#define cbConstantPool(cb)    ((unhand(cb))->constantpool)
#define cbConstantPoolCount(cb) ((unhand(cb))->constantpool_count)
#define    cbFields(cb)          ((unhand(cb))->fields)
#define    cbFieldsCount(cb)     ((unhand(cb))->fields_count)
#define cbFinalizer(cb)       ((unhand(cb))->finalizer)
#define cbFlags(cb)           ((unhand(cb))->flags)
#define cbHandle(cb)          (cb)
#define cbImplements(cb)      ((unhand(cb))->implements)
#define cbImplementsCount(cb) ((unhand(cb))->implements_count)
#define cbInstanceSize(cb)    ((unhand(cb))->instance_size)
#define cbIntfMethodTable(cb) ((unhand(cb))->imethodtable)
#define cbLastSubclassOf(cb)  ((unhand(cb))->last_subclass_of)
#define    cbLoader(cb)          ((unhand(cb))->loader)
#define cbMajorVersion(cb)    ((unhand(cb))->major_version)
#define    cbMethods(cb)         ((unhand(cb))->methods)
#define    cbMethodsCount(cb)    ((unhand(cb))->methods_count)
#define cbMethodTable(cb)     ((unhand(cb))->methodtable)
#define cbMethodTableMem(cb)  ((unhand(cb))->methodtable_mem)
#define cbMethodTableSize(cb) ((unhand(cb))->methodtable_size)
#define cbMinorVersion(cb)    ((unhand(cb))->minor_version)
#define cbName(cb)            ((unhand(cb))->name)
#define cbSigners(cb)         ((unhand(cb))->signers)
#define cbSlotTable(cb)       ((unhand(cb))->slottable)
#define cbSlotTableSize(cb)   ((unhand(cb))->slottbl_size)
#define cbSourceName(cb)      ((unhand(cb))->source_name)
#define cbSuperclass(cb)      ((unhand(cb))->superclass)
#define cbSuperName(cb)       ((unhand(cb))->super_name)
#define cbThisHash(cb)        ((unhand(cb))->hashinfo.cbhash.thishash)
#define cbTotalHash(cb)       ((unhand(cb))->hashinfo.cbhash.totalhash)
#define cbInitThread(cb)      ((unhand(cb))->init_thread)
#define cbInnerClasses(cb)     ((unhand(cb))->inner_classes)
#define cbInnerClassesCount(cb)((unhand(cb))->inner_classes_count)

#define cbAbsoluteSourceName(cb) ((unhand(cb))->absolute_source_name)
#define cbTimestamp(cb)       ((unhand(cb))->timestamp)

#define cbIsInterface(cb)        ((cbAccess(cb) & ACC_INTERFACE) != 0)
#define cbIsAbstract(cb)         ((cbAccess(cb) & ACC_ABSTRACT) != 0)
#define cbAccessNotAbstract(cb)  ((unhand(cb))->access & ~ACC_ABSTRACT)

/* These are currently only valid for primitive types */
#define    cbIsPrimitive(cb)      (CCIs(cb, Primitive))
#define cbTypeCode(cb)           ((unhand(cb))->hashinfo.cbtypeinfo.typecode)
#define cbTypeSig(cb)           ((unhand(cb))->hashinfo.cbtypeinfo.typesig)
#define cbSlotSize(cb)           ((unhand(cb))->hashinfo.cbtypeinfo.slotsize)
#define cbElementSize(cb)      ((unhand(cb))->hashinfo.cbtypeinfo.elementsize)

extern char *classname2string(char *str, char *dst, int size);

#define twoword_static_address(fb) ((fb)->u.static_address)
#define normal_static_address(fb)  (&(fb)->u.static_value)

/* ClassClass flags */
#define CCF_IsSysLock     0x01  /* any instance treated as a "system" lock */
#define CCF_IsResolved      0x02    /* has <clinit> been run? */
#define CCF_IsError      0x04    /* Resolution caused an error */
#define CCF_IsSoftRef      0x08    /* whether this is class Ref or subclass */
#define CCF_IsInitialized 0x10    /* whether this is class has been inited */
#define CCF_IsLinked      0x20    /* Has symbolic entries been linked */
#define CCF_IsVerified    0x40    /* has the verifier been run */

#define CCF_IsPrimitive   0x100    /* if pseudo-class for a primitive type */
#define CCF_IsReferenced  0x200 /* Class is in use */
#define CCF_IsSticky      0x400 /* Don't unload this class */

#define CCIs(cb,flag) (((unhand(cb))->flags & CCF_Is##flag) != 0)
#define CCSet(cb,flag) ((unhand(cb))->flags |= CCF_Is##flag)
#define CCClear(cb,flag) ((unhand(cb))->flags &= ~CCF_Is##flag)

/* map from pc to line number */
struct lineno {
    unsigned long pc, 
    line_number;
};

extern struct lineno *lntbl;
extern long lntsize, lntused;

/* Symbol table entry for local variables and parameters.
   pc0/length defines the range that the variable is valid, slot
   is the position in the local variable array in ExecEnv.
   nameoff and sigoff are offsets into the string table for the
   variable name and type signature.  A variable is defined with
   DefineVariable, and at that time, the node for that name is
   stored in the localvar entry.  When code generate is completed
   for a particular scope, a second pass it made to replace the
   src node entry with the correct length. */

struct localvar {
    long pc0;            /* starting pc for this variable */
    long length;        /* -1 initially, end pc - pc when we're done */
    char *name;                 /*  */
    char *signature;
    long slot;            /* local variable slot */
};

/* Try/catch is implemented as follows.  On a per class basis,
   there is a catch frame handler (below) for each catch frame
   that appears in the source.  It contains the pc range of the
   corresponding try body, a pc to jump to in the event that that
   handler is chosen, and a catchType which must match the object
   being thrown if that catch handler is to be taken.

   The list of catch frames are sorted by pc.  If one range is
   inside another, then outer most range (the one that encompasses
   the other) appears last in the list.  Therefore, it is possible
   to search forward, and the first one that matches is the
   innermost one.

   Methods with catch handlers will layout the code without the
   catch frames.  After all the code is generated, the catch
   clauses are generated and table entries are created.

   When the class is complete, the table entries are dumped along
   with the rest of the class. */

struct CatchFrame {
    long    start_pc, end_pc;    /* pc range of corresponding try block */
    long    handler_pc;            /* pc of catch handler */
    void*   compiled_CatchFrame; /* space to be used by machine code */
    short   catchType;            /* type of catch parameter */
};

#define MC_SUPER        (1<<5)
#define MC_NARGSMASK    (MC_SUPER-1)
#define MC_INT          (0<<6)
#define MC_FLOAT        (1<<6)
#define MC_VOID         (2<<6)
#define MC_OTHER        (3<<6)
#define MC_TYPEMASK     (3<<6)

enum {
    CONSTANT_Utf8 = 1,
    CONSTANT_Unicode,        /* unused */
    CONSTANT_Integer,
    CONSTANT_Float,
    CONSTANT_Long,      
    CONSTANT_Double,
    CONSTANT_Class,
    CONSTANT_String,
    CONSTANT_Fieldref,
    CONSTANT_Methodref,
    CONSTANT_InterfaceMethodref,
    CONSTANT_NameAndType
};

union cp_item_type {
    int i;
    float f;
    char *cp;
    unsigned char *type;        /* for type table */
    ClassClass *clazz;
    struct methodblock *mb;
    struct fieldblock *fb;
    struct Hjava_lang_String *str;
    void *p;                    /* for very rare occasions */
};

typedef union cp_item_type cp_item_type;

#define CONSTANT_POOL_ENTRY_RESOLVED 0x80
#define CONSTANT_POOL_ENTRY_TYPEMASK 0x7F
#define CONSTANT_POOL_TYPE_TABLE_GET(cp,i) (((unsigned char *)(cp))[i])
#define CONSTANT_POOL_TYPE_TABLE_PUT(cp,i,v) (CONSTANT_POOL_TYPE_TABLE_GET(cp,i) = (v))
#define CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(cp,i) \
    (CONSTANT_POOL_TYPE_TABLE_GET(cp,i) |= CONSTANT_POOL_ENTRY_RESOLVED)
#define CONSTANT_POOL_TYPE_TABLE_IS_RESOLVED(cp,i) \
    ((CONSTANT_POOL_TYPE_TABLE_GET(cp,i) & CONSTANT_POOL_ENTRY_RESOLVED) != 0)
#define CONSTANT_POOL_TYPE_TABLE_GET_TYPE(cp,i) \
        (CONSTANT_POOL_TYPE_TABLE_GET(cp,i) & CONSTANT_POOL_ENTRY_TYPEMASK)

#define CONSTANT_POOL_TYPE_TABLE_INDEX 0
#define CONSTANT_POOL_UNUSED_INDEX 1

/* The following are used by the constant pool of "array" classes. */

#define CONSTANT_POOL_ARRAY_DEPTH_INDEX 1
#define CONSTANT_POOL_ARRAY_TYPE_INDEX 2
#define CONSTANT_POOL_ARRAY_CLASS_INDEX 3
#define CONSTANT_POOL_ARRAY_LENGTH 4

/* 
 * Package shorthand: this isn't obviously the correct place.
 */
#define JAVAPKG         "java/lang/"
#define JAVAIOPKG       "java/io/"
#define JAVANETPKG      "java/net/"

#define unhand(o) ((o)->obj)

extern ClassClass *classJavaLangClass;       /* class java/lang/Class */
extern ClassClass *classJavaLangObject;       /* class java/lang/Object */
extern ClassClass *classJavaLangString;       /* class java/lang/String */

extern ClassClass *classJavaLangThrowable;
extern ClassClass *classJavaLangException;
extern ClassClass *classJavaLangError;
extern ClassClass *classJavaLangRuntimeException;
extern ClassClass *classJavaLangThreadDeath;

extern ClassClass *interfaceJavaLangCloneable; /* class java/lang/Cloneable */
extern ClassClass *interfaceJavaIoSerializable; /* class java/io/Serializable */

enum { VERIFY_NONE, VERIFY_REMOTE, VERIFY_ALL };

extern int verifyclasses;
extern bool_t verbose;
extern char * const opnames[];

int jio_snprintf(char *str, size_t count, const char *fmt, ...);
int jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args);

int jio_printf(const char *fmt, ...);
int jio_fprintf(FILE *, const char *fmt, ...);
int jio_vfprintf(FILE *, const char *fmt, va_list args);

struct StrIDhash;
unsigned short Str2ID(struct StrIDhash **, char *, void ***, int);
char *ID2Str(struct StrIDhash *, unsigned short, void ***);
void Str2IDFree(struct StrIDhash **);

unsigned NameAndTypeToHash(char *name, char *type);
bool_t IsSameClassPackage(ClassClass *class1, ClassClass *class2); 
unsigned Signature2ArgsSize(char *method_signature);
char *GetClassConstantClassName(cp_item_type *constant_pool, int index);
ClassClass *FindClass(struct execenv *ee, char *name, bool_t resolve);
ClassClass *FindClassFromClass(struct execenv *ee, char *name, 
                   bool_t resolve, ClassClass *from);

bool_t createInternalClass(unsigned char *bytes, unsigned char *limit,
                           ClassClass *cb, struct Hjava_lang_ClassLoader *,
                            char *utfname, char **detail);

bool_t VerifyClass(ClassClass *cb);
bool_t IsLegalClassname(char *name, bool_t allowArrayClass);
bool_t verify_class_codes(ClassClass *cb);

struct execenv {
    /* Detecting class circularities */
    struct seenclass {
    ClassClass    *cb;
    struct seenclass *next;
    } seenclasses;

    /* error message occurred during class loading */ 
    char *class_loading_msg;
};

typedef struct execenv ExecEnv;

extern ExecEnv * EE();

ClassClass *FindStickySystemClass(struct execenv *, char *, bool_t resolve);
bool_t VerifyClassAccess(ClassClass *, ClassClass *, bool_t);
void InitializeInvoker(ClassClass *cb);
bool_t RunStaticInitializers(ClassClass *cb);
ClassClass *ClassLoaderFindClass(ExecEnv *ee, 
                 struct Hjava_lang_ClassLoader *loader, 
                 char *name, bool_t resolve);
ClassClass *LoadClassLocally(char *name);
ClassClass *createFakeArrayClass(char *name, int base_type, int depth, 
                 ClassClass *inner_cb, 
                 struct Hjava_lang_ClassLoader *);
ClassClass *createPrimitiveClass(char *name, char sig, unsigned char typecode,
    unsigned char slotsize, unsigned char elementsize);

bool_t isJARfile(char *fn, int length);

#define BINCLASS_LOCK()
#define BINCLASS_UNLOCK()
#define LOADCLASS_LOCK()
#define LOADCLASS_UNLOCK()
#define NAMETYPEHASH_LOCK()
#define NAMETYPEHASH_UNLOCK()

#define FINALIZER_METHOD_NAME "finalize"
#define FINALIZER_METHOD_SIGNATURE "()V"

#define CLS_RESLV_INIT_CLASS    "java/lang/Class"
#define CLS_RESLV_INIT_OBJECT   "java/lang/Object"
#define CLS_RESLV_INIT_REF      "sun/misc/Ref"

#define METHOD_FLAG_BITS 5
#define FLAG_MASK       ((1<<METHOD_FLAG_BITS)-1)  /* valid flag bits */
#define METHOD_MASK     (~FLAG_MASK)  /* valid mtable ptr bits */
#define LENGTH_MASK     METHOD_MASK

#define mt_slot(methodtable, slot) (methodtable)->methods[slot]

#define monitorEnter(m)
#define monitorExit(m)

#define exceptionOccurred(ee) 0
void SignalError(struct execenv *, char *, char *);

void panic(const char *format, ...);

void printCurrentClassName(void);

void ensure_dir_exists(char *dir);
void ensure_dir_writable(char *dir);

extern bool_t no_native_methods;
extern bool_t no_floating_point;
extern bool_t no_finalizers;

extern int errorCode;         /* status error returned by program*/
                              /* Set to 1 if any errors encountered 
                               * during class verification in VerifyFile()
                               */
#endif /* !_OOBJ_H_ */
