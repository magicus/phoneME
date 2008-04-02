/*
 *   
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
 */

#include "incls/_precompiled.incl"

extern "C" {

extern const unsigned char oopmap_Empty[];
extern const unsigned char oopmap_ConstantPool[];
extern const unsigned char oopmap_Method[];
extern const unsigned char oopmap_InstanceClass[];
extern const unsigned char oopmap_ArrayClass[];
extern const unsigned char oopmap_ObjNear[];
extern const unsigned char oopmap_FarClass[];
extern const unsigned char oopmap_CompiledMethod[];
extern const unsigned char oopmap_EntryActivation[];
extern const unsigned char oopmap_ClassInfo[];
extern const unsigned char oopmap_StackmapList[];
extern const unsigned char oopmap_TaskMirror[];
extern const unsigned char oopmap_Boundary[];

extern const unsigned char omit_frame_table[];

const unsigned char oopmap_Empty[]            = { 0 };
const unsigned char oopmap_ConstantPool[]     = { 0 };
const unsigned char oopmap_Method[]           = { 0 };
const unsigned char oopmap_InstanceClass[]    = { 0 };
const unsigned char oopmap_ArrayClass[]       = { 0 };
const unsigned char oopmap_ObjNear[]          = { 0 };
const unsigned char oopmap_FarClass[]         = { 0 };
const unsigned char oopmap_CompiledMethod[]   = { 0 };
const unsigned char oopmap_EntryActivation[]  = { 0 };
const unsigned char oopmap_ClassInfo[]        = { 0 };
const unsigned char oopmap_StackmapList[]     = { 0 };
const unsigned char oopmap_TaskMirror[]       = { 0 };
const unsigned char oopmap_Boundary[]         = { 0 };

const unsigned char omit_frame_table[] = { 0 };

void loopgen_check_oopmaps() {}

}
