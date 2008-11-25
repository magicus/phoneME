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

package com.sun.midp.installer;

import com.sun.midp.services.*;
import java.io.*;

class AutoTesterService implements SystemService, Runnable  {
    public final static String SERVICE_ID = "AUTOTEST";

    private SystemServiceConnection con = null;    

    private String url;

    private String domain;

    private int loopCount;

    public String getServiceID() {
        return SERVICE_ID;
    }

    public void start() {
    }

    public void stop() {
    }

    public void acceptConnection(SystemServiceConnection inp_con) {
        if (con != null) {
            return;
        }

        con = inp_con;

        new Thread(this).start();
    }

    public void run() {
        String response = "";

        receiveTestRunParams();
        AutoTesterHelper helper = new AutoTesterHelper(url, domain, loopCount);

        try {
            helper.installAndPerformTests();
        } catch (Throwable t) {
            int suiteId = helper.getTestSuiteId();
            response = helper.getInstallerExceptionMessage(suiteId, t);
        }

        sendResponse(response);
    }

    private void receiveTestRunParams() {
        SystemServiceDataMessage msg = null;
        try {
            msg = (SystemServiceDataMessage)con.receive();
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }

        if (msg == null) {
            return;
        }


        try {
            url = msg.getDataInput().readUTF();
            if (url.length() == 0) {
                url = null;
            }

            domain = msg.getDataInput().readUTF();
            if (domain.length() == 0) {
                domain = null;
            }

            loopCount =  msg.getDataInput().readInt();
        } catch (IOException ex) {
            // ignore
        }
    }

    private void sendResponse(String response) {
        SystemServiceDataMessage msg = SystemServiceMessage.newDataMessage();

        try {
            msg.getDataOutput().writeUTF(response);
        } catch (IOException ex) {
            // ignore
        }

        try {
            con.send(msg);
        } catch (SystemServiceConnectionClosedException ex) {
            // ignore
        }
    }
}
