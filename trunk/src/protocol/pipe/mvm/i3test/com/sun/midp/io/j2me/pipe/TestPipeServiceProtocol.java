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
package com.sun.midp.io.j2me.pipe;

import com.sun.midp.io.j2me.pipe.serviceProtocol.PipeServiceProtocol;
import com.sun.cldc.isolate.Isolate;
import com.sun.cldc.isolate.IsolateStartupException;
import com.sun.midp.i3test.TestCase;
import com.sun.midp.links.ClosedLinkException;
import com.sun.midp.links.Link;
import com.sun.midp.links.LinkMessage;
import com.sun.midp.links.LinkPortal;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.services.SystemServiceLinkPortal;
import com.sun.midp.services.SystemServiceManager;
import java.io.IOException;
import java.io.InterruptedIOException;

public class TestPipeServiceProtocol extends TestCase {

    private static SecurityToken token = SecurityTokenProvider.getToken();

    public static class TestIsolate {

        private static SecurityToken token = SecurityTokenProvider.getToken();

        public static void main(String[] args) {
            
            SystemServiceLinkPortal.linksObtained(LinkPortal.getLinks());
            
            try {
                PipeServiceProtocol client = PipeServiceProtocol.getService(token);
                client.bindClient("SERVER", "1.0");
                Link lIC = client.getInboundLink();
                Link lOC = client.getOutboundLink();
                LinkMessage msgIn = lIC.receive();
                int code = -2;
                try {
                    code = msgIn.containsString() ? Integer.parseInt(msgIn.extractString()) : -1;
                } catch (Exception e) {
                    //
                }
                LinkMessage msgOut = LinkMessage.newStringMessage(Integer.toString(code));
                try {
                    lOC.send(msgOut);
                } catch (InterruptedIOException ex) {
                    // ignore for now. known bug in Link API
                }

                lIC.close();
                lOC.close();
            } catch (ClosedLinkException ex) {
                ex.printStackTrace();
            } catch (InterruptedIOException ex) {
                ex.printStackTrace();
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }

    void testLocalPositive1() throws IOException, IsolateStartupException {
        PipeServiceProtocol.registerService(token);

        Isolate one = new Isolate("com.sun.midp.io.j2me.pipe.TestPipeServiceProtocol$TestIsolate", null);
        one.setAPIAccess(true);


        PipeServiceProtocol server = PipeServiceProtocol.getService(token);
        server.bindServer("SERVER", "1.0");

        one.start();
        Link[] oneLinks = 
                SystemServiceLinkPortal.establishLinksFor(one, token);
        LinkPortal.setLinks(one, oneLinks);
        
        PipeServiceProtocol serverClient = server.acceptByServer();
        
        Link lIS = serverClient.getInboundLink();
        Link lOS = serverClient.getOutboundLink();
        
        LinkMessage msgOut = LinkMessage.newStringMessage(Integer.toString(101));
        
        lOS.send(msgOut);
        
        LinkMessage msgIn = lIS.receive();
        
        assertTrue(msgIn.containsString());
        assertEquals(Integer.parseInt(msgIn.extractString()), 101);
        

        lIS.close();
        lOS.close();

        one.waitForExit();

        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }
    
    void testAcceptClose() throws IOException {
        PipeServiceProtocol.registerService(token);
        
        final PipeServiceProtocol server = PipeServiceProtocol.getService(token);
        server.bindServer("SERVER", "1.0");
        
        new Thread(new Runnable() {

            public void run() {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ex) {
                    ex.printStackTrace();
                    fail(ex.toString());
                }
                try {
                    server.closeServer();
                } catch (IOException ex) {
                    ex.printStackTrace();
                    fail(ex.toString());
                }
            }
            
        }).start();
        // wait here. well, if server.closeServer() above fails
        //  to succeed we'll wait here forever. one might want to
        //  create new isolate to be able to continue testing without
        //  need for user intervention
        boolean interrupted = false;
        try {
            PipeServiceProtocol serverClient = server.acceptByServer();
        } catch (IOException iOException) {
            interrupted = iOException instanceof InterruptedIOException;
            if (!interrupted)
                iOException.printStackTrace();
        }
        assertTrue("acceptByServer has not been interrupted", interrupted);
        
        SystemServiceManager manager = SystemServiceManager.getInstance(token);
        manager.shutdown();
    }

    public void runTests() throws Throwable {
        declare("testLocalPositive1");
        testLocalPositive1();
        
        declare("testAcceptClose");
        testAcceptClose();
    }
}
