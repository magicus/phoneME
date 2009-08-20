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

package com.sun.ams;
import com.sun.j2me.security.AccessController;
import com.sun.midp.ams.*;
import com.sun.midp.main.RuntimeInfo;
import com.sun.midp.security.Permissions;


/** TODO: make singletone */
class TaskManagerHelperImpl {

    MidpTaskManagerInterface helper;

    public TaskManagerHelperImpl() {
        if (VMUtils.getIsolateId() == VMUtils.getAmsIsolateId()) {
            helper = new MidpTaskManagerHelper();
        } else {
            /* performs security check */
            helper = new MidpTaskManagerClient();
        }
    }

    public boolean startTask(TaskInfo task) {
        return helper.startTask(task.suiteID, task.className);
    }

    public boolean stopTask(TaskInfo task) {
        return helper.stopTask(task.suiteID, task.className);
    }

    public boolean pauseTask(TaskInfo task) {
        return helper.pauseTask(task.suiteID, task.className);
    }

    public boolean setForegroundTask(TaskInfo task) {
        return helper.setForegroundTask(task.suiteID, task.className);
    }

    public TaskInfo getForegroundTask(){
        MidletInfo info = helper.getForegroundTask();
        return new TaskInfoImpl(info.suiteID, info.className);
    }

    public boolean setPriority(TaskInfo task, int priority) {
        return helper.setPriority(task.suiteID, task.className, int priority);
    }

    public TaskInfo[] getTaskList() {
        TaskInfoImpl[] result = null;
        MidletInfo[] tasks = helper.getMidletList();
        if (null != tasks && tasks.length > 0) {
            result = new TaskInfoImpl[tasks.length];
            for (int i = 0; i < tasks.length; i++) {
                result[i] = new TaskInfoImpl(tasks[i], this);
            }
        }
        return result;
    }
}