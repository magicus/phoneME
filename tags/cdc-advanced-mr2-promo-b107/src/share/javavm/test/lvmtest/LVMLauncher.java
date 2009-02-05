/*
 * @(#)LVMLauncher.java	1.6 06/10/10
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

import sun.misc.LogicalVM;

public class LVMLauncher {

    private static final String[] usageMessage = {
	"",
	"usage: <cvm> LVMLauncher [[num-lvm [intrl-msec [timeout-msec]]] "
	+ "MainClass [args...]",
	"",
	"   ex: <cvm> LVMLauncher 5 1000 HelloWorld",
	"",
	"  Launches specified MainClass in multiple Logical VMs specified",
	"  by the num-lvm argument.  Each LVM starts off with intrl-msec",
	"  milli-seconds interval. One of the LVM gets forcibly terminated",
	"  after time-out seconds.[this part is not implemented yet]",
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

	LogicalVM lvmToKill = null;

	for (int i = 0; i < lvmNum; i++) {
	    String lvmName = "#" + (i + 1);
	    System.out.println("*** Starting LogicalVM " + lvmName
		+ ": " + className);

	    LogicalVM lvm = new LogicalVM(className, as, lvmName);
	    lvm.start();

	    /* Pick one victim for the termination test later */
	    if (i == (lvmNum - 1) / 2) {
		lvmToKill = lvm;
	    }

	    if (intvlMSec >= 0) {
		try {
		    Thread.sleep(intvlMSec);
		} catch (InterruptedException e) {
		    System.err.println("Exception caught in sleep(): " + e);
		}
	    }
	}

	if (timeOut >= 0) {
	    int timeOutSec = timeOut / 1000;
	    timeOut -= timeOutSec * 1000;
	    try {
		for (int i = 0; i < timeOutSec * 2; i ++) {
		    if (i != 0) {
			Thread.sleep(500);
		    }
		    System.out.print(".");
		}
		System.out.println();
		Thread.sleep(timeOut);
	    } catch (Exception e) {	}

	    System.out.println("*** Terminating Logical VM: " + 
		lvmToKill.getName());
	    lvmToKill.exit(0);
	    try {
		lvmToKill.join();
	    } catch (Exception e) { }
	    System.out.println("*** Terminated Logical VM: " +
		lvmToKill.getName());

	}
    }

    private static void printUsage() {
	for (int i = 0; i < usageMessage.length; i++) {
	    System.err.println(usageMessage[i]);
	}
    }
}
