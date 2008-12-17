/*
 * @(#)compress.h	1.6 06/10/10
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

#ifndef __COMPACTH__
#define __COMPACTH__

/*
 * This file declares no classes
 * This file declares routines for massaging
 * statemap and transition tables,
 * for the purpose of producing more compact output.
 *
 * Towards this end, we iteratively remove redundant
 * rows & columns in transition tables, and redundant
 * states.
 * For our purposes, two states are the same ( and thus one
 * of them is redundant ) if they have all the same transitions.
 * This can be true even if they represent different itemsets and
 * thus different partial matches.
 */

int
compress_output(); // returns # of states deleted.

#endif
