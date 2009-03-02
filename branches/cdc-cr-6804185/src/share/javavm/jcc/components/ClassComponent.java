/*
 * @(#)ClassComponent.java	1.9 06/10/10
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

package components;
import java.io.DataOutput;
import java.io.IOException;

/*
 * An abstract class for representing components of a class
 * This include any field, method, or "constant".
 */

public
abstract class ClassComponent
{
    /**
     * Indicates if flatten() has been called yet i.e. all internal symbollic
     * links has been flattened.  To be flattened means that the symbollic
     * links do not need to indirect through the constant pool to get to a
     * terminal CP entry.  UTF8 and constant values are terminal CP entries
     * for this purpose.
     */
    public boolean isFlat = false;

    abstract public void write( DataOutput o ) throws IOException;

    /**
     * Flattend all internal symbollic links.  Flattening here means that the
     * symbollic links are resolved to the extent that they no longer need to
     * indirect through the constant pool to get to a terminal constant.
     * UTF8 and constant values are terminal constants.  For example, a class
     * constant is defined by a CP index which point to a UTF8 string.  A
     * flattened class constant will refer to the UTF8 string directly instead
     * of needing to index into the constant pool to get to it.
     */
    public void flatten(ConstantPool table) {
        // by default, just note that we're flattened.
	isFlat = true;
    }
}
