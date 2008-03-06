/*
 * @(#)ConstantPool.java	1.2 06/08/10
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.ixc;


import java.util.Iterator;
import java.util.HashMap;
import java.util.ArrayList;
import java.io.DataOutputStream;
import java.io.IOException;

/**
 * This utility class is used in generating a stub.  An instance of
 * this class represents the constant pool of the class being generated.
 **/

class ConstantPool {
    // We write the CP out in this order:  classes, methods, fields,
    // nameAndType, strings, longs.

    /** 
     * HashMap[String, String], becomes HashMap[String, Integer]
     * on write.
     **/
    private HashMap classes = new HashMap(11);	

    private ArrayList fields = new ArrayList();		// ArrayList<String[3]>
    private ArrayList methods = new ArrayList();	// ArrayList<String[3]>
    private ArrayList ifMethods = new ArrayList();	// Interface methods
    private ArrayList nameAndType = new ArrayList();	// ArrayList<String[2]>

    /** 
     *This is the list of CONSTANT_String_info things
     * HashMap<String, String>, becomes HashMap<String, Integer> 
     * on write
     **/
    private HashMap stringConstants = new HashMap();
    
    /**
     * HashMap<String, String>, becomes HashMap<String, Integer> 
     * on write
     **/
    private HashMap strings = new HashMap();

    /**
     * HashMap<String, String>, becomes HashMap<String, Integer> 
     * on write
     **/
    private HashMap longConstants = new HashMap();

    void addStringConstant(String s) {
        addString(s);
        stringConstants.put(s, s);
    }

    void addString(String s) {
        strings.put(s, s);
    }

    void addClass(String s) {
        addString(s);
        classes.put(s, s);
    }

    boolean hasClass(String s) {
        return classes.get(s) != null;
    }

    private void addNameAndType(String name, String type) {
        addString(name);
        addString(type);
        nameAndType.add(new String[] { name, type });
    }

    void addLong(Long longValue) {
        longConstants.put(longValue, longValue);
    }

    int lookupString(Object s) {
        Integer i = (Integer) strings.get(s);
        if (i == null) {
            throw new RuntimeException("" + s + " not found in constant pool");
        }
        return i.intValue();
    }

    int lookupStringConstant(Object s) {
        Integer i = (Integer) stringConstants.get(s);
        if (i == null) {
            throw new RuntimeException("string constant " + s 
                    + " not found in constant pool");
        }
        return i.intValue();
    }

    int lookupLongConstant(Object s) {
        Integer i = (Integer) longConstants.get(s);
        if (i == null) {
            throw new RuntimeException("long " + s 
                    + " not found in constant pool");
        }
        return i.intValue();
    }

    int lookupClass(Object s) {
        Integer i = (Integer) classes.get(s);
        if (i == null) {
            throw new RuntimeException("class " + s 
                    + " not found in constant pool");
        }
        return i.intValue();
    }

    int lookupMethod(String className, String name, String type) {
        for (int i = 0; i < methods.size(); i++) {
            String[] el = (String[]) methods.get(i);
            if (className.equals(el[0]) && name.equals(el[1]) 
		    && type.equals(el[2])) 
	    {
                return 1 + classes.size() + fields.size() + i;
            }
        }
        throw new RuntimeException("Method <" + className + ". " +
                name + " : " + type
                + "> not found in constant pool");
    }

    int lookupField(String className, String name, String type) {
        for (int i = 0; i < fields.size(); i++) {
            String[] el = (String[]) fields.get(i);
            if (className.equals(el[0]) && name.equals(el[1]) 
		    && type.equals(el[2])) 
	    {
                return 1 + classes.size() + i;
            }
        }
        throw new RuntimeException("Field <" + className + ". " +
                name + " : " + type
                + "> not found in constant pool");
    }

    int lookupNameAndType(String name, String type) {
        for (int i = 0; i < nameAndType.size(); i++) {
            String[] el = (String[]) nameAndType.get(i);
            if (name.equals(el[0]) && type.equals(el[1])) {
                return 1 + classes.size() + fields.size() + methods.size() 
                    + ifMethods.size() + i;
            }
        }
        throw new RuntimeException("Name and type <" + name + ", " + type
                + "> not found in constant pool");
    }

    void addField(String className, String name, String type) {
        addString(className);
        addNameAndType(name, type);
        fields.add(new String[] { className, name, type });
    }

    void addMethodReference(String className, String name, String type) {
        addString(className);
        addNameAndType(name, type);
        methods.add(new String[] { className, name, type });
    }

    /**
     * Add an interface method reference
     **/
    void addIfMethodReference(String className, String name, String type) {
        addString(className);
        addNameAndType(name, type);
        ifMethods.add(new String[] { className, name, type });
    }

    void write(DataOutputStream dos) throws IOException {
        // Assign an entry number to each class, stringConstant and string 
        // CP entry.  Remmeber, counting starts at 1!

        int num = 1;
        String[] classesArr = new String[classes.size()]; 
	{
            int i = 0;
	    for (Iterator it = classes.keySet().iterator(); it.hasNext(); ) {
                String key = (String) it.next();
                classesArr[i++] = key;
                classes.put(key, new Integer(num++));
            }
        }
  
        num += fields.size() + methods.size() + ifMethods.size() 
                + nameAndType.size();

        String[] stringConstantArr = new String[stringConstants.size()]; 
	{
            int i = 0;
	    for (Iterator it=stringConstants.keySet().iterator(); it.hasNext();)
	    {
                String key = (String) it.next();
                stringConstantArr[i++] = key;
                stringConstants.put(key, new Integer(num++));
            }
        }

        String[] stringArr = new String[strings.size()]; 
	{
            int i = 0;
	    for (Iterator it = strings.keySet().iterator(); it.hasNext() ; ) {
                String key = (String) it.next();
                stringArr[i++] = key;
                strings.put(key, new Integer(num++));
            }
        }

        long[] longArr = new long[longConstants.size()]; 
	{
            int i = 0;
	    for (Iterator it = longConstants.keySet().iterator(); 
                  it.hasNext() ; ) {
	    //for (int i = 0; i < longConstants.size(); i++) {
                Long key = (Long) it.next();
                longArr[i++] = key.longValue();
                longConstants.put(key, new Integer(num));
                //Longs take 2 slots
                num+=2;
            }
        }

        // Now num is the correct constant_pool_count, one greater than
        // the number of entries.  Yes, entires really are numbered
        // 1..(constant_pool_count-1)!
        dos.writeShort(num);

        for (int i = 0; i < classesArr.length; i++) {
            dos.writeByte(7);	// CONSTANT_Class
            dos.writeShort(lookupString(classesArr[i]));
        }
        for (int i = 0; i < fields.size(); i++) {
            String[] sa = (String[]) fields.get(i);
            dos.writeByte(9);	// CONSTANT_Fieldref
            dos.writeShort(lookupClass(sa[0]));
            dos.writeShort(lookupNameAndType(sa[1], sa[2]));
        }
        for (int i = 0; i < methods.size(); i++) {
            String[] sa = (String[]) methods.get(i);
            dos.writeByte(10);	// CONSTANT_Methodref
            dos.writeShort(lookupClass(sa[0]));
            dos.writeShort(lookupNameAndType(sa[1], sa[2]));
        }
        for (int i = 0; i < ifMethods.size(); i++) {
            String[] sa = (String[]) ifMethods.get(i);
            dos.writeByte(11);	// CONSTANT_InterfaceMethodref
            dos.writeShort(lookupClass(sa[0]));
            dos.writeShort(lookupNameAndType(sa[1], sa[2]));
        }
        for (int i = 0; i < nameAndType.size(); i++) {
            String[] sa = (String[]) nameAndType.get(i);
            dos.writeByte(12);	// CONSTANT_NameAndtype
            dos.writeShort(lookupString(sa[0]));
            dos.writeShort(lookupString(sa[1]));
        }
        for (int i = 0; i < stringConstantArr.length; i++) {
            dos.writeByte(0x8);	// CONSTANT_String_info
            dos.writeShort(lookupString(stringConstantArr[i]));
        }
        for (int i = 0; i < stringArr.length; i++) {
            dos.writeByte(1);	// CONSTANT_Utf8
            dos.writeUTF(stringArr[i]);
        }
        for (int i = 0; i < longArr.length; i++) {
            dos.writeByte(0x5);	// CONSTANT_Long
            dos.writeLong(longArr[i]);
        }
    }
}
