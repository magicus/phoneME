/*
 *
 *
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

package com.sun.midp.wma;

import com.sun.midp.push.reservation.*;


public class PushConnectionsPool implements Runnable {

    static java.util.List ReservationsList = new java.util.LinkedList();
    static PushConnectionsPool This = new PushConnectionsPool();

    static Thread listenerThread = null;

    static void startListenerThreadIfNeed() {
	synchronized(This) {
	    if (listenerThread == null) {
		listenerThread = new Thread(This);
		listenerThread.start();
	    }
	}
    }

    public static void addReservation(ConnectionReservationImpl reservation) 
            throws java.io.IOException, IllegalArgumentException {

	synchronized (ReservationsList) {

            //check duplicates
            java.util.ListIterator iter = ReservationsList.listIterator();
            while (iter.hasNext()) {
                ConnectionReservationImpl reservation1 = (ConnectionReservationImpl)iter.next();
                if (reservation1.getPort()  == reservation.getPort() &&
                    reservation1.getFilter().equals(reservation.getFilter())) {
                    throw new java.io.IOException("connection already registered");
                }
            }

	    addPushPort(reservation.getPort(), reservation.getFilter());
	    ReservationsList.add(reservation);
	    startListenerThreadIfNeed();
	}
    }

    public static void removeReservation(ConnectionReservationImpl reservation) {
	synchronized(ReservationsList) {
	    ReservationsList.remove(reservation);
	    if (ReservationsList.size() == 0) {
		listenerThread.interrupt();
		listenerThread = null;
	    }
	    removePushPort(reservation.getPort());
	}
    }

    private static native void addPushPort(int port, String pushFilter) 
                               throws java.io.IOException, IllegalArgumentException;
    private static native int  removePushPort(int port) throws IllegalStateException;
    public  static native int  hasAvailableData(int port);
    private        native int  waitPushEvent() throws java.io.InterruptedIOException;

    private void notifyPushConnections(int port) {
	synchronized(ReservationsList) {
	    java.util.ListIterator iter = ReservationsList.listIterator();
	    while (iter.hasNext()) {
		ConnectionReservationImpl reservation = (ConnectionReservationImpl)iter.next();
		if (reservation.getPort() == port) {
                    if (0 != hasAvailableData(port)) {
                        reservation.notifyDataAvailable();
                    }
                }
            }
        }
    }

    public void run() {
        while(true) {
            try {
	        int smsPort = waitPushEvent();
                notifyPushConnections(smsPort);
	    } catch (java.io.InterruptedIOException iioe) {
                return;
	    }
        }
    }
}
