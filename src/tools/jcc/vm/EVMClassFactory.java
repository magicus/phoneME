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

package vm;
import jcc.Const;
import components.ClassInfo;

public class EVMClassFactory implements VMClassFactory{
    
    public ClassClass newVMClass( ClassInfo c ){
	return new EVMClass( c );
    }

    private static boolean
    setType( String name, int value ){
	ClassInfo ci = ClassInfo.lookupClass( name );
	ClassClass cc;
	if ( (ci == null) || ( ( cc = ci.vmClass ) == null) ){
	    return false;
	}
	((EVMClass)cc).typeCode = value;
	return true;

    }

    public void
    setTypes(){
	setType( "void", Const.T_VOID );
	setType( "boolean", Const.T_BOOLEAN );
	setType( "char", Const.T_CHAR );
	setType( "byte", Const.T_BYTE );
	setType( "short", Const.T_SHORT );
	setType( "int", Const.T_INT );
	setType( "long", Const.T_LONG );
	setType( "float", Const.T_FLOAT );
	setType( "double", Const.T_DOUBLE );
    }
}
