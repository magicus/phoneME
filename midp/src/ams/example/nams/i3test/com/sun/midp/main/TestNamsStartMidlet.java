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

package com.sun.midp.main;

import java.lang.Thread;

import com.sun.midp.i3test.NamsTestCase;

public class TestNamsStartMidlet extends NamsTestCase {
    
    static final int WAIT_SEC = 10;
    static final int WAIT_MS = 1000 * WAIT_SEC;
    // was 3, but problems with starting new isolate for 3rd dummy
    static final int NUMBER = 2;
    
    int dummyId[];
    
    public TestNamsStartMidlet() {
        dummyId = new int[3];
    }
    
    boolean delay() {
        boolean result = true;
        try {
            Thread.currentThread().sleep(WAIT_MS);
        } catch (InterruptedException e) {
            result = false;
        };
        return result;
    }
    
    void runTestDisplay() {
        boolean wait_status;
        int i, j;
        
        declare("test FG/BG Display");
        
        for (i = 0; i < NUMBER; ++i) {
            dummyId[i] = NamsManager.startInternalMidlet(
                    "com.sun.midp.main.DummyNamsMIDlet" + (i+1), true, true);
            wait_status = namsNotifier.waitForState(
                    dummyId[i], NamsStorage.NAMS_STATE_PAUSED, WAIT_MS); 
            assertTrue(" Wait interrupted !", wait_status);
        }
        
        for (i = 0; i < NUMBER; ++i) {
            NamsManager.midletResume(dummyId[i]);
            wait_status = namsNotifier.waitForState(
                    dummyId[i], NamsStorage.NAMS_STATE_ACTIVE, WAIT_MS); 
            assertTrue(" Wait interrupted !", wait_status);
            
            NamsManager.midletSetForeground(dummyId[i]);
            wait_status = namsNotifier.waitForFG(dummyId[i], WAIT_MS); 
            assertTrue(" Wait interrupted !", wait_status);

            for (j = 0; j < NUMBER; ++j) {
                if (i == j) {
                    assertTrue("#" + (i+1) + " Dummy" + (j+1) + " in FG", 
                            NamsStorage.getDisplayStatus(dummyId[j]));
                    assertFalse("#" + (i+1) + " Dummy" + (j+1) + " state !", 
                        (NamsStorage.getMIDletState(dummyId[j]) == 
                            NamsStorage.NAMS_STATE_PAUSED));
                }
                else
                    assertFalse("#" + (i+1) + " Dummy"+ (j+1) + " in BG", 
                            NamsStorage.getDisplayStatus(dummyId[j]));
            }
        }
        
        delay();

        for (i = 0; i < NUMBER; ++i) {
            NamsManager.midletPause(dummyId[i]);
            wait_status = namsNotifier.waitForState(
                    dummyId[i], NamsStorage.NAMS_STATE_PAUSED, WAIT_MS); 
            assertTrue(" Wait interrupted !", wait_status);
        }
        for (i = 0; i < NUMBER; ++i) {
            assertFalse("#Paused Dummy" + (i+1) + " in BG !", 
                    NamsStorage.getDisplayStatus(dummyId[i]));
            assertTrue("#Paused Dummy" + (i+1) + " state !", 
                    (NamsStorage.getMIDletState(dummyId[i]) == 
                    NamsStorage.NAMS_STATE_PAUSED));
        }
        
        delay();
        
        for (i = 0; i < NUMBER; ++i) {
            NamsManager.midletDestroy(dummyId[i]);
            wait_status = namsNotifier.waitForState(
                    dummyId[i], NamsStorage.NAMS_STATE_DESTROYED, WAIT_MS); 
            assertTrue(" Wait interrupted !", wait_status);
        }
        for (i = 0; i < NUMBER; ++i) {
            assertFalse("#Destroyed Dummy" + (i+1) + " in BG !", 
                    NamsStorage.getDisplayStatus(dummyId[i]));
            assertTrue("#Destroyed Dummy" + (i+1) + " state !", 
                    (NamsStorage.getMIDletState(dummyId[i]) == 
                    NamsStorage.NAMS_STATE_DESTROYED));
        }

        delay();
    }

    
    public void runTests() {
        runTestDisplay();
    }

}
