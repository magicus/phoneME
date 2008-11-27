/*
 * @(#)packages.h	1.10 06/10/10
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


#ifndef _INCLUDED_PACKAGES_H
#define _INCLUDED_PACKAGES_H

#define CVM_PKGINFO_HASHSIZE 31	    /* Number of buckets */

/* Hash table used to keep track of loaded packages */
typedef struct CVMPackage CVMPackage;
struct CVMPackage {
    char *packageName;	    /* Package name */
    char *filename;	    /* Directory or JAR file loaded from */
    CVMPackage* next;       /* Next entry in hash table */
};

/*
 * Returns the package file name corresponding to the specified package
 * or class name, or null if not found.
 */
extern char*
CVMpackagesGetEntry(const char* name);

/*
 * Adds a new package entry for the specified class or package name and
 * corresponding directory or jar file name. If there was already an existing
 * package entry then it is replaced. Returns the previous entry or null if
 * none.
 */
extern CVMBool
CVMpackagesAddEntry(const char *name, const char *filename);

/*
 * Destroys package information on VM exit
 */
extern void
CVMpackagesDestroy();

#endif /* _INCLUDED_PACKAGES_H */
