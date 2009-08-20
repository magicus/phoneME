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

package com.sun.midp.ams;

import com.sun.midp.ams.MidletInfo;
import com.sun.midp.main.RuntimeInfo;

public interface MidpMidletManager {
    public boolean startMidlet(int suiteId, String className);

    public boolean stopMidlet(int suiteId, String className);

    public boolean pauseMidlet(int suiteId, String className);

    public boolean setForegroundMidlet(int suiteId, String className);

    public MidletInfo getForegroundMidlet();

    public boolean setPriority(int suiteId, String className, int priority);

    public RuntimeInfo getMidletInfo(int suiteId, String className);

    public MidletInfo[] getMidletList();

}
