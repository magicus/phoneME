/*
 * @(#)lvmsh.java	1.4 06/10/10
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

import java.io.*;

import sun.misc.LogicalVM;

/*
 * lvmsh provides a shell for developers to use.  It is provide basic features
 * like launching applications either synchronously or in background threads,
 * as well as inspection tools to look query the state of the VM.
 * lvmsh extends cvmsh to add LVM administration functionality.
 */

public class lvmsh extends cvmsh
{
    static final String[] helpMsg = {
	"  lvm <class> [args ...]          - runs the specified app in a new LVM",
    };

    void doHelp() {
	super.doHelp();
	printHelp(helpMsg);
    }

    void runLVM(String className, String[] args) {
	LogicalVM lvm = new LogicalVM(className, args, "LVM");
        lvm.start();
    }

    boolean processToken(String token, CmdStream cmd, CVMSHRunnable server) {
	if (token.equals("lvm")) {
	    String classname = getToken(cmd);
	    String[] args = getArgs(cmd);
	    runLVM(classname, args);

	} else {
	    return super.processToken(token, cmd, server);
	}
	return false;
    }


    public static void main(String[] args) {
	lvmsh sh = new lvmsh();
	sh.parseOptions(args);
    }
}
