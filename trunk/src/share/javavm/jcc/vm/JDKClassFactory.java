/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package vm;
import consts.Const;
import components.ConstantPool;
import components.ClassInfo;
import java.util.Hashtable;

public class JDKClassFactory implements VMClassFactory{
    
    public ClassClass newVMClass( ClassInfo c ){
	return new JDKClass( c );
    }

    private static boolean
    setType( String name, int value ){
	ClassInfo ci = ClassInfo.lookupClass( name );
	ClassClass cc;
	if ( (ci == null) || ( ( cc = ci.vmClass ) == null) ) return false;
	((JDKClass)cc).typeCode = value;
	return true;


    }

    public void
    setTypes(){
	setType( /*NOI18N*/"java/lang/Void", Const.T_VOID );
	setType( /*NOI18N*/"java/lang/Boolean", Const.T_BOOLEAN );
	setType( /*NOI18N*/"java/lang/Character", Const.T_CHAR );
	setType( /*NOI18N*/"java/lang/Byte", Const.T_BYTE );
	setType( /*NOI18N*/"java/lang/Short", Const.T_SHORT );
	setType( /*NOI18N*/"java/lang/Integer", Const.T_INT );
	setType( /*NOI18N*/"java/lang/Long", Const.T_LONG );
	setType( /*NOI18N*/"java/lang/Float", Const.T_FLOAT );
	setType( /*NOI18N*/"java/lang/Double", Const.T_DOUBLE );
    }

    public ConstantPool 
    makeResolvable(ConstantPool cp, Hashtable missingClasses) {
	//
        // we use a LocalConstantPool to manage the pool we're re-building.
        // Because order matters, we collect the new entries to add at end.
        //
        System.err.println(Localizer.getString("classclass.warning_it_has_an_impure_shared_constant_pool"));
	return JDKClass.makeResolvable(cp.getConstants(), missingClasses);
    }
}
