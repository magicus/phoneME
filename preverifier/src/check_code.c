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
 * SUBSYSTEM: Verifies codes within a method block.
 * FILE:      check_code.c
 * OVERVIEW:  Verifies that the code within a method block does not
 *            exploit any security holes.
 *            Initial implementation based on the Classic VM Verifier.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <check_code.h>

#include <opcodes.length>
#include <opcodes.in_out>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

#ifdef DEBUG_VERIFIER
    int verify_verbose = 0;
    static struct context_type *GlobalContext;
#endif

void rewriteCode(context_type *vcontext, struct methodblock *mb);
static void verify_method(context_type *context, struct methodblock *mb);
static void verify_field(context_type *context, struct fieldblock *fb);

static void verify_opcode_operands (context_type *, int inumber, int offset);
static void set_protected(context_type *, int inumber, int key, opcode_type);
static bool_t isSuperClass(context_type *, fullinfo_type);

static void initialize_exception_table(context_type *);
static int instruction_length(unsigned char *iptr);
static bool_t isLegalTarget(context_type *, int offset);
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
                     bool_t isException);
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
                      fullinfo_type a, fullinfo_type b,
                      bool_t assignment);
static bool_t isAssignableTo(context_type *,fullinfo_type a, fullinfo_type b);

static ClassClass *object_fullinfo_to_classclass(context_type *, fullinfo_type);

#define NEW(type, count) \
        ((type *)CCalloc(context, (count)*(sizeof(type)), FALSE))
#define ZNEW(type, count) \
        ((type *)CCalloc(context, (count)*(sizeof(type)), TRUE))

static void CCinit(context_type *context);
static void CCreinit(context_type *context);
static void CCdestroy(context_type *context);
static void *CCalloc(context_type *context, int size, bool_t zero);

static char *cp_index_to_fieldname(context_type *context, int cp_index);
static char *cp_index_to_signature(context_type *context, int cp_index);
static fullinfo_type cp_index_to_class_fullinfo(context_type *, int, bool_t);

static char signature_to_fieldtype(context_type *context, 
                   char **signature_p, fullinfo_type *info);

static void CCerror (context_type *, char *format, ...);

#ifdef DEBUG_VERIFIER
static void print_stack (context_type *, stack_info_type *stack_info);
static void print_registers(context_type *, register_info_type *register_info);
static void print_flags(context_type *, flag_type, flag_type);
static void print_formatted_fieldname(context_type *context, int index);
#endif

/*
 * Access local hash tables without locking. This extra level
 * of naming is intended to emphasize the fact that locks are
 * not held and also to do error handling. Although it is not
 * strictly necessary, one should always use the "_Local"  
 * versions of these functions on the verifier's local hash
 * tables.
 */
#define Str2IDFree_Local(localHashRoot) Str2IDFree(localHashRoot)

/*=========================================================================
 * FUNCTION:      Str2ID_Local 
 * OVERVIEW:      Access local hash tables without locking
 *                This extra level of naming is intended to emphasize the
 *                fact that locks are not held and also to do error handling. 
 *                These functions could all be macros like Str2IDFree_Local(),
 *                except that Str2ID() and ID2Str() can run out of memory, 
 *                and the code in this file makes it inconvenient to check
 *                for that. As an expedient, rather than throwing exceptions,
 *                Str2ID_Local() is a function that calls CCerror() if 
 *                necessary. 
 *                Returns the index given the string which corresponds to the
 *                hash table entry.
 * INTERFACE:
 *   parameters:  context_type *: context 
 *                struct StrIDhash **: hash_ptr 
 *                char *: s
 *                void ***: param
 *                int: Copy
 *                
 *   returns:     unsigned short 
 *=======================================================================*/
unsigned short
Str2ID_Local(context_type *context, struct StrIDhash **hash_ptr, char *s,
         void ***param, int Copy) {
    unsigned short ret;
    if ((ret = Str2ID(hash_ptr, s, param, Copy)) == 0) {
    CCerror(context, "Out of memory");
    }

    return ret;
}

/*=========================================================================
 * FUNCTION:      ID2Str_Local 
 * OVERVIEW:      Access local hash tables without locking
 *                This extra level of naming is intended to emphasize the
 *                fact that locks are not held and also to do error handling. 
 *                These functions could all be macros like Str2IDFree_Local(),
 *                except that Str2ID() and ID2Str() can run out of memory, 
 *                and the code in this file makes it inconvenient to check
 *                for that. As an expedient, rather than throwing exceptions,
 *                ID2Str_Local() is a function that calls CCerror() if 
 *                necessary. 
 *                Returns the string given the index which corresponds to the
 *                hash table entry.
 * INTERFACE:
 *   parameters:  context_type *: context 
 *                struct StrIDhash *: h 
 *                unsigned short: ID 
 *                void ***: param
 *                
 *   returns:     char * 
 *=======================================================================*/
char *
ID2Str_Local(context_type *context, struct StrIDhash *h, unsigned short ID, 
         void ***param) {
    char *ret;
    if ((ret = ID2Str(h, ID, param)) == 0) {
    CCerror(context, "Out of memory");
    }

    return ret;
}

/*=========================================================================
 * FUNCTION:      verify_class_codes 
 * OVERVIEW:      Verifies the code for each of the methods in a class.
 *                Invoked by verify_class(). 
 *                Returns true if the class codes are ok. 
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                
 *   returns:     boolean type
 *=======================================================================*/
bool_t verify_class_codes(ClassClass *cb) {
    context_type context_structure;
    context_type *context = &context_structure;
    bool_t result = TRUE;
    void **addr;
    int i;

#ifdef DEBUG_VERIFIER
    GlobalContext = context;
#endif

    /* Initialize the class-wide fields of the context. */
    context->class = cb;

    context->classHash = 0;
    /* Zero method block field of the context, in case anyone calls CCerrror */
    context->mb = 0;
    context->superClasses = NULL;    /* filled in later */

    /* Don't call CCerror or anything that can call it above the setjmp! */
    if (!setjmp(context->jump_buffer)) {
    struct methodblock *mb;
    struct fieldblock *fb;

    CCinit(context);        /* initialize heap; may throw */

    context->object_info = 
        MAKE_CLASSNAME_INFO(context, JAVAPKG "Object", &addr);
    *addr = classJavaLangObject;
    context->string_info =
        MAKE_CLASSNAME_INFO(context, JAVAPKG "String", &addr);
    *addr = classJavaLangString;
    context->throwable_info =
        MAKE_CLASSNAME_INFO(context, JAVAPKG "Throwable", &addr);
    *addr = classJavaLangThrowable;
    context->currentclass_info =
        MAKE_CLASSNAME_INFO_WITH_COPY(context, cbName(cb), &addr);
    *addr = cb;
    if (cbSuperclass(cb) != 0) {
        ClassClass *super = cbSuperclass(cb);
        context->superclass_info =
            MAKE_CLASSNAME_INFO_WITH_COPY(context, cbName(super), &addr);
        *addr = super;
    } else { 
        context->superclass_info = 0;
    }
    
    /* Look at each method */
    for (i = cbFieldsCount(cb), fb = cbFields(cb); --i >= 0; fb++) 
        verify_field(context, fb);

        unhand(context->class)->new_class_entries = 
        (char **)malloc((cbConstantPoolCount(context->class) * 3 + 100) * sizeof(char *));
        unhand(context->class)->n_new_class_entries = 0;

    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) 
        verify_method(context, mb);

    unhand(context->class)->new_class_entries = 
        (char **)realloc(unhand(context->class)->new_class_entries,
                         unhand(context->class)->n_new_class_entries * sizeof(char *));

    result = TRUE;
    } else { 
    result = FALSE;
    }

    /* Cleanup */
    Str2IDFree_Local(&context->classHash);

#ifdef DEBUG_VERIFIER
    GlobalContext = 0;
#endif

    if (context->superClasses != NULL) {
    sysFree(context->superClasses);
    }
    CCdestroy(context);        /* destroy heap */
    return result;
}

/*=========================================================================
 * FUNCTION:      verify_field
 * OVERVIEW:      Verifies the field block within each method of a class
 *                file.
 *                Invoked by verify_class_codes(). 
 *                Returns true if the field block is ok. 
 * INTERFACE:
 *   parameters:  pointer to the context_type structure. 
 *                pointer to the field block
 *                
 *   returns:     boolean type
 *=======================================================================*/
static void
verify_field(context_type *context, struct fieldblock *fb)
{
    int access_bits = fb->access;

    if (  ((access_bits & ACC_PUBLIC) != 0) && 
      ((access_bits & (ACC_PRIVATE | ACC_PROTECTED)) != 0)) {
    if (verbose) { 
        jio_fprintf(stderr, "VERIFIER ERROR %s.%s:\n", 
            cbName(fieldclass(fb)), fb->name);
        jio_fprintf(stderr, "Inconsistent access bits.");
    }
    longjmp(context->jump_buffer, 1);
    } 
}

/*=========================================================================
 * FUNCTION:      get_type_name
 * OVERVIEW:      Retrieves the type name information given the item type. 
 *                Used by get_type_code() which is invoked by verify_method()
 *                to retrieve the type code for the stack map entries. 
 * INTERFACE:
 *   parameters:  pointer to the context_type structure. 
 *                fullinfo_type: type
 *                char *: buf
 *                
 *   returns:     nothing
 *=======================================================================*/
static void 
get_type_name(context_type *context, fullinfo_type type, char *buf)
{
    int i;
    int indirection = GET_INDIRECTION(type);
    for (i = indirection; i-- > 0; )
    *buf++ = '[';
    switch (GET_ITEM_TYPE(type)) {
        case ITEM_Integer:       
        *buf++ = 'I'; break;
    case ITEM_Float:         
        *buf++ = 'F'; break;
    case ITEM_Double:        
        *buf++ = 'D'; break;
    case ITEM_Long:          
        *buf++ = 'J'; break;
    case ITEM_Char:
        *buf++ = 'C'; break;
    case ITEM_Short:
        *buf++ = 'S'; break;
    case ITEM_Byte:
        *buf++ = 'B'; break;
    case ITEM_Boolean:
        *buf++ = 'Z'; break;
    case ITEM_Object: {
            unsigned short extra = GET_EXTRA_INFO(type);
            if (indirection) *buf++ = 'L';
            if (extra == 0) {
                panic("unexpected");
            } else {
                char *name = ID2Str_Local(context, context->classHash,
                                          extra, 0);
                strcpy(buf, name);
                if (indirection) strcat(buf, ";");
            }
            return; 
        }
        default: 
            panic("bad type");
    }
    *buf = '\0';
}

/*=========================================================================
 * FUNCTION:      check_class_constant 
 * OVERVIEW:      Checks and returns the index corresponding to the given
 *                UTF8 constant entry within the constant pool.
 *                Returns -1 if none was found.
 *                Invoked by get_type_code(). 
 * INTERFACE:
 *   parameters:  pointer to the ClassClass structure. 
 *                char *: utf8
 *                
 *   returns:     long type
 *=======================================================================*/
long check_class_constant(ClassClass *cb, char *utf8)
{
    int i;
    cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned char *type_table = 
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    for (i = 0; i < cbConstantPoolCount(cb); i++) {
    if (type_table[i] == (CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED) &&
        strcmp(utf8, cbName(constant_pool[i].clazz)) == 0) {
        return i;
    }
    if (type_table[i] == CONSTANT_Class &&
        strcmp(utf8, constant_pool[constant_pool[i].i].cp) == 0) {
        return i;
    }
    }
    return -1;
}

/*=========================================================================
 * FUNCTION:      get_type_code
 * OVERVIEW:      Retrieves the type code entry given the item type. 
 *                Invoked by verify_method() to retrieve the type code 
 *                for the stack map entries. 
 * INTERFACE:
 *   parameters:  pointer to the context_type structure. 
 *                fullinfo_type: type
 *                
 *   returns:     struct map_entry 
 *=======================================================================*/
struct map_entry get_type_code(context_type *context, fullinfo_type type)
{
    struct map_entry result = {0,0};
    switch (type) {
        case ITEM_Double_2:
        case ITEM_Long_2:
        case ITEM_Bogus:
        result.type = CF_ITEM_Bogus;
        return result;
    case ITEM_Integer:
        result.type = CF_ITEM_Integer;
        return result;
    case ITEM_Float:
        result.type = CF_ITEM_Float;
        return result;
    case ITEM_Double:
        result.type = CF_ITEM_Double;
        return result;
    case ITEM_Long:
        result.type = CF_ITEM_Long;
        return result;
    case ITEM_InitObject:
        result.type = CF_ITEM_InitObject;
        return result;
    default:
    if (GET_ITEM_TYPE(type) == ITEM_NewObject) {
        int inum = GET_EXTRA_INFO(type);
        result.type = CF_ITEM_NewObject;
        result.info = context->instruction_data[inum].offset;
        return result;
    } else if (GET_ITEM_TYPE(type) == ITEM_Object && GET_EXTRA_INFO(type) == 0) {
        result.type = CF_ITEM_Null;
        return result;
        } else if (GET_ITEM_TYPE(type) == ITEM_Object || GET_INDIRECTION(type) > 0) {
        char type_name[1024];
        int i;
            result.type = CF_ITEM_Object;
        get_type_name(context, type, type_name);
        i = check_class_constant(context->class, type_name);
        if (i >= 0) {
        result.info = i;
        return result;
        }
        for (i = 0; i < unhand(context->class)->n_new_class_entries; i++) {
        if (strcmp(type_name, unhand(context->class)->new_class_entries[i]) == 0) {
            result.info = ~i;
            return result;
        }
        }
        { 
        int entries = unhand(context->class)->n_new_class_entries;
        unhand(context->class)->new_class_entries[entries] = strdup(type_name);
        result.info = ~entries;
        unhand(context->class)->n_new_class_entries = entries + 1;
        return result;
        }
    } else {
        panic("bad type code");
        return result; /* not reached */
    }
    }
}

/*=========================================================================
 * FUNCTION:      verify_method
 * OVERVIEW:      Verifies the code for one method within a class file.
 *                Invoked by verify_class_codes(). 
 * INTERFACE:
 *   parameters:  pointer to the context_type structure. 
 *                struct methodblock*: mb
 *                
 *   returns:     nothing 
 *=======================================================================*/
static void
verify_method(context_type *context, struct methodblock *mb)
{
    int access_bits = mb->fb.access;
    unsigned char *code;
    int code_length;
    short *code_data;
    instruction_data_type *idata = 0;
    int instruction_count;
    int i, offset, inumber;
    int exception_count;
    int n_stack_maps, dead_count, jsr_count;

 again:
    code = mb->code;
    code_length = mb->code_length;

    /* CCerror can give method-specific info once this is set */
    context->mb = mb;

    CCreinit(context);        /* initial heap */
    code_data = NEW(short, code_length);

#ifdef DEBUG_VERIFIER
    if (verify_verbose) {
    jio_fprintf(stdout, "Looking at %s.%s%s 0x%x\n", 
           cbName(fieldclass(&mb->fb)), mb->fb.name, mb->fb.signature, 
           (long)mb);
    }
#endif

    if (((access_bits & ACC_PUBLIC) != 0) && 
    ((access_bits & (ACC_PRIVATE | ACC_PROTECTED)) != 0)) {
    CCerror(context, "Inconsistent access bits.");
    } 

    if ((access_bits & (ACC_ABSTRACT | ACC_NATIVE)) != 0) { 
    /* not much to do for abstract or native methods */
        return;
    } 

    if (code_length >= 65535) {
    CCerror(context, "Code of a method longer than 65535 bytes");
    }

    /* Run through the code.  Mark the start of each instruction, and give
     * the instruction a number */
    for (i = 0, offset = 0; offset < code_length; i++) {
    int length = instruction_length(&code[offset]);
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
    instruction_count = i;    /* number of instructions in code */
    
    /* Allocate a structure to hold info about each instruction. */
    idata = NEW(instruction_data_type, instruction_count);

    /* Initialize the heap, and other info in the context structure. */
    context->code = code;
    context->instruction_data = idata;
    context->code_data = code_data;
    context->instruction_count = instruction_count;
    context->handler_info = NEW(struct handler_info_type, 
                mb->exception_table_length);
    context->bitmask_size = (mb->nlocals + (BITS_PER_INT - 1))/BITS_PER_INT;
    
    if (instruction_count == 0) 
    CCerror(context, "Empty code");
    
    for (inumber = 0, offset = 0; offset < code_length; inumber++) {
    int length = instruction_length(&code[offset]);
    instruction_data_type *this_idata = &idata[inumber];
    this_idata->opcode = code[offset];
    this_idata->offset = offset;
    this_idata->length = length;
    this_idata->stack_info.stack = NULL;
    this_idata->stack_info.stack_size  = UNKNOWN_STACK_SIZE;
    this_idata->register_info.register_count = UNKNOWN_REGISTER_COUNT;
    this_idata->changed = FALSE;  /* no need to look at it yet. */
    this_idata->protected = FALSE;  /* no need to look at it yet. */
        /* Added for inlinejsr */
        this_idata->is_target = FALSE;  /* no need to look at it yet. */
    this_idata->and_flags = (flag_type) -1;    /* "bottom" and value */
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
    context->redoJsr = FALSE;
    run_dataflow(context);

    for (inumber = 0; inumber < instruction_count; inumber++) {
        instruction_data_type *this_idata = &idata[inumber];
        if (  (this_idata->or_flags & FLAG_REACHED) && 
              ((this_idata->opcode == opc_jsr) ||
               (this_idata->opcode == opc_jsr_w)) && 
              (this_idata->operand2.i == UNKNOWN_RET_INSTRUCTION)) { 
            this_idata->changed = TRUE;
            context->redoJsr = TRUE;
        }
    }
    if (context->redoJsr) { 
        run_dataflow(context);
    }

    /* verify checked exceptions, if any */
    if ((exception_count = mb->nexceptions) > 0) {
    unsigned short *exceptions = mb->exceptions;
    for (i = 0; i < (int)exception_count; i++) {
        /* Make sure the constant pool item is CONSTANT_Class */
        verify_constant_pool_type(context, (int)exceptions[i],
        1 << CONSTANT_Class);
    }
    }

    
    dead_count = jsr_count = 0;
    for (inumber = 0; inumber < instruction_count; inumber++) {
        instruction_data_type *this_idata = &idata[inumber];
        if ((this_idata->or_flags & FLAG_REACHED) == 0) { 
            dead_count++;
        } else if (this_idata->opcode == opc_jsr ||
                   this_idata->opcode == opc_jsr_w) { 
            jsr_count++;
        }
    }
    if (dead_count > 0 || jsr_count > 0) { 
        rewriteCode(context, mb);
        goto again;
    }

    n_stack_maps = 0;
    for (inumber = 0; inumber < instruction_count; inumber++) {
        instruction_data_type *this_idata = &idata[inumber];
        if (this_idata->is_target) { 
            n_stack_maps++;
        }
    }

    mb->n_stack_maps = n_stack_maps;
    mb->stack_maps = 
        (struct stack_map *)malloc(n_stack_maps * sizeof(struct stack_map));

    n_stack_maps = 0;
    for (inumber = 0; inumber < instruction_count; inumber++) {
    instruction_data_type *this_idata = &idata[inumber];
    if (this_idata->is_target) {
            struct stack_info_type *stack_info = &this_idata->stack_info;
            struct register_info_type *register_info = &this_idata->register_info;
            int register_count = register_info->register_count;
          struct stack_item_type *stack;
        struct map_entry *new_entries;
        int index, index2;

            /* We may be allocating too big a structure if there are longs 
             * or doubles on the stack, but that's okay */
        new_entries = (struct map_entry *)malloc(register_count * sizeof(struct map_entry));

        for (index2 = 0, index = 0; index < register_count; index++) {
                fullinfo_type info = register_info->registers[index];
                if (info == ITEM_Double || info == ITEM_Long) { 
                    if (index + 1 < register_count && 
                           register_info->registers[index + 1] == info+1) {
                        new_entries[index2++] = get_type_code(context, info);
                        index++;
                    } else {
                        new_entries[index2++] = get_type_code(context, ITEM_Bogus);
                    }
                } else { 
                    new_entries[index2++] = get_type_code(context, info);
                }
            }
            mb->stack_maps[n_stack_maps].offset = this_idata->offset;
        mb->stack_maps[n_stack_maps].locals = new_entries;
        mb->stack_maps[n_stack_maps].nlocals = index2;

        mb->stack_maps[n_stack_maps].nstacks = 0;
        for (stack = stack_info->stack; stack; stack = stack->next) {
        mb->stack_maps[n_stack_maps].nstacks++;
        }
        new_entries = (struct map_entry *)malloc(mb->stack_maps[n_stack_maps].nstacks * sizeof(struct map_entry));
        index = 0;
            for (stack = stack_info->stack; stack; stack = stack->next) {
        new_entries[mb->stack_maps[n_stack_maps].nstacks - (++index)] = 
            get_type_code(context, stack->item);
        }
            mb->stack_maps[n_stack_maps].stacks = new_entries;

        unhand(context->class)->has_stack_maps = 1;
        n_stack_maps++;
    }
    }
}

/*=========================================================================
 * FUNCTION:      verify_opcode_operands
 * OVERVIEW:      Verifies the operands of a single instruction given an
 *                instruction number and an offset.
 *                Also, for simplicity, move the operand into the ->operand
 *                field. 
 *                Make sure that branches do not go into the middle
 *                of nowhere.
 *                Invoked by verify_method(). 
 * INTERFACE:
 *   parameters:  context_type *: context
 *                int: inumber
 *                int: offset
 *                
 *   returns:     nothing 
 *=======================================================================*/
static void
verify_opcode_operands(context_type *context, int inumber, int offset)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    short *code_data = context->code_data;
    struct methodblock *mb = context->mb;
    unsigned char *code = context->code;
    opcode_type opcode = this_idata->opcode;
    int var; 
    
    this_idata->operand.i = 0;
    this_idata->operand2.i = 0;

    switch (opcode) {

    case opc_jsr:
    /* instruction of ret statement */
    this_idata->operand2.i = UNKNOWN_RET_INSTRUCTION;
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
    long *lpc = (long *) UCALIGN(code + offset + 1);
    long *lptr;
    int *saved_operand;
    int keys;
    int k, delta;
    if (opcode == opc_tableswitch) {
        keys = ntohl(lpc[2]) -  ntohl(lpc[1]) + 1;
        delta = 1;
    } else { 
        keys = ntohl(lpc[1]); /* number of pairs */
        delta = 2;
        /* Make sure that the tableswitch items are sorted */
        for (k = keys - 1, lptr = &lpc[2]; --k >= 0; lptr += 2) {
        long this_key = ntohl(lptr[0]);    /* NB: ntohl may be unsigned */
        long next_key = ntohl(lptr[2]);
        if (this_key >= next_key) { 
            CCerror(context, "Unsorted lookup switch");
        }
        }
    }
        /* This code has been changed for inlining.   We know have the keys
         * in the same order that they occur in the code, with the default
         * key being the first one.
         */
    saved_operand = NEW(int, keys + 2);
    if (!isLegalTarget(context, offset + ntohl(lpc[0]))) 
        CCerror(context, "Illegal default target in switch");

        saved_operand[0] = keys + 1; /* number of successors */
    saved_operand[1] = code_data[offset + ntohl(lpc[0])]; /* default */

    for (k = 0, lptr = &lpc[3]; k < keys; lptr += delta, k++) {
        int target = offset + ntohl(lptr[0]);
        if (!isLegalTarget(context, target))
        CCerror(context, "Illegal branch in opc_tableswitch");
        saved_operand[k + 2] = code_data[target];
    }
    this_idata->operand.ip = saved_operand;
    break;
    }
    
    case opc_ldc: {   
    /* Make sure the constant pool item is the right type. */
    int key = code[offset + 1];
    int types = (1 << CONSTANT_Integer) | (1 << CONSTANT_Float) |
                        (1 << CONSTANT_String);
    this_idata->operand.i = key;
    verify_constant_pool_type(context, key, types);
    break;
    }
      
    case opc_ldc_w: {   
    /* Make sure the constant pool item is the right type. */
    int key = (code[offset + 1] << 8) + code[offset + 2];
    int types = (1 << CONSTANT_Integer) | (1 << CONSTANT_Float) |
        (1 << CONSTANT_String);
    this_idata->operand.i = key;
    verify_constant_pool_type(context, key, types);
    break;
    }
      
    case opc_ldc2_w: { 
    /* Make sure the constant pool item is the right type. */
    int key = (code[offset + 1] << 8) + code[offset + 2];
    int types = (1 << CONSTANT_Double) | (1 << CONSTANT_Long);
    this_idata->operand.i = key;
    verify_constant_pool_type(context, key, types);
    break;
    }

    case opc_getfield: case opc_putfield:
    case opc_getstatic: case opc_putstatic: {
    /* Make sure the constant pool item is the right type. */
    int key = (code[offset + 1] << 8) + code[offset + 2];
    this_idata->operand.i = key;
    verify_constant_pool_type(context, key, 1 << CONSTANT_Fieldref);    
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
    char *methodname;
    fullinfo_type clazz_info;
    int kind = (opcode == opc_invokeinterface 
                        ? 1 << CONSTANT_InterfaceMethodref
                        : 1 << CONSTANT_Methodref);
    /* Make sure the constant pool item is the right type. */
    verify_constant_pool_type(context, key, kind);
    methodname = cp_index_to_fieldname(context, key);
    clazz_info = cp_index_to_class_fullinfo(context, key, TRUE);
    this_idata->operand.i = key;
    this_idata->operand2.fi = clazz_info;
    if (strcmp(methodname, "<init>") == 0) {
        if (opcode != opc_invokespecial)
        CCerror(context, 
            "Must call initializers using invokespecial");
        this_idata->opcode = opc_invokeinit;
    } else {
        if (methodname[0] == '<') 
        CCerror(context, "Illegal call to internal method");
        if (opcode == opc_invokespecial 
           && clazz_info != context->currentclass_info  
           && clazz_info != context->superclass_info) {
        ClassClass *cb = context->class;
        for (; ; cb = cbSuperclass(cb)) { 
            if (clazz_info == MAKE_CLASSNAME_INFO_WITH_COPY(context, cbName(cb), 0))
            break;
            /* The optimizer make cause this to happen on local code */
            if (cbSuperclass(cb) == 0) {
            /* optimizer make cause this to happen on local code */
            if (cbLoader(cb) != 0) 
                CCerror(context, 
                    "Illegal use of nonvirtual function call");
            break;
            }
        }
        }
    }
    if (opcode == opc_invokeinterface) { 
        char *signature = cp_index_to_signature(context, key);
        unsigned int args1 = Signature2ArgsSize(signature) + 1;
        unsigned int args2 = code[offset + 3];
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
    verify_constant_pool_type(context, key, 1 << CONSTANT_Class);
    target = cp_index_to_class_fullinfo(context, key, FALSE);
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
        if (    (this_idata->operand2.i > (int)GET_INDIRECTION(target))
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
        case T_INT:    
            full_info = MAKE_FULLINFO(ITEM_Integer, 1, 0); break;
        case T_LONG:   
        full_info = MAKE_FULLINFO(ITEM_Long, 1, 0); break;
        case T_FLOAT:  
                full_info = MAKE_FULLINFO(ITEM_Float, 1, 0); break;
        case T_DOUBLE: 
        full_info = MAKE_FULLINFO(ITEM_Double, 1, 0); break;
        case T_BYTE:
        full_info = MAKE_FULLINFO(ITEM_Byte, 1, 0); break;
        case T_BOOLEAN:
        full_info = MAKE_FULLINFO(ITEM_Boolean, 1, 0); break;
        case T_CHAR:   
        full_info = MAKE_FULLINFO(ITEM_Char, 1, 0); break;
        case T_SHORT:  
        full_info = MAKE_FULLINFO(ITEM_Short, 1, 0); break;
        default:
                full_info = 0;  /* make GCC happy */
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
    this_idata->opcode = code[offset + 1];
    var = (code[offset + 2] << 8) + code[offset + 3];
    switch(this_idata->opcode) {
        case opc_lload:  case opc_dload: 
        case opc_lstore: case opc_dstore:
            goto check_local_variable2;
        default:
            goto check_local_variable;
    }

    case opc_iinc:        /* the increment amount doesn't matter */
    case opc_ret: 
    case opc_aload: case opc_iload: case opc_fload:
    case opc_astore: case opc_istore: case opc_fstore:
    var = code[offset + 1];
    check_local_variable:
    /* Make sure that the variable number isn't illegal. */
    this_idata->operand.i = var;
    if (var >= (int)mb->nlocals) 
        CCerror(context, "Illegal local variable number");
    break;
        
    case opc_lload: case opc_dload: case opc_lstore: case opc_dstore: 
    var = code[offset + 1];
    check_local_variable2:
    /* Make sure that the variable number isn't illegal. */
    this_idata->operand.i = var;
    if ((var + 1) >= (int)mb->nlocals)
        CCerror(context, "Illegal local variable number");
    break;
    
    default:
    if (opcode >= opc_breakpoint) 
        CCerror(context, "Quick instructions shouldn't appear yet.");
    break;
    } /* of switch */
}

/*=========================================================================
 * FUNCTION:      set_protected
 * OVERVIEW:      Checks the field access to see if the instruction is 
 *                protected, is private and is in the same class package, 
 *                then the protected bit for the given instruction is set.
 *                Invoked by verify_operands_opcodes() to set protected bit
 *                for instructions of the following opcode types: 
 *                opc_getfield, opc_putfield, and opc_invokevirtual. 
 * INTERFACE:
 *   parameters:  context_type *: context
 *                int: instruction number
 *                int: key
 *                opcode_type: opcode    
 *                
 *   returns:     nothing 
 *=======================================================================*/
static void 
set_protected(context_type *context, int inumber, int key, opcode_type opcode) 
{
    fullinfo_type clazz_info = cp_index_to_class_fullinfo(context, key, TRUE);
    if (isSuperClass(context, clazz_info)) {
    char *name = cp_index_to_fieldname(context, key);
    char *signature = cp_index_to_signature(context, key);
    unsigned ID = NameAndTypeToHash(name, signature);
    ClassClass *calledClass = 
        object_fullinfo_to_classclass(context, clazz_info);
    struct fieldblock *fb;

    if (ID == 0) {    /* NameAndTypeToHash returns 0 if out of memory */
            CCerror(context, "Out of memory");
            return;
        }
    if (opcode != opc_invokevirtual && opcode != opc_invokespecial) { 
        int n = cbFieldsCount(calledClass);
        fb = cbFields(calledClass);
        for (; --n >= 0; fb++) {
        if (fb->ID == ID) {
            goto haveIt;
        }
        }
        return;
    } else { 
        struct methodblock *mb = cbMethods(calledClass);
        int n = cbMethodsCount(calledClass);
        for (; --n >= 0; mb++) {
        if (mb->fb.ID == ID) {
            fb = &mb->fb;
            goto haveIt;
        }
        }
        return;
    }
    haveIt:
    if (IsProtected(fb->access)) {
        if (IsPrivate(fb->access) ||
            !IsSameClassPackage(calledClass, context->class))
        context->instruction_data[inumber].protected = TRUE;
    }
    }
}

/*=========================================================================
 * FUNCTION:      isSuperClass
 * OVERVIEW:      Determines which of the classes are superclasses.
 *                Returns true if the given clazz_info corresponds to a 
 *                a superclass.
 * INTERFACE:
 *   parameters:  context_type* : context 
 *                fullinfo_type: clazz_info
 *                
 *   returns:     boolean type
 *=======================================================================*/
static bool_t 
isSuperClass(context_type *context, fullinfo_type clazz_info) { 
    fullinfo_type *fptr = context->superClasses;
    if (fptr == NULL) { 
    ClassClass *cb;
    fullinfo_type *gptr;
    int i;
    /* Count the number of superclasses.  By counting ourselves, and
     * not counting Object, we get the same number.    */
    for (i = 0, cb = context->class;
         cb != classJavaLangObject; 
         i++, cb = cbSuperclass(cb));
    /* Can't go on context heap since it survives more than one method */
    context->superClasses = fptr 
        = sysMalloc(sizeof(fullinfo_type)*(i + 1));
    if (fptr == 0) {
        CCerror(context, "Out of memory");
    }
    for (gptr = fptr, cb = context->class; cb != classJavaLangObject; ) { 
        void **addr;
        cb = cbSuperclass(cb);
        *gptr++ = MAKE_CLASSNAME_INFO_WITH_COPY(context, cbName(cb), &addr); 
        *addr = cb;
    }
    *gptr = 0;
    } 
    for (; *fptr != 0; fptr++) { 
    if (*fptr == clazz_info)
        return TRUE;
    }
    return FALSE;
}

/*=========================================================================
 * FUNCTION:      initialize_exception_table
 * OVERVIEW:      Initializes the exception table.
 *                Looks through each item on the exception table and ensures
 *                that each of the fields refers to a legal instruction.
 * INTERFACE:
 *   parameters:  pointer to the context_type structure. 
 *                
 *   returns:     nothing 
 *=======================================================================*/
static void
initialize_exception_table(context_type *context)
{
    struct methodblock *mb = context->mb;
    struct CatchFrame *exception_table = mb->exception_table;
    struct handler_info_type *handler_info = context->handler_info;
    short *code_data = context->code_data;
    unsigned long code_length = mb->code_length;

    int i;
    for (i = mb->exception_table_length; 
         --i >= 0; exception_table++, handler_info++) {
    unsigned long start = exception_table->start_pc;
    unsigned long end = exception_table->end_pc;
    unsigned long handler = exception_table->handler_pc;
    unsigned catchType = exception_table->catchType;
    stack_item_type *stack_item = NEW(stack_item_type, 1);
    if (!(  start < end && start >= 0 
          && isLegalTarget(context, start) 
          && (end ==  code_length || isLegalTarget(context, end)))) {
        CCerror(context, "Illegal exception table range");
    }
    if (!((handler > 0) && isLegalTarget(context, handler))) {
        CCerror(context, "Illegal exception table handler");
    }

    handler_info->start = code_data[start];
    /* end may point to one byte beyond the end of bytecodes. */
    handler_info->end = (end == context->mb->code_length) ? 
        context->instruction_count : code_data[end];
    handler_info->handler = code_data[handler];
    handler_info->stack_info.stack = stack_item;
    handler_info->stack_info.stack_size = 1;

    stack_item->next = NULL;
    if (catchType != 0) {
        union cp_item_type *cp = cbConstantPool(context->class);
        char *classname;
        verify_constant_pool_type(context, catchType, 1 << CONSTANT_Class);
        classname = GetClassConstantClassName(cp, catchType);
        stack_item->item = MAKE_CLASSNAME_INFO_WITH_COPY(context, classname, 0);
    } else {
        stack_item->item = context->throwable_info;
    }
    }
}

/*=========================================================================
 * FUNCTION:      instruction_length
 * OVERVIEW:      Given a pointer to an instruction, return its length.
 *                Use the table opcode_length[] which is automatically built. 
 * INTERFACE:
 *   parameters:  pointer to the instruction
 *                
 *   returns:     int type
 *=======================================================================*/
static int instruction_length(unsigned char *iptr)
{
    int instruction = *iptr;
    switch (instruction) {
        case opc_tableswitch: {
        long *lpc = (long *) UCALIGN(iptr + 1);
        int index = ntohl(lpc[2]) - ntohl(lpc[1]);
        if ((index < 0) || (index > 65535)) {
        return -1;    /* illegal */
        } else { 
        return (unsigned char *)(&lpc[index + 4]) - iptr;
            }
    }
        
    case opc_lookupswitch: {
        long *lpc = (long *) UCALIGN(iptr + 1);
        int npairs = ntohl(lpc[1]);
        if (npairs < 0 || npairs >= 8192) 
        return  -1;
        else 
        return (unsigned char *)(&lpc[2 * (npairs + 1)]) - iptr;
    }

    case opc_wide: 
        switch(iptr[1]) {
            case opc_ret:
            case opc_iload: case opc_istore: 
            case opc_fload: case opc_fstore:
            case opc_aload: case opc_astore:
            case opc_lload: case opc_lstore:
            case opc_dload: case opc_dstore: 
            return 4;
        case opc_iinc:
            return 6;
        default: 
            return -1;
        }

    default: {
        /* A length of 0 indicates an error. */
        int length = opcode_length[instruction];
        return (length <= 0) ? -1 : length;
    }
    }
}

/*=========================================================================
 * FUNCTION:      isLegalTarget
 * OVERVIEW:      Given the target of a branch, make sure that it is a legal
 *                target. Returns true if it is a legal target of a branch.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: offset 
 *                
 *   returns:     boolean type
 *=======================================================================*/
static bool_t 
isLegalTarget(context_type *context, int offset)
{
    struct methodblock *mb = context->mb;
    int code_length = mb->code_length;
    short *code_data = context->code_data;
    return (offset >= 0 && offset < code_length && code_data[offset] >= 0);
}

/*=========================================================================
 * FUNCTION:      verify_constant_pool_type 
 * OVERVIEW:      Make sure that an element of the constant pool is really
 *                of the indicated type.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: index
 *                unsigned: mask 
 *   returns:     nothing 
 *=======================================================================*/
static void
verify_constant_pool_type(context_type *context, int index, unsigned mask)
{
    ClassClass *cb = context->class;
    union cp_item_type *constant_pool = cbConstantPool(cb);
    int nconstants = cbConstantPoolCount(cb);
    unsigned char *type_table =
    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    unsigned type;

    if ((index <= 0) || (index >= nconstants))
    CCerror(context, "Illegal constant pool index");

    type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);
    if ((mask & (1 << type)) == 0) 
    CCerror(context, "Illegal type in constant pool");
}
        

/*=========================================================================
 * Functions for Data Flow Analysis 
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      initialize_dataflow 
 * OVERVIEW:      Initialize for data flow analysis.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *   returns:     nothing 
 *=======================================================================*/
static void
initialize_dataflow(context_type *context) 
{
    instruction_data_type *idata = context->instruction_data;
    struct methodblock *mb = context->mb;
    fullinfo_type *reg_ptr;
    fullinfo_type full_info;
    char *p;

    /* Initialize the function entry, since we know everything about it. */
    idata[0].stack_info.stack_size = 0;
    idata[0].stack_info.stack = NULL;
    idata[0].register_info.register_count = mb->args_size;
    idata[0].register_info.registers = NEW(fullinfo_type, mb->args_size);
    idata[0].register_info.mask_count = 0;
    idata[0].register_info.masks = NULL;
    idata[0].and_flags = 0;    /* nothing needed */
    idata[0].or_flags = FLAG_REACHED; /* instruction reached */
    reg_ptr = idata[0].register_info.registers;

    if ((mb->fb.access & ACC_STATIC) == 0) {
    /* A non static method.  If this is an <init> method, the first
     * argument is an uninitialized object.  Otherwise it is an object of
     * the given class type.  java.lang.Object.<init> is special since
     * we don't call its superclass <init> method.
     */
    if (strcmp(mb->fb.name, "<init>") == 0 
            && context->currentclass_info != context->object_info) {
        *reg_ptr++ = MAKE_FULLINFO(ITEM_InitObject, 0, 0);
        idata[0].or_flags |= FLAG_NEED_CONSTRUCTOR;
    } else {
        *reg_ptr++ = context->currentclass_info;
    }
    }
    /* Fill in each of the arguments into the registers. */
    for (p = mb->fb.signature + 1; *p != SIGNATURE_ENDFUNC; ) {
    char fieldchar = signature_to_fieldtype(context, &p, &full_info);
        if (no_floating_point && (fieldchar == 'D' || fieldchar == 'F')) { 
            CCerror(context, "Floating point arguments not allowed");
        }
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
    p++;            /* skip over right parenthesis */
    if (*p == 'V') {
    context->return_type = MAKE_FULLINFO(ITEM_Void, 0, 0);
    } else {
    signature_to_fieldtype(context, &p, &full_info);
    context->return_type = full_info;
    }

    /* Indicate that we need to look at the first instruction. */
    idata[0].changed = TRUE;
}    

/*=========================================================================
 * FUNCTION:      run_dataflow 
 * OVERVIEW:      Execute the data flow analysis as long as there are things
 *                to change.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *   returns:     nothing 
 *=======================================================================*/
static void
run_dataflow(context_type *context) {
    struct methodblock *mb = context->mb;
    int max_stack_size = mb->maxstack;
    instruction_data_type *idata = context->instruction_data;
    int icount = context->instruction_count;
    bool_t work_to_do = TRUE;
    int inumber;

    /* Run through the loop, until there is nothing left to do. */
    while (work_to_do) {
    work_to_do = FALSE;
    for (inumber = 0; inumber < icount; inumber++) {
        instruction_data_type *this_idata = &idata[inumber];
        if (this_idata->changed) {
        register_info_type new_register_info;
        stack_info_type new_stack_info;
        flag_type new_and_flags, new_or_flags;
        
        this_idata->changed = FALSE;
        work_to_do = TRUE;
#ifdef DEBUG_VERIFIER
        if (verify_verbose) {
            int opcode = this_idata->opcode;
            char *opname = (opcode == opc_invokeinit) ? 
                    "invokeinit" : opnames[opcode];
            jio_fprintf(stdout, "Instruction %d: ", inumber);
            print_stack(context, &this_idata->stack_info);
            print_registers(context, &this_idata->register_info);
            print_flags(context, 
                this_idata->and_flags, this_idata->or_flags);
            jio_fprintf(stdout, "  %s(%d)", opname, this_idata->operand.i);
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
#ifdef DEBUG_VERIFIER
        if (verify_verbose) {
            jio_fprintf(stdout, "  ");
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

/*=========================================================================
 * FUNCTION:      check_register_values 
 * OVERVIEW:      Make sure that the registers contain a legitimate value
 *                for the given instruction. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int inumber
 *   returns:     nothing 
 *=======================================================================*/
static void
check_register_values(context_type *context, int inumber)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    int operand = this_idata->operand.i;
    int register_count = this_idata->register_info.register_count;
    fullinfo_type *registers = this_idata->register_info.registers;
    bool_t double_word = FALSE;    /* default value */
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
        type = ITEM_Long; double_word = TRUE; break;
        case opc_dload:
        type = ITEM_Double; double_word = TRUE; break;
    }
    if (!double_word) {
    fullinfo_type reg = registers[operand];
    /* Make sure we don't have an illegal register or one with wrong type */
    if (operand >= register_count) {
        CCerror(context, 
            "Accessing value from uninitialized register %d", operand);
    } else if (WITH_ZERO_EXTRA_INFO(reg) == MAKE_FULLINFO(type, 0, 0)) {
        /* the register is obviously of the given type */
        return;
    } else if (GET_INDIRECTION(reg) > 0 && type == ITEM_Object) {
        /* address type stuff be used on all arrays */
        return;
    } else if (GET_ITEM_TYPE(reg) == ITEM_ReturnAddress) { 
        CCerror(context, "Cannot load return address from register %d", 
                      operand);
        /* alternatively 
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

/*=========================================================================
 * FUNCTION:      check_flags 
 * OVERVIEW:      Make sure that the flags contain legitimate values for this
 *                instruction. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *   returns:     nothing 
 *=======================================================================*/
static void
check_flags(context_type *context, int inumber)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    if (opcode == opc_return) {
        /* We need a constructor, but we aren't guaranteed it's called */
        if ((this_idata->or_flags & FLAG_NEED_CONSTRUCTOR) && 
           !(this_idata->and_flags & FLAG_CONSTRUCTED))
        CCerror(context, "Constructor must call super() or this()");
        /* fall through */
    } else if ((opcode == opc_ireturn) || (opcode == opc_lreturn) || 
               (opcode == opc_freturn) || (opcode == opc_dreturn) ||
               (opcode == opc_areturn)) { 
        if (this_idata->or_flags & FLAG_NO_RETURN)
        /* This method cannot exit normally */
        CCerror(context, "Cannot return normally");
    }
}

/*=========================================================================
 * FUNCTION:      pop_stack 
 * OVERVIEW:      Make sure that the top of the stack contains reasonable 
 *                values for the given instruction. The post-pop values of
 *                the stack and its size are returned in *new_stack_info. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *                stack_info_type: *new_stack_info
 *   returns:     nothing 
 *=======================================================================*/
static void 
pop_stack(context_type *context, int inumber, stack_info_type *new_stack_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[inumber];
    opcode_type opcode = this_idata->opcode;
    stack_item_type *stack = this_idata->stack_info.stack;
    int stack_size = this_idata->stack_info.stack_size;
    char *stack_operands, *p;
    char buffer[257];        /* for holding manufactured argument lists */
    fullinfo_type stack_extra_info_buffer[256]; /* save info popped off stack */
    fullinfo_type *stack_extra_info = &stack_extra_info_buffer[256];
    fullinfo_type full_info, put_full_info;
    
    switch(opcode) {
        default:
        /* For most instructions, we just use a built-in table */
        stack_operands = opcode_in_out[opcode][0];
        break;

        case opc_putstatic: case opc_putfield: {
        /* The top thing on the stack depends on the signature of
         * the object.                         */
        int operand = this_idata->operand.i;
        char *signature = cp_index_to_signature(context, operand);
        char *ip = buffer;
#ifdef DEBUG_VERIFIER
        if (verify_verbose) {
        print_formatted_fieldname(context, operand);
        }
#endif
        if (opcode == opc_putfield)
        *ip++ = 'A';    /* object for putfield */
        *ip++ = signature_to_fieldtype(context, &signature, &put_full_info);
        *ip = '\0';
        stack_operands = buffer;
        break;
    }

    case opc_invokevirtual: case opc_invokespecial:        
        case opc_invokeinit:    /* invokespecial call to <init> */
    case opc_invokestatic: case opc_invokeinterface: {
        /* The top stuff on the stack depends on the method signature */
        int operand = this_idata->operand.i;
        char *signature = cp_index_to_signature(context, operand);
        char *ip = buffer;
        char *p;
#ifdef DEBUG_VERIFIER
        if (verify_verbose) {
        print_formatted_fieldname(context, operand);
        }
#endif
        if (opcode != opc_invokestatic) 
        /* First, push the object */
        *ip++ = (opcode == opc_invokeinit ? '@' : 'A');
        for (p = signature + 1; *p != SIGNATURE_ENDFUNC; ) {
        *ip++ = signature_to_fieldtype(context, &p, &full_info);
        if (ip >= buffer + sizeof(buffer) - 1)
            CCerror(context, "Signature %s has too many arguments", 
                signature);
        }
        *ip = 0;
        stack_operands = buffer;
        break;
    }

    case opc_multianewarray: {
        /* Count can't be larger than 255. So can't overflow buffer */
        int count = this_idata->operand2.i;    /* number of ints on stack */
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
    int type = *--p;
    fullinfo_type top_type = stack ? stack->item : 0;
    int size = (type == 'D' || type == 'L') ? 2 : 1;
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
        
        case 'A':        /* object or array */
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
            && (    (opcode == opc_astore)
                             || (opcode == opc_aload)
                             || (opcode == opc_ifnull)
                             || (opcode == opc_ifnonnull)
                             || (opcode == opc_if_acmpeq) 
                             || (opcode == opc_if_acmpne)) 
                           ) {
            break;
                    }

                    /* The 2nd edition VM of the specification allows field
                     * initializations before the superclass initializer,
                     * if the field is defined within the current class.
                     */
                     if (   (GET_ITEM_TYPE(top_type) == ITEM_InitObject)
                         && (opcode == opc_putfield)) {

                        int num_fields;
                        int key = this_idata->operand.i;
                        fullinfo_type clazz_info = cp_index_to_class_fullinfo(
                                                            context, key, TRUE);
                        char *name = cp_index_to_fieldname(context, key);
                        char *signature = cp_index_to_signature(context, key);
                        unsigned ID = NameAndTypeToHash(name, signature);

                        ClassClass *calledClass =
                             object_fullinfo_to_classclass(context, clazz_info);
                        struct fieldblock *fb;
                        if (ID == 0) { 
                            /* NameAndTypeToHash returns 0
                             * if out of memory
                             */
                            CCerror(context, "Out of memory");
                            break;
                        }
                        num_fields = cbFieldsCount(calledClass);
                        fb = cbFields(calledClass);
                        for (; --num_fields >= 0; fb++) {
                            if (fb->ID == ID) {
                                break;
                            }
                        }

                        if (num_fields != -1) {
                            top_type = context->currentclass_info;
                            *stack_extra_info = top_type;
                            break;
                        }
                    }
            CCerror(context, "Expecting to find object/array on stack");
        }
        break;

        case '@': {        /* unitialized object, for call to <init> */
        int item_type = GET_ITEM_TYPE(top_type);
        if (item_type != ITEM_NewObject && item_type != ITEM_InitObject)
            CCerror(context, 
                "Expecting to find unitialized object on stack");
        break;
        }

        case 'O':        /* object, not array */
        if (WITH_ZERO_EXTRA_INFO(top_type) != 
               MAKE_FULLINFO(ITEM_Object, 0, 0))
            CCerror(context, "Expecting to find object on stack");
        break;

        case 'a':        /* integer, object, or array */
        if (      (top_type != MAKE_FULLINFO(ITEM_Integer, 0, 0)) 
               && (GET_ITEM_TYPE(top_type) != ITEM_Object)
               && (GET_INDIRECTION(top_type) == 0))
            CCerror(context, 
                "Expecting to find object, array, or int on stack");
        break;

        case 'D':        /* double */
        if (top_type != MAKE_FULLINFO(ITEM_Double, 0, 0))
            CCerror(context, "Expecting to find double on stack");
        break;

        case 'L':        /* long */
        if (top_type != MAKE_FULLINFO(ITEM_Long, 0, 0))
            CCerror(context, "Expecting to find long on stack");
        break;

        case ']':        /* array of some type */
        if (top_type == NULL_FULLINFO) { 
            /* do nothing */
        } else switch(p[-1]) {
            case 'I':    /* array of integers */
                if (top_type != MAKE_FULLINFO(ITEM_Integer, 1, 0) && 
                top_type != NULL_FULLINFO)
                CCerror(context, 
                    "Expecting to find array of ints on stack");
            break;

            case 'L':    /* array of longs */
                if (top_type != MAKE_FULLINFO(ITEM_Long, 1, 0))
                CCerror(context, 
                   "Expecting to find array of longs on stack");
            break;

            case 'F':    /* array of floats */
                if (top_type != MAKE_FULLINFO(ITEM_Float, 1, 0))
                CCerror(context, 
                 "Expecting to find array of floats on stack");
            break;

            case 'D':    /* array of doubles */
                if (top_type != MAKE_FULLINFO(ITEM_Double, 1, 0))
                CCerror(context, 
                "Expecting to find array of doubles on stack");
            break;

            case 'A': {    /* array of addresses (arrays or objects) */
            int indirection = GET_INDIRECTION(top_type);
            if ((indirection == 0) || 
                ((indirection == 1) && 
                    (GET_ITEM_TYPE(top_type) != ITEM_Object)))
                CCerror(context, 
                "Expecting to find array of objects or arrays "
                    "on stack");
            break;
            }

            case 'b':    /* array of bytes or boolean */
                if (top_type != MAKE_FULLINFO(ITEM_Byte, 1, 0) &&
                top_type != MAKE_FULLINFO(ITEM_Boolean, 1, 0))
                CCerror(context, 
                  "Expecting to find array of bytes or booleans on stack");
            break;

            case 'B':    /* array of bytes */
                if (top_type != MAKE_FULLINFO(ITEM_Byte, 1, 0))
                CCerror(context, 
                  "Expecting to find array of bytes on stack");
            break;

            case 'Z':    /* array of boolean */
                if (top_type != MAKE_FULLINFO(ITEM_Boolean, 1, 0))
                CCerror(context, 
                  "Expecting to find array of bytes on stack");
            break;

            case 'C':    /* array of characters */
                if (top_type != MAKE_FULLINFO(ITEM_Char, 1, 0))
                CCerror(context, 
                  "Expecting to find array of chars on stack");
            break;

            case 'S':    /* array of shorts */
                if (top_type != MAKE_FULLINFO(ITEM_Short, 1, 0))
                CCerror(context, 
                 "Expecting to find array of shorts on stack");
            break;

            case '?':    /* any type of array is okay */
                if (GET_INDIRECTION(top_type) == 0) 
                CCerror(context, 
                    "Expecting to find array on stack");
            break;

            default:
            CCerror(context, "Internal error #1");
            sysAssert(FALSE);
            break;
        }
        p -= 2;        /* skip over [ <char> */
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
            p--;    /* ignore */
        }
        break;
        case '+':        /* these should have been caught. */
        default:
        CCerror(context, "Internal error #2");
        sysAssert(FALSE);
    }
    stack_size -= size;
    }
    
    /* For many of the opcodes that had an "A" in their field, we really 
     * need to go back and do a little bit more accurate testing.  We can, of
     * course, assume that the minimal type checking has already been done. 
     */
    switch (opcode) {
    default: break;
    case opc_aastore: {    /* array index object  */
        fullinfo_type array_type = stack_extra_info[0];
        fullinfo_type object_type = stack_extra_info[2];
        fullinfo_type target_type = decrement_indirection(array_type);
            if (array_type == NULL_FULLINFO) {
               break;
            }
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
        if (!isAssignableTo(context, 
                    stack_object, 
                    cp_index_to_class_fullinfo(context, operand,
                                   TRUE))) {
            CCerror(context, 
                "Incompatible type for getting or setting field");
        }
                /* FY:  Why is this commented out?? */
        /*
        if (this_idata->protected && 
            !isAssignableTo(context, stack_object, 
                    context->currentclass_info)) { 
            CCerror(context, "Bad access to protected data");
        }*/
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

    case opc_aaload: {    /* array index */
        /* We need to pass the information to the stack updater */
        fullinfo_type array_type = stack_extra_info[0];
        context->swap_table[0] = decrement_indirection(array_type);
        break;
    }
        
        case opc_invokevirtual: case opc_invokespecial: 
        case opc_invokeinit:
    case opc_invokeinterface: case opc_invokestatic: {
        int operand = this_idata->operand.i;
        char *signature = cp_index_to_signature(context, operand);
        int item;
        char *p;
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
                /* The specification of the structural constraints for
                 * invokespecial needs to be more stringent.
                 *
                 * If invokespecial is used to invoke an instance method 
                 * that is not an instance initialization method, then the
                 * type of the class instance, which is the target of method 
                 * invocation, must be assignment compatible with the 
                 * current class. 
                 */
                if (opcode == opc_invokespecial
                    && !isAssignableTo(context, object_type,
                                       context->currentclass_info)) {
                    /* Make sure object argument is assignment compatible 
                     * to current class 
                     */
                    CCerror(context,
                            "Incompatible object argument for invokespecial");
                }
        if (this_idata->protected 
            && !isAssignableTo(context, object_type, 
                       context->currentclass_info)) { 
            /* This is ugly. Special dispensation.  Arrays pretend to
               implement public Object clone() even though they don't */
            if ((target_type == context->object_info) && 
            (GET_INDIRECTION(object_type) > 0) &&
            (strcmp(cp_index_to_fieldname(context, this_idata->operand.i),
                "clone") == 0)) { 
            } else { 
            CCerror(context, "Bad access to protected data");
            }
        }
        item = 1;
        }
        for (p = signature + 1; *p != SIGNATURE_ENDFUNC; item++)
        if (signature_to_fieldtype(context, &p, &full_info) == 'A') {
            if (!isAssignableTo(context, 
                    stack_extra_info[item], full_info)) {
            CCerror(context, "Incompatible argument to function");
            }
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
         * as an initialized one. 
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

/*=========================================================================
 * FUNCTION:      update_registers 
 * OVERVIEW:      Perform the operation on the registers, and return the 
 *                updated results in new_register_count_p and new_registers.
 *                Note that the instruction has already been determined 
 *                to be legal. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *                register_info_type: pointer to new register info
 *
 *   returns:     nothing 
 *=======================================================================*/
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

    enum { NONE, SINGLE, DOUBLE } access = NONE;
    int i;
    
    /* Remember, we've already verified the type at the top of the stack. */
    switch (opcode) {
    default: break;
        case opc_istore: case opc_fstore: case opc_astore:
        access = SINGLE;
        goto continue_store;

        case opc_lstore: case opc_dstore:
        access = DOUBLE;
        goto continue_store;

    continue_store: {
        /* We have a modification to the registers.  Copy them if needed. */
        fullinfo_type stack_top_type = stack->item;
        int max_operand = operand + ((access == DOUBLE) ? 1 : 0);
        
        if (     max_operand < register_count 
          && registers[operand] == stack_top_type
          && ((access == SINGLE) || 
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
        if (access == DOUBLE)
        new_registers[operand + 1] = stack_top_type + 1;
        break;
    }
     
        case opc_iload: case opc_fload: case opc_aload:
        case opc_iinc: case opc_ret:
        access = SINGLE;
        break;

        case opc_lload: case opc_dload:    
        access = DOUBLE;
        break;

        case opc_jsr: case opc_jsr_w:
        for (i = 0; i < new_mask_count; i++) 
        if (new_masks[i].entry == operand) 
            CCerror(context, "Recursive call to jsr entry");
            if (context->redoJsr 
                   && this_idata->operand2.i == UNKNOWN_RET_INSTRUCTION) { 
                /* Do nothing */
            } else { 
                new_masks = add_to_masks(context, masks, mask_count, operand);
                new_mask_count++; 
            }
        break;
        
        case opc_invokeinit: 
        case opc_new: {
        /* For invokeinit, an uninitialized object has been initialized.
         * For new, all previous occurrences of an uninitialized object
         * from the same instruction must be made bogus.  
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
        bool_t copied_mask = FALSE;
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
                    copied_mask = TRUE;
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

    if ((access != NONE) && (new_mask_count > 0)) {
    int i, j;
    for (i = 0; i < new_mask_count; i++) {
        int *mask = new_masks[i].modifies;
        if ((!IS_BIT_SET(mask, operand)) || 
          ((access == DOUBLE) && !IS_BIT_SET(mask, operand + 1))) {
        new_masks = copy_masks(context, new_masks, mask_count);
        for (j = i; j < new_mask_count; j++) {
            SET_BIT(new_masks[j].modifies, operand);
            if (access == DOUBLE) 
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

/*=========================================================================
 * FUNCTION:      update_flags
 * OVERVIEW:      Update the flags now that we have already determined
 *                the instruction to be legal and have already updated the
 *                registers. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *                flag_type: pointer to new_and_flags
 *                flag_type: pointer to new_or_flags
 *
 *   returns:     nothing 
 *=======================================================================*/
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

/*=========================================================================
 * FUNCTION:      push_stack
 * OVERVIEW:      Perform the operation on the stack.
 *                Note that the instruction has already been determined 
 *                to be legal. 
 *                new_stack_size_p and new_stack_p point to the results after
 *                the pops have already been done. Do the pushes, and then
 *                put the results back there.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *                stack_info_type: pointer to new stack info
 *
 *   returns:     nothing 
 *=======================================================================*/
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
    char buffer[5], *p;        /* actually [2] is big enough */

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
        union cp_item_type *cp = cbConstantPool(context->class);
        unsigned char *type_table = cp[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
        switch (CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, operand)) {
        case CONSTANT_Integer:   
            stack_results = "I"; break;
        case CONSTANT_Float:     
            stack_results = "F"; break;
        case CONSTANT_Double:    
            stack_results = "D"; break;
        case CONSTANT_Long:      
            stack_results = "L"; break;
        case CONSTANT_String: 
            stack_results = "A"; 
            full_info = context->string_info;
            break;
        default:
            CCerror(context, "Internal error #3");
            sysAssert(FALSE);
        }
        break;
    }

        case opc_getstatic: case opc_getfield: {
        /* Look to signature to determine correct result. */
        int operand = this_idata->operand.i;
        char *signature = cp_index_to_signature(context, operand);
#ifdef DEBUG_VERIFIER
        if (verify_verbose) {
        print_formatted_fieldname(context, operand);
        }
#endif
        buffer[0] = signature_to_fieldtype(context, &signature, &full_info);
        buffer[1] = '\0';
        stack_results = buffer;
        break;
    }

    case opc_invokevirtual: case opc_invokespecial:
        case opc_invokeinit:
    case opc_invokestatic: case opc_invokeinterface: {
        /* Look to signature to determine correct result. */
        int operand = this_idata->operand.i;
        char *signature = cp_index_to_signature(context, operand);
        char *result_signature = strchr(signature, SIGNATURE_ENDFUNC) + 1;
        if (result_signature[0] == SIGNATURE_VOID) {
        stack_results = "";
        } else {
        buffer[0] = signature_to_fieldtype(context, &result_signature, 
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
        /* Conventionally, this result type is stored here */
        full_info = this_idata->operand.fi;
            if (no_floating_point && (opcode == opc_newarray) 
                && (full_info == MAKE_FULLINFO(ITEM_Float, 1, 0) || 
                    full_info == MAKE_FULLINFO(ITEM_Double, 1, 0))) {
                CCerror(context, "Cannot create floating point array");
            }
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
                if (no_floating_point) {
                    CCerror(context, "Floating point result not allowed");
                }
        stack->item = MAKE_FULLINFO(ITEM_Float, 0, 0); 
                break;
        case 'D': 
                if (no_floating_point) {
                    CCerror(context, "Floating point result not allowed");
                }
        stack->item = MAKE_FULLINFO(ITEM_Double, 0, 0); 
        stack_size++; 
                break;
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
        sysAssert(full_info != 0);
        stack->item = full_info;
        break;
        default:
        CCerror(context, "Internal error #4");
        sysAssert(FALSE);

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

/*=========================================================================
 * FUNCTION:      merge_into_successors
 * OVERVIEW:      Executed an instruction, and have determined the new
 *                registers and stack values. Look at all of the possibly
 *                subsequent instructions, and merge this stack value into
 *                theirs.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *                stack_info_type: pointer to stack_info
 *                flag_type: and_flags
 *                flag_type: or_flags 
 *
 *   returns:     nothing 
 *=======================================================================*/
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
    int handler_info_length = context->mb->exception_table_length;

    int buffer[2];        /* default value for successors */
    int *successors = buffer;    /* table of successors */
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
        idata[operand].is_target = TRUE; /* inlinejsr */
    break;

    case opc_jsr: case opc_jsr_w: 
    if (this_idata->operand2.i != UNKNOWN_RET_INSTRUCTION) 
        idata[this_idata->operand2.i].changed = TRUE;
    /* FALLTHROUGH */
    case opc_goto: case opc_goto_w:
    successors_count = 1;
    buffer[0] = operand;
        idata[operand].is_target = TRUE; /* inlinejsr */
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
            (idata[i].operand.i == called_instruction)) {
            *ptr++ = i + 1;
                    idata[i + 1].is_target = TRUE; /* inlinejsr */
                }
        }
    }
    successors = this_idata->operand2.ip; /* use this instead */
    successors_count = *successors++;
    break;
        
    }

    case opc_tableswitch:
    case opc_lookupswitch: { 
        int i;
        successors = this_idata->operand.ip; /* use this instead */
        successors_count = *successors++;
        for (i = 0; i < successors_count; i++) { 
            idata[successors[i]].is_target = TRUE; /* inlinejsr */
        }
        break;
    }

    }

#ifdef DEBUG_VERIFIER
    if (verify_verbose) { 
    jio_fprintf(stdout, " [");
    for (i = handler_info_length; --i >= 0; handler_info++)
        if (handler_info->start <= inumber && handler_info->end > inumber)
        jio_fprintf(stdout, "%d* ", handler_info->handler);
    for (i = 0; i < successors_count; i++)
        jio_fprintf(stdout, "%d ", successors[i]);
    jio_fprintf(stdout,   "]\n");
    }
#endif

    handler_info = context->handler_info;
    for (i = handler_info_length; --i >= 0; handler_info++) {
    if (handler_info->start <= inumber && handler_info->end > inumber) {
        int handler = handler_info->handler;
            idata[handler].is_target = TRUE; /* inlinejsr */
        if (opcode != opc_invokeinit) {
        merge_into_one_successor(context, inumber, handler, 
                     &this_idata->register_info, /* old */
                     &handler_info->stack_info, 
                     (flag_type) (and_flags
                              & this_idata->and_flags),
                     (flag_type) (or_flags
                              | this_idata->or_flags),
                     TRUE);
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
                     TRUE);
        merge_into_one_successor(context, inumber, handler, 
                     register_info, 
                     &handler_info->stack_info, 
                     and_flags, temp_or_flags, TRUE);
        }
    }
    }
    for (i = 0; i < successors_count; i++) {
    int target = successors[i];
    if (target >= context->instruction_count) 
        CCerror(context, "Falling off the end of the code");
    merge_into_one_successor(context, inumber, target, 
                 register_info, stack_info, and_flags, or_flags,
                 FALSE);
    }
}

/*=========================================================================
 * FUNCTION:      merge_into_one_successor
 * OVERVIEW:      Executed an instruction, and have determined a new
 *                set of registers and stack values for a given instruction. 
 *                Merge this new set into the values that are already there.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: instruction number
 *                stack_info_type: pointer to stack_info
 *                flag_type: new_and_flags
 *                flag_type: new_or_flags 
 *                boolean type: isException
 *
 *   returns:     nothing 
 *=======================================================================*/
static void
merge_into_one_successor(context_type *context, 
             int from_inumber, int to_inumber, 
             register_info_type *new_register_info,
             stack_info_type *new_stack_info,
             flag_type new_and_flags, flag_type new_or_flags,
             bool_t isException)
{
    instruction_data_type *idata = context->instruction_data;
    register_info_type register_info_buf;
    stack_info_type stack_info_buf;
    instruction_data_type *this_idata = &idata[to_inumber];
#ifdef DEBUG_VERIFIER
    register_info_type old_reg_info;
    stack_info_type old_stack_info;
    flag_type old_and_flags, old_or_flags;
#endif

#ifdef DEBUG_VERIFIER
    if (verify_verbose) {
    old_reg_info = this_idata->register_info;
    old_stack_info = this_idata->stack_info;
    old_and_flags = this_idata->and_flags;
    old_or_flags = this_idata->or_flags;
    }
#endif

    /* All uninitialized objects are set to "bogus" when jsr and
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
     * that needs to get merged into the new instruction is a joining
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

        merge_stack(context, to_inumber - 1, to_inumber, new_stack_info);
        merge_registers(context, to_inumber - 1, to_inumber, 
                &new_new_register_info);
            /* ADDED FOR JSR_INFO.  Is this correct?? */
            merge_flags(context, from_inumber, to_inumber, 
                        new_and_flags, new_or_flags);
    }
    } else {
    merge_stack(context, from_inumber, to_inumber, new_stack_info);
    merge_registers(context, from_inumber, to_inumber, new_register_info);
    merge_flags(context, from_inumber, to_inumber, 
            new_and_flags, new_or_flags);
    }

#ifdef DEBUG_VERIFIER
    if (verify_verbose && idata[to_inumber].changed) {
    register_info_type *register_info = &this_idata->register_info;
    stack_info_type *stack_info = &this_idata->stack_info;
    if (memcmp(&old_reg_info, register_info, sizeof(old_reg_info)) ||
        memcmp(&old_stack_info, stack_info, sizeof(old_stack_info)) || 
        (old_and_flags != this_idata->and_flags) || 
        (old_or_flags != this_idata->or_flags)) {
        jio_fprintf(stdout, "   %2d:", to_inumber);
        print_stack(context, &old_stack_info);
        print_registers(context, &old_reg_info);
        print_flags(context, old_and_flags, old_or_flags);
        jio_fprintf(stdout, " => ");
        print_stack(context, &this_idata->stack_info);
        print_registers(context, &this_idata->register_info);
        print_flags(context, this_idata->and_flags, this_idata->or_flags);
        jio_fprintf(stdout, "\n");
    }
    }
#endif

}

/*=========================================================================
 * FUNCTION:      merge_stack
 * OVERVIEW:      Used by merge_into_one_successor() for merging stack values 
 *                from a given instruction to another specified instruction.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: from instruction number
 *                int: to instruction number
 *                stack_info_type: pointer to new_stack_info
 *
 *   returns:     nothing 
 *=======================================================================*/
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
    this_idata->changed = TRUE;
    } else if (new_stack_size != stack_size) {
    CCerror(context, "Inconsistent stack height %d != %d",
        new_stack_size, stack_size);
    } else { 
    stack_item_type *stack = this_idata->stack_info.stack;
    stack_item_type *old, *new;
    bool_t change = FALSE;
    for (old = stack, new = new_stack; old != NULL; 
               old = old->next, new = new->next) {
        if (!isAssignableTo(context, new->item, old->item)) {
        change = TRUE;
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
                             FALSE);
                if (GET_ITEM_TYPE(old->item) == ITEM_Bogus) {
                    CCerror(context, "Mismatched stack types");
                }
        }
            if (old != NULL || new != NULL) {
                CCerror(context, "Mismatched stack types");
            }
        this_idata->stack_info.stack = stack;
        this_idata->changed = TRUE;
    }
    }
}

/*=========================================================================
 * FUNCTION:      merge_registers
 * OVERVIEW:      Used by merge_into_one_successor() for merging registers
 *                from a given instruction to another specified instruction.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: from instruction number
 *                int: to instruction number
 *                stack_info_type: pointer to new_stack_info
 *
 *   returns:     nothing 
 *=======================================================================*/
static void
merge_registers(context_type *context, int from_inumber, int to_inumber,
         register_info_type *new_register_info)
{
    instruction_data_type *idata = context->instruction_data;
    instruction_data_type *this_idata = &idata[to_inumber];
    register_info_type      *this_reginfo = &this_idata->register_info;

    int            new_register_count = new_register_info->register_count;
    fullinfo_type *new_registers = new_register_info->registers;
    int            new_mask_count = new_register_info->mask_count;
    mask_type     *new_masks = new_register_info->masks;
    

    if (this_reginfo->register_count == UNKNOWN_REGISTER_COUNT) {
    this_reginfo->register_count = new_register_count;
    this_reginfo->registers = new_registers;
    this_reginfo->mask_count = new_mask_count;
    this_reginfo->masks = new_masks;
    this_idata->changed = TRUE;
    } else {
    /* See if we've got new information on the register set. */
    int register_count = this_reginfo->register_count;
    fullinfo_type *registers = this_reginfo->registers;
    int mask_count = this_reginfo->mask_count;
    mask_type *masks = this_reginfo->masks;
    
    bool_t copy = FALSE;
    int i, j;
    if (register_count > new_register_count) {
        /* Any register larger than new_register_count is now bogus */
        this_reginfo->register_count = new_register_count;
        register_count = new_register_count;
        this_idata->changed = TRUE;
    }
    for (i = 0; i < register_count; i++) {
        fullinfo_type prev_value = registers[i];
        if ((i < new_register_count) 
          ? (!isAssignableTo(context, new_registers[i], prev_value))
          : (prev_value != MAKE_FULLINFO(ITEM_Bogus, 0, 0))) {
        copy = TRUE; 
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
                              registers[j], FALSE);
        }
        /* Some of the end items might now be bogus. This step isn't 
         * necessary, but it may save work later. */
        while (   register_count > 0
           && GET_ITEM_TYPE(new_set[register_count-1]) == ITEM_Bogus) 
        register_count--;
        this_reginfo->register_count = register_count;
        this_reginfo->registers = new_set;
        this_idata->changed = TRUE;
    }
    if (mask_count > 0) { 
        /* If the target instruction already has a sequence of masks, then
         * we need to merge new_masks into it.  We want the entries on
         * the mask to be the longest common substring of the two.
         *   (e.g.   a->b->d merged with a->c->d should give a->d)
         * The bits set in the mask should be the or of the corresponding
         * entries in each of the original masks.
         */
        int i, j, k;
        int matches = 0;
        int last_match = -1;
        bool_t copy_needed = FALSE;
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
                copy_needed = TRUE;
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
        this_idata->changed = TRUE;
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

/*=========================================================================
 * FUNCTION:      merge_flags
 * OVERVIEW:      Used by merge_into_one_successor() for merging flags
 *                from a given instruction to a specified instruction.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: from instruction number
 *                int: to instruction number
 *                flag_type: new_and_flags
 *                flag_type: new_or_flags 
 *
 *   returns:     nothing 
 *=======================================================================*/
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
    this_idata->changed = TRUE;
    }
}

/*=========================================================================
 * FUNCTION:      copy_stack
 * OVERVIEW:      Make a copy of a stack.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                stack_item_type: pointer to stack 
 *
 *   returns:     pointer to stack_item_type 
 *=======================================================================*/
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

/*=========================================================================
 * FUNCTION:      copy_masks
 * OVERVIEW:      Make a copy of the masks.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                mask_type: pointer to masks
 *                int: mask count
 *
 *   returns:     pointer to mask_type 
 *=======================================================================*/
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

/*=========================================================================
 * FUNCTION:      add_to_masks
 * OVERVIEW:      Used by Update_registers for adding entries to masks for
 *                JSR instructions.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                mask_type: pointer to masks
 *                int: mask count
 *                int: d
 *
 *   returns:     pointer to mask_type 
 *=======================================================================*/
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
    

/*=========================================================================
 * Storage Management Operations 
 *=======================================================================*/

/* We create our own storage manager, since we malloc lots of little items, 
 * and we do not want to keep track of them when they become free.
 * It would have been nice if we had heaps, which could all be freed when
 * done.
 */

#define CCSegSize 2000

struct CCpool {            /* a segment of allocated memory in the pool */
    struct CCpool *next;
    int segSize;        /* almost always CCSegSize */
    char space[CCSegSize];
};

/*=========================================================================
 * FUNCTION:      CCinit 
 * OVERVIEW:      Initialize the context's heap. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *
 *   returns:     nothing 
 *=======================================================================*/
static void CCinit(context_type *context)
{
    struct CCpool *new = (struct CCpool *) sysMalloc(sizeof(struct CCpool));
    /* Set context->CCroot to 0 if new == 0 to tell CCdestroy to lay off */
    context->CCroot = context->CCcurrent = new;
    if (new == 0) {
    CCerror(context, "Out of memory");
    }
    new->next = NULL;
    new->segSize = CCSegSize;
    context->CCfree_size = CCSegSize;
    context->CCfree_ptr = &new->space[0];
}

/*=========================================================================
 * FUNCTION:      CCreinit 
 * OVERVIEW:      Reuse all the space that we have in the context's heap. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *
 *   returns:     nothing 
 *=======================================================================*/
static void CCreinit(context_type *context)
{
    struct CCpool *first = context->CCroot;
    context->CCcurrent = first;
    context->CCfree_size = CCSegSize;
    context->CCfree_ptr = &first->space[0];
}

/*=========================================================================
 * FUNCTION:      CCdestroy
 * OVERVIEW:      Destroy the context's heap. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *
 *   returns:     nothing 
 *=======================================================================*/
static void CCdestroy(context_type *context)
{
    struct CCpool *this = context->CCroot;
    while (this) {
    struct CCpool *next = this->next;
    sysFree(this);
    this = next;
    }
    /* These two aren't necessary.  But can't hurt either */
    context->CCroot = context->CCcurrent = NULL;
    context->CCfree_ptr = 0;
}

/*=========================================================================
 * FUNCTION:      CCalloc
 * OVERVIEW:      Allocate an object of the given size from the context's heap. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: size
 *                bool_t: zero
 *
 *   returns:     pointer to void
 *=======================================================================*/
static void *
CCalloc(context_type *context, int size, bool_t zero)
{

    register char *p;
    /* Round CC to the size of a pointer */
    size = (size + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1);

    if (context->CCfree_size <  size) {
    struct CCpool *current = context->CCcurrent;
    struct CCpool *new;
    if (size > CCSegSize) {    /* we need to allocate a special block */
        new = (struct CCpool *)sysMalloc(sizeof(struct CCpool) + 
                         (size - CCSegSize));
        if (new == 0) {
        CCerror(context, "Out of memory");
        }
        new->next = current->next;
        new->segSize = size;
        current->next = new;
    } else {
        new = current->next;
        if (new == NULL) {
        new = (struct CCpool *) sysMalloc(sizeof(struct CCpool));
        if (new == 0) {
            CCerror(context, "Out of memory");
        }
        current->next = new;
        new->next = NULL;
        new->segSize = CCSegSize;
        }
    }
    context->CCcurrent = new;
    context->CCfree_ptr = &new->space[0];
    context->CCfree_size = new->segSize;
    }
    p = context->CCfree_ptr;
    context->CCfree_ptr += size;
    context->CCfree_size -= size;
    if (zero) 
    memset(p, 0, size);
    return p;
}

/*=========================================================================
 * FUNCTION:      cp_index_to_signature
 * OVERVIEW:      Get the signature associated with a particular field or 
 *                method in the constant pool. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: constant pool index
 *
 *   returns:     char * type
 *=======================================================================*/
static char *
cp_index_to_signature(context_type *context, int cp_index)
{
    union cp_item_type *cp = cbConstantPool(context->class);
    int index = cp[cp_index].i; /* value of Fieldref field */
    int key2 = index & 0xFFFF;    /* index to NameAndType  */
    int signature_index = cp[key2].i & 0xFFFF; 
    char *signature = cp[signature_index].cp;
    return signature;
}

/*=========================================================================
 * FUNCTION:      cp_index_to_fieldname
 * OVERVIEW:      Get the fieldname for the specific index within the
 *                constant pool. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: constant pool index
 *
 *   returns:     char * type
 *=======================================================================*/
static char *
cp_index_to_fieldname(context_type *context, int cp_index)
{
    union cp_item_type *cp = cbConstantPool(context->class);
    int index = cp[cp_index].i; /* value of Fieldref field */
    int key2 = index & 0xFFFF;    /* index to NameAndType  */
    int name_index = cp[key2].i >> 16;
    return cp[name_index].cp;
}

/*=========================================================================
 * FUNCTION:      cp_index_to_class_fullinfo
 * OVERVIEW:      Get the class associated with a particular field or 
 *                method or class in the constant pool. If is_field is true,
 *                then it is a field or method. Otherwise, if false, it is a
 *                class. 
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: constant pool index
 *                bool_t: is_field
 *
 *   returns:     fullinfo_type
 *=======================================================================*/
static fullinfo_type
cp_index_to_class_fullinfo(context_type *context, int cp_index, bool_t is_field)
{
    union cp_item_type *cp = cbConstantPool(context->class);
    unsigned classkey = is_field ? (cp[cp_index].i >> 16) : cp_index;
    char *classname = GetClassConstantClassName(cp, classkey);
    if (classname[0] == SIGNATURE_ARRAY) {
    fullinfo_type result;
    /* This make recursively call us, in case of a class array */
    signature_to_fieldtype(context, &classname, &result);
    return result;
    } else {
    return MAKE_CLASSNAME_INFO_WITH_COPY(context, classname, 0);
    }
}

/*=========================================================================
 * FUNCTION:      CCerror 
 * OVERVIEW:      Error handling
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                char: pointer to format
 *
 *   returns:     nothing 
 *=======================================================================*/
static void 
CCerror (context_type *context, char *format, ...)
{
    va_list args;
    struct methodblock *mb = context->mb;
    printCurrentClassName();
    if (mb != 0) {
        jio_fprintf(stderr, "VERIFIER ERROR %s.%s%s:\n", 
            cbName(fieldclass(&mb->fb)), mb->fb.name, mb->fb.signature);
    } else {
        jio_fprintf(stderr, "VERIFIER ERROR class %s (mb uninitialized):\n",
            cbName(context->class));
    }
    va_start(args, format);
       jio_vfprintf(stderr, format, args);        
    va_end(args);
    jio_fprintf(stderr, "\n");
    exit(1);
}

/*=========================================================================
 * FUNCTION:      signature_to_fieldtype
 * OVERVIEW:      Given the full info type for a field, returns the field type
 *                which corresponds to this signature.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                char: **signature_p
 *                fullinfo_type: *full_info_p
 *
 *   returns:     char
 *=======================================================================*/
static char 
signature_to_fieldtype(context_type *context, 
               char **signature_p, fullinfo_type *full_info_p)
{
    char *p = *signature_p;
    fullinfo_type full_info = MAKE_FULLINFO(0, 0, 0);
    char result;
    int array_depth = 0;
    
    for (;;) { 
    switch(*p++) {
            default:
        full_info = MAKE_FULLINFO(ITEM_Bogus, 0, 0);
        result = 0; 
        break;

            case SIGNATURE_BYTE: 
        full_info = (array_depth > 0)
                      ? MAKE_FULLINFO(ITEM_Byte, 0, 0)
                      : MAKE_FULLINFO(ITEM_Integer, 0, 0);
        result = 'I'; 
        break;

        case SIGNATURE_BOOLEAN: 
        full_info = (array_depth > 0)
                      ? MAKE_FULLINFO(ITEM_Boolean, 0, 0)
                      : MAKE_FULLINFO(ITEM_Integer, 0, 0);
        result = 'I'; 
        break;

            case SIGNATURE_CHAR:
        full_info = (array_depth > 0)
                      ? MAKE_FULLINFO(ITEM_Char, 0, 0)
                  : MAKE_FULLINFO(ITEM_Integer, 0, 0);
        result = 'I'; 
        break;

            case SIGNATURE_SHORT: 
        full_info = (array_depth > 0)
                      ? MAKE_FULLINFO(ITEM_Short, 0, 0)
                  : MAKE_FULLINFO(ITEM_Integer, 0, 0);
        result = 'I'; 
        break;

            case SIGNATURE_INT:
        full_info = MAKE_FULLINFO(ITEM_Integer, 0, 0);
        result = 'I'; 
        break;

            case SIGNATURE_FLOAT:
        full_info = MAKE_FULLINFO(ITEM_Float, 0, 0);
        result = 'F'; 
        break;

            case SIGNATURE_DOUBLE:
        full_info = MAKE_FULLINFO(ITEM_Double, 0, 0);
        result = 'D'; 
        break;

        case SIGNATURE_LONG:
        full_info = MAKE_FULLINFO(ITEM_Long, 0, 0);
        result = 'L'; 
        break;

            case SIGNATURE_ARRAY:
        array_depth++;
        continue;    /* only time we ever do the loop > 1 */

            case SIGNATURE_CLASS: {
        char buffer_space[256];
        char *buffer = buffer_space;
        char *finish = strchr(p, SIGNATURE_ENDCLASS);
        int length = finish - p;
        if (length + 1 > sizeof(buffer_space)) {
            buffer = sysMalloc(length + 1);
            if (buffer == 0) {
            CCerror(context, "Out of memory");
            }
        }
            memcpy(buffer, p, length);
        buffer[length] = '\0';
        full_info = MAKE_CLASSNAME_INFO_WITH_COPY(context, buffer, 0);
        result = 'A';
        p = finish + 1;
        if (buffer != buffer_space) 
            sysFree(buffer);
        break;
        }
    } /* end of switch */
    break;
    }
    *signature_p = p;
    if (array_depth == 0 || result == 0) { 
    /* either not an array, or result is bogus */
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

/*=========================================================================
 * FUNCTION:      decrement_indirection 
 * OVERVIEW:      Given an array type, create the type that has one less
 *                level of indirection.
 * INTERFACE:
 *   parameters:  fullinfo_type array_info 
 *
 *   returns:     fullinfo_type 
 *=======================================================================*/
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
           && ((type == ITEM_Short || type == ITEM_Byte || type == ITEM_Boolean || type == ITEM_Char)))
        type = ITEM_Integer;
    return MAKE_FULLINFO(type, indirection, extra_info);
    }
}

/*=========================================================================
 * FUNCTION:      isAssignableTo 
 * OVERVIEW:      Given an object of the "from" type, determine if it can be
 *                assigned to an object of the "to" type.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                fullinfo_type: from
 *                fullinfo_type: to 
 *
 *   returns:     boolean type 
 *=======================================================================*/
static bool_t isAssignableTo(context_type *context, 
                 fullinfo_type from, fullinfo_type to)
{
    return (merge_fullinfo_types(context, from, to, TRUE) == to);
}

/*=========================================================================
 * FUNCTION:      merge_fullinfo_types 
 * OVERVIEW:      Given two fullinfo_types, find their lowest common deno-
 *                minator. If the assignable_p argument is non-null, we are
 *                really just calling to find out if "<target> := <value>" 
 *                is a legitimate assignment.
 *                We treat all interfaces as if they were of type '
 *                java/lang/Object, since the runtime will do the full
 *                checking.
 *
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                fullinfo_type: value 
 *                fullinfo_type: target
 *                bool_t for_assignment 
 *
 *   returns:     fullinfo_type 
 *=======================================================================*/
static fullinfo_type 
merge_fullinfo_types(context_type *context, 
             fullinfo_type value, fullinfo_type target,
             bool_t for_assignment)
{
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
    /*   For assignments, Interface := Object, return Interface
     * rather than Object, so that isAssignableTo() will get the right
     * result.      */
    if (for_assignment && (WITH_ZERO_EXTRA_INFO(target) == 
                      MAKE_FULLINFO(ITEM_Object, 0, 0))) {
        ClassClass *cb = object_fullinfo_to_classclass(context, target);
        if (cb && cbIsInterface(cb)) 
        return target;
    }
    return value;
    }
    if (GET_INDIRECTION(value) > 0 || GET_INDIRECTION(target) > 0) {
    /* At least one is an array.  Neither is java/lang/Object or NULL.
     * Moreover, the types are not identical.
     * The result must either be Object, or an array of some object type.
     */
    int dimen_value = GET_INDIRECTION(value);
    int dimen_target = GET_INDIRECTION(target);
    
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
    if (dimen_value == dimen_target) { 
            /* Arrays of the same dimension.  Merge their base types. */
        fullinfo_type value_base = WITH_ZERO_INDIRECTION(value);
        fullinfo_type target_base = WITH_ZERO_INDIRECTION(target);
        fullinfo_type  result_base = 
        merge_fullinfo_types(context, value_base, target_base,
                        for_assignment);
        if (result_base == MAKE_FULLINFO(ITEM_Bogus, 0, 0))
        /* bogus in, bogus out */
        return result_base;
        return MAKE_FULLINFO(ITEM_Object, dimen_value,
                 GET_EXTRA_INFO(result_base));
    } else { 
            /* Arrays of different sizes.  Return Object, with a dimension
             * of the smaller of the two.
             */
        int dimen = dimen_value < dimen_target ? dimen_value : dimen_target;
        return MAKE_Object_ARRAY(dimen);
    }
    } else {
    /* Both are non-array objects. Neither is java/lang/Object or NULL */
    ClassClass *cb_value, *cb_target, *cb_super_value, *cb_super_target;
    void **addr;
    int value_info;

    /* Let's get the classes corresponding to each of these.  Treat 
     * interfaces as if they were java/lang/Object.  See note above. */
    cb_target = object_fullinfo_to_classclass(context, target);
    if (cb_target == 0) 
        return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
    if (cbIsInterface(cb_target)) 
        return for_assignment ? target : context->object_info;
    cb_value = object_fullinfo_to_classclass(context, value);
    if (cb_value == 0) 
        return MAKE_FULLINFO(ITEM_Bogus, 0, 0);
    if (cbIsInterface(cb_value))
        return context->object_info;
    
    /* If this is for assignment of target := value, we just need to see if
     * cb_target is a superclass of cb_value.  Save ourselves a lot of 
     * work.
     */
    if (for_assignment) {
        for (cb_super_value = cb_value; 
         cbSuperclass(cb_super_value) != NULL; 
         cb_super_value = cbSuperclass(cb_super_value)) {
        if (cb_super_value == cb_target) {
            return target;
        }
        }
        return context->object_info;
    }

    /* Find out whether cb_value or cb_target is deeper in the class
     * tree by moving both toward the root, and seeing who gets there
     * first.                                                          */
    for (cb_super_value = cb_value, cb_super_target = cb_target;
         cbSuperclass(cb_super_value) && cbSuperclass(cb_super_target); ) {
        /* Optimization.  If either hits the other when going up looking
         * for a parent, then might as well return the parent immediately */
        if (cb_super_value == cb_target) 
        return target;
        if (cb_super_target == cb_value) 
        return value;
        cb_super_value= cbSuperclass(cb_super_value);
        cb_super_target = cbSuperclass(cb_super_target);
    } 
    /* At most one of the following two while clauses will be executed. 
     * Bring the deeper of cb_target and cb_value to the depth of the 
     * shallower one. 
     */
    while (cbSuperclass(cb_super_value)) { /* cb_value is deeper */
        cb_super_value= cbSuperclass(cb_super_value); 
        cb_value= cbSuperclass(cb_value); 
    }
    while (cbSuperclass(cb_super_target)) { /* cb_target is deeper */
        cb_super_target= cbSuperclass(cb_super_target); 
        cb_target = cbSuperclass(cb_target); 
    }
    
    /* Walk both up, maintaining equal depth, until a join is found.  We
     * know that we will find one.  */
    while (cb_value != cb_target) { 
        cb_value =  cbSuperclass(cb_value);
        cb_target =  cbSuperclass(cb_target);
    }
    /* Get the info for this guy.  We know its cb_value, so we should
     * fill that in, while we're at it.                      */
    value_info = Str2ID_Local(context, &context->classHash,
                  cbName(cb_value), &addr, TRUE);
    *addr = cb_value;
    return MAKE_FULLINFO(ITEM_Object, 0, value_info);
    } /* both items are classes */
}

/*=========================================================================
 * FUNCTION:      object_fullinfo_to_classclass 
 * OVERVIEW:      Given a fullinfo_type corresponding to an Object, return the
 *                pointer to the ClassClass structure of that type.
 *                Returns 0 for an illegal class.
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                fullinfo_type: classinfo 
 *
 *   returns:     pointer to the ClassClass type
 *=======================================================================*/
static ClassClass *
object_fullinfo_to_classclass(context_type *context, fullinfo_type classinfo)
{
    void **addr;
    ClassClass *cb;
        
    unsigned short info = GET_EXTRA_INFO(classinfo);
    char *classname = ID2Str_Local(context, context->classHash, info, &addr);
    if ((cb = *addr) != 0) {
    return cb;
    } else {
    *addr = cb = FindClassFromClass(0, classname, FALSE, context->class);
    if (cb == 0)
        CCerror(context, "Cannot find class %s", classname);
    return cb;
    }
}

/*=========================================================================
 * The functions below are for debugging the preverifier 
 *=======================================================================*/

#ifdef DEBUG_VERIFIER

static void print_fullinfo_type(context_type *, fullinfo_type, bool_t);

/*=========================================================================
 * FUNCTION:      print_stack
 * OVERVIEW:      Prints stack information.
 *               
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                stack_info: pointer to stack_info 
 *
 *   returns:     nothing
 *=======================================================================*/
static void 
print_stack(context_type *context, stack_info_type *stack_info)
{
    stack_item_type *stack = stack_info->stack;
    if (stack_info->stack_size == UNKNOWN_STACK_SIZE) {
    jio_fprintf(stdout, "x");
    } else {
    jio_fprintf(stdout, "(");
    for ( ; stack != 0; stack = stack->next) 
        print_fullinfo_type(context, stack->item, verify_verbose > 1);
    jio_fprintf(stdout, ")");
    }
}    

/*=========================================================================
 * FUNCTION:      print_registers
 * OVERVIEW:      Prints registers.
 *               
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                register_info_type: pointer to register_info 
 *
 *   returns:     nothing
 *=======================================================================*/
static void
print_registers(context_type *context, register_info_type *register_info)
{
    int register_count = register_info->register_count;
    if (register_count == UNKNOWN_REGISTER_COUNT) {
    jio_fprintf(stdout, "x");
    } else {
    fullinfo_type *registers = register_info->registers;
    int mask_count = register_info->mask_count;
    mask_type *masks = register_info->masks;
    int i, j;

    jio_fprintf(stdout, "{");
    for (i = 0; i < register_count; i++) 
        print_fullinfo_type(context, registers[i], verify_verbose > 1);
    jio_fprintf(stdout, "}");
    for (i = 0; i < mask_count; i++) { 
        char *separator = "";
        int *modifies = masks[i].modifies;
        jio_fprintf(stdout, "<%d: ", masks[i].entry);
        for (j = 0; j < context->mb->nlocals; j++) 
        if (IS_BIT_SET(modifies, j)) {
            jio_fprintf(stdout, "%s%d", separator, j);
            separator = ",";
        }
        jio_fprintf(stdout, ">");
    }
    }
}

/*=========================================================================
 * FUNCTION:      print_flags
 * OVERVIEW:      Prints flags.
 *               
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                flag_type: and_flags
 *                flag_type: or_flags
 *
 *   returns:     nothing
 *=======================================================================*/
static void
print_flags(context_type *context, flag_type and_flags, flag_type or_flags)
{ 
    if (and_flags != ((flag_type)-1) || or_flags != 0) {
    jio_fprintf(stdout, "<%x %x>", and_flags, or_flags);
    }
}        

/*=========================================================================
 * FUNCTION:      print_fullinfo_type
 * OVERVIEW:      Prints fullinfo_type information.
 *               
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                fullinfo_type: type
 *                bool_t verbose
 *
 *   returns:     nothing
 *=======================================================================*/
static void 
print_fullinfo_type(context_type *context, fullinfo_type type, bool_t verbose) 
{
    int i;
    int indirection = GET_INDIRECTION(type);
    for (i = indirection; i-- > 0; )
    jio_fprintf(stdout, "[");
    switch (GET_ITEM_TYPE(type)) {
        case ITEM_Integer:       
        jio_fprintf(stdout, "I"); break;
    case ITEM_Float:         
        jio_fprintf(stdout, "F"); break;
    case ITEM_Double:        
        jio_fprintf(stdout, "D"); break;
    case ITEM_Double_2:      
        jio_fprintf(stdout, "d"); break;
    case ITEM_Long:          
        jio_fprintf(stdout, "L"); break;
    case ITEM_Long_2:        
        jio_fprintf(stdout, "l"); break;
    case ITEM_ReturnAddress: 
        jio_fprintf(stdout, "a"); break;
    case ITEM_Object:        
        if (!verbose) {
        jio_fprintf(stdout, "A");
        } else {
        unsigned short extra = GET_EXTRA_INFO(type);
        if (extra == 0) {
            jio_fprintf(stdout, "/Null/");
        } else {
            char *name = ID2Str_Local(context, context->classHash,
                          extra, 0);
            char *name2 = strrchr(name, '/');
            jio_fprintf(stdout, "/%s/", name2 ? name2 + 1 : name);
        }
        }
        break;
    case ITEM_Char:
        jio_fprintf(stdout, "C"); break;
    case ITEM_Short:
        jio_fprintf(stdout, "S"); break;
    case ITEM_Byte:
        jio_fprintf(stdout, "B"); break;
    case ITEM_Boolean:
        jio_fprintf(stdout, "Z"); break;
        case ITEM_NewObject:
        if (!verbose) {
        jio_fprintf(stdout, "@");
        } else {
        int inum = GET_EXTRA_INFO(type);
        fullinfo_type real_type = 
            context->instruction_data[inum].operand2.fi;
        jio_fprintf(stdout, ">");
        print_fullinfo_type(context, real_type, TRUE);
        jio_fprintf(stdout, "<");
        }
        break;
        case ITEM_InitObject:
        jio_fprintf(stdout, verbose ? ">/this/<" : "@");
        break;

    default: 
        jio_fprintf(stdout, "?"); break;
    }
    for (i = indirection; i-- > 0; )
    jio_fprintf(stdout, "]");
}

/*=========================================================================
 * FUNCTION:      print_formatted_fieldname
 * OVERVIEW:      Prints formatted fieldname associated with the given 
 *                index within the constant pool.
 *               
 * INTERFACE:
 *   parameters:  pointer to the context_type
 *                int: index
 *
 *   returns:     nothing
 *=======================================================================*/
static void 
print_formatted_fieldname(context_type *context, int index)
{
    union cp_item_type *constant_pool = cbConstantPool(context->class);
    unsigned char *type_table = 
        constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].type;
    unsigned type = CONSTANT_POOL_TYPE_TABLE_GET_TYPE(type_table, index);

    unsigned key = constant_pool[index].i; 
    unsigned classkey = key >> 16; 
    unsigned nametypekey = key & 0xFFFF;
    unsigned nametypeindex = constant_pool[nametypekey].i;
    unsigned fieldnameindex = nametypeindex >> 16;
    unsigned fieldtypeindex = nametypeindex & 0xFFFF;
    jio_fprintf(stdout, "  <%s.%s%s%s>",
        GetClassConstantClassName(constant_pool, classkey),
        constant_pool[fieldnameindex].cp, 
        type == CONSTANT_Fieldref ? " " : "",
        constant_pool[fieldtypeindex].cp);
}

#endif /*DEBUG_VERIFIER*/

