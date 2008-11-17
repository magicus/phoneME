/*
 * @(#)Const.java	1.30 06/10/10
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

/*
 * Constants used by the Java linker which are not exported in any
 * direct way from the Java runtime or compiler.
 * These include the opcode values of the quick bytecodes, the instruction
 * lengths of same, and the size of certain data structures in the .class
 * file format.
 * These have been split to more appopriate places, but are collected here
 * for the convenience of old code that doesn't know which parts are which.
 * util.ClassFileConst has constants from the Red book, such as access flags,
 *	type numbers and signature characters.
 * vm.VMConst has VM-specific things, such as internal flag bits used in
 *	romizer output
 * OpcodeConst is a file generated from the current opcode list. This is
 *	pretty stable in JVM, very unstable in CVM.
 */
package consts;
public interface Const extends util.ClassFileConst, vm.VMConst, opcodeconsts.OpcodeConst
{
}
