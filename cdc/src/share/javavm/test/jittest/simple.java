/*
 * @(#)simple.java	1.14 06/10/10
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

class simple{

public int simple( int a[], int maxI ){
    int hadException = 0;
    int sum = 0;
    try {
	for ( int i = 0; i < maxI; i ++ ){
	    sum = sum + a[i];
	    a[i] = 0;
	}
    } catch ( NullPointerException e  ){
	hadException = 1;
    }
    return (hadException==1)? -1 : sum;
}

public void ExceptionTest( int a[], int maxI ) throws Exception {
    boolean flag = false;
    int hadException = 0;
    int hadTry = 0;
    int sum = 0;
    try {
        for ( int i = 0; i < maxI; i ++ ){
            sum = sum + a[i];
            a[i] = 0;
            try {
                sum = 5;
                hadTry = 5;
            }
            catch (Exception e4) {
                flag = false;
            }
            hadException = 1;
        }
        try {
            sum = 6;
            hadTry = 6;
        }
        catch (Exception e4) {
            flag = false;
        }
        hadException = 2;
    } catch ( NullPointerException e){
        try {
            sum = 4;
            hadTry = 4;
        }
        catch (Exception e4) {
            flag = true;
        }
        hadException = 3;
    } catch ( ArrayIndexOutOfBoundsException e3){
        hadException = 4;
    } catch ( Exception e3 ){
        hadException = 5;
        flag = false;
        throw e3;
    }
}


public void arraySet( int a[], int v ){
    for ( int i = 0; i < a.length; i++ ){
	a[i] = v;
	a[i+v+1] = 0;
	i = a[i+2];
    }
}

public void testTypeConvert(int arr[]) {
    int i = 0;
    float f = (float)1.0;
    double d = 1.0;
    long l = 1L;
    short s = 10;
    char c = 'a';
    byte b = 'a';

    f = i;
    l = i;
    d = i;
    b = (byte)i;
    c = (char)i;
    s = 20;
    i = (int)l;
    f =l;
    d = l;
    i = (int)f;
    l = (long)f;
    d = f;
    i = (int)d;
    f = (float)d;
    l = (long)d;
}

public void testConst(int arr[]) {
    int i1 = 0;
    int i2 = 1;
    float f1 = (float)1.0;
    float f2 = (float)2.0;
    double d1 = 1.0;
    double d2 = 2.0;
    long l1 = 1L;
    long l2 = 2L;

    i1 = i1 + i2;
    i2 = i1 + 10;
    i1 = arr.length;
    f1 = f1 + f2;
    f2 = f1 + (float)3.0;
    d1 = d1 + d2;
    d2 = d1 + 3.0;
    l1 = l1 + l2;
    l2 = l1 + 10;

}

public void testNewStringObj(String argv[]) {
    String s = "abc";
    s = "test" + "WORD" + s;
    s = argv[argv.length];
    s = new String("def");
    System.out.print(true);
    System.out.print("*TEST FAILURE: ");
    System.out.println(s); 
    System.out.println("*Number of command line arguments: " + argv.length);
}

public void testCast() {
    simple names  = new simple();
    Object o = new Object();
    boolean b;
    if (o instanceof simple) {
	b = true;
    }
    o = (Object)names;
    names = (simple)o;
}

static public int testCMP() {
    long l1 = 1L;
    long l2 = 2L;
    float f1 = (float)3.0;
    float f2 = (float)4.0;
    double d1 = 5.0;
    double d2 = 6.0;
    short a = 0;
    boolean b = false;

    if (l1 > l2) {
        a = 1;
    }
    else if (l1 == l2) {
        a = 2;
    }
    else if (l1 < l2) {
        a = 3;
    }
    else if (l1 <= l2) {
        a = 4;
    }
    else if (b = l1 >= l2) {
        a = 5;
    }
    else if (l1 > 0) {
        a = 6;
    }
    if (f1 > f2) {
        f1 = 1;
    }
    else if (f1 == f2) {
        f1 = 2;
    }
    else if (f1 < f2) {
        f1 = 3;
    }
    else if (f1 <= f2) {
        f1 = 4;
    }
    else if (f1 >= f2) {
        f1 = 5;
    }
    else if (b = (f1 > 0)) {
        f1 = 6;
    }
    return a;
}

public int testTableSwitch( char c ){

        switch ( (int)c - '0' ){
        case 0: c = 'a';
		break;
        case 1:
                return 1;
        case 2:
        case 3:
                return 3;
        case 4:
                return 4;
        /* case 5: DON'T LIKE 5 */

        case 6:
                return 6;
        case 7:
                return 7;
        case 8:
                return 8;
        case 9:
                return 9;
        default:
                return -1;
        }
	c = 'b';
	return 10;
}

public void testLookupSwitch() {
    int a = 50;
    int c = 10;
    int b;
        switch (a) {
          case 5: b = 0;
                break;
          case 6: b = 1;
                break;
          case 7: b = 2;
                break;
          default: b = 10;
                break;
        }

        a++;
        ++b;

        switch (b) {
          case 5: a = 0;
                break;
          case 6: a = 1;
                break;
          case 20: a = 20; break;
          case 21: a = 25; break;
          case 30: a = 30; break;
          default: a = 40;
                break;
        }
	a = b + c;
}

static void printString( String a ){
    System.out.println(a);
}

public static void helloWorld( String a[] ){
    for (int i=0; i < a.length; i++ ){
        printString( a[i] );
    }
}

static int findMax( int i, int j ){
    if ( i > j ) return i;
    return j;
}

public static int
positiveValue( int v ){
    return findMax( v, 0 );
}

public void switch_method(String args[]) {
    int a = 50;
    int c = 10;
        int b;
        switch (a) {
          case 5: b = 0;
                break;
          case 6: b = 1;
                break;
          case 7: b = 2;
                break;
          default: b = 10;
                break;
        }

        a++;
        ++b;

        switch (b) {
          case 5: a = 0;
                break;
          case 6: a = 1;
                break;
          case 20: a = 20; break;
          case 21: a = 25; break;
          case 30: a = 30; break;
          default: a = 40;
                break;
        }
}


}
