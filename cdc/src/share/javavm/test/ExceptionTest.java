/*
 * @(#)ExceptionTest.java	1.14 06/10/10
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
import sun.misc.CVM;

public class ExceptionTest {
    static int sfoo = 3;
    static final int sbar = 6;
    static int sfoobar;
    static Throwable clinitException;

    static {
	try {
	    throw new MyException();
	} catch (MyException e) {
	    e.printStackTrace();
	    clinitException = e;
	}
    }

    public static void main(String argv[]) throws Exception {
	clinitException.printStackTrace();
	int a = 1;
	RuntimeException e1 = new RuntimeException();
	e1.printStackTrace();
	
	try {
	    sfoobar = recurse(20);
	    //sfoobar = recurse(10000);
	} catch (StackOverflowError e) {
	    e.printStackTrace();
	}
	
	exc(1);
	exc(2);
	exc(3);
	exc(4);
	try {
	    try {
		try {
		    exc3();
		} catch ( RuntimeException e ) {
		    e.printStackTrace();
		    sfoobar = -2;
		} finally {
		    sfoobar = -3;
		}
	    } finally {
		sfoobar = -4;
	    }
	} catch ( Throwable e ) {
	    e.printStackTrace();
	}
	try {
	    try {
		if (a == 1) {
		    throw(new RuntimeException());
		} else if (a == 2) {
		    throw(new Exception());
		} else if (a == 3) {
		    throw(new Throwable());
		}
	    } catch ( RuntimeException e ) {
		e.printStackTrace();
		a = 2;
	    } catch ( Exception e ) {
		e.printStackTrace();
		a = 3;
	    }
	} catch ( Throwable e ) {
	    e.printStackTrace();
	    a = 1;
	}
    }
    
    static int exc(int a) {
	try {
	    if (a == 1) {
		throw(new RuntimeException());
	    } else if (a == 2) {
		throw(new Exception());
	    } else if (a == 3) {
		throw(new Throwable());
	    }
	} catch ( RuntimeException e ) {
	    e.printStackTrace();
	    return -1;
	} catch ( Exception e ) {
	    e.printStackTrace();
	    return -2;
	} catch ( Throwable e ) {
	    e.printStackTrace();
	    return -3;
	}
	return -4;
    }
        
    static void exc3() throws Exception {
	sfoo ++;
	if (sfoo > 5) {
	    throw(new Exception());
	}
	else
	    exc3();
    }
    
    static int recurse(int count) {
	if (count > 0)
	    return count + recurse(count - 1);
	return 0;
    }
}

class MyException extends Exception {
    public MyException() {
	super();
	printStackTrace();
    }
}
