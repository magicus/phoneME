/*
 * @(#)TimerThreadTest.java	1.6 06/10/10
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

package foundation;

import java.util.Timer;

public class TimerThreadTest {

    public static void main(String argv[]) {
	Timer t = new Timer();
	t.cancel();

	System.out.println("Starting three timers.");

	Timer t1 = new Timer(true);
	Timer t2 = new Timer(true);
	Timer t3 = new Timer(true);

	System.out.println("Cancelling two timers.");

	t1.cancel();
	t3.cancel();

	// t2 is an uncanceled daemon thread
	// The test passes if the thread exits when the VM shuts down
	// If the thread keeps running, the test has failed

	System.out.println("Returning from main.");
    }

}
