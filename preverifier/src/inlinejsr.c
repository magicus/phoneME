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
 * SUBSYSTEM: JSR inlining
 * FILE:      inlinejsr.c
 * OVERVIEW:  Routines for inlining of JSR and RET bytecodes.  
 *
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <check_code.h>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

/* Maximum byte code size is 64K. */
#define MAX_CODE_SIZE 65535

typedef struct SubrContext {
    int id;                         /* id, for debugging */
    int depth;                      /* depth of subroutine */
    struct SubrContext *parent;     /* subroutine of caller */
    struct CodeRef *caller;         /* jsr that got us there */
    struct CodeRef *nextInstruction; /* first instruction following inlining */
    int target;

    struct SubrContext *next;   /* linked list of all subr contexts */
} SubrContext;

/* global context, keep track of info when a method is rewritten */
typedef struct JsrContext {
    context_type *vcontext;     /* The verifier's context */
    struct CodeRef *codeRef;    /* big array of codeRef's */
    struct CodeRef *codeRefEnd; /* pointer to next codeRef to fill in */
    int scontext_id;            /* ID assigned to last SubrContext */
    struct SubrContext *allSubrContexts; /* pointer to linked list */
    struct CodeRef **mapping;   /* maps inumbers CodeRef's */
} JsrContext;

/* A single instruction in the resulting stream */
typedef struct CodeRef { 
    long inumber;               /* instruction number in original code */
    SubrContext *subroutine;    /* subroutine call that this is part of */
    enum { CRF_NORMAL,          /* normal instruction */
           CRF_SKIP,            /* skip this instruction */
           CRF_JSR_SIMPLE_GOTO, /* jsr to subroutine that doesn't return */
           CRF_JSR_TARGETED_GOTO, /* jsr to subroutine that does return */
           CRF_RET_SIMPLE_GOTO  /* ret that's not at the end of subroutine */
    } flags;
    /* My offset in the new code */
    int offset;
    struct CodeRef *next;       /* next codeRef with same "inumber" */
} CodeRef;

static bool_t matchSubroutine(JsrContext *, instruction_data_type*, 
                            SubrContext *);
static bool_t subroutineGoto(JsrContext *, SubrContext *, SubrContext *);

static void
rewriteOneSubroutine(JsrContext *context, SubrContext *subroutine);

static void fixupCode(JsrContext*);
static void fixupExceptionHandlers(JsrContext*);
static void fixupLineNumberTable(JsrContext*);
static void fixupVariableTable(JsrContext*);

static void
updateTarget(JsrContext *, 
             int inumber, 
             SubrContext* subroutine, 
             void* target, int offset, int size);

void
rewriteCode(context_type *vcontext, struct methodblock *mb)
{
    JsrContext context_buf;
    JsrContext *context = &context_buf;

#if MYDEBUG
    printf("Starting %s.%s%s\n", cbName(mb->fb.clazz), mb->fb.name, mb->fb.signature);
#endif
    /* Initialize the context */
    memset(context, 0, sizeof(context));
    context->vcontext = vcontext; /* The verifier context */
    /* Allow up to MAX_CODE_SIZE instructions.  */
    context->codeRef = (CodeRef *)malloc(MAX_CODE_SIZE * sizeof(CodeRef));
    context->codeRefEnd = context->codeRef;
    /* Id (for debugging) of last subroutine structure created */
    context->scontext_id = 0;   
    /* Keep a list of all subroutines, so that we can easily free() them */
    context->allSubrContexts = NULL;
    /* Make it easy to go from inumber to all CodeRef's that have that inumber*/
    context->mapping = (CodeRef **)calloc(vcontext->instruction_count, 
                                          sizeof(CodeRef **));

    /* Fill in context->codeRef with this routine.  In line all subroutine
     * calls, and delete all unreachable code */
    rewriteOneSubroutine(context, NULL);

    /* Modify mb->code and mb->code_length for the new code */
    fixupCode(context);

    /* Update the exception table */
    if (mb->exception_table_length != 0) { 
        fixupExceptionHandlers(context);
    }

    /* Update the line number table */
    if (mb->line_number_table_length != 0) { 
        fixupLineNumberTable(context);
    }

    /* Update the local variable table */
    if (mb->localvar_table_length != 0) { 
        fixupVariableTable(context);
    }

    /* Clean up */
    free(context->codeRef);
    free(context->mapping);

    /* Free all the subroutine contexts that we created */
    while (context->allSubrContexts != NULL) { 
        SubrContext *this = context->allSubrContexts;
        SubrContext *next = this->next;
        free(this);
        context->allSubrContexts = next;
    }
}

static void
rewriteOneSubroutine(JsrContext *context, SubrContext *subroutine)
{ 
    context_type *vcontext = context->vcontext;
    int depth = subroutine ? subroutine->depth : 0;
    instruction_data_type *idata = vcontext->instruction_data;
    int instruction_count = vcontext->instruction_count;
    CodeRef **mapping = context->mapping;

    instruction_data_type *this_idata;
    int inumber;
    int count = 0;
    CodeRef *retOpcode = NULL;
    

    for (    inumber = 0, this_idata = idata;
             inumber < instruction_count;
             inumber++, this_idata++) { 
        if (    (this_idata->or_flags & FLAG_REACHED) 
            &&  (this_idata->register_info.mask_count == depth)
            &&  ((depth == 0) 
                     || matchSubroutine(context, this_idata, subroutine))) { 
            
            /* We have an instruction that is part of this subroutine */

            CodeRef *codeRef = context->codeRefEnd++;
#if MYDEBUG
            printf("\t%d:\t%d (%d)\t%s (%d)\n", 
                   (codeRef - context->codeRef), /* new instruction index */
                   inumber, (subroutine ? subroutine->id : 0),
                   (this_idata->opcode == 256 
                         ? "invokeinit" : opnames[this_idata->opcode]),
                   this_idata->offset);
#endif
            codeRef->inumber = inumber;
            codeRef->subroutine = subroutine;
            codeRef->flags = CRF_NORMAL;
            codeRef->next = mapping[inumber]; /* Add to inumber mapping */
            mapping[inumber] = codeRef;

            count++;

            if (count == 1 && depth > 0) { 
                /* This is the first instruction included as part of the
                 * subroutine call.  If it's the target of the jsr that got
                 * us here, then we can just "ignore" the jsr.  
                 * Otherwise, we have to convert the 'jsr' into a 'goto'
                 */
                CodeRef *caller = subroutine->caller;
                if (inumber != idata[caller->inumber].operand.i) { 
                    caller->flags = CRF_JSR_TARGETED_GOTO;
                }
            }

            switch(this_idata->opcode) { 
                case opc_jsr: case opc_jsr_w: 
                    if (this_idata->operand2.i == UNKNOWN_RET_INSTRUCTION) { 
                        /* We're calling a subroutine that doesn't return.
                         * The verifier has already made sure that the
                         * subroutine doesn't have a deeper depth.
                         * We turn the JSR into a goto */
                        codeRef->flags = CRF_JSR_SIMPLE_GOTO;
                    } else { 
                        SubrContext *newSubr = malloc(sizeof(SubrContext));

                        /* In rare cases, we'll have to change this in the
                         * subroutine code */
                        codeRef->flags = CRF_SKIP;

                        /* Create a new subroutine, and inline it */
                        newSubr->id = ++context->scontext_id;
                        newSubr->caller = codeRef;
                        newSubr->target = this_idata->operand.i;
                        newSubr->depth = depth + 1;
                        newSubr->nextInstruction = NULL; /* unknown for now */
                        newSubr->parent = subroutine;
                        /* Add this to the list of all subroutine contexts */
                        newSubr->next = context->allSubrContexts;
                        context->allSubrContexts = newSubr;
                        /* Generate the code for this subroutine */
                        rewriteOneSubroutine(context, newSubr);
                    }
                    break;

                case opc_ret:
                    if (retOpcode != NULL) { 
                        /* There should only be one per subroutine */
                        panic("Multiple return opcodes??");
                    } else if (depth == 0) { 
                        /* We're not in a subroutine */
                        panic("Ret at depth = 0");
                    }
                    retOpcode = codeRef;
                    /* Flags are set at the end of the loop, below */
                    break;

                case opc_astore: 
                    /* We discard any astore's that move a return address
                     * from the stack to a register. 
                     */
                    if (GET_ITEM_TYPE(this_idata->stack_info.stack->item) 
                                        == ITEM_ReturnAddress) {
                        codeRef->flags = CRF_SKIP; 
                    } 
                    break;

                default: 
                    /* Nothing to do */
                    break;
            }
        }
    }
    if (depth > 0) { 
        subroutine->nextInstruction = context->codeRefEnd;
        if (retOpcode != NULL) { 
            /* If the last instruction wasn't a 'ret', then we need to
             * convert the 'ret' into a 'goto'.
             */
            if (context->codeRefEnd == retOpcode + 1) { 
                retOpcode->flags = CRF_SKIP;
            } else { 
                retOpcode->flags = CRF_RET_SIMPLE_GOTO;
            }
        }
    }
}

static void
fixupCode(JsrContext *context) 
{
    context_type *vcontext = context->vcontext;
    instruction_data_type *idata = vcontext->instruction_data;
    struct methodblock *mb = vcontext->mb;
    unsigned char *oldCode = mb->code;
    CodeRef *codeRefEnd = context->codeRefEnd;

    unsigned char *newCode;
    CodeRef *codeRef;
    int pc;
    long newCodeLength;

    /* Assign offsets to each instruction. */
#if MYDEBUG
    printf("Assigning offsets\n");
#endif
    for (pc = 0, codeRef = context->codeRef; codeRef < codeRefEnd; codeRef++) {
        instruction_data_type *this_idata = &idata[codeRef->inumber];
        opcode_type opcode = this_idata->opcode;
        
        codeRef->offset = pc;

#if MYDEBUG
        printf("\t%d:\t%d\tpc=%d\t%s (%d) %s\n", 
               (codeRef - context->codeRef), 
               (this_idata - vcontext->instruction_data), 
               pc, 
               (this_idata->opcode == 256 
                    ? "invokeinit" : opnames[this_idata->opcode]),
               this_idata->offset,
               ((codeRef->flags == CRF_SKIP) ? " XX" : "")
               );
#endif
        
        /* Now increment the pc, depending on the instruction */
        if (codeRef->flags == CRF_SKIP) { 
            /* do nothing */
        } else if (opcode == opc_tableswitch || opcode == opc_lookupswitch) {
            /* This mysterious calculation works. 
             * The first term increments pc and then rounds it up to a
             * multiple of 4.   The second term is the size of the word-aligned
             * values.
             */
            pc = ((pc + 1 + 3) & ~3) + ((this_idata->length - 1) & ~3);
        } else if (opcode == opc_ret) { 
            /* We must be turning it into an opc_goto */
            pc += 3;
        } else { 
            pc += this_idata->length;
        }
    }

    /* Create a new code object */
    newCode = (unsigned char *)malloc(pc);
    newCodeLength = pc;

#if MYDEBUG
    printf("Creating code of length %d\n", pc);
#endif

    for (codeRef = context->codeRef; codeRef < codeRefEnd; codeRef++) {
        if (codeRef->flags != CRF_SKIP) { 
            instruction_data_type *this_idata = &idata[codeRef->inumber];
            opcode_type opcode = this_idata->opcode;
            int pc = codeRef->offset;
            unsigned char *source = &oldCode[this_idata->offset];
            unsigned char *target = &newCode[pc];
            
#if MYDEBUG
            printf("\t%d:\t%d\tpc=%d\t%s (%d) \n", 
                   (codeRef - context->codeRef), 
                   (this_idata - vcontext->instruction_data), 
                   pc, 
                   (this_idata->opcode == 256 
                         ? "invokeinit" : opnames[this_idata->opcode]),
                   this_idata->offset
                   );
#endif
            
            switch(opcode) { 
                case opc_ifeq: case opc_ifne: case opc_iflt: 
                case opc_ifge: case opc_ifgt: case opc_ifle:
                case opc_ifnull: case opc_ifnonnull:
                case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmplt: 
                case opc_if_icmpge: case opc_if_icmpgt: case opc_if_icmple:
                case opc_if_acmpeq: case opc_if_acmpne: 
                case opc_goto: case opc_goto_w:
                    target[0] = source[0];
                    updateTarget(context, this_idata->operand.i, 
                                 codeRef->subroutine, 
                                 target + 1, pc, this_idata->length - 1);
                    break;
                
                case opc_jsr: case opc_jsr_w: 
                    target[0] = opc_goto;
                    if (codeRef->flags == CRF_JSR_SIMPLE_GOTO) { 
                        updateTarget(context, this_idata->operand.i, 
                                     codeRef->subroutine, 
                                     target + 1, pc, this_idata->length - 1);
                    } else if (codeRef->flags == CRF_JSR_TARGETED_GOTO) { 
                        updateTarget(context, this_idata->operand.i, 
                                     codeRef[1].subroutine, 
                                     target + 1, pc, this_idata->length - 1);
                    } else { 
                        panic("Shouldn't have anything referring to jsr");
                    }
                    break;

                case opc_ret:
                    if (codeRef->flags & CRF_RET_SIMPLE_GOTO) { 
                        int gotoTarget = 
                            codeRef->subroutine->nextInstruction->offset;
                        target[0] = opc_goto;
                        target[1] = (gotoTarget - pc) >> 8;
                        target[2] = (gotoTarget - pc);
                    } else { 
                        panic("Shouldn't have anything referring to ret");
                    }
                    break;

                default:
                    memcpy(target, source, this_idata->length);
                    break;

                case opc_tableswitch: 
                case opc_lookupswitch: {
                    int *successors = this_idata->operand.ip;
                    int keys = successors[0] - 1; /* don't include default */
                    SubrContext *subroutine = codeRef->subroutine;
                    int i;
                    
                    long *targetPtr, *sourcePtr;
                    target[0] = source[0];
                    target[1] = target[2] = target[3] = 0; /* clear alignment */
                    
                    targetPtr = (long *)UCALIGN(target + 1);
                    sourcePtr = (long *)UCALIGN(source + 1);
                    
                    /* Update the default target */
                    updateTarget(context, successors[1], subroutine, 
                                 targetPtr, pc, 4);
                    if (opcode == opc_tableswitch) { 
                        targetPtr[1] = sourcePtr[1]; /* low */
                        targetPtr[2] = sourcePtr[2]; /* high */
                        for (i = 0; i < keys; i++) { 
                            updateTarget(context, successors[2 + i], subroutine,
                                         &targetPtr[3 + i], pc, 4);
                        }
                    } else { 
                        targetPtr[1] = sourcePtr[1]; /* pairs */ 
                        for (i = 0; i < keys; i++) { 
                            targetPtr[2 + (i << 1)] = sourcePtr[2 + (i << 1)];
                            updateTarget(context, successors[2 + i], subroutine,
                                         &targetPtr[3 + (i << 1)], pc, 4);
                        }
                    }
                    break;

                }
            }
        }
    }
    mb->code = newCode;
    mb->code_length = newCodeLength;
}

static void fixupExceptionHandlers(JsrContext *context) { 
    const int catchFrameSize = sizeof(struct CatchFrame);
    context_type *vcontext = context->vcontext;
    struct methodblock *mb = vcontext->mb;

    short *code_data = vcontext->code_data; /* maps offset to inumber */
    
    CodeRef *codeRefEnd = context->codeRefEnd;
        
    /* Structure to hold new catch frames */
    struct CatchFrame *catchFrames = malloc(catchFrameSize * MAX_CODE_SIZE);
    struct CatchFrame *currentCatchFrame = catchFrames;

    CodeRef *hRef, *instRef;
    unsigned long i;

    /* Look at each exception handler */
    for (i = 0; i < mb->exception_table_length; i++) { 
        struct CatchFrame *this_handler = &mb->exception_table[i];
        int start_inumber   = code_data[this_handler->start_pc];
        int end_inumber     = code_data[this_handler->end_pc];
        int handler_inumber = code_data[this_handler->handler_pc];
            
        /* First instruction that maps to the specified handler */
        for (hRef = context->mapping[handler_inumber]
                   ; hRef != NULL; hRef = hRef->next) {
            /* Find all instructions that go to this handler. */
            bool_t wasMatch = FALSE;
            for (instRef = context->codeRef; instRef < codeRefEnd; instRef++) {
                if (instRef->flags != CRF_SKIP) { 
                    bool_t thisMatch = instRef->inumber >= start_inumber
                                    && instRef->inumber < end_inumber
                                    && subroutineGoto(context, 
                                                      instRef->subroutine,
                                                      hRef->subroutine);
                    if (thisMatch && !wasMatch) { 
                        /* Start a new catch frame */
                        memcpy(currentCatchFrame, this_handler, catchFrameSize);
                        currentCatchFrame->handler_pc = hRef->offset;
                        currentCatchFrame->start_pc = instRef->offset;
                        wasMatch = TRUE;
                    } else if (wasMatch && !thisMatch) { 
                        currentCatchFrame->end_pc = instRef->offset;
                        currentCatchFrame++;
                        wasMatch = FALSE;
                    }
                }
            }
            if (wasMatch) { 
                /* We end the code still in the catch frame */
                currentCatchFrame->end_pc = mb->code_length;
                currentCatchFrame++;
            }
        }
    }
    /* free(mb->exception_table); */
    mb->exception_table_length = currentCatchFrame - catchFrames;
    mb->exception_table = realloc(catchFrames, 
                             (char *)currentCatchFrame - (char *)catchFrames);

}

static void fixupLineNumberTable(JsrContext *context) {
    context_type *vcontext = context->vcontext;
    struct methodblock *mb = vcontext->mb;
    int tableLength = mb->line_number_table_length;

    instruction_data_type *idata = vcontext->instruction_data;
    instruction_data_type *last_idata = &idata[vcontext->instruction_count - 1];
    int oldCodeLength = last_idata->offset + last_idata->length;
    struct lineno *lineTable = malloc(sizeof(struct lineno) * MAX_CODE_SIZE);
    struct lineno *currentLineTableEntry = lineTable;
    unsigned long *mapTable = calloc(sizeof(short *), oldCodeLength);
    CodeRef *codeRefEnd = context->codeRefEnd;
    CodeRef *codeRef;
    int i, currentLineNumber;

    { 
        unsigned long startPC, endPC, line, pc;

        for (i = 0; i < tableLength - 1; i++) { 
            startPC = mb->line_number_table[i].pc;
            endPC   = mb->line_number_table[i + 1].pc;
            line    = mb->line_number_table[i].line_number;
            for (pc = startPC; pc < endPC; pc++) { 
                mapTable[pc] = line;
            }
        }
        startPC = mb->line_number_table[tableLength - 1].pc;
        endPC = oldCodeLength;
        line = mb->line_number_table[tableLength - 1].line_number;
        for (pc = startPC; pc < endPC; pc++) { 
            mapTable[pc] = line;
        }
    }

    currentLineNumber = -1;
    for (codeRef = context->codeRef; codeRef < codeRefEnd; codeRef++) {
        if (codeRef->flags != CRF_SKIP) { 
            instruction_data_type *this_idata = &idata[codeRef->inumber];
            int thisLineNumber = mapTable[this_idata->offset];
            if (thisLineNumber != currentLineNumber) { 
                currentLineTableEntry->line_number = thisLineNumber;
                currentLineTableEntry->pc = codeRef->offset;
                currentLineTableEntry++;
                currentLineNumber = thisLineNumber;
            }
        }
    }
    
    free(mapTable);
    mb->line_number_table = realloc(lineTable, 
                                    (char *)currentLineTableEntry - 
                                    (char *)lineTable);
    mb->line_number_table_length = currentLineTableEntry - lineTable;
}

static void fixupVariableTable(JsrContext *context) { 
    context_type *vcontext = context->vcontext;
    struct methodblock *mb = context->vcontext->mb;
    instruction_data_type *idata = vcontext->instruction_data;
    CodeRef *codeRefEnd = context->codeRefEnd;
    CodeRef *codeRef;
    unsigned long i;

    struct localvar *localVars = 
        malloc(sizeof(struct localvar) * MAX_CODE_SIZE);
    struct localvar *currentLocalVar = localVars;

    for (i = 0; i < mb->localvar_table_length; i++) { 
        struct localvar *oldEntry = &mb->localvar_table[i];
        int startPC = oldEntry->pc0;
        int endPC = startPC + oldEntry->length; /* inclusive! */
            
        bool_t was_matching = FALSE;

        for (codeRef = context->codeRef; codeRef < codeRefEnd; codeRef++) {
            if (codeRef->flags != CRF_SKIP) { 
                instruction_data_type *this_idata = &idata[codeRef->inumber];
                bool_t is_matching = this_idata->offset >= startPC 
                                  && this_idata->offset <= endPC;
                if (!was_matching && is_matching) { 
                    memcpy(currentLocalVar, oldEntry, sizeof(struct localvar));
                    currentLocalVar->pc0 = codeRef->offset;
                    was_matching = TRUE;
                } else if (was_matching && !is_matching) { 
                    currentLocalVar->length = 
                        codeRef[-1].offset - currentLocalVar->pc0;
                    currentLocalVar++;
                    was_matching = FALSE;
                }
            }
        }
        if (was_matching) { 
            currentLocalVar->length = 
                codeRefEnd[-1].offset - currentLocalVar->pc0;
            currentLocalVar++;
        }
    }
                
    /* free(mb->localvar_table); */
    mb->localvar_table_length = currentLocalVar - localVars;
    mb->localvar_table = realloc(localVars, 
                                 (char *)currentLocalVar - (char *)localVars);
}

static void
updateTarget(JsrContext *context, int inumber, 
             SubrContext* subroutine, void* target, int offset, int size)
{
    CodeRef *codeRef;
    for (codeRef = context->mapping[inumber];
             codeRef != NULL;
             codeRef = codeRef->next) { 
        if (subroutineGoto(context, subroutine, codeRef->subroutine)) { 
            int value = codeRef->offset - offset;
            unsigned char *t = target;
            if (size == 2) { 
                t[0] = value >> 8;
                t[1] = value;
            } else if (size == 4) { 
                t[0] = value >> 24;
                t[1] = value >> 16;
                t[2] = value >> 8;
                t[3] = value;
            } else { 
                panic("Bad value passed for size");
            }
            return;
        }
    }
    panic("Cannot find value for updateTarget");
}

static bool_t 
subroutineGoto(JsrContext *context, SubrContext *from, SubrContext *to)
{ 
    if (to == NULL || to == from) { 
        return TRUE;
    } else if (from == NULL || to->depth >= from->depth) {
        return FALSE;
    } else { 
        do { from = from->parent; } while (from->depth > to->depth);
        return from == to;
    }
}

static bool_t 
matchSubroutine(JsrContext *context, 
                instruction_data_type *this_idata, 
                SubrContext *subroutine)
{
    int depth = subroutine->depth;
    int i;
    
    for (i = depth - 1; i >= 0; --i) { 
        if (this_idata->register_info.masks[i].entry != subroutine->target) { 
            return FALSE;
        }
        subroutine = subroutine->parent;
    }
    return TRUE;
}

