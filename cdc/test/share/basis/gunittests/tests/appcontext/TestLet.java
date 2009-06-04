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


package tests.appcontext ;

/**
 * <code>TestLet</code> are isolated instances that performs a test in 
 * a seperate <code>AppContext</code>. Extend this class and provide 
 * concrete implementation for <code>doTest()</code> which executes some
 * test code and returns a result.
 */
public abstract class TestLet implements Runnable {
    private static int id = -1 ;
    Thread thread ;
    Object result ;

    static synchronized int getID() {
        return ++ id ;
    }

    public TestLet() {
        this(true);
    }
                                                                                
    TestLet(boolean autoStart) {
        this.thread = new Thread(new ThreadGroup("TestLet "+getID()),this);
        if (autoStart) {
            this.start();
        }
    }

    public void start() {
        this.thread.start();
    }

    public void run() {
        // register this thread as a new app context
        sun.awt.SunToolkit.createNewAppContext();
        synchronized(this) {
            this.result = null;
            this.result = this.doTest();
            this.notifyAll();
        }
    }

    public Object getResult() {
        synchronized(this) {
            while ( this.result == null ) {
                try {
                    this.wait();
                }
                catch (Exception ex) {}
            }
        }
                                                                                
        return this.result;
    }
                                                                                
    protected abstract Object doTest() ;
}
