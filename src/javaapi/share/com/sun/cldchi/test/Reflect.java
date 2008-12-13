/*
 *   
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

package com.sun.cldchi.test;

import com.sun.cldc.isolate.Isolate;

public class Reflect {

    private Object obj;

    private Reflect() {
        obj = new Object();
    }

    public static Reflect get(Isolate iso, String className, String fieldName) {
        Reflect r = new Reflect();
        r.obj = getStaticField(iso.id(), className, fieldName);
        return r;
    }

    public Reflect getField(String fieldName) {
        Reflect r = new Reflect();
        r.obj = getInstanceField(this.obj, fieldName);
        return r;
    }


    /* Accessors for static fields */
    public static int getStaticIntValue(Isolate iso, String className, String fieldName) {
        return getStaticIntField(iso.id(), className, fieldName);
    }

    public static boolean getStaticBooleanValue(Isolate iso, String className, String fieldName) {
        return getStaticBooleanField(iso.id(), className, fieldName);
    }

    public static String getStaticStringValue(Isolate iso, String className, String fieldName) {
        return (String)(getStaticField(iso.id(), className, fieldName));
    }


    /* Accessors for instance fields */
    public int getIntValue(String fieldName) {
        return getIntField(this.obj, fieldName);
    }

    public boolean getBooleanValue(String fieldName) {
        return getBooleanField(this.obj, fieldName);
    }

    public String getStringValue(String fieldName) {
        return (String)getInstanceField(this.obj, fieldName);
    }

    public Object getObjectValue() {
        return this.obj;
    }


    /* Native methods */
    private static native Object getStaticField(int taskId, String className, String fieldName);
    private static native int getStaticIntField(int taskId, String className, String fieldName);
    private static native boolean getStaticBooleanField(int taskId, String className, String fieldName);

    private native Object getInstanceField(Object obj, String fieldName);
    private native int getIntField(Object obj, String fieldName);
    private native boolean getBooleanField(Object obj, String fieldName);
}
