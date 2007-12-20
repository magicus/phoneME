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

package com.sun.cdc.io.j2me.jcrmi;

import java.rmi.*;

public interface Remote1 extends Remote {

    public void setChannelId(short id) throws RemoteException;
    public short getChannelId() throws RemoteException;
    public short getChannel() throws RemoteException;

    public boolean getBoolean(boolean v) throws RemoteException;
    public byte getByte(byte v) throws RemoteException;
    public short getShort(short v) throws RemoteException;
//    public int getInt(int v) throws RemoteException;

    public boolean[] getBooleanArray(boolean[] v) throws RemoteException;
    public byte[] getByteArray(byte[] bv) throws RemoteException;
    public short[] getShortArray(short[] v) throws RemoteException;
    //public int[] getIntArray(int[] v) throws RemoteException;

    public void voidMethod() throws RemoteException;

    public void throwException(short code) throws Throwable;
    public void throwSubclass(short code) throws Throwable;

    public byte[] getNewArray(short length) throws RemoteException;

    public Remote1 getReference1() throws RemoteException;
    public Remote getReference2() throws RemoteException;
}
