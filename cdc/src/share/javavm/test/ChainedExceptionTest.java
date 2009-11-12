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

//
// A simple test for chained Exception
//

public class ChainedExceptionTest {
    public static void main(String args[]) {
        ChainedExceptionTest test = new ChainedExceptionTest();
        try {
            test.run();
        } catch (ChainedException ce) {
            ce.printStackTrace();
        }
    }

    public void run() throws ChainedException {
        try {
            Object o = null;
            o.toString();
        } catch (Exception e) {
            throw new ChainedException(e);
        }
    } 
}

class ChainedException extends Throwable {
  
    ChainedException() {
        super();
    }

    ChainedException(Throwable cause) {
        super(cause);
    }
}
