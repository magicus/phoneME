/*
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

package com.sun.mmedia;

import java.io.IOException;
import javax.microedition.media.MediaException;

class AsyncExecutor {

    private boolean isBlockedUntilEvent = false;
    private int asyncExecResult = 0;
    private int asyncExecOutputParam = 0;

    synchronized boolean runAsync(Task task) throws MediaException {
        boolean res = task.run();
        if( res ) {
            isBlockedUntilEvent = true;
            while (isBlockedUntilEvent) {
                try {
                    wait();
                } catch (InterruptedException ex) {
                }
            }
            //System.out.println( "HighLevelPlayer: runAsync() unblocked");
        } else {
            //System.out.println( "HighLevelPlayer: runAsync() didn't block" );
        }
        return res;
    }

    synchronized boolean runAsync( TaskWithIO task ) throws MediaException, IOException {
        boolean res = task.run();
        if( res ) {
            isBlockedUntilEvent = true;
            while( isBlockedUntilEvent ) {
                try {
                    wait();
                } catch( InterruptedException ex ) {}
            }
        }
        return res;
    }

    int getResult() {
        return asyncExecResult;
    }

    int getOutputParam() {
        return asyncExecOutputParam;
    }

    synchronized void unblockOnEvent(int result, int outputParam) {
        isBlockedUntilEvent = false;
        asyncExecResult = result;
        asyncExecOutputParam = outputParam;
        notify();
    }

    static interface Task {

        public boolean run() throws MediaException;
    }

    static interface TaskWithIO {
        public boolean run() throws IOException, MediaException;
    }
}
