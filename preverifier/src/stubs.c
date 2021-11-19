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
 * SUBSYSTEM: Stubs.
 * FILE:      stubs.c
 * OVERVIEW:  Miscellaneous functions used during class loading, etc.
 *
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <oobj.h>
#include <sys_api.h>

void
SignalError(struct execenv * ee, char *ename, char *DetailMessage)
{
    printCurrentClassName();
    jio_fprintf(stderr, "%s", ename);
    if (DetailMessage) {
        jio_fprintf(stderr, ": %s\n", DetailMessage);
    } else {
        jio_fprintf(stderr, "\n");
    }
    exit(1);
}

bool_t
VerifyClassAccess(ClassClass *current_class, ClassClass *new_class, 
          bool_t classloader_only) 
{
    return TRUE;
}

/* This is called by FindClassFromClass when a class name is being requested
 * by another class that was loaded via a classloader.  For javah, we don't
 * really care.
 */

ClassClass *
ClassLoaderFindClass(struct execenv *ee, 
             struct Hjava_lang_ClassLoader *loader, 
             char *name, bool_t resolve)
{ 
    return NULL;
}

/* Get the execution environment
 */
ExecEnv *
EE() {
    static struct execenv lee;
    return &lee;
}

bool_t RunStaticInitializers(ClassClass *cb)
{
    return TRUE;
}

void InitializeInvoker(ClassClass *cb)
{
}

int verifyclasses = VERIFY_ALL;
long nbinclasses, sizebinclasses;
ClassClass **binclasses;
bool_t verbose;
struct StrIDhash *nameTypeHash;
struct StrIDhash *stringHash;

void *allocClassClass()
{
    ClassClass *cb = sysCalloc(1, sizeof(ClassClass));
    Classjava_lang_Class *ucb = sysCalloc(1, sizeof(Classjava_lang_Class));
    
    cb->obj = ucb;
    ucb->HandleToSelf = cb;
    return cb;
}

void DumpThreads() {}
