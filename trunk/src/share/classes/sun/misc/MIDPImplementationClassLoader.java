/*
 * @(#)MIDPImplementationClassLoader.java	1.13 06/11/07
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
/*
 * @(#)MIDPImplementationClassLoader.java	1.4	03/08/19
 *
 * Class loader for MIDP implementation running on CDC/PP
 */
package sun.misc;

import java.net.URL;
import java.net.URLConnection;
import java.net.URLClassLoader;
import java.io.IOException;
import java.io.InputStream;
import java.security.CodeSource;
import java.security.PermissionCollection;
import java.security.ProtectionDomain;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedExceptionAction;
import java.security.cert.Certificate;


public class MIDPImplementationClassLoader extends URLClassLoader{

    URL myBase[];
    private PermissionCollection perms;
    private ClassLoader parent;

    public MIDPImplementationClassLoader(
	URL base[],
	PermissionCollection pc,
	ClassLoader parent)
    {
	super(base, parent);
	myBase = base;
	perms = pc;
	this.parent = parent;
	// Register in case classes were preloaded
	CVM.Preloader.registerClassLoader("midp", this);
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
    loadFromParent(String classname, boolean resolve)
                            throws ClassNotFoundException
    {
        /* Call java.lang.ClassLoader(classname, resolve),
         * which uses the NULL classLoader to load
         * class if parent is null, or calls parent.loadClass()
         * when the parent is not null. */
        return super.loadClass(classname, resolve);
    }

    public synchronized Class
    loadClass(String classname, boolean resolve)
                            throws ClassNotFoundException
    {
	Class resultClass;
	classname = classname.intern();

	resultClass = findLoadedClass(classname);

	if (resultClass == null){
	    try {
		resultClass = loadFromParent(classname, resolve);
	    }catch(Exception e){
		/*DEBUG e.printStackTrace(); */
		resultClass = null;
	    }
	}
	
	if (resultClass == null){
	    try {
		resultClass = super.findClass(classname); // from URLClassLoader
	    }catch(Exception e){
		/*DEBUG e.printStackTrace(); */
		resultClass = null;
	    }
	}
	if (resultClass == null)
	    throw new ClassNotFoundException(classname);
	if (resolve){
	    resolveClass(resultClass);
	}
	/*DEBUG if(helperClass){
	 *	System.out.println("returning "+classname+"..."); 
	 *	System.out.println(resultClass); 
	 * }
	 */
	return resultClass;
    }

    /*
    InputStream getResourceAsStream(String name)
    */

}

