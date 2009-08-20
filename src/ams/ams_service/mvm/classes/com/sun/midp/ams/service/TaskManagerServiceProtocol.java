/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.ams.service;

import com.sun.midp.services.*;
import java.io.*;
import com.sun.midp.security.*;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Service side of TaskManager service data exchange protocol.
 */
final class TaskManagerServiceProtocol implements Runnable, SystemServiceConnectionListener {
    public static final int GET_TASK_LIST = 0;
    public static final int GET_RUNTIME_INFO = 1;
    public static final int START = 2;
    public static final int STOP = 3;
    public static final int PAUSE = 4;
    public static final int SET_FOREGROUND = 5;
    public static final int GET_FOREGROUND = 6;
    public static final int SET_PRIORITY = 7;
    public static final int SUBSCRIBE = 8;
    public static final int UNSUBSCRIBE = 9;

    /** Connection between service and client */
    private SystemServiceConnection con;

    /** Public AMS API */
    private TaskManagerHelper hepler;

    /**
     * Constructor.
     *
     * @param theConnection connection between service and client
     */
    private TaskManagerServiceProtocol(SecirutyToken t,
                                       SystemServiceConnection theConnection) {
        hepler = new TaskManagerHelper(t);
        con = theConnection;
    }

    public static void process(SecirutyToken t, 
                               SystemServiceConnection theConnection) {
        new Thread(new TaskManagerPeer(t, theConnection)).start();
    }

    public void run() {
        SystemServiceDataMessage msg = (SystemServiceDataMessage)con.receive();
        response(msg);
    }

    public void onMessage(SystemServiceDataMessage msg) {
        response(msg);
    }

    public void onConnectionClosed() {
        // will cause GC to collect this object
        con.setConnectionListener(null);
    }

    private void response(SystemServiceDataMessage msg) {
        int cmd;
        try {
            while(true) {
                DataInput in = msg.getDataInput();
                cmd = in.readInt();
                /* response message */
                msg = new SystemServiceDataMessage();
                switch (cmd) {
                case GET_RUNTIME_INFO:
                    int suiteId = in.readInt();
                    String mdiletName = in.readUTF();
                    hepler.getRuntimeInfo(int suiteId, String midletName);
                    break;
                case START:
                    int suiteId = in.readInt();
                    String mdiletName = in.readUTF();
                    hepler.startMidlet(int suiteId, String midletName);
                    break;
                case STOP:
                    int suiteId = in.readInt();
                    String mdiletName = in.readUTF();
                    hepler.stopMidlet(int suiteId, String midletName);
                    break;
                case PAUSE:
                    int suiteId = in.readInt();
                    String mdiletName = in.readUTF();
                    hepler.pauseMidlet(int suiteId, String midletName);
                    break;
                case SET_FOREGROUND:
                    int suiteId = in.readInt();
                    String mdiletName = in.readUTF();
                    hepler.setForeground(int suiteId, String midletName);
                    break;
                case GET_FOREGROUND:
                    break;
                case SET_PRIORITY:
                    hepler.setPriority(int suiteId, String midletName);
                    break;
                case GET_TASK_LIST:
                    TaskInfo[] taskList = hepler.getTaskList();
                    /* Send OK */
                    msg.getDataOutput().writeInt(OK);
                    msg.getDataOutput().writeInt(taskList.length);
                    if (null != taskList) {
                        for (int i = 0; i < taskList.length; i++) {
                            msg.getDataOutput().writeInt(taskList[i].suiteID);
                            msg.getDataOutput().writeUTF(taskList[i].className);
                        }
                    }
                    break;
                case SUBSCRIBE:
                case UNSUBSCRIBE:
                    msg.getDataOutput().writeInt(ERROR_UNSUPPORT);
                    break;
                }
                /* send response message */
                con.send(msg);
                /* don't waste memory by unused threads,
                   keep reference to this object inside connection object */
                con.setConnectionListener(this);
            }
        } catch (SystemServiceConnectionClosedException sscce) {
            Logging.report(Logging.CRITICAL, LogChannels.LC_CORE,
                "Connection to TaskManager service client closed unexpectedly");
        } catch (IOException ioe) {
            Logging.report(Logging.CRITICAL, LogChannels.LC_CORE,
                "TaskManager service exception: " + ex.getMessage());
        } finally {
        }
    }

    /* ----------------------------- client part ------------------------------- */
    public static TaskInfo[] getTaskList(SystemServiceConnection con) 
    throws IOException {
        SystemServiceDataMessage msg = new SystemServiceDataMessage();
        TaskInfo[] result = null;
        try {
            msg.getDataOutput().writeInt(GET_TASK_LIST);
            con.send(msg);
            msg = con.receive();
            DataInput in = msg.getDataInput();
            if (in.readInt() == ERROR_OK) {
                int size = in.readInt();
                if (size > 0) {
                    result = TaskInfo[size];
                    for (int i = 0; i < size; i++) {
                        result[i] = new TaskInfo(in.readInt(),
                                                 in.readUTF());
                    }
                }
            }
        } catch (SystemServiceConnectionClosedException sscce) {
        } catch (IOException ioe) {
        }
        return result;
    }
}
