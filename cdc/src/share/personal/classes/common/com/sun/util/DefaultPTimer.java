/*
 * @(#)DefaultPTimer.java	1.11 06/10/10
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

package com.sun.util;

import java.util.Timer;
import java.util.TimerTask;
import java.util.Date;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.LinkedList;
import java.util.Iterator;

/** An implementaion of the PTimer class which uses the java.util.Timer class
 to implement scheduling. PTimer is now deprecated and so it is preferrable to
 use java.util.Timer instead. We also manage a saving of about 900 bytes in the
 process!
*/

final class DefaultPTimer extends PTimer {
    public void schedule(PTimerSpec spec) {
        // Create a TimerTask to notify the listeners of the
        // spec when it is run by the timer
		
        TimerTask timerTask = new SpecTimerTask(spec, this);
        synchronized (this) {
            // Store a map of specs to tasks so that we can deschedule
            // the tasks.
			
            Object tasks = specToTasksMap.get(spec);
            if (tasks == null) {
                // Haven't scheduled a task before so just store it directly
                // in the Map. We don't create a List yet because it is unlikely
                // that it will ever be required.
				
                specToTasksMap.put(spec, timerTask);
            } else if (tasks instanceof TimerTask) {
                // One TimerTask already exists for this spec so we need to create a list
                // to store the existing TimerTask and the new one.
				
                List taskList = new LinkedList();
                taskList.add(tasks);
                taskList.add(timerTask);
                specToTasksMap.put(spec, taskList);
            } else {
                // Already scheduled more than one task for this PTimerSpec
                // so add the new task to the list.
				
                synchronized (tasks) {
                    ((List) tasks).add(timerTask);
                }
            }
        }
        // Schedule the TimerTask on the Timer.
		
        long time = spec.getTime();
        if (spec.isAbsolute())
            timer.schedule(timerTask, new Date(time));
        else {
            if (spec.isRepeat()) {
                if (spec.isRegular())
                    timer.scheduleAtFixedRate(timerTask, time, time);
                else timer.schedule(timerTask, time, time);
            } else timer.schedule(timerTask, time);
        }
    }
	
    public PTimerSpec scheduleTimerSpec(PTimerSpec t) {
        // It is more efficient to call it this way than from the deprecated method as we
        // would have to catch an exception that is never thrown.
		
        schedule(t);
        return t;
    }
	
    public void deschedule(PTimerSpec t) {
        Object tasks = null;
        synchronized (this) {
            tasks = specToTasksMap.get(t);
            // No tasks to cancel
		
            if (tasks == null)
                return;
            specToTasksMap.remove(t);
        }
        // A List of tasks to cancel
		
        if (tasks instanceof List) {
            Iterator i = ((List) tasks).iterator();
            synchronized (tasks) {
                while (i.hasNext())
                    ((TimerTask) i.next()).cancel();
            }
        } // Just one task to cancel
        else ((TimerTask) tasks).cancel();
    }
	
    public long getMinRepeatInterval() {
        return (long) -1;
    }
	
    public long getGranularity() {
        return (long) -1;
    }
    /** The Timer used to implement the scheduling for this PTimer. */
	
    private Timer timer = new Timer();
    /** A Map which maps PTimerSpecs to the TimerTasks which have been scheduled on the
     Timer. The key is a PTimerSpec and the value is either a List of TimerTasks
     running for that spec or just a single TimerTask object. This prevents creating
     a List when the majority of the time a PTimerSpec is only going to be scheduled
     once with a PTimer. */
	
    private Map specToTasksMap = new HashMap();
}

/** TimerTask which will notify the listeners of the corresponding PTimerSpec
 when it is run by the Timer. */
	
class SpecTimerTask extends TimerTask {
    /** Creates a new SpecTimerTask which will notify the listeners of the
     PTimerSpec. The PTimer supplied will be the source of events fired
     to the listeners. */
	
    public SpecTimerTask(PTimerSpec spec, PTimer timer) {
        this.spec = spec;
        this.timer = timer;
    }
	
    public void run() {
        spec.notifyListeners(timer);
    }
    /** The PTimerSpec to notify listeners for. */
	
    private PTimerSpec spec;
    /** The PTimer that created this task and is the source for PTimerWentOffEvents. */
	
    private PTimer timer;
}
