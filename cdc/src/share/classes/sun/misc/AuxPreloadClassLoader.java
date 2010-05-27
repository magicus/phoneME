/*
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

/*
 * @(#)AuxPreloadClassLoader.java	1.5	06/10/10
 *
 * Class loader for MIDP implementation running on CDC/PP
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
import java.security.SecureClassLoader;
import java.security.ProtectionDomain;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedExceptionAction;
import java.security.cert.Certificate;


public class AuxPreloadClassLoader extends SecureClassLoader{

    private PermissionCollection perms;
    private ClassLoader parent;

    public AuxPreloadClassLoader(
	PermissionCollection pc,
	ClassLoader parent)
    {
	super(parent);
	perms = pc;
	this.parent = parent;
    }

    protected PermissionCollection getPermissions(CodeSource cs){
	return perms; // is this right?
    }

    public synchronized Class
    loadClass(String classname, boolean resolve) throws ClassNotFoundException
    {
	Class resultClass;
	resultClass = findLoadedClass(classname);
	if (resultClass == null){
	    String slashname;
	    slashname = classname.replace('.','/');
	    try {
		resultClass = lookupClass(slashname); // from ROM image
	    }catch(Exception e){
		/*DEBUG e.printStackTrace(); */
		resultClass = null;
	    }
	    /* DEBUG if (resultClass!=null) System.out.println("AuxClassLoader found "+classname); */
	}
	/*
	 * WE DON'T DELIGATE. WE LEAVE THAT UP TO OUR CHILD.
	 * WE JUST FAIL.
	 */
	if (resultClass == null)
	    throw new ClassNotFoundException(classname);
	if (resolve)
	    resolveClass(resultClass);
	return resultClass;
    }

    private native Class
    lookupClass(String classname);

}

