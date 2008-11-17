/*
 * @(#)Inet4AddressImpl.java	1.5 06/10/10
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
package java.net;

/*
 * Package private implementation of InetAddressImpl for IPv4.
 *
 * @since 1.4
 */
class Inet4AddressImpl implements InetAddressImpl {
    public native String getLocalHostName() throws UnknownHostException;
    public native byte[][]
        lookupAllHostAddr(String hostname) throws UnknownHostException;
    public native String getHostByAddr(byte[] addr) throws UnknownHostException;

    public synchronized InetAddress anyLocalAddress() {
        if (anyLocalAddress == null) {
            anyLocalAddress = new Inet4Address(); // {0x00,0x00,0x00,0x00}
            anyLocalAddress.hostName = "0.0.0.0";
        }
        return anyLocalAddress;
    }

    public synchronized InetAddress loopbackAddress() {
        if (loopbackAddress == null) {
            byte[] loopback = {0x7f,0x00,0x00,0x01};
            loopbackAddress = new Inet4Address("localhost", loopback);
        }
        return loopbackAddress;
    }

    private InetAddress      anyLocalAddress;
    private InetAddress      loopbackAddress;
}

