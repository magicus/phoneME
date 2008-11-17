/*
 * @(#)MyClassPath.java	1.2 06/10/11
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

package sun.tools;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.security.ProtectionDomain;
import java.security.CodeSource;
import java.net.URL;
import java.net.URLClassLoader;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public class MyClassPath {
    public static void main(String[] argv) throws Throwable {

	if (argv.length == 0) {
	    System.err.println("usage: classname [args...]");
	    return;
	}

	Class me = sun.tools.MyClassPath.class;
	URL url = me.getProtectionDomain().getCodeSource().getLocation();
	URL[] urls = new URL[] {url};

	ClassLoader filter = new ClassLoader() {
	    protected synchronized Class loadClass(String name, boolean resolve)
		throws ClassNotFoundException
	    {
		if (name.startsWith("java.")) {
		    return super.loadClass(name, resolve);
		} else {
		    throw new ClassNotFoundException(name);
		}
	    }
	};

	ClassLoader l = new URLClassLoader(urls, filter);

	{
	    Class mainClass = l.loadClass(argv[0]);
	    String[] args = new String[argv.length - 1];
	    System.arraycopy(argv, 1, args, 0, argv.length - 1);
	    Class [] argsType = {args.getClass()};
	    Method mainMethod = mainClass.getMethod("main", argsType);
	    Object [] args2 = {args};
	    try {
		mainMethod.invoke(null, args2);
	    } catch (InvocationTargetException i) {
		throw i.getTargetException();
	    }
	}
    }
}
