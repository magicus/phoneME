/*
 * @(#)LocalVariableTableEntry.java	1.9 06/10/10
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
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

/*
 * Classes compiled with -g have line number attributes on their
 * code. These give the correspondence between Java bytecode relative PC
 * and source lines (when used with the filename, too!)
 */
public
class LocalVariableTableEntry {
    public int pc0;
    public int length;
    public UnicodeConstant name;
    public UnicodeConstant sig;
    public int slot;

    public static final int size = 10; // bytes in class files

    LocalVariableTableEntry( int p, int l, UnicodeConstant n,
			     UnicodeConstant si, int sl ){
	pc0 = p;
	length = l;
	name = n;
	sig = si;
	slot = sl;
    }

    public void write( DataOutput o ) throws IOException {
	o.writeShort( pc0 );
	o.writeShort( length );
	o.writeShort( name.index );
	o.writeShort( sig.index );
	o.writeShort( slot );
    }

    /*
     * ensure that name, sig are not in constant pool.
     */
    public void
    validate(){
	name.validate();
	sig.validate();
    }
}

