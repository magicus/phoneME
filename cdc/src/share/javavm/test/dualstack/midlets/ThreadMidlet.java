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
 * @(#)ThreadMidlet.java	1.6	06/10/10
 * Skeleton midlet.
 *
 * Make sure we are granted permission to diddle threads.
 */
public class ThreadMidlet extends javax.microedition.midlet.MIDlet{
    class ThreadMidletInner implements Runnable{
	public void run(){
	    System.out.println("ThreadMidlet inner:");
	    for( int i=0; i<10 ; i++){
		try {
		    System.out.print('\t');
		    System.out.println(i);
		    Thread.sleep(1000);
		} catch (Exception e){
		    System.out.println("ThreadMidletInner interrupted!");
		}
	    }
	    System.out.println("ThreadMidletInner exiting");
	}
    }

    // outer
    public void startApp(){
	System.out.println("ThreadMidlet outer:");
	Thread t = new Thread(new ThreadMidletInner(),"ThreadMidlet inner thread");
	t.setPriority(Thread.MIN_PRIORITY);
	t.run();
	try {
	    for(int i=0; i<50; i++){
		Thread.sleep(100);
		t.interrupt();
	    }
	}catch(Exception e){
	    System.err.println("ThreadMidlet outer interrupted while sleeping: ");
	    e.printStackTrace();
	}
	try {
	    t.join();
	}catch(Exception e){
	    System.err.println("ThreadMidlet outer interrupted while waiting: ");
	    e.printStackTrace();
	}
	System.out.println("ThreadMidlet outer exiting");
	notifyDestroyed();
    }

    public static void main(String ignore[]){
	new ThreadMidlet().startApp();
    }

    public void destroyApp(boolean ignore){}
}
