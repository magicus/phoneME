/*
 * @(#)ProxyGenerator.java	1.12 06/10/10
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

package sun.misc;

import java.lang.reflect.*;
import java.io.*;
import java.util.*;

import sun.tools.java.RuntimeConstants;
import sun.security.action.GetBooleanAction;

/**
 * ProxyGenerator contains the code to generate a dynamic proxy class
 * for the java.lang.reflect.Proxy API.
 *
 * The external interfaces to ProxyGenerator is the static
 * "generateProxyClass" method.
 *
 * @author	Peter Jones
 * @version	1.3, 00/02/02
 * @since	JDK1.3
 */
public class ProxyGenerator {
    /*
     * In the comments below, "JVMS" refers to The Java Virtual Machine
     * Specification Second Edition and "JLS" refers to the original
     * version of The Java Language Specification, unless otherwise
     * specified.
     */

    /*
     * Note that this class imports sun.tools.java.RuntimeConstants and
     * references many final static primitive fields of that interface.
     * By JLS section 13.4.8, the compiler should inline all of these
     * references, so their presence should not require the loading of
     * RuntimeConstants at runtime when ProxyGenerator is linked.  This
     * non-requirement is important because ProxyGenerator is intended
     * to be bundled with the JRE, but classes in the sun.tools
     * hierarchy, such as RuntimeConstants, are not.
     *
     * The Java compiler does add a CONSTANT_Class entry in the constant
     * pool of this class for "sun/tools/java/RuntimeConstants".  The
     * evaluation of bugid 4162387 seems to imply that this is for the
     * compiler's implementation of the "-Xdepend" option.  This
     * CONSTANT_Class entry may, however, confuse tools which use such
     * entries to compute runtime class dependencies or virtual machine
     * implementations which use them to effect eager class resolution.
     */

    /** name of the superclass of proxy classes */
    private final static String superclassName = "java/lang/reflect/Proxy";

    /** name of field for storing a proxy instance's invocation handler */
    private final static String handlerFieldName = "h";

    /** debugging flag for saving generated class files */
    private final static boolean saveGeneratedFiles =
	((Boolean) java.security.AccessController.doPrivileged(
	    new GetBooleanAction(
		"sun.misc.ProxyGenerator.saveGeneratedFiles"))).booleanValue();

    /**
     * Generate a proxy class given a name and a list of proxy interfaces.
     */
    public static byte[] generateProxyClass(final String name,
					    Class[] interfaces)
    {
	ProxyGenerator gen = new ProxyGenerator(name, interfaces);
	final byte[] classFile = gen.generateClassFile();

	if (saveGeneratedFiles) {
	    java.security.AccessController.doPrivileged(
	    new java.security.PrivilegedAction() {
		public Object run() {
		    try {
			FileOutputStream file =
			    new FileOutputStream(dotToSlash(name) + ".class");
			file.write(classFile);
			file.close();
			return null;
		    } catch (IOException e) {
			throw new InternalError(
			    "I/O exception saving generated file: " + e);
		    }
		}
	    });
	}

	return classFile;
    }

    /* preloaded Method objects for methods in java.lang.Object */
    private static Method hashCodeMethod;
    private static Method equalsMethod;
    private static Method toStringMethod;
    static {
	try {
	    hashCodeMethod = Object.class.getMethod("hashCode", null);
	    equalsMethod =
		Object.class.getMethod("equals", new Class[] { Object.class });
	    toStringMethod = Object.class.getMethod("toString", null);
        } catch (NoSuchMethodException e) {
	    throw new NoSuchMethodError(e.getMessage());
	}
    }

    /** name of proxy class */
    private String className;

    /** proxy interfaces */ 
    private Class[] interfaces;

    /** constant pool of class being generated */
    private ConstantPool cp = new ConstantPool();

    /** FieldInfo struct for each field of generated class */
    private List fields = new ArrayList();

    /** MethodInfo struct for each method of generated class */
    private List methods = new ArrayList();

    /**
     * for each method to be proxied, maps method name and parameter
     * descriptor to ProxyMethod object
     */
    private Map proxyMethods = new HashMap(11);

    /**
     * Construct a ProxyGenerator to generate a proxy class with the
     * specified name and for the given interfaces.
     *
     * A ProxyGenerator object contains the state for the ongoing
     * generation of a particular proxy class.
     */
    private ProxyGenerator(String className, Class[] interfaces) {
	this.className = className;
	this.interfaces = interfaces;
    }

    /**
     * Generate a class file for the proxy class.  This method drives the
     * class file generation process.
     */
    private byte[] generateClassFile() {

	/* ============================================================
	 * Step 1: Assemble ProxyMethod objects for all methods to
	 * generate proxy dispatching code for.
	 */

	/*
	 * Record that proxy methods are needed for the hashCode, equals,
	 * and toString methods of java.lang.Object.  This is done before
	 * the methods from the proxy interfaces so that the methods from
	 * java.lang.Object take precedence over duplicate methods in the
	 * proxy interfaces.
	 */
	addProxyMethod(hashCodeMethod, Object.class);
	addProxyMethod(equalsMethod, Object.class);
	addProxyMethod(toStringMethod, Object.class);

	/*
	 * Now record all of the methods from the proxy interfaces, giving
	 * earlier interfaces precedence over later ones with duplicate
	 * methods.
	 */
	for (int i = 0; i < interfaces.length; i++) {
	    Method[] methods = interfaces[i].getMethods();
	    for (int j = 0; j < methods.length; j++) {
		addProxyMethod(methods[j], interfaces[i]);
	    }
	}

	/* ============================================================
	 * Step 2: Assemble FieldInfo and MethodInfo structs for all of
	 * fields and methods in the class we are generating.
	 */
	try {
//	    fields.add(new FieldInfo(
//		handlerFieldName, "Ljava/lang/reflect/InvocationHandler;",
//		RuntimeConstants.ACC_PRIVATE | RuntimeConstants.ACC_FINAL));

	    methods.add(generateConstructor());

	    for (Iterator iter = proxyMethods.values().iterator();
		 iter.hasNext();)
	    {
		ProxyMethod pm = (ProxyMethod) iter.next();

		// add static field for method's Method object
		fields.add(new FieldInfo(pm.methodFieldName,
		    "Ljava/lang/reflect/Method;",
		    RuntimeConstants.ACC_PRIVATE |
			RuntimeConstants.ACC_STATIC));

		// generate code for proxy method and add it
		methods.add(pm.generateMethod());
	    }

	    methods.add(generateStaticInitializer());

	} catch (IOException e) {
	    throw new InternalError("unexpected I/O Exception");
	}

	/* ============================================================
	 * Step 3: Write the final class file.
	 */

	/*
	 * Make sure that constant pool indexes are reserved for the
	 * following items before starting to write the final class file.
	 */
	cp.getClass(dotToSlash(className));
	cp.getClass(superclassName);
	for (int i = 0; i < interfaces.length; i++) {
	    cp.getClass(dotToSlash(interfaces[i].getName()));
	}

	/*
	 * Disallow new constant pool additions beyond this point, since
	 * we are about to write the final constant pool table.
	 */
	cp.setReadOnly();

	ByteArrayOutputStream bout = new ByteArrayOutputStream();
	DataOutputStream dout = new DataOutputStream(bout);

	try {
	    /*
	     * Write all the items of the "ClassFile" structure.
	     * See JVMS section 4.1.
	     */
					// u4 magic;
	    dout.writeInt(RuntimeConstants.JAVA_MAGIC);
					// u2 major_version;
	    dout.writeShort(RuntimeConstants.JAVA_DEFAULT_MINOR_VERSION);
					// u2 minor_version;
	    dout.writeShort(RuntimeConstants.JAVA_DEFAULT_VERSION);

	    cp.write(dout);		// (write constant pool)

					// u2 access_flags;
	    dout.writeShort(RuntimeConstants.ACC_PUBLIC |
			    RuntimeConstants.ACC_FINAL |
			    RuntimeConstants.ACC_SUPER);
					// u2 this_class;
	    dout.writeShort(cp.getClass(dotToSlash(className)));
					// u2 super_class;
	    dout.writeShort(cp.getClass(superclassName));

					// u2 interfaces_count;
	    dout.writeShort(interfaces.length);
					// u2 interfaces[interfaces_count];
	    for (int i = 0; i < interfaces.length; i++) {
		dout.writeShort(cp.getClass(
		    dotToSlash(interfaces[i].getName())));
	    }

					// u2 fields_count;
	    dout.writeShort(fields.size());
					// field_info fields[fields_count];
	    for (Iterator iter = fields.iterator(); iter.hasNext();) {
		FieldInfo f = (FieldInfo) iter.next();
		f.write(dout);
	    }

					// u2 methods_count;
	    dout.writeShort(methods.size());
					// method_info methods[methods_count];
	    for (Iterator iter = methods.iterator(); iter.hasNext();) {
		MethodInfo m = (MethodInfo) iter.next();
		m.write(dout);
	    }

	   				 // u2 attributes_count;
	    dout.writeShort(0);	// (no ClassFile attributes for proxy classes)

	} catch (IOException e) {
	    throw new InternalError("unexpected I/O Exception");
	}

	return bout.toByteArray();
    }

    /**
     * Add another method to be proxied, either by creating a new ProxyMethod
     * object or augmenting an old one for a duplicate method.
     *
     * "fromClass" indicates the proxy interface that the method was found
     * through, which may be different from (a subinterface of) the method's
     * "declaring class".  Note that the first Method object passed for a
     * given name and parameter types identifies the Method object (and thus
     * the declaring class) that will be passed to the invocation handler's
     * "invoke" method for a given set of duplicate methods.
     */
    private void addProxyMethod(Method m, Class fromClass) {
	String name = m.getName();
	Class[] parameterTypes = m.getParameterTypes();
	Class returnType = m.getReturnType();
	Class[] exceptionTypes = m.getExceptionTypes();

	String key = name + getParameterDescriptors(parameterTypes);
	ProxyMethod pm = (ProxyMethod) proxyMethods.get(key);
	if (pm != null) {
	    /*
	     * If a proxy method with the same name and parameter types has
	     * already been added, verify that it has the same return type...
	     */
	    if (returnType != pm.returnType) {
		throw new IllegalArgumentException(
		    "methods with same name and parameter " +
		    "signature but different return type in " +
		    pm.fromClass + " and " + fromClass +
		    ": " + key);
	    }
	    /*
	     * ...and compute the greatest common set of exceptions that can
	     * thrown by the proxy method compatibly with both inherited
	     * methods.
	     */
	    List legalExceptions = new ArrayList();
	    collectCompatibleTypes(
		exceptionTypes, pm.exceptionTypes, legalExceptions);
	    collectCompatibleTypes(
		pm.exceptionTypes, exceptionTypes, legalExceptions);
	    pm.exceptionTypes = new Class[legalExceptions.size()];
	    pm.exceptionTypes =
		(Class[]) legalExceptions.toArray(pm.exceptionTypes);
	} else {
	    pm = new ProxyMethod(name, parameterTypes, returnType,
		exceptionTypes, fromClass, "m" + proxyMethods.size());
	    proxyMethods.put(key, pm);
	}
    }

    /**
     * A FieldInfo object contains information about a particular field
     * in the class being generated.  The class mirrors the data items of
     * the "field_info" structure of the class file format (see JVMS 4.5).
     */
    private class FieldInfo {
	public int accessFlags;
	public String name;
	public String descriptor;

	public FieldInfo(String name, String descriptor, int accessFlags) {
	    this.name = name;
	    this.descriptor = descriptor;
	    this.accessFlags = accessFlags;

	    /*
	     * Make sure that constant pool indexes are reserved for the
	     * following items before starting to write the final class file.
	     */
	    cp.getUtf8(name);
	    cp.getUtf8(descriptor);
	}

	public void write(DataOutputStream out) throws IOException {
	    /*
	     * Write all the items of the "field_info" structure.
	     * See JVMS section 4.5.
	     */
					// u2 access_flags;
	    out.writeShort(accessFlags);
					// u2 name_index;
	    out.writeShort(cp.getUtf8(name));
					// u2 descriptor_index;
	    out.writeShort(cp.getUtf8(descriptor));
					// u2 attributes_count;
	    out.writeShort(0);	// (no field_info attributes for proxy classes)
	}
    }

    /**
     * An ExceptionTableEntry object holds values for the data items of
     * an entry in the "exception_table" item of the "Code" attribute of
     * "method_info" structures (see JVMS 4.7.3).
     */
    private static class ExceptionTableEntry {
	public short startPc;
	public short endPc;
	public short handlerPc;
	public short catchType;

	public ExceptionTableEntry(short startPc, short endPc,
				   short handlerPc, short catchType)
	{
	    this.startPc = startPc;
	    this.endPc = endPc;
	    this.handlerPc = handlerPc;
	    this.catchType = catchType;
	}
    };

    /**
     * A MethodInfo object contains information about a particular method
     * in the class being generated.  This class mirrors the data items of
     * the "method_info" structure of the class file format (see JVMS 4.6).
     */
    private class MethodInfo {
	public int accessFlags;
	public String name;
	public String descriptor;
	public short maxStack;
	public short maxLocals;
	public ByteArrayOutputStream code = new ByteArrayOutputStream();
	public List exceptionTable = new ArrayList();
	public short[] declaredExceptions;

	public MethodInfo(String name, String descriptor, int accessFlags) {
	    this.name = name;
	    this.descriptor = descriptor;
	    this.accessFlags = accessFlags;

	    /*
	     * Make sure that constant pool indexes are reserved for the
	     * following items before starting to write the final class file.
	     */
	    cp.getUtf8(name);
	    cp.getUtf8(descriptor);
	    cp.getUtf8("Code");
	    cp.getUtf8("Exceptions");
	}

	public void write(DataOutputStream out) throws IOException {
	    /*
	     * Write all the items of the "method_info" structure.
	     * See JVMS section 4.6.
	     */
					// u2 access_flags;
	    out.writeShort(accessFlags);
					// u2 name_index;
	    out.writeShort(cp.getUtf8(name));
					// u2 descriptor_index;
	    out.writeShort(cp.getUtf8(descriptor));
					// u2 attributes_count;
	    out.writeShort(2);	// (two method_info attributes:)

	    // Write "Code" attribute. See JVMS section 4.7.3.

					// u2 attribute_name_index;
	    out.writeShort(cp.getUtf8("Code"));
					// u4 attribute_length;
	    out.writeInt(12 + code.size() + 8 * exceptionTable.size());
					// u2 max_stack;
	    out.writeShort(maxStack);
					// u2 max_locals;
	    out.writeShort(maxLocals);
					// u2 code_length;
	    out.writeInt(code.size());
					// u1 code[code_length];
	    code.writeTo(out);
					// u2 exception_table_length;
	    out.writeShort(exceptionTable.size());
	    for (Iterator iter = exceptionTable.iterator(); iter.hasNext();) {
		ExceptionTableEntry e = (ExceptionTableEntry) iter.next();
					// u2 start_pc;
		out.writeShort(e.startPc);
					// u2 end_pc;
		out.writeShort(e.endPc);
					// u2 handler_pc;
		out.writeShort(e.handlerPc);
					// u2 catch_type;
		out.writeShort(e.catchType);
	    }
					// u2 attributes_count;
	    out.writeShort(0);

	    // write "Exceptions" attribute.  See JVMS section 4.7.4.

					// u2 attribute_name_index;
	    out.writeShort(cp.getUtf8("Exceptions"));
					// u4 attributes_length;
	    out.writeInt(2 + 2 * declaredExceptions.length);
					// u2 number_of_exceptions;
	    out.writeShort(declaredExceptions.length);
			// u2 exception_index_table[number_of_exceptions];
	    for (int i = 0; i < declaredExceptions.length; i++) {
		out.writeShort(declaredExceptions[i]);
	    }
	}

    }

    /**
     * A ProxyMethod object represents a proxy method in the proxy class
     * being generated: a method whose implementation will encode and
     * dispatch invocations to the proxy instance's invocation handler.
     */
    private class ProxyMethod {

	public String methodName;
	public Class[] parameterTypes;
	public Class returnType;
	public Class[] exceptionTypes;
	public Class fromClass;
	public String methodFieldName;

	private ProxyMethod(String methodName, Class[] parameterTypes,
			    Class returnType, Class[] exceptionTypes,
			    Class fromClass, String methodFieldName)
	{
	    this.methodName = methodName;
	    this.parameterTypes = parameterTypes;
	    this.returnType = returnType;
	    this.exceptionTypes = exceptionTypes;
	    this.fromClass = fromClass;
	    this.methodFieldName = methodFieldName;
	}

	/**
	 * Return a MethodInfo object for this method, including generating
	 * the code and exception table entry.
	 */
	private MethodInfo generateMethod() throws IOException {
	    String desc = getMethodDescriptor(parameterTypes, returnType);
	    MethodInfo minfo = new MethodInfo(methodName, desc,
		RuntimeConstants.ACC_PUBLIC | RuntimeConstants.ACC_FINAL);

	    int[] parameterSlot = new int[parameterTypes.length];
	    int nextSlot = 1;
	    for (int i = 0; i < parameterSlot.length; i++) {
		parameterSlot[i] = nextSlot;
		nextSlot += getWordsPerType(parameterTypes[i]);
	    }
	    int localSlot0 = nextSlot;
	    short pc, tryBegin = 0, tryEnd;

	    DataOutputStream out = new DataOutputStream(minfo.code);

	    code_aload(0, out);

	    out.writeByte(RuntimeConstants.opc_getfield);
	    out.writeShort(cp.getFieldRef(
//		dotToSlash(className),
		superclassName,
		handlerFieldName, "Ljava/lang/reflect/InvocationHandler;"));

	    code_aload(0, out);

	    out.writeByte(RuntimeConstants.opc_getstatic);
	    out.writeShort(cp.getFieldRef(
		dotToSlash(className),
		methodFieldName, "Ljava/lang/reflect/Method;"));

	    if (parameterTypes.length > 0) {

		code_ipush(parameterTypes.length, out);

		out.writeByte(RuntimeConstants.opc_anewarray);
		out.writeShort(cp.getClass("java/lang/Object"));

		for (int i = 0; i < parameterTypes.length; i++) {

		    out.writeByte(RuntimeConstants.opc_dup);

		    code_ipush(i, out);

		    codeWrapArgument(parameterTypes[i], parameterSlot[i], out);

		    out.writeByte(RuntimeConstants.opc_aastore);
		}
	    } else {

		out.writeByte(RuntimeConstants.opc_aconst_null);
	    }

	    out.writeByte(RuntimeConstants.opc_invokeinterface);
	    out.writeShort(cp.getInterfaceMethodRef(
		"java/lang/reflect/InvocationHandler",
		"invoke",
		"(Ljava/lang/Object;Ljava/lang/reflect/Method;" +
		    "[Ljava/lang/Object;)Ljava/lang/Object;"));
	    out.writeByte(4);
	    out.writeByte(0);

	    if (returnType == void.class) {

		out.writeByte(RuntimeConstants.opc_pop);

		out.writeByte(RuntimeConstants.opc_return);

	    } else {

		codeUnwrapReturnValue(returnType, out);
	    }

	    tryEnd = pc = (short) minfo.code.size();

	    List catchList = computeUniqueCatchList(exceptionTypes);
	    if (catchList.size() > 0) {

		for (Iterator iter = catchList.iterator(); iter.hasNext();) {
		    Class ex = (Class) iter.next();
		    minfo.exceptionTable.add(new ExceptionTableEntry(
			tryBegin, tryEnd, pc,
			cp.getClass(dotToSlash(ex.getName()))));
		}

		out.writeByte(RuntimeConstants.opc_athrow);

		pc = (short) minfo.code.size();

		minfo.exceptionTable.add(new ExceptionTableEntry(
		    tryBegin, tryEnd, pc, cp.getClass("java/lang/Throwable")));

		code_astore(localSlot0, out);

		out.writeByte(RuntimeConstants.opc_new);
		out.writeShort(cp.getClass(
		    "java/lang/reflect/UndeclaredThrowableException"));

		out.writeByte(RuntimeConstants.opc_dup);

		code_aload(localSlot0, out);

		out.writeByte(RuntimeConstants.opc_invokespecial);

		out.writeShort(cp.getMethodRef(
		    "java/lang/reflect/UndeclaredThrowableException",
		    "<init>", "(Ljava/lang/Throwable;)V"));

		out.writeByte(RuntimeConstants.opc_athrow);
	    }

	    minfo.maxStack = 10;
	    minfo.maxLocals = (short) (localSlot0 + 1);
	    minfo.declaredExceptions = new short[exceptionTypes.length];
	    for (int i = 0; i < exceptionTypes.length; i++) {
		minfo.declaredExceptions[i] = cp.getClass(
		    dotToSlash(exceptionTypes[i].getName()));
	    }

	    return minfo;
	}

	/**
	 * Generate code for wrapping a parameter of the given type and whose
	 * value can be found at the specified local variable index to be
	 * passed to the invocation handler's "invoke" method (as an Object).
	 * The code is written to the supplied stream.
	 */
	private void codeWrapArgument(Class type, int slot,
				      DataOutputStream out)
	    throws IOException
	{
	    if (type.isPrimitive()) {
		PrimitiveTypeInfo prim = PrimitiveTypeInfo.get(type);

		out.writeByte(RuntimeConstants.opc_new);
		out.writeShort(cp.getClass(prim.wrapperClassName));

		out.writeByte(RuntimeConstants.opc_dup);

		if (type == int.class ||
		    type == boolean.class ||
		    type == byte.class ||
		    type == char.class ||
		    type == short.class)
		{
		    code_iload(slot, out);
		} else if (type == long.class) {
		    code_lload(slot, out);
		} else if (type == float.class) {
		    code_fload(slot, out);
		} else if (type == double.class) {
		    code_dload(slot, out);
		} else {
		    _assert(false);
		}

		out.writeByte(RuntimeConstants.opc_invokespecial);
		out.writeShort(cp.getMethodRef(
		    prim.wrapperClassName,
		    "<init>", prim.wrapperConstructorDesc));

	    } else {

		code_aload(slot, out);
	    }
	}

	/**
	 * Generate code for unwrapping the return value of the given type
	 * from the invocation handler's "invoke" method (of type Object) to
	 * its correct type.  The code is written to the supplied stream.
	 */
	private void codeUnwrapReturnValue(Class type, DataOutputStream out)
	    throws IOException
	{
	    if (type.isPrimitive()) {
		PrimitiveTypeInfo prim = PrimitiveTypeInfo.get(type);

		out.writeByte(RuntimeConstants.opc_checkcast);
		out.writeShort(cp.getClass(prim.wrapperClassName));

		out.writeByte(RuntimeConstants.opc_invokevirtual);
		out.writeShort(cp.getMethodRef(
		    prim.wrapperClassName,
		    prim.unwrapMethodName, prim.unwrapMethodDesc));

		if (type == int.class ||
		    type == boolean.class ||
		    type == byte.class ||
		    type == char.class ||
		    type == short.class)
		{
		    out.writeByte(RuntimeConstants.opc_ireturn);
		} else if (type == long.class) {
		    out.writeByte(RuntimeConstants.opc_lreturn);
		} else if (type == float.class) {
		    out.writeByte(RuntimeConstants.opc_freturn);
		} else if (type == double.class) {
		    out.writeByte(RuntimeConstants.opc_dreturn);
		} else {
		    _assert(false);
		}

	    } else {

		out.writeByte(RuntimeConstants.opc_checkcast);
		out.writeShort(cp.getClass(dotToSlash(type.getName())));

		out.writeByte(RuntimeConstants.opc_areturn);
	    }
	}

	/**
	 * Generate code for initializing the static field that stores
	 * the Method object for this proxy method.  The code is written
	 * to the supplied stream.
	 */
	private void codeFieldInitialization(DataOutputStream out)
	    throws IOException
	{
	    codeClassForName(fromClass, out);

	    code_ldc(cp.getString(methodName), out);

	    code_ipush(parameterTypes.length, out);

	    out.writeByte(RuntimeConstants.opc_anewarray);
	    out.writeShort(cp.getClass("java/lang/Class"));

	    for (int i = 0; i < parameterTypes.length; i++) {

		out.writeByte(RuntimeConstants.opc_dup);

		code_ipush(i, out);

		if (parameterTypes[i].isPrimitive()) {
		    PrimitiveTypeInfo prim =
			PrimitiveTypeInfo.get(parameterTypes[i]);

		    out.writeByte(RuntimeConstants.opc_getstatic);
		    out.writeShort(cp.getFieldRef(
			prim.wrapperClassName, "TYPE", "Ljava/lang/Class;"));

		} else {
		    codeClassForName(parameterTypes[i], out);
		}

		out.writeByte(RuntimeConstants.opc_aastore);
	    }

	    out.writeByte(RuntimeConstants.opc_invokevirtual);
	    out.writeShort(cp.getMethodRef(
		"java/lang/Class",
		"getMethod",
		"(Ljava/lang/String;[Ljava/lang/Class;)" +
		"Ljava/lang/reflect/Method;"));

	    out.writeByte(RuntimeConstants.opc_putstatic);
	    out.writeShort(cp.getFieldRef(
		dotToSlash(className),
		methodFieldName, "Ljava/lang/reflect/Method;"));
	}
    }

    /**
     * Generate the constructor method for the proxy class.
     */
    private MethodInfo generateConstructor() throws IOException {
	MethodInfo minfo = new MethodInfo(
	    "<init>", "(Ljava/lang/reflect/InvocationHandler;)V",
	    RuntimeConstants.ACC_PUBLIC);

	DataOutputStream out = new DataOutputStream(minfo.code);

	code_aload(0, out);

	code_aload(1, out);

	out.writeByte(RuntimeConstants.opc_invokespecial);
	out.writeShort(cp.getMethodRef(
	    superclassName,
	    "<init>", "(Ljava/lang/reflect/InvocationHandler;)V"));

//	code_aload(0, out);
//
//	code_aload(1, out);
//
//	out.writeByte(RuntimeConstants.opc_putfield);
//	out.writeShort(cp.getFieldRef(
//	    dotToSlash(className),
//	    handlerFieldName, "Ljava/lang/reflect/InvocationHandler;"));

	out.writeByte(RuntimeConstants.opc_return);

	minfo.maxStack = 10;
	minfo.maxLocals = 2;
	minfo.declaredExceptions = new short[0];

	return minfo;
    }

    /**
     * Generate the static initializer method for the proxy class.
     */
    private MethodInfo generateStaticInitializer() throws IOException {
	MethodInfo minfo = new MethodInfo(
	    "<clinit>", "()V", RuntimeConstants.ACC_STATIC);

	int localSlot0 = 1;
	short pc, tryBegin = 0, tryEnd;

	DataOutputStream out = new DataOutputStream(minfo.code);

	for (Iterator iter = proxyMethods.values().iterator();
	     iter.hasNext();)
	{
	    ProxyMethod pm = (ProxyMethod) iter.next();
	    pm.codeFieldInitialization(out);
	}

	out.writeByte(RuntimeConstants.opc_return);

	tryEnd = pc = (short) minfo.code.size();

	minfo.exceptionTable.add(new ExceptionTableEntry(
	    tryBegin, tryEnd, pc,
	    cp.getClass("java/lang/NoSuchMethodException")));

	code_astore(localSlot0, out);

	out.writeByte(RuntimeConstants.opc_new);
	out.writeShort(cp.getClass("java/lang/NoSuchMethodError"));

	out.writeByte(RuntimeConstants.opc_dup);

	code_aload(localSlot0, out);

	out.writeByte(RuntimeConstants.opc_invokevirtual);
	out.writeShort(cp.getMethodRef(
	    "java/lang/Throwable", "getMessage", "()Ljava/lang/String;"));

	out.writeByte(RuntimeConstants.opc_invokespecial);
	out.writeShort(cp.getMethodRef(
	    "java/lang/NoSuchMethodError", "<init>", "(Ljava/lang/String;)V"));

	out.writeByte(RuntimeConstants.opc_athrow);

	pc = (short) minfo.code.size();

	minfo.exceptionTable.add(new ExceptionTableEntry(
	    tryBegin, tryEnd, pc,
	    cp.getClass("java/lang/ClassNotFoundException")));

	code_astore(localSlot0, out);

	out.writeByte(RuntimeConstants.opc_new);
	out.writeShort(cp.getClass("java/lang/NoClassDefFoundError"));

	out.writeByte(RuntimeConstants.opc_dup);

	code_aload(localSlot0, out);

	out.writeByte(RuntimeConstants.opc_invokevirtual);
	out.writeShort(cp.getMethodRef(
	    "java/lang/Throwable", "getMessage", "()Ljava/lang/String;"));

	out.writeByte(RuntimeConstants.opc_invokespecial);
	out.writeShort(cp.getMethodRef(
	    "java/lang/NoClassDefFoundError",
	    "<init>", "(Ljava/lang/String;)V"));

	out.writeByte(RuntimeConstants.opc_athrow);

	minfo.maxStack = 10;
	minfo.maxLocals = (short) (localSlot0 + 1);
	minfo.declaredExceptions = new short[0];

	return minfo;
    }


    /*
     * =============== Code Generation Utility Methods ===============
     */

    /*
     * The following methods generate code for the load or store operation
     * indicated by their name for the given local variable.  The code is
     * written to the supplied stream.
     */

    private void code_iload(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_iload, RuntimeConstants.opc_iload_0, out);
    }

    private void code_lload(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_lload, RuntimeConstants.opc_lload_0, out);
    }

    private void code_fload(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_fload, RuntimeConstants.opc_fload_0, out);
    }
    
    private void code_dload(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_dload, RuntimeConstants.opc_dload_0, out);
    }

    private void code_aload(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_aload, RuntimeConstants.opc_aload_0, out);
    }

    private void code_istore(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_istore, RuntimeConstants.opc_istore_0, out);
    }

    /* 6220850
    private void code_lstore(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_lstore, RuntimeConstants.opc_lstore_0, out);
    }

    private void code_fstore(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_fstore, RuntimeConstants.opc_fstore_0, out);
    }

    private void code_dstore(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_dstore, RuntimeConstants.opc_dstore_0, out);
    }
    6220850 */

    private void code_astore(int lvar, DataOutputStream out)
	throws IOException
    {
	codeLocalLoadStore(lvar,
	    RuntimeConstants.opc_astore, RuntimeConstants.opc_astore_0, out);
    }

    /**
     * Generate code for a load or store instruction for the given local
     * variable.  The code is written to the supplied stream.
     *
     * "opcode" indicates the opcode form of the desired load or store
     * instruction that takes an explicit local variable index, and
     * "opcode_0" indicates the corresponding form of the instruction
     * with the implicit index 0.
     */
    private void codeLocalLoadStore(int lvar, int opcode, int opcode_0,
				    DataOutputStream out)
	throws IOException
    {
	_assert(lvar >= 0 && lvar <= 0xFFFF);
	if (lvar <= 3) {
	    out.writeByte(opcode_0 + lvar);
	} else if (lvar <= 0xFF) {
	    out.writeByte(opcode);
	    out.writeByte(lvar & 0xFF);
	} else {
	    /*
	     * Use the "wide" instruction modifier for local variable
	     * indexes that do not fit into an unsigned byte.
	     */
	    out.writeByte(RuntimeConstants.opc_wide);
	    out.writeByte(opcode);
	    out.writeShort(lvar & 0xFFFF);
	}
    }

    /**
     * Generate code for an "ldc" instruction for the given constant pool
     * index (the "ldc_w" instruction is used if the index does not fit
     * into an unsigned byte).  The code is written to the supplied stream.
     */
    private void code_ldc(int index, DataOutputStream out)
	throws IOException
    {
	_assert(index >= 0 && index <= 0xFFFF);
	if (index <= 0xFF) {
	    out.writeByte(RuntimeConstants.opc_ldc);
	    out.writeByte(index & 0xFF);
	} else {
	    out.writeByte(RuntimeConstants.opc_ldc_w);
	    out.writeShort(index & 0xFFFF);
	}
    }

    /**
     * Generate code to push a constant integer value on to the operand
     * stack, using the "iconst_<i>", "bipush", or "sipush" instructions
     * depending on the size of the value.  The code is written to the
     * supplied stream.
     */
    private void code_ipush(int value, DataOutputStream out)
	throws IOException
    {
	if (value >= -1 && value <= 5) {
	    out.writeByte(RuntimeConstants.opc_iconst_0 + value);
	} else if (value >= Byte.MIN_VALUE && value <= Byte.MAX_VALUE) {
	    out.writeByte(RuntimeConstants.opc_bipush);
	    out.writeByte(value & 0xFF);
	} else if (value >= Short.MIN_VALUE && value <= Short.MAX_VALUE) {
	    out.writeByte(RuntimeConstants.opc_sipush);
	    out.writeShort(value & 0xFFFF);
	} else {
	    _assert(false);
	}
    }

    /**
     * Generate code to invoke the Class.forName with the name of the given
     * class to get its Class object at runtime.  The code is written to
     * the supplied stream.  Note that the code generated by this method
     * may caused the checked ClassNotFoundException to be thrown.
     */
    private void codeClassForName(Class cl, DataOutputStream out)
	throws IOException
    {
	code_ldc(cp.getString(cl.getName()), out);

	out.writeByte(RuntimeConstants.opc_invokestatic);
	out.writeShort(cp.getMethodRef(
	    "java/lang/Class",
	    "forName", "(Ljava/lang/String;)Ljava/lang/Class;"));
    }


    /*
     * ==================== General Utility Methods ====================
     */

    /**
     * Assert that an assertion is true: throw InternalError if it is not.
     */
    private static void _assert(boolean assertion) {
	if (assertion != true) {
	    throw new InternalError("assertion failure");
	}
    }

    /**
     * Convert a fully qualified class name that uses '.' as the package
     * separator, the external representation used by the Java language
     * and APIs, to a fully qualified class name that uses '/' as the
     * package separator, the representation used in the class file
     * format (see JVMS section 4.2).
     */
    private static String dotToSlash(String name) {
	return name.replace('.', '/');
    }

    /**
     * Return the "method descriptor" string for a method with the given
     * parameter types and return type.  See JVMS section 4.3.3.
     */
    private static String getMethodDescriptor(Class[] parameterTypes,
					      Class returnType)
    {
	return getParameterDescriptors(parameterTypes) +
	    ((returnType == void.class) ? "V" : getFieldType(returnType));
    }

    /**
     * Return the list of "parameter descriptor" strings enclosed in
     * parentheses corresponding to the given parameter types (in other
     * words, a method descriptor without a return descriptor).  This
     * string is useful for constructing string keys for methods without
     * regard to their return type.
     */
    private static String getParameterDescriptors(Class[] parameterTypes) {
	StringBuffer desc = new StringBuffer("(");
	for (int i = 0; i < parameterTypes.length; i++) {
	    desc.append(getFieldType(parameterTypes[i]));
	}
	desc.append(')');
	return desc.toString();
    }

    /**
     * Return the "field type" string for the given type, appropriate for
     * a field descriptor, a parameter descriptor, or a return descriptor
     * other than "void".  See JVMS section 4.3.2.
     */
    private static String getFieldType(Class type) {
	if (type.isPrimitive()) {
	    return PrimitiveTypeInfo.get(type).baseTypeString;
	} else if (type.isArray()) {
	    /*
	     * According to JLS 20.3.2, the getName() method on Class does
	     * return the VM type descriptor format for array classes (only);
	     * using that should be quicker than the otherwise obvious code:
	     *
	     *     return "[" + getTypeDescriptor(type.getComponentType());
	     */
	    return type.getName().replace('.', '/');
	} else {
	    return "L" + dotToSlash(type.getName()) + ";";
	}
    }

    /**
     * Return the number of abstract "words", or consecutive local variable
     * indexes, required to contain a value of the given type.  See JVMS
     * section 3.6.1.
     *
     * Note that the original version of the JVMS contained a definition of
     * this abstract notion of a "word" in section 3.4, but that definition
     * was removed for the second edition.
     */
    private static int getWordsPerType(Class type) {
	if (type == long.class || type == double.class) {
	    return 2;
	} else {
	    return 1;
	}
    }

    /**
     * Add to the given list all of the types in the "from" array that
     * are not already contained in the liast and are assignable to at
     * least one of the types in the "with" array.
     *
     * This method is useful for computing the greatest common set of
     * declared exceptions from duplicate methods inherited from
     * different interfaces.
     */
    private static void collectCompatibleTypes(Class[] from, Class[] with,
					       List list)
    {
	for (int i = 0; i < from.length; i++) {
	    if (!list.contains(from[i])) {
		for (int j = 0; j < with.length; j++) {
		    if (with[j].isAssignableFrom(from[i])) {
			list.add(from[i]);
			break;
		    }
		}
	    }
	}
    }

    /**
     * Given the exceptions declared in the throws clause of a proxy method,
     * compute the exceptions that need to be caught from the invocation
     * handler's invoke method and rethrown intact in the method's
     * implementation before catching other Throwables and wrapping them
     * in UndeclaredThrowableExceptions.
     *
     * The exceptions to be caught are returned in a List object.  Each
     * exception in the returned list is guaranteed to not be a subclass of
     * any of the other exceptions in the list, so the catch blocks for
     * these exceptions may be generated in any order relative to each other.
     *
     * Error and RuntimeException are each always contained by the returned
     * list (if none of their superclasses are contained), since those
     * unchecked exceptions should always be rethrown intact, and thus their
     * subclasses will never appear in the returned list.
     *
     * The returned List will be empty if java.lang.Throwable is in the
     * given list of declared exceptions, indicating that no exceptions
     * need to be caught.
     */
    private static List computeUniqueCatchList(Class[] exceptions) {
	List uniqueList = new ArrayList();	// unique exceptions to catch

	uniqueList.add(Error.class);		// always catch/rethrow these
	uniqueList.add(RuntimeException.class);

    nextException:
	for (int i = 0; i < exceptions.length; i++) {
	    Class ex = exceptions[i];
	    if (ex.isAssignableFrom(Throwable.class)) {
		/*
		 * If Throwable is declared to be thrown by the proxy method,
		 * then no catch blocks are necessary, because the invoke
		 * can, at most, throw Throwable anyway.
		 */
		uniqueList.clear();
		break;
	    } else if (!Throwable.class.isAssignableFrom(ex)) {
		/*
		 * Ignore types that cannot be thrown by the invoke method.
		 */
		continue;
	    }
	    /*
	     * Compare this exception against the current list of
	     * exceptions that need to be caught:
	     */
	    for (int j = 0; j < uniqueList.size();) {
		Class ex2 = (Class) uniqueList.get(j);
		if (ex2.isAssignableFrom(ex)) {
		    /*
		     * if a superclass of this exception is already on
		     * the list to catch, then ignore this one and continue;
		     */
		    continue nextException;
		} else if (ex.isAssignableFrom(ex2)) {
		    /*
		     * if a subclass of this exception is on the list
		     * to catch, then remove it;
		     */
		    uniqueList.remove(j);
		} else {
		    j++;	// else continue comparing.
		}
	    }
	    // This exception is unique (so far): add it to the list to catch.
	    uniqueList.add(ex);
	}
	return uniqueList;
    }

    /**
     * A PrimitiveTypeInfo object contains assorted information about
     * a primitive type in its public fields.  The struct for a particular
     * primitive type can be obtained using the static "get" method.
     */
    private static class PrimitiveTypeInfo {

	/** "base type" used in various descriptors (see JVMS section 4.3.2) */
	public String baseTypeString;

	/** name of corresponding wrapper class */
	public String wrapperClassName;

	/** method descriptor for wrapper class constructor */
	public String wrapperConstructorDesc;

	/** name of wrapper class method for retrieving primitive value */
	public String unwrapMethodName;

	/** descriptor of same method */
	public String unwrapMethodDesc;

	private static Map table = new HashMap(11);
	static {
	    table.put(int.class, new PrimitiveTypeInfo(
		"I", "java/lang/Integer", "(I)V", "intValue",     "()I"));
	    table.put(boolean.class, new PrimitiveTypeInfo(
		"Z", "java/lang/Boolean", "(Z)V", "booleanValue", "()Z"));
	    table.put(byte.class, new PrimitiveTypeInfo(
		"B", "java/lang/Byte",    "(B)V", "byteValue",    "()B"));
	    table.put(char.class, new PrimitiveTypeInfo(
		"C", "java/lang/Character",    "(C)V", "charValue",    "()C"));
	    table.put(short.class, new PrimitiveTypeInfo(
		"S", "java/lang/Short",   "(S)V", "shortValue",   "()S"));
	    table.put(long.class, new PrimitiveTypeInfo(
		"J", "java/lang/Long",    "(J)V", "longValue",    "()J"));
	    table.put(float.class, new PrimitiveTypeInfo(
		"F", "java/lang/Float",   "(F)V", "floatValue",   "()F"));
	    table.put(double.class, new PrimitiveTypeInfo(
		"D", "java/lang/Double",  "(D)V", "doubleValue",  "()D"));
	}

	private PrimitiveTypeInfo(String baseTypeString,
				  String wrapperClassName,
				  String wrapperConstructorDesc,
				  String unwrapMethodName,
				  String unwrapMethodDesc)
	{
	    this.baseTypeString = baseTypeString;
	    this.wrapperClassName = wrapperClassName;
	    this.wrapperConstructorDesc = wrapperConstructorDesc;
	    this.unwrapMethodName = unwrapMethodName;
	    this.unwrapMethodDesc = unwrapMethodDesc;
	}

	public static PrimitiveTypeInfo get(Class cl) {
	    return (PrimitiveTypeInfo) table.get(cl);
	}
    }


    /**
     * A ConstantPool object represents the constant pool of a class file
     * being generated.  This representation of a constant pool is designed
     * specifically for use by ProxyGenerator; in particular, it assumes
     * that constant pool entries will not need to be resorted (for example,
     * by their type, as the Java compiler does), so that the final index
     * value can be assigned and used when an entry is first created.
     *
     * Note that new entries cannot be created after the constant pool has
     * been written to a class file.  To prevent such logic errors, a
     * ConstantPool instance can be marked "read only", so that further
     * attempts to add new entries will fail with a runtime exception.
     *
     * See JVMS section 4.4 for more information about the constant pool
     * of a class file.
     */
    private static class ConstantPool {

	/**
         * list of constant pool entries, in constant pool index order.
	 *
	 * This list is used when writing the constant pool to a stream
	 * and for assigning the next index value.  Note that element 0
	 * of this list corresponds to constant pool index 1.
	 */
	private List pool = new ArrayList(32);

	/**
	 * maps constant pool data of all types to constant pool indexes.
	 *
	 * This map is used to look up the index of an existing entry for
	 * values of all types.
	 */
	private Map map = new HashMap(16);

        /** true if no new constant pool entries may be added */
	private boolean readOnly = false;

	/**
	 * Get or assign the index for a CONSTANT_Utf8 entry.
	 */
	public short getUtf8(String s) {
	    if (s == null) {
		throw new NullPointerException();
	    }
	    return getValue(s);
	}

	/**
	 * Get or assign the index for a CONSTANT_Integer entry.
	 */
	public short getInteger(int i) {
	    return getValue(new Integer(i));
	}

	/**
	 * Get or assign the index for a CONSTANT_Float entry.
	 */
	public short getFloat(float f) {
	    return getValue(new Float(f));
	}

	/**
	 * Get or assign the index for a CONSTANT_Long entry.
	 */
	public short getLong(long l) {
	    return getValue(new Long(l));
	}

	/**
	 * Get or assign the index for a CONSTANT_Double entry.
	 */
	public short getDouble(double d) {
	    return getValue(new Double(d));
	}

	/**
	 * Get or assign the index for a CONSTANT_Class entry.
	 */
	public short getClass(String name) {
	    short utf8Index = getUtf8(name);
	    return getIndirect(new IndirectEntry(
		RuntimeConstants.CONSTANT_CLASS, utf8Index));
	}

	/**
	 * Get or assign the index for a CONSTANT_String entry.
	 */
	public short getString(String s) {
	    short utf8Index = getUtf8(s);
	    return getIndirect(new IndirectEntry(
		RuntimeConstants.CONSTANT_STRING, utf8Index));
	}

	/**
	 * Get or assign the index for a CONSTANT_FieldRef entry.
	 */
	public short getFieldRef(String className,
				 String name, String descriptor)
	{
	    short classIndex = getClass(className);
	    short nameAndTypeIndex = getNameAndType(name, descriptor);
	    return getIndirect(new IndirectEntry(
	        RuntimeConstants.CONSTANT_FIELD,
		classIndex, nameAndTypeIndex));
	}

	/**
	 * Get or assign the index for a CONSTANT_MethodRef entry.
	 */
	public short getMethodRef(String className,
				  String name, String descriptor)
	{
	    short classIndex = getClass(className);
	    short nameAndTypeIndex = getNameAndType(name, descriptor);
	    return getIndirect(new IndirectEntry(
	        RuntimeConstants.CONSTANT_METHOD,
		classIndex, nameAndTypeIndex));
	}

	/**
	 * Get or assign the index for a CONSTANT_InterfaceMethodRef entry.
	 */
	public short getInterfaceMethodRef(String className, String name,
					   String descriptor)
	{
	    short classIndex = getClass(className);
	    short nameAndTypeIndex = getNameAndType(name, descriptor);
	    return getIndirect(new IndirectEntry(
                RuntimeConstants.CONSTANT_INTERFACEMETHOD,
		classIndex, nameAndTypeIndex));
	}

	/**
	 * Get or assign the index for a CONSTANT_NameAndType entry.
	 */
	public short getNameAndType(String name, String descriptor) {
	    short nameIndex = getUtf8(name);
	    short descriptorIndex = getUtf8(descriptor);
	    return getIndirect(new IndirectEntry(
	        RuntimeConstants.CONSTANT_NAMEANDTYPE,
		nameIndex, descriptorIndex));
	}

	/**
	 * Set this ConstantPool instance to be "read only".
	 *
	 * After this method has been called, further requests to get
	 * an index for a non-existent entry will cause an InternalError
	 * to be thrown instead of creating of the entry.
	 */
	public void setReadOnly() {
	    readOnly = true;
	}

	/**
	 * Write this constant pool to a stream as part of
	 * the class file format.
	 *
	 * This consists of writing the "constant_pool_count" and
	 * "constant_pool[]" items of the "ClassFile" structure, as
	 * described in JVMS section 4.1.
	 */
	public void write(OutputStream out) throws IOException {
	    DataOutputStream dataOut = new DataOutputStream(out);

	    // constant_pool_count: number of entries plus one
	    dataOut.writeShort(pool.size() + 1);

	    for (Iterator iter = pool.iterator(); iter.hasNext();) {
		Entry e = (Entry) iter.next();
		e.write(dataOut);
	    }
	}

	/**
	 * Add a new constant pool entry and return its index.
	 */
	private short addEntry(Entry entry) {
	    pool.add(entry);
	    return (short) pool.size();
	}

	/**
	 * Get or assign the index for an entry of a type that contains
	 * a direct value.  The type of the given object determines the
	 * type of the desired entry as follows:
	 * 
	 *	java.lang.String	CONSTANT_Utf8
	 *	java.lang.Integer	CONSTANT_Integer
	 *	java.lang.Float		CONSTANT_Float
	 *	java.lang.Long		CONSTANT_Long
	 *	java.lang.Double	CONSTANT_DOUBLE
	 */
	private short getValue(Object key) {
	    Short index = (Short) map.get(key);
	    if (index != null) {
		return index.shortValue();
	    } else {
		if (readOnly) {
		    throw new InternalError(
                        "late constant pool addition: " + key);
		}
		short i = addEntry(new ValueEntry(key));
		map.put(key, new Short(i));
		return i;
	    }
	}

	/**
	 * Get or assign the index for an entry of a type that contains
	 * references to other constant pool entries.
	 */
	private short getIndirect(IndirectEntry e) {
	    Short index = (Short) map.get(e);
	    if (index != null) {
		return index.shortValue();
	    } else {
		if (readOnly) {
		    throw new InternalError("late constant pool addition");
		}
		short i = addEntry(e);
		map.put(e, new Short(i));
		return i;
	    }
	}

	/**
	 * Entry is the abstact superclass of all constant pool entry types
	 * that can be stored in the "pool" list; its purpose is to define a
	 * common method for writing constant pool entries to a class file.
	 */
	private static abstract class Entry {
	    public abstract void write(DataOutputStream out)
	        throws IOException;
	}

	/**
	 * ValueEntry represents a constant pool entry of a type that
	 * contains a direct value (see the comments for the "getValue"
	 * method for a list of such types).
	 *
	 * ValueEntry objects are not used as keys for their entries in the
	 * Map "map", so no useful hashCode or equals methods are defined.
	 */
	private static class ValueEntry extends Entry {
	    private Object value;

	    public ValueEntry(Object value) {
		this.value = value;
	    }

	    public void write(DataOutputStream out) throws IOException {
		if (value instanceof String) {
		    out.writeByte(RuntimeConstants.CONSTANT_UTF8);
		    out.writeUTF((String) value);
		} else if (value instanceof Integer) {
		    out.writeByte(RuntimeConstants.CONSTANT_INTEGER);
		    out.writeInt(((Integer) value).intValue());
		} else if (value instanceof Float) {
		    out.writeByte(RuntimeConstants.CONSTANT_FLOAT);
		    out.writeFloat(((Float) value).floatValue());
		} else if (value instanceof Long) {
		    out.writeByte(RuntimeConstants.CONSTANT_LONG);
		    out.writeLong(((Long) value).longValue());
		} else if (value instanceof Double) {
		    out.writeDouble(RuntimeConstants.CONSTANT_DOUBLE);
		    out.writeDouble(((Double) value).doubleValue());
		} else {
		    throw new InternalError("bogus value entry: " + value);
		}
	    }
	}

	/**
	 * IndirectEntry represents a constant pool entry of a type that
	 * references other constant pool entries, i.e., the following types:
	 *
	 *	CONSTANT_Class, CONSTANT_String, CONSTANT_Fieldref,
	 *	CONSTANT_Methodref, CONSTANT_InterfaceMethodref, and
	 *	CONSTANT_NameAndType.
	 *
	 * Each of these entry types contains either one or two indexes of
	 * other constant pool entries.
	 *
	 * IndirectEntry objects are used as the keys for their entries in
	 * the Map "map", so the hashCode and equals methods are overridden
	 * to allow matching.
	 */
	private static class IndirectEntry extends Entry {
	    private int tag;
	    private short index0;
	    private short index1;

	    /**
	     * Construct an IndirectEntry for a constant pool entry type
	     * that contains one index of another entry.
	     */
	    public IndirectEntry(int tag, short index) {
		this.tag = tag;
		this.index0 = index;
		this.index1 = 0;
	    }

	    /**
	     * Construct an IndirectEntry for a constant pool entry type
	     * that contains two indexes for other entries.
	     */
	    public IndirectEntry(int tag, short index0, short index1) {
		this.tag = tag;
		this.index0 = index0;
		this.index1 = index1;
	    }

	    public void write(DataOutputStream out) throws IOException {
		out.writeByte(tag);
		out.writeShort(index0);
		/*
		 * If this entry type contains two indexes, write
		 * out the second, too.
		 */
		if (tag == RuntimeConstants.CONSTANT_FIELD ||
		    tag == RuntimeConstants.CONSTANT_METHOD ||
		    tag == RuntimeConstants.CONSTANT_INTERFACEMETHOD || 
		    tag == RuntimeConstants.CONSTANT_NAMEANDTYPE)
		{
		    out.writeShort(index1);
		}
	    }

	    public int hashCode() {
		return tag + index0 + index1;
	    }

	    public boolean equals(Object obj) {
		if (obj instanceof IndirectEntry) {
		    IndirectEntry other = (IndirectEntry) obj;
		    if (tag == other.tag &&
			index0 == other.index0 && index1 == other.index1)
		    {
			return true;
		    }
		}
		return false;
	    }
	}
    }
}
