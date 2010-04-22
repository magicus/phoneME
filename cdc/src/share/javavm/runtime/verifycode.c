/*
 * @(#)verifycode.c	1.165 06/10/10
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
 
/*-
 *      Verify that the code within a method block doesn't exploit any 
 *      security holes.
 */
/*
   Exported function:

   jint
   VerifyClass(CVMExecEnv*, CVMClassBlock *cb, char *message_buffer,
               jint buffer_length)

 */

#ifdef CVM_CLASSLOADING
#ifndef CVM_TRUSTED_CLASSLOADERS	

#include "javavm/include/clib.h"
#include "javavm/include/porting/ansi/setjmp.h"
#include "javavm/include/porting/ansi/limits.h"
#include "javavm/include/indirectmem.h"

#include "jni.h"
#include "jvm.h"
#include "javavm/include/verify_impl.h"
#include "javavm/include/verify.h"
#include "javavm/include/globals.h"

#include "javavm/include/opcodes.h"
#include "opcodes.in_out"

#ifdef __cplusplus
#define this XthisX
#define new XnewX
#define class XclassX
#define protected XprotectedX
#endif

static const unsigned char *
JVM_GetMethodByteCode(CVMMethodBlock* mb)
{
    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
    if (jmd == NULL) {
	return NULL;
    } else {
	return CVMmbJavaCode(mb);
    }
}

static jint
JVM_GetMethodByteCodeLength(CVMMethodBlock* mb)
{
    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
    if (jmd == NULL) {
	return 0;
    } else {
	return CVMjmdCodeLength(jmd);
    }
}

static CVMExceptionHandler *
JVM_GetMethodExceptionTableEntry(CVMMethodBlock* mb, jint entry_index)
{
    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
    return CVMjmdExceptionTable(jmd) + entry_index;
}


static jint
JVM_GetMethodExceptionTableLength(CVMMethodBlock* mb)
{
    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
    if (jmd == NULL) {
	return 0;
    } else {
	return CVMjmdExceptionTableLength(jmd);
    }
}

static jint
JVM_GetMethodLocalsCount(CVMMethodBlock* mb)
{
    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
     if (jmd == NULL) {
	return 0;
    } else {
	return CVMjmdMaxLocals(jmd);
    }
}

#ifdef CVM_TRACE

static CVMFieldTypeID
JVM_GetCPFieldName(CVMConstantPool* cp, jint cp_index)
{
    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cp_index);
    return CVMcpGetFieldTypeID(cp, typeIDIdx);
}

#endif

static CVMMethodTypeID
JVM_GetCPMethodName(CVMConstantPool* cp, jint cp_index)
{
    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cp_index);
    return CVMcpGetMethodTypeID(cp, typeIDIdx);
}

static CVMClassTypeID
JVM_GetCPFieldSignature(CVMConstantPool* cp, jint cp_index)
{
    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cp_index);
    return CVMtypeidGetType(CVMcpGetFieldTypeID(cp, typeIDIdx));
}

static CVMMethodTypeID
JVM_GetCPMethodSignature(CVMConstantPool* cp, jint cp_index)
{
    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cp_index);
    return CVMcpGetMethodTypeID(cp, typeIDIdx);
}

static CVMClassTypeID
JVM_GetCPClassName(CVMExecEnv* ee, CVMConstantPool* cp, jint classIdx)
{
    CVMClassTypeID classID;
    if (CVMcpTypeIs(cp, classIdx, ClassBlock)) {
	classID = CVMcbClassName(CVMcpGetCb(cp, classIdx));
    } else {
	CVMcpLock(ee);
	if (CVMcpTypeIs(cp, classIdx, ClassBlock)) {
	    classID = CVMcbClassName(CVMcpGetCb(cp, classIdx));
	} else {
	    classID = CVMcpGetClassTypeID(cp, classIdx);
	}
	CVMcpUnlock(ee);
    }
    return classID;
}

static CVMClassTypeID
JVM_GetCPFieldClassName(CVMExecEnv* ee, CVMConstantPool* cp, jint cp_index)
{
    CVMUint16 classIdx = CVMcpGetMemberRefClassIdx(cp, cp_index);
    return JVM_GetCPClassName(ee, cp, classIdx);
}

static CVMClassTypeID
JVM_GetCPMethodClassName(CVMExecEnv* ee, CVMConstantPool* cp, jint cp_index)
{
    CVMUint16 classIdx = CVMcpGetMemberRefClassIdx(cp, cp_index);
    return JVM_GetCPClassName(ee, cp, classIdx);
}

static jint
JVM_GetCPFieldModifiers(CVMConstantPool* cp, int cp_index, 
			CVMClassBlock *cbCalled)
{
    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cp_index);
    CVMFieldTypeID typeID = CVMcpGetFieldTypeID(cp, typeIDIdx);
    int i;
    for (i = 0; i < CVMcbFieldCount(cbCalled); i++) {
	CVMFieldBlock* fb = CVMcbFieldSlot(cbCalled, i);
	if (CVMtypeidIsSame(CVMfbNameAndTypeID(fb), typeID)) {
	    return CVMfbAccessFlags(fb);
	}
    }
    return -1;
}
    
static jint
JVM_GetCPMethodModifiers(CVMConstantPool* cp, int cp_index,
			 CVMClassBlock *cbCalled)
{
    CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cp_index);
    CVMMethodTypeID typeID = CVMcpGetMethodTypeID(cp, typeIDIdx);
    int i;
    for (i = 0; i < CVMcbMethodCount(cbCalled); i++) {
	CVMMethodBlock* mb = CVMcbMethodSlot(cbCalled, i);
	if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), typeID)) {
	    return CVMmbAccessFlags(mb);
	}
    }
    return -1;
}

typedef CVMOpcode opcode_type;

#define MAX_ARRAY_DIMENSIONS 255
/* align byte code */
#ifndef ALIGN_UP
#define ALIGN_UP(n,align_grain) (((n) + ((align_grain) - 1)) & ~((align_grain)-1))
#endif /* ALIGN_UP */
/* 
 * Casts from pointer types to scalar types have to
 * be casts to the type CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms.
 */
#define UCALIGN(n) ((unsigned char *)ALIGN_UP((CVMAddr)(n),sizeof(int)))

#ifdef CVM_TRACE
#define verify_verbose (CVMcheckDebugFlags(CVM_DEBUGFLAG(TRACE_VERIFIER)) != 0)
#endif

/*
 * an analyzed instruction's stack size and register count are initially unknown.
 */
#define UNKNOWN_STACK_SIZE -1
#define UNKNOWN_REGISTER_COUNT -1

/*
 * The first time we encounter a jsr instruction, we set its operand2 field to the
 * following. Thereafter we use the instruction index of the ret instruction
 * corresponding to it. This helps us analyze flow and enforce the
 * correspondence between subroutines and ret instructions.
 */
#define UNKNOWN_RET_INSTRUCTION -1

#undef MAX
#undef MIN 
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BITS_PER_INT   (CHAR_BIT * sizeof(int)/sizeof(char))
#define SET_BIT(flags, i)  (flags[(i)/BITS_PER_INT] |= \
			               ((unsigned)1 << ((i) % BITS_PER_INT)))
#define	IS_BIT_SET(flags, i) (flags[(i)/BITS_PER_INT] & \
			               ((unsigned)1 << ((i) % BITS_PER_INT)))

typedef unsigned int *bitvector;

/* A hash mechanism used by the verifier.
 * Maps class names to unique 16 bit integers.
 * This is very similar to the classic VM's string intern table.
 * The good thing about this one is that it is part of the context
 * so requires less management when we're done.
 */

#define HASH_TABLE_SIZE 503

/* The buckets are managed as a 256 by 256 matrix. We allocate an entire
 * row (256 buckets) at a time to minimize fragmentation. Rows are 
 * allocated on demand so that we don't waste too much space.
 */
 
#define MAX_HASH_ENTRIES 65536
#define HASH_ROW_SIZE 256

typedef struct hash_bucket_type {
    CVMClassTypeID name;  /* also used as the hash */
    CVMClassBlock* class;
    unsigned short next;
    unsigned loadable:1;  /* from context->class loader */
} hash_bucket_type;

typedef struct {
    hash_bucket_type **buckets;
    unsigned short *table;
    int entries_used;
} hash_table_type;

#define GET_BUCKET(class_hash, ID)\
    (class_hash->buckets[ID / HASH_ROW_SIZE] + ID % HASH_ROW_SIZE)

#undef CVM_VERIFY_OPERAND_BUF_SIZE /* operand buffer size used in pop_stack */
#define CVM_VERIFY_OPERAND_BUF_SIZE 257
#undef CVM_VERIFY_TYPE_BUF_SIZE    /* type buffer size used in pop_stack */
#define CVM_VERIFY_TYPE_BUF_SIZE 256

/* The context type encapsulates the current invocation of the byte
 * code verifier. This permits simultaneous invocation of it in multiple threads.
 */
struct context_type {

    CVMExecEnv *ee;             /* current CVMExecEnv */

    /* buffers etc. */
    char stack_operand_buffer[CVM_VERIFY_OPERAND_BUF_SIZE];
    fullinfo_type stack_info_buffer[CVM_VERIFY_TYPE_BUF_SIZE];
    char *message;
    jint message_buf_len;

    /* these fields are per class */
    CVMClassBlock *class;		/* current class */
    CVMUint16 nconstants;
    CVMConstantPool *constant_pool;
    hash_table_type class_hash;

    fullinfo_type object_info;	/* fullinfo for java/lang/Object */
    fullinfo_type string_info;	/* fullinfo for java/lang/String */
    fullinfo_type class_info;	/* fullinfo for java/lang/Class */
    fullinfo_type throwable_info; /* fullinfo for java/lang/Throwable */
    fullinfo_type cloneable_info; /* fullinfo for java/lang/Cloneable */
    fullinfo_type serializable_info; /* fullinfo for java/io/Serializable */

    fullinfo_type currentclass_info; /* fullinfo for context->class */
    fullinfo_type superclass_info;   /* fullinfo for superclass */

    /* these fields are per method */
    CVMMethodBlock *mb;	        /* current method */
    const unsigned char *code;	/* current code object */
    jint code_length;
    int *code_data;		/* offset to instruction number */
    struct instruction_data_type *instruction_data; /* info about each */
    struct handler_info_type *handler_info;
    fullinfo_type *superclasses; /* null terminated superclasses */
    int instruction_count;	/* number of instructions */
    fullinfo_type return_type;	/* function return type */
    fullinfo_type swap_table[4]; /* used for passing information */
    int bitmask_size;		/* words needed to hold bitmap of arguments */
    CVMBool	method_has_jsr;
    CVMBool	method_has_switch;

#if 0 /* See comment in verify_field(). */
    /* these fields are per field */
    CVMFieldBlock *fb;
#endif

    /* Used by the space allocator */
    struct CCpool *CCroot, *CCcurrent;
    char *CCfree_ptr;
    int CCfree_size;

    /* Jump here on any error. */
    jmp_buf jump_buffer;

    /* Class file version we are checking against */
    int major_version;

    jint err_code;
};

/*
 * The state of the stack is a linked list of stack_item_type.
 * The first item is the stack top: lower items are found by
 * following the next pointer. To pop an item from the stack
 * is to replace the "stack" pointer by stack->next and decrement
 * stack_size. To push an item, use
 *	stack_item_type *stack_item = NEW(stack_item_type, 1);
 *	stack_item->next = s->stack;
 *	s->stack = stack_item;
 *	s->stack_size++;
 * obviously, see stack_item_type below.
 */
struct stack_info_type {
    struct stack_item_type *stack;
    int stack_size;
};

/*
 * For each of the register_count registers we know about at this point,
 * a fullinfo_type gives what we know about the content. See above,
 * especially ITEM_ReturnAddress.
 * There is also an array of mask_count mask bitsets. Frequently
 * mask_count == 0. The sets must be at least register_count long.
 * The registers information, as well as the masks information, is
 * modified at every store into a local "register."
 * See mask_type.
 */
struct register_info_type {
    int register_count;		/* number of registers used */
    fullinfo_type *registers;
    int mask_count;		/* number of masks in the following */
    struct mask_type *masks;
};

/*
 * For most instructions, there are multiple code paths from the beginning of the
 * method to the start of the instruction. For code paths which can be
 * reached by way of a jsr (and are thus part of a subroutine as defined by a jsr/ret pair),
 * we keep a bit mask of registers modified (by store instruction) during the subroutine.
 * This helps us merge state information from the ret back to the instruction after the jsr.
 * Because a piece of code can possibly be reached through multiple subroutine calls (think
 * nested try...finally blocks), we have to compute one of these masks for each
 * path. Thus each struct mask_type has an entry field, giving the instruction number of
 * a subroutine entry point, i.e. an instruction which is the target of a jsr.
 */
struct mask_type {
    int entry;
    int *modifies;
};

typedef unsigned short flag_type;

/*
 * Before any dataflow is done, the instructions are unpacked into struct instruction_data_type.
 * Some opcodes are folded together because it is not important to distinguish between them,
 * and at lease one opcode is split off as a special case: opc_invokeinit.
 * Operands are broken out and unpacked.
 * During the course of computation, additional information is added (see use of operand2
 * for the jsr), and it is decorated with the entire state of the computation BEFORE EXECUTION
 * of the instruction.
 * All references to the instruction stream are converted into instruction numbers, which are
 * indexes into an array of this structure.
 */
struct instruction_data_type {
    opcode_type opcode;		/* may turn into "canonical" opcode */
    unsigned changed:1;		/* has it changed */
    unsigned protected:1;	/* must accessor be a subclass of "this" */
    union {
	int i;			/* operand to the opcode */
	int *ip;
	fullinfo_type fi;
    } operand, operand2;
    fullinfo_type p;
    struct stack_info_type stack_info;
    struct register_info_type register_info;
#define FLAG_REACHED            0x01 /* instruction reached */
#define FLAG_NEED_CONSTRUCTOR   0x02 /* must call this.<init> or super.<init> */
#define FLAG_NO_RETURN          0x04 /* must throw out of method */
    flag_type or_flags;		/* true for at least one path to this inst */
#define FLAG_CONSTRUCTED        0x01 /* this.<init> or super.<init> called */
    flag_type and_flags;	/* true for all paths to this instruction */
};

/*
 * As in a class file, a handler has a starting and ending range and a beginning,
 * but this has then as instruction indexes. The initial stack is always
 * a single item of the class type being caught by the handler.
 */
struct handler_info_type {
    int start, end, handler;
    struct stack_info_type stack_info;
};

/*
 * See struct stack_info_type above, which has a pointer to a linked list of these.
 */
struct stack_item_type {
    fullinfo_type item;
    struct stack_item_type *next;
};

typedef struct context_type context_type;
typedef struct instruction_data_type instruction_data_type;
typedef struct stack_item_type stack_item_type;
typedef struct register_info_type register_info_type;
typedef struct stack_info_type stack_info_type;
typedef struct mask_type mask_type;

static void verify_method(context_type *context, CVMClassBlock *cb,
			  CVMMethodBlock *mb);
#if 0 /* See comment in verify_field(). */
static void verify_field(context_type *context, CVMClassBlock *cb,
			 CVMFieldBlock *fb);
#endif
static void verify_opcode_operands (context_type *, int inumber, int offset);
static void set_protected(context_type *, int inumber, int key, opcode_type);
static jboolean is_superclass(context_type *, fullinfo_type);

static void initialize_exception_table(context_type *);
static jboolean isLegalTarget(context_type *, int offset);
static void verify_constant_pool_type(context_type *, int, unsigned);

static void initialize_dataflow(context_type *);
static void run_dataflow(context_type *context);
static void check_register_values(context_type *context, int inumber);
static void check_flags(context_type *context, int inumber);
static void pop_stack(context_type *, int inumber, stack_info_type *);
static void update_registers(context_type *, int inumber, register_info_type *);
static void update_flags(context_type *, int inumber, 
			 flag_type *new_and_flags, flag_type *new_or_flags);
static void push_stack(context_type *, int inumber, stack_info_type *stack);

static void merge_into_successors(context_type *, int inumber, 
				  register_info_type *register_info,
				  stack_info_type *stack_info, 
				  flag_type and_flags, flag_type or_flags);
static void merge_into_one_successor(context_type *context, 
				     int from_inumber, int inumber, 
				     register_info_type *register_info,
				     stack_info_type *stack_info, 
				     flag_type and_flags, flag_type or_flags,
				     jboolean isException);
static void merge_stack(context_type *, int inumber, int to_inumber, 
			stack_info_type *);
static void merge_registers(context_type *, int inumber, int to_inumber, 
			    register_info_type *);
static void merge_flags(context_type *context, int from_inumber, int to_inumber,
			flag_type new_and_flags, flag_type new_or_flags);

static stack_item_type *copy_stack(context_type *, stack_item_type *);
static mask_type *copy_masks(context_type *, mask_type *masks, int mask_count);
static mask_type *add_to_masks(context_type *, mask_type *, int , int);

static fullinfo_type decrement_indirection(fullinfo_type);

static fullinfo_type merge_fullinfo_types(context_type *context, 
					  fullinfo_type a, 
					  fullinfo_type b,
					  jboolean assignment);
static jboolean isAssignableTo(context_type *,
			       fullinfo_type a, 
			       fullinfo_type b);

static CVMClassBlock *object_fullinfo_to_classclass(context_type *, fullinfo_type);


#define NEW(type, count) \
        ((type *)CCalloc(context, (count)*(sizeof(type)), JNI_FALSE))
#define ZNEW(type, count) \
        ((type *)CCalloc(context, (count)*(sizeof(type)), JNI_TRUE))

static void CCinit(context_type *context);
static void CCreinit(context_type *context);
static void CCdestroy(context_type *context);
static void *CCalloc(context_type *context, int size, jboolean zero);

static fullinfo_type cp_index_to_class_fullinfo(context_type *, int, int);

static char signature_to_fieldtype(context_type *context, 
				   CVMFieldTypeID fieldID,
				   fullinfo_type *info);

#define CV_OK        1
#define CV_VERIFYERR -1
#define CV_FORMATERR -2

static void CCerror (context_type *, char *format, ...);
static void CCFerror (context_type *context, char *format, ...);
static void CCout_of_memory (context_type *);

#ifdef CVM_TRACE
static void print_stack (context_type *, stack_info_type *stack_info);
static void print_registers(context_type *, register_info_type *register_info);
static void print_flags(context_type *, flag_type, flag_type);
static void print_formatted_fieldname(context_type *context, int index);
static void print_formatted_methodname(context_type *context, int index);
#endif

static void initialize_class_hash(context_type *context) 
{
    hash_table_type *class_hash = &(context->class_hash);
    class_hash->buckets = (hash_bucket_type **)
        calloc(MAX_HASH_ENTRIES / HASH_ROW_SIZE, sizeof(hash_bucket_type *));
    class_hash->table = (unsigned short *)
        calloc(HASH_TABLE_SIZE, sizeof(unsigned short));
    if (class_hash->buckets == 0 ||
	class_hash->table == 0)
        CCout_of_memory(context);
    class_hash->entries_used = 0;
}

static void finalize_class_hash(context_type *context)
{
    hash_table_type *class_hash = &(context->class_hash);
    int i;
    if (class_hash->buckets) {
        for (i=0;i<MAX_HASH_ENTRIES / HASH_ROW_SIZE; i++) {
	    if (class_hash->buckets[i] == 0)
	        break;
	    free(class_hash->buckets[i]);
	}
    }
    free(class_hash->buckets);
    free(class_hash->table);
}

static hash_bucket_type * 
new_bucket(context_type *context, unsigned short *pID)
{
    hash_table_type *class_hash = &(context->class_hash);
    int i = *pID = class_hash->entries_used + 1;
    int row = i / HASH_ROW_SIZE;
    if (i >= MAX_HASH_ENTRIES)
        CCerror(context, "Exceeded verifier's limit of 65535 referred classes");
    if (class_hash->buckets[row] == 0) {
        class_hash->buckets[row] = (hash_bucket_type*)
	    calloc(HASH_ROW_SIZE, sizeof(hash_bucket_type));
	if (class_hash->buckets[row] == 0)
	    CCout_of_memory(context);
    }
    class_hash->entries_used++; /* only increment when we are sure there
				   is no overflow. */
    return GET_BUCKET(class_hash, i);
}

/*
 * Find a class using the defining loader of the current class
 * and return a local reference to it.
 */
static CVMClassBlock*
load_class_local(context_type *context, CVMClassTypeID classname)
{
    CVMClassBlock *cb = 
	CVMclassLookupByTypeFromClass(context->ee, classname, CVM_FALSE, 
				      context->class);
    if (cb == 0)
	 CCerror(context, "Cannot find class %!C", classname);
    return cb;
} 

/*
 * Find a class using the defining loader of the current class
 * and return a global reference to it.
 */
static CVMClassBlock *
load_class_global(context_type *context, CVMClassTypeID classname)
{
    return load_class_local(context, classname);
} 


/*
 * Return a unique ID given a local class reference. The loadable
 * flag is true if the defining class loader of context->class
 * is known to be capable of loading the class.
 */
static int 
class_to_ID(context_type *context, CVMClassBlock *cb, jboolean loadable)
{
    hash_table_type *class_hash = &(context->class_hash);
    unsigned int hash;
    hash_bucket_type *bucket;
    unsigned short *pID;
    CVMClassTypeID name = CVMcbClassName(cb);

    hash = name;
    pID = &(class_hash->table[hash % HASH_TABLE_SIZE]);
    while (*pID) {
        bucket = GET_BUCKET(class_hash, *pID);
        if (name == bucket->name) {
	    /*
	     * There is an unresolved entry with our name
	     * so we're forced to load it in case it matches us.
	     */
	    if (bucket->class == 0) {
		CVMassert(bucket->loadable == JNI_TRUE);
		bucket->class = load_class_global(context, name);
	    }

	    /*
	     * It's already in the table. Update the loadable
	     * state if it's known and then we're done.
	     */
	    if (cb == bucket->class) {
		if (loadable && !bucket->loadable)
		    bucket->loadable = JNI_TRUE;
	        goto done;
	    }
	}
	pID = &bucket->next;
    }
    bucket = new_bucket(context, pID);
    bucket->next = 0;
    bucket->name = name;
    bucket->loadable = loadable;
    bucket->class = cb;

done:
    return *pID;
}

/*
 * Return a unique ID given a class name from the constant pool.
 * All classes are lazily loaded from the defining loader of
 * context->class.
 */
static int 
class_name_to_ID(context_type *context, CVMClassTypeID name)
{
    hash_table_type *class_hash = &(context->class_hash);
    unsigned int hash = name;
    hash_bucket_type *bucket;
    unsigned short *pID;
    jboolean force_load = JNI_FALSE;

    CVMassert(name == CVMtypeidGetType(name));
    pID = &(class_hash->table[hash % HASH_TABLE_SIZE]);
    while (*pID) {
        bucket = GET_BUCKET(class_hash, *pID);
        if (name == bucket->name) {
	    if (bucket->loadable)
	        goto done;
	    force_load = JNI_TRUE;
	}
	pID = &bucket->next;
    }

    if (force_load) {
	/*
	 * We found at least one matching named entry for a class that
	 * was not known to be loadable through the defining class loader
	 * of context->class. We must load our named class and update
	 * the hash table in case one these entries matches our class.
	 */
	CVMClassBlock *cb = load_class_local(context, name);
	int id = class_to_ID(context, cb, JNI_TRUE);
	return id;
    }

    bucket = new_bucket(context, pID);
    bucket->next = 0;
    bucket->class = 0;
    bucket->loadable = JNI_TRUE; /* name-only IDs are implicitly loadable */
    bucket->name = name;

done:
    return *pID;
}

#ifdef CVM_TRACE
static CVMClassTypeID
ID_to_class_name(context_type *context, int ID)
{
    hash_table_type *class_hash = &(context->class_hash);
    hash_bucket_type *bucket = GET_BUCKET(class_hash, ID);
    return bucket->name;
}
#endif

static CVMClassBlock *
ID_to_class(context_type *context, int ID)
{
    hash_table_type *class_hash = &(context->class_hash);
    hash_bucket_type *bucket = GET_BUCKET(class_hash, ID);
    if (bucket->class == 0) {
	CVMassert(bucket->loadable == JNI_TRUE);
	bucket->class = load_class_global(context, bucket->name);
    }
    return bucket->class;
}

static fullinfo_type 
make_loadable_class_info(context_type *context, CVMClassBlock *cb)
{
    return MAKE_FULLINFO(ITEM_Object, 0,
			   class_to_ID(context, cb, JNI_TRUE));
}

static fullinfo_type 
make_class_info(context_type *context, CVMClassBlock *cb)
{
    return MAKE_FULLINFO(ITEM_Object, 0,
                         class_to_ID(context, cb, JNI_FALSE));
}

static fullinfo_type
make_class_info_from_name(context_type *context, CVMClassTypeID name)
{
    return MAKE_FULLINFO(ITEM_Object, 0, class_name_to_ID(context, name));
}

/*
 * Called by CVMclassVerify.  Verify the code of each of the methods
 * in a class.
 */
jint
VerifyClass(CVMExecEnv *ee, CVMClassBlock *cb, char *buffer, jint len,
            CVMBool isRedefine)
{
    context_type *context = (context_type *)malloc(sizeof(context_type)); 
    jint result = CV_OK;
    int i;

    if (context == NULL) {
    	jio_snprintf(buffer, len, "(class: %C) Out Of Memory", cb);
	return JNI_FALSE;
    }
    memset(context, 0, sizeof(context_type));

    context->major_version = cb->major_version;

    context->message = buffer;
    context->message_buf_len = len;

    context->ee = ee;
    context->class = cb;

    /* Set invalid method/field index of the context, in case anyone 
       calls CCerror */
    context->mb = 0;
#if 0 /* See comment in verify_field(). */
    context->fb = 0;
#endif

    /* Don't call CCerror or anything that can call it above the setjmp! */
    if (!setjmp(context->jump_buffer)) {
	CVMClassBlock *super;

	CCinit(context);		/* initialize heap; may throw */

	initialize_class_hash(context);

	context->nconstants = CVMcbConstantPoolCount(cb);
	context->constant_pool = CVMcbConstantPool(cb);

	context->object_info = 
	    make_class_info(context, CVMsystemClass(java_lang_Object));
	context->string_info = 
	    make_class_info(context, CVMsystemClass(java_lang_String));
	context->class_info = 
	    make_class_info(context, CVMsystemClass(java_lang_Class));
	context->throwable_info = 
	    make_class_info(context, CVMsystemClass(java_lang_Throwable));
	context->cloneable_info = 
	    make_class_info(context, CVMsystemClass(java_lang_Cloneable));
	context->serializable_info = 
	    make_class_info(context, CVMsystemClass(java_io_Serializable));

#ifdef CVM_JVMTI
        if (isRedefine) {
            /* The class being verified is the new class. However, that
             * class will not be in the list of classes as we just use
             * it to hold the methods during the redefine phase.  The oldcb
             * is the class of record for this class so we want to make
             * sure we use that as the target of any verifications
             */
            CVMClassBlock *oldcb = CVMjvmtiGetCurrentRedefinedClass(ee);
            CVMassert(oldcb != NULL);
            context->currentclass_info =
                make_loadable_class_info(context, oldcb);
        } else
#endif
       {
           context->currentclass_info = make_loadable_class_info(context, cb);
       }
	super = CVMcbSuperclass(cb);

	if (super != 0) {
	    fullinfo_type *gptr;
	    int i = 0;

	    context->superclass_info = make_loadable_class_info(context, super);

	    while(super != 0) {
	        CVMClassBlock *tmp_cb = CVMcbSuperclass(super);
		super = tmp_cb;
		i++;
	    }
	    super = 0;

	    /* Can't go on context heap since it survives more than 
	       one method */
	    context->superclasses = gptr = (fullinfo_type *)
	        malloc(sizeof(fullinfo_type)*(i + 1));
	    if (gptr == 0) {
	        CCout_of_memory(context);
	    }

	    super = CVMcbSuperclass(context->class);
	    while(super != 0) {
	        CVMClassBlock *tmp_cb;
		*gptr++ = make_class_info(context, super);
		tmp_cb = CVMcbSuperclass(super); 
		super = tmp_cb;
	    }
	    *gptr = 0;
	} else { 
	    context->superclass_info = 0;
	}
    
	/* Look at each member */
#if 0 /* See comment in verify_field(). */
	for (i = CVMcbFieldCount(cb); --i >= 0;)  
	    verify_field(context, cb, CVMcbFieldSlot(cb, i));
#endif
	for (i = CVMcbMethodCount(cb); --i >= 0;) 
	    verify_method(context, cb, CVMcbMethodSlot(cb, i));
	result = CV_OK;
    } else {
        CVMassert(context->err_code < 0);
	result = context->err_code;
    }

    /* Cleanup: free fields in context */

    finalize_class_hash(context);

    if (context->superclasses)
	free(context->superclasses);

    CCdestroy(context);		/* destroy heap */
    free(context);		/* free context */
    return result;
}

/* NOTE: the format verifier in (verifyformat.c) that runs before this will
   ensure that the field cannot be public, private, or protected at the
   same time.  Hence, this check is superflous.  It is left here just for
   reference.  This verify_field() function is left here to preserve the
   structure and flow of the code verifier in case other field verification
   work needs to be done here in the future.
*/
#if 0
static void
verify_field(context_type *context, CVMClassBlock *cb, CVMFieldBlock *fb)
{
    int access_bits = CVMfbAccessFlags(fb);
    context->fb = fb;

    if (CVMmemberPPPAccessIs(access_bits, FIELD, PUBLIC) && 
	(CVMmemberPPPAccessIs(access_bits, FIELD, PRIVATE) ||
	 CVMmemberPPPAccessIs(access_bits, FIELD, PROTECTED))) {
        CCerror(context, "Inconsistent access bits.");
    } 

    context->fb = 0;
}
#endif


/* Verify the code of one method */

static void
verify_method(context_type *context, CVMClassBlock *cb, CVMMethodBlock* mb)
{
    int access_bits = CVMmbAccessFlags(mb);
    const unsigned char *code;
    int code_length;
    int *code_data;
    instruction_data_type *idata = 0;
    int instruction_count;
    int i, offset, inumber;
    CVMCheckedExceptions* ce;

    if ((access_bits & (CVM_METHOD_ACC_NATIVE | CVM_METHOD_ACC_ABSTRACT))
	!= 0) { 
	/* not much to do for abstract and native methods */
	return;
    }

    code_length = context->code_length =
        JVM_GetMethodByteCodeLength(mb);
    code = context->code = JVM_GetMethodByteCode(mb);
 
    /* CCerror can give method-specific info once this is set */
    context->mb = mb;

    CCreinit(context);		/* initial heap */
    code_data = NEW(int, code_length);

#ifdef CVM_TRACE
    if (verify_verbose) {
        CVMClassTypeID classname = CVMcbClassName(cb);
	CVMMethodTypeID methodname = CVMmbNameAndTypeID(mb);
	CVMconsolePrintf("Looking at %!C.%!M\n", classname, methodname);
    }
#endif

#if 0
    /* NOTE: the format verifier in (verifyformat.c) that runs before this will
       ensure that the field cannot be public, private, or protected at the
       same time.  Hence, this check is superflous.  It is left here just for
       reference.
    */
    if (CVMmemberPPPAccessIs(access_bits, METHOD, PUBLIC) && 
	(CVMmemberPPPAccessIs(access_bits, METHOD, PRIVATE) ||
	 CVMmemberPPPAccessIs(access_bits, METHOD, PROTECTED))) {
	CCerror(context, "Inconsistent access bits.");
    } 
#endif
    context->method_has_jsr = CVM_FALSE;
    context->method_has_switch = CVM_FALSE;

    /* Run through the code.  Mark the start of each instruction, and give
     * the instruction a number */
    for (i = 0, offset = 0; offset < code_length; i++) {
	int length = CVMopcodeGetLengthWithBoundsCheck(
	    &code[offset], &code[code_length]);
	int next_offset = offset + length;
	if (length <= 0) 
	    CCerror(context, "Illegal instruction found at offset %d", offset);
	if (next_offset > code_length) 
	    CCerror(context, "Code stops in the middle of instruction "
		    " starting at offset %d", offset);
	code_data[offset] = i;
	while (++offset < next_offset)
	    code_data[offset] = -1; /* illegal location */
    }
    instruction_count = i;	/* number of instructions in code */
    
    /* Allocate a structure to hold info about each instruction. */
    idata = NEW(instruction_data_type, instruction_count);

    /* Initialize the heap, and other info in the context structure. */
    context->instruction_data = idata;
    context->code_data = code_data;
    context->instruction_count = instruction_count;
    context->handler_info = 
        NEW(struct handler_info_type, 
	    JVM_GetMethodExceptionTableLength(mb));
    context->bitmask_size = 
        (JVM_GetMethodLocalsCount(mb)
	 + (BITS_PER_INT - 1))/BITS_PER_INT;
    
    if (instruction_count == 0) 
	CCerror(context, "Empty code");
	
    for (inumber = 0, offset = 0; offset < code_length; inumber++) {
	int length = CVMopcodeGetLength(&code[offset]);
	instruction_data_type *this_idata = &idata[inumber];
	this_idata->opcode = (CVMOpcode)code[offset];
	this_idata->stack_info.stack = NULL;
	this_idata->stack_info.stack_size  = UNKNOWN_STACK_SIZE;
	this_idata->register_info.register_count = UNKNOWN_REGISTER_COUNT;
	this_idata->changed = JNI_FALSE;  /* no need to look at it yet. */
	this_idata->protected = JNI_FALSE;  /* no need to look at it yet. */
	this_idata->and_flags = (flag_type) -1;	/* "bottom" and value */
	this_idata->or_flags = 0; /* "bottom" or value*/
	/* This also sets up this_data->operand.  It also makes the 
	 * xload_x and xstore_x instructions look like the generic form. */
	verify_opcode_operands(context, inumber, offset);
	offset += length;
    }
    
    
    /* make sure exception table is reasonable. */
    initialize_exception_table(context);
    /* Set up first instruction, and start of exception handlers. */
    initialize_dataflow(context);
    /* Run data flow analysis on the instructions. */
    run_dataflow(context);

    /* verify checked exceptions, if any */
    ce = CVMmbCheckedExceptions(mb);
    if (ce != NULL) {
	for (i = 0; i < ce->numExceptions; i++) {
	    /* Make sure the constant pool item is CVM_CONSTANT_ClassTypeID */
	    verify_constant_pool_type(context, (int)ce->exceptions[i],
				      1 << CVM_CONSTANT_ClassTypeID |
				      1 << CVM_CONSTANT_ClassBlock);
	}
    }

    /*
     * Set flags in the jmd based on what we found.
     * This helps avoid yet another re-parsing of the bytecodes
     */
    {
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
	if (context->method_has_switch){
#ifdef CVM_TRACE
	    if (verify_verbose) {
		CVMconsolePrintf("Switch found: requires alignment\n");
	    }
#endif
	    CVMjmdFlags(jmd) |= CVM_JMD_NEEDS_ALIGNMENT;
	}
	if (context->method_has_jsr){
#ifdef CVM_TRACE
	    if (verify_verbose) {
		CVMconsolePrintf("Jsr found: may require disambiguation\n");
	    }
#endif
	    CVMjmdFlags(jmd) |= CVM_JMD_MAY_NEED_REWRITE;
	}
    }
    context->code = 0;
    context->mb = 0;
}


/* Look at a single instruction, and verify its operands.  Also, for 
 * simplicity, move the operand into the ->operand field.
 * Make sure that branches don't go into the middle of nowhere.
 */

static jint CVMntohl(jint n)
{
    unsigned char *p = (unsigned char *)&n;
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void
verify_opcode_operands(context_type *context, int inumber, int offset)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    int *code_data = context->code_data;
    CVMMethodBlock* mb = context->mb;
    const unsigned char *code = context->code;
    opcode_type opcode = this_idata->opcode;
    int var; 
    
    /*
     * Set the ip fields to 0 not the i fields because the ip fields
     * are 64 bits on 64 bit architectures, the i field is only 32
     */
    this_idata->operand.ip = 0;
    this_idata->operand2.ip = 0;

    switch (opcode) {

    case opc_jsr:
	/* instruction of ret statement */
	this_idata->operand2.i = UNKNOWN_RET_INSTRUCTION;
	context->method_has_jsr = CVM_TRUE;
	/* FALLTHROUGH */
    case opc_ifeq: case opc_ifne: case opc_iflt: 
    case opc_ifge: case opc_ifgt: case opc_ifle:
    case opc_ifnull: case opc_ifnonnull:
    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmplt: 
    case opc_if_icmpge: case opc_if_icmpgt: case opc_if_icmple:
    case opc_if_acmpeq: case opc_if_acmpne:   
    case opc_goto: {
	/* Set the ->operand to be the instruction number of the target. */
	int jump = (((signed char)(code[offset+1])) << 8) + code[offset+2];
	int target = offset + jump;
	if (!isLegalTarget(context, target))
	    CCerror(context, "Illegal target of jump or branch");
	this_idata->operand.i = code_data[target];
	break;
    }
	
    case opc_jsr_w:
	/* instruction of ret statement */
	this_idata->operand2.i = UNKNOWN_RET_INSTRUCTION;
	context->method_has_jsr = CVM_TRUE;
	/* FALLTHROUGH */
    case opc_goto_w: {
	/* Set the ->operand to be the instruction number of the target. */
	int jump = (((signed char)(code[offset+1])) << 24) + 
	             (code[offset+2] << 16) + (code[offset+3] << 8) + 
	             (code[offset + 4]);
	int target = offset + jump;
	if (!isLegalTarget(context, target))
	    CCerror(context, "Illegal target of jump or branch");
	this_idata->operand.i = code_data[target];
	break;
    }

    case opc_tableswitch: 
    case opc_lookupswitch: {
	/* Set the ->operand to be a table of possible instruction targets. */
	int *lpc = (int *) UCALIGN(code + offset + 1);
	int *lptr;
	int *saved_operand;
	int keys;
	int k, delta;
        unsigned char* bptr = (unsigned char*) (code + offset + 1);
	context->method_has_switch = CVM_TRUE;
        for (; bptr < (unsigned char*)lpc; bptr++) {
	    if (*bptr != 0) {
                CCerror(context, "Non zero padding bytes in switch");
            }
        }
	if (opcode == opc_tableswitch) {
	    keys = CVMntohl(lpc[2]) -  CVMntohl(lpc[1]) + 1;
	    delta = 1;
	} else { 
	    keys = CVMntohl(lpc[1]); /* number of pairs */
	    delta = 2;
	    /* Make sure that the tableswitch items are sorted */
	    for (k = keys - 1, lptr = &lpc[2]; --k >= 0; lptr += 2) {
		int this_key = CVMntohl(lptr[0]);
		int next_key = CVMntohl(lptr[2]);
		if (this_key >= next_key) { 
		    CCerror(context, "Unsorted lookup switch");
		}
	    }
	}
	saved_operand = NEW(int, keys + 2);
	if (!isLegalTarget(context, offset + CVMntohl(lpc[0]))) 
	    CCerror(context, "Illegal default target in switch");
	saved_operand[keys + 1] = code_data[offset + CVMntohl(lpc[0])];
	for (k = keys, lptr = &lpc[3]; --k >= 0; lptr += delta) {
	    int target = offset + CVMntohl(lptr[0]);
	    if (!isLegalTarget(context, target))
		CCerror(context, "Illegal branch in opc_tableswitch");
	    saved_operand[k + 1] = code_data[target];
	}
	saved_operand[0] = keys + 1; /* number of successors */
	this_idata->operand.ip = saved_operand;
	break;
    }
	
    case opc_ldc: {   
	/* Make sure the constant pool item is the right type. */
	int key = code[offset + 1];
	int types = (1 << CVM_CONSTANT_Integer) | (1 << CVM_CONSTANT_Float) |
	                    (1 << CVM_CONSTANT_StringICell);
	if (context->major_version >= 49) {
	    types |= (1 << CVM_CONSTANT_ClassTypeID);
	}
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, types);
	break;
    }
	  
    case opc_ldc_w: {   
	/* Make sure the constant pool item is the right type. */
	int key = (code[offset + 1] << 8) + code[offset + 2];
	int types = (1 << CVM_CONSTANT_Integer) | (1 << CVM_CONSTANT_Float) |
	    (1 << CVM_CONSTANT_StringICell);
	if (context->major_version >= 49) {
	    types |= (1 << CVM_CONSTANT_ClassTypeID);
	}
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, types);
	break;
    }
	  
    case opc_ldc2_w: { 
	/* Make sure the constant pool item is the right type. */
	int key = (code[offset + 1] << 8) + code[offset + 2];
	int types = (1 << CVM_CONSTANT_Double) | (1 << CVM_CONSTANT_Long);
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, types);
	break;
    }

    case opc_getfield: case opc_putfield:
    case opc_getstatic: case opc_putstatic: {
	/* Make sure the constant pool item is the right type. */
	int key = (code[offset + 1] << 8) + code[offset + 2];
	this_idata->operand.i = key;
	verify_constant_pool_type(context, key, 1 << CVM_CONSTANT_Fieldref);	
	if (opcode == opc_getfield || opcode == opc_putfield) 
	    set_protected(context, inumber, key, opcode);
	break;
    }

    case opc_invokevirtual:
    case opc_invokespecial: 
    case opc_invokestatic:
    case opc_invokeinterface: {
	/* Make sure the constant pool item is the right type. */
	int key = (code[offset + 1] << 8) + code[offset + 2];
	CVMMethodTypeID methodname;
	fullinfo_type clazz_info;
	int is_constructor, is_internal;
	int kind = (opcode == opc_invokeinterface 
	                    ? 1 << CVM_CONSTANT_InterfaceMethodref
	                    : 1 << CVM_CONSTANT_Methodref);
	/* Make sure the constant pool item is the right type. */
	verify_constant_pool_type(context, key, kind);
	methodname = JVM_GetCPMethodName(context->constant_pool, key);
	is_constructor = CVMtypeidIsConstructor(methodname);
	is_internal = is_constructor || CVMtypeidIsClinit(methodname);

	clazz_info = cp_index_to_class_fullinfo(context, key,
						CVM_CONSTANT_Methodref);
	this_idata->operand.i = key;
	this_idata->operand2.fi = clazz_info;
	if (is_constructor) {
	    if (opcode != opc_invokespecial) {
		CCerror(context, 
			"Must call initializers using invokespecial");
	    }
	    this_idata->opcode = (CVMOpcode)opc_invokeinit;
	} else {
	    if (is_internal) {
		CCerror(context, "Illegal call to internal method");
	    }
	    if (opcode == opc_invokespecial 
		   && clazz_info != context->currentclass_info  
		   && clazz_info != context->superclass_info) {
		int not_found = 1;

		CVMClassBlock *super = CVMcbSuperclass(context->class);
		while(super != 0) {
		    CVMClassBlock *tmp_cb;
		    fullinfo_type new_info = make_class_info(context, super);
		    if (clazz_info == new_info) {
		        not_found = 0;
			break;
		    }
		    tmp_cb = CVMcbSuperclass(super);
		    super = tmp_cb;
		}
		
		/* The optimizer make cause this to happen on local code */
		if (not_found) {
#ifdef BROKEN_JAVAC
		    jobject loader = JVM_GetClassLoader(env, context->class);
		    int has_loader = (loader != 0);
		    (*env)->DeleteLocalRef(env, loader);
		    if (has_loader)
#endif /* BROKEN_JAVAC */
		        CCerror(context, 
				"Illegal use of nonvirtual function call");
		}
	    }
	}
	if (opcode == opc_invokeinterface) { 
	    unsigned int args1;
	    unsigned int args2;
	    CVMMethodTypeID signature = 
	        JVM_GetCPMethodSignature(context->constant_pool, key);
	    args1 = CVMtypeidGetArgsSize(signature) + 1;
	    args2 = code[offset + 3];
	    if (args1 != args2) {
		CCerror(context, 
			"Inconsistent args_size for opc_invokeinterface");
	    } 
            if (code[offset + 4] != 0) {
                CCerror(context,
                        "Fourth operand byte of invokeinterface must be zero");
            }
	} else if (opcode == opc_invokevirtual 
		      || opcode == opc_invokespecial) 
	    set_protected(context, inumber, key, opcode);
	break;
    }
	

    case opc_instanceof: 
    case opc_checkcast: 
    case opc_new:
    case opc_anewarray: 
    case opc_multianewarray: {
	/* Make sure the constant pool item is a class */
	int key = (code[offset + 1] << 8) + code[offset + 2];
	fullinfo_type target;
	verify_constant_pool_type(context, key, 
				  1 << CVM_CONSTANT_ClassTypeID |
				  1 << CVM_CONSTANT_ClassBlock);
	target = cp_index_to_class_fullinfo(context, key,
					    CVM_CONSTANT_ClassTypeID);
	if (GET_ITEM_TYPE(target) == ITEM_Bogus) 
	    CCerror(context, "Illegal type");
	switch(opcode) {
	case opc_anewarray:
	    if ((GET_INDIRECTION(target)) >= MAX_ARRAY_DIMENSIONS)
		CCerror(context, "Array with too many dimensions");
	    this_idata->operand.fi = MAKE_FULLINFO(GET_ITEM_TYPE(target),
						   GET_INDIRECTION(target) + 1,
						   GET_EXTRA_INFO(target));
	    break;
	case opc_new:
	    if (WITH_ZERO_EXTRA_INFO(target) !=
		             MAKE_FULLINFO(ITEM_Object, 0, 0))
		CCerror(context, "Illegal creation of multi-dimensional array");
	    /* operand gets set to the "unitialized object".  operand2 gets
	     * set to what the value will be after it's initialized. */
	    this_idata->operand.fi = MAKE_FULLINFO(ITEM_NewObject, 0, inumber);
	    this_idata->operand2.fi = target;
	    break;
	case opc_multianewarray:
	    this_idata->operand.fi = target;
	    this_idata->operand2.i = code[offset + 3];
	    if (    (this_idata->operand2.i > GET_INDIRECTION(target))
		 || (this_idata->operand2.i == 0))
		CCerror(context, "Illegal dimension argument");
	    break;
	default:
	    this_idata->operand.fi = target;
	}
	break;
    }
	
    case opc_newarray: {
	/* Cache the result of the opc_newarray into the operand slot */
	fullinfo_type full_info;
	switch (code[offset + 1]) {
	    case JVM_T_INT:    
	        full_info = MAKE_FULLINFO(ITEM_Integer, 1, 0); break;
	    case JVM_T_LONG:   
		full_info = MAKE_FULLINFO(ITEM_Long, 1, 0); break;
	    case JVM_T_FLOAT:  
		full_info = MAKE_FULLINFO(ITEM_Float, 1, 0); break;
	    case JVM_T_DOUBLE: 
		full_info = MAKE_FULLINFO(ITEM_Double, 1, 0); break;
	    case JVM_T_BYTE: case JVM_T_BOOLEAN:
		full_info = MAKE_FULLINFO(ITEM_Byte, 1, 0); break;
	    case JVM_T_CHAR:   
		full_info = MAKE_FULLINFO(ITEM_Char, 1, 0); break;
	    case JVM_T_SHORT:  
		full_info = MAKE_FULLINFO(ITEM_Short, 1, 0); break;
	    default:
		full_info = 0;		/* Keep lint happy */
		CCerror(context, "Bad type passed to opc_newarray");
	}
	this_idata->operand.fi = full_info;
	break;
    }
	  
    /* Fudge iload_x, aload_x, etc to look like their generic cousin. */
    case opc_iload_0: case opc_iload_1: case opc_iload_2: case opc_iload_3:
	this_idata->opcode = opc_iload;
	var = opcode - opc_iload_0;
	goto check_local_variable;
	  
    case opc_fload_0: case opc_fload_1: case opc_fload_2: case opc_fload_3:
	this_idata->opcode = opc_fload;
	var = opcode - opc_fload_0;
	goto check_local_variable;

    case opc_aload_0: case opc_aload_1: case opc_aload_2: case opc_aload_3:
	this_idata->opcode = opc_aload;
	var = opcode - opc_aload_0;
	goto check_local_variable;

    case opc_lload_0: case opc_lload_1: case opc_lload_2: case opc_lload_3:
	this_idata->opcode = opc_lload;
	var = opcode - opc_lload_0;
	goto check_local_variable2;

    case opc_dload_0: case opc_dload_1: case opc_dload_2: case opc_dload_3:
	this_idata->opcode = opc_dload;
	var = opcode - opc_dload_0;
	goto check_local_variable2;

    case opc_istore_0: case opc_istore_1: case opc_istore_2: case opc_istore_3:
	this_idata->opcode = opc_istore;
	var = opcode - opc_istore_0;
	goto check_local_variable;
	
    case opc_fstore_0: case opc_fstore_1: case opc_fstore_2: case opc_fstore_3:
	this_idata->opcode = opc_fstore;
	var = opcode - opc_fstore_0;
	goto check_local_variable;

    case opc_astore_0: case opc_astore_1: case opc_astore_2: case opc_astore_3:
	this_idata->opcode = opc_astore;
	var = opcode - opc_astore_0;
	goto check_local_variable;

    case opc_lstore_0: case opc_lstore_1: case opc_lstore_2: case opc_lstore_3:
	this_idata->opcode = opc_lstore;
	var = opcode - opc_lstore_0;
	goto check_local_variable2;

    case opc_dstore_0: case opc_dstore_1: case opc_dstore_2: case opc_dstore_3:
	this_idata->opcode = opc_dstore;
	var = opcode - opc_dstore_0;
	goto check_local_variable2;

    case opc_wide: 
	this_idata->opcode = (CVMOpcode)code[offset + 1];
	var = (code[offset + 2] << 8) + code[offset + 3];
	switch(this_idata->opcode) {
	    case opc_lload:  case opc_dload: 
	    case opc_lstore: case opc_dstore:
	        goto check_local_variable2;
	    default:
	        goto check_local_variable;
	}

    case opc_iinc:		/* the increment amount doesn't matter */
    case opc_ret: 
    case opc_aload: case opc_iload: case opc_fload:
    case opc_astore: case opc_istore: case opc_fstore:
	var = code[offset + 1];
    check_local_variable:
	/* Make sure that the variable number isn't illegal. */
	this_idata->operand.i = var;
	if (var >= JVM_GetMethodLocalsCount(mb))
	    CCerror(context, "Illegal local variable number");
	break;
	    
    case opc_lload: case opc_dload: case opc_lstore: case opc_dstore: 
	var = code[offset + 1];
    check_local_variable2:
	/* Make sure that the variable number isn't illegal. */
	this_idata->operand.i = var;
	if ((var + 1) >= JVM_GetMethodLocalsCount(mb))
	    CCerror(context, "Illegal local variable number");
	break;
	
    default:
	if (opcode >= opc_breakpoint) 
	    CCerror(context, "Quick instructions shouldn't appear yet.");
	break;
    } /* of switch */
}


static void 
set_protected(context_type *context, int inumber, int key, opcode_type opcode) 
{
    CVMExecEnv *ee = context->ee;
    fullinfo_type clazz_info;
    if (opcode != opc_invokevirtual && opcode != opc_invokespecial) {
        clazz_info = cp_index_to_class_fullinfo(context, key, 
						CVM_CONSTANT_Fieldref);
    } else {
        clazz_info = cp_index_to_class_fullinfo(context, key, 
						CVM_CONSTANT_Methodref);
    }
    if (is_superclass(context, clazz_info)) {
	CVMClassBlock *calledClass = 
	    object_fullinfo_to_classclass(context, clazz_info);
	int access;

        do {
   	    if (opcode != opc_invokevirtual && opcode != opc_invokespecial) {
	        access = JVM_GetCPFieldModifiers
	            (context->constant_pool, key, calledClass);
	    } else {
	        access = JVM_GetCPMethodModifiers
	            (context->constant_pool, key, calledClass);
	    }
	    if (access != -1) {
               break;
            }
            calledClass = CVMcbSuperclass(calledClass);
	} while (calledClass != 0);

        if (access == -1) {
             /* field/method not found, detected at runtime. */
        } else if (CVMmemberPPPAccessIs(access, FIELD, PROTECTED)) {

	    /* NOTE: the format verifier in (verifyformat.c) that runs before
	       this will ensure that the field cannot be public, private, or
	       protected at the same time.  Hence, this check for the
	       PRIVATE flag is superflous.  It is left here in the comment
	       just for reference.

	    if (CVMmemberPPPAccessIs(access, FIELD, PRIVATE) ||
		!CVMisSameClassPackage(ee, calledClass, context->class))
	    */
	    if (!CVMisSameClassPackage(ee, calledClass, context->class)) {
		context->instruction_data[inumber].protected = JNI_TRUE;
	    }
	}
    }
}


static jboolean 
is_superclass(context_type *context, fullinfo_type clazz_info) { 
    fullinfo_type *fptr = context->superclasses;

    if (fptr == 0)
        return JNI_FALSE;
    for (; *fptr != 0; fptr++) { 
	if (*fptr == clazz_info)
	    return JNI_TRUE;
    }
    return JNI_FALSE;
}




/* Look through each item on the exception table.  Each of the fields must 
 * refer to a legal instruction. 
 */
static void
initialize_exception_table(context_type *context)
{
    CVMExecEnv *ee = context->ee;
    CVMMethodBlock *mb = context->mb;
    struct handler_info_type *handler_info = context->handler_info;
    int *code_data = context->code_data;
    int code_length = context->code_length;
    int i;
    for (i = JVM_GetMethodExceptionTableLength(mb); 
	 --i >= 0; 
	 handler_info++) {
        CVMExceptionHandler *einfo;
	stack_item_type *stack_item = NEW(stack_item_type, 1);

	einfo = JVM_GetMethodExceptionTableEntry(mb, i);

	if (!(einfo->startpc < einfo->endpc &&
	      isLegalTarget(context, einfo->startpc) &&
	      (einfo->endpc ==  code_length || 
	       isLegalTarget(context, einfo->endpc)))) {
#if (JAVASE == 14)
	    CCerror(context, "Illegal exception table range");
#else
	    CCFerror(context, "Illegal exception table range");
#endif
	}
	if (!((einfo->handlerpc > 0) && 
	      isLegalTarget(context, einfo->handlerpc))) {
#if (JAVASE == 14)
	    CCerror(context, "Illegal exception table handler");
#else
            CCFerror(context, "Illegal exception table handler");
#endif
	}

	handler_info->start = code_data[einfo->startpc];
	/* einfo->endpc may point to one byte beyond the end of bytecodes. */
	handler_info->end = (einfo->endpc == context->code_length) ? 
	    context->instruction_count : code_data[einfo->endpc];
	handler_info->handler = code_data[einfo->handlerpc];
	handler_info->stack_info.stack = stack_item;
	handler_info->stack_info.stack_size = 1;
	stack_item->next = NULL;
	if (einfo->catchtype != 0) {
	    CVMClassTypeID classname;
	    verify_constant_pool_type(context, einfo->catchtype, 
				      1 << CVM_CONSTANT_ClassTypeID |
				      1 << CVM_CONSTANT_ClassBlock);
	    classname = JVM_GetCPClassName(ee, context->constant_pool,
					   einfo->catchtype);
	    stack_item->item = make_class_info_from_name(context, classname);
	    if (!isAssignableTo(context, 
				stack_item->item, 
				context->throwable_info))
	        CCerror(context, "catch_type not a subclass of Throwable");
	} else {
	    stack_item->item = context->throwable_info;
	}
    }
}


/* Given the target of a branch, make sure that it's a legal target. */
static jboolean 
isLegalTarget(context_type *context, int offset)
{
    int code_length = context->code_length;
    int *code_data = context->code_data;
    return (offset >= 0 && offset < code_length && code_data[offset] >= 0);
}


/* Make sure that an element of the constant pool really is of the indicated 
 * type.
 */
static void
verify_constant_pool_type(context_type *context, int index, unsigned mask)
{
    int nconstants = context->nconstants;
    CVMConstantPool *constant_pool = context->constant_pool;
    unsigned type;

    if ((index <= 0) || (index >= nconstants))
	CCerror(context, "Illegal constant pool index");

    type = CVMcpEntryType(constant_pool, index);
    if ((mask & (1 << type)) == 0) 
	CCerror(context, "Illegal type in constant pool");
}
        

static void
initialize_dataflow(context_type *context) 
{
    instruction_data_type *idata = context->instruction_data;
    CVMMethodBlock *mb = context->mb;
    int args_size = CVMmbArgsSize(mb);
    fullinfo_type *reg_ptr;
    fullinfo_type full_info;

    /* Initialize the function entry, since we know everything about it. */
    idata[0].stack_info.stack_size = 0;
    idata[0].stack_info.stack = NULL;
    idata[0].register_info.register_count = args_size;
    idata[0].register_info.registers = NEW(fullinfo_type, args_size);
    idata[0].register_info.mask_count = 0;
    idata[0].register_info.masks = NULL;
    idata[0].and_flags = 0;	/* nothing needed */
    idata[0].or_flags = FLAG_REACHED; /* instruction reached */
    reg_ptr = idata[0].register_info.registers;

    if ((CVMmbAccessFlags(mb) & CVM_METHOD_ACC_STATIC) == 0) {
	/* A non static method.  If this is an <init> method, the first
	 * argument is an uninitialized object.  Otherwise it is an object of
	 * the given class type.  java.lang.Object.<init> is special since
	 * we don't call its superclass <init> method.
	 */
	if (CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb))
	        && context->currentclass_info != context->object_info) {
	    *reg_ptr++ = MAKE_FULLINFO(ITEM_InitObject, 0, 0);
	    idata[0].or_flags |= FLAG_NEED_CONSTRUCTOR;
	} else {
	    *reg_ptr++ = context->currentclass_info;
	}
    }

    {
	/* Fill in each of the arguments into the registers. */
	CVMMethodTypeID methodID = CVMmbNameAndTypeID(mb);
	CVMSigIterator iter;
	CVMClassTypeID arg;
	CVMtypeidGetSignatureIterator(methodID, &iter);
	while((arg = CVM_SIGNATURE_ITER_NEXT(iter)) != CVM_TYPEID_ENDFUNC) {
	    char fieldchar = 
		signature_to_fieldtype(context, arg, &full_info);
	    switch (fieldchar) {
	        case 'D': case 'L': 
		    *reg_ptr++ = full_info;
		    *reg_ptr++ = full_info + 1;
		    break;
	        default:
		    *reg_ptr++ = full_info;
		    break;
	    }
	}

	/* setup the return type */
	arg = CVM_SIGNATURE_ITER_RETURNTYPE(iter);
	if (arg == CVM_TYPEID_VOID) {
	    context->return_type = MAKE_FULLINFO(ITEM_Void, 0, 0);
	} else {
	    signature_to_fieldtype(context, arg, &full_info);
	    context->return_type = full_info;
	}
    }
    /* Indicate that we need to look at the first instruction. */
    idata[0].changed = JNI_TRUE;
}	


/* Run the data flow analysis, as long as there are things to change. */
static void
run_dataflow(context_type *context) {
    CVMMethodBlock *mb = context->mb;
    int max_stack_size = CVMjmdMaxStack(CVMmbJmd(mb));
    instruction_data_type *idata = context->instruction_data;
    int icount = context->instruction_count;
    jboolean work_to_do = JNI_TRUE;
    int inumber;

    /* Run through the loop, until there is nothing left to do. */
    while (work_to_do) {
	work_to_do = JNI_FALSE;
	for (inumber = 0; inumber < icount; inumber++) {
	    instruction_data_type *this_idata = &idata[inumber];
	    if (this_idata->changed) {
		register_info_type new_register_info;
		stack_info_type new_stack_info;
		flag_type new_and_flags, new_or_flags;
		
		this_idata->changed = JNI_FALSE;
		work_to_do = JNI_TRUE;
#ifdef CVM_TRACE
		if (verify_verbose) {
		    int opcode = this_idata->opcode;
		    const char *opname = (opcode == opc_invokeinit) ? 
				    "invokeinit" : CVMopnames[opcode];
		    CVMconsolePrintf("Instruction %d: ", inumber);
		    print_stack(context, &this_idata->stack_info);
		    print_registers(context, &this_idata->register_info);
		    print_flags(context, 
				this_idata->and_flags, this_idata->or_flags);
		    CVMconsolePrintf("  %s(%d)", opname,
				this_idata->operand.i);
		    fflush(stdout);
		}
#endif
		/* Make sure the registers and flags are appropriate */
		check_register_values(context, inumber);
		check_flags(context, inumber);

		/* Make sure the stack can deal with this instruction */
		pop_stack(context, inumber, &new_stack_info);

		/* Update the registers  and flags */
		update_registers(context, inumber, &new_register_info);
		update_flags(context, inumber, &new_and_flags, &new_or_flags);

		/* Update the stack. */
		push_stack(context, inumber, &new_stack_info);

		if (new_stack_info.stack_size > max_stack_size) 
		    CCerror(context, "Stack size too large");
#ifdef CVM_TRACE
		if (verify_verbose) {
		    CVMconsolePrintf("  ");
		    print_stack(context, &new_stack_info);
		    print_registers(context, &new_register_info);
		    print_flags(context, new_and_flags, new_or_flags);
		    fflush(stdout);
		}
#endif
		/* Add the new stack and register information to any
		 * instructions that can follow this instruction.     */
		merge_into_successors(context, inumber, 
				      &new_register_info, &new_stack_info,
				      new_and_flags, new_or_flags);
	    }
	}
    }
}


/* Make sure that the registers contain a legitimate value for the given
 * instruction.
*/

static void
check_register_values(context_type *context, int inumber)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int operand = this_idata->operand.i;
    int register_count = this_idata->register_info.register_count;
    fullinfo_type *registers = this_idata->register_info.registers;
    jboolean double_word = JNI_FALSE;	/* default value */
    int type;
    
    switch (opcode) {
        default:
	    return;
        case opc_iload: case opc_iinc:
	    type = ITEM_Integer; break;
        case opc_fload:
	    type = ITEM_Float; break;
        case opc_aload:
	    type = ITEM_Object; break;
        case opc_ret:
	    type = ITEM_ReturnAddress; break;
        case opc_lload:	
	    type = ITEM_Long; double_word = JNI_TRUE; break;
        case opc_dload:
	    type = ITEM_Double; double_word = JNI_TRUE; break;
    }
    if (!double_word) {
	fullinfo_type reg;
	/* Make sure we don't have an illegal register or one with wrong type */
	if (operand >= register_count) {
	    CCerror(context, 
		    "Accessing value from uninitialized register %d", operand);
	}
	reg = registers[operand];
	
	if (WITH_ZERO_EXTRA_INFO(reg) == MAKE_FULLINFO(type, 0, 0)) {
	    /* the register is obviously of the given type */
	    return;
	} else if (GET_INDIRECTION(reg) > 0 && type == ITEM_Object) {
	    /* address type stuff be used on all arrays */
	    return;
	} else if (GET_ITEM_TYPE(reg) == ITEM_ReturnAddress) { 
	    CCerror(context, "Cannot load return address from register %d", 
		              operand);
	    /* alternativeively 
	              (GET_ITEM_TYPE(reg) == ITEM_ReturnAddress)
                   && (opcode == opc_iload) 
		   && (type == ITEM_Object || type == ITEM_Integer)
	       but this never occurs
	    */
	} else if (reg == ITEM_InitObject && type == ITEM_Object) {
	    return;
	} else if (WITH_ZERO_EXTRA_INFO(reg) == 
		        MAKE_FULLINFO(ITEM_NewObject, 0, 0) && 
		   type == ITEM_Object) {
	    return;
        } else {
	    CCerror(context, "Register %d contains wrong type", operand);
	}
    } else {
	/* Make sure we don't have an illegal register or one with wrong type */
	if ((operand + 1) >= register_count) {
	    CCerror(context, 
		    "Accessing value from uninitialized register pair %d/%d", 
		    operand, operand+1);
	} else {
	    if ((registers[operand] == MAKE_FULLINFO(type, 0, 0)) &&
		(registers[operand + 1] == MAKE_FULLINFO(type + 1, 0, 0))) {
		return;
	    } else {
		CCerror(context, "Register pair %d/%d contains wrong type", 
		        operand, operand+1);
	    }
	}
    } 
}


/* Make sure the flags contain legitimate values for this instruction.
*/

static void
check_flags(context_type *context, int inumber)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    switch (opcode) {
        case opc_return:
	    /* We need a constructor, but we aren't guaranteed it's called */
	    if ((this_idata->or_flags & FLAG_NEED_CONSTRUCTOR) && 
		   !(this_idata->and_flags & FLAG_CONSTRUCTED))
		CCerror(context, "Constructor must call super() or this()");
	    /* fall through */
        case opc_ireturn: case opc_lreturn: 
        case opc_freturn: case opc_dreturn: case opc_areturn: 
	    if (this_idata->or_flags & FLAG_NO_RETURN)
		/* This method cannot exit normally */
		CCerror(context, "Cannot return normally");
        default:
	    break; /* nothing to do. */
    }
}

/* Make sure that the top of the stack contains reasonable values for the 
 * given instruction.  The post-pop values of the stack and its size are 
 * returned in *new_stack_info.
 */

static void 
pop_stack(context_type *context, int inumber, stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    stack_item_type *stack = this_idata->stack_info.stack;
    int stack_size = this_idata->stack_info.stack_size;
    char *stack_operands, *p;
    char *buffer = context->stack_operand_buffer;/* for holding manufactured argument lists */
    fullinfo_type *stack_extra_info_buffer = context->stack_info_buffer; /* save info popped off stack */
    fullinfo_type *stack_extra_info = stack_extra_info_buffer + CVM_VERIFY_TYPE_BUF_SIZE; 
    fullinfo_type full_info, put_full_info = 0;

    switch(opcode) {
        default:
	    /* For most instructions, we just use a built-in table */
	    stack_operands = opcode_in_out[opcode][0];
	    break;

        case opc_putstatic: case opc_putfield: {
	    /* The top thing on the stack depends on the signature of
	     * the object.                         */
	    int operand = this_idata->operand.i;
	    CVMClassTypeID signature = 
	        JVM_GetCPFieldSignature(context->constant_pool, operand);
	    char *ip = buffer;
#ifdef CVM_TRACE
	    if (verify_verbose) {
		print_formatted_fieldname(context, operand);
	    }
#endif
	    if (opcode == opc_putfield)
		*ip++ = 'A';	/* object for putfield */
	    *ip++ = signature_to_fieldtype(context, signature, &put_full_info);
	    *ip = '\0';
	    stack_operands = buffer;
	    break;
	}

	case opc_invokevirtual: case opc_invokespecial:        
        case opc_invokeinit:	/* invokespecial call to <init> */
	case opc_invokestatic: case opc_invokeinterface: {
	    /* The top stuff on the stack depends on the method signature */
	    int operand = this_idata->operand.i;
	    CVMSigIterator iter;
	    CVMClassTypeID argClassID;
	    CVMMethodTypeID signature = 
	        JVM_GetCPMethodSignature(context->constant_pool, operand);
	    char *ip = buffer;
#ifdef CVM_TRACE
	    if (verify_verbose) {
		print_formatted_methodname(context, operand);
	    }
#endif
	    if (opcode != opc_invokestatic) 
		/* First, push the object */
		*ip++ = (opcode == opc_invokeinit ? '@' : 'A');
	    CVMtypeidGetSignatureIterator(signature, &iter);
	    while((argClassID = CVM_SIGNATURE_ITER_NEXT(iter))
		  != CVM_TYPEID_ENDFUNC) {
		*ip++ =
		    signature_to_fieldtype(context, argClassID, &full_info);
		if (ip >= buffer + CVM_VERIFY_OPERAND_BUF_SIZE - 1)
		    CCerror(context, "Signature %s has too many arguments", 
			    signature);
	    }
	    *ip = 0;
	    stack_operands = buffer;
	    break;
	}

	case opc_multianewarray: {
	    /* Count can't be larger than 255. So can't overflow buffer */
	    int count = this_idata->operand2.i;	/* number of ints on stack */
	    memset(buffer, 'I', count);
	    buffer[count] = '\0';
	    stack_operands = buffer;
	    break;
	}

    } /* of switch */

    /* Run through the list of operands >>backwards<< */
    for (   p = stack_operands + strlen(stack_operands);
	    p > stack_operands; 
	    stack = stack->next) {
	fullinfo_type top_type = stack ? stack->item : 0;
	int type;
	int size;
	--p;
	type = *p;
	size = (type == 'D' || type == 'L') ? 2 : 1;
	*--stack_extra_info = top_type;
	if (stack == NULL) 
	    CCerror(context, "Unable to pop operand off an empty stack");
	switch (type) {
	    case 'I': 
	        if (top_type != MAKE_FULLINFO(ITEM_Integer, 0, 0))
		    CCerror(context, "Expecting to find integer on stack");
		break;
		
	    case 'F': 
		if (top_type != MAKE_FULLINFO(ITEM_Float, 0, 0))
		    CCerror(context, "Expecting to find float on stack");
		break;
		
	    case 'A':		/* object or array */
		if (   (GET_ITEM_TYPE(top_type) != ITEM_Object) 
		    && (GET_INDIRECTION(top_type) == 0)) { 
		    /* The thing isn't an object or an array.  Let's see if it's
		     * one of the special cases  */
		    if (  (WITH_ZERO_EXTRA_INFO(top_type) == 
			        MAKE_FULLINFO(ITEM_ReturnAddress, 0, 0))
			&& (opcode == opc_astore)) 
			break;
		    if (   (GET_ITEM_TYPE(top_type) == ITEM_NewObject 
			    || (GET_ITEM_TYPE(top_type) == ITEM_InitObject))
			&& ((opcode == opc_astore) 
                            || (opcode == opc_aload)
                            || (opcode == opc_ifnull) 
                            || (opcode == opc_ifnonnull)))
			break;
                    /* The 2nd edition VM of the specification allows field
                     * initializations before the superclass initializer,
                     * if the field is defined within the current class.
                     */
                    if (   (GET_ITEM_TYPE(top_type) == ITEM_InitObject)
                        && (opcode == opc_putfield)) {
                        int operand = this_idata->operand.i;
                        int access_bits = JVM_GetCPFieldModifiers(context->constant_pool,
                                                                  operand,
                                                                  context->class);
                        /* Note: This relies on the fact that
                         * JVM_GetCPFieldModifiers retrieves only local fields,
                         * and does not respect inheritance.
                         */
                         if (access_bits != -1) {
                             if (cp_index_to_class_fullinfo(
                                     context, operand, JVM_CONSTANT_Fieldref) ==
                                     context->currentclass_info) {
                                 top_type = context->currentclass_info;
                                 *stack_extra_info = top_type;
                                 break;
                             }
                         }
                    } 
		    CCerror(context, "Expecting to find object/array on stack");
		}
		break;

	    case '@': {		/* unitialized object, for call to <init> */
		int item_type = GET_ITEM_TYPE(top_type);
		if (item_type != ITEM_NewObject && item_type != ITEM_InitObject)
		    CCerror(context, 
			    "Expecting to find unitialized object on stack");
		break;
	    }

	    case 'O':		/* object, not array */
		if (WITH_ZERO_EXTRA_INFO(top_type) != 
		       MAKE_FULLINFO(ITEM_Object, 0, 0))
		    CCerror(context, "Expecting to find object on stack");
		break;

	    case 'a':		/* integer, object, or array */
		if (      (top_type != MAKE_FULLINFO(ITEM_Integer, 0, 0)) 
		       && (GET_ITEM_TYPE(top_type) != ITEM_Object)
		       && (GET_INDIRECTION(top_type) == 0))
		    CCerror(context, 
			    "Expecting to find object, array, or int on stack");
		break;

	    case 'D':		/* double */
		if (top_type != MAKE_FULLINFO(ITEM_Double, 0, 0))
		    CCerror(context, "Expecting to find double on stack");
		break;

	    case 'L':		/* long */
		if (top_type != MAKE_FULLINFO(ITEM_Long, 0, 0))
		    CCerror(context, "Expecting to find long on stack");
		break;

	    case ']':		/* array of some type */
		if (top_type == NULL_FULLINFO) { 
		    /* do nothing */
		} else switch(p[-1]) {
		    case 'I':	/* array of integers */
		        if (top_type != MAKE_FULLINFO(ITEM_Integer, 1, 0) && 
			    top_type != NULL_FULLINFO)
			    CCerror(context, 
				    "Expecting to find array of ints on stack");
			break;

		    case 'L':	/* array of longs */
		        if (top_type != MAKE_FULLINFO(ITEM_Long, 1, 0))
			    CCerror(context, 
				   "Expecting to find array of longs on stack");
			break;

		    case 'F':	/* array of floats */
		        if (top_type != MAKE_FULLINFO(ITEM_Float, 1, 0))
			    CCerror(context, 
				 "Expecting to find array of floats on stack");
			break;

		    case 'D':	/* array of doubles */
		        if (top_type != MAKE_FULLINFO(ITEM_Double, 1, 0))
			    CCerror(context, 
				"Expecting to find array of doubles on stack");
			break;

		    case 'A': {	/* array of addresses (arrays or objects) */
			int indirection = GET_INDIRECTION(top_type);
			if ((indirection == 0) || 
			    ((indirection == 1) && 
			        (GET_ITEM_TYPE(top_type) != ITEM_Object)))
			    CCerror(context, 
				"Expecting to find array of objects or arrays "
				    "on stack");
			break;
		    }
			
		    case 'B':	/* array of bytes */
		        if (top_type != MAKE_FULLINFO(ITEM_Byte, 1, 0))
			    CCerror(context, 
				  "Expecting to find array of bytes on stack");
			break;

		    case 'C':	/* array of characters */
		        if (top_type != MAKE_FULLINFO(ITEM_Char, 1, 0))
			    CCerror(context, 
				  "Expecting to find array of chars on stack");
			break;

		    case 'S':	/* array of shorts */
		        if (top_type != MAKE_FULLINFO(ITEM_Short, 1, 0))
			    CCerror(context, 
				 "Expecting to find array of shorts on stack");
			break;

		    case '?':	/* any type of array is okay */
		        if (GET_INDIRECTION(top_type) == 0) 
			    CCerror(context, 
				    "Expecting to find array on stack");
			break;

		    default:
			CCerror(context, "Internal error #1");
			break;
		}
		p -= 2;		/* skip over [ <char> */
		break;

	    case '1': case '2': case '3': case '4': /* stack swapping */
		if (top_type == MAKE_FULLINFO(ITEM_Double, 0, 0) 
		    || top_type == MAKE_FULLINFO(ITEM_Long, 0, 0)) {
		    if ((p > stack_operands) && (p[-1] == '+')) {
			context->swap_table[type - '1'] = top_type + 1;
			context->swap_table[p[-2] - '1'] = top_type;
			size = 2;
			p -= 2;
		    } else {
			CCerror(context, 
				"Attempt to split long or double on the stack");
		    }
		} else {
		    context->swap_table[type - '1'] = stack->item;
		    if ((p > stack_operands) && (p[-1] == '+')) 
			p--;	/* ignore */
		}
		break;
	    case '+':		/* these should have been caught. */
	    default:
		CCerror(context, "Internal error #2");
	}
	stack_size -= size;
    }
    
    /* For many of the opcodes that had an "A" in their field, we really 
     * need to go back and do a little bit more accurate testing.  We can, of
     * course, assume that the minimal type checking has already been done. 
     */
    switch (opcode) {
	default: break;
	case opc_aastore: {	/* array index object  */
	    fullinfo_type array_type = stack_extra_info[0];
	    fullinfo_type object_type = stack_extra_info[2];
	    fullinfo_type target_type = decrement_indirection(array_type);
	    if ((WITH_ZERO_EXTRA_INFO(target_type) == 
		             MAKE_FULLINFO(ITEM_Object, 0, 0)) &&
		 (WITH_ZERO_EXTRA_INFO(object_type) == 
		             MAKE_FULLINFO(ITEM_Object, 0, 0))) {
		/* I disagree.  But other's seem to think that we should allow
		 * an assignment of any Object to any array of any Object type.
		 * There will be an runtime error if the types are wrong.
		 */
		break;
	    }
	    if (!isAssignableTo(context, object_type, target_type))
		CCerror(context, "Incompatible types for storing into array of "
			            "arrays or objects");
	    break;
	}

	case opc_putfield: 
	case opc_getfield: 
        case opc_putstatic: {	
	    int operand = this_idata->operand.i;
	    fullinfo_type stack_object = stack_extra_info[0];
	    if (opcode == opc_putfield || opcode == opc_getfield) {
		if (!isAssignableTo
		        (context, 
			 stack_object, 
			 cp_index_to_class_fullinfo
			     (context, operand, CVM_CONSTANT_Fieldref))) {
		    CCerror(context, 
			    "Incompatible type for getting or setting field");
		}
		if (this_idata->protected && 
		    !isAssignableTo(context, stack_object, 
				    context->currentclass_info)) { 
		    CCerror(context, "Bad access to protected data");
		}
	    }
	    if (opcode == opc_putfield || opcode == opc_putstatic) { 
		int item = (opcode == opc_putfield ? 1 : 0);
		if (!isAssignableTo(context, 
				    stack_extra_info[item], put_full_info)) { 
		    CCerror(context, "Bad type in putfield/putstatic");
		}
	    }
	    break;
	}

        case opc_athrow: 
	    if (!isAssignableTo(context, stack_extra_info[0], 
				context->throwable_info)) {
		CCerror(context, "Can only throw Throwable objects");
	    }
	    break;

	case opc_aaload: {	/* array index */
	    /* We need to pass the information to the stack updater */
	    fullinfo_type array_type = stack_extra_info[0];
	    context->swap_table[0] = decrement_indirection(array_type);
	    break;
	}
	    
        case opc_invokevirtual: case opc_invokespecial: 
        case opc_invokeinit:
	case opc_invokeinterface: case opc_invokestatic: {
	    int operand = this_idata->operand.i;
	    CVMSigIterator iter;
	    CVMClassTypeID argClassID;
	    CVMMethodTypeID signature = 
	        JVM_GetCPMethodSignature(context->constant_pool, operand);
	    int item;
	    if (opcode == opc_invokestatic) {
		item = 0;
	    } else if (opcode == opc_invokeinit) {
		fullinfo_type init_type = this_idata->operand2.fi;
		fullinfo_type object_type = stack_extra_info[0];
		context->swap_table[0] = object_type; /* save value */
		if (GET_ITEM_TYPE(stack_extra_info[0]) == ITEM_NewObject) {
		    /* We better be calling the appropriate init.  Find the
		     * inumber of the "opc_new" instruction", and figure 
		     * out what the type really is. 
		     */
		    int new_inumber = GET_EXTRA_INFO(stack_extra_info[0]);
		    fullinfo_type target_type = idata[new_inumber].operand2.fi;
		    context->swap_table[1] = target_type;
		    if (target_type != init_type) {
			CCerror(context, "Call to wrong initialization method");
		    }
		} else {
		    /* We better be calling super() or this(). */
		    if (init_type != context->superclass_info && 
			init_type != context->currentclass_info) {
			CCerror(context, "Call to wrong initialization method");
		    }
		    context->swap_table[1] = context->currentclass_info;
		}
		item = 1;
	    } else {
		fullinfo_type target_type = this_idata->operand2.fi;
		fullinfo_type object_type = stack_extra_info[0];
		if (!isAssignableTo(context, object_type, target_type)){
		    CCerror(context, 
			    "Incompatible object argument for function call");
		}
                if (opcode == opc_invokespecial
                    && !isAssignableTo(context, object_type,
                                       context->currentclass_info)) {
                    /* Make sure object argument is assignment compatible to 
                       the current class */
                    CCerror(context,
                            "Incompatible object argument for invokespecial");
                }
		if (this_idata->protected 
		    && !isAssignableTo(context, object_type, 
				       context->currentclass_info)) { 
		    /* This is ugly. Special dispensation.  Arrays pretend to
		       implement public Object clone() even though they don't */
		    CVMMethodTypeID methodName = 
		        JVM_GetCPMethodName(context->constant_pool,
					    this_idata->operand.i);
		    int is_clone = 
			CVMtypeidIsSameName(methodName, CVMglobals.cloneTid);

		    if ((target_type == context->object_info) && 
			(GET_INDIRECTION(object_type) > 0) &&
			is_clone) {
		    } else { 
			CCerror(context, "Bad access to protected data");
		    }
		}
		item = 1;
	    }
	    CVMtypeidGetSignatureIterator(signature, &iter);
	    while((argClassID = CVM_SIGNATURE_ITER_NEXT(iter))
		  != CVM_TYPEID_ENDFUNC) {
		if (signature_to_fieldtype(context, argClassID,
					   &full_info) == 'A') {
		    if (!isAssignableTo(context, 
					stack_extra_info[item], full_info)) {
			CCerror(context, "Incompatible argument to function");
		    }
		}
		item++;
	    }
	    break;
	}
	    
        case opc_return:
	    if (context->return_type != MAKE_FULLINFO(ITEM_Void, 0, 0)) 
		CCerror(context, "Wrong return type in function");
	    break;

	case opc_ireturn: case opc_lreturn: case opc_freturn: 
        case opc_dreturn: case opc_areturn: {
	    fullinfo_type target_type = context->return_type;
	    fullinfo_type object_type = stack_extra_info[0];
	    if (!isAssignableTo(context, object_type, target_type)) {
		CCerror(context, "Wrong return type in function");
	    }
	    break;
	}

        case opc_new: {
	    /* Make sure that nothing on the stack already looks like what
	     * we want to create.  I can't image how this could possibly happen
	     * but we should test for it anyway, since if it could happen, the
	     * result would be an unitialized object being able to masquerade
	     * as an initialiozed one. 
	     */
	    stack_item_type *item;
	    for (item = stack; item != NULL; item = item->next) { 
		if (item->item == this_idata->operand.fi) { 
		    CCerror(context, 
			    "Uninitialized object on stack at creating point");
		}
	    }
	    /* Info for update_registers */
	    context->swap_table[0] = this_idata->operand.fi;
	    context->swap_table[1] = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    
	    break;
	}
    }
    new_stack_info->stack = stack;
    new_stack_info->stack_size = stack_size;
}


/* We've already determined that the instruction is legal.  Perform the 
 * operation on the registers, and return the updated results in
 * new_register_count_p and new_registers.
 */

static void
update_registers(context_type *context, int inumber, 
		 register_info_type *new_register_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int operand = this_idata->operand.i;
    int register_count = this_idata->register_info.register_count;
    fullinfo_type *registers = this_idata->register_info.registers;
    stack_item_type *stack = this_idata->stack_info.stack;
    int mask_count = this_idata->register_info.mask_count;
    mask_type *masks = this_idata->register_info.masks;

    /* Use these as default new values. */
    int            new_register_count = register_count;
    int            new_mask_count = mask_count;
    fullinfo_type *new_registers = registers;
    mask_type     *new_masks = masks;

    enum { ACCESS_NONE, ACCESS_SINGLE, ACCESS_DOUBLE } access = ACCESS_NONE;
    int i;
    
    /* Remember, we've already verified the type at the top of the stack. */
    switch (opcode) {
	default: break;
        case opc_istore: case opc_fstore: case opc_astore:
	    access = ACCESS_SINGLE;
	    goto continue_store;

        case opc_lstore: case opc_dstore:
	    access = ACCESS_DOUBLE;
	    goto continue_store;

	continue_store: {
	    /* We have a modification to the registers.  Copy them if needed. */
	    fullinfo_type stack_top_type = stack->item;
	    int max_operand = operand + ((access == ACCESS_DOUBLE) ? 1 : 0);
	    
	    if (     max_operand < register_count 
		  && registers[operand] == stack_top_type
		  && ((access == ACCESS_SINGLE) || 
		         (registers[operand + 1]== stack_top_type + 1))) 
		/* No changes have been made to the registers. */
		break;
	    new_register_count = MAX(max_operand + 1, register_count);
	    new_registers = NEW(fullinfo_type, new_register_count);
	    for (i = 0; i < register_count; i++) 
		new_registers[i] = registers[i];
	    for (i = register_count; i < new_register_count; i++) 
		new_registers[i] = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    new_registers[operand] = stack_top_type;
	    if (access == ACCESS_DOUBLE)
		new_registers[operand + 1] = stack_top_type + 1;
	    break;
	}
	 
        case opc_iload: case opc_fload: case opc_aload:
        case opc_iinc: case opc_ret:
	    access = ACCESS_SINGLE;
	    break;

        case opc_lload: case opc_dload:	
	    access = ACCESS_DOUBLE;
	    break;

        case opc_jsr: case opc_jsr_w:
	    for (i = 0; i < new_mask_count; i++) 
		if (new_masks[i].entry == operand) 
		    CCerror(context, "Recursive call to jsr entry");
	    new_masks = add_to_masks(context, masks, mask_count, operand);
	    new_mask_count++; 
	    break;
	    
        case opc_invokeinit: 
        case opc_new: {
	    /* For invokeinit, an uninitialized object has been initialized.
	     * For new, all previous occurrences of an uninitialized object
	     * from the same instruction must be made invalid.  
	     * We find all occurrences of swap_table[0] in the registers, and
	     * replace them with swap_table[1];   
	     */
	    fullinfo_type from = context->swap_table[0];
	    fullinfo_type to = context->swap_table[1];

	    int i;
	    for (i = 0; i < register_count; i++) {
		if (new_registers[i] == from) {
		    /* Found a match */
		    break;
		}
	    }
	    if (i < register_count) { /* We broke out loop for match */
		/* We have to change registers, and possibly a mask */
		jboolean copied_mask = JNI_FALSE;
		int k;
		new_registers = NEW(fullinfo_type, register_count);
		memcpy(new_registers, registers, 
		       register_count * sizeof(registers[0]));
		for ( ; i < register_count; i++) { 
		    if (new_registers[i] == from) { 
			new_registers[i] = to;
			for (k = 0; k < new_mask_count; k++) {
			    if (!IS_BIT_SET(new_masks[k].modifies, i)) { 
				if (!copied_mask) { 
				    new_masks = copy_masks(context, new_masks, 
							   mask_count);
				    copied_mask = JNI_TRUE;
				}
				SET_BIT(new_masks[k].modifies, i);
			    }
			}
		    }
		}
	    }
	    break;
	} 
    } /* of switch */

    if ((access != ACCESS_NONE) && (new_mask_count > 0)) {
	int i, j;
	for (i = 0; i < new_mask_count; i++) {
	    int *mask = new_masks[i].modifies;
	    if ((!IS_BIT_SET(mask, operand)) || 
		  ((access == ACCESS_DOUBLE) && 
		   !IS_BIT_SET(mask, operand + 1))) {
		new_masks = copy_masks(context, new_masks, mask_count);
		for (j = i; j < new_mask_count; j++) {
		    SET_BIT(new_masks[j].modifies, operand);
		    if (access == ACCESS_DOUBLE) 
			SET_BIT(new_masks[j].modifies, operand + 1);
		} 
		break;
	    }
	}
    }

    new_register_info->register_count = new_register_count;
    new_register_info->registers = new_registers;
    new_register_info->masks = new_masks;
    new_register_info->mask_count = new_mask_count;
}



/* We've already determined that the instruction is legal, and have updated 
 * the registers.  Update the flags, too. 
 */


static void
update_flags(context_type *context, int inumber, 
	     flag_type *new_and_flags, flag_type *new_or_flags)

{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    flag_type and_flags = this_idata->and_flags;
    flag_type or_flags = this_idata->or_flags;

    /* Set the "we've done a constructor" flag */
    if (this_idata->opcode == opc_invokeinit) {
	fullinfo_type from = context->swap_table[0];
	if (from == MAKE_FULLINFO(ITEM_InitObject, 0, 0))
	    and_flags |= FLAG_CONSTRUCTED;
    }
    *new_and_flags = and_flags;
    *new_or_flags = or_flags;
}



/* We've already determined that the instruction is legal.  Perform the 
 * operation on the stack;
 *
 * new_stack_size_p and new_stack_p point to the results after the pops have
 * already been done.  Do the pushes, and then put the results back there. 
 */

static void
push_stack(context_type *context, int inumber, stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int operand = this_idata->operand.i;

    int stack_size = new_stack_info->stack_size;
    stack_item_type *stack = new_stack_info->stack;
    char *stack_results;
	
    fullinfo_type full_info = 0;
    char buffer[5], *p;		/* actually [2] is big enough */

    /* We need to look at all those opcodes in which either we can't tell the
     * value pushed onto the stack from the opcode, or in which the value
     * pushed onto the stack is an object or array.  For the latter, we need
     * to make sure that full_info is set to the right value.
     */
    switch(opcode) {
        default:
	    stack_results = opcode_in_out[opcode][1]; 
	    break;

	case opc_ldc: case opc_ldc_w: case opc_ldc2_w: {
	    /* Look to constant pool to determine correct result. */
	    CVMConstantPool *constant_pool = context->constant_pool;
	    switch (CVMcpEntryType(constant_pool, operand)) {
		case CVM_CONSTANT_Integer:   
		    stack_results = "I"; break;
		case CVM_CONSTANT_Float:     
		    stack_results = "F"; break;
		case CVM_CONSTANT_Double:    
		    stack_results = "D"; break;
		case CVM_CONSTANT_Long:      
		    stack_results = "L"; break;
		case CVM_CONSTANT_StringICell: 
		    stack_results = "A"; 
		    full_info = context->string_info;
		    break;
		case CVM_CONSTANT_ClassTypeID:
		    stack_results = "A"; 
		    full_info = context->class_info;
		    break;
		default:
		    CCerror(context, "Internal error #3");
		    stack_results = "";	/* Never reached: keep lint happy */
	    }
	    break;
	}

        case opc_getstatic: case opc_getfield: {
	    /* Look to signature to determine correct result. */
	    int operand = this_idata->operand.i;
	    CVMClassTypeID signature =
		JVM_GetCPFieldSignature(context->constant_pool, operand);
#ifdef CVM_TRACE
	    if (verify_verbose) {
		print_formatted_fieldname(context, operand);
	    }
#endif
	    buffer[0] = signature_to_fieldtype(context, signature, &full_info);
	    buffer[1] = '\0';
	    stack_results = buffer;
	    break;
	}

	case opc_invokevirtual: case opc_invokespecial:
        case opc_invokeinit:
	case opc_invokestatic: case opc_invokeinterface: {
	    /* Look to signature to determine correct result. */
	    int operand = this_idata->operand.i;
	    CVMSigIterator iter;
	    CVMClassTypeID argClassID;
	    CVMMethodTypeID signature =
		JVM_GetCPMethodSignature(context->constant_pool, operand);
	    CVMtypeidGetSignatureIterator(signature, &iter);
	    argClassID = CVM_SIGNATURE_ITER_RETURNTYPE(iter);
	    if (argClassID == CVM_TYPEID_VOID) {
		stack_results = "";
	    } else {
		buffer[0] = signature_to_fieldtype(context, argClassID, 
						   &full_info);
		buffer[1] = '\0';
		stack_results = buffer;
	    }
	    break;
	}
	    
	case opc_aconst_null:
	    stack_results = opcode_in_out[opcode][1]; 
	    full_info = NULL_FULLINFO; /* special NULL */
	    break;

	case opc_new: 
	case opc_checkcast: 
	case opc_newarray: 
	case opc_anewarray: 
        case opc_multianewarray:
	    stack_results = opcode_in_out[opcode][1]; 
	    /* Conventiently, this result type is stored here */
	    full_info = this_idata->operand.fi;
	    break;
			
	case opc_aaload:
	    stack_results = opcode_in_out[opcode][1]; 
	    /* pop_stack() saved value for us. */
	    full_info = context->swap_table[0];
	    break;
		
	case opc_aload:
	    stack_results = opcode_in_out[opcode][1]; 
	    /* The register hasn't been modified, so we can use its value. */
	    full_info = this_idata->register_info.registers[operand];
	    break;
    } /* of switch */
    
    for (p = stack_results; *p != 0; p++) {
	int type = *p;
	stack_item_type *new_item = NEW(stack_item_type, 1);
	new_item->next = stack;
	stack = new_item;
	switch (type) {
	    case 'I': 
	        stack->item = MAKE_FULLINFO(ITEM_Integer, 0, 0); break;
	    case 'F': 
		stack->item = MAKE_FULLINFO(ITEM_Float, 0, 0); break;
	    case 'D': 
		stack->item = MAKE_FULLINFO(ITEM_Double, 0, 0); 
		stack_size++; break;
	    case 'L': 
		stack->item = MAKE_FULLINFO(ITEM_Long, 0, 0); 
		stack_size++; break;
	    case 'R': 
		stack->item = MAKE_FULLINFO(ITEM_ReturnAddress, 0, operand);
		break;
	    case '1': case '2': case '3': case '4': {
		/* Get the info saved in the swap_table */
		fullinfo_type stype = context->swap_table[type - '1'];
		stack->item = stype;
		if (stype == MAKE_FULLINFO(ITEM_Long, 0, 0) || 
		    stype == MAKE_FULLINFO(ITEM_Double, 0, 0)) {
		    stack_size++; p++;
		}
		break;
	    }
	    case 'A': 
		/* full_info should have the appropriate value. */
		CVMassert(full_info != 0);
		stack->item = full_info;
		break;
	    default:
		CCerror(context, "Internal error #4");

	    } /* switch type */
	stack_size++;
    } /* outer for loop */

    if (opcode == opc_invokeinit) {
	/* If there are any instances of "from" on the stack, we need to
	 * replace it with "to", since calling <init> initializes all versions
	 * of the object, obviously.     */
	fullinfo_type from = context->swap_table[0];
	stack_item_type *ptr;
	for (ptr = stack; ptr != NULL; ptr = ptr->next) {
	    if (ptr->item == from) {
		fullinfo_type to = context->swap_table[1];
		stack = copy_stack(context, stack);
		for (ptr = stack; ptr != NULL; ptr = ptr->next) 
		    if (ptr->item == from) ptr->item = to;
		break;
	    }
	}
    }

    new_stack_info->stack_size = stack_size;
    new_stack_info->stack = stack;
}


/* We've performed an instruction, and determined the new registers and stack 
 * value.  Look at all of the possibly subsequent instructions, and merge
 * this stack value into theirs. 
 */

static void
merge_into_successors(context_type *context, int inumber, 
		      register_info_type *register_info,
		      stack_info_type *stack_info, 
		      flag_type and_flags, flag_type or_flags)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int operand = this_idata->operand.i;
    struct handler_info_type *handler_info = context->handler_info;
    int handler_info_length = 
        JVM_GetMethodExceptionTableLength(context->mb); 


    int buffer[2];		/* default value for successors */
    int *successors = buffer;	/* table of successors */
    int successors_count;
    int i;
    
    switch (opcode) {
    default:
	successors_count = 1; 
	buffer[0] = inumber + 1;
	break;

    case opc_ifeq: case opc_ifne: case opc_ifgt: 
    case opc_ifge: case opc_iflt: case opc_ifle:
    case opc_ifnull: case opc_ifnonnull:
    case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmpgt: 
    case opc_if_icmpge: case opc_if_icmplt: case opc_if_icmple:
    case opc_if_acmpeq: case opc_if_acmpne:
	successors_count = 2;
	buffer[0] = inumber + 1; 
	buffer[1] = operand;
	break;

    case opc_jsr: case opc_jsr_w: 
	if (this_idata->operand2.i != UNKNOWN_RET_INSTRUCTION) 
	    idata[this_idata->operand2.i].changed = JNI_TRUE;
	/* FALLTHROUGH */
    case opc_goto: case opc_goto_w:
	successors_count = 1;
	buffer[0] = operand;
	break;


    case opc_ireturn: case opc_lreturn: case opc_return:	
    case opc_freturn: case opc_dreturn: case opc_areturn: 
    case opc_athrow:
	/* The testing for the returns is handled in pop_stack() */
	successors_count = 0;
	break;

    case opc_ret: {
	/* This is slightly slow, but good enough for a seldom used instruction.
	 * The EXTRA_ITEM_INFO of the ITEM_ReturnAddress indicates the
	 * address of the first instruction of the subroutine.  We can return
	 * to 1 after any instruction that jsr's to that instruction.
	 */
	if (this_idata->operand2.ip == NULL) {
	    fullinfo_type *registers = this_idata->register_info.registers;
	    int called_instruction = GET_EXTRA_INFO(registers[operand]);
	    int i, count, *ptr;;
	    for (i = context->instruction_count, count = 0; --i >= 0; ) {
		if (((idata[i].opcode == opc_jsr) ||
		     (idata[i].opcode == opc_jsr_w)) && 
		    (idata[i].operand.i == called_instruction)) 
		    count++;
	    }
	    this_idata->operand2.ip = ptr = NEW(int, count + 1);
	    *ptr++ = count;
	    for (i = context->instruction_count, count = 0; --i >= 0; ) {
		if (((idata[i].opcode == opc_jsr) ||
		     (idata[i].opcode == opc_jsr_w)) && 
		    (idata[i].operand.i == called_instruction)) 
		    *ptr++ = i + 1;
	    }
	}
	successors = this_idata->operand2.ip; /* use this instead */
	successors_count = *successors++;
	break;

    }

    case opc_tableswitch:
    case opc_lookupswitch: 
	successors = this_idata->operand.ip; /* use this instead */
	successors_count = *successors++;
	break;
    }

#ifdef CVM_TRACE
    if (verify_verbose) { 
	CVMconsolePrintf(" [");
	for (i = handler_info_length; --i >= 0; handler_info++)
	    if (handler_info->start <= inumber && handler_info->end > inumber)
		CVMconsolePrintf("%d* ", handler_info->handler);
	for (i = 0; i < successors_count; i++)
	    CVMconsolePrintf("%d ", successors[i]);
	CVMconsolePrintf(  "]\n");
    }
#endif

    handler_info = context->handler_info;
    for (i = handler_info_length; --i >= 0; handler_info++) {
	if (handler_info->start <= inumber && handler_info->end > inumber) {
	    int handler = handler_info->handler;
	    if (opcode != opc_invokeinit) {
		merge_into_one_successor(context, inumber, handler, 
					 &this_idata->register_info, /* old */
					 &handler_info->stack_info, 
					 (flag_type) (and_flags
						      & this_idata->and_flags),
					 (flag_type) (or_flags
						      | this_idata->or_flags),
					 JNI_TRUE);
	    } else {
		/* We need to be a little bit more careful with this 
		 * instruction.  Things could either be in the state before
		 * the instruction or in the state afterwards */
		fullinfo_type from = context->swap_table[0];
		flag_type temp_or_flags = or_flags;
		if (from == MAKE_FULLINFO(ITEM_InitObject, 0, 0))
		    temp_or_flags |= FLAG_NO_RETURN;
		merge_into_one_successor(context, inumber, handler, 
					 &this_idata->register_info, /* old */
					 &handler_info->stack_info, 
					 this_idata->and_flags,
					 this_idata->or_flags,
					 JNI_TRUE);
		merge_into_one_successor(context, inumber, handler, 
					 register_info, 
					 &handler_info->stack_info, 
					 and_flags, temp_or_flags, JNI_TRUE);
	    }
	}
    }
    for (i = 0; i < successors_count; i++) {
	int target = successors[i];
	if (target >= context->instruction_count) 
	    CCerror(context, "Falling off the end of the code");
	merge_into_one_successor(context, inumber, target, 
				 register_info, stack_info, and_flags, or_flags,
				 JNI_FALSE);
    }
}

/* We have a new set of registers and stack values for a given instruction.
 * Merge this new set into the values that are already there. 
 */

static void
merge_into_one_successor(context_type *context, 
			 int from_inumber, int to_inumber, 
			 register_info_type *new_register_info,
			 stack_info_type *new_stack_info,
			 flag_type new_and_flags, flag_type new_or_flags,
			 jboolean isException)
{
    instruction_data_type *idata = context->instruction_data;
    register_info_type register_info_buf;
    stack_info_type stack_info_buf;
#ifdef CVM_TRACE
    instruction_data_type *this_idata = &idata[to_inumber];
    register_info_type old_reg_info;
    stack_info_type old_stack_info;
    flag_type old_and_flags = 0;
    flag_type old_or_flags = 0;
#endif

#ifdef CVM_TRACE
    if (verify_verbose) {
	old_reg_info = this_idata->register_info;
	old_stack_info = this_idata->stack_info;
	old_and_flags = this_idata->and_flags;
	old_or_flags = this_idata->or_flags;
    }
#endif

    /* All uninitialized objects are set to "invalid" when jsr and
     * ret are executed. Thus uninitialized objects can't propagate
     * into or out of a subroutine.
     */
    if (idata[from_inumber].opcode == opc_ret ||
	idata[from_inumber].opcode == opc_jsr ||
	idata[from_inumber].opcode == opc_jsr_w) {
	int new_register_count = new_register_info->register_count;
	fullinfo_type *new_registers = new_register_info->registers;
        int i;
	stack_item_type *item;

	for (item = new_stack_info->stack; item != NULL; item = item->next) {
	    if (GET_ITEM_TYPE(item->item) == ITEM_NewObject) {
	        /* This check only succeeds for hand-contrived code.
		 * Efficiency is not an issue.
		 */
	        stack_info_buf.stack = copy_stack(context, 
						  new_stack_info->stack);
		stack_info_buf.stack_size = new_stack_info->stack_size;
		new_stack_info = &stack_info_buf;
		for (item = new_stack_info->stack; item != NULL; 
		     item = item->next) {
		    if (GET_ITEM_TYPE(item->item) == ITEM_NewObject) {
		        item->item = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		    }
		}
	        break;
	    }
	}
	for (i = 0; i < new_register_count; i++) {
	    if (GET_ITEM_TYPE(new_registers[i]) == ITEM_NewObject) {
	        /* This check only succeeds for hand-contrived code.
		 * Efficiency is not an issue.
		 */
	        fullinfo_type *new_set = NEW(fullinfo_type, 
					     new_register_count);
		for (i = 0; i < new_register_count; i++) {
		    fullinfo_type t = new_registers[i];
		    new_set[i] = GET_ITEM_TYPE(t) != ITEM_NewObject ?
		        t : MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		}
		register_info_buf.register_count = new_register_count;
		register_info_buf.registers = new_set;
		register_info_buf.mask_count = new_register_info->mask_count;
		register_info_buf.masks = new_register_info->masks;
		new_register_info = &register_info_buf;
		break;
	    }
	}
    }

    /* Returning from a subroutine is somewhat ugly.  The actual thing
     * that needs to get merged into the new instruction is a joinging
     * of info from the ret instruction with stuff in the jsr instruction 
     */
    if (idata[from_inumber].opcode == opc_ret && !isException) {
	int new_register_count = new_register_info->register_count;
	fullinfo_type *new_registers = new_register_info->registers;
	int new_mask_count = new_register_info->mask_count;
	mask_type *new_masks = new_register_info->masks;
	int operand = idata[from_inumber].operand.i;
	int called_instruction = GET_EXTRA_INFO(new_registers[operand]);
	instruction_data_type *jsr_idata = &idata[to_inumber - 1];
	register_info_type *jsr_reginfo = &jsr_idata->register_info;
	if (jsr_idata->operand2.i != from_inumber) {
	    if (jsr_idata->operand2.i != UNKNOWN_RET_INSTRUCTION)
		CCerror(context, "Multiple returns to single jsr");
	    jsr_idata->operand2.i = from_inumber;
	}
	if (jsr_reginfo->register_count == UNKNOWN_REGISTER_COUNT) {
	    /* We don't want to handle the returned-to instruction until 
	     * we've dealt with the jsr instruction.   When we get to the
	     * jsr instruction (if ever), we'll re-mark the ret instruction
	     */
	    ;
	} else { 
	    int register_count = jsr_reginfo->register_count;
	    fullinfo_type *registers = jsr_reginfo->registers;
	    int max_registers = MAX(register_count, new_register_count);
	    fullinfo_type *new_set = NEW(fullinfo_type, max_registers);
	    int *return_mask;
	    struct register_info_type new_new_register_info;
	    int i;
	    /* Make sure the place we're returning from is legal! */
	    for (i = new_mask_count; --i >= 0; ) 
		if (new_masks[i].entry == called_instruction) 
		    break;
	    if (i < 0)
		CCerror(context, "Illegal return from subroutine");
	    /* pop the masks down to the indicated one.  Remember the mask
	     * we're popping off. */
	    return_mask = new_masks[i].modifies;
	    new_mask_count = i;
	    for (i = 0; i < max_registers; i++) {
		if (IS_BIT_SET(return_mask, i)) 
		    new_set[i] = i < new_register_count ? 
			  new_registers[i] : MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		else 
		    new_set[i] = i < register_count ? 
			registers[i] : MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    }
	    new_new_register_info.register_count = max_registers;
	    new_new_register_info.registers      = new_set;
	    new_new_register_info.mask_count     = new_mask_count;
	    new_new_register_info.masks          = new_masks;


	    merge_stack(context, from_inumber, to_inumber, new_stack_info);
	    merge_registers(context, to_inumber - 1, to_inumber, 
			    &new_new_register_info);
            merge_flags(context, from_inumber, to_inumber,
                        new_and_flags, new_or_flags);
	}
    } else {
	merge_stack(context, from_inumber, to_inumber, new_stack_info);
	merge_registers(context, from_inumber, to_inumber, new_register_info);
	merge_flags(context, from_inumber, to_inumber, 
		    new_and_flags, new_or_flags);
    }

#ifdef CVM_TRACE
    if (verify_verbose && idata[to_inumber].changed) {
	register_info_type *register_info = &this_idata->register_info;
	stack_info_type *stack_info = &this_idata->stack_info;
	if (memcmp(&old_reg_info, register_info, sizeof(old_reg_info)) ||
	    memcmp(&old_stack_info, stack_info, sizeof(old_stack_info)) || 
	    (old_and_flags != this_idata->and_flags) || 
	    (old_or_flags != this_idata->or_flags)) {
	    CVMconsolePrintf("   %2d:", to_inumber);
	    print_stack(context, &old_stack_info);
	    print_registers(context, &old_reg_info);
	    print_flags(context, old_and_flags, old_or_flags);
	    CVMconsolePrintf(" => ");
	    print_stack(context, &this_idata->stack_info);
	    print_registers(context, &this_idata->register_info);
	    print_flags(context, this_idata->and_flags, this_idata->or_flags);
	    CVMconsolePrintf("\n");
	}
    }
#endif

}

static void
merge_stack(context_type *context, int from_inumber, int to_inumber, 
	    stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[to_inumber];

    int new_stack_size =  new_stack_info->stack_size;
    stack_item_type *new_stack = new_stack_info->stack;

    int stack_size = this_idata->stack_info.stack_size;

    if (stack_size == UNKNOWN_STACK_SIZE) {
	/* First time at this instruction.  Just copy. */
	this_idata->stack_info.stack_size = new_stack_size;
	this_idata->stack_info.stack = new_stack;
	this_idata->changed = JNI_TRUE;
    } else if (new_stack_size != stack_size) {
	CCerror(context, "Inconsistent stack height %d != %d",
		new_stack_size, stack_size);
    } else { 
	stack_item_type *stack = this_idata->stack_info.stack;
	stack_item_type *old, *new;
	jboolean change = JNI_FALSE;
	for (old = stack, new = new_stack; old != NULL; 
	           old = old->next, new = new->next) {
	    if (!isAssignableTo(context, new->item, old->item)) {
		change = JNI_TRUE;
		break;
	    }
	}
	if (change) {
	    stack = copy_stack(context, stack);
	    for (old = stack, new = new_stack; old != NULL; 
		          old = old->next, new = new->next) {
	        if (new == NULL) {
		    break;
		}
		old->item = merge_fullinfo_types(context, old->item, new->item,
						 JNI_FALSE);
		if (GET_ITEM_TYPE(old->item) == ITEM_Bogus) {
		    CCerror(context, "Mismatched stack types");
                }
		
	    }
	    if (old != NULL || new != NULL) {
	        CCerror(context, "Mismatched stack types");
	    }
	    this_idata->stack_info.stack = stack;
	    this_idata->changed = JNI_TRUE;
	}
    }
}

static void
merge_registers(context_type *context, int from_inumber, int to_inumber,
		 register_info_type *new_register_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[to_inumber];
    register_info_type	  *this_reginfo = &this_idata->register_info;

    int            new_register_count = new_register_info->register_count;
    fullinfo_type *new_registers = new_register_info->registers;
    int            new_mask_count = new_register_info->mask_count;
    mask_type     *new_masks = new_register_info->masks;
    

    if (this_reginfo->register_count == UNKNOWN_REGISTER_COUNT) {
	this_reginfo->register_count = new_register_count;
	this_reginfo->registers = new_registers;
	this_reginfo->mask_count = new_mask_count;
	this_reginfo->masks = new_masks;
	this_idata->changed = JNI_TRUE;
    } else {
	/* See if we've got new information on the register set. */
	int register_count = this_reginfo->register_count;
	fullinfo_type *registers = this_reginfo->registers;
	int mask_count = this_reginfo->mask_count;
	mask_type *masks = this_reginfo->masks;
	
	jboolean copy = JNI_FALSE;
	int i, j;
	if (register_count > new_register_count) {
	    /* Any register larger than new_register_count is now invalid */
	    this_reginfo->register_count = new_register_count;
	    register_count = new_register_count;
	    this_idata->changed = JNI_TRUE;
	}
	for (i = 0; i < register_count; i++) {
	    fullinfo_type prev_value = registers[i];
	    if ((i < new_register_count) 
		  ? (!isAssignableTo(context, new_registers[i], prev_value))
		  : (prev_value != MAKE_FULLINFO(ITEM_Bogus, 0, 0))) {
		copy = JNI_TRUE; 
		break;
	    }
	}
	
	if (copy) {
	    /* We need a copy.  So do it. */
	    fullinfo_type *new_set = NEW(fullinfo_type, register_count);
	    for (j = 0; j < i; j++) 
		new_set[j] =  registers[j];
	    for (j = i; j < register_count; j++) {
		if (i >= new_register_count) 
		    new_set[j] = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
		else 
		    new_set[j] = merge_fullinfo_types(context, 
						      new_registers[j], 
						      registers[j], JNI_FALSE);
	    }
	    /* Some of the end items might now be invalid. This step isn't 
	     * necessary, but it may save work later. */
	    while (   register_count > 0
		   && GET_ITEM_TYPE(new_set[register_count-1]) == ITEM_Bogus) 
		register_count--;
	    this_reginfo->register_count = register_count;
	    this_reginfo->registers = new_set;
	    this_idata->changed = JNI_TRUE;
	}
	if (mask_count > 0) { 
	    /* If the target instruction already has a sequence of masks, then
	     * we need to merge new_masks into it.  We want the entries on
	     * the mask to be the longest common substring of the two.
	     *   (e.g.   a->b->d merged with a->c->d should give a->d)
	     * The bits set in the mask should be the or of the corresponding
	     * entries in eachof the original masks.
	     */
	    int i, j, k;
	    int matches = 0;
	    int last_match = -1;
	    jboolean copy_needed = JNI_FALSE;
	    for (i = 0; i < mask_count; i++) {
		int entry = masks[i].entry;
		for (j = last_match + 1; j < new_mask_count; j++) {
		    if (new_masks[j].entry == entry) {
			/* We have a match */
			int *prev = masks[i].modifies;
			int *new = new_masks[j].modifies;
			matches++; 
			/* See if new_mask has bits set for "entry" that 
			 * weren't set for mask.  If so, need to copy. */
			for (k = context->bitmask_size - 1;
			       !copy_needed && k >= 0;
			       k--) 
			    if (~prev[k] & new[k])
				copy_needed = JNI_TRUE;
			last_match = j;
			break;
		    }
		}
	    }
	    if ((matches < mask_count) || copy_needed) { 
		/* We need to make a copy for the new item, since either the
		 * size has decreased, or new bits are set. */
		mask_type *copy = NEW(mask_type, matches);
		for (i = 0; i < matches; i++) {
		    copy[i].modifies = NEW(int, context->bitmask_size);
		}
		this_reginfo->masks = copy;
		this_reginfo->mask_count = matches;
		this_idata->changed = JNI_TRUE;
		matches = 0;
		last_match = -1;
		for (i = 0; i < mask_count; i++) {
		    int entry = masks[i].entry;
		    for (j = last_match + 1; j < new_mask_count; j++) {
			if (new_masks[j].entry == entry) {
			    int *prev1 = masks[i].modifies;
			    int *prev2 = new_masks[j].modifies;
			    int *new = copy[matches].modifies;
			    copy[matches].entry = entry;
			    for (k = context->bitmask_size - 1; k >= 0; k--) 
				new[k] = prev1[k] | prev2[k];
			    matches++;
			    last_match = j;
			    break;
			}
		    }
		}
	    }
	}
    }
}


static void 
merge_flags(context_type *context, int from_inumber, int to_inumber,
	    flag_type new_and_flags, flag_type new_or_flags) 
{
    /* Set this_idata->and_flags &= new_and_flags
           this_idata->or_flags |= new_or_flags
     */
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[to_inumber];
    flag_type this_and_flags = this_idata->and_flags;
    flag_type this_or_flags = this_idata->or_flags;
    flag_type merged_and = this_and_flags & new_and_flags;
    flag_type merged_or = this_or_flags | new_or_flags;
    
    if ((merged_and != this_and_flags) || (merged_or != this_or_flags)) {
	this_idata->and_flags = merged_and;
	this_idata->or_flags = merged_or;
	this_idata->changed = JNI_TRUE;
    }
}


/* Make a copy of a stack */

static stack_item_type *
copy_stack(context_type *context, stack_item_type *stack)
{
    int length;
    stack_item_type *ptr;
    
    /* Find the length */
    for (ptr = stack, length = 0; ptr != NULL; ptr = ptr->next, length++);
    
    if (length > 0) { 
	stack_item_type *new_stack = NEW(stack_item_type, length);
	stack_item_type *new_ptr;
	for (    ptr = stack, new_ptr = new_stack; 
	         ptr != NULL;
	         ptr = ptr->next, new_ptr++) {
	    new_ptr->item = ptr->item;
	    new_ptr->next = new_ptr + 1;
	}
	new_stack[length - 1].next = NULL;
	return new_stack;
    } else {
	return NULL;
    }
}


static mask_type *
copy_masks(context_type *context, mask_type *masks, int mask_count)
{
    mask_type *result = NEW(mask_type, mask_count);
    int bitmask_size = context->bitmask_size;
    int *bitmaps = NEW(int, mask_count * bitmask_size);
    int i;
    for (i = 0; i < mask_count; i++) { 
	result[i].entry = masks[i].entry;
	result[i].modifies = &bitmaps[i * bitmask_size];
	memcpy(result[i].modifies, masks[i].modifies, bitmask_size * sizeof(int));
    }
    return result;
}


static mask_type *
add_to_masks(context_type *context, mask_type *masks, int mask_count, int d)
{
    mask_type *result = NEW(mask_type, mask_count + 1);
    int bitmask_size = context->bitmask_size;
    int *bitmaps = NEW(int, (mask_count + 1) * bitmask_size);
    int i;
    for (i = 0; i < mask_count; i++) { 
	result[i].entry = masks[i].entry;
	result[i].modifies = &bitmaps[i * bitmask_size];
	memcpy(result[i].modifies, masks[i].modifies, bitmask_size * sizeof(int));
    }
    result[mask_count].entry = d;
    result[mask_count].modifies = &bitmaps[mask_count * bitmask_size];
    memset(result[mask_count].modifies, 0, bitmask_size * sizeof(int));
    return result;
}
    


/* We create our own storage manager, since we malloc lots of little items, 
 * and I don't want to keep track of when they become free.  I sure wish that
 * we had heaps, and I could just free the heap when done. 
 */

#define CCSegSize 2000

struct CCpool {			/* a segment of allocated memory in the pool */
    struct CCpool *next;
    int segSize;		/* almost always CCSegSize */
    /*
     * Offsets into 'space' have to be correctly aligned for pointer
     * size access (see CCalloc() below). The cleanest way to make this
     * sure for offset 0 is to define 'space' as an array of pointers.
     */
    void *space[CCSegSize / sizeof(void *)];
};

/* Initialize the context's heap. */
static void CCinit(context_type *context)
{
    struct CCpool *new = (struct CCpool *) malloc(sizeof(struct CCpool));
    /* Set context->CCroot to 0 if new == 0 to tell CCdestroy to lay off */
    context->CCroot = context->CCcurrent = new;
    if (new == 0) {
	CCout_of_memory(context);
    }
    new->next = NULL;
    new->segSize = CCSegSize;
    context->CCfree_size = CCSegSize;
    context->CCfree_ptr = (char*)&new->space[0];
}


/* Reuse all the space that we have in the context's heap. */
static void CCreinit(context_type *context)
{
    struct CCpool *first = context->CCroot;
    context->CCcurrent = first;
    context->CCfree_size = CCSegSize;
    context->CCfree_ptr = (char*)&first->space[0];
}

/* Destroy the context's heap. */
static void CCdestroy(context_type *context)
{
    struct CCpool *this = context->CCroot;
    while (this) {
	struct CCpool *next = this->next;
	free(this);
	this = next;
    }
    /* These two aren't necessary.  But can't hurt either */
    context->CCroot = context->CCcurrent = NULL;
    context->CCfree_ptr = 0;
}

/* Allocate an object of the given size from the context's heap. */
static void *
CCalloc(context_type *context, int size, jboolean zero)
{

    register char *p;
    /* Round CC to the size of a pointer */
    size = (size + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1);

    if (context->CCfree_size <  size) {
	struct CCpool *current = context->CCcurrent;
	struct CCpool *new;
	if (size > CCSegSize) {	/* we need to allocate a special block */
	    new = (struct CCpool *)malloc(sizeof(struct CCpool) + 
					  (size - CCSegSize));
	    if (new == 0) {
		CCout_of_memory(context);
	    }
	    new->next = current->next;
	    new->segSize = size;
	    current->next = new;
	} else {
	    new = current->next;
	    if (new == NULL) {
		new = (struct CCpool *) malloc(sizeof(struct CCpool));
		if (new == 0) {
		    CCout_of_memory(context);
		}
		current->next = new;
		new->next = NULL;
		new->segSize = CCSegSize;
	    }
	}
	context->CCcurrent = new;
	context->CCfree_ptr = (char *)&new->space[0];
	context->CCfree_size = new->segSize;
    }
    p = context->CCfree_ptr;
    context->CCfree_ptr += size;
    context->CCfree_size -= size;
    if (zero) 
	memset(p, 0, size);
    return p;
}

/* Get the class associated with a particular field or method or class in the
 * constant pool.  If is_field is true, we've got a field or method.  If 
 * false, we've got a class.
 */
static fullinfo_type
cp_index_to_class_fullinfo(context_type *context, int cp_index, int kind)
{
    CVMExecEnv *ee = context->ee;
    fullinfo_type result;
    CVMClassTypeID classname;
    switch (kind) {
    case CVM_CONSTANT_ClassTypeID: 
    case CVM_CONSTANT_ClassBlock: 
        classname = JVM_GetCPClassName(ee, context->constant_pool,
				       cp_index);
	break;
    case CVM_CONSTANT_Methodref:
        classname = JVM_GetCPMethodClassName(ee, context->constant_pool,
					     cp_index);
	break;
    case CVM_CONSTANT_Fieldref:
        classname = JVM_GetCPFieldClassName(ee, context->constant_pool,
					    cp_index);
	break;
    default:
	classname = 0; /* get rid of compiler warning */
        CCerror(context, "Internal error #5");
    }
    
    if (CVMtypeidIsArray(classname)) {
	signature_to_fieldtype(context, classname, &result);
    } else {
	result = make_class_info_from_name(context, classname);
    }
    return result;
}

static void 
CCerror (context_type *context, char *format, ...)
{
    va_list args;
    CVMClassBlock *cb = context->class;
    CVMClassTypeID classname = CVMcbClassName(cb);
    int n = 0;
    if (context->mb != 0) {
	CVMMethodTypeID methodname = CVMmbNameAndTypeID(context->mb);
	n += jio_snprintf(context->message, context->message_buf_len,
			  "(class: %!C, method: %!M)", 
			  classname, methodname);
#if 0 /* See comment in verify_field(). */
    } else if (context->fb != 0 ) {
        CVMFieldTypeID fieldname =
	    CVMfbNameAndTypeID(context->fb);
	n += jio_snprintf(context->message, context->message_buf_len,
			  "(class: %!C, field: %!F) ", 
			  classname, fieldname);
#endif
    } else {
        n += jio_snprintf(context->message, context->message_buf_len,
			  "(class: %!C) ", classname);
    }
    if (n >= 0) {
        va_start(args, format);
	jio_vsnprintf(context->message + n, context->message_buf_len - n,
		      format, args);        
	va_end(args);
    }

    context->err_code = CV_VERIFYERR;
    longjmp(context->jump_buffer, 1);
}

static void 
CCFerror (context_type *context, char *format, ...)
{
    va_list args;
    CVMClassBlock *cb = context->class;
    CVMClassTypeID classname = CVMcbClassName(cb);
    int n = 0;
    if (context->mb != 0) {
	CVMMethodTypeID methodname = CVMmbNameAndTypeID(context->mb);
	n += jio_snprintf(context->message, context->message_buf_len,
			  "(class: %!C, method: %!M)", 
			  classname, methodname);
    } else {
        n += jio_snprintf(context->message, context->message_buf_len,
			  "(class: %!C) ", classname);
    }
    if (n >= 0) {
        va_start(args, format);
	jio_vsnprintf(context->message + n, context->message_buf_len - n,
		      format, args);        
	va_end(args);
    }

    context->err_code = CV_FORMATERR;
    longjmp(context->jump_buffer, 1);
}

static void
CCout_of_memory(context_type *context)
{
    CCerror(context, "Out Of Memory");
}


static char 
signature_to_fieldtype(context_type *context, 
			CVMFieldTypeID fieldID, fullinfo_type *full_info_p)
{
    fullinfo_type full_info = MAKE_FULLINFO(0, 0, 0);
    char result;
    int array_depth = 0;
    
    if (CVMtypeidIsArray(fieldID)) {
	array_depth = CVMtypeidGetArrayDepth(fieldID);
	fieldID = CVMtypeidGetArrayBasetype(fieldID);
    }

    if (!CVMtypeidIsPrimitive(fieldID)) {
	full_info = make_class_info_from_name(context, fieldID);
	result = 'A';
    } else switch(fieldID) {
        default:
	    full_info = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    result = 0; 
	    break;
	    
        case CVM_TYPEID_BOOLEAN: case CVM_TYPEID_BYTE: 
	    full_info = (array_depth > 0)
		? MAKE_FULLINFO(ITEM_Byte, 0, 0)
		: MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;
	    
        case CVM_TYPEID_CHAR:
	    full_info = (array_depth > 0)
		? MAKE_FULLINFO(ITEM_Char, 0, 0)
		: MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;
	    
        case CVM_TYPEID_SHORT: 
	    full_info = (array_depth > 0)
		? MAKE_FULLINFO(ITEM_Short, 0, 0)
		: MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;
	    
        case CVM_TYPEID_INT:
	    full_info = MAKE_FULLINFO(ITEM_Integer, 0, 0);
	    result = 'I'; 
	    break;
	    
        case CVM_TYPEID_FLOAT:
	    full_info = MAKE_FULLINFO(ITEM_Float, 0, 0);
	    result = 'F'; 
	    break;
	
        case CVM_TYPEID_DOUBLE:
	    full_info = MAKE_FULLINFO(ITEM_Double, 0, 0);
	    result = 'D'; 
	    break;
	    
        case CVM_TYPEID_LONG:
	    full_info = MAKE_FULLINFO(ITEM_Long, 0, 0);
	    result = 'L'; 
	    break;
    } /* end of switch */

    if (array_depth == 0 || result == 0) { 
	/* either not an array, or result is invalid */
	*full_info_p = full_info;
	return result;
    } else {
	if (array_depth > MAX_ARRAY_DIMENSIONS) 
	    CCerror(context, "Array with too many dimensions");
	*full_info_p = MAKE_FULLINFO(GET_ITEM_TYPE(full_info),
				     array_depth, 
				     GET_EXTRA_INFO(full_info));
	return 'A';
    }
}


/* Given an array type, create the type that has one less level of 
 * indirection.
 */

static fullinfo_type
decrement_indirection(fullinfo_type array_info)
{
    if (array_info == NULL_FULLINFO) { 
	return NULL_FULLINFO;
    } else { 
	int type = GET_ITEM_TYPE(array_info);
	int indirection = GET_INDIRECTION(array_info) - 1;
	int extra_info = GET_EXTRA_INFO(array_info);
	if (   (indirection == 0) 
	       && ((type == ITEM_Short || type == ITEM_Byte || type == ITEM_Char)))
	    type = ITEM_Integer;
	return MAKE_FULLINFO(type, indirection, extra_info);
    }
}


/* See if we can assign an object of the "from" type to an object
 * of the "to" type.
 */

static jboolean isAssignableTo(context_type *context, 
			     fullinfo_type from, fullinfo_type to)
{
    return (merge_fullinfo_types(context, from, to, JNI_TRUE) == to);
}

#ifdef CVM_CSTACKANALYSIS
static CVMBool
CVMCstackDummy2object_fullinfo_to_classclass(context_type *context,
                                             fullinfo_type target,
                                             CVMClassBlock **cb_ptr)
{
    /* C stack checks */
    if (!CVMCstackCheckSize(context->ee,
        CVM_REDZONE_CVMCstackmerge_fullinfo_to_types,
        "CVM_REDZONE_CVMCstackobject_fullinfo_to_classclass",
        CVM_TRUE)) {
        return CVM_FALSE;
    }    
    *cb_ptr = object_fullinfo_to_classclass(context, target);
    return CVM_TRUE;
}

static CVMBool
CVMCstackDummy2merge_fullinfo_types(context_type *context,
                                    fullinfo_type value_base,
                                    fullinfo_type target_base,
                                    jboolean for_assignment,
                                    fullinfo_type *result_base)
{
    /* C stack checks */
    if (!CVMCstackCheckSize(context->ee,
        CVM_REDZONE_CVMCstackmerge_fullinfo_to_types,
        "CVM_REDZONE_CVMCstackmerge_fullinfo_to_types",
        CVM_TRUE)) {
        return CVM_FALSE;
    }    
    *result_base = merge_fullinfo_types(context, value_base, target_base,
                                        for_assignment);

    return CVM_TRUE;
}
#endif

/* Given two fullinfo_type's, find their lowest common denominator.  If
 * the assignable_p argument is non-null, we're really just calling to find
 * out if "<target> := <value>" is a legitimate assignment.  
 *
 * We treat all interfaces as if they were of type java/lang/Object, since the
 * runtime will do the full checking.
 */
static fullinfo_type 
merge_fullinfo_types(context_type *context, 
		     fullinfo_type value, fullinfo_type target,
		     jboolean for_assignment)
{
    CVMExecEnv *ee = context->ee;

    if (value == target) {
	/* If they're identical, clearly just return what we've got */
	return value;
    }

    /* Both must be either arrays or objects to go further */
    if (GET_INDIRECTION(value) == 0 && GET_ITEM_TYPE(value) != ITEM_Object)
	return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
    if (GET_INDIRECTION(target) == 0 && GET_ITEM_TYPE(target) != ITEM_Object)
	return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
    
    /* If either is NULL, return the other. */
    if (value == NULL_FULLINFO) 
	return target;
    else if (target == NULL_FULLINFO)
	return value;

    /* If either is java/lang/Object, that's the result. */
    if (target == context->object_info)
	return target;
    else if (value == context->object_info) {
	/* Note:  For assignments, Interface := Object, return Interface
	 * rather than Object, so that isAssignableTo() will get the right
	 * result.      */
	if (for_assignment && (WITH_ZERO_EXTRA_INFO(target) == 
			          MAKE_FULLINFO(ITEM_Object, 0, 0))) {
	    CVMClassBlock *cb = NULL;

#ifdef CVM_CSTACKANALYSIS
            /* C stack check */
            if (!CVMCstackDummy2object_fullinfo_to_classclass(context,
                target, &cb)) {
                /* exception already thrown */
                CVMclearLocalException(ee);
                return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
            }
#else
	    cb = object_fullinfo_to_classclass(context, target);
#endif
	    {
	    int is_interface = cb && CVMcbIs(cb, INTERFACE);
	    if (is_interface)
		return target;
	    }
	}
	return value;
    }
    if (GET_INDIRECTION(value) > 0 || GET_INDIRECTION(target) > 0) {
	/* At least one is an array.  Neither is java/lang/Object or NULL.
	 * Moreover, the types are not identical.
	 * The result must either be Object, or an array of some object type.
	 */
        fullinfo_type value_base, target_base;
	int dimen_value = GET_INDIRECTION(value);
	int dimen_target = GET_INDIRECTION(target);

	if (target == context->cloneable_info || 
	    target == context->serializable_info) {
	    return target;
	}

	if (value == context->cloneable_info || 
	    value == context->serializable_info) {
	    return value;
	}
	
	/* First, if either item's base type isn't ITEM_Object, promote it up
         * to an object or array of object.  If either is elemental, we can
	 * punt.
    	 */
	if (GET_ITEM_TYPE(value) != ITEM_Object) { 
	    if (dimen_value == 0)
		return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    dimen_value--;
	    value = MAKE_Object_ARRAY(dimen_value);
	    
	}
	if (GET_ITEM_TYPE(target) != ITEM_Object) { 
	    if (dimen_target == 0)
		return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	    dimen_target--;
	    target = MAKE_Object_ARRAY(dimen_target);
	}
	/* Both are now objects or arrays of some sort of object type */
        value_base = WITH_ZERO_INDIRECTION(value);
	target_base = WITH_ZERO_INDIRECTION(target);
	if (dimen_value == dimen_target) { 
            /* Arrays of the same dimension.  Merge their base types. */
            fullinfo_type  result_base;

#ifdef CVM_CSTACKANALYSIS
           /* C stack check */
            if (!CVMCstackDummy2merge_fullinfo_types(context, value_base,
                target_base, for_assignment, &result_base)) {
                /* exception already thrown */
                CVMclearLocalException(ee);
                return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
            }
	    result_base=0;
#else
            /* C stack check */
            if (!CVMCstackCheckSize(ee,
                                    CVM_REDZONE_CVMCstackmerge_fullinfo_to_types,
                                    "merge_fullinfo_types",
                                    CVM_FALSE)) {
	        CCerror(context, "C stack limit exceeded");
                return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
            }
	    result_base = merge_fullinfo_types(context, value_base, target_base,
                                            for_assignment);
#endif
	    if (result_base == MAKE_FULLINFO(ITEM_Bogus, 0, 0))
		return result_base;
	    return MAKE_FULLINFO(ITEM_Object, dimen_value,
				 GET_EXTRA_INFO(result_base));
	} else { 
            /* Arrays of different sizes. If the smaller dimension array's base
             * type is java/lang/Cloneable or java/io/Serializable, return it.
             * Otherwise return java/lang/Object with a dimension of the smaller
             * of the two */
            if (dimen_value < dimen_target) {
                if (value_base == context->cloneable_info ||
                    value_base == context ->serializable_info) {
                    return value;
                }
                return MAKE_Object_ARRAY(dimen_value);
            } else {
                if (target_base == context->cloneable_info ||
                    target_base == context->serializable_info) {
                    return target;
                }
                return MAKE_Object_ARRAY(dimen_target);
            }
	}
    } else {
	/* Both are non-array objects. Neither is java/lang/Object or NULL */
	CVMClassBlock *cb_value=NULL, *cb_target=NULL, *cb_super_value=NULL, *cb_super_target=NULL;
	fullinfo_type result_info=0;

	/* Let's get the classes corresponding to each of these.  Treat 
	 * interfaces as if they were java/lang/Object.  See note above. */

#ifdef CVM_CSTACKANALYSIS
        /* C stack check */
        if (!CVMCstackDummy2object_fullinfo_to_classclass(context,
            target, &cb_target)) {
            /* exception already thrown */
            CVMclearLocalException(ee);
            return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
        }
#else
	cb_target = object_fullinfo_to_classclass(context, target);
#endif
	if (cb_target == 0) 
	    return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	if (CVMcbIs(cb_target, INTERFACE))
	    return for_assignment ? target : context->object_info;

#ifdef CVM_CSTACKANALYSIS
        /* C stack check */
        if (!CVMCstackDummy2object_fullinfo_to_classclass(context,
            value, &cb_value)) {
            /* exception already thrown */
            CVMclearLocalException(ee);
            return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
        }
#else
	cb_value = object_fullinfo_to_classclass(context, value);
#endif
	if (cb_value == 0) 
	    return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
	if (CVMcbIs(cb_value, INTERFACE))
	    return context->object_info;
	
	/* If this is for assignment of target := value, we just need to see if
	 * cb_target is a superclass of cb_value.  Save ourselves a lot of 
	 * work.
	 */
	if (for_assignment) {
	    cb_super_value = CVMcbSuperclass(cb_value); 
	    while (cb_super_value != 0) {
	        CVMClassBlock *tmp_cb;
	        if (cb_super_value == cb_target) {
		    return target;
		}
		tmp_cb =  CVMcbSuperclass(cb_super_value); 
		cb_super_value = tmp_cb;
	    }
	    return context->object_info;
	}

	/* Find out whether cb_value or cb_target is deeper in the class
	 * tree by moving both toward the root, and seeing who gets there
	 * first.                                                          */
	cb_super_value = CVMcbSuperclass(cb_value);
	cb_super_target = CVMcbSuperclass(cb_target);
	while((cb_super_value != 0) &&
	      (cb_super_target != 0)) {
	    CVMClassBlock *tmp_cb;
	    /* Optimization.  If either hits the other when going up looking
	     * for a parent, then might as well return the parent immediately */
	    if (cb_super_value == cb_target) {
		return target;
	    }
	    if (cb_super_target == cb_value) {
		return value;
	    }
	    tmp_cb = CVMcbSuperclass(cb_super_value);
	    cb_super_value = tmp_cb;

	    tmp_cb = CVMcbSuperclass(cb_super_target);
	    cb_super_target = tmp_cb;
	} 
	/* At most one of the following two while clauses will be executed. 
	 * Bring the deeper of cb_target and cb_value to the depth of the 
	 * shallower one. 
	 */
	while (cb_super_value != 0) { 
	  /* cb_value is deeper */
	    CVMClassBlock *cb_tmp;

	    cb_tmp = CVMcbSuperclass(cb_super_value);
	    cb_super_value = cb_tmp;

	    cb_tmp = CVMcbSuperclass(cb_value);
	    cb_value = cb_tmp;
	}
	while (cb_super_target != 0) { 
	  /* cb_target is deeper */
	    CVMClassBlock *cb_tmp;
	    
	    cb_tmp = CVMcbSuperclass(cb_super_target);
	    cb_super_target = cb_tmp;

	    cb_tmp = CVMcbSuperclass(cb_target);
	    cb_target = cb_tmp;
	}
    
	/* Walk both up, maintaining equal depth, until a join is found.  We
	 * know that we will find one.  */
	while (cb_value != cb_target) { 
	    CVMClassBlock *cb_tmp;
	    cb_tmp = CVMcbSuperclass(cb_value);
	    cb_value = cb_tmp;
	    cb_tmp = CVMcbSuperclass(cb_target);
	    cb_target = cb_tmp;
	}
	result_info = make_class_info(context, cb_value);
	return result_info;
    } /* both items are classes */
}


/* Given a fullinfo_type corresponding to an Object, return the CVMClassBlock*
 * of that type.
 *
 * This function always returns a global reference!
 */

static CVMClassBlock *
object_fullinfo_to_classclass(context_type *context, fullinfo_type classinfo)
{
    unsigned short info = GET_EXTRA_INFO(classinfo);
    return ID_to_class(context, info);
}

#ifdef CVM_TRACE

/* Below are for debugging. */

static void print_fullinfo_type(context_type *, fullinfo_type, jboolean);

static void 
print_stack(context_type *context, stack_info_type *stack_info)
{
    stack_item_type *stack = stack_info->stack;
    if (stack_info->stack_size == UNKNOWN_STACK_SIZE) {
	CVMconsolePrintf("x");
    } else {
	CVMconsolePrintf("(");
	for ( ; stack != 0; stack = stack->next) 
	    print_fullinfo_type(context, stack->item, 
	       	(jboolean)(verify_verbose > 1 ? JNI_TRUE : JNI_FALSE));
	CVMconsolePrintf(")");
    }
}	

static void
print_registers(context_type *context, register_info_type *register_info)
{
    int register_count = register_info->register_count;
    if (register_count == UNKNOWN_REGISTER_COUNT) {
	CVMconsolePrintf("x");
    } else {
	fullinfo_type *registers = register_info->registers;
	int mask_count = register_info->mask_count;
	mask_type *masks = register_info->masks;
	int i, j;

	CVMconsolePrintf("{");
	for (i = 0; i < register_count; i++) 
	    print_fullinfo_type(context, registers[i], 
		(jboolean)(verify_verbose > 1 ? JNI_TRUE : JNI_FALSE));
	CVMconsolePrintf("}");
	for (i = 0; i < mask_count; i++) { 
	    char *separator = "";
	    int *modifies = masks[i].modifies;
	    CVMconsolePrintf("<%d: ", masks[i].entry);
	    for (j = 0; 
		 j < JVM_GetMethodLocalsCount(context->mb); 
		 j++) 
		if (IS_BIT_SET(modifies, j)) {
		    CVMconsolePrintf("%s%d", separator, j);
		    separator = ",";
		}
	    CVMconsolePrintf(">");
	}
    }
}


static void
print_flags(context_type *context, flag_type and_flags, flag_type or_flags)
{ 
    if (and_flags != ((flag_type)-1) || or_flags != 0) {
	CVMconsolePrintf("<%x %x>", and_flags, or_flags);
    }
}	    

static void 
print_fullinfo_type(context_type *context, fullinfo_type type, jboolean verbose) 
{
    int i;
    int indirection;
    int newObjectDepth = 0;

print_fullinfo_iterate:
    indirection = GET_INDIRECTION(type);
    for (i = indirection; i-- > 0; )
	CVMconsolePrintf("[");
    switch (GET_ITEM_TYPE(type)) {
        case ITEM_Integer:       
	    CVMconsolePrintf("I"); break;
	case ITEM_Float:         
	    CVMconsolePrintf("F"); break;
	case ITEM_Double:        
	    CVMconsolePrintf("D"); break;
	case ITEM_Double_2:      
	    CVMconsolePrintf("d"); break;
	case ITEM_Long:          
	    CVMconsolePrintf("L"); break;
	case ITEM_Long_2:        
	    CVMconsolePrintf("l"); break;
	case ITEM_ReturnAddress: 
	    CVMconsolePrintf("a"); break;
	case ITEM_Object:        
	    if (!verbose) {
		CVMconsolePrintf("A");
	    } else {
		unsigned short extra = GET_EXTRA_INFO(type);
		if (extra == 0) {
		    CVMconsolePrintf("/Null/");
		} else {
		    CVMClassTypeID name = ID_to_class_name(context, extra);
		    CVMconsolePrintf("/%!C/", name);
		}
	    }
	    break;
	case ITEM_Char:
	    CVMconsolePrintf("C"); break;
	case ITEM_Short:
	    CVMconsolePrintf("S"); break;
	case ITEM_Byte:
	    CVMconsolePrintf("B"); break;
        case ITEM_NewObject:
	    if (!verbose) {
		CVMconsolePrintf("@");
	    } else {
		int inum = GET_EXTRA_INFO(type);
		type = 
		    context->instruction_data[inum].operand2.fi;
		CVMconsolePrintf(">");
		newObjectDepth += 1;
		verbose = JNI_TRUE;
		goto print_fullinfo_iterate;
		/* formerly print_fullinfo_type(context, real_type, JNI_TRUE); */
		/* CVMconsolePrintf("<"); */
	    }
	    break;
        case ITEM_InitObject:
	    CVMconsolePrintf(verbose ? ">/this/<" : "@");
	    break;

	default: 
	    CVMconsolePrintf("?"); break;
    }
    for (i = newObjectDepth; i-- > 0; )
	CVMconsolePrintf("<");
    for (i = indirection; i-- > 0; )
	CVMconsolePrintf("]");
}


static void 
print_formatted_fieldname(context_type *context, int index)
{
    CVMExecEnv *ee = context->ee;
    CVMConstantPool *cp = context->constant_pool;
    CVMClassTypeID classname =
	JVM_GetCPFieldClassName(ee, cp, index);
    CVMFieldTypeID fieldname = JVM_GetCPFieldName(cp, index);
    CVMconsolePrintf("  <%!C.%!F>", classname, fieldname);
}

static void 
print_formatted_methodname(context_type *context, int index)
{
    CVMExecEnv *ee = context->ee;
    CVMConstantPool *cp = context->constant_pool;
    CVMClassTypeID classname =
	JVM_GetCPMethodClassName(ee, cp, index);
    CVMMethodTypeID methodname = JVM_GetCPMethodName(cp, index);
    CVMconsolePrintf("  <%!C.%!M>", classname, methodname);
}

#endif /*CVM_TRACE*/

#endif /* CVM_TRUSTED_CLASSLOADERS */
#endif /* CVM_CLASSLOADING */
