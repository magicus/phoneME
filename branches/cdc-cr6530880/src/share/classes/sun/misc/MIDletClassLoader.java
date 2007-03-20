/*
 * @(#)MIDletClassLoader.java	1.11 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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
/*
 * @(#)MIDletClassLoader.java	1.5	03/07/09
 *
 * Class loader for midlets running on CDC/PP
 *
 * It loads from a single JAR file and deligates to other class loaders. 
 * It has a few of interesting properties:
 * 1) it is careful, perhaps overly so, about not loading classes that
 *    are in system packages from the JAR file persumed to contain the MIDlet
 *    code.
 * 2) it uses a MemberFilter to process classes for illegal field references.
 *    This is easiest to do after the constant pool is set up.
 * 3) it remembers which classes have failed the above test and refuses to
 *    load them, even though the system thinks they're already loaded.
 *
 * It lets the underlying URLClassLoader do all the hard work.
 *    
 */
package sun.misc;

import java.net.URL;
import java.net.URLConnection;
import java.net.URLClassLoader;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.security.CodeSource;
import java.security.PermissionCollection;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedAction;


public class MIDletClassLoader extends URLClassLoader {

    URL	    myBase[];
    String[]systemPkgs;
    private MemberFilter memberChecker; /* to check for amputated members */
    private PermissionCollection perms;
    private HashSet badMidletClassnames = new HashSet();
    private MIDPImplementationClassLoader implementationClassLoader;
    private AccessControlContext ac = AccessController.getContext();

    public MIDletClassLoader(
	URL base[],
	String systemPkgs[],
	PermissionCollection pc,
	MemberFilter mf,
	MIDPImplementationClassLoader  parent)
    {
	super(base, parent);
	myBase = base;
	this.systemPkgs = systemPkgs;
	memberChecker = mf;
	perms = pc;
	implementationClassLoader = parent;
    }


    protected PermissionCollection getPermissions(CodeSource cs){
	URL srcLocation = cs.getLocation();
	for (int i=0; i<myBase.length; i++){
	    if (srcLocation.equals(myBase[i])){
		return perms;
	    }
	}
	return super.getPermissions(cs);
    }


    private Class
    loadFromUrl(String classname) throws ClassNotFoundException
    {
	// first ensure we like componentName.
	// it should not have a systemPkg entry as a prefix!
	// this is probably paranoid. There are probably other
	// mechanisms to avoid this.
	String forbidden[] = systemPkgs;
	int fLength = forbidden.length;
	for (int i=0; i< fLength; i++){
	    if (classname.startsWith(forbidden[i])){
		return null; // go look elsewhere.
	    }
	}
	Class newClass;
	try {
	    newClass = super.findClass(classname); // call URLClassLoader
	}catch(Exception e){
	    /*DEBUG e.printStackTrace(); */
	    // didn't find it.
	    return null;
	}
	if (newClass == null )
	    return null;
	try {
	    // memberChecker will throw an Error if it doesn't like
	    // the class.
	    memberChecker.checkMemberAccessValidity(newClass);
	    return newClass;
	} catch (Error e){
	    // If this happens, act as if we cannot find the class.
	    // remember this class, too. If the MIDlet catches the
	    // Exception and tries again, we don't want findLoadedClass()
	    // to return it!!
	    badMidletClassnames.add(classname);
	    throw new ClassNotFoundException(e.getMessage());
	}
    }


    public synchronized Class
    loadClass(String classname, boolean resolve) throws ClassNotFoundException
    {
	Class resultClass;
	classname = classname.intern();
	if (badMidletClassnames.contains(classname)){
	    // the system thinks it successfully loaded this class.
	    // But the member checker does not think we should be able
	    // to use it. We threw an Exception before. Do it again.
	    throw new ClassNotFoundException(classname.concat(
			" contains illegal member reference"));
	}
	resultClass = findLoadedClass(classname);
	if (resultClass == null){
	    resultClass = loadFromUrl(classname);
	    if (resultClass == null){
		resultClass = implementationClassLoader.loadClass(classname,
								  false, true);
	    }
	}
	if (resultClass == null)
	    throw new ClassNotFoundException(classname);
	if (resolve)
	    resolveClass(resultClass);
	return resultClass;
    }

   public InputStream
   getResourceAsStream(final String name){
	// prohibit reading .class as a resource
	if (name.endsWith(".class")){
	    return null; // not allowed!
	}
	// do not delegate. We only use our own URLClassLoader.findResource to
	// look in our own JAR file. That is always allowed.
	// Nothing else is.
	InputStream retval;
	retval = (InputStream) AccessController.doPrivileged(
		new PrivilegedAction(){
		    public Object run(){
			URL url = findResource(name);
			try {
			    return url != null ? url.openStream() : null;
			} catch (IOException e) {
			    return null;
			}
		    }
		}, ac);
	return retval;
    }

}

