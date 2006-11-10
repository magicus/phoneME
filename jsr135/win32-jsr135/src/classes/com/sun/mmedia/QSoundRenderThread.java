/*
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
package com.sun.mmedia;

class QSoundRenderThread extends Thread
{
    private int gm;
    private boolean keepRendering;
    
    QSoundRenderThread(int globMan)
    {
        gm = globMan;
        keepRendering = true;
    }
    
    private native int nRender(int globMan);
    
    public void run()
    {
        int s;
        while(keepRendering)
        {        
            s = nRender(gm);
            if(s > 0)
                try{Thread.sleep(s);}catch(Exception e){}
        }
    }
    
    /*
     * This is CLDC 1.1 method, are we CLDC 1.0 compliant ?
     * moreover it is not used anywhere explicitely ...
     *
     * Without this unused method (public void interrupt())
     * JSR135 MMAPI-TCK MIDIControl#SetProgram test case
     * fails (MIDIControl2008 for "device://midi")...
     * I commented it in r956, but now reverted to r955 due to this reason ! 
     */
    public void interrupt()
    {
        keepRendering = false;
        super.interrupt();
    }
    
}



