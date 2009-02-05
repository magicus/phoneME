/*
 * @(#)PlainLauncher.java	1.4 06/10/10
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

package lvmtest;

import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public class PlainLauncher {

    private static final String[] usageMessage = {
	"",
	"usage: <cvm> PlainLauncher [[num-lvm [intrl-msec]] "
	+ "MainClass [args...]",
	"",
	"   ex: <cvm> PlainLauncher 5 1000 HelloWorld",
	"",
    };

    private static final int DefaultLVMNum = 3;
    private static final int DefaultIntvlMSec = -1;
    private static final int DefaultTimeOut = -1;

    public static void main(String[] args) {

	int lvmNum = DefaultLVMNum;
	int intvlMSec = DefaultIntvlMSec;
	int timeOut = DefaultTimeOut;
	int argp = 0;

	try {
	    lvmNum = Integer.valueOf(args[argp]).intValue();
	    argp++;
	    intvlMSec = Integer.valueOf(args[argp]).intValue();
	    argp++;
	    timeOut = Integer.valueOf(args[argp]).intValue();
	    argp++;
	} catch (NumberFormatException e) {
	    /* Non-number argument encountered. Keep going */
	} catch (ArrayIndexOutOfBoundsException e) {
	    /* There should be at least one non-number argument */
	    printUsage();
	    System.exit(-1);
	}

	if (lvmNum <= 0 || args.length <= argp) {
	    printUsage();
	    System.exit(-1);
	}

	String className = args[argp++];

	String[] as = new String[args.length - argp];
	for (int i = 0; i < args.length - argp; i++) {
	    as[i] = args[argp + i];
	}

	for (int i = 0; i < lvmNum; i++) {
	    String lvmName = "#" + (i + 1);
	    System.out.println("*** Starting Plain launcher " + lvmName
		+ ": " + className);

	    Launcher la = new Launcher(className, as, lvmName);
	    la.start();

	    if (intvlMSec >= 0) {
		try {
		    Thread.sleep(intvlMSec);
		} catch (InterruptedException e) {
		    System.err.println("Exception caught in sleep(): " + e);
		}
	    }
	}

	if (timeOut >= 0) {
	    System.out.println("*** Time-out " + timeOut + " is ignored");
	}
    }

    private static class Launcher extends Thread {
	private String className;
	private String[] args;
	private String name;
	
	Launcher(String cn, String[] as, String n) {
	    className = cn;
	    args = as;
	    name = n;
	}
	public void run() {
	    try {
		Class cls = Class.forName(className);
		Class[] argClses = {String[].class};
		Method mainMethod = cls.getMethod("main", argClses);
		Object[] argObjs = {args};
		mainMethod.invoke(null, argObjs);
	    } catch (InvocationTargetException ite) {
		ite.getTargetException().printStackTrace();
	    } catch (Exception e) {
		e.printStackTrace();
	    }
	}
    }

    private static void printUsage() {
	for (int i = 0; i < usageMessage.length; i++) {
	    System.err.println(usageMessage[i]);
	}
    }
}
