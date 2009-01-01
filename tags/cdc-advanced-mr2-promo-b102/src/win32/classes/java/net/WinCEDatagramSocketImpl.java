/*
 * @(#)WinCEDatagramSocketImpl.java	1.6 06/10/10
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

package java.net;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InterruptedIOException;

class WinCEDatagramSocketImpl extends PlainDatagramSocketImpl
{
    private DatagramPacket peekPacket = null;
    private int bufLength = 0;

    protected synchronized void create() throws SocketException {
	super.create();
	bufLength = ((Integer)getOption(SO_RCVBUF)).intValue();
    }

    protected synchronized int peek(InetAddress i) throws IOException {
	if (peekPacket == null) {
	    int len = bufLength;
	    DatagramPacket p = new DatagramPacket(new byte[len], len);
	    receive(p);
	    peekPacket = p;
	}
	i.address = peekPacket.getAddress().address;
	i.family = peekPacket.getAddress().family;
	return peekPacket.getPort();
    }

    protected synchronized int peekData(DatagramPacket pd) throws IOException {
	if (peekPacket == null) {
	    int len = bufLength;
	    DatagramPacket p = new DatagramPacket(new byte[len], len);
	    receive(p);
	    peekPacket = p;
	}
	int peeklen = Math.min(pd.getLength(), peekPacket.getLength());
	System.arraycopy(peekPacket.getData(), peekPacket.getOffset(),
			 pd.getData(), pd.getOffset(), peeklen);
	pd.setLength(peeklen);
	pd.setAddress(peekPacket.getAddress());
	pd.setPort(peekPacket.getPort());
	return peekPacket.getPort();
    }

    protected synchronized void receive(DatagramPacket p) throws IOException {
	if (peekPacket == null) {
	    super.receive(p);
	} else {
	    p.setPort(peekPacket.getPort());
	    p.setAddress(peekPacket.getAddress());
	    int len = Math.min(peekPacket.getLength(), p.getLength());
	    System.arraycopy(peekPacket.getData(), peekPacket.getOffset(),
			     p.getData(), p.getOffset(),
			     len);
	    p.setLength(len);
	    peekPacket = null;
	}
    }

    public void setOption(int optID, Object o) throws SocketException {
	super.setOption(optID, o);
	if (optID == SO_RCVBUF) {
	    bufLength = ((Integer)getOption(SO_RCVBUF)).intValue();
	}
    }

}
