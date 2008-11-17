/*
 * @(#)multiJoin.java	1.10 06/10/10
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

package jittest;

class X {
    public int x = 2;
    public int foo() {return 1;}
}

class multiJoin{
    boolean b;
    int x;
    X a;

boolean foo() {return b;}

void test1() {
    a.x = (a.foo() == 1? 1 : 0);
}

void test2() {
    this.x = this.foo() ? 1 : 0;
}

int
doubleIfThen( boolean a, int b, boolean c, int d ){
    int sum;
    sum = (a ? b : 0) + ( c ? d : 0 );
    return sum;
}

int
computeParamList( boolean control[], boolean alist[], int blist[] ){

    return doubleIfThen(
        control[0]?alist[0]:false,
        control[1]?blist[0]:0,
        control[2]?alist[1]:false,
        control[3]?blist[1]:0);
}

}
