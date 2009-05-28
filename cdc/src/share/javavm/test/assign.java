/*
 * @(#)assign.java	1.11 06/10/10
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
import java.util.*;

//
// A few kinds of simple writes, to test out write barriers
//
public class assign 
{
    int ix;
    Object ax, ay, az;
    Vector v;
    
    assign() 
    {
    }
    
    public int doVirtInd(Vector v, int i) 
    {
	return i + v.size() + v.size();
    }

    public int doVirt(int j)
    {
	return j + doVirtInd(new Vector(), j);
    }
	
    public void doitObjArr(Object[] arr) 
    {
	arr[3] = this;
    }
    
    public void doitIntArr(int[] arr) 
    {
	int x = -4;
	
	arr[3] = x;
    }
    
    public void doitField()
    {
	assign a = new assign();
	a.ax = null;
	a.ay = null;
	a = new assign();
	a.ax = null;
	a.ay = null;
	
	ix = 4;
	ax = null;
	ay = null;
	az = null;
	
    }
    
    public void doitFloat()
    {
	float a, b, c, d;
	a = (float)1.0;
	b = (float)2.0;
	c = (float)3.0;
	d = a + a;
	b = a + c;
	System.err.println("a="+a+" b="+b+" c="+c+" d="+d);
    }
    
    public static void main(String[] args) 
    {
	assign a = new assign();
	Object[] arr = new Object[6];
	int[] iarr = new int[6];
	a.doitObjArr(arr);
	a.doitIntArr(iarr);
	a.doitField();
	a.doitFloat();
    }
    
}
