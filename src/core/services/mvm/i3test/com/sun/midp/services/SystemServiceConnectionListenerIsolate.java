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

package com.sun.midp.services;

import com.sun.midp.links.*;
import java.io.*;
import com.sun.midp.security.*;
import com.sun.cldc.isolate.*;

/**
 * Isolate created in TestSystemService test.
 */
public class SystemServiceConnectionListenerIsolate  {
    private static SecurityToken token = SecurityTokenProvider.getToken();

    class ConnectionListener implements SystemServiceConnectionListener {
        private SystemServiceConnection con = null;
        private boolean done = false;

        ConnectionListener(SystemServiceConnection con) {
            this.con = con;
        }

        public void onMessage(SystemServiceMessage msg) {
            try {
                // read received string
                String testString = msg.getDataInput().readUTF();

                // convert string to upper case and sent it back to service
                msg = SystemServiceMessage.newMessage();
                msg.getDataOutput().writeUTF(testString.toUpperCase());
                con.send(msg);

                // we are done
                synchronized (this) {
                    done = true;
                    notifyAll();
                }
            } catch (Throwable t) {
                System.err.println("Exception: " + t);
                t.printStackTrace();
            }
        }

        public void onConnectionClosed() {
        }

        void await() {
            try {
                synchronized (this) {
                    while (!done) {
                        wait();
                    }
                }
            } catch (InterruptedException ignore) { }
        }        
    }

    public static void main(String[] args) 
        throws ClosedLinkException, 
               SystemServiceConnectionClosedException,
               InterruptedIOException, 
               IOException {

        SystemServiceConnectionListenerIsolate is = 
            new SystemServiceConnectionListenerIsolate();

        is.run();
    }

    private void run() 
        throws ClosedLinkException,
               SystemServiceConnectionClosedException,
               InterruptedIOException, 
               IOException {

        Link[] isolateLinks = LinkPortal.getLinks();
        NamedLinkPortal.receiveLinks(isolateLinks[0]);

        // request service
        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);

        SystemServiceConnection con = null;
        con = serviceRequestor.requestService(
                TestSystemService.SERVICE_ID);

        // send an empty string to service to start messages exchange
        SystemServiceMessage msg = SystemServiceMessage.newMessage();
        msg.getDataOutput().writeUTF("");
        con.send(msg);

        // set listener and wait for exchange to complete
        ConnectionListener l = new ConnectionListener(con);
        con.setConnectionListener(l);
        l.await();

        Isolate cur = Isolate.currentIsolate();
        cur.exit(0);        
    }
}


