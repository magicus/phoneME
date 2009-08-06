/*
 * @(#)jitcomments.c	1.7 06/10/10
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

#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/porting/ansi/stdarg.h"
#include "javavm/include/utils.h"

/*************************************
 * Codegen comments implementation
 *************************************/

#ifdef CVM_TRACE_JIT

/* Purpose: Sets the symbol name about to be added. */
void CVMJITprivateSetSymbolName(CVMJITCompilationContext *con,
                                const char *format, ...)
{
    if (format != NULL) {
        va_list args;
        char buf[256];
        size_t length;
        char *symbolName = NULL;

        /* Fill in the formatted string: */
        va_start(args, format);
        CVMformatStringVaList(buf, sizeof(buf), format, args);
        va_end(args);

        /* Add the string as a codegen comment: */
        length = strlen(buf);
        if (length > 0) {
            symbolName = CVMJITmemNew(con, JIT_ALLOC_DEBUGGING, length + 1);
            strcpy(symbolName, buf);
        }
        con->symbolName = symbolName;
    } else {
        con->symbolName = NULL;
    }
}

/* Purpose: Instantiates and adds a new comment to the compilation context's
            comment list. */
void CVMJITprivateAddCodegenComment(CVMJITCompilationContext *con,
                                    const char *format, ...)
{
    if (format != NULL) {
        va_list args;
        char buf[256];
        size_t length;

        /* Fill in the formatted string: */
        va_start(args, format);
        CVMformatStringVaList(buf, sizeof(buf), format, args);
        va_end(args);

        /* Add the string as a codegen comment: */
        length = strlen(buf);
        if (length > 0) {
            char *commentStr;
            CVMCodegenComment *comment =
                CVMJITmemNew(con, JIT_ALLOC_DEBUGGING,
			     sizeof(CVMCodegenComment));
            comment->next = NULL;

            commentStr = CVMJITmemNew(con, JIT_ALLOC_DEBUGGING, length + 1);
            strcpy(commentStr, buf);
            comment->commentStr = commentStr;

            if (con->comments == NULL) {
                con->comments = comment;
            } else {
                CVMassert(con->lastComment != NULL);
                con->lastComment->next = comment;
            }
            con->lastComment = comment;
        }
    }
}

/* Purpose: Pops the current comments list out of the compilation context. */
/* See also: CVMJITpushCodegenComments(). */
void CVMJITprivatePopCodegenComment(CVMJITCompilationContext *con,
                                    CVMCodegenComment **commentPtr)
{
    CVMCodegenComment *comments = con->comments;
    CVMJITresetCodegenComments(con);
    if (commentPtr != NULL) {
        *commentPtr = comments;
    }
}

/* Purpose: Pops the current comments list out of the compilation context. */
/* See also: CVMJITpopCodegenComment(). */
void CVMJITprivatePushCodegenComment(CVMJITCompilationContext *con,
                                     CVMCodegenComment *comment)
{
    if (comment != NULL) {
        con->comments = comment;
        while (comment) {
            con->lastComment = comment;
            comment = comment->next;
        }
    }
}

/* Purpose: Dumps all the collected comments for the instructions emitted
            in the codegen phase. */
void CVMJITdumpCodegenComments(CVMJITCompilationContext *con)
{
    CVMCodegenComment *comment = con->comments;
    if (comment != NULL) {
        CVMtraceJITCodegenExec({
            CVMconsolePrintf("\t@");
            while (comment) {
                CVMconsolePrintf(" %s", comment->commentStr);
                comment = comment->next;
            }
        });
        CVMJITresetCodegenComments(con);
    }
    CVMtraceJITCodegen(("\n"));
}

/* Purpose: Prints the specified comment immediately. */
void CVMJITprivatePrintCodegenComment(const char *format, ...)
{
    CVMtraceJITCodegenExec({
        va_list argList;
        char buf[256];
        va_start(argList, format);
        CVMformatStringVaList(buf, sizeof(buf), format, argList);
        va_end(argList);
        CVMconsolePrintf("          \t\t@ %s\n", buf);
    });
}

/* Purpose: Resets and clears all comments. */
void CVMJITresetCodegenComments(CVMJITCompilationContext *con)
{
    con->comments = NULL;
    con->lastComment = NULL;
}

#endif /* CVM_TRACE_JIT */
