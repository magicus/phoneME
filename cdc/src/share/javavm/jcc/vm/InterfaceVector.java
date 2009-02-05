/*
 * @(#)InterfaceVector.java	1.11 06/10/10
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

package vm;
import components.ClassInfo;
/*
 * A class's InterfaceMethodTable is an array of InterfaceVector,
 * giving the correspondence between the methods of the interface and the
 * methods of the containing class.
 * This could be implemented as an array-of-short, except that we
 * want to do sharing at runtime, so must tag each such array with an "owner",
 * for naming purposes.
 */

public class
InterfaceVector {
    public ClassClass	parent;
    public ClassInfo	intf;
    public short	v[];
    public boolean	generated; // for use of output writer.
    public int		offset;    // for use of output writer.

    public InterfaceVector( ClassClass p, ClassInfo i, short vec[] ){
	parent = p;
	intf = i;
	v = vec;
	generated = false;
	offset = -1; // clearly an invalid value.
    }
}
