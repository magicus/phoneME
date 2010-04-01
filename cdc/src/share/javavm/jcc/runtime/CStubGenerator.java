/*
 * @(#)CStubGenerator.java	1.15 06/10/10
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

package runtime;
import consts.Const;
import util.*;
import components.*;
import vm.*;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

public class CStubGenerator{
    char		CDelim = '_';
    String		exportPrefix = "";
    boolean		tracemode;
    String		outfileName;
    final static char	DIR_DELIM = '/';
    final static char	INNER_DELIM = '$';
    PrintStream		o;

    public CStubGenerator( boolean dotrace, PrintStream outfile ){
	tracemode = dotrace;
	o = outfile;
    }

    // this data is really per-header.
    // it is guarded by synchronous method access
    String      className;

    static String
    strsub( String src, char substitute ){
	return src.replace( DIR_DELIM, substitute).replace(INNER_DELIM, substitute );
    }

    class StubGenerator extends util.SignatureIterator{
	public String funcName;
	public boolean tracemode;
	public boolean isStatic;
	public int    argn;
	public String retTypeWord = null;
	public String params;
	public String proto	  = "";
	public String protoThis;
	public String result	  = "";
	String quotedClassFunc;
	String methodName;

	public StubGenerator( String name, boolean isStaticMember, String initialProto, String sig, boolean tm, String mname ){
	    super( sig );
	    funcName  = name;
	    isStatic  = isStaticMember;
	    if ( isStaticMember ){
		params    = "NULL";
		argn      = 0;
	    } else {
		params    = "_P_[0].p";
		argn      = 1;
	    }
	    protoThis = initialProto;
	    tracemode = tm;
	    methodName = mname;
	}

	private void prefix( String returntype ){
	    result += "	extern "+returntype+" "+funcName+"("+protoThis+proto+");\n";
	    if ( tracemode ){
		quotedClassFunc = "\""+className+"\",\""+methodName+"\"";
		String thisParam = isStatic ? ",%s,\"NULL\"" : ",0x%08x,_P_[0].p";
		result += "	ENTER_STUB("+quotedClassFunc+thisParam+",\""+proto+"\");\n";
	    }
	}
    
	private void suffix( String fmt, String val, String rtn ){
	    if ( tracemode ){
		if ( fmt == null )
		    result += "	EXIT_VOID_STUB("+quotedClassFunc+");\n";
		else
		    result += "	EXIT_STUB("+quotedClassFunc+",\""+fmt+"\","+val+");\n";
	    }
	    result += rtn;
	}

	public void do_scalar( char t ){
	    // default scalar types: int, char, byte, short
	    if (  isReturnType ){
		prefix( "long" );
		result += "	_P_[0].i = "+funcName+"("+params+");\n";
		suffix( "%d", "_P_[0].i", "\treturn _P_ + 1;\n");
	    } else {
		params += ", _P_["+argn+"].i";
		proto  += ",int";
		argn += 1;
	    }
	}

	public void do_float(){
	    if ( isReturnType ){
		prefix( "float" );
		result += "	_P_[0].f = "+funcName+"("+params+");\n";
		suffix( "%f", "_P_[0].f", "\treturn _P_ + 1;\n");
	    } else {
		params += ", _P_["+argn+"].f";
		proto  += ",float";
		argn += 1;
	    }
	}

	public void do_double(){
	    if ( isReturnType ){
		result += "	Java8 _tval;\n";
		prefix( "double" );
		result += "	SET_DOUBLE(_tval, _P_, "+funcName+"("+params+"));\n";
		suffix( null, null, "\treturn _P_ + 2;\n" );
	    } else {
		result += "	Java8 _t"+argn+";\n";
		params += ",GET_DOUBLE(_t"+argn+", _P_+"+argn+") ";
		proto  += ",double";
		argn += 2;
	    }
	}

	public void do_long(){
	    if ( isReturnType ){
		result += "	Java8 _tval;\n";
		prefix ("int64_t");
		result += "	SET_INT64(_tval, _P_, "+funcName+"("+params+"));\n";
		suffix( null, null, "\treturn _P_ + 2;\n" );
	    } else {
		result += "	Java8 _t"+argn+";\n";
		params += ",GET_INT64(_t"+argn+", _P_+"+argn+")";
		proto  += ",int64_t";
		argn += 2;
	    }
	}

	public void do_void(){
	    if ( isReturnType ){
		prefix("void");
		result += "	(void) "+funcName+"("+params+");\n";
		suffix( null, null, "\treturn _P_;\n" );
	    } else {
		System.out.println(Localizer.getString("cstubgenerator.void_parameter_type"));
	    }
	}

	public void do_boolean(){
	    if ( isReturnType ){
		prefix("long");
		result += "	_P_[0].i = "+funcName+"("+params+")? TRUE : FALSE;\n";
		suffix( "%d", "_P_[0].i", "\treturn _P_ + 1;\n");
	    } else {
		params += ", _P_["+argn+"].i";
		proto  += ",int";
		argn += 1;
	    }
	}

	public void do_object( int startname, int endname ){
	    if ( isReturnType ){
		prefix ("void*");
		result += "	_P_[0].p = "+funcName+"("+params+");\n";
		suffix("0x%08x", "_P_[0].p", "\treturn _P_ + 1;\n");
	    } else {
		params += ", _P_["+argn+"].p";
		proto  += ",void *";
		argn += 1;
	    }
	}

	public void do_array( int depth, int startname, int endname ){
	    if ( isReturnType ){
		prefix ("void*");
		result += "	_P_[0].p = "+funcName+"("+params+");\n";
		suffix("0x%08x", "_P_[0].p", "\treturn _P_ + 1;\n");
	    } else {
		params += ", _P_["+argn+"].p";
		proto  += ",void *";
		argn += 1;
	    }
	}
    }

    private boolean printStub( MethodInfo m ){
	if ( (m.access&Const.ACC_NATIVE) == 0 )
	    return false; // we're here in error.
	String  mName    = strsub( m.name.string, CDelim );
	String  funcName = className+"_"+mName;
	String  stubName = "Java_"+funcName+"_stub";

	StubGenerator g = new StubGenerator( funcName, m.isStaticMember(), "void *", m.type.string, tracemode, mName );
	try {
	    g.iterate_parameters();
	    g.iterate_returntype();
	} catch ( DataFormatException e ){
	    e.printStackTrace();
	    return false;
	}

	o.println(exportPrefix+"stack_item *"+stubName+"(stack_item *_P_,struct execenv *_EE_) {");
	o.print( g.result );
	o.println("}");
	return true;
    }

    private void prolog(){
	o.println("/* Stubs for class "+className+" */");
    }
    private void epilog(){
    }

    void fileProlog(){
	o.println("/* DO NOT EDIT THIS FILE - it is machine generated */");
	o.println("#include <StubPreamble.h>");
	if ( tracemode ){
	    o.println("#include <threads.h>\n");
	    o.println("#ifndef ENTER_STUB");
	    o.println("#define ENTER_STUB(n,fcn,fmt,s,p)\\");
	    o.println("	printf(\"0x%08x: %s_%s(\" #fmt \"%s)\\n\", threadSelf(), n, fcn, s, p);");
	    o.println("#endif");
	    o.println("#ifndef EXIT_STUB");
	    o.println("#define EXIT_STUB(n,f,fmt,r)\\");
	    o.println("	printf(\"0x%08x: %s_%s returned \" #fmt \"\\n\", threadSelf(), n, f, r);");
	    o.println("#define EXIT_VOID_STUB(n,f)\\");
	    o.println("	printf(\"0x%08x: %s_%s returned\\n\", threadSelf(), n, f);");
	    o.println("#endif");
	}
    }

    private
    void dumpStubs( ClassInfo c ){
	className = strsub( c.className, CDelim );
	if ( (c.methods == null) || (c.methods.length==0 ))
		return;
	prolog();
	MethodInfo t[] = c.methods;
	for ( int i = 0; i < t.length; i++ ){
	    if ( (t[i].access&Const.ACC_NATIVE) != 0 )
		printStub( t[i] );
	}
	epilog();
    }

    synchronized public void
    writeStubs( ClassInfo c[], int nclasses, ClassnameFilterList nativeTypes ){
	fileProlog();
	for ( int i = 0; i < nclasses; i++ ){
	    String[] types = nativeTypes.getTypes( c[i].className );
	    for ( int j = 0; j < types.length; ++j ) {
		String type = types[j];
		if ( type.substring(0, 3).equals("JDK") ){
		    dumpStubs( c[i] );
		    break;
		}
	    }
	}
    }

}
