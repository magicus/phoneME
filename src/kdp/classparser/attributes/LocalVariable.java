/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package kdp.classparser.attributes;
import kdp.Log;

public class LocalVariable
{
    private String name;
    private String type ;
    private int length;
    private int slot;
    private long codeIndex;
    
    public LocalVariable(String name, String type, int codeIndex, 
                         int length, int slot ){
        this.name = name;
        this.type = type;
        this.codeIndex = codeIndex;
        this.length = length;
        this.slot = slot;
    }

    public String getName(){
        return name;
    }
    
    public String getType(){
        return type;   
    }
    
    public int getLength(){
        return length;
    }
    
    public int getSlot() { 
        return slot;
    }
    
    public long getCodeIndex(){
        return codeIndex;
    }
    
    public void print(){
        Log.LOGN(5, "Name: " + name + "\nClass: " + type);  
        Log.LOGN(5, "CodeIndex: " + codeIndex + "\nLength: " + length);
        Log.LOGN(5, "Slot: " + slot);
    }
}
