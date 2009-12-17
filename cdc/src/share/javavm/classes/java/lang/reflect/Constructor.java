/*
 * @(#)Constructor.java	1.34 06/10/10
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

package java.lang.reflect;

import sun.misc.CVM;

/**
 * <code>Constructor</code> provides information about, and access to, a single
 * constructor for a class.
 *
 * <p><code>Constructor</code> permits widening conversions to occur when matching the
 * actual parameters to newInstance() with the underlying
 * constructor's formal parameters, but throws an
 * <code>IllegalArgumentException</code> if a narrowing conversion would occur.
 *
 * @see Member
 * @see java.lang.Class
 * @see java.lang.Class#getConstructors()
 * @see java.lang.Class#getConstructor(Class[])
 * @see java.lang.Class#getDeclaredConstructors()
 *
 * @author	Nakul Saraiya
 */
public final
class Constructor extends AccessibleObject implements Member {

    private Class		clazz;
    private int			slot;
    private Class[]		parameterTypes;
    private Class[]		exceptionTypes;
    private int			modifiers;

    /**
     * Constructor.  Only the Java Virtual Machine may construct
     * a Constructor.
     */
    private Constructor() {}

    /**
     * Returns the <code>Class</code> object representing the class that declares
     * the constructor represented by this <code>Constructor</code> object.
     */
    public Class getDeclaringClass() {
	return clazz;
    }

    /**
     * Returns the name of this constructor, as a string.  This is
     * always the same as the simple name of the constructor's declaring
     * class.
     */
    public String getName() {
	return getDeclaringClass().getName();
    }

    /**
     * Returns the Java language modifiers for the constructor
     * represented by this <code>Constructor</code> object, as an integer. The
     * <code>Modifier</code> class should be used to decode the modifiers.
     *
     * @see Modifier
     */
    public int getModifiers() {
	return modifiers;
    }

    /**
     * Returns an array of <code>Class</code> objects that represent the formal
     * parameter types, in declaration order, of the constructor
     * represented by this <code>Constructor</code> object.  Returns an array of
     * length 0 if the underlying constructor takes no parameters.
     *
     * @return the parameter types for the constructor this object
     * represents
     */
    public Class[] getParameterTypes() {
	return Method.copy(parameterTypes);
    }

    /**
     * Returns an array of <code>Class</code> objects that represent the types of
     * of exceptions declared to be thrown by the underlying constructor
     * represented by this <code>Constructor</code> object.  Returns an array of
     * length 0 if the constructor declares no exceptions in its <code>throws</code> clause.
     *
     * @return the exception types declared as being thrown by the
     * constructor this object represents
     */
    public Class[] getExceptionTypes() {
	return Method.copy(exceptionTypes);
    }

    /**
     * Compares this <code>Constructor</code> against the specified object.
     * Returns true if the objects are the same.  Two <code>Constructor</code> objects are
     * the same if they were declared by the same class and have the
     * same formal parameter types.
     */
    public boolean equals(Object obj) {
	if (obj != null && obj instanceof Constructor) {
	    Constructor other = (Constructor)obj;
	    if (getDeclaringClass() == other.getDeclaringClass()) {
		/* Avoid unnecessary cloning */
		Class[] params1 = parameterTypes;
		Class[] params2 = other.parameterTypes;
		if (params1.length == params2.length) {
		    for (int i = 0; i < params1.length; i++) {
			if (params1[i] != params2[i])
			    return false;
		    }
		    return true;
		}
	    }
	}
	return false;
    }

    /**
     * Returns a hashcode for this <code>Constructor</code>. The hashcode is
     * the same as the hashcode for the underlying constructor's
     * declaring class name.
     */
    public int hashCode() {
	return getDeclaringClass().getName().hashCode();
    }

    /**
     * Returns a string describing this <code>Constructor</code>.  The string is
     * formatted as the constructor access modifiers, if any,
     * followed by the fully-qualified name of the declaring class,
     * followed by a parenthesized, comma-separated list of the
     * constructor's formal parameter types.  For example:
     * <pre>
     *    public java.util.Hashtable(int,float)
     * </pre>
     *
     * <p>The only possible modifiers for constructors are the access
     * modifiers <tt>public</tt>, <tt>protected</tt> or
     * <tt>private</tt>.  Only one of these may appear, or none if the
     * constructor has default (package) access.
     */
    public String toString() {
	try {
	    StringBuffer sb = new StringBuffer();
	    int mod = getModifiers();
	    if (mod != 0) {
		sb.append(Modifier.toString(mod) + " ");
	    }
	    sb.append(Field.getTypeName(getDeclaringClass()));
	    sb.append("(");
	    Class[] params = parameterTypes; // avoid clone
	    for (int j = 0; j < params.length; j++) {
		sb.append(Field.getTypeName(params[j]));
		if (j < (params.length - 1))
		    sb.append(",");
	    }
	    sb.append(")");
	    Class[] exceptions = exceptionTypes; // avoid clone
	    if (exceptions.length > 0) {
		sb.append(" throws ");
		for (int k = 0; k < exceptions.length; k++) {
		    sb.append(exceptions[k].getName());
		    if (k < (exceptions.length - 1))
			sb.append(",");
		}
	    }
	    return sb.toString();
	} catch (Exception e) {
	    return "<" + e + ">";
	}
    }

    /** An internal class to be able to distinguish between an
      IllegalArgumentException because of incorrectly-typed objects in
      the argument array to Constructor.newInstance() and one thrown
      by the invoked method. NOTE: do not change the name of this
      class as it is referenced by name in reflect.c, utils.c and
      elsewhere. */
    private static class ArgumentException extends Exception {
    }

    /** An internal class to be able to distinguish between an
      IllegalAccessException because of an invalid attempt to invoke a
      protected or private method in Constructor.newInstance() and one
      thrown by the invoked method. NOTE: do not change the name of
      this class as it is referenced by name in reflect.c, utils.c and
      elsewhere. */
    private static class AccessException extends Exception {
    }

    /**
     * Uses the constructor represented by this <code>Constructor</code> object to
     * create and initialize a new instance of the constructor's
     * declaring class, with the specified initialization parameters.
     * Individual parameters are automatically unwrapped to match
     * primitive formal parameters, and both primitive and reference
     * parameters are subject to method invocation conversions as necessary.
     *
     * <p>If the number of formal parameters required by the underlying constructor
     * is 0, the supplied <code>initargs</code> array may be of length 0 or null.
     *
     * <p>If the required access and argument checks succeed and the
     * instantiation will proceed, the constructor's declaring class
     * is initialized if it has not already been initialized.
     *
     * <p>If the constructor completes normally, returns the newly
     * created and initialized instance.
     *
     * @param initargs array of objects to be passed as arguments to
     * the constructor call; values of primitive types are wrapped in
     * a wrapper object of the appropriate type (e.g. a <tt>float</tt>
     * in a {@link java.lang.Float Float})
     *
     * @return a new object created by calling the constructor
     * this object represents
     * 
     * @exception IllegalAccessException    if this <code>Constructor</code> object
     *              enforces Java language access control and the underlying
     *              constructor is inaccessible.
     * @exception IllegalArgumentException  if the number of actual
     *              and formal parameters differ; if an unwrapping
     *              conversion for primitive arguments fails; or if,
     *              after possible unwrapping, a parameter value
     *              cannot be converted to the corresponding formal
     *              parameter type by a method invocation conversion.
     * @exception InstantiationException    if the class that declares the
     *              underlying constructor represents an abstract class.
     * @exception InvocationTargetException if the underlying constructor
     *              throws an exception.
     * @exception ExceptionInInitializerError if the initialization provoked
     *              by this method fails.
     */
    public Object newInstance(Object[] initargs)
	throws InstantiationException, IllegalAccessException,
	    IllegalArgumentException, InvocationTargetException
    {
	/* Null pointer is acceptable for args if no arguments to method */
	if (parameterTypes.length == 0) {
	    if ((initargs != null) && (initargs.length != 0))
		throw new IllegalArgumentException(
		    "wrong number of arguments"
		);
	} else {
	    if ((initargs == null) ||
		(initargs.length != parameterTypes.length))
		throw new IllegalArgumentException(
		    "wrong number of arguments"
		);
	}

	/* %comment: rt038 */

	CVM.setContextArtificial();

	Object res = allocateUninitializedObject(clazz);
	// If result is null this means the allocation threw an
	// exception, so we will never call invokeConstructor with a
	// null object pointer.
	try {
	    invokeConstructor(res, initargs);
	}
	catch (ArgumentException e) {
	    throw new IllegalArgumentException(
		"wrong object type, or unwrapping conversion failed"
	    );
	}
	catch (AccessException e) {
	    throw new IllegalAccessException();
	}
	catch (Throwable e) {
	    throw new InvocationTargetException(e);
	}
	return res;
    }
    
    private static native Object allocateUninitializedObject(Class clazz)
	throws InstantiationException, IllegalAccessException;
    private native void invokeConstructor(Object obj, Object[] args)
	throws ArgumentException, AccessException, Exception;
}
