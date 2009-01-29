/*
 * @(#)JNIHeader.java	1.21 06/10/22
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
import jcc.Util;
import util.*;
import components.*;
import vm.*;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

public class JNIHeader extends HeaderDump{

    public JNIHeader( ){
	super( '_' );
    }

    class ParamTypeGenerator extends util.SignatureIterator {
	String	returnType;
	Vector  paramTypes = new Vector();

	ParamTypeGenerator( String s ){ super( s ); }

	private void put( String typestring ){
	    if ( isReturnType ) returnType = typestring;
	    else paramTypes.addElement( typestring );
	}

	public void do_boolean(){ put("jboolean"); }
	public void do_byte(){    put( "jbyte"); }
	public void do_short(){   put( "jshort" ); }
	public void do_char(){    put( "jchar" ); }
	public void do_int(){     put( "jint" ); }
	public void do_long(){    put( "jlong" ); }
	public void do_float(){   put( "jfloat" ); }
	public void do_double(){  put( "jdouble" ); }
	public void do_void(){    put( "void" ); }

	public void do_array( int depth, int start, int end ){
	    if ( depth > 1 ){
		put("jobjectArray");
	    }else switch (sig.charAt(start)){
		case Const.SIGC_BYTE:	put( "jbyteArray" );
					break;
		case Const.SIGC_CHAR:	put( "jcharArray" );
					break;
		case Const.SIGC_SHORT:	put( "jshortArray" );
					break;
		case Const.SIGC_BOOLEAN:put( "jbooleanArray" );
					break;
		case Const.SIGC_INT:	put( "jintArray" );
					break;
		case Const.SIGC_LONG:	put( "jlongArray" );
					break;
		case Const.SIGC_FLOAT:	put( "jfloatArray" );
					break;
		case Const.SIGC_DOUBLE:	put( "jdoubleArray" );
					break;
		case Const.SIGC_CLASS:
		case Const.SIGC_ARRAY:	put( "jobjectArray" );
					break;
		default:		put( "void *" );
					break;
	    }
	}

	public void do_object( int start, int end ){
	    String name = className(start,end);
	    if ( name.equals("java/lang/String") ){
		put( "jstring");
	    }else if ( name.equals("java/lang/Class") ){
		put( "jclass");
	    }else if ( isThrowable( name ) ){
		put( "jthrowable");
	    } else {
		put( "jobject");
	    }
	}

    }

    boolean
    isThrowable( String classname ){
	ClassInfo c = ClassTable.lookupClass(classname);
	if ( c == null ){
	    System.err.println(Localizer.getString("jniheader.cannot_find_class", classname));
	    return false;
	}
	while ( c != null ){
	    if ( c.className.equals("java/lang/Throwable"))
		return true;
	    c = c.superClassInfo;
	}
	return false;
    }

    private boolean
    generateNatives( MethodInfo methods[], boolean cplusplusguard ){
	if ( methods == null ) return false;
	boolean anyMethods = false;
	for ( int i =0; i < methods.length; i++ ){
	    MethodInfo m = methods[i];
	    if ( (m.access&Const.ACC_NATIVE) == 0 ) continue;
	    if ( cplusplusguard && ! anyMethods ){
		o.println("#ifdef __cplusplus\nextern \"C\"{\n#endif");
		anyMethods = true;
	    }
	    String methodsig = m.type.string;
	    o.println("/*");
	    o.println(" * Class:	"+className);
	    o.println(" * Method:	"+m.name.string);
	    o.println(" * Signature:	"+methodsig);
	    o.println(" */");
	    ParamTypeGenerator ptg;
	    try {
		ptg = new ParamTypeGenerator( methodsig );
		ptg.iterate();
	    } catch ( Exception e ){
		e.printStackTrace();
		return anyMethods;
	    }
	    //
	    // decide whether to use long name or short.
	    // we use the long name only if we find another
	    // native method with the same short name.
	    //
	    String nameParams = null;
	    for ( int j =0; j < methods.length; j++ ){
		MethodInfo other = methods[j];
		if ( (other.access&Const.ACC_NATIVE) == 0 ) continue;
		if ( other == m ) continue; // don't compare with self!
		if ( other.name.string.equals( m.name.string ) ){
		    nameParams = methodsig;
		    break;
		}
	    }
	    // print return type and name.
	    o.print("JNIEXPORT ");
	    o.print( ptg.returnType);
	    o.print(" JNICALL ");
	    o.println( Util.convertToJNIName( className, m.name.string, nameParams ) );
	    // print parameter list
	    o.print("  (JNIEnv *,");
	    Enumeration tlist = ptg.paramTypes.elements();
	    // implicit first parameter of "this"
	    // or class pointer
	    if ( m.isStaticMember() ){
		o.print(" jclass");
	    } else {
		o.print(" jobject");
	    }
	    while (tlist.hasMoreElements()){
		o.print(", ");
		o.print( tlist.nextElement() );
	    }
	    o.println(");\n");
	}
	if ( cplusplusguard && anyMethods ){
	    o.println("#ifdef __cplusplus\n}\n#endif");
	    return true;
	}
	return false;
    }

    private void prolog(){
	o.println("/* DO NOT EDIT THIS FILE - it is machine generated */\n" +
	    "#include \"javavm/export/jni.h\"");
	o.println("/* Header for class "+className+" */\n");
	String includeName = "_CVM_JNI_"+strsub(className,CDelim);
	o.println("#ifndef "+includeName);
	o.println("#define "+includeName);
    }
    private void epilog(){
	o.println("#endif");
    }

    synchronized public boolean
    dumpHeader( ClassInfo c, PrintStream outfile ){
	boolean didWork;
	o = outfile;
	className = c.className;
	prolog();
	didWork = generateConsts( Util.convertToClassName( c.className ), c.fields );
	didWork |= generateNatives( c.methods, true );
	epilog();
	return didWork;
    }

    synchronized public boolean
    dumpExternals( ClassInfo c, PrintStream outfile ){
	boolean didWork;
	o = outfile;
	className = c.className;
	didWork = generateNatives( c.methods, false );
	return didWork;
    }
}
