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

/*
 * @(#)MIDlet.java	1.4	@(#)
 */
package javax.microedition.midlet;

/*
 * Just barely a skeleton of the class MIDlet.
 * All I needed for this demo was startApp, so that's about
 * all there is here.
 * In a real implementation, this will be a full implementation
 * of course.
 */
public abstract class
MIDlet {
    boolean appIsDestroyed = false;

    public abstract void startApp();
    public abstract void destroyApp(boolean unconditional);
    public final void notifyDestroyed(){ appIsDestroyed = true; }

}
