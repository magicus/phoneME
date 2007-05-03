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

#include <setjmp.h>

#include <oobj.h>
#include <opcodes.h>
#include <tree.h>
#include <sys_api.h>

#define MAX_ARRAY_DIMENSIONS 255

#define UNKNOWN_STACK_SIZE -1
#define UNKNOWN_REGISTER_COUNT -1
#define UNKNOWN_RET_INSTRUCTION -1

#undef MAX
#undef MIN 
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BITS_PER_INT   (CHAR_BIT * sizeof(int)/sizeof(char))
#define SET_BIT(flags, i)  (flags[(i)/BITS_PER_INT] |= \
                           ((unsigned)1 << ((i) % BITS_PER_INT)))
#define    IS_BIT_SET(flags, i) (flags[(i)/BITS_PER_INT] & \
                           ((unsigned)1 << ((i) % BITS_PER_INT)))

typedef unsigned long fullinfo_type;
typedef unsigned int *bitvector;

#define GET_ITEM_TYPE(thing) ((thing) & 0x1F)
#define GET_INDIRECTION(thing) (((thing) & 0xFFFF) >> 5)
#define GET_EXTRA_INFO(thing) ((unsigned short)((thing) >> 16))
#define WITH_ZERO_INDIRECTION(thing) ((thing) & ~(0xFFE0))
#define WITH_ZERO_EXTRA_INFO(thing) ((thing) & 0xFFFF)

#define MAKE_FULLINFO(type, indirect, extra) \
     ((fullinfo_type)((type) + ((indirect) << 5) + ((extra) << 16)))
#define MAKE_CLASSNAME_INFO(context, classname, addr) \
       MAKE_FULLINFO(ITEM_Object, 0, \
       Str2ID_Local(context, &context->classHash, (classname), (addr), FALSE))
#define MAKE_CLASSNAME_INFO_WITH_COPY(context, classname, addr) \
       MAKE_FULLINFO(ITEM_Object, 0, \
       Str2ID_Local(context, &context->classHash, (classname), (addr), TRUE))
#define MAKE_Object_ARRAY(indirect) \
       (context->object_info + ((indirect) << 5))

#define NULL_FULLINFO MAKE_FULLINFO(ITEM_Object, 0, 0)

/* opc_invokespecial calls to <init> need to be treated special */
#define opc_invokeinit 0x100    

struct context_type {
    /* these fields are per class */
    ClassClass *class;        /* current class */
    struct StrIDhash *classHash;
    fullinfo_type object_info;    /* fullinfo for java/lang/Object */
    fullinfo_type string_info;    /* fullinfo for java/lang/String */
    fullinfo_type throwable_info; /* fullinfo for java/lang/Throwable */

    fullinfo_type currentclass_info; /* fullinfo for context->class */
    fullinfo_type superclass_info;   /* fullinfo for superclass */

    /* these fields are per method */
    struct methodblock *mb;    /* current method */
    unsigned char *code;    /* current code object */
    short *code_data;        /* offset to instruction number */
    struct instruction_data_type *instruction_data; /* info about each */
    struct handler_info_type *handler_info;
    fullinfo_type *superClasses; /* null terminated superclasses */
    int instruction_count;    /* number of instructions */
    fullinfo_type return_type;    /* function return type */
    fullinfo_type swap_table[4]; /* used for passing information */
    int bitmask_size;        /* words needed to hold bitmap of arguments */

    /* Used by inliner */
    bool_t redoJsr;

    /* Used by the space allocator */
    struct CCpool *CCroot, *CCcurrent;
    char *CCfree_ptr;
    int CCfree_size;

    /* Jump here on any error. */
    jmp_buf jump_buffer;
};

struct stack_info_type {
    struct stack_item_type *stack;
    int stack_size;
};

struct register_info_type {
    int register_count;        /* number of registers used */
    fullinfo_type *registers;
    int mask_count;        /* number of masks in the following */
    struct mask_type *masks;
};

struct mask_type {
    int entry;
    int *modifies;
};

typedef unsigned short flag_type;

struct instruction_data_type {
    opcode_type opcode;        /* may turn into "canonical" opcode */
    unsigned changed:1;        /* has it changed */
    unsigned protected:1;    /* must accessor be a subclass of "this" */
    unsigned is_target:1;

    union {
    int i;            /* operand to the opcode */
    int *ip;
    fullinfo_type fi;
    } operand, operand2;
    fullinfo_type p;
    struct stack_info_type stack_info;
    struct register_info_type register_info;
#define FLAG_REACHED            0x01 /* instruction reached */
#define FLAG_NEED_CONSTRUCTOR   0x02 /* must call this.<init> or super.<init> */
#define FLAG_NO_RETURN          0x04 /* must throw out of method */
    flag_type or_flags;        /* true for at least one path to this inst */
#define FLAG_CONSTRUCTED        0x01 /* this.<init> or super.<init> called */
    flag_type and_flags;    /* true for all paths to this instruction */
    unsigned short offset;
    unsigned short length;
};

struct handler_info_type {
    int start, end, handler;
    struct stack_info_type stack_info;
};

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
